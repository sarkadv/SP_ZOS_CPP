/*
 * Struktura pro virtualni file system, ktera se vytvori v pameti po nahrani FS z disku.
 */

#ifndef SP_ZOS_CPP_VFS_H
#define SP_ZOS_CPP_VFS_H

#include "superblock.h"
#include "bitmap.h"
#include "directory.h"
#include "inode.h"
#include "data_block.h"

typedef struct {
    bool loaded;    // po celem nacteni nastaveno na true
    char *name;
    superblock *superblock;
    bitmap *bitmapi;
    bitmap *bitmapd;
    directory *root_directory;      // korenovy adresar
    directory *current_directory;   // aktualni adresar
    directory *all_directories[INODE_COUNT + 1];    // na all_directories[i] je adresar, ktery odpovida inode = i
    inode *inodes[INODE_COUNT];
    data_block **data_blocks;       // vsechny data bloky
} vfs;

#endif //SP_ZOS_CPP_VFS_H
