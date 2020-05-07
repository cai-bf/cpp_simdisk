//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_FILE_H
#define SIMDISK_FILE_H

#include "const.h"

// file描述
struct file {
    char name[MAX_NAME_LEN];
    int inode_idx; // inode表下标
};

#endif //SIMDISK_FILE_H
