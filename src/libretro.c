#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "libretro.h"

#include "plat-joystick.h"
#include "ibm.h"
#include "cpu.h"
#include "model.h"
#include "nvr.h"
#include "video.h"

typedef struct
{
   unsigned char r;
   unsigned char g;
   unsigned char b;
}APALETTE;

static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
static bool use_audio_cb;
static float last_aspect;
static float last_sample_rate;

int quited = 0;

int pcem_key[272];
int rawinputkey[272];

int romspresent[ROM_MAX];
int gfx_present[GFX_MAX];

static retro_video_refresh_t video_cb;

/* forward declarations */
void closepc(void);
void saveconfig(void);
void savenvr(void);
int loadbios(void);
void initpc(int argc, char *argv[]);
void runpc(void);
void resetpchard(void);

void set_window_title(char *s)
{
}

void get_executable_name(char *s, int size)
{
}

int mouse_buttons;
joystick_t joystick_state[2];
int joysticks_present;

void joystick_init()
{
#if 0
        install_joystick(JOY_TYPE_AUTODETECT);
        joysticks_present = MIN(num_joysticks, 2);
#endif
}
void joystick_close()
{
}
void joystick_poll()
{
   int c;

#if 0
   poll_joystick();

   for (c = 0; c < MIN(num_joysticks, 2); c++)
   {
      joystick_state[c].x = joy[c].stick[0].axis[0].pos * 256;
      joystick_state[c].y = joy[c].stick[0].axis[1].pos * 256;
      joystick_state[c].b[0] = joy[c].button[0].b;
      joystick_state[c].b[1] = joy[c].button[1].b;
      joystick_state[c].b[2] = joy[c].button[2].b;
      joystick_state[c].b[3] = joy[c].button[3].b;
   }
#endif
}

void mouse_init()
{
}

void mouse_get_mickeys(int *x, int *y)
{
}

void mouse_poll_host()
{
   //poll_mouse();
}

static int key_convert[128] =
{
          -1, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, /*   ,   A,   B,   C,  D,  E,  F,  G*/
        0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, /*  H,   I,   J,   K,  L,  M,  N,  O*/
        0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, /*  P,   Q,   R,   S,  T,  U,  V,  W*/
        0x2d, 0x15, 0x2c, 0x0b, 0x02, 0x03, 0x04, 0x05, /*  X,   Y,   Z,   0,  1,  2,  3,  4*/
        0x06, 0x07, 0x08, 0x09, 0x0a, 0x52, 0x4f, 0x50, /*  5,   6,   7,   8,  9, p0, p1, p2*/
        0x51, 0x4b, 0x4c, 0x4d, 0x47, 0x48, 0x49, 0x3b, /* p3,  p4,  p5,  p6, p7, p8, p9, F1*/
        0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, /* F2,  F3,  F4,  F5, F6, F7, F8, F9*/
        0x44, 0x57, 0x58, 0x01, 0x29, 0x0c, 0x0d, 0x0e, /*F10, F11, F12, ESC, `ª, -_, =+, backspace*/
        0x0f, 0x1a, 0x1b, 0x1c, 0x27, 0x28, 0x2b, 0x56, /*TAB,  [{,  ]}, ENT, ;:, '@, \|, #~*/
        0x33, 0x34, 0x35, 0x39, 0xd2, 0xd3, 0xc7, 0xcf, /* ,<,  .>,  /?, SPC, INS, DEL, HOME, END*/
        0xc9, 0xd1, 0xcb, 0xcd, 0xc8, 0xd0, 0xb5, 0x37, /*PGU, PGD, LFT, RHT,  UP,  DN,  /, * */
        0x4a, 0x4e, 0x53, 0x9c, 0xff,   -1,   -1,   -1, /* p-,  p+, pDL, pEN, psc, pse, abnt, yen*/
          -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, /*kana, convert, noconvert, at, circumflex, colon2, kanji, pad equals*/
          -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
          -1,   -1,   -1, 0x2a, 0x36, 0x1d, 0x9d, 0x38, /*, , lshift, rshift, lctrl, rctrl, alt*/
        0xb8, 0xdb, 0xdc, 0xdd, 0x46, 0x45, 0x3a,   -1  /*altgr, lwin, rwin, menu, scrlock, numlock, capslock*/
};

void keyboard_init()
{
#if 0
   install_keyboard();
#endif
}

void keyboard_close()
{
}

void keyboard_poll_host()
{
#if 0
        int c;

        for (c = 0; c < 128; c++)
        {
                int key_idx = key_convert[c];
                if (key_idx == -1)
                        continue;

#if 0
                if (key[c] != pcem_key[key_idx])
                        pcem_key[key_idx] = key[c];
#endif
        }
#endif
}

void updatewindowsize(int x, int y) { }

void hline(BITMAP *b, int x1, int y, int x2, uint32_t col)
{
        if (y < 0 || y >= buffer8->h)
           return;

        if (b == buffer8)
           memset(&b->line[y][x1], col, x2 - x1);
        else
           memset(&((uint32_t *)b->line[y])[x1], col, (x2 - x1) * 4);
}

BITMAP *create_bitmap(int x,int y)
{
   BITMAP *b = malloc(sizeof(BITMAP) + (y * sizeof(uint8_t *)));
   int c;
   b->dat = malloc(x * y * 4);
   for (c = 0; c < y; c++)
   {
      b->line[c] = b->dat + (c * x * 4);
   }
   b->w = x;
   b->h = y;
   return b;
}

int destroy_bitmap(BITMAP *buff)
{
   if (buff->dat)
      free(buff->dat);
   free(buff);

   return 0;
}

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

void blit(BITMAP *src, int x1, int y1, int x2, int y2, int xs, int ys)
{
   video_cb(src->dat, src->w, src->h, src->w);
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
static uint32_t video_buffer[2048 * 2048];
int winsizex=640;
int winsizey=480;

static void libretro_blit_memtoscreen(int x, int y, int y1, int y2, int w, int h)
{
   int xx, yy;
   uint32_t *p;

   if((w < 0) || (w > 2048) || (h < 0) || (h > 2048))
      return;

   memset(video_buffer, 0, 2048 * 2048 * sizeof(uint32_t));

   for (yy = y1; yy < y2; yy++)
   {
      if ((y + yy) >= 0 && (y + yy) < buffer8->h)
         memcpy(video_buffer + yy * 2048, &(((uint32_t *)buffer32->line[y + yy])[x]), w * 4);
   }

   if (readflash)
   {
      readflash = 0;
      if (enable_flash)
      {
         for (yy = 8; yy < 14; yy++)
         {
            p = video_buffer + yy * 2048;
            for (xx = (w - 40); xx < (w - 8); xx++)
               p[xx] = 0xffffffff;
         }
      }
   }

   video_cb(video_buffer, w, h, 2048 * 4);
}

static void libretro_blit_memtoscreen_8(int x, int y, int w, int h)
{
   int xx, yy;
   uint32_t *p;

   if((w < 0) || (w > 2048) || (h < 0) || (h > 2048))
      return;

   memset(video_buffer, 0, 2048 * 2048 * sizeof(uint32_t));

   for (yy = 0; yy < h; yy++)
   {
      if ((y + yy) >= 0 && (y + yy) < buffer8->h)
      {
         p = video_buffer + yy * 2048;
         for (xx = 0; xx < w; xx++)
         {
            p[xx] = pal_lookup[buffer8->line[y + yy][x + xx]];
            /* If brown circuity is disabled, double the green component. */
            if ((buffer8->line[y + yy][x + xx] == 0x16) && !cga_brown)  p[xx] += (p[xx] & 0xff00);
         }
      }
   }

   if (readflash)
   {
      readflash = 0;
      if (enable_flash)
      {
         for (yy = 8; yy < 14; yy++)
         {
            p = (uint32_t *)(video_buffer + (yy * sizeof(uint32_t)));
            for (xx = (w - 40); xx < (w - 8); xx++)
               p[xx] = 0xffffffff;
         }
      }
   }

   video_cb(video_buffer, w, h, 2048 * 4);
}

void rectfill(BITMAP *b, int x1, int y1, int x2, int y2, uint32_t col)
{
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
   info->library_version  = "v12 alpha";
   info->need_fullpath    = false;
   info->valid_extensions = NULL; // Anything is fine, we don't care.
}

static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

const char* retro_get_system_directory(void)
{
    const char* dir;
    environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir);

    return dir ? dir : ".";
}

void retro_init(void)
{
#ifdef _WIN32
   char slash = '\\';
#else
   char slash = '/';
#endif
   unsigned i;
   unsigned color_mode = RETRO_PIXEL_FORMAT_XRGB8888;
   const char *system_dir = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &color_mode);

   system_dir = retro_get_system_directory();

   sprintf(pcempath, "%s%c%s%c", system_dir, slash, "pcem", slash);

   /* video initialization */
   video_blit_memtoscreen   = libretro_blit_memtoscreen;
   video_blit_memtoscreen_8 = libretro_blit_memtoscreen_8;

   for (i = 0; i < 256; i++)
      pal_lookup[i] = makecol(cgapal[i].r << 2, cgapal[i].g << 2, cgapal[i].b << 2);

}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps            = 60;
   info->timing.sample_rate    = 44100;
   info->geometry.base_width   = 320;
   info->geometry.base_height  = 240;
   info->geometry.max_width    = 2048;
   info->geometry.max_height   = 2048;
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

   if (quited)
      return;

   {
      runpc();
      frames++;
      if (frames >= 200 && nvr_dosave)
      {
         frames = 0;
         nvr_dosave = 0;
         savenvr();
      }
   }
   /* missing: audio_cb / video_cb */

}

static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
   bool latch = down ? 1 : 0;


   log_cb(RETRO_LOG_INFO, "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
         down ? "yes" : "no", keycode, character, mod);

   for(unsigned c = 0;c<272;c++)
   {
      pcem_key[c] = 0;
   }


   switch (keycode)
   {
      case RETROK_F8:
         pcem_key[0x42] = latch;
         break;
      case RETROK_RETURN:
         pcem_key[0x1c] = latch;
         break;
      case RETROK_a:
         pcem_key[0x1e] = latch;
         break;
      case 0:
         break;
   }
}

static int midi_cmd_pos, midi_len;
static uint8_t midi_command[3];
static int midi_lengths[8] = {3, 3, 3, 3, 2, 2, 3, 0};

void midi_init(void)
{
   /* TODO ? */
}

void midi_close(void)
{
   /* TODO ? */
}

void midi_write(uint8_t val)
{
        if (val & 0x80)
        {
                midi_cmd_pos = 0;
                midi_len = midi_lengths[(val >> 4) & 7];
                midi_command[0] = midi_command[1] = midi_command[2] = 0;
        }

        if (midi_len && midi_cmd_pos < 3)
        {
                midi_command[midi_cmd_pos] = val;

                midi_cmd_pos++;

#if 0
#ifdef USE_ALLEGRO_MIDI
                if (midi_cmd_pos == midi_len)
                        midi_out(midi_command, midi_len);
#endif
#endif
        }
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
   initpc(0, NULL);

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

