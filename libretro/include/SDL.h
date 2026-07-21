
#ifndef HATARI_SDL_H
#define HATARI_SDL_H

//RETRO HACK
//#warning This is just an SDL wrapper for the retro core.

extern int Reset_Cold(void);
extern int Reset_Warm(void);

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <time.h>

#include "SDL_types.h"
#include "SDL_keyboard.h"
#include "SDL_video.h"

#include <SDL_endian.h>
#include <SDL_types.h>

#define SDL_SRCCOLORKEY 0x1
/* r: 5 bits, g: 6 bits, b: 5 bits */
#define RGB565(r, g, b)  (((r) << 11) | ((g) << 5) | (b))
extern int Retro_SetPalette(SDL_Surface * surface, int flags, const SDL_Color * colors,int firstcolor, int ncolors);
extern unsigned int Retro_MapRGB(SDL_PixelFormat *a, int r, int g, int b);
extern long GetTicks(void);
extern SDL_Surface *Retro_CreateRGBSurface( int w,int h, int d, int rm,int rg,int rb,int ra);
extern SDL_Surface *Retro_SetVideoMode(int w,int h,int b);
extern void Retro_FreeSurface(SDL_Surface *surf );
extern void Retro_BlitSurface(SDL_Surface *ss,SDL_Rect* sr,SDL_Surface *ds,SDL_Rect* dr);
extern void Retro_Fillrect(SDL_Surface * surf,SDL_Rect *rect,unsigned int col);
extern void Retro_GetRGB(int coul,SDL_PixelFormat *format, int *r,int *g,int *b);
extern int Retro_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key);
extern int Retro_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);

#define SDL_MapRGB(a, r, g, b) Retro_MapRGB(a, r, g, b)

typedef struct SDL_Event{
   Uint8 type;
} SDL_Event;

#define SDL_GRAB_OFF 0
#define SDL_GRAB_ON 1
#define SDL_HWSURFACE 0
#define SDL_FULLSCREEN 1
#define SDL_SWSURFACE 2
#define SDL_HWPALETTE 4
#define SDL_DISABLE 0
#define SDL_INIT_JOYSTICK 0
#define SDL_PHYSPAL 0
#define SDL_DOUBLEBUF 0
#define SDL_RESIZABLE 0

#define SDL_SetVideoMode(w, h, b, f) Retro_SetVideoMode((w),(h),(b))
#define SDL_FreeSurface(a) Retro_FreeSurface((a))
#define SDL_CreateRGBSurface(a,w,h,d,rm,gm,bm,am) Retro_CreateRGBSurface((w),(h),(d),(rm),(gm),(bm),(am))
#define SDL_BlitSurface(ss,sr,ds,dr) Retro_BlitSurface((ss),(sr),(ds),(dr))
#define SDL_SetPalette(s, f,c,d,n) Retro_SetPalette((s),(f),(c),(d),(n))

#if defined(__PS3__) 
#include <unistd.h> //stat() is defined here
#ifdef __PSL1GHT__
#include <sys/systime.h>
#else
#include <sys/sys_time.h>
#include <sys/timer.h>
#endif
#endif


#endif
