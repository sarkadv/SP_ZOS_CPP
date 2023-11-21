/*
 * Bitmapa pro zjisteni dostupnosti i-uzlu nebo datovych bloku.
 * Jednotlive bity jsou ukladany do 32 bitoveho integeru.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitmap.h"

/*
 * Vytvoreni bitmapu o velikosti size_b bitu.
 * Tato hodnota je prevedena na byty.
 */
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
    new_bitmap->array = (int32_t *)calloc(1, size_B);
    new_bitmap->array_size_B = size_B;
    new_bitmap->array_size_b = size_b;

    return new_bitmap;
}

/*
 * Nastavi bit v bitmap na indexu bit_index na 1.
 */
int set_bit(bitmap *bitmap, int32_t bit_index) {
    if (!bitmap) {
        return 0;
    }

    if (bit_index < 0 || bit_index > bitmap->array_size_b - 1) {
        return 0;
    }

    int32_t flag = 1;
    int32_t index = bit_index / 8;
    int32_t index_position = bit_index % 32;

    flag = flag << index_position;
    bitmap->array[index] = bitmap->array[index] | flag;     // logicky soucet

    return 1;
}

/*
 * Nastavi bit v bitmap na indexu bit_index na 0.
 */
int clear_bit(bitmap *bitmap, int32_t bit_index) {
    if (!bitmap) {
        return 0;
    }

    if (bit_index < 0 || bit_index > bitmap->array_size_b - 1) {
        return 0;
    }

    int32_t flag = 1;
    int32_t index = bit_index / 8;
    int32_t index_position = bit_index % 32;

    flag = flag << index_position;
    bitmap->array[index] = bitmap->array[index] & (~flag);     // logicky soucin a negace

    return 1;
}

/*
 * Vrati hodnotu true nebo false podle toho, zda je bit na indexu bit_index v bitmap 1 nebo 0.
 */
bool get_bit(bitmap *bitmap, int32_t bit_index) {
    bool result;

    if (!bitmap) {
        return 0;
    }

    if (bit_index < 0 || bit_index > bitmap->array_size_b - 1) {
        return 0;
    }

    int32_t flag = 1;
    int32_t index = bit_index / 8;
    int32_t index_position = bit_index % 32;

    flag = flag << index_position;
    result = (bitmap->array[index] & flag) != 0;    // logicky soucin
    return result;
}

/*
 * Uvolni strukturu bitmap.
 */
void free_bitmap(bitmap *bitmap) {
    if (!bitmap) {
        return;
    }

    free(bitmap->array);
    free(bitmap);
}
