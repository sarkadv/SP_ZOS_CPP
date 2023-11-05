//
// Created by Šári Dvořáková on 30.10.2023.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stack>
#include "commands.h"
#include "inode.h"
#include "data_block.h"
#include "parser.h"

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

int write_vfs_to_file(vfs *vfs) {
    FILE* fout;
    int i;

    if (!vfs) {
        return 0;
    }

    fout = fopen(vfs->name, "w");

    if (!fout) {
        return 0;
    }

    fwrite(vfs->superblock, sizeof(superblock), 1, fout);
    fwrite(vfs->bitmapd->array, sizeof(vfs->bitmapd->array_size_B), 1, fout);
    fwrite(vfs->bitmapi->array, sizeof(vfs->bitmapi->array_size_B), 1, fout);

    for (i = 0; i < INODE_COUNT; i++) {
        fwrite(vfs->inodes[i], sizeof(inode), 1, fout);
    }

    for (i = 0; i < vfs->superblock->cluster_count; i++) {
        fwrite(vfs->data_blocks[i], sizeof(data_block), 1, fout);
    }

    fclose(fout);

    return 1;
}

void create_root_directory(vfs *vfs) {
    directory_item *root_item = create_directory_item(1, (char*)"", NULL);
    vfs->root_directory = create_directory(NULL, NULL, NULL, root_item);
    vfs->all_directories[1] = vfs->root_directory;

    vfs->inodes[0]->nodeid = 1;
    vfs->inodes[0]->isDirectory = true;
    vfs->inodes[0]->direct1 = 1;    // zabrany 1. datovy blok

    set_bit(vfs->bitmapi, 0);
    set_bit(vfs->bitmapd, 0);

    vfs->current_directory = vfs->root_directory;
}

int add_subdirectory_to_directory(vfs *fs, directory *parent_directory, directory_item *subdirectory) {
    inode *inode;
    directory_item *subitem;

    if (!fs || !parent_directory || !subdirectory) {
        return 0;
    }

    inode = fs->inodes[subdirectory->inode];

    if (inode->isDirectory) {
        if (parent_directory->subdirectories == NULL) {
            parent_directory->subdirectories = subdirectory;
            subdirectory->next = NULL;
        }
        else {
            subitem = parent_directory->subdirectories;

            while (subitem->next != NULL) {
                subitem = subitem->next;
            }

            subitem->next = subdirectory;
            subdirectory->next = NULL;

        }
    }
    else {
        return 0;
    }

    return 1;
}

int add_file_to_directory(vfs *fs, directory *parent_directory, directory_item *file) {
    inode *inode;
    directory_item *subitem;

    if (!fs || !parent_directory || !file) {
        return 0;
    }

    inode = fs->inodes[file->inode];

    if (!inode->isDirectory) {
        if (parent_directory->files == NULL) {
            parent_directory->files = file;
            file->next = NULL;
        }
        else {
            subitem = parent_directory->files;

            while (subitem->next != NULL) {
                subitem = subitem->next;
            }

            subitem->next = file;
            file->next = NULL;

        }
    }
    else {
        return 0;
    }

    return 1;
}

void read_directory(vfs *fs, inode *inode, directory *dir);

void load_items_from_data_block(vfs *fs, directory *dir, data_block *block) {
    directory_item *loaded_directory_item;
    int32_t loaded_items = 0;
    directory *created_directory;

    if (!fs || !dir || !block) {
        return;
    }

    do {
        loaded_directory_item = (directory_item*)malloc(sizeof(directory_item));
        memcpy(loaded_directory_item, block + loaded_items*sizeof(directory_item), sizeof(directory_item));
        loaded_items++;

        if (fs->inodes[loaded_directory_item->inode]->isDirectory) {    // nacetli jsme adresar
            created_directory = create_directory(NULL, NULL, dir, loaded_directory_item);
            fs->all_directories[loaded_directory_item->inode] = created_directory;

            add_subdirectory_to_directory(fs, dir, loaded_directory_item);
            read_directory(fs, fs->inodes[loaded_directory_item->inode], created_directory);
        }
        else {  // nacetli jsme klasicky soubor
            add_file_to_directory(fs, dir, loaded_directory_item);
        }
    } while (loaded_directory_item->next != NULL);
}

void read_directory(vfs *fs, inode *inode, directory *dir) {
    data_block *current_data_block;

    if (!fs || !inode || !dir) {
        return;
    }

    if (inode->direct1 != 0) {
        current_data_block = fs->data_blocks[inode->direct1];
        if (inode->file_size != 0) {     // pro neprazdny adresar
            load_items_from_data_block(fs, dir, current_data_block);
        }

        if (inode->direct2 != 0) {
            load_items_from_data_block(fs, dir, current_data_block);

            if (inode->direct3 != 0) {
                load_items_from_data_block(fs, dir, current_data_block);

                if (inode->direct4 != 0) {
                    load_items_from_data_block(fs, dir, current_data_block);

                    if (inode->direct5 != 0) {
                        load_items_from_data_block(fs, dir, current_data_block);

                        if (inode->indirect1 != 0) {

                            if (inode->indirect2 != 0) {

                            }
                        }
                    }
                }
            }
        }
    }
}

int read_structure(vfs *fs) {
    if (!fs) {
        return 0;
    }

    read_directory(fs, fs->inodes[0], fs->root_directory);    // jdeme od root directory
    return 1;
}

int read_vfs_from_file(vfs *fs) {
    superblock *sblock;
    bitmap *bitmapi;
    bitmap *bitmapd;
    FILE *fin;
    int i;

    if (!fs) {
        return 0;
    }

    sblock = (superblock*)malloc(sizeof(superblock));
    fs->superblock = sblock;

    fin = fopen(fs->name, "r");
    if (!fin) {
        return 0;
    }

    fread(sblock, sizeof(superblock), 1, fin);

    bitmapd = create_bitmap(sblock->cluster_count);
    bitmapi = create_bitmap(INODE_COUNT);

    fread(bitmapd->array, sizeof(bitmapd->array_size_B), 1, fin);
    fread(bitmapi->array, sizeof(bitmapi->array_size_B), 1, fin);

    fs->bitmapd = bitmapd;
    fs->bitmapi = bitmapi;

    for (i = 0; i < INODE_COUNT; i++) {
        fs->inodes[i] = create_inode(ID_ITEM_FREE);
    }

    fs->data_blocks = (data_block **)calloc(sblock->cluster_count, sizeof(data_block*));

    for (i = 0; i < sblock->cluster_count; i++) {
        fs->data_blocks[i] = create_data_block();
    }

    for (i = 0; i < INODE_COUNT; i++) {
        fread(fs->inodes[i], sizeof(inode), 1, fin);
    }

    for (i = 0; i < fs->superblock->cluster_count; i++) {
        fread(fs->data_blocks[i], sizeof(data_block), 1, fin);
    }

    fclose(fin);

    create_root_directory(fs);

    if (!read_structure(fs)) {
        return 0;
    }

    fs->loaded = true;

    return 1;
}

int command_format(char *size, vfs *vfs) {
    char units[3];
    uint32_t format_size_B;
    superblock *sblock = NULL;
    uint32_t available_space;
    uint32_t cluster_count;
    bitmap *bitmapd;
    bitmap *bitmapi;
    int i;

    memset(units, 0, sizeof(units));

    if (!size || !vfs) {
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

    FILE *fp = fopen(vfs->name, "w");
    fseek(fp, format_size_B - 1, SEEK_SET);
    fputc('\n', fp);
    fclose(fp);

    available_space = format_size_B - sizeof(superblock) - sizeof(inode)*INODE_COUNT - INODE_COUNT/8;
    cluster_count = (8 * available_space - 8)/(8 * sizeof(data_block) + 1);

    bitmapd = create_bitmap(cluster_count);
    vfs->bitmapd = bitmapd;
    bitmapi = create_bitmap(INODE_COUNT);
    vfs->bitmapi = bitmapi;

    sblock = create_superblock((char*)SIGNATURE, format_size_B, DATA_BLOCK_SIZE_B,
                               cluster_count, bitmapd->array_size_B, bitmapi->array_size_B);
    vfs->superblock = sblock;

    for (i = 0; i < INODE_COUNT; i++) {
        vfs->inodes[i] = create_inode(ID_ITEM_FREE);
    }

    vfs->data_blocks = (data_block**)calloc(cluster_count, sizeof(data_block*));

    for (i = 0; i < cluster_count; i++) {
        vfs->data_blocks[i] = create_data_block();
    }

    create_root_directory(vfs);

    if (!write_vfs_to_file(vfs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    vfs->loaded = true;

    return 1;
}

int command_list(vfs *fs, char *path) {
    directory *dir;

    if (!fs) {
        return 0;
    }

    dir = parse_path(fs, path);
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
        return command_list(fs, param1);
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
