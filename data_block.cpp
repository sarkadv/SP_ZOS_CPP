//
// Created by Šári Dvořáková on 02.11.2023.
//

#include <stdlib.h>
#include <string.h>
#include "data_block.h"

data_block *create_data_block() {
    data_block *db = (data_block*)calloc(1, sizeof(data_block));
    memset(db->data, 0, sizeof(DATA_BLOCK_SIZE_B));

    return db;
}

int write_dir_items_to_data_block(data_block *block, directory_item *subdirectories, directory_item *files) {
    int32_t offset = 0;

    if (!block) {
        return 0;
    }

    memset(block->data, 0, DATA_BLOCK_SIZE_B);

    while (subdirectories != NULL) {
        memcpy(block->data + offset, subdirectories, sizeof(directory_item));

        offset += sizeof(directory_item);
        subdirectories = subdirectories->next;
    }

    while (files != NULL) {
        memcpy(block->data + offset, files, sizeof(directory_item));

        offset += sizeof(directory_item);
        files = files->next;
    }

    return 1;
}
