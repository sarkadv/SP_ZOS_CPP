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

int write_data_to_data_block(data_block *block, char *data) {
    if (!block || !data) {
        return 0;
    }

    strncpy(block->data, data, DATA_BLOCK_SIZE_B);

    return 1;
}

int write_references_to_data_block(data_block *block, int32_t *references, int32_t references_count) {
    int i;

    if (!block || !references || references_count <= 0) {
        return 0;
    }

    memset(block->data, 0, DATA_BLOCK_SIZE_B);

    for (i = 0; i < references_count; i++) {
        memcpy(block->data + i*sizeof(int32_t), references + i, sizeof(int32_t));
    }

    return 1;
}

int data_block_empty(data_block *block) {
    int i;

    if (!block) {
        return 0;
    }

    for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
        if (block->data[i] != '\0') {
            return 0;
        }
    }

    return 1;
}
