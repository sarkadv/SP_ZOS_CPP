//
// Created by Šári Dvořáková on 02.11.2023.
//

#ifndef SP_ZOS_CPP_INODE_H
#define SP_ZOS_CPP_INODE_H

#include <cinttypes>

#define INODE_COUNT 1000
#define DIRECT_REFERENCES_COUNT 5

const int32_t ID_ITEM_FREE = 0;

typedef struct {
    int32_t nodeid;               //ID i-uzlu, pokud ID = ID_ITEM_FREE, je polozka volna
    bool isDirectory;               //soubor, nebo adresar
    int8_t references;              //počet odkazů na i-uzel, používá se pro hardlinky
    int32_t file_size;              //velikost souboru v bytech
    int32_t direct1;                // 1. přímý odkaz na datové bloky
    int32_t direct2;                // 2. přímý odkaz na datové bloky
    int32_t direct3;                // 3. přímý odkaz na datové bloky
    int32_t direct4;                // 4. přímý odkaz na datové bloky
    int32_t direct5;                // 5. přímý odkaz na datové bloky
    int32_t indirect1;              // 1. nepřímý odkaz (odkaz - datové bloky)
    int32_t indirect2;              // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
} inode;

inode *create_inode(int32_t nodeid);

#endif //SP_ZOS_CPP_INODE_H
