//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_DIR_H
#define SIMDISK_DIR_H

#include "const.h"
#include "file.h"
#include "super_block.h"
#include "inode.h"
#include "user.h"



// dir描述
struct dir {
    char name[MAX_NAME_LEN];
    int inode_idx;  // inode table index
    int files_num; // 目录下包含文件数
    struct file files[MAX_FILE_NUM]; // 目录下文件描述入口
};


extern struct dir currDir; // 当前目录

bool getDir (std::string, dir&);

void cd_dir(std::vector<std::string>);

void cd_dir_inner(std::string);

void display_dir(std::vector<std::string>);

void remove_dir(std::vector<std::string>);

void make_dir(std::vector<std::string>);

void ls(std::vector<std::string>);

#endif //SIMDISK_DIR_H
