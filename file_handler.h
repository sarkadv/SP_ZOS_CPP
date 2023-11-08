//
// Created by Šári Dvořáková on 07.11.2023.
//

#ifndef SP_ZOS_CPP_FILE_HANDLER_H
#define SP_ZOS_CPP_FILE_HANDLER_H

#include "vfs.h"

int update_data_block(vfs *fs, int32_t block_index);
int update_bitmaps(vfs *fs);
int update_inode(vfs *fs, int32_t inode_index);

#endif //SP_ZOS_CPP_FILE_HANDLER_H
