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

int count_directory_contents(directory *dir) {
    int32_t sum = 0;
    directory_item *current_file;
    directory_item *current_subdirectory;

    if (!dir) {
        return 0;
    }

    current_file = dir->files;
    current_subdirectory = dir->subdirectories;

    while (current_file != NULL) {
        sum++;
        current_file = current_file->next;
    }

    while (current_subdirectory != NULL) {
        sum++;
        current_subdirectory = current_subdirectory->next;
    }

    return sum;
}
