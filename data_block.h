//
// Created by Šári Dvořáková on 02.11.2023.
//

#ifndef SP_ZOS_CPP_DATA_BLOCK_H
#define SP_ZOS_CPP_DATA_BLOCK_H

#define DATA_BLOCK_SIZE_B 1000

typedef struct {
    char data[DATA_BLOCK_SIZE_B];
} data_block;

data_block *create_data_block();

#endif //SP_ZOS_CPP_DATA_BLOCK_H
