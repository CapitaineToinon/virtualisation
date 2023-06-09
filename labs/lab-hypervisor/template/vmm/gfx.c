/// @file gfx.c
/// @author Florent Gluck
/// @date 2016-2020
/// Helper routines to render pixels in fullscreen graphic mode.
/// Requires the SDL2 library.

#include "gfx.h"
#include "font.h"

uint32_t gfx_colors[GFX_COLORS_COUNT] = {
    GFX_COLOR(0x00, 0x00, 0x00),
    GFX_COLOR(0x00, 0x00, 0xAA),
    GFX_COLOR(0x00, 0xAA, 0x00),
    GFX_COLOR(0x00, 0xAA, 0xAA),
    GFX_COLOR(0xAA, 0x00, 0x00),
    GFX_COLOR(0xAA, 0x00, 0xAA),
    GFX_COLOR(0xAA, 0x55, 0x00),
    GFX_COLOR(0xAA, 0xAA, 0xAA),
    GFX_COLOR(0x55, 0x55, 0x55),
    GFX_COLOR(0x55, 0x55, 0xFF),
    GFX_COLOR(0x55, 0xFF, 0x55),
    GFX_COLOR(0x55, 0xFF, 0xFF),
    GFX_COLOR(0xFF, 0x55, 0x55),
    GFX_COLOR(0xFF, 0x55, 0xFF),
    GFX_COLOR(0xFF, 0xFF, 0x55),
    GFX_COLOR(0xFF, 0xFF, 0xFF),
};

/// Create a fullscreen graphic window.
/// @param title Title of the window.
/// @param width Width of the window in pixels.
/// @param height Height of the window in pixels.
/// @return a pointer to the graphic context or NULL if it failed.
gfx_context_t *gfx_create(char *title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "%s", SDL_GetError());
        goto error;
    }
    SDL_Window *window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    uint32_t *pixels = malloc(width * height * sizeof(uint32_t));
    gfx_context_t *ctxt = malloc(sizeof(gfx_context_t));

    if (!window || !renderer || !texture || !pixels || !ctxt)
        goto error;

    ctxt->renderer = renderer;
    ctxt->texture = texture;
    ctxt->window = window;
    ctxt->width = width;
    ctxt->height = height;
    ctxt->pixels = pixels;

    SDL_ShowCursor(SDL_DISABLE);
    gfx_clear(ctxt, GFX_BLACK);
    return ctxt;

error:
    return NULL;
}

/// Draw a pixel in the specified graphic context.
/// @param ctxt Graphic context where the pixel is to be drawn.
/// @param x X coordinate of the pixel.
/// @param y Y coordinate of the pixel.
/// @param color Color of the pixel.
void gfx_putpixel(gfx_context_t *ctxt, int x, int y, uint32_t color)
{
    if (x < ctxt->width && y < ctxt->height)
        ctxt->pixels[ctxt->width * y + x] = color;
}

/// Draw a character in the specified graphic context.
void gfx_putchar(gfx_context_t *ctxt, int x, int y, uint8_t character, uint32_t fg_color, uint32_t bg_color)
{
    for (int h = 0; h < FONT_HEIGHT; h++)
    {
        uint8_t b = font_8x16[character * FONT_HEIGHT + h];

        for (int w = 0; w < FONT_WIDTH; w++)
        {
            int actual_x = (x * FONT_WIDTH) + FONT_WIDTH - w;
            int actual_y = (y * FONT_HEIGHT) + h;

            if ((b >> w) & 0b1)
            {
                gfx_putpixel(ctxt, actual_x, actual_y, fg_color);
            }
            else
            {
                gfx_putpixel(ctxt, actual_x, actual_y, bg_color);
            }
        }
    }
}

/// Clear the specified graphic context.
/// @param ctxt Graphic context to clear.
/// @param color Color to use.
void gfx_clear(gfx_context_t *ctxt, uint32_t color)
{
    int n = ctxt->width * ctxt->height;
    while (n)
        ctxt->pixels[--n] = color;
}

/// Display the graphic context.
/// @param ctxt Graphic context to clear.
void gfx_present(gfx_context_t *ctxt)
{
    SDL_UpdateTexture(ctxt->texture, NULL, ctxt->pixels, ctxt->width * sizeof(uint32_t));
    SDL_RenderCopy(ctxt->renderer, ctxt->texture, NULL, NULL);
    SDL_RenderPresent(ctxt->renderer);
}

/// Destroy a graphic window.
/// @param ctxt Graphic context of the window to close.
void gfx_destroy(gfx_context_t *ctxt)
{
    SDL_ShowCursor(SDL_ENABLE);
    SDL_DestroyTexture(ctxt->texture);
    SDL_DestroyRenderer(ctxt->renderer);
    SDL_DestroyWindow(ctxt->window);
    free(ctxt->pixels);
    ctxt->texture = NULL;
    ctxt->renderer = NULL;
    ctxt->window = NULL;
    ctxt->pixels = NULL;
    SDL_Quit();
    free(ctxt);
}

/// If a key was pressed, returns its key code (non blocking call).
/// List of key codes: https://wiki.libsdl.org/SDL_Keycode
/// @return the key that was pressed or 0 if none was pressed.
SDL_Keycode gfx_keypressed()
{
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
            return event.key.keysym.sym;
    }
    return 0;
}
