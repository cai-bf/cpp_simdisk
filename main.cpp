#include <iostream>
#include "src/header/system.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
using namespace std;

#define	MAXEPOLL	20
#define	MAXLINE		2048
#define PORT	    7112
#define	MAXBACK	    100


typedef void (* fun) (vector<string>);
const unordered_map<std::string, fun> cmd = {
        {"login", login},
        {"logout", logout},
        {"help", help},
        {"info",display_sys_info},
        {"cd", cd_dir},
        {"dir", display_dir},
        {"md", make_dir},
        {"rd", remove_dir},
        {"ls", ls},
        {"newfile", make_file},
        {"cat", cat},
        {"copy", copy},
        {"del", del_file}
};

// 设置非阻塞
int setnonblocking( int fd )
{
    if( fcntl( fd, F_SETFL, fcntl( fd, F_GETFD, 0 )|O_NONBLOCK ) == -1 )
    {
        printf("Set blocking error : %d\n", errno);
        return -1;
    }
    return 0;
}

struct Msg {
    dir currDir;
    user currUser;
};

static unordered_map<int, Msg> profile{};


int main() {
    initial();

    dir rootDir = currDir;
    // 任务一
//    while (true) {
//        string input;
//        getline(cin, input);
//        trim(input);
//        vector<string> argv = split(input, " ");
//
//        if (argv.empty()) {
//            printf("参数错误!\n");
//        }
//        // ----------任务一------
//        if (argv[0] == "exit")
//            break;
//        // -------------------
//        if (cmd.count(argv[0]) != 0) {
//
//            cmd.at(argv[0])(argv);
//
//        } else {
//            dprintf(output, "命令错误\n");
//        }
//    }

    // 任务二三
    int 		listen_fd;
    int 		conn_fd;
    int 		epoll_fd;
    int 		nread;
    int 		cur_fds; // 连接数
    int 		wait_fds; // 每次epoll返回的数量
    int		i;
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;
    struct 	epoll_event	ev;
    struct 	epoll_event	evs[MAXEPOLL];
    struct 	rlimit	rlt;
    char 	buf[MAXLINE];
    socklen_t len = sizeof( struct sockaddr_in );

    // 设置最大连接数
    rlt.rlim_max = rlt.rlim_cur = MAXEPOLL;
    if( setrlimit( RLIMIT_NOFILE, &rlt ) == -1 )
    {
        printf("Setrlimit Error : %d\n", errno);
        exit( EXIT_FAILURE );
    }

    // 初始化socket
    bzero( &servaddr, sizeof( servaddr ) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
    servaddr.sin_port = htons( PORT );

    // 建立socket
    if( ( listen_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
    {
        printf("Socket Error...\n" , errno );
        exit( EXIT_FAILURE );
    }

    // 设置非阻塞模式
    if( setnonblocking( listen_fd ) == -1 )
    {
        printf("Setnonblocking Error : %d\n", errno);
        exit( EXIT_FAILURE );
    }

    // 绑定
    if( bind( listen_fd, ( struct sockaddr *)&servaddr, sizeof( struct sockaddr ) ) == -1 )
    {
        printf("Bind Error : %d\n", errno);
        exit( EXIT_FAILURE );
    }

    // 监听
    if( listen( listen_fd, MAXBACK ) == -1 )
    {
        printf("Listen Error : %d\n", errno);
        exit( EXIT_FAILURE );
    }

    // 创建epoll
    epoll_fd = epoll_create( MAXEPOLL );
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_fd;
    if( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev ) < 0 )
    {
        printf("Epoll Error : %d\n", errno);
        exit( EXIT_FAILURE );
    }
    cur_fds = 1;

    while( true ) {

        if( ( wait_fds = epoll_wait( epoll_fd, evs, cur_fds, -1 ) ) == -1 ) {
            printf( "Epoll Wait Error : %d\n", errno );
            exit( EXIT_FAILURE );
        }

        for( i = 0; i < wait_fds; i++ ) {
            // 有socket建立连接
            if( evs[i].data.fd == listen_fd && cur_fds < MAXEPOLL ) {
                if( ( conn_fd = accept( listen_fd, (struct sockaddr *)&cliaddr, &len ) ) == -1 )
                {
                    printf("Accept Error : %d\n", errno);
                    exit( EXIT_FAILURE );
                }
                cout << "建立连接" << endl;
                // 将新建立连接的客户端加入epoll
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_fd;
                if( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev ) < 0 )
                {
                    printf("Epoll Error : %d\n", errno);
                    exit( EXIT_FAILURE );
                }
                ++cur_fds;

                // 加入profile, 此时的信息时空白的, 没有关系, 也因为登录后会更新,shell端会强制先登录
                Msg msg{};
                msg.currDir = rootDir;
                profile.insert({ conn_fd, msg });

                continue;
            }
            // 清空buf
            memset(buf, 0, MAXLINE);

            // 获取数据, 出错或主动退出则关闭并且删除用户信息
            nread = read( evs[i].data.fd, buf, sizeof( buf ) );
            if( nread <= 0 || string(buf) == "exit" )
            {
                // 关闭socket
                close( evs[i].data.fd );
                epoll_ctl( epoll_fd, EPOLL_CTL_DEL, evs[i].data.fd, &ev );
                --cur_fds;

                // 删除用户信息
                profile.erase(evs[i].data.fd);

                continue;
            }
            cout << "回复状态" << endl;
            // 恢复状态
            Msg msg = profile.at(evs[i].data.fd);
            currDir = msg.currDir;
            currUser = msg.currUser;
            output = evs[i].data.fd;

            // 处理请求命令
            cout << "处理请求" << endl;
            string arg = string(buf);
            auto argv = split(arg, " ");
            if (argv.empty()) {
                dprintf(output, "参数错误");
            }
            // 处理创建文件时内容处理
            if (argv[0] == "newfile") {
                int pos = arg.find_first_of(' ', 8); // 跳过第一个空格
                if (pos != -1) {
                    argv = { argv[0], argv[1], arg.substr(pos + 1) };
                }
            }
            cout << currUser.name << endl;
            cout << currDir.name << endl;

            // 调用函数
            if (cmd.count(argv[0]) != 0) {
                cmd.at(argv[0])(argv);
            } else {
                dprintf(output, "命令错误\n");
            }

            // 保存状态
            msg.currUser = currUser;
            msg.currDir = currDir;
            profile[evs[i].data.fd] = msg;
        }
    }
}
