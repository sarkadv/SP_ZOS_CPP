//
// Created by Šári Dvořáková on 01.11.2023.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitmap.h"

bitmap *create_bitmap(int32_t size_b) {
    bitmap *new_bitmap;

    if (size_b <= 0) {
        return NULL;
    }

    int32_t size_B = size_b/8;

    if (size_b % 8 != 0) {
        size_B++;
    }

    new_bitmap = (bitmap*)malloc(sizeof(bitmap));
    new_bitmap->array = (char*)calloc(size_B, 1);
    new_bitmap->array_size = size_B;

    return new_bitmap;
}

int set_bit(bitmap *bitmap, int32_t bit_index) {
    if (!bitmap) {
        return 0;
    }

    if (bit_index < 0 || bit_index > bitmap->array_size - 1) {
        return 0;
    }

    int8_t flag = 1;
    int8_t index = bit_index / 8;
    int8_t index_position = bit_index % 8;

    flag = flag << index_position;
    bitmap->array[index] = bitmap->array[index] | flag;     // logicky soucet

    return 1;
}

int clear_bit(bitmap *bitmap, int32_t bit_index) {
    if (!bitmap) {
        return 0;
    }

    if (bit_index < 0 || bit_index > bitmap->array_size - 1) {
        return 0;
    }

    int8_t flag = 1;
    int8_t index = bit_index / 8;
    int8_t index_position = bit_index % 8;

    flag = flag << index_position;
    bitmap->array[index] = bitmap->array[index] & (~flag);     // logicky soucin a negace

    return 1;
}

bool get_bit(bitmap *bitmap, int32_t bit_index) {
    if (!bitmap) {
        return 0;
    }

    if (bit_index < 0 || bit_index > bitmap->array_size - 1) {
        return 0;
    }

    int8_t flag = 1;
    int8_t index = bit_index / 8;
    int8_t index_position = bit_index % 8;

    flag = flag << index_position;
    return (bitmap->array[index] & flag) != 0;     // logicky soucin
}

void free_bitmap(bitmap *bitmap) {
    if (!bitmap) {
        return;
    }

    free(bitmap->array);
    free(bitmap);
}
