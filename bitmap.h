//
// Created by Šári Dvořáková on 01.11.2023.
//

#ifndef SP_ZOS_CPP_BITMAP_H
#define SP_ZOS_CPP_BITMAP_H

typedef struct {
    char *array;
    int32_t array_size;
} bitmap;

bitmap *create_bitmap(int32_t size_b);
void free_bitmap(bitmap *bitmap);
int set_bit(bitmap *bitmap, int32_t bit_index);
int clear_bit(bitmap *bitmap, int32_t bit_index);
bool get_bit(bitmap *bitmap, int32_t bit_index);

#endif //SP_ZOS_CPP_BITMAP_H
