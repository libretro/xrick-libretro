/*
 * xrick/src/xrick.c
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

#include "system.h"
#include "game.h"

/* main */
int skel_main(int argc, char *argv[])
{
   if (sys_init(argc, argv) == -1)
      return -1;
   if (sysarg_args_data)
      return data_setpath(sysarg_args_data);
   return data_setpath("data.zip");
}

/* eof */
