#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "libretro.h"

#include "plat-joystick.h"
#include "ibm.h"
#include "cpu.h"
#include "model.h"
#include "nvr.h"
#include "video.h"

static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static bool use_audio_cb;
static float last_aspect;
static float last_sample_rate;

int quited = 0;

int romspresent[ROM_MAX];
int gfx_present[GFX_MAX];

int mouse_buttons;
joystick_t joystick_state[2];
int joysticks_present;

void initalmain(int argc, char *argv[])
{
}

void closeal()
{
}

void inital()
{
}

void check()
{
}

void givealbuffer_cd(int16_t *buf)
{
}

void givealbuffer(int16_t *buf)
{
}

void startblit()
{
}

void endblit()
{
}

static int ticks = 0;
static void timer_rout()
{
        ticks++;
}

uint64_t timer_freq;
uint64_t timer_read()
{
        return 0;
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   (void)level;
   va_list va;
   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

static PALETTE cgapal=
{
        {0,0,0},{0,42,0},{42,0,0},{42,21,0},
        {0,0,0},{0,42,42},{42,0,42},{42,42,42},
        {0,0,0},{21,63,21},{63,21,21},{63,63,21},
        {0,0,0},{21,63,63},{63,21,63},{63,63,63},

        {0,0,0},{0,0,42},{0,42,0},{0,42,42},
        {42,0,0},{42,0,42},{42,21,00},{42,42,42},
        {21,21,21},{21,21,63},{21,63,21},{21,63,63},
        {63,21,21},{63,21,63},{63,63,21},{63,63,63},

        {0,0,0},{0,21,0},{0,0,42},{0,42,42},
        {42,0,21},{21,10,21},{42,0,42},{42,0,63},
        {21,21,21},{21,63,21},{42,21,42},{21,63,63},
        {63,0,0},{42,42,0},{63,21,42},{41,41,41},
        
        {0,0,0},{0,42,42},{42,0,0},{42,42,42},
        {0,0,0},{0,42,42},{42,0,0},{42,42,42},
        {0,0,0},{0,63,63},{63,0,0},{63,63,63},
        {0,0,0},{0,63,63},{63,0,0},{63,63,63},
};

static uint32_t pal_lookup[256];

static void libretro_blit_memtoscreen(int x, int y, int y1, int y2, int w, int h)
{
#if 0
   if (h < winsizey)
   {
      int yy;

      for (yy = y+y1; yy < y+y2; yy++)
      {
         if (yy >= 0)
         {
            memcpy(&((uint32_t *)buffer32_vscale->line[yy*2])[x], &((uint32_t *)buffer32->line[yy])[x], w*4);
            memcpy(&((uint32_t *)buffer32_vscale->line[(yy*2)+1])[x], &((uint32_t *)buffer32->line[yy])[x], w*4);
         }
      }

      blit(buffer32_vscale, screen, x, (y+y1)*2, 0, y1, w, (y2-y1)*2);
   }
   else
      blit(buffer32, screen, x, y+y1, 0, y1, w, y2-y1);
#endif
}

static void libretro_blit_memtoscreen_8(int x, int y, int w, int h)
{
#if 0
   int xx, yy;

   if (y < 0)
   {
      h += y;
      y = 0;
   }

   for (yy = y; yy < y+h; yy++)
   {
      int dy = yy*2;
      for (xx = x; xx < x+w; xx++)
      {
         ((uint32_t *)buffer32->line[dy])[xx] =
            ((uint32_t *)buffer32->line[dy + 1])[xx] = pal_lookup[buffer->line[yy][xx]];
      }
   }

   if (readflash)
   {
      rectfill(buffer32, x+SCREEN_W-40, y*2+8, SCREEN_W-8, y*2+14, makecol(255, 255, 255));
      readflash = 0;
   }

   blit(buffer32, screen, x, y*2, 0, 0, w, h*2);
#endif
}

void retro_init(void)
{
   unsigned c;

   video_blit_memtoscreen   = libretro_blit_memtoscreen;
   video_blit_memtoscreen_8 = libretro_blit_memtoscreen_8;

   /* video initialization */
   for (c = 0; c < 256; c++)
      pal_lookup[c] = makecol(cgapal[c].r << 2, cgapal[c].g << 2, cgapal[c].b << 2);

}

void retro_deinit(void)
{
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "PCem";
   info->library_version  = "v1";
   info->need_fullpath    = false;
   info->valid_extensions = NULL; // Anything is fine, we don't care.
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.sample_rate    = 44100;
   info->geometry.base_width   = 320;
   info->geometry.base_height  = 240;
   info->geometry.max_width    = 320;
   info->geometry.max_height   = 240;
   info->geometry.aspect_ratio = (float)4/3;
}

static struct retro_rumble_interface rumble;

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   bool no_content = true;
   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   resetpchard();
}

static void check_variables(void)
{
}

static void audio_set_state(bool enable)
{
   (void)enable;
}

void retro_run(void)
{
   static int ticks = 0;
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();

   input_poll_cb();

   while (quited)
      return;

   if (ticks)
   {
      ticks--;
      runpc();
      frames++;
      if (frames >= 200 && nvr_dosave)
      {
         frames = 0;
         nvr_dosave = 0;
         savenvr();
      }
   }
   else
      rest(1);

           if (ticks > 10)
              ticks = 0;
   /* missing: audio_cb / video_cb */
}

static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
   log_cb(RETRO_LOG_INFO, "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
         down ? "yes" : "no", keycode, character, mod);
}

static void midi_init(void)
{
   /* TODO ? */
}

static void midi_close(void)
{
   /* TODO ? */
}

bool retro_load_game(const struct retro_game_info *info)
{
   int c, d;
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
   }

   struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);

   check_variables();

   midi_init();
   initpc(NULL, NULL);

   d = romset;
   for (c = 0; c < ROM_MAX; c++)
   {
      romset = c;
      romspresent[c] = loadbios();
      pclog("romset %i - %i\n", c, romspresent[c]);
   }

   for (c = 0; c < ROM_MAX; c++)
   {
      if (romspresent[c])
         break;
   }
   if (c == ROM_MAX)
   {
      printf("No ROMs present!\nYou must have at least one romset to use PCem.");
      return 0;
   }

   romset=d;
   c=loadbios();

   if (!c)
   {
      if (romset != -1)
         printf("Configured romset not available.\nDefaulting to available romset.");
      for (c = 0; c < ROM_MAX; c++)
      {
         if (romspresent[c])
         {
            romset = c;
            model = model_getmodel(romset);
            saveconfig();
            resetpchard();
            break;
         }
      }
   }

   for (c = 0; c < GFX_MAX; c++)
      gfx_present[c] = video_card_available(video_old_to_new(c));

   if (!video_card_available(video_old_to_new(gfxcard)))
   {
      if (gfxcard) printf("Configured video BIOS not available.\nDefaulting to available romset.");
      for (c = GFX_MAX-1; c >= 0; c--)
      {
         if (gfx_present[c])
         {
            gfxcard = c;
            saveconfig();
            resetpchard();
            break;
         }
      }
   }

	resetpchard();

   (void)info;
   return true;
}

void retro_unload_game(void)
{
   closepc();

   midi_close();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   return retro_load_game(NULL);
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return 0;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

