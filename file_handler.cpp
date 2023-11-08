//
// Created by Šári Dvořáková on 07.11.2023.
//

#include <stdio.h>
#include "file_handler.h"

int update_data_block(vfs *fs, int32_t block_index) {
    FILE* fout;

    if (!fs || block_index < 0) {
        return 0;
    }

    fout = fopen(fs->name, "rb+");

    if (!fout) {
        return 0;
    }

    fseek(fout, fs->superblock->data_start_address + block_index*sizeof(data_block), SEEK_SET);
    fwrite(fs->data_blocks[block_index], sizeof(data_block), 1, fout);

    fclose(fout);

    return 1;

}

int update_bitmaps(vfs *fs) {
    FILE* fout;

    if (!fs) {
        return 0;
    }

    fout = fopen(fs->name, "rb+");

    if (!fout) {
        return 0;
    }

    fseek(fout, fs->superblock->bitmapd_start_address, SEEK_SET);
    fwrite(fs->bitmapd->array, sizeof(fs->bitmapd->array_size_B), 1, fout);
    fwrite(fs->bitmapi->array, sizeof(fs->bitmapi->array_size_B), 1, fout);

    fclose(fout);

    return 1;
}

int update_inode(vfs *fs, int32_t inode_index) {
    FILE* fout;

    if (!fs || inode_index < 0) {
        return 0;
    }

    fout = fopen(fs->name, "rb+");

    if (!fout) {
        return 0;
    }

    fseek(fout, fs->superblock->inode_start_address + inode_index*sizeof(data_block), SEEK_SET);
    fwrite(fs->inodes[inode_index], sizeof(inode), 1, fout);

    fclose(fout);

    return 1;
}
