//
// Created by Šári Dvořáková on 02.11.2023.
//

#include <string.h>
#include <stdlib.h>
#include "directory.h"

directory *create_directory(directory_item *files, directory_item *subdirectories, directory *parent, directory_item *this_item) {
    directory *dir;

    dir = (directory*)malloc(sizeof(directory));

    dir->files = files;
    dir->subdirectories = subdirectories;
    dir->parent = parent;
    dir->this_item = this_item;

    return dir;
}

directory_item *create_directory_item(int32_t inode, char *name, directory_item *next) {
    directory_item *dir_item;

    if (inode <= 0 || !name) {
        return NULL;
    }

    dir_item = (directory_item*)malloc(sizeof(directory_item));
    dir_item->inode = inode;
    strncpy(dir_item->name, name, FILENAME_LENGTH);
    dir_item->next = next;

    return dir_item;
}
