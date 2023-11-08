//
// Created by Šári Dvořáková on 02.11.2023.
//

#ifndef SP_ZOS_CPP_DIRECTORY_H
#define SP_ZOS_CPP_DIRECTORY_H

#define FILENAME_LENGTH 12
#define DATA_BLOCK_SIZE_B 1000

#include <cinttypes>

typedef struct dir_item {
    int32_t inode;                   // inode odpovídající souboru
    char name[FILENAME_LENGTH];              //8+3 + /0 C/C++ ukoncovaci string znak
    dir_item *next;
} directory_item;

typedef struct dir {
    directory_item *files;
    directory_item *subdirectories;
    dir *parent;
    directory_item *this_item;
} directory;

#define MAX_DIRECTORY_ITEMS DATA_BLOCK_SIZE_B/sizeof(directory_item)

directory *create_directory(directory_item *files, directory_item *subdirectories, directory *parent, directory_item *this_item);
directory_item *create_directory_item(int32_t inode, char *name, directory_item *next);
int count_directory_contents(directory *dir);

#endif //SP_ZOS_CPP_DIRECTORY_H
