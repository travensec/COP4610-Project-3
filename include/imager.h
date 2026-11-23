#pragma once

#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// BIOS Parameter Block
typedef struct{
    uint8_t  BS_jmpBoot[3];
    uint8_t  BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t  BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t  BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t  BPB_Reserved[12];
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved1;
    uint8_t  BS_BootSig;
    uint32_t BS_VolID;
    uint8_t  BS_VolLab[11];
    uint8_t  BS_FilSysType[8];
} BPB;

// Define a struct to keep track of info
typedef struct {
    FILE* image;
    BPB bpb; // BIOS Parameter Block / Boot Sector Info

    uint32_t current_dir_cluster;
    uint64_t image_size;
} FAT32;

FAT32 * mount_image(char * input);
FAT32 * close_image(FAT32 * input);
char * get_image_name();
void print_info(FAT32 * input);