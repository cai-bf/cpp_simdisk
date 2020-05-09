//
// Created by cbf on 20-2-20.
//
// 定义常量
#ifndef SIMDISK_CONST_H
#define SIMDISK_CONST_H


#define RO 1 // 只读
#define WO 2 // 只写
#define RW 3 // 读写

#define NUSED false // bitmap未使用
#define USED true  // bitmap 已使用

#define UNKNOWN 0 // inode尚未分配到某类型
#define FILE__ 1 // 文件类型
#define DIRECTORY 2 // 目录类型

//#define NOT_OPEN 0 // 未打开
//#define READING 1   // 在读
//#define WRITING 2  // 在写

#define MAX_NAME_LEN 64 // 最长文件名
#define MAX_FILE_NUM 128 // 目录可拥有最大文件数
#define MAX_USER_NUM 8 // 可拥有最多用户数

#define BLOCK_SIZE 1024 // 磁盘块大小 1kB
#define TOTAL_BLOCK_NUM 102400 // 总磁盘块数
#define TOTAL_INODE_NUM 102400 // 总inode数

#define FIRST_DATA_ADDR (sizeof(super_block) + \
                            sizeof(bool) * (TOTAL_INODE_NUM + TOTAL_BLOCK_NUM) + \
                            sizeof(inode) * TOTAL_INODE_NUM + \
                            sizeof(user) * MAX_USER_NUM) // 除去各种必须信息后，数据块的起始偏移量
#define DIR_SIZE (sizeof(dir) % BLOCK_SIZE == 0 ? \
                    (sizeof(dir) / BLOCK_SIZE) : (sizeof(dir) / BLOCK_SIZE + 1)) // 目录的大小

#endif //SIMDISK_CONST_H
