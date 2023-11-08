//
// Created by Šári Dvořáková on 02.11.2023.
//

#ifndef SP_ZOS_CPP_SUPERBLOCK_H
#define SP_ZOS_CPP_SUPERBLOCK_H

#include <cinttypes>

#define SIGNATURE "saridv"
#define MIN_FS_SIZE_B 50000

typedef struct {
    char signature[12];              //login autora FS
    //char volume_descriptor[251];    //popis vygenerovaného FS
    int32_t disk_size;              //celkova velikost VFS
    int32_t data_block_size;           //velikost clusteru
    int32_t data_block_count;          //pocet clusteru
    int32_t bitmapi_start_address;  //adresa pocatku bitmapy i-uzlů
    int32_t bitmapd_start_address;   //adresa pocatku bitmapy datových bloků
    int32_t inode_start_address;    //adresa pocatku  i-uzlů
    int32_t data_start_address;     //adresa pocatku datovych bloku
} superblock;

superblock *create_superblock(char *signature, int32_t disk_size, int32_t cluster_size, int32_t cluster_count,
                              int32_t bitmapd_size_B, int32_t bitmapi_size_B);

#endif //SP_ZOS_CPP_SUPERBLOCK_H
