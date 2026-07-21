/*
 * xrick/src/state.c
 *
 * Savestate serialisation.
 */

#include <string.h>

#include "state.h"
#include "game.h"
#include "draw.h"
#include "maps.h"
#include "ents.h"
#include "control.h"
#include "scroller.h"
#include "screens.h"
#include "e_rick.h"
#include "e_them.h"
#include "e_bomb.h"
#include "e_bullet.h"
#include "e_sbonus.h"
#include "sysvid.h"

#define STATE_MAGIC   0x4B434952UL   /* 'RICK' */
#define STATE_VERSION 1

/*
 * Reserve n bytes and return where they live, or NULL if the buffer is out.
 */
static U8 *serial_claim(serial_t *s, U32 n)
{
   U8 *p;

   U32 at;

   if (s->failed)
      return NULL;
   if (s->pos + n > s->size)
   {
      s->failed = TRUE;
      return NULL;
   }
   at = s->pos;
   s->pos += n;

   /* measuring pass: advance the cursor, touch nothing */
   if (!s->buf)
      return NULL;

   p = s->buf + at;
   return p;
}

void serial_u8(serial_t *s, U8 *v)
{
   U8 *p = serial_claim(s, 1);
   if (!p)
      return;
   if (s->saving)
      p[0] = *v;
   else
      *v = p[0];
}

void serial_s8(serial_t *s, S8 *v)
{
   U8 t = (U8)*v;
   serial_u8(s, &t);
   if (!s->saving)
      *v = (S8)t;
}

void serial_u16(serial_t *s, U16 *v)
{
   U8 *p = serial_claim(s, 2);
   if (!p)
      return;
   if (s->saving)
   {
      p[0] = (U8)(*v & 0xff);
      p[1] = (U8)((*v >> 8) & 0xff);
   }
   else
      *v = (U16)(p[0] | ((U16)p[1] << 8));
}

void serial_s16(serial_t *s, S16 *v)
{
   U16 t = (U16)*v;
   serial_u16(s, &t);
   if (!s->saving)
      *v = (S16)t;
}

void serial_u32(serial_t *s, U32 *v)
{
   U8 *p = serial_claim(s, 4);
   if (!p)
      return;
   if (s->saving)
   {
      p[0] = (U8)(*v & 0xff);
      p[1] = (U8)((*v >> 8) & 0xff);
      p[2] = (U8)((*v >> 16) & 0xff);
      p[3] = (U8)((*v >> 24) & 0xff);
   }
   else
      *v = (U32)p[0] | ((U32)p[1] << 8) | ((U32)p[2] << 16) | ((U32)p[3] << 24);
}

void serial_bytes(serial_t *s, void *ptr, U32 n)
{
   U8 *p = serial_claim(s, n);
   if (!p)
      return;
   if (s->saving)
      memcpy(p, ptr, n);
   else
      memcpy(ptr, p, n);
}

/*
 * Walk every module's state.
 */
static void state_serialize(serial_t *s)
{
   U32 magic   = STATE_MAGIC;
   U32 version = STATE_VERSION;

   serial_u32(s, &magic);
   serial_u32(s, &version);
   if (!s->saving && (magic != STATE_MAGIC || version != STATE_VERSION))
   {
      s->failed = TRUE;
      return;
   }

   game_serialize(s);
   input_serialize(s);
   control_serialize(s);
   draw_serialize(s);
   maps_serialize(s);
   ents_serialize(s);
   scroller_serialize(s);
   scr_xrick_serialize(s);
   scr_imain_serialize(s);
   scr_imap_serialize(s);
   scr_getname_serialize(s);
   scr_gameover_serialize(s);
   e_rick_serialize(s);
   e_them_serialize(s);
   e_bomb_serialize(s);
   e_bullet_serialize(s);
   e_sbonus_serialize(s);
   sysvid_serialize(s);
}

/*
 * Size of a state.
 *
 * Measured by running the walk against a null cursor rather than by adding
 * up sizeof()s by hand, so it can not fall out of step with what is actually
 * written.
 */
U32 state_size(void)
{
   static U32 cached = 0;
   serial_t s;

   if (cached)
      return cached;

   memset(&s, 0, sizeof(s));
   s.buf    = NULL;             /* measuring pass */
   s.size   = 0xffffffffUL;
   s.pos    = 0;
   s.saving = FALSE;
   s.failed = FALSE;

   state_serialize(&s);

   cached = s.pos;
   return cached;
}

int state_save(void *buf, U32 size)
{
   serial_t s;

   memset(&s, 0, sizeof(s));
   s.buf    = (U8 *)buf;
   s.size   = size;
   s.pos    = 0;
   s.saving = TRUE;
   s.failed = FALSE;

   state_serialize(&s);

   return s.failed ? -1 : 0;
}

int state_load(const void *buf, U32 size)
{
   serial_t s;

   memset(&s, 0, sizeof(s));
   s.buf    = (U8 *)buf;   /* not written to when saving is FALSE */
   s.size   = size;
   s.pos    = 0;
   s.saving = FALSE;
   s.failed = FALSE;

   state_serialize(&s);

   return s.failed ? -1 : 0;
}

/* eof */
