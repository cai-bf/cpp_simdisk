//
// Created by cbf on 20-2-20.
//

#ifndef SIMDISK_USER_H
#define SIMDISK_USER_H


struct user {
    char name[10]; // 用户名
    unsigned int uid; // 用户id
    char password[20]; //密码
};


extern struct user users[MAX_USER_NUM]; // 用户表
extern struct user currUser; // 当前用户

#endif //SIMDISK_USER_H
