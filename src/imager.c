#include "imager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Keep track of loaded image name, for shell
static char loaded_image_name[256] = {0};
//char cwd_name[256] = "/";

FAT32 * mount_image(char * input)
{
    FILE * image;
    if((image = fopen(input, "r+")) == NULL) {   // r+ to open file for read, write, and append
        printf("%s does not exist\n.", input);
        return NULL;
    }

    FAT32* fat_image = malloc(sizeof(FAT32));
    fat_image->image = image;

    // Saves the image name for shell prompt
    // Strip the input by delimiter / and acquire the last valid token for img name
    char * token = strtok(input, " / ");
    char * last_token = token;
    while (token)
    {
        token = strtok(NULL, " / ");
        if (token) last_token = token;
    }
    strncpy(loaded_image_name, last_token, sizeof(loaded_image_name)-1);
    token = NULL;
    last_token = NULL;

    // Reads the entire BPB
    fseek(image, 0, SEEK_SET);
    size_t bytes_read = fread(&fat_image->bpb, sizeof(BPB), 1, image);
    
    if (bytes_read != 1) {
        fprintf(stderr, "Error: Failed trying to read boot sector\n");
        fclose(image);
        image = NULL;
        return NULL;
    }
    
    // Initializes current directory to root using / (for shell output)
    fat_image->current_dir_cluster = fat_image->bpb.BPB_RootClus;
    fat_image->cwd_path[0] = '\0';

    return fat_image;
}

FAT32 * close_image(FAT32 * fat_image)
{
    if (fat_image->image)
    {
        fclose(fat_image->image);
        fat_image->image = NULL;
    }
    return fat_image;
}

char * get_image_name()
{
    return loaded_image_name;
}

void print_info(FAT32 * fat_image)
{
    printf("Root Cluster: %u\n", fat_image->bpb.BPB_RootClus);
    printf("Bytes Per Sector: %u\n", fat_image->bpb.BPB_BytsPerSec);
    printf("Sectors Per Cluster: %u\n", fat_image->bpb.BPB_SecPerClus);
    
    // Calculates total sectors used by FATs
    uint32_t fat_sectors = fat_image->bpb.BPB_NumFATs * fat_image->bpb.BPB_FATSz32;
    
    // Calculates data region sectors
    uint32_t data_sectors = fat_image->bpb.BPB_TotSec32 - 
        (fat_image->bpb.BPB_RsvdSecCnt + fat_sectors);
    
    // Calculates total clusters in data region
    uint32_t total_clusters = data_sectors / fat_image->bpb.BPB_SecPerClus;
    
    // Calculates entries per FAT
    uint32_t entries_per_fat = 
        (fat_image->bpb.BPB_FATSz32 * fat_image->bpb.BPB_BytsPerSec) / 4;
    
    printf("Total clusters in data region: %u\n", total_clusters);
    printf("Entries in one FAT: %u\n", entries_per_fat);
    
    // Size of the image in bytes
    fseek(fat_image->image, 0, SEEK_END);
    long size = ftell(fat_image->image);
    printf("Size of Image (bytes): %ld\n", size);
}