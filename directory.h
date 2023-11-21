/*
 * Struktury pro praci s adresari.
 */

#ifndef SP_ZOS_CPP_DIRECTORY_H
#define SP_ZOS_CPP_DIRECTORY_H

#define FILENAME_LENGTH 12
#define DATA_BLOCK_SIZE_B 1000

#include <cinttypes>

// struktura pro soubor / adresar, mapuje jmeno na inode
typedef struct dir_item {
    int32_t inode;                   // inode odpovídající souboru
    char name[FILENAME_LENGTH];      // 8+3 + /0 C/C++ ukoncovaci string znak
    dir_item *next;                 // dalsi directory_item ve zretezenem seznamu
} directory_item;

// struktura pro adresar, obsahuje soubory a podadresare, vsechny adresare ulozeny ve vfs->all_directories
typedef struct dir {
    directory_item *files;              // zretezeny seznam souboru
    directory_item *subdirectories;     // zretezeny seznam podadresaru
    dir *parent;                        // rodic adresare
    directory_item *this_item;          // prislusny directory_item
} directory;

#define MAX_DIRECTORY_ITEMS DATA_BLOCK_SIZE_B/sizeof(directory_item)
#define MAX_DIRECTORY_REFERENCES DATA_BLOCK_SIZE_B/sizeof(int32_t)

directory *create_directory(directory_item *files, directory_item *subdirectories, directory *parent, directory_item *this_item);
directory_item *create_directory_item(int32_t inode, char *name, directory_item *next);
int32_t count_directory_contents(directory *dir);

#endif //SP_ZOS_CPP_DIRECTORY_H