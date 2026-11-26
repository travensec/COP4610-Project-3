#include "fat.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//number of bytes in a cluster
uint32_t cluster_size(FAT32 *fs)
{
    return fs->bpb.BPB_BytsPerSec * fs->bpb.BPB_SecPerClus;
}

//byte offset in the image where cluster N begins
long cluster_offset(FAT32 *fs, uint32_t cluster)
{
    // FAT32 has no fixed root directory region, so the data region starts right after the FATs
    uint32_t first_data_sector = fs->bpb.BPB_RsvdSecCnt
        + fs->bpb.BPB_NumFATs * fs->bpb.BPB_FATSz32;
    uint32_t sector = first_data_sector + (cluster - 2) * fs->bpb.BPB_SecPerClus;
    return (long)sector * fs->bpb.BPB_BytsPerSec;
}

//looking up cluster N in the first FAT and returns the next cluster in the chain
uint32_t get_next_cluster(FAT32 *fs, uint32_t cluster)
{
    long fat_offset = (long)fs->bpb.BPB_RsvdSecCnt * fs->bpb.BPB_BytsPerSec + cluster * 4;
    uint32_t next = 0;

    fseek(fs->image, fat_offset, SEEK_SET);
    if (fread(&next, sizeof(next), 1, fs->image) != 1)
        return FAT_EOC;  //failed read = end of the chain
    return next & FAT_MASK;
}

bool is_end_of_chain(uint32_t cluster)
{
    return cluster < 2 || cluster >= FAT_EOC;
}

//high word and low word stored separately
uint32_t entry_first_cluster(const DirEntry *entry)
{
    return ((uint32_t)entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
}

// Turns an 11-byte padded name ("RED        ") into a string ("RED").
// out must hold at least 13 chars (8 name + '.' + 3 ext + '\0').
void format_name(const DirEntry *entry, char *out)
{
    int len = 0;

    for (int i = 0; i < 8 && entry->DIR_Name[i] != ' '; i++)
        out[len++] = entry->DIR_Name[i];

    if (entry->DIR_Name[8] != ' ') {  // only add a dot if there is an extension
        out[len++] = '.';
        for (int i = 8; i < 11 && entry->DIR_Name[i] != ' '; i++)
            out[len++] = entry->DIR_Name[i];
    }
    out[len] = '\0';
}

// Reads every in-use short-name entry of a directory, following its whole cluster chain.
// Skips deleted, long-name and volume-label entries.
// Returns a malloc'd array (caller must free) and stores its length in *count.
DirRecord *read_directory(FAT32 *fs, uint32_t dir_cluster, size_t *count)
{
    size_t capacity = 16;
    DirRecord *records = malloc(capacity * sizeof(DirRecord));
    uint32_t entries_per_cluster = cluster_size(fs) / sizeof(DirEntry);
    bool done = false;

    *count = 0;
    if (dir_cluster == 0)  // a ".." entry uses cluster 0 to mean the root directory
        dir_cluster = fs->bpb.BPB_RootClus;

    for (uint32_t cluster = dir_cluster; !done && !is_end_of_chain(cluster);
         cluster = get_next_cluster(fs, cluster)) {
        long base = cluster_offset(fs, cluster);

        for (uint32_t i = 0; i < entries_per_cluster && !done; i++) {
            DirEntry entry;
            long offset = base + (long)(i * sizeof(DirEntry));

            fseek(fs->image, offset, SEEK_SET);
            if (fread(&entry, sizeof(entry), 1, fs->image) != 1
                || entry.DIR_Name[0] == ENTRY_END) {
                done = true;  // nothing valid after this point
                continue;
            }
            if (entry.DIR_Name[0] == ENTRY_FREE)
                continue;
            if ((entry.DIR_Attr & ATTR_LONG_NAME) == ATTR_LONG_NAME)
                continue;
            if (entry.DIR_Attr & ATTR_VOLUME_ID)
                continue;

            if (*count == capacity) {
                capacity *= 2;
                records = realloc(records, capacity * sizeof(DirRecord));
            }
            records[*count].entry = entry;
            records[*count].offset = offset;
            (*count)++;
        }
    }
    return records;
}

// Searches a directory for an entry called name (case-insensitive).
// On success copies the entry into *out and its image offset into *offset_out
// (either pointer may be NULL) and returns true.
bool find_entry(FAT32 *fs, uint32_t dir_cluster, const char *name,
                DirEntry *out, long *offset_out)
{
    char wanted[13];
    char entry_name[13];
    size_t len = strlen(name);
    size_t count;
    bool found = false;

    if (len == 0 || len > 12)
        return false;
    for (size_t i = 0; i <= len; i++)  // names are stored in uppercase
        wanted[i] = toupper((unsigned char)name[i]);

    DirRecord *records = read_directory(fs, dir_cluster, &count);
    for (size_t i = 0; i < count && !found; i++) {
        format_name(&records[i].entry, entry_name);
        if (strcmp(entry_name, wanted) == 0) {
            found = true;
            if (out)
                *out = records[i].entry;
            if (offset_out)
                *offset_out = records[i].offset;
        }
    }
    free(records);
    return found;
}