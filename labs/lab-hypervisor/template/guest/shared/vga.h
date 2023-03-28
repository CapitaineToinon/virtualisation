#ifndef _VGA_H_
#define _VGA_H_

#include "shared/vga.h"

// Display a character.
// Real hardware emulated version.
extern void putchar(int x, int y, char ch, color_t fg, color_t bg);

#endif
