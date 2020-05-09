//
// Created by cbf on 20-2-21.
//

#ifndef SIMDISK_HELPERS_H
#define SIMDISK_HELPERS_H

#include <vector>
#include <string>
#include <string.h>


std::string& trim(std::string &);

std::vector<std::string> split(const std::string, const std::string);

std::string join(const std::vector<std::string> &, int, std::string);

#endif //SIMDISK_HELPERS_H
