/*
 * Struktura pro ukladani samotnych dat ve FS.
 * Pole pro ukladani ma delku DATA_BLOCK_SIZE_B a je typu unsigned char, aby nedochazelo k zapornym hodnotam.
 */

#ifndef SP_ZOS_CPP_DATA_BLOCK_H
#define SP_ZOS_CPP_DATA_BLOCK_H

#include "directory.h"
#include "constants.h"

#define DATA_BLOCK_SIZE_B 1000

typedef struct {
    unsigned char data[DATA_BLOCK_SIZE_B];
} data_block;

data_block *create_data_block();
int write_dir_items_to_data_block(data_block *block, directory_item *subdirectories, directory_item *files);
bool data_block_empty(data_block *block);
int write_data_to_data_block(data_block *block, unsigned char *data);
int write_references_to_data_block(data_block *block, int32_t *references, int32_t references_count);
char *get_symlink_reference(data_block *block);
void free_data_block(data_block *block);

#endif //SP_ZOS_CPP_DATA_BLOCK_H
