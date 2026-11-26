#include "navigation.h"
#include "fat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ls: prints the name of every file and directory in the current working directory
void cmd_ls(FAT32 *fs)
{
    size_t count;
    char name[13];
    DirRecord *records = read_directory(fs, fs->current_dir_cluster, &count);

    for (size_t i = 0; i < count; i++) {
        format_name(&records[i].entry, name);
        printf("%s\n", name);
    }
    free(records);
}

// cd: moves the current working directory into DIRNAME ("." and ".." included)
void cmd_cd(FAT32 *fs, const char *name)
{
    DirEntry entry;
    char dir_name[13];
    bool at_root = (fs->current_dir_cluster == fs->bpb.BPB_RootClus);

    // The root directory has no "." or ".." entries; going up from root stays at root
    if (at_root && (strcmp(name, ".") == 0 || strcmp(name, "..") == 0))
        return;

    if (!find_entry(fs, fs->current_dir_cluster, name, &entry, NULL)) {
        printf("cd: %s: No such file or directory\n", name);
        return;
    }
    if (!(entry.DIR_Attr & ATTR_DIRECTORY)) {
        printf("cd: %s: Not a directory\n", name);
        return;
    }

    uint32_t target = entry_first_cluster(&entry);
    if (target == 0)  // ".." of a top-level directory points to cluster 0 = root
        target = fs->bpb.BPB_RootClus;

    if (strcmp(name, "..") == 0) {
        char *last_slash = strrchr(fs->cwd_path, '/');  // drop the last path part
        if (last_slash)
            *last_slash = '\0';
    } else if (strcmp(name, ".") != 0) {
        format_name(&entry, dir_name);  // use the name as stored (uppercase)
        if (strlen(fs->cwd_path) + strlen(dir_name) + 2 > sizeof(fs->cwd_path)) {
            printf("cd: path too long\n");
            return;
        }
        strcat(fs->cwd_path, "/");
        strcat(fs->cwd_path, dir_name);
    }
    fs->current_dir_cluster = target;
}