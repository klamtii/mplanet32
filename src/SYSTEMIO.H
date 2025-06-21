#ifndef _SYSTEMIO_H_
#define _SYSTEMIO_H_
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#define MAX_BITMAPS        32
#define DRAWWIN_X          640
#define DRAWWIN_Y          480
#define MAX_FONT           4
struct DIB_HEADER 
{
  uint32_t size;
  int32_t width;
  int32_t  height;
  uint16_t  planes;
  uint16_t  bitcount;
  uint32_t compression;
  uint32_t sizeimage;
  int32_t  xpelspermeter;
  int32_t  ypelspermeter;
  uint32_t clrused;
  uint32_t clrimportant;
};
int (* CALLCONV set_clickable_func(int (*CALLCONV func)(unsigned int,unsigned int)))(unsigned int,unsigned int);
int win_init(int on);
int win_flush(void);
int win_lock(void);
volatile int win_input_ready(void);
volatile uint32_t win_input_get(void);
int free_bitmap(int slot);
int alloc_bitmap(int slot, void *hdr,void *pal,void *dat,
 void *hdrm,void *palm,void *datm,unsigned int tilesize,int noscale);
int draw_bitmaps_x(int num,int slot,int *at_xdst,int *at_ydst,int *at_dx,int *at_dy,int *at_xsrc,int *at_ysrc,int operation);
int draw_bitmaps(unsigned int num,unsigned int slot,int *at_xdst,int *at_ydst,int *at_dx,int *at_dy,int *at_xsrc,int *at_ysrc);
int free_font(unsigned int slot);
int alloc_font(int n,const char *name,unsigned int x,unsigned int y,int bold);
int write_text(int n,int x,int y,int dx,int dy,const char *str,uint32_t f,int cent);
#ifdef FEATURE_UNICODE
unsigned int set_codepage(unsigned int cp);
#endif /*FEATURE_UNICODE*/
int fill_rectangles(unsigned int n,int *x1,int *y1,int *x2,int *y2,uint32_t *f);
void minimize_window(void);
int window_minimized(void);

#define MAX_SONGS 16
int unregister_song(int n);
int register_song(int n,void *dat,int len);
int play_song(int n); /* -1 zum Aufhören*/
void set_midi_volume(unsigned short v); /*0-100*/
unsigned short get_midi_volume(void); /*0-100*/
void set_midi_onoff(int p);
int get_midi_onoff(void);

int wave_is_playing(void);
int play_wave(void *dat,unsigned int len);
void set_wave_volume(unsigned short v); /*0-100*/
unsigned short get_wave_volume(void); /*0-100*/
void set_wave_onoff(int p);
int get_wave_onoff(void);

void yield_thread(void);
void clk_wait(clock_t t);

void *load_file_stdio(uint32_t *len,const char *name);
void *load_file_lzexpand(uint32_t *len,/*const*/ char *name);

const char * get_filename(int save);
char * find_filenames(const char *srch);
const char *iso_language_code(void);

#endif /*_SYSTEMIO_H_*/
