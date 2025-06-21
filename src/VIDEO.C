#include <stddef.h>
#include <malloc.h>
#include <stdint.h>
#include <time.h>

#include "EDITION.H"
#include "SPIELDAT.H"
#include "SYSTEMIO.H"
#include "MPLANET.H"
#include "BMCACHE.H"
#include "MPBITMAP.H"
#include "GUI.H"

#define ANIM_MAX 32

#define ANIM_POSX  320
#define ANIM_POSY  250
#define ANIM_FPS 16

#pragma pack(push,1)
struct ANIM_BLOCK
{
 uint8_t image;
 uint8_t delay;
 uint8_t flags;
 int16_t x;   /*ungerade Adresse !*/
 int16_t y;
 uint8_t res1[3];
 uint8_t loopflag;
 uint8_t loopcount;
 uint8_t loopjump;
 uint8_t res2;
 uint16_t trigger_mask;
};

struct GRAF_BLOCK
{
 uint8_t frames;
 uint8_t res1;
 uint8_t trans;
 uint8_t res2;      
 char name[64];
};

#pragma pack(pop)

static unsigned int video_x=0,video_y=0,anim_num=0,graf_num=0,bg_bitmap=0;;
static struct ANIM_BLOCK *anim_ptr[ANIM_MAX];
static unsigned int anim_sizex[ANIM_MAX],anim_sizey[ANIM_MAX],
 anim_len[ANIM_MAX],anim_bitmap[ANIM_MAX],anim_loopctr[ANIM_MAX],
 anim_frame[ANIM_MAX],anim_timeout[ANIM_MAX];
static int anim_posx[ANIM_MAX],anim_posy[ANIM_MAX];


static unsigned int path_to_number(const char *ptr)
{
 int a,b;
 char puffer[10];
 for(a=0;ptr[a]&&ptr[a]!='.';a++)
 puffer[9]=0;a--;
 b=8;
 while(b&&a&&ptr[a]!='\\')
  puffer[b--]=ptr[a--];                        
  printf("%s:",puffer+b+1);
 a=bitmap_name_to_number(puffer+b+1);       
 printf("%d ",a);
 return a;
}

void clip_coord(int *s,int *ds,int *ssrc,int min,int max)
{
 int a;
 a=*s-min;
 if(a<0)
 {    
  *s-=a;
  *ds+=a;
  *ssrc-=a;
 }
 a=max-(*s+*ssrc);
 if(a<0)
 {
  *ds+=a;       
 }   
}

int play_video(void *dat)
{
 int a,b,c;
 uint8_t *bptr=NULL;
 struct GRAF_BLOCK *grafik=NULL; 
 int x0,y0,dx,dy,xsrc,ysrc;
 uint16_t triggers=0;
 time_t clk_old=0;
 int stop_ctr=0;
  
 bptr=dat;
 if(bptr[0]!='A'||bptr[1]!='N') return -1;
 graf_num=bptr[4];
 anim_num=bptr[5];
 if(anim_num>=ANIM_MAX) anim_num=ANIM_MAX-1;
 video_x=*((uint16_t*)(&(bptr[0x59])));
 video_y=*((uint16_t*)(&(bptr[0x5B])));
 bptr+=0x64;
 bg_bitmap=path_to_number(bptr);
 bptr+=0x8A;
 grafik=(void*) bptr;
 bptr+=68*graf_num; 
 for(a=0;a<anim_num;a++)
 {
  anim_bitmap[a]=path_to_number(grafik[*(bptr++)].name);
  /*grafik[*bptr].frames*/
  anim_sizex[a]=bitmap_get_size(&(anim_sizey[a]),anim_bitmap[a]);
  while(*bptr) bptr++;
  bptr+=6; /*ubekannt*/
  while(!*bptr) bptr++;
  bptr++; /*1*/
  anim_len[a]=*(bptr++);
  bptr+=19;
  anim_ptr[a]=(void*)bptr;
  anim_frame[a]=0;
  anim_posx[a]=anim_ptr[a]->x;
  anim_posy[a]=anim_ptr[a]->y;
  anim_timeout[a]=anim_ptr[a]->delay;
  bptr+=16*anim_len[a];
  triggers|=anim_ptr[a][anim_frame[a]].trigger_mask;
  anim_loopctr[a]=0;
 }
 
 draw_frame(ANIM_POSX-video_x/2-2,ANIM_POSY-video_y/2-2,video_x+4,video_y+4,
 2,FARBE_DUNKEL,FARBE_HELL);
   
 while(stop_ctr<5)
 {
  b=0;
  for(a=0;a<anim_num;a++)
  {
   if(anim_frame[a]+1<anim_len[a]) b++;
  }
  if(!b) stop_ctr++;
  x0=ANIM_POSX-video_x/2;
  y0=ANIM_POSY-video_y/2;
  dx=video_x;
  dy=video_y;
  xsrc=ysrc=0;
  
  draw_bitmaps(1,get_bitmap_slot(bg_bitmap),&x0,&y0,&dx,&dy,&xsrc,&ysrc);    
  for(a=0;a<anim_num;a++)
  {
   if(anim_frame[a]<anim_len[a])
   {
    x0=anim_posx[a];
    y0=anim_posy[a];
    x0+=ANIM_POSX-anim_sizex[a]/2;
    y0+=ANIM_POSY-anim_sizey[a]/2;
    dx=anim_sizex[a];
    dy=anim_sizey[a];
    xsrc=ysrc=0;
    b=video_x/2;     
    clip_coord(&x0,&dx,&xsrc,ANIM_POSX-b,ANIM_POSX+b);
    b=video_y/2;     
    clip_coord(&y0,&dy,&ysrc,ANIM_POSY-b,ANIM_POSY+b);
    if(dx>0&&dy>0)
     draw_bitmaps(1,get_bitmap_slot(anim_bitmap[a]+anim_ptr[a][anim_frame[a]].image),&x0,&y0,&dx,&dy,&xsrc,&ysrc);    
   } 
  }
  win_flush();
  for(a=0;a<anim_num;a++)
  {
   if(anim_timeout[a]) anim_timeout[a]--;
   else
   {
    if(
     ((!(anim_ptr[a][anim_frame[a]].flags&0x08))||
     (triggers&(1<<a)))&&
      (!(anim_ptr[a][anim_frame[a]].flags&0x10))
      )
    {
     b=anim_frame[a];
     if(anim_ptr[a][b].loopflag)
     {
      if(++(anim_loopctr[a])<anim_ptr[a][b].loopcount)
       b=anim_frame[a]=anim_ptr[a][b].loopjump;
      else 
      {
       b=++(anim_frame[a]);
       anim_loopctr[a]=0;
      } 
     }
     else b=++(anim_frame[a]);
     if(b>=anim_len[a])
     {
      b=anim_frame[a]=anim_len[a]-1;
      anim_timeout[a]=10;
      if(!(anim_ptr[a][b].flags&0x20))
      {
       anim_frame[a]++;
      }
     }
     else
     {
      anim_timeout[a]=anim_ptr[a][b].delay;           
      if(anim_ptr[a][b].flags&0x80)
       anim_posx[a]=0;
      if(anim_ptr[a][b].flags&0x40)
       anim_posy[a]=0;
      anim_posx[a]+=anim_ptr[a][b].x;
      anim_posy[a]+=anim_ptr[a][b].y;
      triggers&=~(1<<a);
      triggers|=anim_ptr[a][b].trigger_mask;
     }
/*     get_bitmap_slot(anim_bitmap[a]+anim_ptr[a][anim_frame[a]].image);*/
    }
   }
  }   
  if(game_input_ready())
  {
   if(game_input_get()==13) break;
  }
  clk_old=(clk_old+CLK_TCK/ANIM_FPS+1)-clock();
  if(clk_old<1) clk_old=1;
  if(clk_old>CLK_TCK/ANIM_FPS+1) clk_old=CLK_TCK/ANIM_FPS+1;
  clk_wait(clk_old);
  clk_old=clock();
 }
 return 0;
}
