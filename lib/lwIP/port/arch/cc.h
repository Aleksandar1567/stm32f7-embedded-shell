/* port/arch/cc.h — compiler abstraction for lwIP on ARM GCC */
#pragma once

#include <stdlib.h>
#include <stdio.h>

typedef int sys_prot_t;

#define LWIP_PROVIDE_ERRNO

#define LWIP_TIMEVAL_PRIVATE 0
#include <sys/time.h>

/* Packed struct support for GCC */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT  __attribute__((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#define LWIP_PLATFORM_ASSERT(x) \
    do { printf("lwIP assert: %s  @ %s:%d\n", x, __FILE__, __LINE__); } while(0)

#define LWIP_RAND() ((u32_t)rand())
