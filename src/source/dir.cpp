#include <utility>

//
// Created by cbf on 20-2-21.
//

#include "../header/dir.h"
using namespace std;

/*
 * 将路径以‘/’分隔开，根据每一个子字符串判断应该向前寻找子目录还是向后返回目目录
 * 特别注意第一个字符需单独判断，因为可能是'/'根目录开头
 */
bool getDir (string str, dir& cur_dir) {
    // 路径分解
    vector<string> path_divide = split(str, "/");
    if (path_divide.empty() && str[0] != '/') {
        dprintf(output, "cd路径错误\n");
        return false;
    }

    dir tempDir = cur_dir;

    fd = fopen(FILE_NAME, "rb");
    // 判断是否根目录开头
    if (str[0] == '/') {
        fseek(fd, FIRST_DATA_ADDR, SEEK_SET);
        fread(&tempDir, sizeof(dir), 1, fd);
    }
    // 根据分解路径进行判断，迭代递进路径，遇到不存在的情况直接返回错误
    for (const string &s : path_divide) {
        bool success = false;
        // 遍历该目录下所有file，判断是否存在该dir
        for (int i = 0; i < tempDir.files_num; ++i) {
            if (strcmp(s.c_str(), tempDir.files[i].name) == 0 &&
                inodeTable[tempDir.files[i].inode_idx].mode == DIRECTORY) {
                fseek(fd, inodeTable[tempDir.files[i].inode_idx].block_addr, SEEK_SET);
                fread(&tempDir, sizeof(dir), 1, fd);
                success = true;
                break;
            }
        }
        if (!success) {
            fclose(fd);
            return false;
        }
    }
    fclose(fd);
    cur_dir = tempDir;
    return true;
}


/*
 * cd内部函数，供内部使用，不对外暴露
 * 一般用于用户登录初始化当前目录
 */
void cd_dir_inner(string str) {
    getDir(std::move(str), currDir);
}


/*
 * 调用getDir进入对应目录
 */
void cd_dir(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "cd参数错误\n");
        return;
    }

    bool success = getDir(argv[1], currDir);
    if (!success) {
        dprintf(output, "cd: 路径不存在\n");
        return;
    }
    // cd完成
    dprintf(output, "cd: 成功\n"); // TODO 后期修改为返回对应的路径
}


/*
 * 调用getDir获取对应目录，遍历目录获取所有内容信息
 */
void display_dir(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "dir参数错误\n");
        return;
    }

    dir tempDir = currDir;
    bool success = getDir(argv[1], tempDir);
    if (!success) {
        dprintf(output, "dir路径不存在\n");
        return;
    }
    string res;
    for (int i = 0; i < tempDir.files_num; i++) {
        res += string(tempDir.files[i].name) + "\t" +
                (inodeTable[tempDir.files[i].inode_idx].mode == DIRECTORY ? "dir\t" : "file\t") +
                to_string(inodeTable[tempDir.files[i].inode_idx].size) + "B \n";
    }
    dprintf(output, res.c_str());
}


/*
 * 递归判断目录是否有文件并删除
 */
void remove_dir (dir &rm_dir) {
    inode i = inodeTable[rm_dir.inode_idx];
    for (int j = 2; j < rm_dir.files_num; ++j) {
        // 判断是否为目录
        if (inodeTable[rm_dir.files[j].inode_idx].mode == DIRECTORY) {
            fd = fopen(FILE_NAME, "rb");
            fseek(fd, inodeTable[rm_dir.files[j].inode_idx].block_addr, SEEK_SET);
            dir sub_dir{};
            fread(&sub_dir, sizeof(dir), 1, fd);
            fclose(fd);
            remove_dir(sub_dir);
        } else { // file
            inode tmp = inodeTable[rm_dir.files[j].inode_idx];
            superBlock.free_blocks_num += tmp.blocks; // 归还空闲block
            superBlock.free_inodes_num++; // 归还空闲inode
            inodeBitmap[rm_dir.files[j].inode_idx] = NUSED; // 更新bitmap
            unsigned int blockIndex = (tmp.block_addr - FIRST_DATA_ADDR) / BLOCK_SIZE;
            for (int k = 0; k < tmp.blocks; ++k) {
                blockBitmap[blockIndex + k] = NUSED;
            }
        }
    }
    // 删除完子文件子目录， 将自身删除
    superBlock.free_blocks_num += i.blocks; // 归还空闲block
    superBlock.free_inodes_num++; // 归还空闲inode
    // 更新bitmap
    inodeBitmap[rm_dir.inode_idx] = NUSED;
    unsigned int blockIndex = (i.block_addr - FIRST_DATA_ADDR) / BLOCK_SIZE;
    for (int k = 0; k < i.blocks; ++k) {
        blockBitmap[blockIndex + k] = NUSED;
    }
}


/*
 * 获取要删除的目录，判断目录与当前目录是否一致，一致的话将当前目录修改至用户目录下。
 * 再判断是否时根目录或者用户目录或者属于其他用户的目录，是的话返回失败
 * 判断是否-f flag，所目录不为空且无强制flag，返回失败
 * 否则执行删除，标记inode为未被使用 并返回成功
 */
void remove_dir(vector<string> argv) {
    if (argv.size() != 2 && argv.size() != 3) {
        dprintf(output, "rd参数错误\n");
        return;
    }

    // 获取目录
    dir tempDir = currDir;
    bool find = getDir(argv.size() == 2 ? argv[1] : argv[2], tempDir);
    if (!find) {
        dprintf(output, "rd路径不存在\n");
        return;
    }

    // 判断当前目录所有者
    if (currUser.uid != inodeTable[tempDir.inode_idx].uid) {
        dprintf(output, "没有权限删除该目录\n");
        return;
    }

    // 判断是否用户主目录
    dir root_dir{};
    fd = fopen(FILE_NAME, "rb");
    fseek(fd, FIRST_DATA_ADDR, SEEK_SET);
    fread(&root_dir, sizeof(dir), 1, fd);
    for (file &f : root_dir.files) {
        if (f.inode_idx == tempDir.inode_idx) {
            dprintf(output, "无法删除用户主目录");
            fclose(fd);
            return;
        }
    }
    fclose(fd);

    // 如果用户当前处于该目录下， 将切换到用户主目录
    if (tempDir.inode_idx == currDir.inode_idx) {
        cd_dir_inner("/" + string(currUser.name));
    }

    // 判断是否强制递归删除
    if (argv[1] != "-f") {
        if (tempDir.files_num > 2) {
            dprintf(output, "目录不为空，若要继续删除，请加上-f，详情请输入help查看帮助\n");
            return;
        }
    }

    // 递归删除目录
    remove_dir(tempDir);
    // 删除上级目录的file项
    inode parentNode = inodeTable[tempDir.files[1].inode_idx];
    dir parent{};
    fd = fopen(FILE_NAME, "rb");
    fseek(fd, parentNode.block_addr, SEEK_SET);
    fread(&parent, sizeof(dir), 1, fd);
    fclose(fd);
    for (int i = 2; i < parent.files_num; ++i) {
        if (parent.files[i].inode_idx == tempDir.inode_idx) {
            for (int j = i; j < parent.files_num - 1; ++j) {
                parent.files[j].inode_idx = parent.files[j + 1].inode_idx;
                strcpy(parent.files[j].name, parent.files[j + 1].name);
            }
            parent.files_num--;
            break;
        }
    }

    // 更新文件信息
    fd = fopen(FILE_NAME, "rb+");
    fseek(fd, 0, SEEK_SET);
    fwrite(&superBlock, sizeof(super_block), 1, fd);
    fwrite(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fwrite(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fseek(fd, parentNode.block_addr, SEEK_SET);
    fwrite(&parent, sizeof(dir), 1, fd);
    fclose(fd);

    dprintf(output, "rd删除目录成功\n");
}

/*
 * 创建目录
 * 判断目录参数正确，判断权限
 * 判断是否空闲inode、block
 */
void make_dir(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "md: 参数错误\n");
        return;
    }

    // 找出最后一个目录
    dir curr = currDir;
    size_t pos = argv[1].find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            dprintf(output, "md: 没有权限\n");
            return;
        }
        bool succ = getDir(argv[1].substr(0, pos), curr);
        if (!succ) {
            dprintf(output, "md目录不存在, 当前md不支持嵌套创建目录");
            return;
        }
    }

    // 判断权限
    if (inodeTable[curr.inode_idx].uid != currUser.uid) {
        dprintf(output, "md: 没有权限\n");
        return;
    }

    // 获取空闲inode
    if (superBlock.free_inodes_num < 1) {
        dprintf(output, "md: inode不足\n");
        return;
    }
    int i = 0;
    for (; i < TOTAL_INODE_NUM; ++i) {
        if (inodeBitmap[i] == NUSED) {
            inodeBitmap[i] = USED;
            break;
        }
    }
    inodeTable[i].uid = currUser.uid;
    inodeTable[i].size = sizeof(dir);
    inodeTable[i].mode = DIRECTORY;
    inodeTable[i].blocks = DIR_SIZE;
    inodeTable[i].row = RW;
    // 找出连续的空闲块
    if (superBlock.free_blocks_num < DIR_SIZE) {
        dprintf(output, "md: 内存不足\n");
        return;
    }
    bool succ = false;
    int j = 0;
    for (; j < TOTAL_BLOCK_NUM; ++j) {
        if (blockBitmap[j] == NUSED) {
            int num = 0;
            for (int k = j; k < TOTAL_BLOCK_NUM; ++k) {
                if (blockBitmap[k] == NUSED) {
                    num++;
                    if (num == DIR_SIZE)
                        break;
                } else
                    break;
            }
            if (num == DIR_SIZE) {
                succ = true;
                break;
            }
        }
    }
    if (!succ) {
        dprintf(output, "md: 内存不足\n");
        return;
    }
    // 更新bitmap
    for (int l = 0; l < DIR_SIZE; ++l) {
        blockBitmap[j + l] = USED;
    }
    // 更新inode指向偏移量
    inodeTable[i].block_addr = FIRST_DATA_ADDR + j * BLOCK_SIZE;

    // 创建dir
    dir new_dir{};
    new_dir.inode_idx = i;
    strcpy(new_dir.name, argv[1].substr(pos + 1).c_str());
    new_dir.files_num = 2;
    strcpy(new_dir.files[0].name, ".");
    new_dir.files[0].inode_idx = new_dir.inode_idx;
    strcpy(new_dir.files[1].name, "..");
    new_dir.files[1].inode_idx = curr.inode_idx;

    // 更新父目录
    curr.files[curr.files_num].inode_idx = new_dir.inode_idx;
    strcpy(curr.files[curr.files_num].name, new_dir.name);
    curr.files_num++;

    // 更新super_block
    superBlock.free_inodes_num--;
    superBlock.free_blocks_num -= DIR_SIZE;

    // 写入文件
    fd = fopen(FILE_NAME, "rb+");
    fseek(fd, 0, SEEK_SET);
    fwrite(&superBlock, sizeof(super_block), 1, fd);
    fwrite(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fwrite(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fwrite(inodeTable, sizeof(inode), TOTAL_INODE_NUM, fd);
    fseek(fd, inodeTable[curr.inode_idx].block_addr, SEEK_SET);
    fwrite(&curr, sizeof(dir), 1, fd);
    fseek(fd, inodeTable[new_dir.inode_idx].block_addr, SEEK_SET);
    fwrite(&new_dir, sizeof(dir), 1, fd);
    fclose(fd);

    dprintf(output, "md: 创建目录成功\n");
}

void ls(vector<string> argv) {
    display_dir({"dir", "."});
}