//
// Created by Šári Dvořáková on 22.10.2023.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "constants.h"
#include "parser.h"
#include "vfs.h"
#include "commands.h"

vfs *fs;

void term(int signum)
{
    command_end(fs);
}

void reset_buffers(char *input, char *command, char *param1, char *param2) {
    memset(input, 0, COMMAND_LENGTH);
    memset(command, 0, STRING_LENGTH);
    memset(param1, 0, STRING_LENGTH);
    memset(param2, 0, STRING_LENGTH);
}

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
    char *input = (char*)calloc(COMMAND_LENGTH, sizeof(char));     // aktualni uzivatelsky input
    char *command = (char*)calloc(STRING_LENGTH, sizeof(char));   // aktualni prikaz
    char *param1 = (char*)calloc(STRING_LENGTH, sizeof(char));   // prvni zadany adresar
    char *param2 = (char*)calloc(STRING_LENGTH, sizeof(char));   // druhy zadany adresar

    fs = (vfs*)malloc(sizeof(vfs));

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

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
        if (!read_vfs_from_file(fs)) {
            printf("VFS could not be loaded from file.\n");
        }
        else {
            printf("VFS %s was loaded.\n", fs->name);
        }
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

        reset_buffers(input, command, param1, param2);
    }
}


