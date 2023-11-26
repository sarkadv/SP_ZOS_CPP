/*
 * Prikazy, ktere muze uzivatel zadavat pro praci s file systemem.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stack>
#include "commands.h"
#include "inode.h"
#include "data_block.h"
#include "parser.h"
#include "constants.h"

/*
 * Prikaz zobrazi vsechny dostupne prikazy programu.
 */
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

/*
 * Nalezne a vrati index volneho i-uzlu ve file systemu fs.
 */
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

/*
 * Nalezne a vrati indexy volnych datovych bloku ve file systemu fs.
 * Pocet pozadovanych bloku je parametr required_blocks.
 */
int32_t *find_free_data_blocks(vfs *fs, int32_t required_blocks) {
    int32_t *blocks;
    int32_t i;
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

    free(blocks);
    return NULL;    // nedostatek prazdnych bloku
}

/*
 * Vytvori korenovy adresar file systemu fs.
 */
void create_root_directory(vfs *fs) {
    directory_item *root_item = create_directory_item(1, (char*)"", NULL);
    fs->root_directory = create_directory(NULL, NULL, NULL, root_item);
    fs->all_directories[1] = fs->root_directory;

    fs->inodes[0]->nodeid = 1;      // zabrany 1. i-uzel
    fs->inodes[0]->is_directory = true;
    fs->inodes[0]->direct1 = 1;    // zabrany 1. datovy blok

    set_bit(fs->bitmapi, 0);
    set_bit(fs->bitmapd, 0);

    fs->current_directory = fs->root_directory;
}

/*
 * Prida podadresar subdirectory do adresare parent_directory ve file systemu fs.
 */
int add_subdirectory_to_directory(vfs *fs, directory *parent_directory, directory_item *subdirectory) {
    inode *inode;
    directory_item *subitem;

    if (!fs || !parent_directory || !subdirectory) {
        return 0;
    }

    inode = fs->inodes[subdirectory->inode - 1];

    if (inode->is_directory) {
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

/*
 * Prida soubor file do adresare parent_directory ve file systemu fs.
 */
int add_file_to_directory(vfs *fs, directory *parent_directory, directory_item *file) {
    inode *inode;
    directory_item *subitem;

    if (!fs || !parent_directory || !file) {
        return 0;
    }

    inode = fs->inodes[file->inode - 1];

    if (!inode->is_directory) {
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

/*
 * Odstrani podadresar subdirectory z adresare parent_directory ve file systemu fs.
 */
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

    if (inode->is_directory) {
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

/*
 * Odstrani soubor file z adresare parent_directory ve file systemu fs.
 */
int remove_file_from_directory(vfs *fs, directory *parent_directory, directory_item *file) {
    inode *inode;
    directory_item *subitem;
    bool removed = false;

    if (!fs || !parent_directory || !file) {
        return 0;
    }

    if (parent_directory->files == NULL) {
        return 0;
    }

    inode = fs->inodes[file->inode - 1];

    if (!inode->is_directory) {
        subitem = parent_directory->files;

        if (!strcmp(subitem->name, file->name)) {    // odstraneni ze zacatku
            parent_directory->files = subitem->next;
            free(file);
            removed = true;
        }
        else {
            while (subitem->next != NULL) {
                if (!strcmp(subitem->next->name, file->name)) {
                    subitem->next = subitem->next->next;
                    free(file);
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

/*
 * Z datoveho bloku block prislusnemu adresari dir ve file systemu fs nacte polozky directory_item.
 * Pokud je nactena polozka adresar, funkce je volana rekurzivne.
 * Pokud je nactena polozka soubor, rekurze je zastavena.
 */
void load_items_from_data_block(vfs *fs, directory *dir, data_block *block) {
    directory_item *loaded_directory_item;
    int32_t loaded_items = 0;
    directory *created_directory;

    if (!fs || !dir || !block) {
        return;
    }

    while (1) {
        loaded_directory_item = (directory_item*)malloc(sizeof(directory_item));
        memcpy(loaded_directory_item, block->data + loaded_items*sizeof(directory_item), sizeof(directory_item));
        loaded_items++;

        // nacetli jsme prazdny usek, nejedna se o directory item
        if (loaded_directory_item->inode == 0) {
            free(loaded_directory_item);
            break;
        }

        if (fs->inodes[loaded_directory_item->inode - 1]->is_directory) {    // nacetli jsme adresar
            created_directory = create_directory(NULL, NULL, dir, loaded_directory_item);
            fs->all_directories[loaded_directory_item->inode] = created_directory;

            add_subdirectory_to_directory(fs, dir, loaded_directory_item);
            read_directory(fs, fs->inodes[loaded_directory_item->inode - 1], created_directory);
        }
        else {  // nacetli jsme klasicky soubor
            add_file_to_directory(fs, dir, loaded_directory_item);
        }
    }
}

/*
 * Rekurzivne nacte polozky directory_item z i-uzlu inode prislusnemu adresari dir ve file systemu fs.
 * Adresare mohou zabrat pouze 1 cluster = datovy blok.
 */
void read_directory(vfs *fs, inode *inode, directory *dir) {
    if (!fs || !inode || !dir) {
        return;
    }

    if (inode->direct1 != 0) {
        if (!inode->is_symlink) {
            if (!data_block_empty(fs->data_blocks[inode->direct1 - 1])) {     // pro neprazdny adresar
                load_items_from_data_block(fs, dir, fs->data_blocks[inode->direct1 - 1]);
            }
        }
    }
}

/*
 * Zacne cteni file systemu fs od korenoveho adresare fs->root_directory.
 */
int read_structure(vfs *fs) {
    if (!fs) {
        return 0;
    }

    read_directory(fs, fs->inodes[0], fs->root_directory);    // jdeme od root directory
    return 1;
}

/*
 * Nacteni celeho file systemu fs ze souboru na disku.
 */
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

    fin = fopen(fs->name, "rb");
    if (!fin) {
        return 0;
    }

    // nacteni superblocku
    fread(sblock, sizeof(superblock), 1, fin);

    bitmapd = create_bitmap(sblock->data_block_count);
    bitmapi = create_bitmap(INODE_COUNT);

    // nacteni bitmap
    fread(bitmapd->array, bitmapd->array_size_B, 1, fin);
    fread(bitmapi->array, bitmapi->array_size_B, 1, fin);

    fs->bitmapd = bitmapd;
    fs->bitmapi = bitmapi;

    for (i = 0; i < INODE_COUNT; i++) {
        fs->inodes[i] = create_inode(ID_ITEM_FREE);
    }

    fs->data_blocks = (data_block **)calloc(sblock->data_block_count, sizeof(data_block*));

    for (i = 0; i < sblock->data_block_count; i++) {
        fs->data_blocks[i] = create_data_block();
    }

    // nacteni i-uzlu
    for (i = 0; i < INODE_COUNT; i++) {
        fread(fs->inodes[i], sizeof(inode), 1, fin);
    }

    // nacteni datovych bloku
    for (i = 0; i < fs->superblock->data_block_count; i++) {
        fread(fs->data_blocks[i], sizeof(data_block), 1, fin);
    }

    fclose(fin);

    create_root_directory(fs);

    // nacteni adresarove struktury
    if (!read_structure(fs)) {
        return 0;
    }

    fs->loaded = true;

    return 1;
}

/*
 * Zapise file system fs do souboru na disku.
 */
int write_vfs_to_file(vfs *fs) {
    FILE* fout;
    int i;

    if (!fs) {
        return 0;
    }

    fout = fopen(fs->name, "wb");

    if (!fout) {
        return 0;
    }

    // zapis superblocku, bitmap
    fwrite(fs->superblock, sizeof(superblock), 1, fout);
    fwrite(fs->bitmapd->array, fs->bitmapd->array_size_B, 1, fout);
    fwrite(fs->bitmapi->array, fs->bitmapi->array_size_B, 1, fout);

    // zapis i-uzlu
    for (i = 0; i < INODE_COUNT; i++) {
        fwrite(fs->inodes[i], sizeof(inode), 1, fout);
    }

    // zapis datovych bloku
    for (i = 0; i < fs->superblock->data_block_count; i++) {
        fwrite(fs->data_blocks[i], sizeof(data_block), 1, fout);
    }

    fclose(fout);

    return 1;
}

/*
 * Naformatuje file system fs na danou hodnotu size.
 */
int command_format(char *size, vfs *fs) {
    char units[3];
    uint32_t format_size_B;
    superblock *sblock = NULL;
    uint32_t available_space;
    uint32_t cluster_count;
    bitmap *bitmapd;
    bitmap *bitmapi;
    int i;

    memset(units, 0, 3);

    if (!size || !fs) {
        return 0;
    }

    sscanf(size, " %u %2[^0-9]\n", &format_size_B, units);

    // vypocet velikosti v bytech
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

    FILE *fp = fopen(fs->name, "wb");
    fseek(fp, format_size_B - 1, SEEK_SET);
    fputc('\n', fp);
    fclose(fp);

    // vypocet volneho mista pro bloky
    available_space = format_size_B - sizeof(superblock) - sizeof(inode)*INODE_COUNT - INODE_COUNT/8;

    // vypocet poctu bloku
    cluster_count = (8 * available_space - 8)/(8 * sizeof(data_block) + 1);

    // vytvoreni bitmap
    bitmapd = create_bitmap(cluster_count);
    fs->bitmapd = bitmapd;
    bitmapi = create_bitmap(INODE_COUNT);
    fs->bitmapi = bitmapi;

    // vytvoreni superblocku
    sblock = create_superblock((char*)SIGNATURE, format_size_B, DATA_BLOCK_SIZE_B,
                               cluster_count, bitmapd->array_size_B, bitmapi->array_size_B);
    fs->superblock = sblock;

    // vytvoreni i-uzlu
    for (i = 0; i < INODE_COUNT; i++) {
        fs->inodes[i] = create_inode(ID_ITEM_FREE);
    }

    // vytvoreni datovych bloku
    fs->data_blocks = (data_block**)calloc(cluster_count, sizeof(data_block*));

    for (i = 0; i < cluster_count; i++) {
        fs->data_blocks[i] = create_data_block();
    }

    // vytvoreni korenoveho adresare
    create_root_directory(fs);

    // zapis do souboru
    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    fs->loaded = true;

    printf("OK\n");
    return 1;
}

/*
 * Vypise obsah adresare na ceste path ve file systemu fs.
 */
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

/*
 * Najde a vrati textovy retezec absolutni cesty k adresari dir.
 */
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

/*
 * Vypise aktualni cestu ve file systemu fs.
 */
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

/*
 * Zmeni aktualni cestu do adresare na ceste path ve file systemu fs.
 */
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

/*
 * Vytvori adresar na ceste created_dir_path ve file systemu fs.
 */
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

    created_dir_name = get_last_part_of_path(fs, created_dir_path);

    if (strchr(created_dir_name, '/') != NULL) {
        printf("Directory name cannot contain the / character.\n");
        return 0;
    }

    if (find_diritem_in_dir_by_name(parent_dir, created_dir_name)) {
        printf("Directory already contains a file with this name.\n");
        return 0;
    }

    free_inode = find_free_inode(fs);

    if (free_inode == 0) {
        printf("No free i-nodes have been found.\n");
        return 0;
    }

    free_data_blocks = find_free_data_blocks(fs, 1);

    if (!free_data_blocks) {
        printf("Not enough free data blocks have been found.\n");
        return 0;
    }

    free_data_block = free_data_blocks[0];

    created_directory_item = create_directory_item(free_inode + 1, created_dir_name, NULL);
    created_directory = create_directory(NULL, NULL, parent_dir, created_directory_item);

    fs->inodes[free_inode]->nodeid = free_inode + 1;
    fs->inodes[free_inode]->is_directory = true;
    fs->inodes[free_inode]->direct1 = free_data_block + 1;
    fs->inodes[free_inode]->file_size = DATA_BLOCK_SIZE_B;
    fs->inodes[free_inode]->references = 1;

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

    free(created_dir_name);
    free(free_data_blocks);

    printf("OK\n");
    return 1;
}

/*
 * Vymaze prazdny adresar na ceste removed_dir_path ve file systemu fs.
 */
int command_remove_dir(vfs *fs, char *removed_dir_path) {
    directory *parent_dir;
    directory *removed_dir;
    char *removed_dir_name;
    int32_t freed_inode_index;
    int32_t freed_data_block_index;
    directory_item *removed_directory_item;
    inode *parent_dir_inode;
    data_block *parent_dir_data_block;

    if (!fs || !removed_dir_path) {
        return 0;
    }

    parent_dir = parse_path(fs, removed_dir_path, true);

    if (!parent_dir) {
        printf("Directory not found.\n");
        return 0;
    }

    removed_dir_name = get_last_part_of_path(fs, removed_dir_path);
    removed_directory_item = find_diritem_in_dir_by_name(parent_dir, removed_dir_name);

    if (!fs->inodes[removed_directory_item->inode - 1]->is_directory) {
        if (!parent_dir) {
            printf("File is not a directory.\n");
            return 0;
        }
    }

    removed_dir = fs->all_directories[removed_directory_item->inode];

    if (count_directory_contents(removed_dir) > 0) {
        printf("Directory not empty.\n");
        return 0;
    }

    freed_inode_index = removed_directory_item->inode - 1;
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

    free(removed_dir_name);
    printf("OK\n");
    return 1;
}

/*
 * Nalezne a vrati pole vsech bloku odkazovanych 1. neprimym odkazem v bloku block.
 * Na adresu count je ulozen pocet bloku.
 */
int32_t *find_all_indirect1_data_blocks(data_block *block, int32_t *count) {
    int32_t *loaded_block;
    int32_t *result;

    if (!block || !count) {
        return NULL;
    }

    loaded_block = (int32_t*)malloc(sizeof(int32_t));
    result = (int32_t*)calloc(MAX_DIRECTORY_REFERENCES, sizeof(int32_t));
    *count = 0;

    while (1) {
        memcpy(loaded_block, block->data + (*count)*sizeof(int32_t), sizeof(int32_t));

        if (*loaded_block == 0) {
            break;
        }

        memcpy(result + (*count), block->data + (*count)*sizeof(int32_t), sizeof(int32_t));
        (*count)++;
    }

    free(loaded_block);

    return result;
}

/*
 * Nalezne a vrati pole vsech bloku odkazovanych 2. neprimym odkazem v bloku block ve file systemu fs.
 * Na adresu count je ulozen pocet bloku.
 */
int32_t *find_all_indirect2_data_blocks(vfs *fs, data_block *block, int32_t *count) {
    int32_t *loaded_indirect2_block;
    int32_t indirect2_counter = 0;
    int32_t *loaded_indirect1_blocks;
    int32_t loaded_indirect1_blocks_count = 0;
    int32_t *result;
    int32_t i;

    if (!fs || !block || !count) {
        return NULL;
    }

    loaded_indirect2_block = (int32_t*)malloc(sizeof(int32_t));
    result = (int32_t*)calloc(MAX_DIRECTORY_REFERENCES*MAX_DIRECTORY_REFERENCES, sizeof(int32_t));

    while (1) {
        memcpy(loaded_indirect2_block, block->data + indirect2_counter*sizeof(int32_t), sizeof(int32_t));

        if (*loaded_indirect2_block == 0) {
            break;
        }

        loaded_indirect1_blocks = find_all_indirect1_data_blocks(fs->data_blocks[*loaded_indirect2_block - 1], &loaded_indirect1_blocks_count);

        for (i = 0; i < loaded_indirect1_blocks_count; i++) {
            memcpy(result + (*count), loaded_indirect1_blocks + i, sizeof(int32_t));
            (*count)++;
        }

        indirect2_counter++;
    }

    free(loaded_indirect2_block);
    free(loaded_indirect1_blocks);

    return result;
}

/*
 * Nalezne a vrati vsechna data (ne odkazy na bloky) jako textovy retezec souboru s prislusnym i-uzlem inode ve file systemu fs.
 */
unsigned char *get_all_data_as_string(vfs *fs, inode *inode) {
    unsigned char *result;
    int32_t *indirect1_blocks = NULL;
    int32_t indirect1_blocks_count = 0;
    int32_t *indirect2_blocks = NULL;
    int32_t indirect2_blocks_count = 0;
    int32_t i, j;
    unsigned char *current_char;
    int32_t char_count = 0;

    if (!fs || !inode) {
        return NULL;
    }

    result = (unsigned char*)calloc(inode->file_size, sizeof(unsigned char));

    if (inode->direct1 != 0) {
        for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
            current_char = fs->data_blocks[inode->direct1 - 1]->data + i;
            result[char_count] = *current_char;
            char_count++;

            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (inode->direct2 != 0) {
        for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
            current_char = fs->data_blocks[inode->direct2 - 1]->data + i;
            result[char_count] = *current_char;
            char_count++;

            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (inode->direct3 != 0) {
        for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
            current_char = fs->data_blocks[inode->direct3 - 1]->data + i;
            result[char_count] = *current_char;
            char_count++;

            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (inode->direct4 != 0) {
        for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
            current_char = fs->data_blocks[inode->direct4 - 1]->data + i;
            result[char_count] = *current_char;
            char_count++;

            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (inode->direct5 != 0) {
        for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
            current_char = fs->data_blocks[inode->direct5 - 1]->data + i;
            result[char_count] = *current_char;
            char_count++;

            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (inode->indirect1 != 0) {
        indirect1_blocks = find_all_indirect1_data_blocks(fs->data_blocks[inode->indirect1 - 1], &indirect1_blocks_count);

        for (i = 0; i < indirect1_blocks_count; i++) {
            for (j = 0; j < DATA_BLOCK_SIZE_B; j++) {
                current_char = fs->data_blocks[indirect1_blocks[i] - 1]->data + j;
                result[char_count] = *current_char;
                char_count++;

                if (char_count == inode->file_size) {
                    break;
                }
            }
            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (inode->indirect2 != 0) {
        indirect2_blocks = find_all_indirect2_data_blocks(fs, fs->data_blocks[inode->indirect2 - 1], &indirect2_blocks_count);

        for (i = 0; i < indirect2_blocks_count; i++) {
            for (j = 0; j < DATA_BLOCK_SIZE_B; j++) {
                current_char = fs->data_blocks[indirect2_blocks[i] - 1]->data + j;
                result[char_count] = *current_char;
                char_count++;

                if (char_count == inode->file_size) {
                    break;
                }
            }
            if (char_count == inode->file_size) {
                break;
            }
        }
    }

    if (indirect1_blocks) {
        free(indirect1_blocks);
    }

    if (indirect2_blocks) {
        free(indirect2_blocks);
    }

    return result;
}

/*
 * Nalezne a vrati vsechna data (ne odkazy na bloky) jako bloky velikosti DATA_BLOCK_SIZE_B souboru s prislusnym i-uzlem inode ve file systemu fs.
 */
unsigned char **get_all_data_as_blocks(vfs *fs, inode *inode) {
    unsigned char **blocks;
    unsigned char *output;
    int32_t block_count;
    int32_t i;

    if (!fs || !inode) {
        return NULL;
    }

    output = get_all_data_as_string(fs, inode);

    block_count = inode->file_size / DATA_BLOCK_SIZE_B;

    if (inode->file_size % DATA_BLOCK_SIZE_B != 0) {
        block_count++;
    }

    blocks = (unsigned char**)calloc(block_count, sizeof(unsigned char*));

    for (i = 0; i < block_count; i++) {
        blocks[i] = (unsigned char*)calloc(DATA_BLOCK_SIZE_B, sizeof(unsigned char));
        memcpy(blocks[i], output + i*DATA_BLOCK_SIZE_B, DATA_BLOCK_SIZE_B);
    }

    free(output);

    return blocks;
}

/*
 * Vypise informace o souboru / adresari na ceste path ve file systemu fs.
 * Informace zahrnuji jmeno, velikost, i-uzel, prime a neprime odkazy, symbolicky odkaz.
 */
int command_info(vfs *fs, char *path) {
    directory *parent_dir;
    directory_item *item;
    char *item_name;
    inode *inode;
    int32_t *indirect1_blocks = NULL;
    int32_t indirect1_blocks_count = 0;
    int32_t *indirect2_blocks = NULL;
    int32_t indirect2_blocks_count = 0;
    int32_t i;
    int32_t data_block_count = 0;

    if (!fs || !path) {
        return 0;
    }

    parent_dir = parse_path(fs, path, true);

    if (!parent_dir) {
        printf("Path not found.\n");
        return 0;
    }

    item_name = get_last_part_of_path(fs, path);
    item = find_diritem_in_dir_by_name(parent_dir, item_name);

    if (!item) {
        printf("File not found.\n");
        return 0;
    }

    inode = fs->inodes[item->inode - 1];

    printf("* name: %s - size: %d B - i-node ID: %d *\n", item->name, inode->file_size, inode->nodeid);

    if (inode->is_symlink) {
        printf("* %s -> %s *\n", item->name, get_symlink_reference(fs->data_blocks[inode->direct1 - 1]));
    }

    printf("* used data blocks IDs *\n");
    printf("\t- direct: ");

    if (inode->direct1 != 0) {
        printf("%d ", inode->direct1);
        data_block_count++;
    }
    if (inode->direct2 != 0) {
        printf("%d ", inode->direct2);
        data_block_count++;
    }
    if (inode->direct3 != 0) {
        printf("%d ", inode->direct3);
        data_block_count++;
    }
    if (inode->direct4 != 0) {
        printf("%d ", inode->direct4);
        data_block_count++;
    }
    if (inode->direct5 != 0) {
        printf("%d ", inode->direct5);
        data_block_count++;
    }
    printf("\n");

    printf("\t- 1. indirect: ");
    if (inode->indirect1 != 0) {
        printf("%d ", inode->indirect1);
        data_block_count++;

        indirect1_blocks = find_all_indirect1_data_blocks(fs->data_blocks[inode->indirect1 - 1], &indirect1_blocks_count);

        for (i = 0; i < indirect1_blocks_count; i++) {
            printf("%d ", indirect1_blocks[i]);
            data_block_count++;
        }
    }
    printf("\n");

    printf("\t- 2. indirect: ");
    if (inode->indirect2 != 0) {
        printf("%d ", inode->indirect2);
        data_block_count++;

        indirect1_blocks = find_all_indirect1_data_blocks(fs->data_blocks[inode->indirect2 - 1], &indirect1_blocks_count);

        for (i = 0; i < indirect1_blocks_count; i++) {
            printf("%d ", indirect1_blocks[i]);
            data_block_count++;
        }

        indirect2_blocks = find_all_indirect2_data_blocks(fs, fs->data_blocks[inode->indirect2 - 1], &indirect2_blocks_count);

        for (i = 0; i < indirect2_blocks_count; i++) {
            printf("%d ", indirect2_blocks[i]);
            data_block_count++;
        }
    }

    printf("\n");

    printf("* used data block count: %d *\n", data_block_count);

    free(item_name);

    if (indirect1_blocks != NULL) {
        free(indirect1_blocks);
    }

    if (indirect2_blocks != NULL) {
        free(indirect2_blocks);
    }

    return 1;
}

/*
 * Nacte soubor na ceste path z pevneho disku s jednotlivymi prikazy pro file system fs.
 * Prikazy zacne sekvencne vykonavat.
 * Format je 1 prikaz na 1 radek.
 */
int command_load(vfs *fs, char *path) {
    FILE *file;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (!fs || !path) {
        return 0;
    }

    char *command = (char*)calloc(STRING_LENGTH, sizeof(char));   // aktualni prikaz
    char *param1 = (char*)calloc(STRING_LENGTH, sizeof(char));   // prvni zadany adresar
    char *param2 = (char*)calloc(STRING_LENGTH, sizeof(char));   // druhy zadany adresar

    file = fopen(path, "rb");
    if (!file) {
        printf("File not found.\n");
        return 0;
    }

    while ((read = getline(&line, &len, file)) != -1) {
        printf("Executing line: %s", line);
        if (!parse_input(line, command, param1, param2)) {
            printf("Could not parse input.\n");
            continue;
        }
        if (!execute_command(command, param1, param2, fs)) {
            printf("Could not execute command.\n");
        }

        memset(command, 0, STRING_LENGTH);
        memset(param1, 0, STRING_LENGTH);
        memset(param2, 0, STRING_LENGTH);
    }

    fclose(file);
    free(line);
    free(command);
    free(param1);
    free(param2);

    return 1;
}

/*
 * Nahraje soubor z pevneho disku na ceste disk_file_path do umisteni fs_file_path ve file systemu fs.
 */
int command_in_copy(vfs *fs, char *disk_file_path, char *fs_file_path) {
    FILE *file;
    directory *parent_dir;
    int32_t file_size;
    char *filename;
    int32_t remaining_bytes;
    int32_t needed_direct_blocks = 0;
    int32_t needed_indirect1_blocks = 0;
    int32_t needed_indirect2_blocks = 0;
    int32_t total_needed_blocks = 0;
    int32_t total_needed_blocks_for_data = 0;
    int32_t free_inode;
    int32_t *free_data_blocks;
    directory_item *created_directory_item;
    inode *parent_dir_inode;
    data_block *parent_dir_data_block;
    unsigned char *current_char;
    int32_t char_count;
    unsigned char **loaded_data;
    int32_t written_blocks = 0;
    int32_t used_block_count = 0;
    int32_t *indirect1_references_to_write = NULL;
    int32_t *indirect2_references_to_write = NULL;
    int32_t indirect1_references_counter = 0;
    int32_t indirect2_references_counter = 0;
    int i, j;

    if (!fs || !disk_file_path || !fs_file_path) {
        return 0;
    }

    parent_dir = parse_path(fs, fs_file_path, true);

    filename = get_last_part_of_path(fs, fs_file_path);

    if (!parent_dir) {
        printf("Path not found.\n");
        return 0;
    }

    if (find_diritem_in_dir_by_name(parent_dir, filename)) {
        printf("Parent directory already contains a subdirectory or file with this name.\n");
        return 0;
    }

    if (count_directory_contents(parent_dir) >= MAX_DIRECTORY_ITEMS) {
        printf("Directory has reached the maximum count of items.\n");
        return 0;
    }

    file = fopen(disk_file_path, "rb");
    if (!file) {
        printf("File not found.\n");
        return 0;
    }

    fseek(file, 0, SEEK_END);   // seek na konec souboru
    file_size = ftell(file);    // ziskani velikosti souboru pomoci offsetu
    fseek(file, 0, SEEK_SET);   // seek na zacatek souboru

    // vypocet kolik budeme potrebovat primych bloku
    remaining_bytes = file_size;
    for (i = DIRECT_REFERENCES_COUNT; i > 0; i--) {
        remaining_bytes -= DATA_BLOCK_SIZE_B;
        needed_direct_blocks++;

        if (remaining_bytes <= 0) {
            break;
        }
    }

    // vypocet kolik budeme potrebovat 1. neprimych bloku
    if (remaining_bytes > 0) {
        needed_indirect1_blocks++;

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            remaining_bytes -= DATA_BLOCK_SIZE_B;
            needed_indirect1_blocks++;

            if (remaining_bytes <= 0) {
                break;
            }
        }
    }

    // vypocet kolik budeme potrebovat 2. neprimych bloku
    if (remaining_bytes > 0) {
        needed_indirect2_blocks += 2;

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            if (remaining_bytes <= 0) {
                break;
            }
            for (j = MAX_DIRECTORY_REFERENCES; j > 0; j--) {
                remaining_bytes -= DATA_BLOCK_SIZE_B;
                needed_indirect2_blocks++;

                if (remaining_bytes <= 0) {
                    break;
                }
            }
            needed_indirect2_blocks++;
        }
    }

    free_inode = find_free_inode(fs);

    if (free_inode == 0) {
        printf("No free i-nodes have been found.\n");
        return 0;
    }

    // bloky celkem
    total_needed_blocks = needed_direct_blocks + needed_indirect1_blocks + needed_indirect2_blocks;
    total_needed_blocks_for_data = file_size / DATA_BLOCK_SIZE_B;

    if (file_size % DATA_BLOCK_SIZE_B != 0) {
        total_needed_blocks_for_data++;
    }

    free_data_blocks = find_free_data_blocks(fs, total_needed_blocks);

    if (!free_data_blocks) {
        printf("Not enough free data blocks have been found.\n");
        return 0;
    }

    fs->inodes[free_inode]->nodeid = free_inode + 1;
    fs->inodes[free_inode]->is_directory = false;
    fs->inodes[free_inode]->file_size = file_size;
    fs->inodes[free_inode]->references = 1;

    // zabrani primych odkazu
    if (needed_direct_blocks > 0) {
        fs->inodes[free_inode]->direct1 = free_data_blocks[0] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 1) {
        fs->inodes[free_inode]->direct2 = free_data_blocks[1] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 2) {
        fs->inodes[free_inode]->direct3 = free_data_blocks[2] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 3) {
        fs->inodes[free_inode]->direct4 = free_data_blocks[3] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 4) {
        fs->inodes[free_inode]->direct5 = free_data_blocks[4] + 1;
        used_block_count++;
    }

    set_bit(fs->bitmapi, free_inode);

    for (i = 0; i < total_needed_blocks; i++) {
        set_bit(fs->bitmapd, free_data_blocks[i]);
    }

    created_directory_item = create_directory_item(free_inode + 1, filename, NULL);
    add_file_to_directory(fs, parent_dir, created_directory_item);

    parent_dir_inode = fs->inodes[parent_dir->this_item->inode - 1];
    parent_dir_data_block = fs->data_blocks[parent_dir_inode->direct1 - 1];

    // zapis rodicovskeho adresare do fs->data_blocks
    write_dir_items_to_data_block(parent_dir_data_block, parent_dir->subdirectories, parent_dir->files);

    // nacteni dat ze souboru do poli velikosti datovych bloku
    loaded_data = (unsigned char**)calloc(total_needed_blocks_for_data, sizeof(unsigned char*));
    for (i = 0; i < total_needed_blocks_for_data; i++) {
        loaded_data[i] = (unsigned char*)calloc(DATA_BLOCK_SIZE_B, sizeof(unsigned char));
    }

    current_char = (unsigned char*)malloc(sizeof(unsigned char));
    char_count = 0;
    i = 0;
    j = 0;
    do
    {
        fread(current_char,sizeof(unsigned char),1,file);

        loaded_data[i][j] = *current_char;
        j++;

        if (j % DATA_BLOCK_SIZE_B == 0) {
            i++;
            j = 0;
        }

        char_count++;

        if (char_count == fs->inodes[free_inode]->file_size) {
            break;
        }

    }  while (1);

    fclose(file);

    // zapis souboru do fs->data_blocks
    // zapis primych odkazu
    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct1 - 1], loaded_data[written_blocks]);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct2 - 1], loaded_data[written_blocks]);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct3 - 1], loaded_data[written_blocks]);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct4 - 1], loaded_data[written_blocks]);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct5 - 1], loaded_data[written_blocks]);
        written_blocks++;
    }

    // zapis 1. neprimeho odkazu
    if (written_blocks < total_needed_blocks) {
        fs->inodes[free_inode]->indirect1 = free_data_blocks[used_block_count] + 1;
        used_block_count++;
        indirect1_references_to_write = (int32_t*)calloc(MAX_DIRECTORY_REFERENCES, sizeof(int32_t));

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            indirect1_references_to_write[indirect1_references_counter] = free_data_blocks[used_block_count] + 1;

            write_data_to_data_block(fs->data_blocks[free_data_blocks[used_block_count]], loaded_data[written_blocks]);
            written_blocks++;
            indirect1_references_counter++;
            used_block_count++;

            if (written_blocks >= total_needed_blocks_for_data) {
                break;
            }
        }

        write_references_to_data_block(fs->data_blocks[fs->inodes[free_inode]->indirect1 - 1], indirect1_references_to_write, indirect1_references_counter);

    }

    // zapis 2. neprimeho odkazu
    if (written_blocks < total_needed_blocks_for_data) {
        fs->inodes[free_inode]->indirect2 = free_data_blocks[used_block_count] + 1;
        used_block_count++;

        memset(indirect1_references_to_write, 0, MAX_DIRECTORY_REFERENCES);
        indirect1_references_counter = 0;

        indirect2_references_to_write = (int32_t*)calloc(MAX_DIRECTORY_REFERENCES, sizeof(int32_t));

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            if (written_blocks >= total_needed_blocks_for_data) {
                break;
            }

            indirect2_references_to_write[indirect2_references_counter] = free_data_blocks[used_block_count] + 1;
            used_block_count++;
            indirect2_references_counter++;

            for (j = 0; j < MAX_DIRECTORY_REFERENCES; j++) {
                indirect1_references_to_write[indirect1_references_counter] = free_data_blocks[used_block_count] + 1;

                write_data_to_data_block(fs->data_blocks[free_data_blocks[used_block_count]], loaded_data[written_blocks]);
                written_blocks++;
                indirect1_references_counter++;
                used_block_count++;

                if (written_blocks >= total_needed_blocks_for_data) {
                    break;
                }
            }
            write_references_to_data_block(fs->data_blocks[indirect2_references_to_write[indirect2_references_counter - 1] - 1], indirect1_references_to_write, indirect1_references_counter);
            memset(indirect1_references_to_write, 0, MAX_DIRECTORY_REFERENCES);
            indirect1_references_counter = 0;
        }

        write_references_to_data_block(fs->data_blocks[fs->inodes[free_inode]->indirect2 - 1], indirect2_references_to_write, indirect2_references_counter);
    }

    // uvolneni pameti
    for (i = 0; i < total_needed_blocks_for_data; i++) {
        free(loaded_data[i]);
    }
    free(loaded_data);

    if (indirect1_references_to_write != NULL) {
        free(indirect1_references_to_write);
    }

    if (indirect2_references_to_write != NULL) {
        free(indirect2_references_to_write);
    }

    free(filename);
    free(free_data_blocks);

    // zapis vfs do souboru
    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    printf("OK\n");
    return 1;

}

/*
 * Vypise obsah souboru na ceste filepath ve file systemu fs.
 */
int command_concatenate(vfs *fs, char *filepath) {
    directory *parent_dir;
    directory_item *file;
    char *filename;
    inode *inode;
    unsigned char *output;
    int32_t i;

    if (!fs || !filepath) {
        return 0;
    }

    parent_dir = parse_path(fs, filepath, true);
    filename = get_last_part_of_path(fs, filepath);
    file = find_diritem_in_dir_by_name(parent_dir, filename);
    file = find_symlink_target_file(fs, file);

    if (!file) {
        printf("File not found.\n");
        return 0;
    }

    inode = fs->inodes[file->inode - 1];

    if (inode->is_directory) {
        printf("Cannot concatenate a directory.\n");
        return 0;
    }

    output = get_all_data_as_string(fs, inode);

    for (i = 0; i < inode->file_size; i++) {
        printf("%c", output[i]);
    }

    printf("\n");

    free(output);
    free(filename);

    return 1;
}

/*
 * Nahraje soubor na ceste fs_file_path ve file systemu fs do umisteni disk_file_path na pevnem disku.
 */
int command_out_copy(vfs *fs, char *fs_file_path, char *disk_file_path) {
    directory *parent_dir;
    char *filename;
    directory_item *fs_file;
    inode *inode;
    FILE *fout;
    unsigned char *output;

    if (!fs || !disk_file_path || !fs_file_path) {
        return 0;
    }

    parent_dir = parse_path(fs, fs_file_path, true);

    if (!parent_dir) {
        printf("File not found.\n");
        return 0;
    }

    filename = get_last_part_of_path(fs, fs_file_path);
    fs_file = find_diritem_in_dir_by_name(parent_dir, filename);
    fs_file = find_symlink_target_file(fs, fs_file);

    if (!fs_file) {
        printf("File not found.\n");
        return 0;
    }

    inode = fs->inodes[fs_file->inode - 1];

    if (inode->is_directory) {
        printf("File not found.\n");
        return 0;
    }

    fout = fopen(disk_file_path, "wb");

    if (!fout) {
        printf("Path not found.\n");
        return 0;
    }

    output = get_all_data_as_string(fs, inode);
    fwrite(output, sizeof(unsigned char), inode->file_size, fout);
    fclose(fout);

    free(filename);
    free(output);

    printf("OK\n");
    return 1;
}

/*
 * Smaze soubor na ceste path ve file systemu fs.
 * Timto prikazem nejdou smazat adresare, pro ty je nutne pouzit rmdir.
 */
int command_remove_file(vfs *fs, char *path) {
    directory *parent_dir;
    char *filename;
    directory_item *file_to_remove;
    int32_t *indirect1_blocks = NULL;
    int32_t indirect1_blocks_count = 0;
    int32_t *indirect2_blocks = NULL;
    int32_t indirect2_blocks_count = 0;
    inode *freed_inode;
    int32_t i;
    int32_t freed_inode_index;
    inode *parent_dir_inode;
    data_block *parent_dir_data_block;

    if (!fs || !path) {
        return 0;
    }

    parent_dir = parse_path(fs, path, true);

    if (!parent_dir) {
        printf("File not found.\n");
        return 0;
    }

    filename = get_last_part_of_path(fs, path);
    file_to_remove = find_diritem_in_dir_by_name(parent_dir, filename);
    //file_to_remove = find_symlink_target(fs, file_to_remove);

    if (!file_to_remove) {
        printf("File not found.\n");
        return 0;
    }

    freed_inode = fs->inodes[file_to_remove->inode - 1];

    if (freed_inode->is_directory) {
        printf("Use *rmdir* to remove a directory.\n");
        return 0;
    }

    if (freed_inode->direct1 != 0) {
        memset(fs->data_blocks[freed_inode->direct1 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->direct1 - 1);
    }

    if (freed_inode->direct2 != 0) {
        memset(fs->data_blocks[freed_inode->direct2 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->direct2 - 1);
    }

    if (freed_inode->direct3 != 0) {
        memset(fs->data_blocks[freed_inode->direct3 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->direct3 - 1);
    }

    if (freed_inode->direct4 != 0) {
        memset(fs->data_blocks[freed_inode->direct4 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->direct4 - 1);
    }

    if (freed_inode->direct5 != 0) {
        memset(fs->data_blocks[freed_inode->direct5 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->direct5 - 1);
    }

    if (freed_inode->indirect1 != 0) {
        indirect1_blocks = find_all_indirect1_data_blocks(fs->data_blocks[freed_inode->indirect1 - 1], &indirect1_blocks_count);

        for (i = 0; i < indirect1_blocks_count; i++) {
            memset(fs->data_blocks[indirect1_blocks[i] - 1]->data, 0, DATA_BLOCK_SIZE_B);
            clear_bit(fs->bitmapd, indirect1_blocks[i] - 1);
        }

        memset(fs->data_blocks[freed_inode->indirect1 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->indirect1 - 1);
    }

    indirect1_blocks_count = 0;

    if (freed_inode->indirect2 != 0) {
        indirect2_blocks = find_all_indirect2_data_blocks(fs, fs->data_blocks[freed_inode->indirect2 - 1], &indirect2_blocks_count);

        for (i = 0; i < indirect2_blocks_count; i++) {
            memset(fs->data_blocks[indirect2_blocks[i] - 1]->data, 0, DATA_BLOCK_SIZE_B);
            clear_bit(fs->bitmapd, indirect2_blocks[i] - 1);
        }

        indirect1_blocks = find_all_indirect1_data_blocks(fs->data_blocks[freed_inode->indirect2 - 1], &indirect1_blocks_count);

        for (i = 0; i < indirect1_blocks_count; i++) {
            memset(fs->data_blocks[indirect1_blocks[i] - 1]->data, 0, DATA_BLOCK_SIZE_B);
            clear_bit(fs->bitmapd, indirect1_blocks[i] - 1);
        }

        memset(fs->data_blocks[freed_inode->indirect2 - 1]->data, 0, DATA_BLOCK_SIZE_B);
        clear_bit(fs->bitmapd, freed_inode->indirect2 - 1);
    }

    freed_inode_index = freed_inode->nodeid - 1;

    remove_file_from_directory(fs, parent_dir, file_to_remove);

    memset(fs->inodes[freed_inode_index], 0, sizeof(inode));

    clear_bit(fs->bitmapi, freed_inode_index);

    parent_dir_inode = fs->inodes[parent_dir->this_item->inode - 1];
    parent_dir_data_block = fs->data_blocks[parent_dir_inode->direct1 - 1];

    // zapis do fs->data_blocks
    write_dir_items_to_data_block(parent_dir_data_block, parent_dir->subdirectories, parent_dir->files);

    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    if (indirect1_blocks != NULL) {
        free(indirect1_blocks);
    }

    if (indirect2_blocks != NULL) {
        free(indirect2_blocks);
    }

    free(filename);

    printf("OK\n");
    return 1;
}

/*
 * Presune soubor na ceste file do umisteni path ve file systemu fs.
 */
int command_move(vfs *fs, char *file, char *path) {
    directory *parent_dir;
    directory_item *file_to_alter;
    char *filename;
    directory *dir_for_moving;
    char *new_filename;
    inode *parent_dir_inode;
    data_block *parent_dir_data_block;
    inode *moving_dir_inode;
    data_block *moving_dir_data_block;
    directory_item *moved_directory_item;

    if (!fs || !file || !path) {
        return 0;
    }

    parent_dir = parse_path(fs, file, true);

    if (!parent_dir) {
        printf("File not found.\n");
        return 0;
    }

    filename = get_last_part_of_path(fs, file);
    file_to_alter = find_diritem_in_dir_by_name(parent_dir, filename);
    file_to_alter = find_symlink_target_file(fs, file_to_alter);

    if (!file_to_alter) {
        printf("File not found.\n");
        return 0;
    }

    dir_for_moving = parse_path(fs, path, true);

    if (!dir_for_moving) {
        printf("Path not found.\n");
        return 0;
    }

    new_filename = get_last_part_of_path(fs, path);

    if (find_diritem_in_dir_by_name(dir_for_moving, new_filename) != NULL) {
        printf("Directory already contains a file with this name.\n");
        return 0;
    }

    parent_dir_inode = fs->inodes[parent_dir->this_item->inode - 1];
    parent_dir_data_block = fs->data_blocks[parent_dir_inode->direct1 - 1];

    moving_dir_inode = fs->inodes[dir_for_moving->this_item->inode - 1];
    moving_dir_data_block = fs->data_blocks[moving_dir_inode->direct1 - 1];

    moved_directory_item = (directory_item*)malloc(sizeof(directory_item));
    moved_directory_item->inode = file_to_alter->inode;
    strncpy(moved_directory_item->name, new_filename, FILENAME_LENGTH);
    moved_directory_item->next = NULL;

    if (fs->inodes[file_to_alter->inode - 1]->is_directory) {
        remove_subdirectory_from_directory(fs, parent_dir, file_to_alter);
        add_subdirectory_to_directory(fs, dir_for_moving, moved_directory_item);
    }
    else {
        remove_file_from_directory(fs, parent_dir, file_to_alter);
        add_file_to_directory(fs, dir_for_moving, moved_directory_item);
    }

    // zapis do fs->data_blocks
    write_dir_items_to_data_block(parent_dir_data_block, parent_dir->subdirectories, parent_dir->files);
    write_dir_items_to_data_block(moving_dir_data_block, dir_for_moving->subdirectories, dir_for_moving->files);

    free(filename);
    free(new_filename);

    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    printf("OK\n");
    return 1;
}

/*
 * Zkopiruje soubor na ceste file_path do umisteni copy_path ve file systemu fs.
 */
int command_copy(vfs *fs, char *file_path, char *copy_path) {
    directory_item *file_to_copy;
    char *filename;
    char *copy_filename;
    directory *parent_dir;
    directory *dir_for_copying;
    inode *copying_dir_inode;
    data_block *copying_dir_data_block;
    int32_t file_size;
    int32_t remaining_bytes;
    int32_t needed_direct_blocks = 0;
    int32_t needed_indirect1_blocks = 0;
    int32_t needed_indirect2_blocks = 0;
    int32_t total_needed_blocks = 0;
    int32_t total_needed_blocks_for_data = 0;
    int32_t free_inode;
    int32_t *free_data_blocks;
    directory_item *created_directory_item;
    unsigned char **loaded_data;
    int32_t written_blocks = 0;
    int32_t used_block_count = 0;
    int32_t *indirect1_references_to_write = NULL;
    int32_t *indirect2_references_to_write = NULL;
    int32_t indirect1_references_counter = 0;
    int32_t indirect2_references_counter = 0;
    int i, j;

    if (!fs || !file_path || !copy_path) {
        return 0;
    }

    parent_dir = parse_path(fs, file_path, true);

    if (!parent_dir) {
        printf("File not found.\n");
        return 0;
    }

    filename = get_last_part_of_path(fs, file_path);
    file_to_copy = find_diritem_in_dir_by_name(parent_dir, filename);
    file_to_copy = find_symlink_target_file(fs, file_to_copy);

    if (!file_to_copy) {
        printf("File not found.\n");
        return 0;
    }

    if (fs->inodes[file_to_copy->inode - 1]->is_directory) {
        printf("Cannot copy a directory.\n");
        return 0;
    }

    dir_for_copying = parse_path(fs, copy_path, true);

    if (!dir_for_copying) {
        printf("Path not found.\n");
        return 0;
    }

    copy_filename = get_last_part_of_path(fs, copy_path);

    if (find_diritem_in_dir_by_name(dir_for_copying, copy_filename) != NULL) {
        printf("Directory already contains a file with this name.\n");
        return 0;
    }

    file_size = fs->inodes[file_to_copy->inode - 1]->file_size;

    // vypocet kolik budeme potrebovat primych bloku
    remaining_bytes = file_size;
    for (i = DIRECT_REFERENCES_COUNT; i > 0; i--) {
        remaining_bytes -= DATA_BLOCK_SIZE_B;
        needed_direct_blocks++;

        if (remaining_bytes <= 0) {
            break;
        }
    }

    // vypocet kolik budeme potrebovat 1. neprimych bloku
    if (remaining_bytes > 0) {
        needed_indirect1_blocks++;

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            remaining_bytes -= DATA_BLOCK_SIZE_B;
            needed_indirect1_blocks++;

            if (remaining_bytes <= 0) {
                break;
            }
        }
    }

    // vypocet kolik budeme potrebovat 2. neprimych bloku
    if (remaining_bytes > 0) {
        needed_indirect2_blocks += 2;

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            if (remaining_bytes <= 0) {
                break;
            }
            for (j = MAX_DIRECTORY_REFERENCES; j > 0; j--) {
                remaining_bytes -= DATA_BLOCK_SIZE_B;
                needed_indirect2_blocks++;

                if (remaining_bytes <= 0) {
                    break;
                }
            }
            needed_indirect2_blocks++;
        }
    }

    free_inode = find_free_inode(fs);

    if (free_inode == 0) {
        printf("No free i-nodes have been found.\n");
        return 0;
    }

    // bloky celkem
    total_needed_blocks = needed_direct_blocks + needed_indirect1_blocks + needed_indirect2_blocks;
    total_needed_blocks_for_data = file_size / DATA_BLOCK_SIZE_B;

    if (file_size % DATA_BLOCK_SIZE_B != 0) {
        total_needed_blocks_for_data++;
    }

    free_data_blocks = find_free_data_blocks(fs, total_needed_blocks);

    if (!free_data_blocks) {
        printf("Not enough free data blocks have been found.\n");
        return 0;
    }

    fs->inodes[free_inode]->nodeid = free_inode + 1;
    fs->inodes[free_inode]->is_directory = false;
    fs->inodes[free_inode]->file_size = file_size;
    fs->inodes[free_inode]->references = 1;

    // zabrani primych odkazu
    if (needed_direct_blocks > 0) {
        fs->inodes[free_inode]->direct1 = free_data_blocks[0] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 1) {
        fs->inodes[free_inode]->direct2 = free_data_blocks[1] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 2) {
        fs->inodes[free_inode]->direct3 = free_data_blocks[2] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 3) {
        fs->inodes[free_inode]->direct4 = free_data_blocks[3] + 1;
        used_block_count++;
    }

    if (needed_direct_blocks > 4) {
        fs->inodes[free_inode]->direct5 = free_data_blocks[4] + 1;
        used_block_count++;
    }

    set_bit(fs->bitmapi, free_inode);

    for (i = 0; i < total_needed_blocks; i++) {
        set_bit(fs->bitmapd, free_data_blocks[i]);
    }

    created_directory_item = create_directory_item(free_inode + 1, copy_filename, NULL);
    add_file_to_directory(fs, dir_for_copying, created_directory_item);

    copying_dir_inode = fs->inodes[dir_for_copying->this_item->inode - 1];
    copying_dir_data_block = fs->data_blocks[copying_dir_inode->direct1 - 1];

    // zapis rodicovskeho adresare do fs->data_blocks
    write_dir_items_to_data_block(copying_dir_data_block, dir_for_copying->subdirectories, dir_for_copying->files);

    // nacteni dat do poli velikosti datovych bloku
    loaded_data = get_all_data_as_blocks(fs, fs->inodes[file_to_copy->inode - 1]);

    // zapis souboru do fs->data_blocks
    // zapis primych odkazu
    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct1 - 1], fs->data_blocks[fs->inodes[file_to_copy->inode - 1]->direct1 - 1]->data);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct2 - 1], fs->data_blocks[fs->inodes[file_to_copy->inode - 1]->direct2 - 1]->data);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct3 - 1], fs->data_blocks[fs->inodes[file_to_copy->inode - 1]->direct3 - 1]->data);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct4 - 1], fs->data_blocks[fs->inodes[file_to_copy->inode - 1]->direct4 - 1]->data);
        written_blocks++;
    }

    if (written_blocks < total_needed_blocks_for_data) {
        write_data_to_data_block(fs->data_blocks[fs->inodes[free_inode]->direct5 - 1], fs->data_blocks[fs->inodes[file_to_copy->inode - 1]->direct5 - 1]->data);
        written_blocks++;
    }

    // zapis 1. neprimeho odkazu
    if (written_blocks < total_needed_blocks) {
        fs->inodes[free_inode]->indirect1 = free_data_blocks[used_block_count] + 1;
        used_block_count++;
        indirect1_references_to_write = (int32_t*)calloc(MAX_DIRECTORY_REFERENCES, sizeof(int32_t));

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            indirect1_references_to_write[indirect1_references_counter] = free_data_blocks[used_block_count] + 1;

            write_data_to_data_block(fs->data_blocks[free_data_blocks[used_block_count]], loaded_data[written_blocks]);
            written_blocks++;
            indirect1_references_counter++;
            used_block_count++;

            if (written_blocks >= total_needed_blocks_for_data) {
                break;
            }
        }

        write_references_to_data_block(fs->data_blocks[fs->inodes[free_inode]->indirect1 - 1], indirect1_references_to_write, indirect1_references_counter);

    }

    // zapis 2. neprimeho odkazu
    if (written_blocks < total_needed_blocks_for_data) {
        fs->inodes[free_inode]->indirect2 = free_data_blocks[used_block_count] + 1;
        used_block_count++;

        memset(indirect1_references_to_write, 0, MAX_DIRECTORY_REFERENCES);
        indirect1_references_counter = 0;

        indirect2_references_to_write = (int32_t*)calloc(MAX_DIRECTORY_REFERENCES, sizeof(int32_t));

        for (i = MAX_DIRECTORY_REFERENCES; i > 0; i--) {
            if (written_blocks >= total_needed_blocks_for_data) {
                break;
            }

            indirect2_references_to_write[indirect2_references_counter] = free_data_blocks[used_block_count] + 1;
            used_block_count++;
            indirect2_references_counter++;

            for (j = 0; j < MAX_DIRECTORY_REFERENCES; j++) {
                indirect1_references_to_write[indirect1_references_counter] = free_data_blocks[used_block_count] + 1;

                write_data_to_data_block(fs->data_blocks[free_data_blocks[used_block_count]], loaded_data[written_blocks]);
                written_blocks++;
                indirect1_references_counter++;
                used_block_count++;

                if (written_blocks >= total_needed_blocks_for_data) {
                    break;
                }
            }
            write_references_to_data_block(fs->data_blocks[indirect2_references_to_write[indirect2_references_counter - 1] - 1], indirect1_references_to_write, indirect1_references_counter);
            memset(indirect1_references_to_write, 0, MAX_DIRECTORY_REFERENCES);
            indirect1_references_counter = 0;
        }

        write_references_to_data_block(fs->data_blocks[fs->inodes[free_inode]->indirect2 - 1], indirect2_references_to_write, indirect2_references_counter);
    }

    // uvolneni pameti
    for (i = 0; i < total_needed_blocks_for_data; i++) {
        free(loaded_data[i]);
    }
    free(loaded_data);

    if (indirect1_references_to_write != NULL) {
        free(indirect1_references_to_write);
    }

    if (indirect2_references_to_write != NULL) {
        free(indirect2_references_to_write);
    }

    free(filename);
    free(copy_filename);
    free(free_data_blocks);

    // zapis vfs do souboru
    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    printf("OK\n");
    return 1;
}

/*
 * Vytvori symbolicky odkaz na soubor na ceste existing_file_path s cestou a nazvem danym symbolic_file_path ve file systemu fs.
 */
int command_sym_link(vfs *fs, char *existing_file_path, char *symbolic_file_path) {
    directory *existing_file_parent_dir;
    inode *existing_file_parent_dir_inode;
    data_block *existing_file_parent_dir_data_block;
    directory_item *existing_file;
    char *existing_file_name;
    char *existing_file_absolute_path;
    directory *sym_file_parent_dir;
    inode *sym_file_parent_dir_inode;
    data_block *sym_file_parent_dir_data_block;
    int32_t free_inode;
    int32_t *free_data_blocks;
    int32_t free_data_block;
    directory_item *created_sym_file_directory_item;
    data_block *sym_file_data_block;
    char *sym_file_name;
    directory *created_sym_file_directory;  // jen pokud delame symbolicky link na adresar

    if (!fs || !existing_file_path || !symbolic_file_path) {
        return 0;
    }

    existing_file_parent_dir = parse_path(fs, existing_file_path, true);

    if (!existing_file_parent_dir) {
        printf("File not found.\n");
        return 0;
    }

    existing_file_name = get_last_part_of_path(fs, existing_file_path);
    existing_file = find_diritem_in_dir_by_name(existing_file_parent_dir, existing_file_name);
    existing_file = find_symlink_target_file(fs, existing_file);

    if (!existing_file) {
        printf("File not found.\n");
        return 0;
    }

    sym_file_parent_dir = parse_path(fs, symbolic_file_path, true);

    if (!sym_file_parent_dir) {
        printf("Path not found.\n");
        return 0;
    }

    if (count_directory_contents(sym_file_parent_dir) >= MAX_DIRECTORY_ITEMS) {
        printf("Directory has reached the maximum count of items.\n");
        return 0;
    }

    sym_file_name = get_last_part_of_path(fs, symbolic_file_path);

    if (strchr(sym_file_name, '/') != NULL) {
        printf("File name cannot contain the / character.\n");
        return 0;
    }

    if (find_diritem_in_dir_by_name(sym_file_parent_dir, sym_file_name) != NULL) {
        printf("Directory already contains a file with this name.\n");
        return 0;
    }

    free_inode = find_free_inode(fs);

    if (free_inode == 0) {
        printf("No free i-nodes have been found.\n");
        return 0;
    }

    free_data_blocks = find_free_data_blocks(fs, 1);

    if (!free_data_blocks) {
        printf("Not enough free data blocks have been found.\n");
        return 0;
    }

    free_data_block = free_data_blocks[0];

    created_sym_file_directory_item = create_directory_item(free_inode + 1, sym_file_name, NULL);

    if (fs->inodes[existing_file->inode - 1]->is_directory) {
        created_sym_file_directory = create_directory(NULL, NULL, sym_file_parent_dir, created_sym_file_directory_item);
    }

    fs->inodes[free_inode]->nodeid = free_inode + 1;
    fs->inodes[free_inode]->is_directory = fs->inodes[existing_file->inode - 1]->is_directory;
    fs->inodes[free_inode]->is_symlink = true;
    fs->inodes[free_inode]->direct1 = free_data_block + 1;
    fs->inodes[free_inode]->file_size = DATA_BLOCK_SIZE_B;
    fs->inodes[free_inode]->references = 1;

    // vytvoreni absolutni cesty existujiciho souboru
    existing_file_absolute_path = (char*)calloc(STRING_LENGTH, sizeof(char));
    strcat(existing_file_absolute_path, find_absolute_path(existing_file_parent_dir));
    strcat(existing_file_absolute_path, existing_file_name);

    // zapis absolutni cesty do datoveho bloku symbolickeho odkazu
    sym_file_data_block = fs->data_blocks[free_data_block];
    write_data_to_data_block(sym_file_data_block, (unsigned char*)existing_file_path);

    set_bit(fs->bitmapi, free_inode);
    set_bit(fs->bitmapd, free_data_block);

    if (fs->inodes[existing_file->inode - 1]->is_directory) {
        fs->all_directories[fs->inodes[free_inode]->nodeid] = created_sym_file_directory;
        add_subdirectory_to_directory(fs, sym_file_parent_dir, created_sym_file_directory_item);
    }
    else {
        add_file_to_directory(fs, sym_file_parent_dir, created_sym_file_directory_item);
    }

    existing_file_parent_dir_inode = fs->inodes[existing_file_parent_dir->this_item->inode - 1];
    existing_file_parent_dir_data_block = fs->data_blocks[existing_file_parent_dir_inode->direct1 - 1];

    sym_file_parent_dir_inode = fs->inodes[sym_file_parent_dir->this_item->inode - 1];
    sym_file_parent_dir_data_block = fs->data_blocks[sym_file_parent_dir_inode->direct1 - 1];

    // zapis do fs->data_blocks
    write_dir_items_to_data_block(existing_file_parent_dir_data_block, existing_file_parent_dir->subdirectories, existing_file_parent_dir->files);
    write_dir_items_to_data_block(sym_file_parent_dir_data_block, sym_file_parent_dir->subdirectories, sym_file_parent_dir->files);

    if (!write_vfs_to_file(fs)) {
        printf("There was an error writing to the VFS file.\n");
        return 0;
    }

    free(existing_file_name);
    free(existing_file_absolute_path);
    free(sym_file_name);
    free(free_data_blocks);

    printf("OK\n");
    return 1;
}

/*
 * Uvolni struktury file systemu fs a ukonci program.
 */
int command_end(vfs *fs) {
    int32_t i;

    printf("Ending program.\n");
    free_bitmap(fs->bitmapd);
    free_bitmap(fs->bitmapi);

    for (i = 0; i < INODE_COUNT; i++) {
        free(fs->inodes[i]);
    }

    for (i = 0; i < fs->superblock->data_block_count; i++) {
        free(fs->data_blocks[i]);
    }

    for (i = 1; i < INODE_COUNT + 1; i++) {
        if (fs->all_directories[i] != NULL) {
            free(fs->all_directories[i]);
        }
    }

    free(fs->superblock);
    free(fs);
    exit(EXIT_SUCCESS);
    return 1;
}

/*
 * Pokusi se vykonat prikaz command s parametry param1 a param2 ve file systemu fs.
 */
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
            return command_copy(fs, param1, param2);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_MOVE, command) == 0) {
        if (fs->loaded) {
            return command_move(fs, param1, param2);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_REMOVE, command) == 0) {
        if (fs->loaded) {
            return command_remove_file(fs, param1);
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
            return command_concatenate(fs, param1);
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
            return command_in_copy(fs, param1, param2);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_OUT_COPY, command) == 0) {
        if (fs->loaded) {
            return command_out_copy(fs, param1, param2);
        }
        else {
            return 0;
        }
    }
    else if (strcmp(COMMAND_LOAD, command) == 0) {
        if (fs->loaded) {
            return command_load(fs, param1);
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
            return command_sym_link(fs, param1, param2);
        }
        else {
            return 0;
        }
    }
    else {
        printf("Unknown command. Use the command *help* to list available commands.\n");
        return 0;
    }

    return 0;
}