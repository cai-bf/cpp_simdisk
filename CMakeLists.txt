cmake_minimum_required(VERSION 3.14)
project(simdisk)

set(CMAKE_CXX_STANDARD 11)

add_executable(simdisk src/main.cpp src/header/const.h src/header/super_block.h src/header/inode.h src/header/file.h src/header/dir.h src/header/user.h src/header/system.h src/source/system.cpp src/header/unity.h src/source/unity.cpp src/header/helpers.h src/source/helpers.cpp src/source/dir.cpp src/header/global.h src/source/file.cpp)