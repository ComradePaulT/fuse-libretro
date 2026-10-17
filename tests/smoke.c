/* Minimal libretro frontend for Windows build smoke testing. */
#include <windows.h>
#include <libretro.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned frames, audio_frames, last_w, last_h;
static uint16_t *last_frame;
static unsigned key_f9, key_a, key_down, key_r3;
static int test_auto_load;
static int test_auto_play;
static int test_show_deck;
static int test_model_switch, model_128, variables_changed;
static int test_m3u_tape;
static struct retro_disk_control_ext_callback disk_control;

static bool env(unsigned cmd, void *data)
{
   switch (cmd) {
      case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: return *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
      case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: *(unsigned *)data = 2; return true;
      case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
         *(bool *)data = variables_changed != 0;
         variables_changed = 0;
         return true;
      case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: *(const char **)data = "."; return true;
      case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION:
         *(unsigned *)data = 1; return true;
      case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
         disk_control = *(const struct retro_disk_control_ext_callback *)data;
         return true;
      case RETRO_ENVIRONMENT_GET_VARIABLE:
      {
         struct retro_variable *var=(struct retro_variable *)data;
         if(!test_auto_load && !test_model_switch) return false;
         if(!strcmp(var->key,"fuse_machine")) var->value=test_model_switch ?
            (model_128 ? "Spectrum 128K" : "Spectrum 48K") : "Spectrum 128K";
         else if(!strcmp(var->key,"fuse_auto_load")) var->value=test_model_switch ? "disabled" : "enabled";
         else if(!strcmp(var->key,"fuse_fast_load")) var->value="disabled";
         else if(!strcmp(var->key,"fuse_load_sound")) var->value="enabled";
         else return false;
         return true;
      }
      default: return false;
   }
}

static void video(const void *data, unsigned w, unsigned h, size_t pitch)
{
   unsigned y;
   ++frames;
   if (!data) return;
   free(last_frame);
   last_frame = (uint16_t *)malloc(w*h*sizeof(uint16_t));
   if (!last_frame) exit(2);
   for (y=0;y<h;++y)
      memcpy(last_frame+y*w,(const uint8_t *)data+y*pitch,w*sizeof(uint16_t));
   last_w=w; last_h=h;
}

static void audio(int16_t left, int16_t right) { (void)left; (void)right; ++audio_frames; }
static size_t audio_batch(const int16_t *data,size_t count) { (void)data; audio_frames += (unsigned)count; return count; }
static void poll(void) {}
static int16_t input(unsigned port,unsigned device,unsigned index,unsigned id)
{
   (void)index;
   if (port) return 0;
   if (device==RETRO_DEVICE_KEYBOARD && id==RETROK_F9) return key_f9;
   if (device==RETRO_DEVICE_JOYPAD && id==RETRO_DEVICE_ID_JOYPAD_A) return key_a;
   if (device==RETRO_DEVICE_JOYPAD && id==RETRO_DEVICE_ID_JOYPAD_DOWN) return key_down;
   if (device==RETRO_DEVICE_JOYPAD && id==RETRO_DEVICE_ID_JOYPAD_R3) return key_r3;
   return 0;
}

static void save_bmp(const char *path)
{
   FILE *out=fopen(path,"wb");
   unsigned row, col, stride=(last_w*3+3)&~3u, size=54+stride*last_h;
   uint8_t header[54]={0};
   if (!out || !last_frame) exit(3);
   header[0]='B'; header[1]='M';
   memcpy(header+2,&size,4); header[10]=54; header[14]=40;
   memcpy(header+18,&last_w,4); memcpy(header+22,&last_h,4);
   header[26]=1; header[28]=24;
   fwrite(header,1,54,out);
   for(row=last_h;row>0;--row) {
      for(col=0;col<last_w;++col) {
         uint16_t p=last_frame[(row-1)*last_w+col];
         uint8_t rgb[3]={(uint8_t)((p&31)<<3),(uint8_t)(((p>>5)&63)<<2),(uint8_t)(((p>>11)&31)<<3)};
         fwrite(rgb,1,3,out);
      }
      for(col=last_w*3;col<stride;++col) fputc(0,out);
   }
   fclose(out);
}

int main(int argc,char **argv)
{
   HMODULE lib;
   void (*set_environment)(retro_environment_t), (*set_video_refresh)(retro_video_refresh_t);
   void (*set_audio_sample)(retro_audio_sample_t), (*set_audio_sample_batch)(retro_audio_sample_batch_t);
   void (*set_input_poll)(retro_input_poll_t), (*set_input_state)(retro_input_state_t);
   void (*init)(void), (*run)(void), (*unload_game)(void), (*deinit)(void);
   void (*get_system_av_info)(struct retro_system_av_info *);
   bool (*load_game)(const struct retro_game_info *);
   FILE *file; long length; void *bytes; struct retro_game_info game={0};
   int i;
   if(argc<4) return 2;
   test_auto_load=argc>4 && !strcmp(argv[4],"autoload");
   test_auto_play=argc>4 && !strcmp(argv[4],"autoload_play");
   test_show_deck=argc>4 && !strcmp(argv[4],"autoload_play_show");
   test_model_switch=argc>4 && !strcmp(argv[4],"switch_model");
   test_m3u_tape=argc>4 && !strcmp(argv[4],"m3u_tape");
   if (test_show_deck) test_auto_play=1;
   if (test_auto_play) test_auto_load=1;
   lib=LoadLibraryA(argv[1]); if(!lib) {printf("LoadLibrary error %lu\n",GetLastError());return 2;}
#define GET(name,type) name=(type)GetProcAddress(lib,"retro_" #name); if(!name) return 3
   GET(set_environment,void(*)(retro_environment_t));
   GET(set_video_refresh,void(*)(retro_video_refresh_t));
   GET(set_audio_sample,void(*)(retro_audio_sample_t));
   GET(set_audio_sample_batch,void(*)(retro_audio_sample_batch_t));
   GET(set_input_poll,void(*)(retro_input_poll_t));
   GET(set_input_state,void(*)(retro_input_state_t));
   GET(init,void(*)(void)); GET(run,void(*)(void));
   GET(get_system_av_info,void(*)(struct retro_system_av_info *));
   GET(load_game,bool(*)(const struct retro_game_info *));
   GET(unload_game,void(*)(void)); GET(deinit,void(*)(void));
#undef GET
   file=fopen(argv[2],"rb"); if(!file)return 4;
   fseek(file,0,SEEK_END);length=ftell(file);rewind(file);
   bytes=malloc(length);if(!bytes||fread(bytes,1,length,file)!=(size_t)length)return 4;
   fclose(file);
   game.path=argv[2];game.data=bytes;game.size=length;
   set_environment(env);set_video_refresh(video);set_audio_sample(audio);set_audio_sample_batch(audio_batch);
   set_input_poll(poll);set_input_state(input);init();
   if(!load_game(&game)) {puts("retro_load_game failed");return 5;}
   if (test_m3u_tape) {
      char path[1024], label[128];
      if (!disk_control.get_num_images || disk_control.get_num_images()!=2 ||
          !disk_control.get_image_path(0,path,sizeof(path)) ||
          !strstr(path,"side-a.tap") ||
          !disk_control.get_image_label(1,label,sizeof(label)) ||
          !strstr(label,"side-b.tap")) {
         puts("M3U tape list incorrect"); return 8;
      }
      if (!disk_control.set_eject_state(true) ||
          !disk_control.get_eject_state() ||
          !disk_control.set_image_index(1) ||
          !disk_control.set_eject_state(false) ||
          disk_control.get_eject_state() ||
          disk_control.get_image_index()!=1) {
         puts("M3U tape side change failed"); return 9;
      }
      puts("M3U tape sides switched");
   }
   {
      struct retro_system_av_info av={0};
      get_system_av_info(&av);
      printf("reported_geometry=%ux%u fps=%.6f sample_rate=%.0f\n",
             av.geometry.base_width,av.geometry.base_height,
             av.timing.fps,av.timing.sample_rate);
   }
   {
      int total=argc>5 ? atoi(argv[5]) : 100;
      unsigned min_audio=~0u, max_audio=0, silent_runs=0;
      for(i=0;i<total;++i) {
      unsigned before_audio=audio_frames;
      if (test_model_switch && i==20) { model_128=1; variables_changed=1; }
      key_f9=(i==5 && !test_auto_load) ||
             (i==300 && test_auto_play) ||
             (i==400 && test_auto_play && !test_show_deck);
      key_down=(i==25 && argc==4);
      key_a=(i==40 && !test_auto_load) ||
            (i==335 && test_auto_play); key_r3=0;
      run();
      {
         unsigned produced=audio_frames-before_audio;
         if (produced<min_audio) min_audio=produced;
         if (produced>max_audio) max_audio=produced;
         if (!produced) ++silent_runs;
      }
      }
      printf("audio_per_run_min=%u max=%u silent_runs=%u\n",
             min_audio,max_audio,silent_runs);
   }
   save_bmp(argv[3]);
   if (test_model_switch) {
      struct retro_system_av_info av={0};
      get_system_av_info(&av);
      printf("after_model_switch_fps=%.6f\n", av.timing.fps);
      if (av.timing.fps < 50.0 || av.timing.fps > 50.03) return 7;
   }
   printf("frames=%u audio_samples=%u image=%ux%u\n",frames,audio_frames,last_w,last_h);
   unload_game();deinit();free(bytes);free(last_frame);FreeLibrary(lib);
   return frames>0 && audio_frames>0 && last_w>=256 && last_h>=192 ? 0:6;
}
