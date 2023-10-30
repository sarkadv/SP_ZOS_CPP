//
// Created by Šári Dvořáková on 30.10.2023.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "commands.h"

int command_help() {
    printf("--- Available commands ---\n");
    printf("\n");
    printf("* help ... shows available commands\n");
    printf("* end ... exits the program\n");
    printf("* cp <file> <path> ... copies <file> to <path>\n");
    printf("* mv <file1> <path/file2> ... moves <file1> to <path>, or renames <file1> to <file2>\n");
    printf("* rm <file> ... removes <file>\n");
    printf("* mkdir <directory> ... creates <directory>\n");
    printf("* rmdir <directory> ... removes <directory>\n");
    printf("* ls <directory> / ls ... shows the contents of <directory> / current directory\n");
    printf("* cat <file> ... displays the contents of <file>\n");
    printf("* cd <directory> ... changes the current path to <directory>\n");
    printf("* pwd ... displays the current path\n");
    printf("* info <file> / info <directory> ... displays information about <file> or <directory>\n");
    printf("* incp <file> <path> ... loads <file> from harddisk to VFS in <path>\n");
    printf("* outcp <file> <path> ... loads <file> from VFS to harddisk in <path>\n");
    printf("* load <file> ... loads <file> from harddisk with instructions, executes them\n");
    printf("* format <size> ... formats VFS to <size>\n");
    printf("* slink <file1> <file2> ... creates symbolic link to <file1> with name <file2>\n");
    printf("\n");
    return 1;
}

int command_end() {
    printf("Ending program.\n");
    exit(EXIT_SUCCESS);
    return 1;
}

int execute_command(char *command, char *param1, char *param2) {
    if (!command) {
        return 0;
    }

    if (strcmp(COMMAND_HELP, command) == 0) {
        return command_help();
    }
    else if (strcmp(COMMAND_END, command) == 0) {
        return command_end();
    }
    else if (strcmp(COMMAND_COPY, command) == 0) {
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
}
