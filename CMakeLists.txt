cmake_minimum_required(VERSION 3.14)
project(simdisk)

set(CMAKE_CXX_STANDARD 11)

add_executable(simdisk main.cpp header/const.h header/super_block.h header/inode.h header/file.h header/dir.h header/block_group.h header/user.h header/system.h source/system.cpp header/unity.h source/unity.cpp header/helpers.h source/helpers.cpp source/dir.cpp header/global.h)