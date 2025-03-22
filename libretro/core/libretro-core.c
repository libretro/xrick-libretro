#include <libretro.h>
#include <streams/file_stream.h>
#include <file/file_path.h>
#include <string/stdstring.h>

#ifdef _MSC_VER
#include <compat/msvc.h>
#endif

#include "libretro-core.h"
#include "libretro_core_options.h"

#include "game.h"
#include "ents.h"
#include "system.h"

#define VIDEO_WIDTH 256
#define VIDEO_OFFSET_X ((WINDOW_WIDTH - VIDEO_WIDTH) >> 1)

int retrow=320; 
int retroh=200;
#ifdef FRONTEND_SUPPORTS_RGB565
#define BPP 2
#else
#define BPP 4
#endif

static struct retro_input_descriptor input_descriptors[] = {
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Jump" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Fire Gun" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Jab Stick (+ Left/Right)" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Set Dynamite" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause" },
   { 0 },
};

static bool retro_cheat1 = false;
static bool retro_cheat2 = false;
static bool retro_cheat3 = false;
static bool retro_cheat_pending = false;

static bool retro_crop_borders = false;

extern int SND;
static char RPATH[1024];
static char RETRO_DIR[1024];

SDL_Surface *sdlscrn; 

void SDL_Uninit(void)
{
   if (sdlscrn)
   {
      if(sdlscrn->format)	
      {
         if(sdlscrn->format->palette)	
         {
            if(sdlscrn->format->palette->colors)	
               free(sdlscrn->format->palette->colors);
            free(sdlscrn->format->palette);
         }
         free(sdlscrn->format);
      }

      if(sdlscrn->pixels)
         sdlscrn->pixels=NULL;

      free(sdlscrn);
      sdlscrn = NULL;
   }
}

#include "cmdline.c"

extern void texture_init(void);
extern void texture_uninit(void);

static retro_video_refresh_t video_cb;
retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;

bool libretro_supports_bitmasks = false;

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;
   bool option_cats_supported;
   bool no_content = true;

   environ_cb = cb;

   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   libretro_set_core_options(environ_cb, &option_cats_supported);

	vfs_iface_info.required_interface_version = 1;
	vfs_iface_info.iface                      = NULL;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
		filestream_vfs_init(&vfs_iface_info);
}

static void update_variables(bool startup)
{
   struct retro_variable var    = {0};
   bool retro_crop_borders_prev = retro_crop_borders;
   bool retro_cheat1_prev       = retro_cheat1;
   bool retro_cheat2_prev       = retro_cheat2;
   bool retro_cheat3_prev       = retro_cheat3;

   /* Crop Borders */
   var.key            = "xrick_crop_borders";
   var.value          = NULL;
   retro_crop_borders = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value) &&
       string_is_equal(var.value, "enabled"))
      retro_crop_borders = true;

   if (!startup && (retro_crop_borders != retro_crop_borders_prev))
   {
      struct retro_system_av_info av_info;
      retro_get_system_av_info(&av_info);
      environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);
   }

   /* Cheat 1: Trainer Mode */
   var.key      = "xrick_cheat1";
   var.value    = NULL;
   retro_cheat1 = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value) &&
       string_is_equal(var.value, "enabled"))
      retro_cheat1 = true;

   /* Cheat 2: Invulnerability Mode */
   var.key      = "xrick_cheat2";
   var.value    = NULL;
   retro_cheat2 = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value) &&
       string_is_equal(var.value, "enabled"))
      retro_cheat2 = true;

   /* Cheat 3: Expose Mode */
   var.key      = "xrick_cheat3";
   var.value    = NULL;
   retro_cheat3 = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value) &&
       string_is_equal(var.value, "enabled"))
      retro_cheat3 = true;

   /* Check if cheat settings have changed */
   if ((retro_cheat1 != retro_cheat1_prev) ||
       (retro_cheat2 != retro_cheat2_prev) ||
       (retro_cheat3 != retro_cheat3_prev))
      retro_cheat_pending = true;
}

void retro_reset(void) { }

void StartTicks(void);

void retro_init(void)
{    	
   const char *system_dir      = NULL;
#ifdef FRONTEND_SUPPORTS_RGB565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#else
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
#endif

   StartTicks();

   /* if defined, use the system directory */
   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) 
         && system_dir)
      strlcpy(RETRO_DIR, system_dir, sizeof(RETRO_DIR));
   else
      strlcpy(RETRO_DIR, ".", sizeof(RETRO_DIR));

   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      exit(0);

   memset(Key_Sate,0,512);
   memset(Key_Sate2,0,512);

   texture_init();

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;
}

void retro_deinit(void)
{	 
   texture_uninit();

   libretro_supports_bitmasks = false;

   retro_cheat1 = false;
   retro_cheat2 = false;
   retro_cheat3 = false;
   retro_cheat_pending = false;
   retro_crop_borders = false;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) { }

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "xrick";
   info->library_version  = "021212-Dev";
   info->valid_extensions = "zip";
   info->need_fullpath    = true;
   info->block_extract    = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   info->timing.fps               = 25.0;
   info->timing.sample_rate       = 22050.0;

   if (retro_crop_borders)
   {
      info->geometry.base_width   = VIDEO_WIDTH;
      info->geometry.base_height  = retroh;
      info->geometry.aspect_ratio = 4.0 / 3.0;
   }
   else
   {
      info->geometry.base_width   = retrow;
      info->geometry.base_height  = retroh;
      info->geometry.aspect_ratio = 5.0 / 3.0;
   }

   info->geometry.max_width       = retrow;
   info->geometry.max_height      = retroh;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

extern int Retro_PollEvent(void);
extern void syssnd_callback(U8 *stream, int len);

void retro_run(void)
{
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) 
         && updated)
      update_variables(false);

   if (retro_cheat_pending)
   {
      if (game_state != INTRO_MAIN && game_state != INTRO_MAP &&
          game_state != GAMEOVER && game_state != GETNAME &&
#ifdef ENABLE_DEVTOOLS
          game_state != DEVTOOLS &&
#endif
          game_state != XRICK && game_state != EXIT)
      {
         game_enableCheats(retro_cheat1, retro_cheat2, retro_cheat3);
         retro_cheat_pending = false;
      }
   }

   Retro_PollEvent();

   game_iterate();

   if (sdlscrn)
   {
      if (retro_crop_borders)
         video_cb((unsigned char *)sdlscrn->pixels + (VIDEO_OFFSET_X * BPP),
               VIDEO_WIDTH, retroh, retrow << PIXEL_BYTES);
      else
         video_cb((unsigned char *)sdlscrn->pixels,
               retrow, retroh, retrow << PIXEL_BYTES);
   }
}

bool retro_load_game(const struct retro_game_info *info)
{
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS,
         input_descriptors);

   if (info && !string_is_empty(info->path))
      snprintf(RPATH, sizeof(RPATH), "\"xrick\" \"-data\" \"%s\"", info->path);
   else
   {
      char data_path[1024];
      data_path[0] = '\0';

      fill_pathname_join_special_ext(data_path,
            RETRO_DIR, "xrick", "data", ".zip",
            sizeof(data_path));

      if (!path_is_valid(data_path))
      {
         unsigned msg_interface_version = 0;
         environ_cb(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION,
               &msg_interface_version);

         if (msg_interface_version >= 1)
         {
            struct retro_message_ext msg = {
               "XRick game files missing from frontend system directory",
               3000,
               3,
               RETRO_LOG_ERROR,
               RETRO_MESSAGE_TARGET_ALL,
               RETRO_MESSAGE_TYPE_NOTIFICATION,
               -1
            };
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
         }
         else
         {
            struct retro_message msg = {
               "XRick game files missing from frontend system directory",
               180
            };
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
         }

         goto error;
      }

      snprintf(RPATH, sizeof(RPATH), "\"xrick\" \"-data\" \"%s/xrick/data.zip\"", RETRO_DIR);
   }

#ifdef FRONTEND_SUPPORTS_RGB565
   memset(Retro_Screen, 0, WINDOW_WIDTH * WINDOW_HEIGHT * 2);
   sdlscrn = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 16, 0);
#else
   memset(Retro_Screen, 0, WINDOW_WIDTH * WINDOW_HEIGHT * 2 * 2);
   sdlscrn = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0);
#endif

   update_variables(true);

   SND=1;
   if (pre_main(RPATH) == -1)
      goto error;

   game_run();

   return true;

error:
   if (sdlscrn)
      SDL_Uninit();
   return false;
}

void freedata(void);

void retro_unload_game(void)
{
   freedata(); /* free cached data */
   data_closepath();
   sys_shutdown();

   rects_free(ent_rects);
   ent_rects = NULL;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   switch (id)
   {
      case RETRO_MEMORY_SAVE_RAM:
         return (void*)&game_hscores;
      default:
         break;
   }

   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   switch (id)
   {
      case RETRO_MEMORY_SAVE_RAM:
         return sizeof(game_hscores);
      default:
         break;
   }

   return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

