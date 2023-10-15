cmake_minimum_required(VERSION 3.26)
project(cLox C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(cLox
        common.h
        main.c
        chunk.h
        chunk.c
        memory.h
        memory.c
        debug.h
        debug.c
)
