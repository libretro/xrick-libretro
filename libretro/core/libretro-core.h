#ifndef LIBRETRO_CORE_H
#define LIBRETRO_CORE_H 1

#include <stdint.h>
#include <string.h>
#include <stdlib.h>



#include <boolean.h>

/*
 * Internal key codes.
 *
 * These are private to the core: the input mapper turns libretro joypad and
 * keyboard events into them, and retro_key_down/up turn them into
 * control_status bits. They used to be SDL keysyms, which meant dragging in
 * a 286 line SDL_keysym.h to name eight values.
 */
enum
{
   XRICK_KEY_UP = 1,
   XRICK_KEY_DOWN,
   XRICK_KEY_LEFT,
   XRICK_KEY_RIGHT,
   XRICK_KEY_SPACE,
   XRICK_KEY_ESCAPE,
   XRICK_KEY_PAUSE,   /* was SDLK_p */
   XRICK_KEY_END      /* was SDLK_e */
};

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



#define LOGI printf

/* r: 5 bits, g: 6 bits, b: 5 bits */
#define RGB565(r, g, b)  (((r) << 11) | ((g) << 5) | (b))

#endif
