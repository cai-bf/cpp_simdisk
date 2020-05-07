#include <iostream>
#include "header/system.h"
using namespace std;


typedef void (* fun) (vector<string>);
const unordered_map<std::string, fun> cmd = {
        {"login", login},
        {"logout", logout},
        {"help", help},
        {"info",display_sys_info},
        {"cd", cd_dir},
        {"dir", display_dir},
        {"md", make_dir},
        {"rd", remove_dir}
};


int main() {
    initial();

    while (true) {
        string input;
        getline(cin, input);
        trim(input);
        vector<string> argv = split(input, " ");

        if (argv.empty()) {
            printf("参数错误!\n");
        }
        // ----------任务一------
        if (argv[0] == "exit")
            break;
        // -------------------
        if (cmd.count(argv[0]) != 0) {

            cmd.at(argv[0])(argv);

        } else {
            dprintf(output, "命令错误\n");
        }
    }

}
