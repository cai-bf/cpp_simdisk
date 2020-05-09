//
// Created by cbf on 2020/5/9.
//

#include "../header/dir.h"
#include "../header/const.h"
using namespace std;

// 创建文件
void make_file(vector<string> argv) {
    if (argv.size() < 3) {
        dprintf(output, "newfile: 参数错误\n");
        return;
    }
    // 找出最后一个目录
    dir curr = currDir;
    size_t pos = argv[1].find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            dprintf(output, "newfile: 没有权限\n");
            return;
        }
        bool succ = getDir(argv[1].substr(0, pos), curr);
        if (!succ) {
            dprintf(output, "newfile: 目录不存在, 不支持嵌套创建\n");
            return;
        }
    }

    // 判断权限
    if (inodeTable[curr.inode_idx].uid != currUser.uid) {
        dprintf(output, "newfile: 没有权限\n");
        return;
    }

    // 获取空闲inode
    if (superBlock.free_inodes_num < 1) {
        dprintf(output, "newfile: inode不足\n");
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
    inodeTable[i].mode = FILE__;
    inodeTable[i].row = RW;
    // 获取内容，由于前面split，这里要补回join
    string  cont = join(argv, 2, " ");
    const char *content = cont.c_str();
    int size = strlen(content);
    inodeTable[i].size = size;
    inodeTable[i].blocks = size % BLOCK_SIZE == 0 ? size / BLOCK_SIZE : size / BLOCK_SIZE + 1;

    // 找出连续的空闲块
    if (superBlock.free_blocks_num < inodeTable[i].blocks) {
        dprintf(output, "newfile: 内存不足\n");
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
                    if (num == inodeTable[i].blocks)
                        break;
                } else
                    break;
            }
            if (num == inodeTable[i].blocks) {
                succ = true;
                break;
            }
        }
    }
    if (!succ) {
        dprintf(output, "newfile: 内存不足\n");
        return;
    }
    // 更新bitmap
    for (int l = 0; l < inodeTable[i].blocks; ++l) {
        blockBitmap[j + l] = USED;
    }
    // 更新inode指向偏移量
    inodeTable[i].block_addr = FIRST_DATA_ADDR + j * BLOCK_SIZE;

    // 创建文件
    file new_file{};
    new_file.inode_idx = i;
    strcpy(new_file.name, argv[1].substr(pos + 1).c_str());

    // 更新父目录
    curr.files[curr.files_num].inode_idx = new_file.inode_idx;
    strcpy(curr.files[curr.files_num].name, new_file.name);
    curr.files_num++;
    // 更新super block
    superBlock.free_inodes_num--;
    superBlock.free_blocks_num -= inodeTable[i].blocks;

    // 写入文件
    fd = fopen(FILE_NAME, "rb+");
    fseek(fd, 0, SEEK_SET);
    fwrite(&superBlock, sizeof(super_block), 1, fd);
    fwrite(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fwrite(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fwrite(inodeTable, sizeof(inode), TOTAL_INODE_NUM, fd);
    fseek(fd, inodeTable[curr.inode_idx].block_addr, SEEK_SET);
    fwrite(&curr, sizeof(dir), 1, fd);
    fseek(fd, inodeTable[new_file.inode_idx].block_addr, SEEK_SET);
    fwrite(&new_file, sizeof(file), 1, fd);
    fclose(fd);

    dprintf(output, "newfile: 新建文件成功\n");
}


void cat(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "cat: 参数错误\n");
        return;
    }


}


void copy(vector<string> argv) {

}


void del_file(vector<string> argv) {

}