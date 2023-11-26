/*
 * Struktura pro datovy blok (ukladani samotnych dat ve FS).
 * Pole pro ukladani ma delku DATA_BLOCK_SIZE_B a je typu unsigned char, aby nedochazelo k zapornym hodnotam.
 */

#include <stdlib.h>
#include <string.h>
#include "data_block.h"

/*
 * Vytvori a vrati strukturu datoveho bloku.
 */
data_block *create_data_block() {
    data_block *db = (data_block*)calloc(1, sizeof(data_block));

    return db;
}

/*
 * Zapise vsechny podadresare subdirectories a soubory files do datoveho bloku prislusnemu adresari block.
 */
int write_dir_items_to_data_block(data_block *block, directory_item *subdirectories, directory_item *files) {
    int32_t offset = 0;

    if (!block) {
        return 0;
    }

    memset(block->data, 0, DATA_BLOCK_SIZE_B);

    while (subdirectories != NULL) {
        memcpy(block->data + offset, subdirectories, sizeof(directory_item));

        offset += sizeof(directory_item);
        subdirectories = subdirectories->next;
    }

    while (files != NULL) {
        memcpy(block->data + offset, files, sizeof(directory_item));

        offset += sizeof(directory_item);
        files = files->next;
    }

    return 1;
}

/*
 * Zapise data do datoveho bloku block.
 */
int write_data_to_data_block(data_block *block, unsigned char *data) {
    if (!block || !data) {
        return 0;
    }

    memset(block->data, 0, DATA_BLOCK_SIZE_B);
    memcpy(block->data, data, DATA_BLOCK_SIZE_B);

    return 1;
}

/*
 * Zapise reference na dalsi datove bloky references do datoveho bloku block.
 * Pocet referenci je refernces_count.
 */
int write_references_to_data_block(data_block *block, int32_t *references, int32_t references_count) {
    int i;

    if (!block || !references || references_count <= 0) {
        return 0;
    }

    memset(block->data, 0, DATA_BLOCK_SIZE_B);

    for (i = 0; i < references_count; i++) {
        memcpy(block->data + i*sizeof(int32_t), references + i, sizeof(int32_t));
    }

    return 1;
}

/*
 * Zkontroluje, zda je datovy blok prazdny (obsahuje same 0) nebo ne.
 */
bool data_block_empty(data_block *block) {
    int i;

    if (!block) {
        return false;
    }

    for (i = 0; i < DATA_BLOCK_SIZE_B; i++) {
        if (block->data[i] != '\0') {
            return false;
        }
    }

    return true;
}

/*
 * Vrati referenci na soubor z datoveho bloku block prislusnemu souboru typu symbolicky odkaz.
 */
char *get_symlink_reference(data_block *block) {
    char *result;

    if (!block) {
        return NULL;
    }

    result = (char*)calloc(STRING_LENGTH, sizeof(char));
    memcpy(result, block->data, STRING_LENGTH);

    return result;
}

/*
 * Uvolni strukturu datoveho bloku block.
 */
void free_data_block(data_block *block) {
    if (!block) {
        return;
    }

    free(block);
}
