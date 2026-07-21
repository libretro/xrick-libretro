#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H 1

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "SDL.h"

#include <boolean.h>

#define WINDOW_WIDTH  320
#define WINDOW_HEIGHT 240

#ifdef FRONTEND_SUPPORTS_RGB565
extern uint16_t Retro_Screen[WINDOW_WIDTH*WINDOW_HEIGHT];
#define PIXEL_BYTES 1
#define PIXEL_TYPE uint16_t
#else
extern uint32_t Retro_Screen[WINDOW_WIDTH*WINDOW_HEIGHT];
#define PIXEL_BYTES 2
#define PIXEL_TYPE uint32_t 
#endif 

extern char Key_Sate[512];
extern char Key_Sate2[512];

#include "SDL_video.h"

#define LOGI printf

/* r: 5 bits, g: 6 bits, b: 5 bits */
#define RGB565(r, g, b)  (((r) << 11) | ((g) << 5) | (b))

#endif
