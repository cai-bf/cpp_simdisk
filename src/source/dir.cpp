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
 * 内部函数，供内部使用， 不对外暴露
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
        dprintf(output, "cd路径不存在\n");
        return;
    }
    // cd完成
    dprintf(output, "cd成功\n"); // TODO 后期修改为返回对应的路径
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
    string res = argv[1] +  "目录下内容: \n";
    for (int i = 2; i < tempDir.files_num; i++) {
        res += "\t" + string(tempDir.files[i].name) + "\t" +
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
            dir sub_dir;
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
 * 否则标记inode为未被使用 并返回成功
 */
void remove_dir(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "rd参数错误\n");
        return;
    }

    // 获取目录
    dir tempDir;
    bool find = getDir(argv[1], tempDir);
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
    dir root_dir;
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

    // 递归删除目录
    remove_dir(tempDir);
    // 更新文件信息
    fd = fopen(FILE_NAME, "rb+");
    fseek(fd, 0, SEEK_SET);
    fwrite(&superBlock, sizeof(super_block), 1, fd);
    fwrite(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fwrite(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fclose(fd);

    dprintf(output, "rd删除目录成功\n");
}


void make_dir(vector<string> argv) {

}