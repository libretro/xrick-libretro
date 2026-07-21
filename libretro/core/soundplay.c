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

/*
 * Mix one video frame's worth of audio into a stereo S16 buffer.
 *
 * stream: interleaved L/R, must hold frames * 2 samples
 * frames: number of sample frames to produce
 *
 * The mix accumulator is S32 and the user volume is applied once, to the
 * summed result. It used to be applied per channel, to the 8 bit sample,
 * with an integer divide by SDL_MIX_MAXVOLUME - so every active channel
 * contributed its own rounding error before the sum. At full volume the
 * divide is exact and this is bit identical; below full volume it is simply
 * more accurate.
 *
 * The accumulator is deliberately still clamped to the signed 8 bit window
 * before being scaled up. That clip point is the original mixer's behaviour
 * and part of how the game sounds when several effects overlap; widening it
 * would change the output, not just its precision.
 */
void syssnd_mix(S16 *stream, int frames)
{
   U8 c;
   S32 acc;
   S16 s;
   int i;

   for (i = 0; i < frames; i++)
   {
      acc = 0;
      for (c = 0; c < SYSSND_MIXCHANNELS; c++)
      {
         if (channel[c].loop != 0)
         {  /* channel is active */
            if (channel[c].len > 0)
            {  /* not ending */
               acc += (S32)(*channel[c].buf) - 0x80;
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
                  acc += (S32)(*channel[c].buf) - 0x80;
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
         acc = 0;
      else
      {
         acc = (acc * (S32)sndVol) / SDL_MIX_MAXVOLUME;
         if (acc >  127) acc =  127;
         if (acc < -128) acc = -128;
      }

      /* multiply rather than shift: the operand is signed and the old
       * '(s - 128) << 8' was undefined whenever the sample went negative */
      s = (S16)(acc * 256);

      stream[i * 2]     = s;
      stream[i * 2 + 1] = s;
   }
}


static void
end_channel(U8 c)
{
	channel[c].loop = 0;
	if (channel[c].snd && channel[c].snd->dispose)
		syssnd_free(channel[c].snd);
	channel[c].snd = NULL;
	channel[c].buf = NULL;
	channel[c].len = 0;
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
   {
      channel[c].loop = 0;  /* deactivate */
      channel[c].snd  = NULL;
      channel[c].buf  = NULL;
      channel[c].len  = 0;
   }

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
   U8 i;

   if (!isAudioActive)
      return -1;
   if (sound == NULL || sound->buf == NULL)
      return -1;

   /* Reuse the channel this sound is already playing on if there is one,
    * otherwise take the first free channel.
    *
    * NOTE the bound must be tested before channel[c] is dereferenced. The
    * previous form tested it last, so the scan read channel[SYSSND_MIXCHANNELS]
    * - one element past the array - whenever every channel was busy. */
   c = -1;
   for (i = 0; i < SYSSND_MIXCHANNELS; i++)
   {
      if (channel[i].loop != 0 && channel[i].snd == sound)
      {
         c = i;   /* already playing: restart it in place */
         break;
      }
      if (channel[i].loop == 0 && c < 0)
         c = i;   /* remember the first free slot, keep looking for a match */
   }

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
      /* clear the snd/buf mirrors too: leaving them set behind a zeroed loop
       * count leaves dangling pointers into sounds that may since be freed */
      for (c = 0; c < SYSSND_MIXCHANNELS; c++)
         if (channel[c].snd)
            end_channel(c);
         else
            channel[c].loop = 0;
   }
}

/*
 * Stop a channel
 */
void syssnd_stopchan(S8 chan)
{
   if (chan < 0 || chan >= SYSSND_MIXCHANNELS)
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

   if (!s || !f)
      goto error;

   data_file_read(f, &head, 1, WAV_HEADER_SIZE);

   s->len         = retro_le_to_cpu32(head.Subchunk2Size);
   s->buf         = malloc(s->len);
   if (!s->buf)
      goto error;

   data_file_read(f, s->buf, 1, s->len);
   s->dispose = FALSE;

   data_file_close(f);

   return s;

error:
   if (s)
      free(s);
   if (f)
      data_file_close(f);
   return NULL;
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
	free(s);
}


