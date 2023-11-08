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

    if (!fs) {
        return NULL;
    }

    if (!input) {
        return fs->current_directory;
    }

    if (input[0] == '/') {  // absolutni cesta
        current_directory = fs->root_directory;
    }
    else {  // relativni cesta
        current_directory = fs->current_directory;
    }

    delimiter = (char*)PATH_DELIMITER;

    token = strtok(input, delimiter);

    // pokud chceme parsovat i posledni cast cesty, staci ze token neni NULL
    // pokud nechceme parsovat posledni cast cesty, cyklus se zastavi kdyz cesta uz nebude obsahovat /
    while ((!without_last_part && token != NULL) || (without_last_part && strstr(token, delimiter))) {
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

        token = strtok(NULL, delimiter);
    }

    return current_directory;
}

char *get_last_part_of_path(vfs *fs, char *input) {
    char *result;
    char *delimiter;
    char *token;

    if (!fs || !input) {
        return NULL;
    }

    delimiter = (char*)PATH_DELIMITER;
    token = strtok(input, delimiter);
    result = (char*)calloc(FILENAME_LENGTH, sizeof(char));

    while (token != NULL && strstr(token, delimiter)) {
        token = strtok(NULL, delimiter);
    }

    strncpy(result, token, FILENAME_LENGTH);

    return result;
}
