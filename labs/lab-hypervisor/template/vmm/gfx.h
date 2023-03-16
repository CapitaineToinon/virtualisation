#ifndef _GFX_H_
#define _GFX_H_

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define GFX_COLOR(r,g,b) ((uint32_t)b|((uint32_t)g<<8)|((uint32_t)r<<16))

#define GFX_GET_B(color) (color & 0xff)
#define GFX_GET_G(color) ((color >> 8) & 0xff)
#define GFX_GET_R(color) ((color >> 16) & 0xff)

#define GFX_BLACK  0x00000000
#define GFX_RED    0x00FF0000
#define GFX_GREEN  0x0000FF00
#define GFX_BLUE   0x000000FF
#define GFX_WHITE  0x00FFFFFF
#define GFX_YELLOW 0x00FFFF00

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t *pixels;
    int width;
    int height;
} gfx_context_t;

extern void gfx_putpixel(gfx_context_t *ctxt, int x, int y, uint32_t color);
extern void gfx_clear(gfx_context_t *ctxt, uint32_t color);
extern gfx_context_t* gfx_create(char *text, int width, int height);
extern void gfx_destroy(gfx_context_t *ctxt);
extern void gfx_present(gfx_context_t *ctxt);
extern SDL_Keycode gfx_keypressed();

#endif
