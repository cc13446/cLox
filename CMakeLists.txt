cmake_minimum_required(VERSION 3.26)
project(cLox C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(cLox
        chunk.h
        chunk.c
        common.h
        compiler.h
        compiler.c
        debug.h
        debug.c
        main.c
        memory.h
        memory.c
        scanner.h
        scanner.c
        trie.h
        trie.c
        value.h
        value.c
        vm.h
        vm.c
)
