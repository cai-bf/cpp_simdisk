//
// Created by cbf on 2020/5/9.
//

#include "../header/dir.h"
#include "../header/const.h"
#include <sys/stat.h>
using namespace std;

// 内部创建文件函数
void make_file_inner(vector<string>& argv, string& msg) {
    if (argv.size() < 3) {
        msg = "newfile: 参数错误";
        return;
    }
    // 找出最后一个目录
    struct dir curr = currDir;
    bool same_dir = false;
    size_t pos = argv[1].find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            msg = "newfile: 没有权限";
            return;
        }
        bool succ = getDir(argv[1].substr(0, pos), curr);
        if (!succ) {
            msg = "newfile: 目录不存在, 不支持嵌套创建";
            return;
        }
    } else {
        same_dir = true;
    }

    // 判断权限
    if (inodeTable[curr.inode_idx].uid != currUser.uid) {
        msg = "newfile: 没有权限";
        return;
    }

    // 判断重名
    for (int m = 2; m < curr.files_num; ++m) {
        if (string(curr.files[m].name) == argv[1].substr(pos + 1) &&
            inodeTable[curr.files[m].inode_idx].mode == FILE__) {
            msg = "newfile: 已存在该文件";
            return;
        }
    }

    // 获取空闲inode
    if (superBlock.free_inodes_num < 1) {
        msg = "newfile: inode不足";
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
        msg = "newfile: 内存不足";
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
        msg = "newfile: 内存不足";
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
    fwrite(content, size, 1, fd);
    fclose(fd);

    msg = "newfile: 新建文件成功";

    // 如果同目录, 需要更新currDir, 不然之后的操作会出错
    if (same_dir)
        currDir = curr;
}


// 创建文件
void make_file(vector<string> argv) {
    string msg;
    make_file_inner(argv, msg);
    dprintf(output, msg.c_str());
}


void cat(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "cat: 参数错误");
        return;
    }
    // 获取当前目录及文件名
    struct dir curr = currDir;
    string file_name;
    size_t pos = argv[1].find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            getDir("/", curr);
            file_name = argv[1].substr(1);
        } else {
            bool succ = getDir(argv[1].substr(0, pos), curr);
            if (!succ) {
                dprintf(output, "cat: 路径错误");
                return;
            }
            file_name = argv[1].substr(pos + 1);
        }

    } else
        file_name = argv[1];
    // 获取file
    file f{};
    f.inode_idx = -1;
    for (int i = 2; i < curr.files_num; ++i) {
        if (string(curr.files[i].name) == file_name && inodeTable[curr.files[i].inode_idx].mode == FILE__) {
            f = curr.files[i];
            break;
        }
    }
    if (f.inode_idx == -1) {
        dprintf(output, "cat: 文件不存在");
        return;
    }
    // 获取内容
    auto node = inodeTable[f.inode_idx];
    char *content = new char[node.size + 2];
    fd = fopen(FILE_NAME, "r");
    fseek(fd, node.block_addr, SEEK_SET);
    fread(content, node.size, 1, fd);
    fclose(fd);
    strcat(content, "");

    dprintf(output, content);

    // delete
    delete[] content;
}


// del内部函数
void del_file(string& path, string& msg) {
    struct dir curr = currDir;
    string file_name;
    size_t pos = path.find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            getDir("/", curr);
            file_name = path.substr(1);
        } else {
            bool succ = getDir(path.substr(0, pos), curr);
            if (!succ) {
                dprintf(output, "cat: 路径错误");
                return;
            }
            file_name = path.substr(pos + 1);
        }

    } else
        file_name = path;

    bool same_dir = curr.inode_idx == currDir.inode_idx;

    //获取文件
    struct file f{};
    f.inode_idx = -1;
    for (int i = 2; i < curr.files_num; ++i) {
        if (inodeTable[curr.files[i].inode_idx].mode == FILE__ && string(curr.files[i].name) == file_name) {
            f = curr.files[i];
            break;
        }
    }
    if (f.inode_idx == -1) {
        dprintf(output, "del: 文件不存在");
        return;
    }
    // 更新super block / bitmap
    struct inode node = inodeTable[f.inode_idx];
    superBlock.free_inodes_num++; // 归还inode
    superBlock.free_blocks_num += node.blocks; // 归还block

    inodeBitmap[f.inode_idx] = NUSED;

    int blockIndex = (node.block_addr - FIRST_DATA_ADDR) / BLOCK_SIZE;
    for (int j = 0; j < node.blocks; ++j) {
        blockBitmap[blockIndex + j] = NUSED;
    }

    // 更新父目录
    for (int k = 2; k < curr.files_num; ++k) {
        if (curr.files[k].inode_idx == f.inode_idx) {
            for (int i = k; i < curr.files_num - 1; ++i) {
                curr.files[i].inode_idx = curr.files[i + 1].inode_idx;
                strcpy(curr.files[i].name, curr.files[i + 1].name);
            }
            curr.files_num--;
            break;
        }
    }

    // 写入文件
    fd = fopen(FILE_NAME, "rb+");
    fseek(fd, 0, SEEK_SET);
    fwrite(&superBlock, sizeof(super_block), 1, fd);
    fwrite(blockBitmap, sizeof(bool), TOTAL_BLOCK_NUM, fd);
    fwrite(inodeBitmap, sizeof(bool), TOTAL_INODE_NUM, fd);
    fseek(fd, inodeTable[curr.inode_idx].block_addr, SEEK_SET);
    fwrite(&curr, sizeof(dir), 1, fd);
    fclose(fd);

    // 判断目录是否一致, 若一致更新当前目录
    if (same_dir)
        currDir = curr;

    msg = "del: 删除文件成功";
}


// 复制系统文件
void copy_outer(string src, string dest, string& msg) {
    // 打开外部文件
    FILE *outer_fd = fopen(src.c_str(),"r");
    if (outer_fd == NULL) {
        msg = "copy: 不存在该文件";
        return;
    }
    // 获取外部文件大小
    struct stat status;
    stat(src.c_str(), &status);
    size_t size = status.st_size;
    // 读取文件内容
    char *content = new char[size + 1];
    fseek(outer_fd, 0, SEEK_SET);
    fread(content, size, 1, outer_fd);
    fclose(outer_fd);

    // 判断目的目录是否已存在
    struct dir dest_dir = currDir;
    string file_name;
    int pos = dest.find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            getDir("/", dest_dir);
            file_name = dest.substr(1);
        } else {
            bool succ = getDir(dest.substr(0, pos), dest_dir);
            if (!succ) {
                msg = "copy: 目的文件路径错误";
                return;
            }
            file_name = dest.substr(pos + 1);
        }

    } else
        file_name = dest;
    // 判断目的文件是否已存在, 存在的话删除, 再进行复制
    if (inodeTable[dest_dir.inode_idx].uid != currUser.uid) {
        msg = "copy: 没有权限";
        return;
    }
    for (int j = 2; j < dest_dir.files_num; ++j) {
        if (inodeTable[dest_dir.files[j].inode_idx].mode == FILE__ &&
            string(dest_dir.files[j].name) == file_name) {
            del_file(dest, msg);
            break;
        }
    }

    // 创建新文件
    vector<string> vec{"newfile", dest, string(content)};
    make_file_inner(vec, msg);
    // 判断是否成功
    if (msg.find_first_of("成功") != -1) {
        msg = "copy: 复制成功";
        return;
    }
    msg = "copy" + msg.substr(7);

    delete[] content;

}

// 复制模拟磁盘内部文件
void copy_inner(string src, string dest, string& msg) {
    struct dir curr = currDir;
    // 找出对应目录以及文件名
    string file_name;
    size_t pos = src.find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            getDir("/", curr);
            file_name = src.substr(1);
        } else {
            bool succ = getDir(src.substr(0, pos), curr);
            if (!succ) {
                msg = "copy: 源文件路径错误";
                return;
            }
            file_name = src.substr(pos + 1);
        }

    } else
        file_name = src;
    // 判断文件是否存在
    struct file src_file{};
    src_file.inode_idx = -1;
    for (int i = 2; i < curr.files_num; ++i) {
        if (inodeTable[curr.files[i].inode_idx].mode == FILE__ &&
                string(curr.files[i].name) == file_name) {
            src_file = curr.files[i];
            break;
        }
    }
    if (src_file.inode_idx == -1) {
        msg = "copy: 源文件不存在";
        return;
    }

    // 判断目的目录是否已存在
    struct dir dest_dir = currDir;
    pos = dest.find_last_of('/');
    if (pos != -1) { // 找到 / ，表示有多级目录
        if (pos == 0) { // 位于根目录
            getDir("/", dest_dir);
            file_name = dest.substr(1);
        } else {
            bool succ = getDir(dest.substr(0, pos), dest_dir);
            if (!succ) {
                msg = "copy: 目的文件路径错误";
                return;
            }
            file_name = dest.substr(pos + 1);
        }

    } else
        file_name = dest;
    // 判断目的文件是否已存在, 存在的话删除, 再进行复制
    if (inodeTable[dest_dir.inode_idx].uid != currUser.uid) {
        msg = "copy: 没有权限";
        return;
    }
    for (int j = 2; j < dest_dir.files_num; ++j) {
        if (inodeTable[dest_dir.files[j].inode_idx].mode == FILE__ &&
                string(dest_dir.files[j].name) == file_name) {
            del_file(dest, msg);
            break;
        }
    }

    // 获取源文件内容
    char *content = new char[inodeTable[src_file.inode_idx].size + 1];
    fd = fopen(FILE_NAME, "r");
    fseek(fd, inodeTable[src_file.inode_idx].block_addr,SEEK_SET);
    fread(content, inodeTable[src_file.inode_idx].size, 1, fd);
    fclose(fd);

    // 创建新文件
    vector<string> vec{"newfile", dest, string(content)};
    make_file_inner(vec, msg);
    // 判断是否成功
    if (msg.find_first_of("成功") != -1) {
        msg = "copy: 复制成功";
        return;
    }
    msg = "copy" + msg.substr(7);

    delete[] content;
}


void copy(vector<string> argv) {
    if (argv.size() != 3) {
        dprintf(output, "copy: 参数错误");
    }

    // 消息
    string msg;

    // 判断文件所处位置
    if (argv[1].length() > 6 && argv[1].find_first_of("<host>") == 0) {
        copy_outer(argv[1].substr(6), argv[2], msg);
    } else {
        copy_inner(argv[1], argv[2], msg);
    }

    dprintf(output, msg.c_str());
}


// 删除文件
void del_file(vector<string> argv) {
    if (argv.size() != 2) {
        dprintf(output, "del: 参数错误");
        return;
    }

    string msg;
    del_file(argv[1], msg);
    dprintf(output, msg.c_str());
}