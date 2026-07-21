/*
 * xrick/src/control.c
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
#include "config.h"
#include "system.h"
#include "game.h"

U8 control_status = 0;
U8 control_last = 0;
U8 control_active = TRUE;

/*
 * Reset to the state a fresh session expects.
 */
void control_reset(void)
{
  control_status = 0;
  control_last   = 0;
  control_active = TRUE;
}

void control_serialize(serial_t *s)
{
  serial_u8(s, &control_status);
  serial_u8(s, &control_last);
  serial_u8(s, &control_active);
}

/* eof */


