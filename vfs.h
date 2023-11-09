//
// Created by Šári Dvořáková on 22.10.2023.
//

#ifndef SP_ZOS_CPP_VFS_H
#define SP_ZOS_CPP_VFS_H

#include "superblock.h"
#include "bitmap.h"
#include "directory.h"
#include "inode.h"
#include "data_block.h"

typedef struct {
    bool loaded;
    char *name;
    superblock *superblock;
    bitmap *bitmapi;
    bitmap *bitmapd;
    directory *root_directory;
    directory *current_directory;
    directory *all_directories[INODE_COUNT + 1];    // na all_directories[i] je adresar, ktery odpovida inode = i
    inode *inodes[INODE_COUNT];
    data_block **data_blocks;
} vfs;

#endif //SP_ZOS_CPP_VFS_H
