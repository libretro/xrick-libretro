/*
 * xrick/src/sysvid.c
 *
 * Copyright (C) 1998-2002 BigOrno (bigorno@bigorno.net). All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#include <stdlib.h>
#include <string.h>

#include "libretro-core.h"

#include "state.h"

#include "system.h"
#include "game.h"
#include "img.h"
#include "debug.h"

#ifdef __MSVC__
#include <memory.h> /* memset */
#endif

U8 *sysvid_fb = NULL; /* frame buffer */

/* the palette, pre-packed into the frontend's pixel format */
static PIXEL_TYPE palette[256];

/* ...and the unpacked source, kept so a savestate can carry the palette
 * without baking in whichever pixel format this build happens to use */
static img_color_t palette_rgb[256];

/*
 * The 8bpp shadow of what is currently on screen.
 *
 * This is not the same thing as sysvid_fb: it only ever receives the parts of
 * sysvid_fb that a rectangle marked dirty, so anywhere the game draws without
 * declaring a rectangle the two disagree. The expansion has to be driven from
 * the shadow to keep the output identical to the old full frame blit.
 */
static U8 *shadow = NULL;

#include "img_icon.e"

/*
 * color tables
 */

#ifdef GFXPC
static U8 RED[] = { 0x00, 0x50, 0xf0, 0xf0, 0x00, 0x50, 0xf0, 0xf0 };
static U8 GREEN[] = { 0x00, 0xf8, 0x50, 0xf8, 0x00, 0xf8, 0x50, 0xf8 };
static U8 BLUE[] = { 0x00, 0x50, 0x50, 0x50, 0x00, 0xf8, 0xf8, 0xf8 };
#endif
#ifdef GFXST
static U8 RED[] = { 0x00, 0xd8, 0xb0, 0xf8,
                    0x20, 0x00, 0x00, 0x20,
                    0x48, 0x48, 0x90, 0xd8,
                    0x48, 0x68, 0x90, 0xb0,
                    /* cheat colors */
                    0x50, 0xe0, 0xc8, 0xf8,
                    0x68, 0x50, 0x50, 0x68,
                    0x80, 0x80, 0xb0, 0xe0,
                    0x80, 0x98, 0xb0, 0xc8
};
static U8 GREEN[] = { 0x00, 0x00, 0x6c, 0x90,
                      0x24, 0x48, 0x6c, 0x48,
                      0x6c, 0x24, 0x48, 0x6c,
                      0x48, 0x6c, 0x90, 0xb4,
		      /* cheat colors */
                      0x54, 0x54, 0x9c, 0xb4,
                      0x6c, 0x84, 0x9c, 0x84,
                      0x9c, 0x6c, 0x84, 0x9c,
                      0x84, 0x9c, 0xb4, 0xcc
};
static U8 BLUE[] = { 0x00, 0x00, 0x68, 0x68,
                     0x20, 0xb0, 0xd8, 0x00,
                     0x20, 0x00, 0x00, 0x00,
                     0x48, 0x68, 0x90, 0xb0,
		     /* cheat colors */
                     0x50, 0x50, 0x98, 0x98,
                     0x68, 0xc8, 0xe0, 0x50,
                     0x68, 0x50, 0x50, 0x50,
                     0x80, 0x98, 0xb0, 0xc8};
#endif

/*
 * Pack one palette entry into a frontend pixel.
 */
static PIXEL_TYPE sysvid_pack(U8 r, U8 g, U8 b)
{
#ifdef FRONTEND_SUPPORTS_RGB565
   return (PIXEL_TYPE)RGB565(r >> 3, g >> 2, b >> 3);
#else
   return ((PIXEL_TYPE)r << 16) | ((PIXEL_TYPE)g << 8) | (PIXEL_TYPE)b;
#endif
}

void sysvid_setPalette(img_color_t *pal, U16 n)
{
   U16 i, x, y;
   const U8 *p;
   PIXEL_TYPE *q;

   for (i = 0; i < n; i++)
   {
      palette_rgb[i] = pal[i];
      palette[i]     = sysvid_pack(pal[i].r, pal[i].g, pal[i].b);
   }

   /* The palette is baked into the pixels as they are expanded, so changing
    * it invalidates every pixel already in the frontend buffer, including
    * ones no rectangle will mark dirty. Repaint from the shadow. The old code
    * got this for free by re-expanding the whole surface every frame; here it
    * costs one pass per palette change, and the palette changes twice over a
    * 1200 frame run. */
   if (!shadow)
      return;

   p = shadow;
   q = Retro_Screen;
   for (y = 0; y < SYSVID_HEIGHT; y++)
   {
      for (x = 0; x < SYSVID_WIDTH; x++)
         q[x] = palette[p[x]];
      p += SYSVID_WIDTH;
      q += WINDOW_WIDTH;
   }
}

void sysvid_setGamePalette(void)
{
   U8 i;
   img_color_t pal[256];

   for (i = 0; i < 32; ++i)
   {
      pal[i].r = RED[i];
      pal[i].g = GREEN[i];
      pal[i].b = BLUE[i];
   }
   sysvid_setPalette(pal, 32);
}

/*
 * Initialise video
 */
void sysvid_init(void)
{
   /* the 8bpp shadow of the visible screen; starts blank, as the surface it
    * replaces did (it was calloc'd) */
   shadow = calloc(1, SYSVID_WIDTH * SYSVID_HEIGHT);

   /*
    * create v_ frame buffer
    */
   sysvid_fb = calloc(1, SYSVID_WIDTH * SYSVID_HEIGHT);
}

/*
 * Shutdown video
 */
void sysvid_shutdown(void)
{
   if (sysvid_fb)
      free(sysvid_fb);
   sysvid_fb = NULL;

   if (shadow)
      free(shadow);
   shadow = NULL;

   memset(palette, 0, sizeof(palette));
   memset(palette_rgb, 0, sizeof(palette_rgb));
}

/*
 * Update screen
 * NOTE errors processing ?
 */
/*
 * Update the visible screen from the frame buffer.
 *
 * The dirty rectangles used to drive an 8 to 8 copy into a second surface,
 * and blit() then expanded that entire surface into the frontend buffer once
 * per frame regardless of what had changed - so the rectangle list narrowed
 * only the cheap half of the work and the palette expansion always ran over
 * the full 320x200. The expansion now happens here, over the dirty
 * rectangles, and blit() is gone.
 */
void sysvid_update(rect_t *rects)
{
   U16 x, y;
   const U8 *p, *p0;
   U8 *s, *s0;
   PIXEL_TYPE *q, *q0;

   if (!rects || !shadow)
      return;

   while (rects)
   {
      p0 = sysvid_fb    + rects->x + rects->y * SYSVID_WIDTH;
      s0 = shadow       + rects->x + rects->y * SYSVID_WIDTH;
      q0 = Retro_Screen + rects->x + rects->y * WINDOW_WIDTH;

      for (y = 0; y < rects->height; y++)
      {
         p = p0;
         s = s0;
         q = q0;
         for (x = 0; x < rects->width; x++)
         {
            *s = *p;
            *q = palette[*p];
            s++;
            q++;
            p++;
         }
         p0 += SYSVID_WIDTH;
         s0 += SYSVID_WIDTH;
         q0 += WINDOW_WIDTH;
      }

      rects = rects->next;
   }
}

/*
 * Clear screen
 * (077C)
 */
void sysvid_clear(void)
{
   memset(sysvid_fb, 0, SYSVID_WIDTH * SYSVID_HEIGHT);
}

/*
 * Repaint the whole frontend buffer from the shadow.
 */
static void sysvid_repaint(void)
{
   U16 x, y;
   const U8 *p;
   PIXEL_TYPE *q;

   if (!shadow)
      return;

   p = shadow;
   q = Retro_Screen;
   for (y = 0; y < SYSVID_HEIGHT; y++)
   {
      for (x = 0; x < SYSVID_WIDTH; x++)
         q[x] = palette[p[x]];
      p += SYSVID_WIDTH;
      q += WINDOW_WIDTH;
   }
}

/*
 * Return the video layer to the state a fresh load leaves it in.
 */
void sysvid_reset(void)
{
   if (sysvid_fb)
      memset(sysvid_fb, 0, SYSVID_WIDTH * SYSVID_HEIGHT);
   if (shadow)
      memset(shadow, 0, SYSVID_WIDTH * SYSVID_HEIGHT);

   memset(palette,      0, sizeof(palette));
   memset(palette_rgb,  0, sizeof(palette_rgb));
   memset(Retro_Screen, 0, sizeof(Retro_Screen));
}

void sysvid_serialize(serial_t *s)
{
   U16 i;

   if (sysvid_fb)
      serial_bytes(s, sysvid_fb, SYSVID_WIDTH * SYSVID_HEIGHT);
   if (shadow)
      serial_bytes(s, shadow, SYSVID_WIDTH * SYSVID_HEIGHT);

   /* the palette travels unpacked, so a state written by an RGB565 build
    * still loads into an XRGB8888 one */
   for (i = 0; i < 256; i++)
   {
      serial_u8(s, &palette_rgb[i].r);
      serial_u8(s, &palette_rgb[i].g);
      serial_u8(s, &palette_rgb[i].b);
   }

   if (!s->saving)
   {
      for (i = 0; i < 256; i++)
         palette[i] = sysvid_pack(palette_rgb[i].r, palette_rgb[i].g,
                                  palette_rgb[i].b);
      sysvid_repaint();
   }
}

/* eof */
