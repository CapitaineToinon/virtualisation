#ifndef _IDE_H_
#define _IDE_H_

#include "shared/ide.h"
#include <linux/kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct ide
{
    void (*next)(struct ide *ide, struct kvm_run *run);
    char *disk_path;
    int sector_idx;
    int data_count;
    void *data;
};

typedef struct ide ide_t;

ide_t *create_ide_state_machine(char *disk_path);
void destroy_ide_state_machine(ide_t *ide);
void write_data_to_sector(char *disk_path, int sector_idx, void *data);

#endif
