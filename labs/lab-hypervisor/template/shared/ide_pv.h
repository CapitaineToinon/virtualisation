#ifndef _IDEPV_SHARED_H_
#define _IDEPV_SHARED_H_

#include "ide.h"

#define HYPERCALL_ADDR 0xFA000
#define HYPERCALL_PORT 0xABBA
#define HYPERCALL_MAGIC 1

typedef struct hypercall
{
    int sector_idx;
    char data[SECTOR_SIZE];
} hypercall_t;

#endif