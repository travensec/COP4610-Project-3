#pragma once
#include "imager.h"

void set_fat(FAT32 *fs, uint32_t cluster, uint32_t value);
uint32_t alloc_cluster(FAT32 *fs);
long find_free_slot(FAT32 *fs, uint32_t dir_cluster);

void cmd_mkdir(FAT32 *fs, const char *name);
void cmd_creat(FAT32 *fs, const char *name);