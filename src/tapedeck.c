/* Small, CRT-readable libretro tape deck. The tape transport remains Fuse's
   own tape engine; this file only supplies UI and input. */
#include <tapedeck.h>
#include <tapedeck_font.h>
#include <tapedeck_logo.h>
#include <tape.h>
#include <stdio.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_MAX_DIMENSIONS 8192
#include <stb_image.h>

#define PANEL_W 144
#define PANEL_H 224
#define RGB565(r,g,b) (((r) << 11) | ((g) << 5) | (b))

static int open_deck;
static int slide;
static int selection;
static int old_f9, old_r3, old_up, old_down, old_left, old_right;
static int old_confirm, old_cancel;
static unsigned reel_frame;
static unsigned char *artwork;
static int artwork_w, artwork_h;

static const char *const button_names[6] = {
   "PLAY", "STOP", "REW", "F.FWD", "PREV", "NEXT"
};

static void count_block(libspectrum_tape_block *block, void *user_data)
{
   (void)block;
   ++*(int *)user_data;
}

void tapedeck_reset(void)
{
   stbi_image_free(artwork);
   artwork = NULL;
   artwork_w = artwork_h = 0;
   open_deck = slide = selection = 0;
   old_f9 = old_r3 = old_up = old_down = old_left = old_right = 0;
   old_confirm = old_cancel = 0;
   reel_frame = 0;
}

static int load_artwork_in(const char *directory, const char *stem)
{
   static const char *const extensions[] = { ".png", ".jpg", ".jpeg", ".bmp" };
   char path[2048];
   unsigned i;
   if (!directory || !*directory) return 0;
   for (i=0; i<sizeof(extensions)/sizeof(extensions[0]); ++i) {
      int w,h,channels;
      unsigned char *pixels;
      int n=snprintf(path,sizeof(path),"%s/%s%s",directory,stem,extensions[i]);
      if (n<0 || (size_t)n>=sizeof(path)) continue;
      if (!stbi_info(path,&w,&h,&channels) || w<1 || h<1 ||
          (double)w*h>16000000.0) continue;
      pixels=stbi_load(path,&w,&h,&channels,4);
      if (!pixels) continue;
      artwork=pixels;
      artwork_w=w;
      artwork_h=h;
      return 1;
   }
   return 0;
}

void tapedeck_load_artwork(const char *tape_path, const char *system_dir)
{
   const char *name, *slash, *backslash, *dot;
   char stem[512], directory[2048];
   size_t length, parent_length;
   stbi_image_free(artwork);
   artwork=NULL;
   artwork_w=artwork_h=0;
   if (!tape_path || !*tape_path) return;
   slash=strrchr(tape_path,'/');
   backslash=strrchr(tape_path,'\\');
   name=(slash && (!backslash || slash>backslash)) ? slash+1 :
        (backslash ? backslash+1 : tape_path);
   dot=strrchr(name,'.');
   length=dot ? (size_t)(dot-name) : strlen(name);
   if (!length || length>=sizeof(stem)) return;
   memcpy(stem,name,length);
   stem[length]=0;

   /* RetroArch's central system folder is the portable default. Also
      recognize the standalone Fuse layout beside a game's tape folder. */
   if (system_dir) {
      int n=snprintf(directory,sizeof(directory),"%s/fuse/images/tape",system_dir);
      if (n>0 && (size_t)n<sizeof(directory) && load_artwork_in(directory,stem)) return;
   }
   parent_length=(size_t)(name-tape_path);
   if (parent_length>=sizeof(directory)-13) return;
   if (parent_length) memcpy(directory,tape_path,parent_length);
   else { directory[0]='.'; directory[1]='/'; parent_length=2; }
   directory[parent_length]=0;
   snprintf(directory+parent_length,sizeof(directory)-parent_length,"images/tape");
   load_artwork_in(directory,stem);
}

int tapedeck_visible(void)
{
   return open_deck || slide > 0;
}

static void transport(int action)
{
   int block;
   if (!tape_present()) return;
   switch (action)
   {
      case 0: tape_do_play(0); break;
      case 1: tape_stop(); break;
      case 2: tape_stop(); tape_rewind(); break;
      case 3: /* Skip several blocks while stopped. */
      case 5:
         tape_stop();
         block = tape_get_current_block();
         if (block >= 0) {
            int total = 0, target = block + (action == 3 ? 5 : 1);
            tape_foreach(count_block, &total);
            if (target < total) tape_select_block((size_t)target);
         }
         break;
      case 4:
         tape_stop();
         block = tape_get_current_block();
         if (block > 0) tape_select_block((size_t)block - 1);
         break;
   }
}

int tapedeck_poll(retro_input_state_t input)
{
   int f9 = input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F9) != 0;
   int r3 = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3) != 0;
   int up, down, left, right, confirm, cancel, enter, escape;
   int owned;

   if ((f9 && !old_f9) || (r3 && !old_r3))
      open_deck = !open_deck;
   old_f9 = f9;
   old_r3 = r3;
   owned = open_deck;

   if (slide < (open_deck ? PANEL_W : 0)) slide += 12;
   if (slide > (open_deck ? PANEL_W : 0)) slide -= 12;
   if (slide < 0) slide = 0;
   if (slide > PANEL_W) slide = PANEL_W;

   if (!open_deck) {
      old_up = old_down = old_left = old_right = 0;
      old_confirm = old_cancel = 0;
      return owned;
   }

   up = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ||
        input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP);
   down = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ||
          input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN);
   left = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ||
          input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT);
   right = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ||
           input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT);
   enter = input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RETURN);
   escape = input(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ESCAPE);
   confirm = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) || enter;
   cancel = input(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) || escape;

   if (up && !old_up) selection = (selection + 4) % 6;
   if (down && !old_down) selection = (selection + 2) % 6;
   if ((left && !old_left) || (right && !old_right)) selection ^= 1;
   if (confirm && !old_confirm) transport(selection);
   if (cancel && !old_cancel) open_deck = 0;
   old_up = up; old_down = down; old_left = left; old_right = right;
   old_confirm = confirm; old_cancel = cancel;
   return owned;
}

static void pixel(uint16_t *dst, unsigned stride, unsigned w, unsigned h,
                  int x, int y, uint16_t color)
{
   if (x >= 0 && y >= 0 && (unsigned)x < w && (unsigned)y < h)
      dst[y * stride + x] = color;
}

void tapedeck_draw(uint16_t *dst, unsigned stride, unsigned width,
                   unsigned height)
{
   int scale = width >= 500 ? 2 : 1;
   int w = (int)width / scale, h = (int)height / scale;
   int x = w - slide, y = (h - PANEL_H) / 2;
   int i, total = 0;
   char status[24];
   const uint16_t shell = RGB565(7, 14, 7);
   const uint16_t metal = RGB565(13, 26, 13);
   const uint16_t cream = RGB565(26, 52, 25);
   const uint16_t white = RGB565(30, 60, 30);
   const uint16_t amber = RGB565(31, 42, 8);
   const uint16_t cyan = RGB565(7, 48, 26);
   if (slide <= 0) return;

   /* Draw at one Spectrum pixel per source pixel, doubling only on Timex. */
#define RECT(px,py,pw,ph,c) do { int sy,sx; for (sy=0;sy<(ph)*scale;++sy) for (sx=0;sx<(pw)*scale;++sx) pixel(dst,stride,width,height,((px)*scale)+sx,((py)*scale)+sy,c); } while(0)
#define DECK_TEXT(px,py,str,c) do { int p; const char *t=(str); for(p=0;t[p];++p) { int row,col; unsigned char ch=(unsigned char)t[p]; if(ch<32||ch>127)ch='?'; for(row=0;row<8;++row)for(col=0;col<8;++col)if(tapedeck_font[ch-32][row]&(0x80>>col))RECT((px)+p*8+col,(py)+row,1,1,c); } } while(0)

   RECT(x,y,PANEL_W,PANEL_H,shell);
   RECT(x+2,y+2,PANEL_W-4,24,metal);
   for (i=0;i<13;++i) {
      int bx,bit;
      for (bx=0;bx<11;++bx)
         for (bit=0;bit<8;++bit)
            if (tapedeck_logo[i][bx] & (0x80 >> bit))
               RECT(x+7+bx*8+bit,y+7+i,1,1,RGB565(31,20,18));
   }
   DECK_TEXT(x+102,y+9,"TAPE",white);
   RECT(x+6,y+32,PANEL_W-12,70,cream);
   RECT(x+10,y+36,PANEL_W-20,62,metal);
   RECT(x+16,y+42,112,4,RGB565(31,10,6));
   RECT(x+16,y+46,112,3,amber);
   RECT(x+16,y+49,112,3,cyan);
   for (i=0;i<2;++i) {
      int cx = x + (i ? 107 : 37), cy = y + 74, dx, dy;
      for (dy=-17;dy<=17;++dy)
         for (dx=-17;dx<=17;++dx)
            if (dx*dx+dy*dy <= 17*17)
               RECT(cx+dx,cy+dy,1,1,RGB565(10,20,10));
      for (dy=-10;dy<=10;++dy)
         for (dx=-10;dx<=10;++dx)
            if (dx*dx+dy*dy <= 10*10)
               RECT(cx+dx,cy+dy,1,1,cream);
      RECT(cx-3,cy-3,7,7,metal);
      if (tape_is_playing()) {
         int spoke = (reel_frame/5+i*2)%4;
         RECT(cx + (spoke==0?9:spoke==1?-10:0),cy + (spoke==2?9:spoke==3?-10:0),2,2,white);
      }
   }
   if (artwork) {
      int py,px;
      for (py=0;py<62;++py) for (px=0;px<124;++px) {
         const unsigned char *rgba=artwork+4*((py*artwork_h/62)*artwork_w+px*artwork_w/124);
         if (rgba[3]>=128)
            RECT(x+10+px,y+36+py,1,1,RGB565(rgba[0]>>3,rgba[1]>>2,rgba[2]>>3));
      }
      /* A small moving spindle mark remains visible over real cassette art. */
      if (tape_is_playing()) for (i=0;i<2;++i) {
         int cx=x+(i?107:37), cy=y+74, spoke=(reel_frame/5+i*2)%4;
         RECT(cx+(spoke==0?7:spoke==1?-8:0),
              cy+(spoke==2?7:spoke==3?-8:0),2,2,white);
      }
   }
   if (tape_is_playing()) ++reel_frame;
   if (!tape_present()) snprintf(status,sizeof(status),"NO TAPE");
   else {
      tape_foreach(count_block, &total);
      snprintf(status,sizeof(status),"%s %02d/%02d",tape_is_playing()?"PLAY":"STOP",tape_get_current_block()+1,total);
   }
   DECK_TEXT(x+7,y+108,status,cyan);

   for (i=0;i<6;++i) {
      int bx=x+7+(i%2)*66, by=y+126+(i/2)*26;
      RECT(bx,by,62,23,i==selection?amber:metal);
      RECT(bx+2,by+2,58,19,i==selection?metal:shell);
      DECK_TEXT(bx+5,by+8,button_names[i],i==selection?amber:white);
   }
   DECK_TEXT(x+7,y+209,"F9/R3 HIDE",white);
#undef RECT
#undef DECK_TEXT
}
