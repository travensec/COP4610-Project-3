#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "imager.h"

//more info on dir fields attributes etc are in the spec pdf and 
//fat slides on canvas; check more thoroughly later

//DIR_Attr bit flags from fat32 spec section 6
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME 0x0F  // READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID

//special values of DIR_Name[0]
#define ENTRY_END  0x00  //this and later entries unused
#define ENTRY_FREE 0xE5  //entry was deleted

// FAT32 entries only use the low 28 bits; values >= FAT_EOC mean end of chain
#define FAT_MASK 0x0FFFFFFF
#define FAT_EOC  0x0FFFFFF8

//short-name directory entry
typedef struct {
    uint8_t  DIR_Name[11];
    uint8_t  DIR_Attr;
    uint8_t  DIR_NTRes;
    uint8_t  DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((packed)) DirEntry;

//directory entry & where it lives in the image (later parts write back to it)
typedef struct {
    DirEntry entry;
    long offset;
} DirRecord;

uint32_t cluster_size(FAT32 *fs);

long cluster_offset(FAT32 *fs, uint32_t cluster);

uint32_t get_next_cluster(FAT32 *fs, uint32_t cluster);

bool is_end_of_chain(uint32_t cluster);

uint32_t entry_first_cluster(const DirEntry *entry);

void format_name(const DirEntry *entry, char *out);

DirRecord *read_directory(FAT32 *fs, uint32_t dir_cluster, size_t *count);

bool find_entry(FAT32 *fs, uint32_t dir_cluster, const char *name,
                DirEntry *out, long *offset_out);