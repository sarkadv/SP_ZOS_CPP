/*
 * Struktura pro i-uzel.
 */

#include <stdlib.h>
#include "inode.h"

/*
 * Vytvori i-uzel s ID nodeid.
 */
inode *create_inode(int32_t nodeid) {
    inode *in = (inode*)calloc(1, sizeof(inode));
    in->nodeid = nodeid;

    return in;
}
