#ifndef _IDE_H_
#define _IDE_H_

#include "../../shared/ide.h"

// Write a sector.
// Real hardware emulated version.
extern void ide_write_sector_emul(int sector_idx, void *src);

// Write a sector.
// Paravirtualized version.
extern void ide_write_sector_pv(int sector_idx, void *src);

#endif
