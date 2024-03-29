/*
 * Struktura superblocku, ktery obsahuje informace o FS.
 */

#include <stdlib.h>
#include <string.h>
#include "superblock.h"
#include "inode.h"
#include "bitmap.h"

/*
 * Vytvori a vrati strukturu superblock.
 * Superblock obsahuje login autora signature, velikost disku disk_size, velikost jednoho datoveho bloku data_block_size,
 * pocet datovych bloku data_block_count a velikost bitmapy datovych bloku a i-uzlu v bytech.
 */
superblock *create_superblock(char *signature, int32_t disk_size, int32_t data_block_size, int32_t data_block_count, int32_t bitmapd_size_B, int32_t bitmapi_size_B) {
    if (!signature || disk_size <= 0 || data_block_size <= 0 || data_block_count <= 0 || bitmapd_size_B <= 0 || bitmapi_size_B <= 0) {
        return NULL;
    }

    superblock *sblock;

    sblock = (superblock*)calloc(1, sizeof(superblock));
    strncpy(sblock->signature, signature, 12);
    sblock->disk_size = disk_size;
    sblock->data_block_size = data_block_size;
    sblock->data_block_count = data_block_count;

    // bitmapa dat. bloku
    sblock->bitmapd_start_address = sizeof(superblock);

    // bitmapa i-uzlu
    sblock->bitmapi_start_address = sblock->bitmapd_start_address +  bitmapd_size_B;

    // i uzly
    sblock->inode_start_address = sblock->bitmapi_start_address + bitmapi_size_B;

    // datove bloky
    sblock->data_start_address = sblock->inode_start_address + INODE_COUNT*sizeof(inode);

    return sblock;

}