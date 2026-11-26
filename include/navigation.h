#pragma once

#include "imager.h"

void cmd_ls(FAT32 *fs);
void cmd_cd(FAT32 *fs, const char *name);