#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <unordered_map>
#include "helper.hpp"

using namespace std;

#define PORT 7112
#define BUFLEN 2048

// 命令及其对应的参数数目(包含自身)
const unordered_map<string, unsigned short> cmd = {
        {"login", 3},
        {"logout", 1},
        {"help", 1},
        {"info",1},
        {"cd", 2},
        {"dir", 2},
        {"md", 2},
        {"rd", 2},
        {"ls", 1},
        {"newfile", 3},
        {"cat", 2},
        {"copy", 3},
        {"del", 2}
};


bool checkCmd(string &str, string &msg) {
    if (str.empty() || trim(str).empty()) {
        msg = "";
        return false;
    }

    vector<string> argv = split(str, " ");

    if (cmd.count(argv[0]) == 0) {
        msg = "command " + argv[0] + " not found";
        return false;
    }

    if (argv.size() < cmd.at(argv[0])) {
        msg = "command " + argv[0] + " need " + to_string(cmd.at(argv[0]) - 1) + " parameters";
        return false;
    }

    return true;
}


int main(){
    // 建立socket连接
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    int succ = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (succ == -1) {
        printf("连接文件系统失败\n");
        exit(1);
    }

    char buf[BUFLEN];
    int count = 0;

    // 登录
    while (true) {
        // 清空buf
        memset(buf, 0, BUFLEN);
        string user, pswd;
        cout << "请输入用户名: " << endl;
        getline(cin, user);
        cout << "请输入密码: " << endl;
        getline(cin, pswd);
        string c = "login " + user + " " + pswd;
        dprintf(fd, "%s", c.c_str());
        string res;
        count = read(fd, buf, BUFLEN);
        if (count < 0) {
            cout << "连接失败, 自动退出" << endl;
            exit(1);
        }
        res = string(buf);

        if (res.length() < 2 || res != "登陆成功") {
            cout << "用户或密码错误, 请重试!" << endl;
            continue;
        }

        cout << res << endl;

        break;
    }

    // 命令处理
    string command;
    string res;
    while (true) {
        // 清空buf
        memset(buf, 0, BUFLEN);
        cout << "$ ";
        cout.flush();
        getline(cin, command);

        // 判断是否退出
        if (command.length() >= 4 && command == "exit") {
            close(fd);
            return 0;
        }
        // 判断命令是否正确
        bool ok = checkCmd(command, res);
        if (!ok) {
            cout << res << endl;
            continue;
        }

        // 发送数据
        dprintf(fd, "%s", command.c_str());

        // 阻塞接收数据
        res.clear();
        count = read(fd, buf, BUFLEN);
//        while( () >= BUFLEN) {
//            res += string(buf);
//        }
        if (count < 0) {
            cout << "连接失败, 自动退出" << endl;
            exit(1);
        }
        res = string(buf);

        cout << res << endl;
    }
}