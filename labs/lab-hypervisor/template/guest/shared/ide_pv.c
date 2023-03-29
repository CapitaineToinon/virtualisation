#include <stdint.h>
#include <string.h>

#include "ide.h"
#include "pmio.h"
#include "../../shared/ide_pv.h"

/**
 * Write a sector using paravirtualization.
 * @param sector_idx sector to write (0-indexed).
 * @param src address of the data to be written.
 */
void ide_write_sector_pv(int sector_idx, void *src)
{
    hypercall_t *addr = (hypercall_t *)HYPERCALL_ADDR;
    addr->sector_idx = sector_idx;

    char *data = (char *)src;
    for (int i = 0; i < SECTOR_SIZE; i++)
    {
        addr->data[i] = (char)data[i];
    }

    outb(HYPERCALL_PORT, HYPERCALL_MAGIC);
}
