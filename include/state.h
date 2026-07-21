/*
 * xrick/include/state.h
 *
 * Savestate serialisation.
 */

#ifndef _STATE_H
#define _STATE_H

#include "system.h"

/*
 * One cursor used for both directions, so a save and its matching load can
 * not drift apart: every module has a single serialise function and the
 * direction is a flag, not a second copy of the field list.
 *
 * All multi byte values are written little endian regardless of host byte
 * order, so states are portable between platforms.
 */
typedef struct {
   U8  *buf;
   U32  size;
   U32  pos;
   U8   saving;   /* TRUE when writing, FALSE when reading */
   U8   failed;   /* set if the buffer ran out */
} serial_t;

extern void serial_u8(serial_t *s, U8 *v);
extern void serial_s8(serial_t *s, S8 *v);
extern void serial_u16(serial_t *s, U16 *v);
extern void serial_s16(serial_t *s, S16 *v);
extern void serial_u32(serial_t *s, U32 *v);
extern void serial_bytes(serial_t *s, void *p, U32 n);

extern void input_reset(void);
extern void input_serialize(serial_t *s);

extern U32 state_size(void);
extern int state_save(void *buf, U32 size);
extern int state_load(const void *buf, U32 size);

#endif

/* eof */
