#include "ide.h"
#include "ide_pv.h"

void handle_hypercall(struct hypercall_host *host, hypercall_t *hypercall, struct kvm_run *run)
{
    if (run->io.direction == KVM_EXIT_IO_OUT && run->io.size == 1 && run->io.port == HYPERCALL_PORT)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = *addr;

        if (value == HYPERCALL_MAGIC)
        {
            printf("handle_hypercall\n");
            write_data_to_sector(host->disk_path, hypercall->sector_idx, hypercall->data);
        }
    }
}

hypercall_host_t *create_hypercall_host(char *disk_path)
{
    hypercall_host_t *hypercall_host = (hypercall_host_t *)malloc(sizeof(hypercall_host_t));
    hypercall_host->disk_path = disk_path;
    hypercall_host->next = &handle_hypercall;
    return hypercall_host;
}

void destroy_hypercall_host(hypercall_host_t *hypercall_host)
{
    free(hypercall_host);
}