#ifndef _VGA_SHARED_H_
#define _VGA_SHARED_H_

#define VGA_FB_ADDR 0xB8000
#define VGA_FB_PITCH 160
#define VGA_XRES 80
#define VGA_YRES 25

typedef enum
{
    COL_BLACK = 0,
    COL_BLUE = 1,
    COL_GREEN = 2,
    COL_CYAN = 3,
    COL_RED = 4,
    COL_MAGENTA = 5,
    COL_BROWN = 6,
    COL_LIGHT_GREY = 7,
    COL_DARK_GREY = 8,
    COL_LIGHT_BLUE = 9,
    COL_LIGHT_GREEN = 10,
    COL_LIGHT_CYAN = 11,
    COL_LIGHT_RED = 12,
    COL_LIGHT_MAGENTA = 13,
    COL_YELLOW = 14,
    COL_WHITE = 15,
    _COL_END_
} color_t;

#endif