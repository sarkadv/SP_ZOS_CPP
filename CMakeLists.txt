cmake_minimum_required(VERSION 3.26)
project(SP_ZOS_CPP)

set(CMAKE_CXX_STANDARD 17)

add_executable(SP_ZOS_CPP fs.cpp
        structures.h
        parser.cpp
        parser.h
        constants.h
        commands.h
        commands.cpp)
