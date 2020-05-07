//
// Created by cbf on 20-2-20.
//
#include "../header/system.h"
#include <errno.h>
#include <string.h>


/*
 * 创建文件并初始化
 */
void generate() {
    // 分配空间
    long file_size = FIRST_DATA_ADDR + BLOCK_SIZE * TOTAL_BLOCK_NUM;
    char *buf = new char[file_size];
    // 初始化文件
    fd = fopen(FILE_NAME, "wb+");
    if (fd == nullptr) {
        printf("创建磁盘文件失败： %s \n", strerror(errno));
        exit(1);
    }
    fseek(fd, 0, SEEK_SET); // 指针指向文件头
    fwrite(buf, file_size, 1, fd);
    delete[](buf);
    fclose(fd);

    // 创建用户
    for (int i = 0; i < MAX_USER_NUM; ++i) {
        users[i].uid = 666666 + i;
        sprintf(users[i].name, "user_%d", i);
        strcpy(users[i].password, "password");
    }

    // 初始化超级块
    superBlock.blocks_num = TOTAL_BLOCK_NUM;
    superBlock.inodes_num = TOTAL_INODE_NUM;
    superBlock.block_size = BLOCK_SIZE;
    superBlock.free_blocks_num = TOTAL_BLOCK_NUM - DIR_SIZE * (MAX_USER_NUM + 1);
    superBlock.free_inodes_num = TOTAL_INODE_NUM - 1 - MAX_USER_NUM;

    // 初始化 bitmap & inode table
    for (bool & j : blockBitmap) {
        j = NUSED;
    }
    for (bool & i : inodeBitmap) {
        i = NUSED;
    }
    for (auto & i : inodeTable) {
        i.mode = UNKNOWN;
        i.row = RO;
        i.uid = 0;
        i.size = 0;
        i.blocks = 0;
        i.block_addr = 0;
    }
    for (int i = 0; i < DIR_SIZE * (1 + MAX_USER_NUM); ++i) {
        blockBitmap[i] = USED;
    }
    for (int k = 0; k < 1 + MAX_USER_NUM; ++k) {
        inodeBitmap[k] = USED;
        inodeTable[k].mode = DIRECTORY;
        inodeTable[k].size = sizeof(dir);
        inodeTable[k].blocks = DIR_SIZE;
        inodeTable[k].block_addr = FIRST_DATA_ADDR + DIR_SIZE * k * BLOCK_SIZE;
        inodeTable[k].uid = k > 0 ? users[k-1].uid : 0;
    }


    // 创建根目录
    strcpy(currDir.name, "/");
    currDir.inode_idx = 0; // 根目录inode表下标为0
    currDir.files_num = 2; // 存在两个文件
    strcpy(currDir.files[0].name, "."); // 当前目录指向自身
    currDir.files[0].inode_idx = 0;
    strcpy(currDir.files[1].name, ".."); // 上级目录指向自身
    currDir.files[1].inode_idx = 0;

    // 初始化用户目录
    dir userDir[MAX_USER_NUM];
    for (int i = 0; i < MAX_USER_NUM; ++i) {
        strcpy(userDir[i].name, users[i].name);
        userDir[i].inode_idx = i + 1;
        userDir[i].files_num = 2;
        strcpy(userDir[i].files[0].name, ".");
        userDir[i].files[0].inode_idx = i + 1;
        strcpy(userDir[i].files[1].name,"..");
        userDir[i].files[1].inode_idx = 0;
        currDir.files_num++;
        strcpy(currDir.files[i + 2].name, users[i].name);
        currDir.files[i + 2].inode_idx = i + 1;
    }


    // 初始化当前路径
    strcpy(currPath, "/");

    // 写入磁盘
    fd = fopen(FILE_NAME, "rb+");
    if (fd == nullptr) {
        printf("写入数据失败： %s \n", strerror(errno));
        exit(1);
    }
    fseek(fd, 0, SEEK_SET);
    fwrite(&superBlock, sizeof(super_block), 1, fd);
    fwrite(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fwrite(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fwrite(inodeTable, sizeof(inode), TOTAL_INODE_NUM, fd);
    fwrite(users, sizeof(user), MAX_USER_NUM, fd);
    fwrite(&currDir, sizeof(dir), 1, fd);
    for (auto & d : userDir) {
        fseek(fd, inodeTable[d.inode_idx].block_addr, SEEK_SET);
        fwrite(&d, sizeof(dir), 1, fd);
    }
    fclose(fd);
}


// 启动时初始化数据
void initial() {
    fd = fopen(FILE_NAME, "rb");
    if (fd == nullptr) { // 文件不存在， 即第一次启动程序， 创建文件
        generate();
    }
    fseek(fd, 0, SEEK_SET);
    fread(&superBlock, sizeof(super_block), 1, fd);
    fread(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fread(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fread(inodeTable, sizeof(inode), TOTAL_INODE_NUM, fd);
    fread(users, sizeof(user), MAX_USER_NUM, fd);
    fread(&currDir, sizeof(dir), 1, fd);
    fclose(fd);
}

void login(std::vector<std::string> argv) {
    if (argv.size() != 3) {
        dprintf(output, "login参数错误，请重新确认\n");
        return;
    }
    bool loged = false;
    for (user u : users) {
        if (strcmp(u.name, argv[1].c_str()) == 0 && strcmp(u.password, argv[2].c_str()) == 0) {
            loged = true;
            currUser = u;
        }
    }
    if (!loged) {
        dprintf(output, "login: 用户或密码错误，请重新确认\n");
        return;
    }
    cd_dir_inner(std::string(currUser.name));
    char r[20] = "/";
    strcat(r, currUser.name);
    strcpy(currPath, r);
    dprintf(output, "登陆成功\n");
}

void logout(std::vector<std::string> argv) {
    // TODO 任务一不需要该函数
}

void help(std::vector<std::string> argv) {
    std::string str;
    str += "---------------simdisk命令手册-----------------\n";
    str += "登录命令: login <username> <password>\n";
    str += "展示系统使用情况： info\n";
    // TODO 后续补充其他命令
    dprintf(output, str.c_str());
}

void display_sys_info(std::vector<std::string> argv) {
    char *buf = new char[200];
    sprintf(buf, "------simdisk文件系统情况------\n"
                 "磁盘空间: %15d KB\n"
                 "已用空间: %15d KB\n"
                 "剩余空间: %15d KB\n",
                 superBlock.blocks_num * BLOCK_SIZE / 1024,
                 (superBlock.blocks_num - superBlock.free_blocks_num) * BLOCK_SIZE / 1024,
                 superBlock.free_blocks_num * BLOCK_SIZE / 1024);
    dprintf(output, buf);
    delete[](buf);
}
