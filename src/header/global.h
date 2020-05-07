//
// Created by cbf on 20-2-21.
//

#ifndef SIMDISK_GLOBAL_H
#define SIMDISK_GLOBAL_H

#include <stdio.h>
#include "helpers.h"

extern FILE *fd; // 文件指针

extern char currPath[1000]; // 当前路径

extern int output; // 输出的fileno

extern char FILE_NAME[20];

#endif //SIMDISK_GLOBAL_H
