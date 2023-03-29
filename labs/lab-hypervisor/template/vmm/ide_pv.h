#ifndef _IDEPV_H_
#define _IDEPV_H_

#include "shared/ide_pv.h"

struct hypercall_host
{
    char *disk_path;
    void (*next)(struct hypercall_host *hypercall_host, hypercall_t *hypercall, struct kvm_run *run);
};

typedef struct hypercall_host hypercall_host_t;

hypercall_host_t *create_hypercall_host(char *disk_path);
void destroy_hypercall_host(hypercall_host_t *hypercall_host);

#endif
