//
// Created by Šári Dvořáková on 22.10.2023.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "constants.h"
#include "parser.h"
#include "structures.h"
#include "commands.h"
#include "bitmap.h"

int vfs_exists(char *vfs) {
    FILE *file;
    if ((file = fopen(vfs, "r")))
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char input[COMMAND_LENGTH];     // aktualni uzivatelsky input
    char command[STRING_LENGTH];   // aktualni prikaz
    char param1[STRING_LENGTH];   // prvni zadany adresar
    char param2[STRING_LENGTH];   // druhy zadany adresar

    vfs *fs = (vfs*)malloc(sizeof(vfs));

    if (argc < ARG_COUNT) {
        printf("Usage: \n");
        printf("./vfs <file system name>\n");
        return EXIT_FAILURE;
    }

    fs->name = argv[1];

    if (!vfs_exists(fs->name)) {
        printf("VFS could not be loaded. Use the *format* command first.\n");
        fs->loaded = false;
    }
    else {
        // load vfs
        fs->loaded = true;
    }

    while (1) {
        printf("------------------------------------------------\n");
        printf("Type your command:\n");
        fgets(input, COMMAND_LENGTH, stdin);
        if (!parse_input(input, command, param1, param2)) {
            printf("Could not parse input.\n");
            continue;
        }
        if (!execute_command(command, param1, param2, fs)) {
            printf("Could not execute command.\n");
        }
    }
}


