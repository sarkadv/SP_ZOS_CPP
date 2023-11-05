#include <string.h>
#include "parser.h"

int parse_input(char *input, char *command, char *param1, char *param2) {
    char *delimiter = NULL;
    char *token = NULL;

    if (!input || !command || !param1 || !param2) {
        return 0;
    }

    input[strlen(input) - 1] = '\0';

    delimiter = (char*)" ";
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

directory *parse_path(vfs *fs, char *input) {
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

    delimiter = (char*)"/";

    token = strtok(input, delimiter);

    while (token != NULL) {
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
