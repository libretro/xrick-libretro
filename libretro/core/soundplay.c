#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "game.h"
#include "syssnd.h"
#include "debug.h"
#include "data.h"
#include <retro_endianness.h>

#define SDL_MIX_MAXVOLUME 128

#define ADJVOL(S) (((S)*sndVol)/SDL_MIX_MAXVOLUME)

static U8 isAudioActive = FALSE;
static channel_t channel[SYSSND_MIXCHANNELS];

static U8 sndVol = SDL_MIX_MAXVOLUME;  /* internal volume */
static U8 sndUVol = SYSSND_MAXVOL;  /* user-selected volume */
static U8 sndMute = FALSE;  /* mute flag */
static void end_channel(U8);

#include "libretro.h"
extern  retro_audio_sample_t audio_cb;

/*
 * Callback -- this is also where all sound mixing is done
 *
 * Note: it may not be that much a good idea to do all the mixing here ; it
 * may be more efficient to mix samples every frame, or maybe everytime a
 * new sound is sent to be played. I don't know.
 */
void syssnd_callback(U8 *stream, int len)
{
   U8 c;
   S16 s;
   U32 i;

   for (i = 0; i < (U32)len; i++)
   {
      s = 0;
      for (c = 0; c < SYSSND_MIXCHANNELS; c++)
      {
         if (channel[c].loop != 0)
         {  /* channel is active */
            if (channel[c].len > 0)
            {  /* not ending */
               s += ADJVOL(*channel[c].buf - 0x80);
               channel[c].buf++;
               channel[c].len--;
            }
            else
            {  /* ending */
               if (channel[c].loop > 0)
                  channel[c].loop--;

               if (channel[c].loop)
               {  /* just loop */
                  channel[c].buf = channel[c].snd->buf;
                  channel[c].len = channel[c].snd->len;
                  s += ADJVOL(*channel[c].buf - 0x80);
                  channel[c].buf++;
                  channel[c].len--;
               }
               else
               {  /* end for real */
                  end_channel(c);
               }
            }
         }
      }

      if (sndMute)
      {
         //stream[i] = 0x80;
         s = 0;
      }
      else
      {
#if 0
         s += 0x80;
         if (s > 0xff)
            s = 0xff;
         if (s < 0x00)
            s = 0x00;
         //stream[i] = (U8)s;
#endif
         s=s<<8 |s;
      }
      audio_cb(s,s);
   }

}


static void
end_channel(U8 c)
{
	channel[c].loop = 0;
	if (channel[c].snd->dispose)
		syssnd_free(channel[c].snd);
	channel[c].snd = NULL;
}


void syssnd_init(void)
{
   U8 c;

   if (sysarg_args_vol != 0)
   {
      sndUVol = sysarg_args_vol;
      sndVol = SDL_MIX_MAXVOLUME * sndUVol / SYSSND_MAXVOL;
   }

   for (c = 0; c < SYSSND_MIXCHANNELS; c++)
      channel[c].loop = 0;  /* deactivate */

   isAudioActive = TRUE;

}


/*
 * Shutdown
 */
void syssnd_shutdown(void)
{
   if (!isAudioActive)
      return;
   isAudioActive = FALSE;
}


/*
 * Toggle mute
 *
 * When muted, sounds are still managed but not sent to the dsp, hence
 * it is possible to un-mute at any time.
 */
void syssnd_toggleMute(void)
{
   sndMute = !sndMute;
}

void syssnd_vol(S8 d)
{
   if ((d < 0 && sndUVol > 0) ||
         (d > 0 && sndUVol < SYSSND_MAXVOL))
   {
      sndUVol += d;
      sndVol = SDL_MIX_MAXVOLUME * sndUVol / SYSSND_MAXVOL;
   }
}

/*
 * Play a sound
 *
 * loop: number of times the sound should be played, -1 to loop forever
 * returns: channel number, or -1 if none was available
 *
 * NOTE if sound is already playing, simply reset it (i.e. can not have
 * twice the same sound playing -- tends to become noisy when too many
 * bad guys die at the same time).
 */
S8 syssnd_play(sound_t *sound, S8 loop)
{
   S8 c;

   if (!isAudioActive)
      return -1;
   if (sound == NULL || sound->buf == NULL)
      return -1;

   c = 0;
   while ((channel[c].snd != sound || channel[c].loop == 0) &&
         channel[c].loop != 0 &&
         c < SYSSND_MIXCHANNELS)
      c++;
   if (c == SYSSND_MIXCHANNELS)
      c = -1;

   if (c >= 0)
   {
      channel[c].loop = loop;
      channel[c].snd = sound;
      channel[c].buf = sound->buf;
      channel[c].len = sound->len;
   }

   return c;
}

/*
 * Pause
 *
 * pause: TRUE or FALSE
 * clear: TRUE to cleanup all sounds and make sure we start from scratch
 */
void
syssnd_pause(U8 pause, U8 clear)
{
   U8 c;

   if (!isAudioActive)
      return;

   if (clear == TRUE)
   {
      for (c = 0; c < SYSSND_MIXCHANNELS; c++)
         channel[c].loop = 0;
   }
}

/*
 * Stop a channel
 */
void syssnd_stopchan(S8 chan)
{
   if (chan < 0 || chan > SYSSND_MIXCHANNELS)
      return;

   if (channel[chan].snd)
      end_channel(chan);

}

/*
 * Stop a sound
 */
void syssnd_stopsound(sound_t *sound)
{
   U8 i;

   if (!sound)
      return;

   for (i = 0; i < SYSSND_MIXCHANNELS; i++)
      if (channel[i].snd == sound) end_channel(i);

}

/*
 * See if a sound is playing
 */
int syssnd_isplaying(sound_t *sound)
{
   U8 i, playing;

   playing = 0;
   for (i = 0; i < SYSSND_MIXCHANNELS; i++)
      if (channel[i].snd == sound)
         playing = 1;

   return playing;
}


/*
 * Stops all channels.
 */
void
syssnd_stopall(void)
{
	U8 i;

	for (i = 0; i < SYSSND_MIXCHANNELS; i++)
		if (channel[i].snd)
         end_channel(i);
}


#define WAV_HEADER_SIZE 44

typedef struct 
{
   char ChunkID[4];
   uint32_t ChunkSize;
   char Format[4];
   char Subchunk1ID[4];
   uint32_t Subchunk1Size;
   uint16_t AudioFormat;
   uint16_t NumChannels;
   uint32_t SampleRate;
   uint32_t ByteRate;
   uint16_t BlockAlign;
   uint16_t BitsPerSample;
   char Subchunk2ID[4];
   uint32_t Subchunk2Size;
} wavhead_t;

//wavhead_t head;

/*
 * Load a sound.
 */
sound_t *syssnd_load(char *name)
{
   wavhead_t head;
   sound_t *s     = malloc(sizeof(sound_t));
   data_file_t *f = data_file_open(name);

   data_file_read(f, &head, 1, WAV_HEADER_SIZE);

   s->len=retro_le_to_cpu32(head.Subchunk2Size);
   s->buf=malloc(s->len);
   if (s->buf == NULL)
     return NULL;

   data_file_read(f, s->buf, 1, s->len);
   s->dispose = FALSE;

   data_file_close(f);

   return s;
}

/*
 *
 */
void syssnd_free(sound_t *s)
{
	if (!s)
      return;
	if (s->buf)
      free(s->buf);
	s->buf = NULL;
	s->len = 0;
}


