#include <string>
#include <vector>


std::string& trim(std::string &s) {
    if (s.empty()) {
        return s;
    }
    s.erase(0,s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
    return s;
}

std::vector<std::string> split(const std::string str, const std::string sep){
    char* cstr = const_cast<char*>(str.c_str());
    char* current;
    std::vector<std::string> arr;
    current=strtok(cstr, sep.c_str());
    while(current != nullptr){
        arr.emplace_back(current);
        current=strtok(nullptr, sep.c_str());
    }
    return arr;
}