#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <mem.h>
#include <malloc.h>

#include "EDITION.H"
#include "SYSTEMIO.H"
#include "SPIELDAT.H"
#include "BMCACHE.H"
#include "RANDOM.H"
#include "MPBITMAP.H"
#include "MPLANET.H"
#include "NSTRING.H"

struct WELT welt;
uint8_t welt_tilemap_alt[28*28];
uint8_t welt_einheiten_alt[28*28];
uint8_t welt_tilemap[28*28];
uint8_t welt_einheiten[28*28];
uint8_t welt_relief[28*28];
/*uint16_t welt_bewertung[28*28];*/

struct SPIELER spieler[6];

/*Punkte nach Anzahl der Einheiten zuweisen*/
void welt_punkte_verteilen(void)
{
 int a,b,c,d;
 int letzte_einheiten=-1;
 int letzte_punkte=0;
 int punkte_diff;
 if(welt.runde)
 {
  if(welt.runde_max<10) punkte_diff=10;
  else if(welt.runde_max<20) punkte_diff=7;
  else punkte_diff=5;
 }
 else /*Erste Runde*/
 {
  punkte_diff=0;
  letzte_punkte=100;    
 }
 /*Punkte zuteilen*/
 /*Besiegte Spieler gelten als schlecht -> andere Spieler bekommen viele Punkte*/
 /*Fehlende Spieler gelten als gut -> andere Spieler bekommen wenig Punkte*/
 for(a=0;a<6;a++)
 {
  c=-1;d=1000;
  for(b=0;b<6;b++)
  {
   if(spieler[b].evol.einheiten<d&&
    spieler[b].evol.einheiten>letzte_einheiten&&
    spieler[b].evol.typ!=SPIELERTYP_AUS) 
   {c=a;d=spieler[b].evol.einheiten;}
  }
  if(c<0) break;
  letzte_einheiten=d;
  c=letzte_punkte+punkte_diff;
  for(b=0;b<6;b++)
  {
   if(spieler[b].evol.einheiten==letzte_einheiten)
   {
    spieler[b].evol.punkte_runde=c;
    letzte_punkte+=punkte_diff;
   }
  }
 }
 /*Punkte berechnen*/ 
 for(a=0;a<6;a++)         
 {
  c=spieler[a].evol.punkte_runde;
  for(b=0;b<EVO_NUM;b++)
   c+=spieler[a].evol.eigenschaften[b];   
  spieler[a].evol.punkte_gesamt=c;
  spieler[a].evol.rang=a; /*vorläufig*/
 }
 /*Ränge sortieren*/
 for(a=0;a<6;a++)
 {
  for(b=0;b<6;b++)
  {
   c=spieler[a].evol.rang-spieler[b].evol.rang;
   d=spieler[a].evol.punkte_gesamt-spieler[b].evol.punkte_gesamt;
   if(!d)
    d=spieler[a].evol.punkte_runde-spieler[b].evol.punkte_runde;
   if(!d)
    d=-(spieler[a].sammel.sterblichkeit-spieler[b].sammel.sterblichkeit);
   if(c*d>0) /*delta-Rang * delta-Punkte muss negativ sein*/
   {
    c=spieler[a].evol.rang; d=spieler[b].evol.rang;
    spieler[a].evol.rang=d; spieler[b].evol.rang=c;
   }
  }
 }   
}

/*Weltkarte anzeigen*/
void welt_bildaufbau(int alles)
{
 int a,b;
 int n=0;
 int xdst[10],ydst[10],delta[10],xsrc[10],ysrc[10],xmsrc[10];
 /*Tilemap*/
 for(a=0;a<28*28;a++)
 {
  b=welt_tilemap[a];
  if(alles||b!=welt_tilemap_alt[a]||welt_einheiten_alt[a]!=welt_einheiten[a])
  {
   xdst[n]=(a%28)*16+SPIELFELD_X+5;
   ydst[n]=(a/28)*16+SPIELFELD_Y+5;
   delta[n]=16;
   xsrc[n]=(b%40)*16;
   ysrc[n++]=(b/40)*16;
   welt_tilemap_alt[a]=b;
   welt_einheiten_alt[a]=0;
  }
  if(n>=10)
  {
   draw_bitmaps(n,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,delta,delta,xsrc,ysrc);
   n=0;
  }
 }
 if(n)
 {
  draw_bitmaps(n,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,delta,delta,xsrc,ysrc);
  n=0;
 }
 /*Sprites*/
 for(a=0;a<28*28;a++)
 {
  b=welt_einheiten[a];
  if(b&&(alles||b!=welt_einheiten_alt[a]))
  {
   xdst[n]=(a%28)*16+SPIELFELD_X+5;
   ydst[n]=(a/28)*16+SPIELFELD_Y+5;
   xsrc[n]=240+b*32;
   xmsrc[n]=256+b*32;
   delta[n]=ysrc[n]=16;
   n++;
   welt_einheiten_alt[a]=b;
  }
  if(n>=10)
  {
   draw_bitmaps_x(n,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,delta,delta,xmsrc,ysrc,-1);
   draw_bitmaps_x(n,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,delta,delta,xsrc,ysrc,1);
   n=0;
  }
 }
 if(n)
 {
  draw_bitmaps_x(n,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,delta,delta,xmsrc,ysrc,-1);
  draw_bitmaps_x(n,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,delta,delta,xsrc,ysrc,1);
  n=0;
 }
}

/*Statistik über Aufstellung in Spielerstruktur eintragen*/
void welt_statistik(void)
{ 
 auto const uint8_t *tilemap_ptr=welt_tilemap,*sprite_ptr=welt_einheiten;
 unsigned int a,b,c,d;
 const static short pos[8]={-29,-28,-27,-1,1,27,28,29};
 for(a=0;a<6;a++)
 {
  memset(&(spieler[a].stell),0,sizeof(struct STELLUNG));
  spieler[a].evol.einheiten=0;
 }
 welt.raumschiff=0;
 for(a=0;a<28;a++)
  for(b=0;b<28;b++)
  {
   if(*sprite_ptr)
   {
    c=(*sprite_ptr)-1;
    spieler[c].evol.einheiten++;
 /*   printf("Einheiten(%d):%d\n",c,spieler[c].evol.einheiten);*/
    spieler[c].stell.pflanzen[(*tilemap_ptr)-48]++;
    for(d=0;d<8;d++)
    {
     if(sprite_ptr[pos[d]])
      spieler[c].stell.nachbarn[sprite_ptr[pos[d]]-1]++;
     if(tilemap_ptr[pos[d]]==54)
      spieler[c].stell.gebirge++;
     else if(tilemap_ptr[pos[d]]<48)
      spieler[c].stell.wasser++;
     else spieler[c].stell.leer++;
    }    
   }
   if(*tilemap_ptr==56)
    welt.raumschiff++;
   tilemap_ptr++;
   sprite_ptr++;
  }  
}

/*Mauseingabe in Koordinaten umrechnen*/
int welt_klick(uint32_t m)
{
 int r=-1;
 unsigned int x,y;
 x=MOUSE_X(m);
 y=MOUSE_Y(m);
 if((x<(5+SPIELFELD_X))||(y<(5+SPIELFELD_Y))) return -1;
 x-=5+SPIELFELD_X; y-=5+SPIELFELD_Y;
 if(x<16*28&&y<16*28)
  r=(y/16)*28+(x/16);
 return r;
}

/*Anzeige für Ausbreitungsrunde anzeigen*/
void welt_stat_info(int n)
{
 int a,b,c;
 char puffer[16];
 int xdst[6],ydst[6],dx[6],dy[6],xsrc[6],ysrc[6];
 uint32_t f[7]={0xFF0000,0x00FF00,0xFFFF00,0x00FFFF,0xFFFFFF,0x000000,0x0000FF};
 xdst[0]=HUD_X+16; ydst[0]=HUD_Y+12;
 dx[0]=xdst[0]+45-1;dy[0]=ydst[0]+30-1;
 fill_rectangles(1,xdst,ydst,dx,dy,f+5);
 dx[0]=45;dy[0]=30;
 draw_frame(xdst[0],ydst[0],dx[0],dy[0],2,FARBE_DUNKEL,FARBE_HELL);
 sprintf(puffer,"%u",(unsigned int)(welt.runde));
 write_text(FONT_RUNDE,xdst[0],ydst[0],dx[0],dy[0],puffer,0xFFFFFF,1);

 xdst[0]=545; ydst[0]=HUD_Y+8;
 dx[0]=40;    dy[0]=100;
 xsrc[0]=0;   ysrc[0]=0;
 draw_bitmaps_x(1,get_bitmap_slot(BM_WASS_MASKE),xdst,ydst,dx,dy,xsrc,ysrc,-1);
 draw_bitmaps_x(1,get_bitmap_slot(BM_WASSER),xdst,ydst,dx,dy,xsrc,ysrc,1);
 xdst[1]=552; ydst[1]=HUD_Y+8+100-welt.feuchtigkeit;
 dx[1]=576;    dy[1]=HUD_Y+8+94;
 fill_rectangles(1,xdst+1,ydst+1,dx+1,dy+1,f+6);
 draw_bitmaps_x(1,get_bitmap_slot(BM_WASS_FRONT_MASKE),xdst,ydst,dx,dy,xsrc,ysrc,-1);
 draw_bitmaps_x(1,get_bitmap_slot(BM_WASS_FRONT),xdst,ydst,dx,dy,xsrc,ysrc,1);
 xdst[0]=585;
 draw_bitmaps_x(1,get_bitmap_slot(BM_TEMP_MASKE),xdst,ydst,dx,dy,xsrc,ysrc,-1);
 draw_bitmaps_x(1,get_bitmap_slot(BM_TEMPERATUR),xdst,ydst,dx,dy,xsrc,ysrc,1);
 xdst[1]=604;  ydst[1]=HUD_Y+4+100-welt.temperatur;
 dx[1]=605;    dy[1]=HUD_Y+4+90;
 fill_rectangles(1,xdst+1,ydst+1,dx+1,dy+1,f);
 draw_bitmaps_x(1,get_bitmap_slot(BM_TEMP_FRONT),xdst,ydst,dx,dy,xsrc,ysrc,-1);
 
 for(a=0;a<6;a++)
 {
  xdst[a]=HUD_X+5;
  ydst[a]=HUD_Y+125+a*20;
  dy[a]=dx[a]=16;
  xsrc[a]=288+32*a; ysrc[a]=16;
 }   
 draw_bitmaps_x(6,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,xsrc,ysrc,-1);
 b=0;
 for(a=0;a<6;a++)
 {
  xdst[a]=HUD_X+5;
  ydst[a]=HUD_Y+125+a*20;
  dy[a]=dx[a]=16;
  xsrc[a]=272+32*a; ysrc[a]=16;
  if(spieler[a].evol.einheiten>b) b=spieler[a].evol.einheiten;
 }   
 draw_bitmaps_x(6,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,xsrc,ysrc,1);
 if(b<1) b=1;
 c=0;
 for(a=0;a<6;a++)
 {
  xdst[a]=HUD_X+25;
  ydst[a]=HUD_Y+125+a*20+4;
  dx[a]=HUD_X+25+(spieler[a].evol.einheiten*100)/b;
  dy[a]=HUD_Y+125+a*20+7+4;
  if(spieler[a].evol.einheiten>b) b=spieler[a].evol.einheiten;
  if(spieler[a].evol.einheiten<1)
   {xdst[a]=dx[a]=0;dy[a]=ydst[a];}
  else c=a+1;
 }   
 if(c) fill_rectangles(c,xdst,ydst,dx,dy,f);
 b=0;
 if(n>=0&&n<6)
 {
  xdst[0]=HUD_X+5; ydst[0]=HUD_Y+50;
  dy[0]=dx[0]=64;ysrc[0]=0;xsrc[0]=64*n;
  draw_bitmaps(1,get_bitmap_slot(BM_STAND_DAT),xdst,ydst,dx,dy,xsrc,ysrc);

  for(a=0;a<spieler[n].sammel.kinder;a++)
  {
   xdst[b]=HUD_X+5+16*(a%10);
   ydst[b]=HUD_Y+265+20*(a/10);
   dy[b]=dx[b]=16;
   xsrc[b]=288+32*n;
   ysrc[b++]=272+32*n;
   if(b>=6)
   {
    draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,xsrc,dy,-1);  
    draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,ysrc,dy,1);  
    b=0;
   }
  }
  if(b)
  {
   draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,xsrc,dy,-1);  
   draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,ysrc,dy,1);  
   b=0;
  }
  b=0;
  for(a=0;a<spieler[n].sammel.bewegung;a++)
  {
   xdst[b]=HUD_X+5+16*(a%10);
   ydst[b]=HUD_Y+265+75+20*(a/10);
   dy[b]=dx[b]=16;
   xsrc[b]=288+32*n;
   ysrc[b++]=272+32*n;
   if(b>=6)
   {
    draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,xsrc,dy,-1);  
    b=0;
   }
  }
  if(b)
  {
   draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),xdst,ydst,dx,dy,xsrc,dy,-1);  
   b=0;
  }
 }
}
 
/* Bit gesetzt, wenn Nachbar Land ist
   1   2   4
   8   X  16
  32  64 128
*/
static uint8_t kueste[256]=
{
  0,  3, 30, 30,   2,  9, 30, 30,  31, 31, 39, 39,  22, 22, 39, 39,
 29, 23, 38, 38,  29, 23, 38, 38,  41, 41, 45, 45,  41, 41, 45, 45,
  4, 10, 20, 20,   5, 11, 20, 20,  31, 31, 39, 39,  22, 22, 39, 39,
 18, 25, 34, 34,  18, 25, 34, 34,  41, 41, 45, 45,  41, 41, 45, 45,

 28, 16, 40, 40,  17, 24, 40, 40,  36, 36, 42, 42,  32, 32, 42, 42,
 37, 33, 44, 44,  37, 33, 44, 44,  43, 43, 46, 46,  43, 43, 46, 46,
 28, 16, 40, 40,  17, 24, 40, 40,  36, 36, 42, 42,  32, 32, 42, 42,
 37, 33, 44, 44,  37, 33, 44, 44,  43, 43, 46, 46,  43, 43, 46, 46,

  1,  6, 21, 21,   8, 13, 21, 21,  19, 19, 35, 35,  27, 27, 35, 35,
 29, 23, 38, 38,  29, 23, 38, 38,  41, 41, 45, 45,  41, 41, 45, 45,
  7, 12, 26, 26,  14, 15, 26, 26,  19, 19, 35, 35,  27, 27, 35, 35,
 18, 25, 34, 34,  18, 25, 34, 34,  41, 41, 45, 45,  41, 41, 45, 45,

 28, 16, 40, 40,  17, 24, 40, 40,  36, 36, 42, 42,  32, 32, 42, 42,
 37, 33, 44, 44,  37, 33, 44, 44,  43, 43, 46, 46,  43, 43, 46, 46,
 28, 16, 40, 40,  17, 24, 40, 40,  36, 36, 42, 42,  32, 32, 42, 42,
 37, 33, 44, 44,  37, 33, 44, 44,  43, 43, 46, 46,  43, 43, 46, 46,
};
   
static short pflanze_feuchte[6]={65,72,85,50,70,40};
static short pflanze_temp[6]={60,50,90,30,75,40};

/*Tilemap berechnen (Pflanzen und Küsten)*/
void welt_make_tilemap(void)
{
 int a,b,c;
 const uint8_t *relief_ptr=welt_relief;
 uint8_t *tilemap_ptr=welt_tilemap,*sprite_ptr=welt_einheiten;
 for(a=0;a<28;a++)
  for(b=0;b<28;b++)
  {
   if(*relief_ptr<welt.wasser)
   { /*Wasser*/
    c=0; /*index in kueste[]*/
    if(a>1)
    {
     if(b>1)
     {
      if(relief_ptr[-28-1]>=welt.wasser) c|=1;
     }
     if(relief_ptr[-28]>=welt.wasser) c|=2;
     if(b<26)
     {            
      if(relief_ptr[-28+1]>=welt.wasser) c|=4;
     }                          
    }
    if(b>1)
    {
     if(relief_ptr[-1]>=welt.wasser) c|=8;
    }
    if(b<26)
    {            
     if(relief_ptr[1]>=welt.wasser) c|=16;
    }                          
    if(a<26)
    {            
     if(b>1)
     {
      if(relief_ptr[28-1]>=welt.wasser) c|=32;
     }
     if(relief_ptr[28]>=welt.wasser) c|=64;
     if(b<26)
     {            
      if(relief_ptr[28+1]>=welt.wasser) c|=128;
     }                          
    }
    *tilemap_ptr=kueste[c];
    *sprite_ptr=0; /*Einheiten im Meer ertrinken*/
   }
   else /*Land*/
   {  
    if(*relief_ptr>80) /* Gebirge */
    {
     *tilemap_ptr=54;
     if(*relief_ptr==101)
      *tilemap_ptr=55; /*Krater*/
    }
    else /*Pflanzen*/
    {
     int f,t;
     int best_d,best_i,d1,d2;
     t=((int)(welt.temperatur))+ (a-14)*3 + 50 - ((int)(*relief_ptr));
     f=((int)(welt.feuchtigkeit))+ 50 - ((int)(*relief_ptr));     
     if(t<0) t=0; if(t>100) t=100;
     if(f<0) f=0; if(f>100) f=100;
     best_d=1000;best_i=0;
     for(c=0;c<6;c++)
     { 
      d1=abs(t-pflanze_temp[c]);
      d2=abs(f-pflanze_feuchte[c]);
      if(d2>d1) d1=d2;
      if(d1<best_d) {best_d=d1;best_i=c;}
     }
     *tilemap_ptr=48+best_i;
/*     c=*relief_ptr;
     c=(240+welt.wasser)/2-c;     
     c=-c/2+welt.temperatur;
     if(a<14) c=(c*a)/14;
     else c=(c*(28-a)+200*(a-14))/14;
     c=(c*3)/210;
     if(c<0) c=0; if(c>2) c=2;
     *tilemap_ptr=48+pflanzen[c][((*relief_ptr>(welt.wasser+210)/2)?1:0)];*/
    }
   }
   relief_ptr++;
   tilemap_ptr++;
   sprite_ptr++;
  }
}

/*Relief durch Zufällige Pyramiden erzeugen (neu ignoriert)*/
void relief_erzeugen(int neu)
{
 int a,b,c,d,e,f;
 uint8_t *rptr=NULL;
 
 rptr=welt_relief;
 /*Zufallszahlen, gewichtet durch Pyramide*/ 
 for(a=0;a<28;a++)
 {
  c=abs(a-14);
  for(b=0;b<28;b++)
  {
   if(a<1||b<1||a>26||b>26)
    *rptr=0;
   else
   {
    d=abs(b-14);
    if(d<c) d=c;
    *rptr=qp_random((14-d)*100)/14;            
   }              
   rptr++;
  }
 }
 /*Kleinere Pyramiden*/ 
 for(a=0;a<20;a++)
 {
  b=qp_random(22)+3;
  c=qp_random(22)+3;
  for(d=-2;d<=2;d++)
   for(e=-2;e<=2;e++)
   {
    f=abs(e); if(f>abs(d)) f=abs(d);
    f=3-f;
    while(f--)
    {
     rptr=&(welt_relief[(b+d)*28+(c+e)]);
     *rptr+=qp_random(10)+qp_random(10);
     if(*rptr>100) *rptr=100;
    }
   } 
 }
}

/*Relief auf dem Bildschirm anzeigen*/
void dump_relief(int xdst,int ydst)
{
 uint32_t a;
 struct DIB_HEADER dib_header=
 {sizeof(struct DIB_HEADER),28,-28,1,8,0,28*28,1024,1024,256,0};
 uint32_t *palette=NULL;
 int x=0,y=0,dx=28,dy=28/*,xdst=320,ydst=240*/;
 palette=malloc(256*sizeof(uint32_t));
 if(palette==NULL) return;
 for(a=0;a<256;a++)
 {
  if(a>240)
   palette[a]=a*0x010000;
  else if(a>127)
   palette[a]=a*0x010101;
  else
   palette[a]=a;
 }
 register_bitmap(200,&dib_header,palette,welt_relief,NULL,NULL,NULL);
 draw_bitmaps(1,get_bitmap_slot(200),&xdst,&ydst,&dx,&dy,&x,&y);
/* win_flush();*/
 register_bitmap(200,NULL,NULL,NULL,NULL,NULL,NULL);
}

/*Explosion für Kampf/Vulkanausbruch*/
void explosion_klein(int pos)
{
 int a;
 int xdst,ydst,delta,xsrc,ysrc;
 xdst=SPIELFELD_X+5+(pos%28)*16;     
 ydst=SPIELFELD_Y+5+(pos/28)*16;     
 delta=16;
 ysrc=16;
 for(a=0;a<4;a++)
 {
  welt_bildaufbau(1);
  xsrc=464+32*a+16;
  draw_bitmaps_x(1,get_bitmap_slot(BM_MEER_DAT),&xdst,&ydst,&delta,&delta,&xsrc,&ysrc,-1);
  xsrc-=16;
  draw_bitmaps_x(1,get_bitmap_slot(BM_MEER_DAT),&xdst,&ydst,&delta,&delta,&xsrc,&ysrc,1);
  win_flush();
  yield_thread();
  clk_wait(CLK_TCK/(welt.fast_fight?13:10));
 }
}

/*Explosion für Meteroriteneinschlag*/
void explosion_gross(int pos)
{
 int a;
 int xdst,ydst,delta,xsrc,ysrc;
 xdst=SPIELFELD_X+5+(pos%28)*16-16;     
 ydst=SPIELFELD_Y+5+(pos/28)*16-16;     
 delta=48;
 for(a=0;a<6;a++)
 {
  welt_bildaufbau(1);
  xsrc=0+96*a+48;
  ysrc=32;
  draw_bitmaps_x(1,get_bitmap_slot(BM_MEER_DAT),&xdst,&ydst,&delta,&delta,&xsrc,&ysrc,-1);
  xsrc-=48;
  draw_bitmaps_x(1,get_bitmap_slot(BM_MEER_DAT),&xdst,&ydst,&delta,&delta,&xsrc,&ysrc,1);
  win_flush();
  yield_thread();
  clk_wait(CLK_TCK/8);
 }     
 for(a=0;a<6;a++)
 {
  xsrc=0+48*a;
  ysrc=80;
  draw_bitmaps(1,get_bitmap_slot(BM_MEER_DAT),&xdst,&ydst,&delta,&delta,&xsrc,&ysrc);
  win_flush();
  yield_thread();
  clk_wait(CLK_TCK/8);
 }     
}

/*Kampf ausführen - NOTZERO heißt Verteitiger gewinnt*/
int welt_kampf(angreifer,verteidiger,_feld)
{
 int a=angreifer;
 int feld=welt_tilemap[_feld]-48; 
 /*Revierkampf gegen "sich selbst" bedeutet gegen Raubtiere*/
 angreifer= spieler[angreifer].evol.eigenschaften[EVO_ANGRIFF]*2+
            spieler[angreifer].evol.eigenschaften[EVO_PFLANZE(feld)]*2+
            spieler[angreifer].evol.eigenschaften[EVO_INT]+
            (spieler[angreifer].sammel.revierkampf[0]+
            spieler[angreifer].sammel.revierkampf[1]+
            spieler[angreifer].sammel.revierkampf[2]+
            spieler[angreifer].sammel.revierkampf[3]+
            spieler[angreifer].sammel.revierkampf[4]+
            spieler[angreifer].sammel.revierkampf[5])*10;
 verteidiger= spieler[verteidiger].evol.eigenschaften[EVO_VERTEID]*2+
            spieler[verteidiger].evol.eigenschaften[EVO_PFLANZE(feld)]*2+
            spieler[verteidiger].evol.eigenschaften[EVO_INT]+
            (spieler[verteidiger].sammel.revierkampf[0]+
            spieler[verteidiger].sammel.revierkampf[1]+
            spieler[verteidiger].sammel.revierkampf[2]+
            spieler[verteidiger].sammel.revierkampf[3]+
            spieler[verteidiger].sammel.revierkampf[4]+
            spieler[verteidiger].sammel.revierkampf[5])*10/*+
            10+((spieler[verteidiger].evol.einheiten<2)?25:0)*/;
 printf("Angreifer:%3u   Verteidiger:%3u\n");
 if(wave_is_playing()) stop_wave();
 play_wave(wav_ptrs[SOUND_KAMPF_W-1],wav_lens[SOUND_KAMPF_W-1]);
 explosion_klein(_feld);
 if(wave_is_playing()&&!welt.fast_fight) explosion_klein(_feld);
 if(qp_random(angreifer)>qp_random(verteidiger)) return 1;
 return 0;
}

/*Kann der Spieler auf das Feld ziehen ?*/
int ist_nachbarschaft(int index,int spieler_num)
{
 int a,b,c;
 spieler_num++;
 for(a=-28;a<=28;a+=28)
  for(b=-1;b<=1;b++)
  {
   c=a+b+index;
   if(c<0||c>=28*28) continue; /*Außerhalb des Spielfelds*/
   if(a&&b) continue; /*Diagonal*/
   if(welt_einheiten[c]==spieler_num)
    return 1;
  } 
 return 0;
}

static void CALLCONV redraw_func(void*p)
{
 if(!(((int)p)&8))
  menue_init();
 draw_frame(HUD_X-1,HUD_Y-1,180+1,437+2,1,0,0);
 draw_frame(HUD_X,HUD_Y,180-1,437,2,FARBE_HELL,FARBE_DUNKEL);
 draw_frame(SPIELFELD_X,SPIELFELD_Y,64*7+10,64*7+10,2,FARBE_HELL,FARBE_DUNKEL);
 draw_frame(SPIELFELD_X+4,SPIELFELD_Y+4,64*7+2,64*7+2,2,0,0);
 weiter_knopf(0);
 welt_bildaufbau(1);
 welt_stat_info(((int)p)&7);
}

/*Qualität des Spielzugs. Rekursion wird heruntergezählt*/
long feld_bewerten(int feld,int spieler_num,int rekursion)
{
 int a,b,c;
 long r=0;
 static unsigned int localrand=0xDEADBEEF;
 a=welt_tilemap[feld];
 r=localrand&15; 
 a-=48; 
 if(a>=0&&a<6)
 {
  r+=spieler[spieler_num].evol.eigenschaften[a];
  b=welt_einheiten[feld];
  b--;
  if(b>=0)
  {
   if(welt.runde&&b!=spieler_num)
   {
    r-=25;
    r+=spieler[spieler_num].evol.eigenschaften[a];
    r-=spieler[b].evol.eigenschaften[a];
    r+=spieler[spieler_num].evol.eigenschaften[EVO_ANGRIFF];
    r-=spieler[b].evol.eigenschaften[EVO_VERTEID];
    if(rekursion<2)
    {
     r+=spieler[spieler_num].evol.eigenschaften[EVO_VERTEID];
     r-=spieler[b].evol.eigenschaften[EVO_ANGRIFF];                  
    }
    r+=spieler[spieler_num].evol.eigenschaften[EVO_INT]/4;
    r-=spieler[b].evol.eigenschaften[EVO_INT]/4;
   }
   else r-=100;
  }
 }
 else if(a>5) {r-=20;}
 else if(a<-1) {r-=30;}
 c=feld/48; b=feld%48;
 localrand=((localrand*12345)+54321)^r; 
 r/=16;
 if(rekursion>0)
 {
  rekursion--;
/*  if(a>=0&&a<=5)
   spieler[spieler_num].evol.eigenschaften[a]+=rekursion;*/
  if(c) r+=feld_bewerten(feld-28,spieler_num,rekursion);
  if(b) r+=feld_bewerten(feld-1,spieler_num,rekursion);
  if(c<27) r+=feld_bewerten(feld+28,spieler_num,rekursion);
  if(b<27) r+=feld_bewerten(feld+1,spieler_num,rekursion);
/*  if(a>=0&&a<=5)
   spieler[spieler_num].evol.eigenschaften[a]-=rekursion;*/
 }
 return r; 
} 

static int CALLCONV clickable_func(unsigned int x,unsigned int y)
{
 if(y<21) return menue_clickable(x);
 if(y>480-20&&x>HUD_X) return 1;
 return 0;
}

/*Ausbreitungsrunde für Computergegner*/
void ausbreitungsrunde_ki(int spieler_num)
{
 int a,b;long c;
 int best_tile; long best_val;
 int pflanze0=0;
 uint32_t inkey;
 qp_randomize();
 set_redraw_func(redraw_func,(void*)(spieler_num|8));
 redraw_func((void*)spieler_num);
 win_flush();
 set_clickable_func(clickable_func);
 while(spieler[spieler_num].sammel.kinder||spieler[spieler_num].sammel.bewegung)
 {
  if(spieler[spieler_num].sammel.kinder)
  {
   spieler[spieler_num].sammel.kinder--;
   best_tile=0;
   best_val=-30000;
   /*Feld suchen*/
   for(a=1;a<27;a++)
    for(b=1;b<27;b++)
    {
     c=a*28+b;
     if(welt_tilemap[c]>=47&&welt_tilemap[c]<=53&&
       (ist_nachbarschaft(c,spieler_num)||spieler[spieler_num].evol.einheiten<1)&&
       (welt.runde||welt_einheiten[c]==0)&&welt_einheiten[c]!=spieler_num+1)
     {
      c=feld_bewerten(c,spieler_num,welt.runde?(5-spieler[spieler_num].evol.iq):5);
      if(c==best_val)
       if(qp_random(3)) c++;
      if(c>best_val)
      {
       best_val=c;
       best_tile=a*28+b;              
      }
     }
    }   
   printf("tile=%d,%d\n",best_tile/28,best_tile%28);
   if(best_tile) /*Feld gefunden ?*/
   {
    /*Kampf*/
    if(welt_einheiten[best_tile])    
    {
     if(welt_kampf(spieler_num,welt_einheiten[best_tile]-1,best_tile))
     {
      spieler[welt_einheiten[best_tile]-1].evol.einheiten--;
      goto ABSETZEN;
     }     
    }
    else
    {
ABSETZEN:
     welt_einheiten[best_tile]=spieler_num+1;
     spieler[spieler_num].evol.einheiten++;
     clk_wait(25); 
     /*in der Runde 0 an Pflanze anpassen*/
     if(welt.runde==0&&spieler[spieler_num].evol.einheiten==1)
     {
      spieler[spieler_num].evol.eigenschaften[welt_tilemap[best_tile]-48]+=30;
      pflanze0=welt_tilemap[best_tile]-48;
     }
    }
   }
  }
  else /*Einheit aufheben*/
  {
   spieler[spieler_num].sammel.bewegung--;
   if(spieler[spieler_num].evol.einheiten>1)
   {
    best_tile=0;
    best_val=30000;
    for(a=1;a<27;a++)
     for(b=1;b<27;b++)
     {
      c=a*28+b;
      if(welt_einheiten[c]==spieler_num+1)        
      {
       c=feld_bewerten(c,spieler_num,5-spieler[spieler_num].evol.iq)+qp_random(100);
       if(c<best_val)
       {
        best_val=c;
        best_tile=a*28+b;              
       }
      }
    }   
    if(best_tile)
    {
     welt_einheiten[best_tile]=0;     
     spieler[spieler_num].sammel.kinder++;
     spieler[spieler_num].evol.einheiten--;
    }                                    
   }
  }
  welt_statistik();
/*  if(game_input_get()==27) exit(0);*/
  redraw_func((void*)spieler_num);        
  win_flush();
 }
 /*Veränderung in der Runde 0 rückgängig machen*/
 if(welt.runde==0)
  spieler[spieler_num].evol.eigenschaften[pflanze0]-=30;
  
 if(welt.auto_weiter) return;
 inkey=0;
 while(inkey!=13)
 {
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,25+437,640,480,0))
   inkey=13 ;
  a=welt_klick(inkey);
  if(a>-1&&(inkey&MOUSE_KEYMASK)==MOUSE_KEYR)
  {
   if(!(inkey&MOUSE_UP))
   {
    printf("R");
    if(welt_einheiten[a]) /*rechtsklick auf Einheit*/
    {
     while(game_input_ready()) game_input_get();
     b=welt_einheiten[a];
     welt_einheiten[a]=0;
     welt_bildaufbau(0);
     win_flush();
     inkey=game_input_get();
     printf("I");
     welt_einheiten[a]=b;
     welt_bildaufbau(0);
     win_flush();
    }                    
   }
  }
 }
}

/*Ausbreitungsrunde für Spieler*/
void ausbreitungsrunde(int spieler_num)
{ 
 int a,b;
 uint32_t inkey=0;
 set_redraw_func(redraw_func,(void*)(spieler_num|8));
 redraw_func((void*)spieler_num);
 win_flush();
 set_clickable_func(clickable_func);
 while(inkey!=13)
 {
  inkey=game_input_get();
PROC_INKEY:
  if(mouse_in_field(inkey,64*7+8,26+437,640,480,0)) 
  {
   weiter_knopf(1);
   inkey=13;
  }
  if(inkey==13&&spieler[spieler_num].sammel.kinder)
  {
   if(!msgbox_janein(get_string(STR_CONF))) inkey=0;                                                 
  }
  a=welt_klick(inkey);
  if(a>-1)
  {
   printf("a=%d inkey=%X\n",a,inkey);
   if((inkey&MOUSE_KEYMASK)==MOUSE_KEYR)
   {
    if(!(inkey&MOUSE_UP))
    {
     printf("R");
     if(welt_einheiten[a]) /*rechtsklick auf Einheit*/
     {
      while(game_input_ready()) game_input_get();
      b=welt_einheiten[a];
      welt_einheiten[a]=0;
      welt_bildaufbau(0);
      win_flush();
      inkey=game_input_get();
      printf("I");
      welt_einheiten[a]=b;
      welt_bildaufbau(0);
      win_flush();
      goto PROC_INKEY;
     }                    
    }
   }
   if((inkey&MOUSE_KEYMASK)==MOUSE_KEYL)
   {
    if(!(inkey&MOUSE_UP)) /*linksklick*/
    {
     if(welt_einheiten[a]) /*Einheit aufheben/Angreifen*/
     {
      if(welt_einheiten[a]==(spieler_num+1)) /*Einheit aufheben*/
      {
       if(spieler[spieler_num].sammel.bewegung&&spieler[spieler_num].evol.einheiten>1)
       {
        spieler[spieler_num].sammel.bewegung--; 
        spieler[spieler_num].sammel.kinder++; 
/*        spieler[spieler_num].evol.einheiten--;*/
        welt_statistik();
        welt_einheiten[a]=0;
        redraw_func((void*)spieler_num);        
        win_flush();
        welt_statistik();
       }
      }
      else /*Einheit angreifen*/
      {
       if(spieler[spieler_num].sammel.kinder
        &&ist_nachbarschaft(a,spieler_num)
        &&welt_tilemap[a]>=48&&welt_tilemap[a]<=53)
       {
        spieler[spieler_num].sammel.kinder--;
        if(welt_kampf(spieler_num,welt_einheiten[a]-1,a))
        {
         spieler[welt_einheiten[a]-1].evol.einheiten--;
         welt_einheiten[a]=spieler_num+1;
/*         spieler[spieler_num].evol.einheiten++;*/
         welt_statistik();
        }
        redraw_func((void*)spieler_num);        
        win_flush();
       }         
      }
     }
     else /*Einheit absetzen*/
     {
      if(spieler[spieler_num].sammel.kinder
       &&(ist_nachbarschaft(a,spieler_num)||!spieler[spieler_num].evol.einheiten)
       &&welt_tilemap[a]>=47&&welt_tilemap[a]<=53)
      {
       spieler[spieler_num].sammel.kinder--; 
       welt_einheiten[a]=spieler_num+1;;
       /*spieler[spieler_num].evol.einheiten++;*/
       welt_statistik();
       redraw_func((void*)spieler_num);        
       win_flush();
      }         
     }
    }
   }         
  }                
 }           
 welt_statistik();
}

#ifdef GAME_MAGNETIC_PLANET
/*Bild für eine Katastrophe anzeigen*/
static void CALLCONV kat_redraw_func(void*p)
{
 int s=0,b=0;
 int xdst,ydst,dx,dy,xysrc,num; 
 num=(int)p;
 redraw_func((void*)-1);
 switch(num)
 {
  case KAT_HITZE: s=STR_HEAT; b=BM_KAT_HITZE;break;
  case KAT_KAELTE: s=STR_COLD; b=BM_KAT_KAELTE;break;
  case KAT_METEOR: s=STR_METEOR; b=BM_KAT_METEOR;break;
  case KAT_VIREN: s=STR_VIRUS; b=BM_KAT_VIREN;break;
  case KAT_VULKAN: s=STR_VULKAN; b=BM_KAT_VULKAN;break;
  case KAT_FLUT: s=STR_FLOOD; b=BM_KAT_FLUT;break;
  case KAT_BEBEN: s=STR_QUAKE; b=BM_KAT_BEBEN;break;
  case KAT_SIEG: s=STR_VICTORY; b=BM_VICTORY;break;
  case KAT_TOT: s=STR_DEFEAT; b=BM_DEFEAT;break;
  default: return;           
 }
 texture_rectangle(160-3,120-20,320+6,240+3+20,get_bitmap_slot(BM_WINDOW_MAIN),0,0,8,480);
 button_text(160-2,120-19,320+4,20,get_string(s),0);
 draw_frame(160-3,120-20,320+6,240+3+20,1,0,0);
 draw_frame(160-2,120-19,320+4,240+2+20,2,FARBE_HELL,FARBE_DUNKEL);
 xysrc=0; 
 xdst=160; ydst=121;
 dx=320;dy=240;
 draw_bitmaps(1,get_bitmap_slot(b),&xdst,&ydst,&dx,&dy,&xysrc,&xysrc);
}

static int CALLCONV kat_clickable_func(unsigned int x,unsigned int y)
{
 if(y<21) return menue_clickable(x);
 if(y>480-20&&x>HUD_X) return 1;
 if(x>160-3&&y>120-20&&x<160-3+320+6&&y<120-20+240+3+20) return 1;
 return 0;
}

int katastrophe_bild(int num)
{
 int inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;

 set_redraw_func(kat_redraw_func,(void*)num);
 kat_redraw_func((void*)num);
 win_flush();
 cf_alt=set_clickable_func(kat_clickable_func);
 while(game_input_ready()) game_input_get();
 while(inkey!=13)
 {
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,25+437,640,480,0)) 
  {
   weiter_knopf(1);
   inkey=13;
  }  
  if(mouse_in_field(inkey,160-3,120-20,160-3+320+6,120-20+240+3+20,0)) 
   inkey=13;
 }
 menue_init();
 welt_bildaufbau(1);
 welt_stat_info(-1);
 win_flush();    
 set_clickable_func(cf_alt);
}

#endif  

/*Zahl (Meeresspiegel,Temperatur,Feuchtigkeit) ändern (10-90)*/
static int welt_aendern(uint8_t *ptr,int val)
{
 val+=(int)(*ptr);
 if(val<10) val=10;
 if(val>90) val=90;
 if(val==*ptr) return 0;
 *ptr=val;
 return 1;
}

/*Katastrophe ausführen*/
int katastrophe(int num)
{
 int a,b,c,d;
 uint32_t inkey;
 set_redraw_func(redraw_func,(void*)-1);
 redraw_func((void*)-1);
 set_clickable_func(clickable_func);
 switch(num)
 {   
  case KAT_HITZE:
    if(!(welt_aendern(&(welt.temperatur),10)|
       welt_aendern(&(welt.feuchtigkeit),-10)|
       welt_aendern(&(welt.wasser),-2))) return 0;
       welt_make_tilemap();
    break; 
  case KAT_KAELTE:
    if(!(welt_aendern(&(welt.temperatur),-10)|
       welt_aendern(&(welt.feuchtigkeit),10))) return 0;
       welt_make_tilemap();
    break; 
  case KAT_METEOR:
    for(a=10000;a>0;a--)
    {
     b=(qp_random(24)+2)*28+qp_random(24)+2;
     if(welt_relief[b]>welt.wasser) break;
    }
    if(a<1) return 0;
    explosion_gross(b);
    for(a=-28;a<=28;a+=28)
     for(c=-1;c<=1;c++)
     {
      welt_relief[a+b+c]=100;
      welt_einheiten[a+b+c]=0;    
     }
    welt_relief[b]=101;
    welt_aendern(&(welt.temperatur),-10);
    welt_aendern(&(welt.wasser),-3);
    welt_make_tilemap();
    break; 
   case KAT_VIREN:
    for(a=10000;a>0;a--)
    {
     b=(qp_random(22)+3)*28+qp_random(22)+3;
     if(welt_einheiten[b]) break;
    }
    if(a<1) return 0;
    for(a=-3*28;a<=3*28;a+=28)
     for(c=-3;c<=3;c++)
     {
      yield_thread();
      if(!welt_einheiten[a+b+c]) continue;
      if(qp_random(100)<50) continue;
      welt_einheiten[a+b+c]=0;
      welt_bildaufbau(0);
      welt_stat_info(-1);
      win_flush();      
      clk_wait(CLK_TCK/10);
     }        
    break;
   case KAT_VULKAN:
    for(c=0;c<5;c++)
    {
     for(a=20000-1000*c;a>0;a--)
     {
      b=(qp_random(24)+2)*28+qp_random(24)+2;
      if(welt_tilemap[b]==54) break;
     }
     if(a<1&&c<1) return 0;
     if(a<1) break;
     explosion_klein(b);
     for(a=-28;a<=28;a+=28)
      for(d=-1;d<=1;d++)
      {
       welt_einheiten[a+b+d]=0;
      }             
    }
    break;
   case KAT_FLUT:
    if(!(welt_aendern(&(welt.temperatur),10)|
       welt_aendern(&(welt.wasser),5))) return 0;
    welt_make_tilemap();
    break;
   case KAT_BEBEN:
    qp_randomize();
    relief_erzeugen(1);
    welt_make_tilemap();
    break;
   case -1: break; /*nur Warten*/
   default: return 0;
 }    
 /*Jemand im Wasser oder Gebirge ?*/
 for(a=0;a<28*28;a++)
 {
  if(welt_einheiten[a])
  {
   b=welt_tilemap[a];
   if(b<47||b>53) welt_einheiten[a]=0;                     
  }                    
 }
 welt_statistik();
 redraw_func((void*)-1);
 win_flush();
 inkey=0;
 while( inkey!=13)
 {
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,25+437,640,480,0)) 
  {
   weiter_knopf(1);
   inkey=13;
  }          
  a=welt_klick(inkey);
  if(a>-1&&(inkey&MOUSE_KEYMASK)==MOUSE_KEYR)
  {
   if(!(inkey&MOUSE_UP))
   {
    printf("R");
    if(welt_einheiten[a]) /*rechtsklick auf Einheit*/
    {
     while(game_input_ready()) game_input_get();
     b=welt_einheiten[a];
     welt_einheiten[a]=0;
     welt_bildaufbau(0);
     win_flush();
     inkey=game_input_get();
     printf("I");
     welt_einheiten[a]=b;
     welt_bildaufbau(0);
     win_flush();
    }                    
   }
  }
 }
 return 1;
}

void welt_ende_pruefen(void)
{
 int a,b=0,c=0;
 for(a=0;a<6;a++)
  if(!spieler[a].evol.tot)
  {
   if(!spieler[a].evol.einheiten)
   {
    spieler[a].evol.tot=1;
    if(spieler[a].evol.typ==SPIELERTYP_MENSCH)
     msgbox_ok(get_string(STR_DEAD)); 
    if(spieler[a].evol.typ==SPIELERTYP_COMPUTER)
     msgbox_ok(get_string(STR_AIDEAD)); 
   }
   else {b++;c=a;}
  }  
 if(b==1)
 {
  if(spieler[c].evol.typ==SPIELERTYP_MENSCH)
  {
   if(!welt.allein)
   {
    recolour_disasters(c);
    katastrophe_bild(KAT_SIEG);          
    if(!msgbox_janein(get_string(STR_ALONE)))
     exit(0);
    welt.allein=1;
   }                                         
  }
  else b=0;        
 }
 if(b<1)
 {
  katastrophe_bild(KAT_TOT);
  exit(0);      
 }
}

/*Einheiten nach der Sammelrunde töten*/
int welt_sterben_nach_sammeln(int sp)
{
 unsigned int a,b;
 uint8_t *bptr;
 bptr=welt_einheiten;
 printf("Spieler %d:  Nahrung=%d Tote=%d Sterblichkeit=%d\n",
          sp,spieler[sp].sammel.nahrung,
          spieler[sp].sammel.tote,spieler[sp].sammel.sterblichkeit); 
 sp++;
 for(a=28*28;a;a--)
 {
  if(*bptr==sp)
  {
   if(qp_random(100)<spieler[sp-1].sammel.sterblichkeit)
    *bptr=0;
  }
  bptr++;
 }
 welt_statistik();
}
