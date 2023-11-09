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
#include "constants.h"

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

int32_t find_free_inode(vfs *fs) {
    int i;

    if (!fs) {
        return 0;
    }

    for (i = 0; i < INODE_COUNT; i++) {
        if (!get_bit(fs->bitmapi, i)) {
            return i;
        }
    }

    return 0;    // zadny prazdny i uzel
}

int32_t *find_free_data_blocks(vfs *fs, int32_t required_blocks) {
    int32_t *blocks;
    int i;
    int32_t found = 0;

    if (!fs || required_blocks <= 0) {
        return NULL;
    }

    blocks = (int32_t*)malloc(sizeof(int32_t)*required_blocks);

    for (i = 0; i < fs->superblock->data_block_count; i++) {
        if (!get_bit(fs->bitmapd, i)) {
            blocks[found] = i;
            found++;

            if (found == required_blocks) {
                return blocks;
            }
        }
    }

    return NULL;    // nedostatek prazdnych bloku
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

directory_item *find_item_in_directory(vfs *fs, directory *parent_dir, char *item_name) {
    directory_item *subitem;

    if (!fs || !parent_dir || !item_name) {
        return NULL;
    }

    subitem = parent_dir->subdirectories;
    while (subitem != NULL) {
        if (!strcmp(subitem->name, item_name)) {
            return subitem;
        }
        subitem = subitem->next;
    }

    subitem = parent_dir->files;
    while (subitem != NULL) {
        if (!strcmp(subitem->name, item_name)) {
            return subitem;
        }
        subitem = subitem->next;
    }

    return NULL;
}

int add_subdirectory_to_directory(vfs *fs, directory *parent_directory, directory_item *subdirectory) {
    inode *inode;
    directory_item *subitem;

    if (!fs || !parent_directory || !subdirectory) {
        return 0;
    }

    inode = fs->inodes[subdirectory->inode - 1];

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

int remove_subdirectory_from_directory(vfs *fs, directory *parent_directory, directory_item *subdirectory) {
    inode *inode;
    directory_item *subitem;
    bool removed = false;

    if (!fs || !parent_directory || !subdirectory) {
        return 0;
    }

    if (parent_directory->subdirectories == NULL) {
        return 0;
    }

    inode = fs->inodes[subdirectory->inode - 1];

    if (inode->isDirectory) {
        subitem = parent_directory->subdirectories;

        if (!strcmp(subitem->name, subdirectory->name)) {    // odstraneni ze zacatku
            parent_directory->subdirectories = subitem->next;
            free(subdirectory);
            removed = true;
        }
        else {
            while (subitem->next != NULL) {
                if (!strcmp(subitem->next->name, subdirectory->name)) {
                    subitem->next = subitem->next->next;
                    free(subdirectory);
                    removed = true;
                    break;
                }
                else {
                    subitem = subitem->next;
                }
            }
        }
    }
    else {
        return 0;
    }

    return removed;
}

void read_directory(vfs *fs, inode *inode, directory *dir);

void load_items_from_data_block(vfs *fs, directory *dir, data_block *block) {
    directory_item *loaded_directory_item;
    int32_t loaded_items = 0;
    directory *created_directory;
    bool next;

    if (!fs || !dir || !block) {
        return;
    }

    do {
        loaded_directory_item = (directory_item*)malloc(sizeof(directory_item));
        memcpy(loaded_directory_item, block->data + loaded_items*sizeof(directory_item), sizeof(directory_item));
        loaded_items++;

        if (loaded_directory_item->next != NULL) {
            next = true;
        }
        else {
            next = false;
        }

        if (fs->inodes[loaded_directory_item->inode - 1]->isDirectory) {    // nacetli jsme adresar
            created_directory = create_directory(NULL, NULL, dir, loaded_directory_item);
            fs->all_directories[loaded_directory_item->inode] = created_directory;

            add_subdirectory_to_directory(fs, dir, loaded_directory_item);
            read_directory(fs, fs->inodes[loaded_directory_item->inode - 1], created_directory);
        }
        else {  // nacetli jsme klasicky soubor
            add_file_to_directory(fs, dir, loaded_directory_item);
        }
    } while (next);
}

/**
 * Rekurzivne nacte directory_itemy z i-uzlu predstavujiciho adresar.
 * Adresare mohou zabrat pouze 1 cluster = datovy blok.
 * @param fs        filesystem
 * @param inode     i uzel
 * @param dir       adresar
 */
void read_directory(vfs *fs, inode *inode, directory *dir) {
     if (!fs || !inode || !dir) {
        return;
    }

    if (inode->direct1 != 0) {
        if (!data_block_empty(fs->data_blocks[inode->direct1 - 1])) {     // pro neprazdny adresar
            load_items_from_data_block(fs, dir, fs->data_blocks[inode->direct1 - 1]);
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

    bitmapd = create_bitmap(sblock->data_block_count);
    bitmapi = create_bitmap(INODE_COUNT);

    fread(bitmapd->array, sizeof(bitmapd->array_size_B), 1, fin);
    fread(bitmapi->array, sizeof(bitmapi->array_size_B), 1, fin);

    fs->bitmapd = bitmapd;
    fs->bitmapi = bitmapi;

    for (i = 0; i < INODE_COUNT; i++) {
        fs->inodes[i] = create_inode(ID_ITEM_FREE);
    }

    fs->data_blocks = (data_block **)calloc(sblock->data_block_count, sizeof(data_block*));

    for (i = 0; i < sblock->data_block_count; i++) {
        fs->data_blocks[i] = create_data_block();
    }

    for (i = 0; i < INODE_COUNT; i++) {
        fread(fs->inodes[i], sizeof(inode), 1, fin);
    }

    for (i = 0; i < fs->superblock->data_block_count; i++) {
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

    for (i = 0; i < vfs->superblock->data_block_count; i++) {
        fwrite(vfs->data_blocks[i], sizeof(data_block), 1, fout);
    }

    fclose(fout);

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

    printf("OK\n");
    return 1;
}

int command_list(vfs *fs, char *path) {
    directory *dir;
    directory_item *subdirectory;
    directory_item *file;

    if (!fs) {
        return 0;
    }

    dir = parse_path(fs, path, false);

    if (!dir) {
        printf("Path not found.\n");
        return 0;
    }

    subdirectory = dir->subdirectories;
    file = dir->files;

    if (subdirectory == NULL && file == NULL) {
        printf("No contents found.\n");
    }

    while (subdirectory != NULL) {
        printf("+%s\n", subdirectory->name);
        subdirectory = subdirectory->next;
    }

    while (file != NULL) {
        printf("-%s\n", file->name);
        file = file->next;
    }

    return 1;
}

char *find_absolute_path(directory *dir) {
    std::vector<directory*> path;
    char *reversed;
    directory *current_directory;
    int i;
    char *delimiter = (char*)PATH_DELIMITER;

    if (!dir) {
        return NULL;
    }

    current_directory = dir;

    while (current_directory != NULL) {
        path.push_back(current_directory);
        current_directory = current_directory->parent;
    }

    reversed = (char*)calloc(sizeof(char), COMMAND_LENGTH);

    strcat(reversed, delimiter);

    for (i = path.size() - 2; i >= 0; i--) {
        strcat(reversed, path[i]->this_item->name);

        if (i != 0) {
            strcat(reversed, delimiter);
        }
    }

    return reversed;

}

int command_print_work_dir(vfs *fs) {
    char *path;

    if (!fs) {
        return 0;
    }

    path = find_absolute_path(fs->current_directory);
    printf("%s\n", path);
    free(path);

    return 1;
}

int command_change_dir(vfs *fs, char *path) {
    directory *result_directory;

    if (!fs) {
        return 0;
    }

    result_directory = parse_path(fs, path, false);

    if (!result_directory) {
        printf("Path not found.\n");
        return 0;
    }

    fs->current_directory = result_directory;

    printf("OK\n");
    return 1;
}

int command_make_dir(vfs *fs, char *created_dir_path) {
    directory *parent_dir;
    char *created_dir_name;
    int32_t free_inode;
    int32_t *free_data_blocks;
    int32_t free_data_block;
    directory_item *created_directory_item;
    directory *created_directory;
    inode *parent_dir_inode;
    data_block *parent_dir_data_block;

    if (!fs || !created_dir_path) {
        return 0;
    }

    if (parse_path(fs, created_dir_path, false) != NULL) {     // adresar uz existuje
        printf("Directory already exists.\n");
        return 0;
    }

    parent_dir = parse_path(fs, created_dir_path, true);

    if (!parent_dir) {
        printf("Path not found.\n");
        return 0;
    }

    if (count_directory_contents(parent_dir) >= MAX_DIRECTORY_ITEMS) {
        printf("Directory has reached the maximum count of items.\n");
        return 0;
    }

    free_inode = find_free_inode(fs);

    if (free_inode == 0) {
        printf("No free i-nodes have been found.\n");
    }

    free_data_blocks = find_free_data_blocks(fs, 1);

    if (!free_data_blocks) {
        printf("Not enough free data blocks have been found.\n");
    }

    free_data_block = free_data_blocks[0];
    created_dir_name = get_last_part_of_path(fs, created_dir_path);

    created_directory_item = create_directory_item(free_inode + 1, created_dir_name, NULL);
    created_directory = create_directory(NULL, NULL, parent_dir, created_directory_item);

    fs->inodes[free_inode]->nodeid = free_inode + 1;
    fs->inodes[free_inode]->isDirectory = true;
    fs->inodes[free_inode]->direct1 = free_data_block + 1;
    fs->inodes[free_inode]->file_size = DATA_BLOCK_SIZE_B;

    set_bit(fs->bitmapi, free_inode);
    set_bit(fs->bitmapd, free_data_block);

    fs->all_directories[fs->inodes[free_inode]->nodeid] = created_directory;
    add_subdirectory_to_directory(fs, parent_dir, created_directory_item);

    parent_dir_inode = fs->inodes[parent_dir->this_item->inode - 1];
    parent_dir_data_block = fs->data_blocks[parent_dir_inode->direct1 - 1];

    // zapis do fs->data_blocks
    write_dir_items_to_data_block(parent_dir_data_block, parent_dir->subdirectories, parent_dir->files);

    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    printf("OK\n");
    return 1;
}

int command_remove_dir(vfs *fs, char *removed_dir_path) {
    directory *parent_dir;
    directory *removed_dir;
    int32_t freed_inode_index;
    int32_t freed_data_block_index;
    directory_item *removed_directory_item;
    inode *parent_dir_inode;
    data_block *parent_dir_data_block;

    if (!fs || !removed_dir_path) {
        return 0;
    }

    removed_dir = parse_path(fs, removed_dir_path, false);

    if (!removed_dir) {
        printf("Directory not found.\n");
        return 0;
    }

    if (count_directory_contents(removed_dir) > 0) {
        printf("Directory not empty.\n");
        return 0;
    }

    parent_dir = removed_dir->parent;
    removed_directory_item = removed_dir->this_item;
    freed_inode_index = removed_dir->this_item->inode - 1;
    freed_data_block_index = fs->inodes[freed_inode_index]->direct1 - 1;

    remove_subdirectory_from_directory(fs, parent_dir, removed_directory_item);
    fs->all_directories[fs->inodes[freed_inode_index]->nodeid] = NULL;

    memset(fs->inodes[freed_inode_index], 0, sizeof(inode));

    clear_bit(fs->bitmapi, freed_inode_index);
    clear_bit(fs->bitmapd, freed_data_block_index);

    parent_dir_inode = fs->inodes[parent_dir->this_item->inode - 1];
    parent_dir_data_block = fs->data_blocks[parent_dir_inode->direct1 - 1];

    // zapis do fs->data_blocks
    write_dir_items_to_data_block(parent_dir_data_block, parent_dir->subdirectories, parent_dir->files);

    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    printf("OK\n");
    return 1;
}


int command_info(vfs *fs, char *path) {
    directory *parent_dir;
    directory_item *item;
    char *item_name;
    inode *inode;

    if (!fs || !path) {
        return 0;
    }

    parent_dir = parse_path(fs, path, true);

    if (!parent_dir) {
        printf("Path not found.\n");
        return 0;
    }

    item_name = get_last_part_of_path(fs, path);
    item = find_item_in_directory(fs, parent_dir, item_name);
    free(item_name);

    if (!item) {
        printf("File not found.\n");
        return 0;
    }

    inode = fs->inodes[item->inode - 1];

    printf("* name: %s - size: %dB - i-node ID: %d *\n", item->name, inode->file_size, inode->nodeid);
    printf("* used data blocks IDs *\n");
    printf("\t- direct: ");

    if (inode->direct1 != 0) {
        printf("%d ", inode->direct1);
    }
    if (inode->direct2 != 0) {
        printf("%d ", inode->direct2);
    }
    if (inode->direct3 != 0) {
        printf("%d ", inode->direct3);
    }
    if (inode->direct4 != 0) {
        printf("%d ", inode->direct4);
    }
    if (inode->direct5 != 0) {
        printf("%d ", inode->direct5);
    }
    printf("\n");

    printf("\t- indirect: ");
    if (inode->indirect1 != 0) {
        printf("%d ", inode->indirect1);
    }
    if (inode->indirect2 != 0) {
        printf("%d ", inode->indirect2);
    }
    printf("\n");

    return 1;
}


int command_end(vfs *fs) {
    printf("Ending program.\n");
    free(fs);
    exit(EXIT_SUCCESS);
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
        if (fs->loaded) {
            printf("copy\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_MOVE, command) == 0) {
        if (fs->loaded) {
            printf("move\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_REMOVE, command) == 0) {
        if (fs->loaded) {
            printf("remove\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_MAKE_DIR, command) == 0) {
        if (fs->loaded) {
            return command_make_dir(fs, param1);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_REMOVE_DIR, command) == 0) {
        if (fs->loaded) {
            return command_remove_dir(fs, param1);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_LIST, command) == 0) {
        if (fs->loaded) {
            return command_list(fs, param1);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_CONCATENATE, command) == 0) {
        if (fs->loaded) {
            printf("concatenate\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_CHANGE_DIR, command) == 0) {
        if (fs->loaded) {
            return command_change_dir(fs, param1);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_PRINT_WORK_DIR, command) == 0) {
        if (fs->loaded) {
            return command_print_work_dir(fs);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_INFO, command) == 0) {
        if (fs->loaded) {
            return command_info(fs, param1);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_IN_COPY, command) == 0) {
        if (fs->loaded) {
            printf("input copy\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_OUT_COPY, command) == 0) {
        if (fs->loaded) {
            printf("output copy\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_LOAD, command) == 0) {
        if (fs->loaded) {
            printf("load\n");
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_FORMAT, command) == 0) {
        return command_format(param1, fs);
    }
    else if (strcmp(COMMAND_SYMB_LINK, command) == 0) {
        if (fs->loaded) {
            printf("symbolic link\n");
        }
        else {
            return 0;
        }
    }
    else {
        printf("Unknown command. Use the command *help* to list available commands.\n");
        return 0;
    }
}
