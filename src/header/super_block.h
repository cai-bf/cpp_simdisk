//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_SUPER_BLOCK_H
#define SIMDISK_SUPER_BLOCK_H

#include "global.h"

// 超级块结构， 保存磁盘的信息
struct super_block {
    unsigned int inodes_num; // inode数目
    unsigned int blocks_num; // block数目
    unsigned int block_size; // block大小
    unsigned int free_blocks_num; // 空闲block数目
    unsigned int free_inodes_num; // 空闲inode数目
};

extern struct super_block superBlock; // 超级块
extern bool blockBitmap[TOTAL_BLOCK_NUM]; // block位图

#endif //SIMDISK_SUPER_BLOCK_H
