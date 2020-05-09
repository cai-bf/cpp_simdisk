//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_FILE_H
#define SIMDISK_FILE_H

#include "const.h"
#include <vector>
#include <string>

// file描述
struct file {
    char name[MAX_NAME_LEN];
    int inode_idx; // inode表下标
};

void make_file(std::vector<std::string>);

void cat(std::vector<std::string>);

void copy(std::vector<std::string>);

void del_file(std::vector<std::string>);

#endif //SIMDISK_FILE_H
