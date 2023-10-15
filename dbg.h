//
// Created by chen chen on 2023/10/15.
//

#ifndef CLOX_DBG_H
#define CLOX_DBG_H

#include <stdio.h>

#define debug
#ifdef debug
#define dbg(format, ...)                                                                    \
            printf("%s %s FILE [%s] LINE [%d] : ", __DATE__, __TIME__, __FILE__, __LINE__); \
            printf(format, ##__VA_ARGS__);                                                  \
            printf("\r\n");
#else
#define dbg(format, ...)
#endif

#endif //CLOX_DBG_H