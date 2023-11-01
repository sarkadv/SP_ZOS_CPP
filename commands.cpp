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

int command_end(vfs *fs) {
    printf("Ending program.\n");
    free(fs);
    exit(EXIT_SUCCESS);
    return 1;
}

int command_format(char *size, vfs *fs) {
    char units[3];
    uint32_t format_size_B;
    superblock *sblock = NULL;
    uint32_t available_space;
    uint32_t cluster_count;

    memset(units, 0, sizeof(units));

    if (!size || !fs) {
        return 0;
    }

    sscanf(size, " %u %2[^0-9]\n", &format_size_B, units);

    if (!strcmp("B", units)) {
        format_size_B = format_size_B * 1;
    }
    else if (!strcmp("KB", units)) {
        format_size_B = format_size_B * 1000;
    }
    else if (!strcmp("MB", units)) {
        format_size_B = format_size_B * 1000 * 1000;
    }
    else if (!strcmp("GB", units)) {
        format_size_B = format_size_B * 1000 * 1000 * 1000;
    }
    else {
        printf("Wrong size unit, use B, KB, MB or GB.\n");
        return 0;
    }

    if (format_size_B < MIN_FS_SIZE_B) {
        printf("Size should be at least 50 KB.\n");
        return 0;
    }

    FILE *fp = fopen(fs->name, "w");
    fseek(fp, format_size_B - 1, SEEK_SET);
    fputc('\n', fp);
    fclose(fp);

    sblock = (superblock*)malloc(sizeof(superblock));
    strcpy(sblock->signature, SIGNATURE);
    sblock->disk_size = format_size_B;
    sblock->cluster_size = CLUSTER_SIZE_B;

    fs->superblock = sblock;

    available_space = sblock->disk_size - sizeof(superblock) - sizeof(pseudo_inode)*INODE_COUNT - INODE_COUNT/8;
    cluster_count = (8 * available_space - 8)/(8 * sblock->cluster_size + 1);
    sblock->cluster_count = cluster_count;

    fs->loaded = true;

    return 1;
}

int execute_command(char *command, char *param1, char *param2, vfs *fs) {
    if (!command) {
        return 0;
    }

    if (strcmp(COMMAND_HELP, command) == 0) {
        return command_help();
    }
    else if (strcmp(COMMAND_END, command) == 0) {
        return command_end(fs);
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
        return command_format(param1, fs);
    }
    else if (strcmp(COMMAND_SYMB_LINK, command) == 0) {
        printf("symbolic link\n");
    }
    else {
        printf("Unknown command.\n");
    }
}
