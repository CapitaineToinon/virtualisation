#include <stdint.h>
#include "ide.h"

#define STATUS_PORT 0x1F7
#define DATA_PORT   0x1F0

/**
 * Write a sector using paravirtualization.
 * @param sector_idx sector to write (0-indexed).
 * @param src address of the data to be written.
 */
void ide_write_sector_pv(int sector_idx, void *src) {
    // ...
}
