#include "libretro.h"
#include "libretro-core.h"


 
//TIME
#ifdef __PS3__
#ifdef __PSL1GHT__
#include <sys/systime.h>
#else
#include <sys/sys_time.h>
#include <sys/timer.h>
#define sysGetCurrentTime sys_time_get_current_time
#endif
#elif defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#endif

#include "system.h"
#include "control.h"
#include "game.h"

#define SETBIT(x,b) x |= (b)
#define CLRBIT(x,b) x &= ~(b)

//VIDEO
#ifdef FRONTEND_SUPPORTS_RGB565
uint16_t Retro_Screen[WINDOW_WIDTH*WINDOW_HEIGHT];
#else
uint32_t Retro_Screen[WINDOW_WIDTH*WINDOW_HEIGHT];
#endif

//KEYBOARD
char Key_Sate[512];
char Key_Sate2[512];

static retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

#ifdef _WIN32
#define TIME_WRAP_VALUE	(~(DWORD)0)
/* The first (low-resolution) ticks value of the application */
static DWORD start;
#endif

void StartTicks(void)
{
#ifdef _WIN32
	start = GetTickCount();
#endif
}

long GetTicks(void)
{
#ifdef _WIN32
   DWORD ticks;
   DWORD now = GetTickCount();

   if ( now < start )
      ticks = (TIME_WRAP_VALUE-start) + now;
   else
      ticks = (now - start);

   return(ticks);
#else
#ifndef _ANDROID_

#ifdef __PS3__
   unsigned long	ticks_micro;
   uint64_t secs;
   uint64_t nsecs;

   sysGetCurrentTime(&secs, &nsecs);
   ticks_micro =  secs * 1000000UL + (nsecs / 1000);

   return ticks_micro/1000;
#else
   struct timeval tv;
   gettimeofday (&tv, NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec)/1000;
#endif

#else

   struct timespec now;
   clock_gettime(CLOCK_MONOTONIC, &now);
   return (now.tv_sec*1000000 + now.tv_nsec/1000)/1000;
#endif
#endif
} 

void texture_uninit(void)
{
   memset(Retro_Screen, 0, sizeof(Retro_Screen));
}

void texture_init(void)
{
   memset(Retro_Screen, 0, sizeof(Retro_Screen));
}

static void retro_key_down(unsigned short key)
{
   switch (key)
   {
      case XRICK_KEY_UP:
         SETBIT(control_status, CONTROL_UP);
         control_last = CONTROL_UP;
         break;
      case XRICK_KEY_DOWN:
         SETBIT(control_status, CONTROL_DOWN);
         control_last = CONTROL_DOWN;
         break;
      case XRICK_KEY_LEFT:
         SETBIT(control_status, CONTROL_LEFT);
         control_last = CONTROL_LEFT;
         break;
      case XRICK_KEY_RIGHT:
         SETBIT(control_status, CONTROL_RIGHT);
         control_last = CONTROL_RIGHT;
         break;
      case XRICK_KEY_PAUSE:
         SETBIT(control_status, CONTROL_PAUSE);
         control_last = CONTROL_PAUSE;
         break;
      case XRICK_KEY_END:
         SETBIT(control_status, CONTROL_END);
         control_last = CONTROL_END;
         break;
      case XRICK_KEY_ESCAPE:
         SETBIT(control_status, CONTROL_EXIT);
         control_last = CONTROL_EXIT;
         break;
      case XRICK_KEY_SPACE:
         SETBIT(control_status, CONTROL_FIRE);
         control_last = CONTROL_FIRE;
         break;
   }
}

static void retro_key_up(unsigned short key)
{
   switch (key)
   {
      case XRICK_KEY_UP:
         CLRBIT(control_status, CONTROL_UP);
         control_last = CONTROL_UP;
         break;
      case XRICK_KEY_DOWN:
         CLRBIT(control_status, CONTROL_DOWN);
         control_last = CONTROL_DOWN;
         break;
      case XRICK_KEY_LEFT:
         CLRBIT(control_status, CONTROL_LEFT);
         control_last = CONTROL_LEFT;
         break;
      case XRICK_KEY_RIGHT:
         CLRBIT(control_status, CONTROL_RIGHT);
         control_last = CONTROL_RIGHT;
         break;
      case XRICK_KEY_PAUSE:
         CLRBIT(control_status, CONTROL_PAUSE);
         control_last = CONTROL_PAUSE;
         break;
      case XRICK_KEY_END:
         CLRBIT(control_status, CONTROL_END);
         control_last = CONTROL_END;
         break;
      case XRICK_KEY_ESCAPE:
         CLRBIT(control_status, CONTROL_EXIT);
         control_last = CONTROL_EXIT;
         break;
      case XRICK_KEY_SPACE:
         CLRBIT(control_status, CONTROL_FIRE);
         control_last = CONTROL_FIRE;
         break;
   }
}



int SurfaceFormat=3;

#define key_latch(key) \
   if(Key_Sate[key]  && Key_Sate2[key]==0) \
   { \
      retro_key_down(key); \
      Key_Sate2[key] = 1; \
   } \
   else if ( !Key_Sate[key] && Key_Sate2[key]==1 ) \
   { \
      retro_key_up(key); \
      Key_Sate2[key] = 0; \
   }

int Retro_PollEvent(void)
{
   extern bool libretro_supports_bitmasks;
   unsigned int i;
   unsigned joypad_bits;
   unsigned short key = 0;

   bool jump_pressed         = false;
   bool fire_gun_pressed     = false;
   bool set_dynamite_pressed = false;
   bool jab_stick_pressed    = false;

   /* An annoyance - 'Fire Gun' and 'Set Dynamite'
    * will not register correctly unless a directional
    * input is triggered on the frame *after* the fire
    * button is triggered */
   static bool fire_gun_pressed_prev     = false;
   static bool set_dynamite_pressed_prev = false;

   input_poll_cb();

   if (libretro_supports_bitmasks)
      joypad_bits = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
   else
   {
      joypad_bits = 0;
      for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
         joypad_bits |= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
   }

   /* If we are currently in a menu-type screen,
    * then directions are registered normally and
    * all face buttons trigger a 'fire' input */
   if ((game_state == INTRO_MAIN) ||
       (game_state == INTRO_MAP) ||
       (game_state == GAMEOVER) ||
       (game_state == GETNAME))
   {
      key           = XRICK_KEY_UP;
      Key_Sate[key] = joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP) ? 0x80: 0;
      key_latch(key);

      key           = XRICK_KEY_DOWN;
      Key_Sate[key] = joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN) ? 0x80: 0;
      key_latch(key);

      key           = XRICK_KEY_LEFT;
      Key_Sate[key] = joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT) ? 0x80: 0;
      key_latch(key);

      key           = XRICK_KEY_RIGHT;
      Key_Sate[key] = joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT) ? 0x80: 0;
      key_latch(key);

      key           = XRICK_KEY_SPACE;
      Key_Sate[key] = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A)) ||
                      (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B)) ||
                      (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_Y)) ||
                      (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_X))    ? 0x80: 0;
      key_latch(key);

      fire_gun_pressed_prev     = false;
      set_dynamite_pressed_prev = false;
      return 1;
   }

   jump_pressed         = (bool)(joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A));
   fire_gun_pressed     = (bool)(joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B));
   set_dynamite_pressed = (bool)(joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_Y));
   jab_stick_pressed    = (bool)(joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_X));

   /* Up is triggered when:
    * - 'Jump' is pressed
    * - 'Fire Gun' is pressed (prev and current)
    * - 'Up' is pressed
    * Up is ignored when:
    * - 'Set Dynamite' is pressed
    * - 'Jab Stick' is pressed */
   key           = XRICK_KEY_UP;
   Key_Sate[key] = (jump_pressed ||
                    (fire_gun_pressed && fire_gun_pressed_prev) ||
                    (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP))) &&
                   !(set_dynamite_pressed || jab_stick_pressed) ? 0x80: 0;
   key_latch(key);

   /* Down is triggered when:
    * - 'Set Dynamite' is pressed (prev and current)
    * - 'Down' is pressed
    * Down is ignored when:
    * - 'Jump' is pressed
    * - 'Fire Gun' is pressed
    * - 'Jab Stick' is pressed */
   key           = XRICK_KEY_DOWN;
   Key_Sate[key] = ((set_dynamite_pressed && set_dynamite_pressed_prev) ||
                    (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))) &&
                   !(jump_pressed || fire_gun_pressed || jab_stick_pressed) ? 0x80: 0;
   key_latch(key);

   /* Left is triggered when:
    * - 'Left' is pressed
    * Left is ignored when:
    * - 'Fire Gun' is pressed
    * - 'Set Dynamite' is pressed */
   key           = XRICK_KEY_LEFT;
   Key_Sate[key] = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) &&
                   !(fire_gun_pressed || set_dynamite_pressed) ? 0x80: 0;
   key_latch(key);

   /* Right is triggered when:
    * - 'Right' is pressed
    * Right is ignored when:
    * - 'Fire Gun' is pressed
    * - 'Set Dynamite' is pressed */
   key           = XRICK_KEY_RIGHT;
   Key_Sate[key] = (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) &&
                   !(fire_gun_pressed || set_dynamite_pressed) ? 0x80: 0;
   key_latch(key);

   /* Fire is triggered when:
    * - 'Fire Gun' is pressed
    * - 'Set Dynamite' is pressed
    * - 'Jab Stick' is pressed
    * Fire is ignored when:
    * - 'Jump' is pressed */
   key           = XRICK_KEY_SPACE;
   Key_Sate[key] = (fire_gun_pressed ||
                    set_dynamite_pressed ||
                    jab_stick_pressed) &&
                   !jump_pressed ? 0x80: 0;
   key_latch(key);

   /* Pause */
   key           = XRICK_KEY_PAUSE;
   Key_Sate[key] = joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START) ? 0x80: 0;
   key_latch(key);

   fire_gun_pressed_prev     = fire_gun_pressed;
   set_dynamite_pressed_prev = set_dynamite_pressed;

   return 1;
}

