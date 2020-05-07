//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_SYSTEM_H
#define SIMDISK_SYSTEM_H

#include "unity.h"

void initial(); // 启动时初始化数据

void generate(); // 第一次启动时创建并初始化文件

void login(std::vector<std::string>);

void logout(std::vector<std::string>);

void help(std::vector<std::string>);

void display_sys_info(std::vector<std::string>);

#endif //SIMDISK_SYSTEM_H
