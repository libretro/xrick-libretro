/*
 * xrick/src/scr_gameover.c
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

#include "state.h"
#include "stddef.h" /* NULL */

#include "system.h"
#include "game.h"
#include "screens.h"

#include "draw.h"
#include "control.h"

/*
 * Display the game over screen
 *
 * return: SCREEN_RUNNING, SCREEN_DONE, SCREEN_EXIT
 */
static U8 seq = 0;
static U8 period = 0;
static U32 tm = 0;
static sound_t *snd;
static U8 chan;

/*
 * Reset to the state a fresh session expects.
 */
void scr_gameover_reset(void)
{
	seq = 0;
	period = 0;
	tm = 0;
	snd = NULL;
	chan = 0;
}

void scr_gameover_serialize(serial_t *s)
{
	serial_u8(s, &seq);
	serial_u8(s, &period);
	serial_u32(s, &tm);
	serial_u8(s, &chan);
	/* snd points at a cached sound owned by game.c; it is re-established by
	 * the sequence itself and deliberately not carried in the state */
}

U8
screen_gameover(void)
{
#ifdef GFXST
#endif
#ifdef ENABLE_SOUND
#endif

	if (seq == 0) {
		draw_tilesBank = 0;
		seq = 1;
		period = game_period; /* save period, */
		game_period = 50;     /* and use our own */
#ifdef ENABLE_SOUND
		game_setmusic("sounds/gameover.wav", 1);
#endif
	}

	switch (seq) {
	case 1:  /* display banner */
#ifdef GFXST
		sysvid_clear();
		tm = sys_gettime();
#endif
		draw_tllst = screen_gameovertxt;
		draw_setfb(120, 80);
#ifdef GFXPC
		draw_filter = 0xAAAA;
#endif
		draw_tilesList();

		game_rects = &draw_SCREENRECT;
		seq = 2;
		break;

	case 2:  /* wait for key pressed */
		if (control_status & CONTROL_FIRE)
			seq = 3;
#ifdef GFXST
		else if (sys_gettime() - tm > SCREEN_TIMEOUT)
			seq = 4;
#endif
		break;

	case 3:  /* wait for key released */
		if (!(control_status & CONTROL_FIRE))
			seq = 4;
		break;
	}

	if (control_status & CONTROL_EXIT)  /* check for exit request */
		return SCREEN_EXIT;

	if (seq == 4) {  /* we're done */
		sysvid_clear();
		seq = 0;
		game_period = period;
		return SCREEN_DONE;
	}

  return SCREEN_RUNNING;
}

/* eof */

