/*
 * xrick/src/system.c
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

#include "system.h"

/* forward declaration */
extern long GetTicks(void);

/*
 * Return number of microseconds elapsed since first call
 */
static U32 sys_ticks_base = 0;

U32 sys_gettime(void)
{
  U32 ticks = GetTicks();

  if (!sys_ticks_base)
    sys_ticks_base = ticks;

  return ticks - sys_ticks_base;
}

/*
 * Initialize system
 *
 * ret: 0 on success, -1 on failure
 *
 * NOTE no atexit() and no signal() here. This is a libretro core: atexit()
 * handlers registered from a dlopen()ed object are left dangling once the
 * frontend dlclose()s us, and hijacking SIGINT/SIGTERM steals them from the
 * host application.
 */
int sys_init(int argc, char **argv)
{
	sys_ticks_base = 0;

	if (sysarg_init(argc, argv) == -1)
		return -1;

	sysvid_init();
#ifdef ENABLE_SOUND
	if (sysarg_args_nosound == 0)
		syssnd_init();
#endif
	return 0;
}

/*
 * Shutdown system
 */
void sys_shutdown(void)
{
#ifdef ENABLE_SOUND
	syssnd_shutdown();
#endif
	sysvid_shutdown();
}

/* eof */
