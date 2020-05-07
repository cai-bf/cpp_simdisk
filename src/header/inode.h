//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_INODE_H
#define SIMDISK_INODE_H

// inode描述
struct inode {
    unsigned short mode; // 对应文件类型
    unsigned short row; // 读写模式 read_or_write
    unsigned int uid; // 用户id
    unsigned int size; // 文件大小
    unsigned int blocks; // 分配block数目
    unsigned long block_addr; // 起始地址偏移量
};


extern struct inode inodeTable[TOTAL_INODE_NUM]; // inode表
extern bool inodeBitmap[TOTAL_INODE_NUM]; // inode位图

#endif //SIMDISK_INODE_H
