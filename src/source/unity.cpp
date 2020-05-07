//
// Created by cbf on 20-2-20.
//

#include "../header/unity.h"

struct super_block superBlock; // 超级块
struct inode inodeTable[TOTAL_INODE_NUM]; // inode表
struct user users[MAX_USER_NUM]; // 用户表
struct dir currDir; // 当前目录
bool blockBitmap[TOTAL_BLOCK_NUM]; // block位图
bool inodeBitmap[TOTAL_INODE_NUM]; // inode位图
FILE *fd = nullptr; // 文件指针
struct user currUser; // 当前用户
char currPath[1000]; // 当前路径

char FILE_NAME[20] = "linux_simdisk_file";


// 输出的fileno， 多shell时可重定向为对应的socket， 方便复用代码
int output = fileno(stdout);