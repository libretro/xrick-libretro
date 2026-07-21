/*
 * xrick/include/system.h
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

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include "config.h"

/*
 * If compiling w/gcc, then we can use attributes. UNUSED(x) flags a
 * parameter or a variable as potentially being unused, so that gcc doesn't
 * complain about it.
 *
 * Note: from OpenAL code: Darwin OS cc is based on gcc and has __GNUC__
 * defined, yet does not support attributes. So in theory we should exclude
 * Darwin target here.
 */
#ifdef __GNUC__
#define UNUSED(x) x __attribute((unused))
#else
#define UNUSED(x) x
#endif

/*
 * Detect Microsoft Visual C
 */
#ifdef _MSC_VER
#define __MSVC__
/*
 * FIXME disable "integral size mismatch in argument; conversion supplied" warning
 * as long as the code has not been cleared -- there are so many of them...
 */

#pragma warning( disable : 4761 )
#endif

/* there are true at least on x86 platforms */
typedef unsigned char U8;         /*  8 bits unsigned */
typedef unsigned short int U16;   /* 16 bits unsigned */
typedef unsigned int U32;         /* 32 bits unsigned */
typedef signed char S8;           /*  8 bits signed   */
typedef signed short int S16;     /* 16 bits signed   */
typedef signed int S32;           /* 32 bits signed   */

/* this must be after typedefs because it relies on types defined above */
#include "rects.h"
#include "img.h"

/*
 * main section
 */
extern int sys_init(int, char **);
extern void sys_shutdown(void);
extern U32 sys_gettime(void);

/*
 * video section
 */
#define SYSVID_MAXZOOM 4
#define SYSVID_WIDTH 320
#define SYSVID_HEIGHT 200

extern void sysvid_init(void);
extern void sysvid_shutdown(void);
extern void sysvid_update(rect_t *);
extern void sysvid_clear(void);
extern void sysvid_setGamePalette(void);
extern void sysvid_setPalette(img_color_t *, U16);

/*
 * sound section
 */
#ifdef ENABLE_SOUND
typedef struct {
#ifdef DEBUG
  char *name;
#endif
  U8 *buf;
  U32 len;
  U8 dispose;
} sound_t;

extern void syssnd_init(void);
extern void syssnd_shutdown(void);
extern void syssnd_vol(S8);
extern void syssnd_toggleMute(void);
extern S8 syssnd_play(sound_t *, S8);
extern void syssnd_pause(U8, U8);
extern void syssnd_stopchan(S8);
extern void syssnd_stopsound(sound_t *);
extern void syssnd_stopall();
extern int syssnd_isplaying(sound_t *);
extern sound_t *syssnd_load(char *name);
extern void syssnd_free(sound_t *);
#endif

/*
 * args section
 */
extern int sysarg_args_period;
extern int sysarg_args_map;
extern int sysarg_args_submap;
#ifdef ENABLE_SOUND
extern int sysarg_args_nosound;
extern int sysarg_args_vol;
#endif
extern char *sysarg_args_data;

extern int sysarg_init(int, char **);
extern int sysarg_fail(char *);

#endif

/* eof */


