#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "EDITION.H"
#include "SPIELDAT.H"
#include "MPBITMAP.H"
#include "MPLANET.H"
#include "WELT.H"
#include "NSTRING.H"
#include "STRINGS.H"


static void draw_difficulty(int x,int y,int player,int dif,int typ,int typdown)
{
 int dx=308,dy=140,xy0=0,bm;
/* recolour_disasters(player);*/
 bm=get_bitmap_slot(BM_WINDOW_INIT0+player);
#if 0 
 draw_bitmaps(1,bm,&x,&y,&dx,&dy,&xy0,&xy0);
#endif
/*Deutschen Text im Bild abschneiden, und durch übersetzten Text ersetzen*/
 dx=140;xy0+=20; x+=20;y+=20;
 draw_bitmaps(1,bm,&x,&y,&dx,&dy,&xy0,&xy0);
 x-=20;y-=20; 
 if(dif)
 {
  dy=13;
  x+=145;
  y+=37+(dif-1)*22;
  dx=308;
  draw_bitmaps_x(1,bm,&x,&y,&dy,&dy,&dx,&dy,0);
  x-=145;
  y-=37+(dif-1)*22;
 }
 /*Text schreiben*/
 shadow_print(x+130,y+8,160,22,get_string(STR_DIFFIC),1);
 shadow_print(x+170,y+33,130,22,get_string(STR_DIFF1),0);
 shadow_print(x+170,y+55,130,22,get_string(STR_DIFF2),0);
 shadow_print(x+170,y+77,130,22,get_string(STR_DIFF3),0);
 shadow_print(x+170,y+99,130,22,get_string(STR_DIFF4),0);
 if(typ>=0)
 {
  shadow_print(x+40,y+18,60,22,get_string(STR_SPEC1+player),0);
  button_text(x+40,y+119,120,22,get_string(STR_POFF+typ),typdown);

  if(!typ) /*Roboter übermalen wenn Spieler aus ist*/
  {
   uint32_t f=0x502010;
   dx=x+104;dy=y+114;
   x+=42;y+=38;
   fill_rectangles(1,&x,&y,&dx,&dy,&f);
  }
 }
}

/*extern int mouse_in_field(uint32_t m,unsigned int x1,unsigned int y1,
  unsigned int x2,unsigned int y2,uint32_t flags);*/

static int difficulty_click(int *_dif,int *_typ,int x,int y,int player,uint32_t inkey)
{
 int a,r=0,dif=0,typ=-1;
 if(_dif!=NULL) dif=*_dif;
 if(_typ!=NULL) typ=*_typ;
 if(mouse_in_field(inkey,x+145-2,y+37-2,x+145+13+2,y+37+22*3+13+2,0))
 {
  for(a=0;a<4;a++)
   if(mouse_in_field(inkey,x+145-2,y+37-2+a*22,x+145+13+2,y+37+13+2+a*22,0))
    {dif=a+1;r=1;}
 }
 if(typ>=0&&
   (mouse_in_field(inkey,x+40,y+119,x+40+120,y+119+22,0)||
    mouse_in_field(inkey,x+42,y+38,x+104,y+114,0)))
 {
  draw_difficulty(x,y,player,dif,typ,1);
  win_flush();
  clk_wait(CLK_TCK/8);
  typ=(typ+1)%3;
  r=1;
 }
 if(r)
 {
  if(_typ!=NULL) *_typ=typ;
  if(_dif!=NULL) *_dif=dif;
 }
 return r;
}

int difficulty_clickable(int dx,int dy,int mp,int x,int y)
{
 /*Schwierigkeitsgrad*/
 if(x>dx+145-2&&x<dx+145+13+2&&y>dy+37-2&&y<dy+37+13+2+22*3)
 {
  return ((y-dy-35)%22)<17;
 }
 if(mp)
 {
  if(x>dx+40&&x<dx+40+120&&y>dy+119&&y<dy+119+22) return 1;
  if(x>dx+42&&x<dx+104&&y>dy+38&&y<dy+114) return 1;
 }
 return 0;
}

static void CALLCONV redraw_func( void*p)
{       
 int a,b;
 const char *str;
 menue_init();
 draw_frame(0,HUD_Y,640-1,460-1-21,2,FARBE_HELL,FARBE_DUNKEL);    
 weiter_knopf(0);
 button_text(0,480-22,HUD_X/2,22,get_string(STR_LOAD),0);
 button_text(HUD_X/2,480-22,HUD_X-HUD_X/2,22,get_string((((int)p)&1)?STR_SINGLEP:STR_MULTIP),0);
 str=get_string((((int)p)&1)?STR_MULTIP:STR_SINGLEP);
 write_text(FONT_RUNDE,11,23,160,22,str,0x000000,0);
 write_text(FONT_RUNDE,10,22,160,22,str,0xFFFFFF,0);
 if(((int)p)&1)
 {
  for(a=0;a<3;a++)
   for(b=0;b<2;b++)
    draw_difficulty(640/2-308*(1-b),20+a*140,a*2+b,spieler[a*2+b].evol.iq,spieler[a*2+b].evol.typ,0);
 }
 else
  draw_difficulty(640/2-308/2,480/2-140/2,0,spieler[0].evol.iq,-1,0);    
}


static int CALLCONV clickable_sp(unsigned int x,unsigned int y)
{
 if(y<21) return menue_clickable(x);
 if(y>480-20) return 1;
 if(x<640/2-308/2||y<480/2-140/2||x>640/2+308/2||y>480/2+140/2) return 0;
 return difficulty_clickable(640/2-308/2,480/2-140/2,0,x,y);
}

static int CALLCONV clickable_mp(unsigned int x,unsigned int y)
{
 int dx=640/2-308;
 if(y<21) return menue_clickable(x);
 if(y>480-20) return 1;
 if(x>640/2)dx+=308;
 if(y<20+140*2)
 {
  if(y<20+140) return difficulty_clickable(dx,20,1,x,y);
  else return difficulty_clickable(dx,20+140,1,x,y);
 }
 else if(y<20+140*3) return difficulty_clickable(dx,20+140*2,1,x,y);
 return 0;
}

void difficulty_input(int inkey,int p)
{
 int a,b,c,d;
 if(p&1)
 {
  for(a=0;a<3;a++)
   for(b=0;b<2;b++)
   {
    c=spieler[a*2+b].evol.iq;
    d=spieler[a*2+b].evol.typ;
    if(difficulty_click(&c,&d,640/2-308*(1-b),20+a*140,a*2+b,inkey))
    {
     spieler[a*2+b].evol.iq=c;
     spieler[a*2+b].evol.typ=d;
     redraw_func((void*)1);
    }
   }
 }
 else
 {
  c=spieler[0].evol.iq;
  if(difficulty_click(&c,NULL,640/2-308/2,480/2-140/2,0,inkey))
  {
   for(a=0;a<6;a++)
    spieler[a].evol.iq=c;
   redraw_func((void*)0);
  }
 }
}

static unsigned runden_button=3;

static void CALLCONV redraw_runden( void*p)
{       
 int a;
 int x,y,x0,y0,dxy;
 static unsigned short robot_x[4]={9*64,8*64,0,64*2};
 static unsigned short robot_y[4]={4*64,4*64,0,0};
 static unsigned short robot_img[4]={BM_SPECIES1,BM_SPECIES1,BM_SPECIES1,BM_TOT_X};
 const char *str;
 menue_init();
 draw_frame(0,HUD_Y,640-1,460-1-21,2,FARBE_HELL,FARBE_DUNKEL);    
 weiter_knopf(0);
 button_text(0,480-22,HUD_X/2,22,get_string(STR_BACK),0);
 str=get_string(STR_ROUNDS);
 write_text(FONT_RUNDE,11,23,220,22,str,0x000000,1);
 write_text(FONT_RUNDE,10,22,220,22,str,0xFFFFFF,1);
 
 for(a=0;a<4;a++)
 {
  shadow_print(170,80+a*75,200,20,get_string(STR_ROUNDS1+a),0);  
  x=150;y=80+a*75;
  x0=143;y0=35; dxy=17;
  draw_bitmaps(1,get_bitmap_slot(BM_WINDOW_INIT0),&x,&y,&dxy,&dxy,&x0,&y0);
  x+=2;y+=2;x0=308;y0=0; dxy=13;
  if(runden_button==a) y0=13;
  draw_bitmaps_x(1,get_bitmap_slot(BM_WINDOW_INIT0),&x,&y,&dxy,&dxy,&x0,&y0,1);
  x=460;y-=20;dxy=64;
  x0=robot_x[a]; y0=robot_y[a];
  draw_bitmaps(1,get_bitmap_slot(robot_img[a]),&x,&y,&dxy,&dxy,&x0,&y0);
 }

}

static int CALLCONV clickable_runden(unsigned int x,unsigned int y)
{
 int a;
 if(y<21) return menue_clickable(x);
 if(y>480-20)
 {
  if(x>HUD_X||x<HUD_X/2) return 1;
  else return 0;
 }
 if(x>150&&x<370)
 {
  if(y<80) return 0;
  if(((y-80)%75)<20) return 1;
  return 0;
 } 
 return 0; 
}


static runden_optionen[4]={5,10,20,255};

int newgame(int p)
{
 int a,b;
 uint32_t inkey=0;      
WECHSEL:
 if(p) p=1;
NEWGAME:
 set_redraw_func(redraw_func,(void*)p);
 redraw_func((void*)p);    
 win_flush();
 set_clickable_func(p?clickable_mp:clickable_sp);

 while(inkey!=13)
 { 
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,25+437,640,480,0)) 
  {
   weiter_knopf(1);
   clk_wait(CLK_TCK/10);
   inkey=13;
  }
  if(mouse_in_field(inkey,8,25+437,32*7+7,480,0)) 
  {
   button_text(0,480-22,HUD_X/2,22,get_string(STR_LOAD),1);
   clk_wait(CLK_TCK/10);
   if(load_game()) return 2;
  }
  if(mouse_in_field(inkey,32*7+8,25+437,64*7+7,480,0)) 
  {
   button_text(HUD_X/2,480-22,HUD_X-HUD_X/2,22,get_string((((int)p)&1)?STR_SINGLEP:STR_MULTIP),0);
   clk_wait(CLK_TCK/10);
   p^=1;
   inkey=0;   
   goto WECHSEL;
  }
  if(inkey==13) break;
  
  difficulty_input(inkey,p);
  redraw_func((void*)p);
  win_flush();
 }  

 set_redraw_func(redraw_runden,NULL);
 redraw_runden(NULL);    
 win_flush();
 set_clickable_func(clickable_runden);

 inkey=0;
 while(inkey!=13)
 { 
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,25+437,640,480,0)) 
  {
   weiter_knopf(1);
   clk_wait(CLK_TCK/10);
   inkey=13;
  }
  if(mouse_in_field(inkey,8,25+437,32*7+7,480,0)) 
  {
   button_text(0,480-22,HUD_X/2,22,get_string(STR_BACK),1);
   clk_wait(CLK_TCK/10);
   inkey=27;
  }
  if(inkey==13) break;
  if(inkey==27) goto NEWGAME;
  for(a=0;a<4;a++)
  {
   if(mouse_in_field(inkey,150,80+75*a,370,80+20+75*a,0))
    runden_button=a;                                                      
  }
  redraw_runden(NULL);    
  win_flush();
 }  
 welt.runde_max=runden_optionen[runden_button];
 
 return 0;
}
