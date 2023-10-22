//
// Created by Šári Dvořáková on 22.10.2023.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "parser.h"
#include "fs.h"

int main(int argc, char *argv[]) {
    char *fs_name = NULL;   // nazev souboroveho systemu
    char *input = NULL;     // aktualni uzivatelsky input
    char *command = NULL;   // aktualni prikaz
    char *directory1 = NULL;   // prvni zadany adresar
    char *directory2 = NULL;   // druhy zadany adresar

    if (argc < ARG_COUNT) {
        printf("Usage: \n");
        printf("fs.c <file system name>\n");
        return EXIT_FAILURE;
    }

    fs_name = argv[1];
    input = (char*)malloc(COMMAND_LENGTH);
    command = (char*)malloc(STRING_LENGTH);
    directory1 = (char*)malloc(STRING_LENGTH);
    directory2 = (char*)malloc(STRING_LENGTH);

    do {
        printf("------------------------\n");
        printf("Type your command:\n");
        fgets(input, COMMAND_LENGTH, stdin);
        if (!parse_input(input, command, directory1, directory2)) {
            return EXIT_FAILURE;
        }

        if (strcmp(COMMAND_COPY, command) == 0) {
            printf("copy\n");
        }
        else if (strcmp(COMMAND_MOVE, command) == 0) {
            printf("move\n");
        }
        else if (strcmp(COMMAND_REMOVE, command) == 0) {
            printf("remove\n");
        }
        else if (strcmp(COMMAND_MAKE_DIR, command) == 0) {
            printf("make dir\n");
        }
        else if (strcmp(COMMAND_REMOVE_DIR, command) == 0) {
            printf("remove dir\n");
        }
        else if (strcmp(COMMAND_LIST, command) == 0) {
            printf("list\n");
        }
        else if (strcmp(COMMAND_CONCATENATE, command) == 0) {
            printf("concatenate\n");
        }
        else if (strcmp(COMMAND_CHANGE_DIR, command) == 0) {
            printf("change dir\n");
        }
        else if (strcmp(COMMAND_PRINT_WORK_DIR, command) == 0) {
            printf("print work dir\n");
        }
        else if (strcmp(COMMAND_INFO, command) == 0) {
            printf("info\n");
        }
        else if (strcmp(COMMAND_IN_COPY, command) == 0) {
            printf("input copy\n");
        }
        else if (strcmp(COMMAND_OUT_COPY, command) == 0) {
            printf("output copy\n");
        }
        else if (strcmp(COMMAND_LOAD, command) == 0) {
            printf("load\n");
        }
        else if (strcmp(COMMAND_FORMAT, command) == 0) {
            printf("format\n");
        }
        else if (strcmp(COMMAND_SYMB_LINK, command) == 0) {
            printf("symbolic link\n");
        }
        else {
            printf("Unknown command.\n");
        }
    } while (strcmp(COMMAND_END, command) != 0);

    free(input);
    free(command);
    free(directory1);
    free(directory2);

    return EXIT_SUCCESS;
}


