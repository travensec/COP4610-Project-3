#include "create.h"
#include "fat.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Sets the FAT entry for `cluster` to `value` 
 * The FAT is an array of 4-byte entries, one per cluster, stored right after the
 * reserved sectors. An entry holds the next cluster in a chain, 0 if the cluster is
 * free, or FAT_MASK if it is the last cluster of a chain.
 * The image keeps BPB_NumFATs identical copies of the FAT back to back, so every
 * change has to be written to each copy or the copies stop matching.
 */
void set_fat(FAT32 *fs, uint32_t cluster, uint32_t value)
{
    for (int i = 0; i < fs->bpb.BPB_NumFATs; i++) {
        long fat_start = (long)(fs->bpb.BPB_RsvdSecCnt + i * fs->bpb.BPB_FATSz32)
            * fs->bpb.BPB_BytsPerSec;
        fseek(fs->image, fat_start + cluster * 4, SEEK_SET);
        fwrite(&value, 4, 1, fs->image);
    }
}

/*
 * Claims an unused cluster and returns its number (0 if the image is full)
 * Valid data clusters are numbered 2 up to (number of data clusters + 1), so we
 * scan that range for the first one whose FAT entry is 0 (free). The cluster is
 * marked end-of-chain so it is no longer free, then zeroed so it holds no leftover
 * data (a new directory relies on this: a 0x00 byte means "no more entries").
 * Used by mkdir, by find_free_slot when a directory grows, and later by write.
 */
uint32_t alloc_cluster(FAT32 *fs)
{
    uint32_t data_sectors = fs->bpb.BPB_TotSec32 - fs->bpb.BPB_RsvdSecCnt
        - fs->bpb.BPB_NumFATs * fs->bpb.BPB_FATSz32;
    uint32_t last = data_sectors / fs->bpb.BPB_SecPerClus + 1;

    for (uint32_t c = 2; c <= last; c++) {
        if (get_next_cluster(fs, c) == 0) {
            char *zeros = calloc(cluster_size(fs), 1);
            set_fat(fs, c, FAT_MASK);
            fseek(fs->image, cluster_offset(fs, c), SEEK_SET);
            fwrite(zeros, cluster_size(fs), 1, fs->image);
            free(zeros);
            return c;
        }
    }
    return 0;
}

/*
 * Returns the byte offset in the image where a new 32-byte entry can be written
 * inside the directory starting at `dir_cluster`.
 *
 * A directory is a chain of clusters filled with 32-byte entries. We walk every
 * cluster of the chain and check each entry's first byte: 0x00 means the slot was
 * never used, 0xE5 means it held a deleted entry. Either one can be reused.
 * If every slot is taken, the directory is full: we allocate a new cluster, link
 * it to the end of the chain (the old last cluster now points to it in the FAT),
 * and return the first slot of that new cluster.
 */
long find_free_slot(FAT32 *fs, uint32_t dir_cluster)
{
    uint32_t last = dir_cluster;

    for (uint32_t c = dir_cluster; !is_end_of_chain(c); c = get_next_cluster(fs, c)) {
        for (uint32_t i = 0; i < cluster_size(fs); i += sizeof(DirEntry)) {
            uint8_t first = 0;
            fseek(fs->image, cluster_offset(fs, c) + i, SEEK_SET);
            fread(&first, 1, 1, fs->image);
            if (first == ENTRY_END || first == ENTRY_FREE)
                return cluster_offset(fs, c) + i;
        }
        last = c;
    }
    uint32_t new_cluster = alloc_cluster(fs);
    set_fat(fs, last, new_cluster);
    return cluster_offset(fs, new_cluster);
}

/*
 * Writes one directory entry at `offset`. The name is stored uppercase and padded
 * with spaces to 11 bytes ("foo" -> "FOO        "). FAT32 stores the first cluster
 * as two 16-bit halves (high and low). Dates, times and size are left at 0.
 */
static void write_entry(FAT32 *fs, long offset, const char *name, uint8_t attr,
                        uint32_t cluster)
{
    DirEntry entry;

    memset(&entry, 0, sizeof(entry));
    memset(entry.DIR_Name, ' ', 11);
    for (int i = 0; name[i]; i++)
        entry.DIR_Name[i] = toupper((unsigned char)name[i]);
    entry.DIR_Attr = attr;
    entry.DIR_FstClusHI = cluster >> 16;
    entry.DIR_FstClusLO = cluster & 0xFFFF;

    fseek(fs->image, offset, SEEK_SET);
    fwrite(&entry, sizeof(entry), 1, fs->image);
    fflush(fs->image);
}

// Prints an error and returns false if the name is too long, has a '.' or '/',
// or is already used by a file or directory in the current directory
static bool name_ok(FAT32 *fs, const char *cmd, const char *name)
{
    if (strlen(name) > 8 || strchr(name, '.') || strchr(name, '/')) {
        printf("%s: %s: invalid name\n", cmd, name);
        return false;
    }
    if (find_entry(fs, fs->current_dir_cluster, name, NULL, NULL)) {
        printf("%s: %s: already exists\n", cmd, name);
        return false;
    }
    return true;
}

/*
 * Creates an empty directory in the current directory.
 * 1. Find a free slot in the current directory for the new entry.
 * 2. Allocate a zeroed cluster to hold the new directory's contents.
 * 3. Write the two entries every directory starts with: "." (points to itself)
 *    and ".." (points to the parent; FAT uses cluster 0 when the parent is root).
 * 4. Write the new directory's entry into the parent last, so it only appears
 *    once the directory is fully set up.
 */
void cmd_mkdir(FAT32 *fs, const char *name)
{
    if (!name_ok(fs, "mkdir", name))
        return;

    long slot = find_free_slot(fs, fs->current_dir_cluster);
    uint32_t cluster = alloc_cluster(fs);
    uint32_t parent = fs->current_dir_cluster;
    if (parent == fs->bpb.BPB_RootClus)
        parent = 0;

    write_entry(fs, cluster_offset(fs, cluster), ".", ATTR_DIRECTORY, cluster);
    write_entry(fs, cluster_offset(fs, cluster) + 32, "..", ATTR_DIRECTORY, parent);
    write_entry(fs, slot, name, ATTR_DIRECTORY, cluster);
}

/*
 * creat FILENAME. creates an empty file in the current directory.
 * Only a directory entry is written (size 0, first cluster 0). No cluster is
 * allocated because the file has no data yet; write will allocate one later.
 */
void cmd_creat(FAT32 *fs, const char *name)
{
    if (name_ok(fs, "creat", name))
        write_entry(fs, find_free_slot(fs, fs->current_dir_cluster), name, ATTR_ARCHIVE, 0);
}