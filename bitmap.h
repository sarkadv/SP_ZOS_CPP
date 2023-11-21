/*
 * Bitmapa pro zjisteni dostupnosti i-uzlu nebo datovych bloku.
 * Jednotlive bity jsou ukladany do 32 bitoveho integeru.
 */

#ifndef SP_ZOS_CPP_BITMAP_H
#define SP_ZOS_CPP_BITMAP_H


typedef struct {
    int32_t *array;         // pole bitu
    int32_t array_size_B;   // velikost v bytech
    int32_t array_size_b;   // velikost v bitech
} bitmap;

bitmap *create_bitmap(int32_t size_b);
void free_bitmap(bitmap *bitmap);
int set_bit(bitmap *bitmap, int32_t bit_index);
int clear_bit(bitmap *bitmap, int32_t bit_index);
bool get_bit(bitmap *bitmap, int32_t bit_index);

#endif //SP_ZOS_CPP_BITMAP_H
