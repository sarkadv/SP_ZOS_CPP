cmake_minimum_required(VERSION 3.26)
project(SP_ZOS_CPP)

set(CMAKE_CXX_STANDARD 17)

add_executable(SP_ZOS_CPP vfs.cpp
        vfs.h
        parser.cpp
        parser.h
        constants.h
        commands.h
        commands.cpp
        bitmap.cpp
        bitmap.h
        superblock.cpp
        superblock.h
        data_block.cpp
        data_block.h
        inode.cpp
        inode.h
        directory.cpp
        directory.h
        file_handler.cpp
        file_handler.h
)
