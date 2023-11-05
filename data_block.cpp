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
