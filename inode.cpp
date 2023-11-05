//
// Created by Šári Dvořáková on 02.11.2023.
//

#include <stdlib.h>
#include "inode.h"

inode *create_inode(int32_t nodeid) {
    inode *in = (inode*)malloc(sizeof(inode));
    in->nodeid = nodeid;

    return in;
}
