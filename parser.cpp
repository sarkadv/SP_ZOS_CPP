#include <string.h>
#include <stdlib.h>
#include "parser.h"

int parse_input(char *input, char *command, char *param1, char *param2) {
    char *delimiter = NULL;
    char *token = NULL;

    if (!input || !command || !param1 || !param2) {
        return 0;
    }

    input[strlen(input) - 1] = '\0';

    delimiter = (char*)INPUT_DELIMITER;
    token = strtok(input, delimiter);

    if (token) {
        strcpy(command, token);
    }
    else {
        command = NULL;
    }

    token = strtok(NULL, delimiter);

    if (token) {
        strcpy(param1, token);
    }
    else {
        param1 = NULL;
    }

    token = strtok(NULL, delimiter);

    if (token) {
        strcpy(param2, token);
    }
    else {
        param2 = NULL;
    }

    return 1;
}

directory *parse_path(vfs *fs, char *input, bool without_last_part) {
    directory *current_directory;
    directory_item *subdirectory;
    char *delimiter;
    char *token;
    bool found;
    char *input_copy;
    int32_t offset = 0;
    char* saveptr = NULL;

    if (!fs) {
        return NULL;
    }

    if (!input) {
        return fs->current_directory;
    }

    input_copy = (char*)calloc((strlen(input) + 2), sizeof(char));
    strncpy(input_copy, input, strlen(input));

    if (input_copy[0] == '/') {  // absolutni cesta
        current_directory = fs->root_directory;
        offset++;
    }
    else {  // relativni cesta
        current_directory = fs->current_directory;
    }

    input_copy[strlen(input)] = '/';
    input_copy[strlen(input) + 1] = '\0';

    delimiter = (char*)PATH_DELIMITER;

    token = strtok_r(input_copy, delimiter, &saveptr);

    if (token != NULL) {
        offset += strlen(token);
    }

    // pokud chceme parsovat i posledni cast cesty, staci ze token neni NULL
    // pokud nechceme parsovat posledni cast cesty, cyklus se zastavi kdyz cesta uz nebude obsahovat /
    while ((!without_last_part && token != NULL) || (without_last_part && token != NULL && input[offset] == '/')) {
        found = false;

        if (!strcmp(token, ".")) {  // aktualni adresar
            // nic nezmenime
        }
        else if (!strcmp(token, "..")) {    // nadrazeny adresar
            if (current_directory->parent != NULL) {
                current_directory = current_directory->parent;
            }
            else {  // neplatna cesta
                return NULL;
            }
        }
        else {  // najit podadresar
            subdirectory = current_directory->subdirectories;

            while (subdirectory != NULL) {
                if (!strcmp(token, subdirectory->name)) {
                    subdirectory = find_symlink_target_file(fs, subdirectory);
                    found = true;
                    current_directory = fs->all_directories[subdirectory->inode];
                    break;
                }
                subdirectory = subdirectory->next;
            }
            if (!found) {   // neplatna cesta
                return NULL;
            }
        }

        token = strtok_r(NULL, delimiter, &saveptr);
        if (token != NULL) {
            offset += strlen(token) + 1;
        }
    }

    free(input_copy);

    return current_directory;
}

char *get_last_part_of_path(vfs *fs, char *input) {
    char *result;
    char *last_found;
    int32_t last_found_index;

    if (!fs || !input) {
        return NULL;
    }

    result = (char*)calloc(FILENAME_LENGTH, sizeof(char));
    last_found = strrchr(input, '/');

    if (!last_found) {
        strncpy(result, input, FILENAME_LENGTH);
    }
    else {
        last_found_index = last_found - input + 1;
        strncpy(result, input + last_found_index, FILENAME_LENGTH);
    }

    return result;
}

directory_item *find_diritem_in_dir_by_name(directory *dir, char *name) {
    directory_item *subitem;

    subitem = dir->subdirectories;

    while (subitem != NULL) {
        if (!strcmp(name, subitem->name)) {
            return subitem;
        }
        subitem = subitem->next;
    }

    subitem = dir->files;

    while (subitem != NULL) {
        if (!strcmp(name, subitem->name)) {
            return subitem;
        }
        subitem = subitem->next;
    }

    return NULL;
}

directory_item *find_symlink_target_file(vfs *fs, directory_item *symlink_file) {
    directory_item *current_file;
    char *current_file_target;
    directory *target_file_parent_dir;
    char *target_file_name;

    if (!symlink_file) {
        return NULL;
    }

    if (!fs->inodes[symlink_file->inode - 1]->is_symlink) {
        return symlink_file;
    }

    current_file = symlink_file;

    while (1) {
        current_file_target = get_symlink_reference(fs->data_blocks[fs->inodes[current_file->inode - 1]->direct1 - 1]);
        target_file_parent_dir = parse_path(fs, current_file_target, true);

        if (!target_file_parent_dir) {
            free(current_file_target);

            if (target_file_name) {
                free(target_file_name);
            }
            return NULL;
        }

        target_file_name = get_last_part_of_path(fs, current_file_target);
        current_file = find_diritem_in_dir_by_name(target_file_parent_dir, target_file_name);

        if (!current_file) {
            free(current_file_target);

            if (target_file_name) {
                free(target_file_name);
            }
            return NULL;
        }

        if (!fs->inodes[current_file->inode - 1]->is_symlink) {
            free(current_file_target);
            free(target_file_name);
            return current_file;
        }
    }

}

