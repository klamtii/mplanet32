#include <stddef.h>
#include <stdio.h>

#include "EDITION.H"
#include "SPIELDAT.H"
#include "SYSTEMIO.H"
#include "MPBITMAP.H"
#include "MPLANET.H"
#include "STRINGS.H"
#include "NSTRING.H"
#include "WELT.H"
#include "GUI.H"

#define TEXTURE_BACKGROUND(x,y,dx,dy) texture_rectangle((x),(y),(dx),(dy),get_bitmap_slot(BM_WINDOW_MAIN),0,0,8,480)

/*Funktion um Untergrund widerherzustellen*/
static void (*CALLCONV redraw_func)( void*p)=NULL;
/*Parameter für die Funktion*/
static void *redraw_p=NULL;

void menue_init(void);

static void redraw_bg(void)
{
 menue_init();
 if(redraw_func!=NULL)
  redraw_func(redraw_p);      
 win_flush();
}

/*Funktionszeiger zum Zeichnen des Hintergrunds festlegen*/
void set_redraw_func( void (*CALLCONV func)( void*p),void *p)
{
 redraw_func=NULL;
 redraw_p=p;
 redraw_func=func;
}

/*liefer NOTZERO wenn Mauseingabe in einem Bereich liegt*/
int mouse_in_field(uint32_t m,unsigned int x1,unsigned int y1,
  unsigned int x2,unsigned int y2,uint32_t flags)
{
 unsigned int a;
 if(!(m&MOUSE_FLAG)) return 0;
 if((m^flags)&(MOUSE_UP|MOUSE_KEYMASK)) return 0;
 if(x1>x2) {a=x2;x2=x1;x1=a;}
 a=MOUSE_X(m);
 if(a<x1||a>=x2) return 0;
 if(y1>y2) {a=y2;y2=y1;y1=a;}
 a=MOUSE_Y(m);
 if(a<y1||a>=y2) return 0;
 return 1; 
};

/*Popup-Fenster zeichnen (mit Knöpfen)*/
void draw_popup(int x,int y,int dx,int dy,const char *text,
 int knopf_schliesen,const char *knopf_links,const char *knopf_rechts,int down)
{
 if(down==0)TEXTURE_BACKGROUND(x,y,dx,dy);        
 draw_frame(x,y,dx,dy,1,0,0);
 draw_frame(x+1,y+1,dx-2,dy-2,2,FARBE_HELL,FARBE_DUNKEL);
 draw_frame(x,y,dx,20,1,0,0);
 draw_frame(x,y+dy-20,dx,20,1,0,0);
 button_text(x+1+(knopf_schliesen?20:0),y+1,dx-1-(knopf_schliesen?20:0),20,text,0);
 if(knopf_schliesen)
  button_icon(x+1,y+1,20,20,get_bitmap_slot(BM_SCHLIESEN),5,8,0,(down==1));
 if(knopf_links==NULL) knopf_links=" "; 
 if(knopf_rechts==NULL) knopf_rechts=" "; 
 if(knopf_links!=NULL)
 {
  button_text(x+1,y+dy-20,dx/2-1,20,knopf_links,(down==2));
 }
 if(knopf_rechts!=NULL)
 {
  button_text(x+dx/2,y+dy-20,x+dx-(dx/2+x),20,knopf_rechts,(down==3));
 } 
}

/*Klick-Animation für Knöpfe in Popup-Fenster*/
void click_popup(int x,int y,int dx,int dy,const char *text,
 int knopf_schliesen,const char *knopf_links,const char *knopf_rechts,int down)
{
 draw_popup(x,y,dx,dy,text,knopf_schliesen,knopf_links,knopf_rechts,down);
 win_flush();
 clk_wait((CLK_TCK/8)|1); 
 draw_popup(x,y,dx,dy,text,knopf_schliesen,knopf_links,knopf_rechts,0); 
 win_flush();
}

/*Scrollbar mit Knöpfen Zeichnen 
 0=Evolution, 1=Evolutionspunkte, 2=Einstellung*/
void draw_scrollbar(int x,int y,int dx,int typ,int proz,int down)
{
 switch(typ)
 {
  case 0: typ=10-typ/10; break;
  case 1: typ=0; break;
  default: typ=8;break;
 }
 button_plusminus(x,y,16,16,-1,down==1);
 scrollbar(x+15,y,dx,(proz*dx)/100,typ);
 button_plusminus(x+dx+14,y,16,16,1,down==2);      
}

/*Knöpfe in Scrollbar anklicken*/
void click_scrollbar(int x,int y,int dx,int typ,int proz,int down)
{
 draw_scrollbar(x,y,dx,typ,proz,down);
 win_flush();
 clk_wait((CLK_TCK/8)|1); 
 draw_scrollbar(x,y,dx,typ,proz,0);
 win_flush();
}

static const unsigned short dim_msgbox[]={160,160,320,160};

static int CALLCONV clickable_msgjn(unsigned int x,unsigned int y)
{
 if(x<=dim_msgbox[0]||x>=dim_msgbox[0]+dim_msgbox[2]) return 0;
 if(y<=dim_msgbox[1]||y>=dim_msgbox[1]+dim_msgbox[3]) return 0;
 if(y>dim_msgbox[1]+dim_msgbox[3]-20) return 1;
 if(y<dim_msgbox[1]+20&&x<dim_msgbox[0]+20) return 1;
 return 0;
}

/*Messagebox Ja/Nein - liefert NOTZERO bei ja zurück*/
int msgbox_janein(const char *text)
{
 int r=0;
 uint32_t inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;

 draw_popup(dim_msgbox[0],dim_msgbox[1],dim_msgbox[2],dim_msgbox[3],
  get_string(STR_MSGBOX),0,get_string(STR_YES),get_string(STR_NO),0);
 button_text(dim_msgbox[0]+1,dim_msgbox[1]+20,dim_msgbox[2]-1,dim_msgbox[3]-40,text,0);
 win_flush();
 cf_alt=set_clickable_func(clickable_msgjn);
 while(inkey!=13&&inkey!=27)
 {
  inkey=win_input_get();
  if(mouse_in_field(inkey,dim_msgbox[0],dim_msgbox[1]+dim_msgbox[3]-20,
    dim_msgbox[0]+dim_msgbox[2]/2,dim_msgbox[1]+dim_msgbox[3],0))
    {
     click_popup(dim_msgbox[0],dim_msgbox[1],dim_msgbox[2],dim_msgbox[3],
      get_string(STR_MSGBOX),0,get_string(STR_YES),get_string(STR_NO),2);
     r=1;
     break;
    }
  if(mouse_in_field(inkey,dim_msgbox[0]+dim_msgbox[2]/2,dim_msgbox[1]+dim_msgbox[3]-20,
    dim_msgbox[0]+dim_msgbox[2],dim_msgbox[1]+dim_msgbox[3],0))
    {
     click_popup(dim_msgbox[0],dim_msgbox[1],dim_msgbox[2],dim_msgbox[3],
      get_string(STR_MSGBOX),0,get_string(STR_YES),get_string(STR_NO),3);
     r=0;
     break;
    }
  entropy_add(inkey);
 }
 redraw_bg();    
 set_clickable_func(cf_alt);
 return r;
}

static int CALLCONV clickable_msgok(unsigned int x,unsigned int y)
{
 if(x<=dim_msgbox[0]||x>=dim_msgbox[0]+dim_msgbox[2]) return 0;
 if(y<=dim_msgbox[1]||y>=dim_msgbox[1]+dim_msgbox[3]) return 0;
 if(y>dim_msgbox[1]+dim_msgbox[3]-20&&x>dim_msgbox[0]+dim_msgbox[2]/2) return 1;
 if(y<dim_msgbox[1]+20&&x<dim_msgbox[0]+20) return 1;
 return 0;
}

/*Messagebox OK */
void msgbox_ok(const char *text)
{
 uint32_t inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;
 draw_popup(dim_msgbox[0],dim_msgbox[1],dim_msgbox[2],dim_msgbox[3],
  get_string(STR_INFOBOX),0,0,get_string(STR_CONT),0);
 button_text(dim_msgbox[0]+1,dim_msgbox[1]+20,dim_msgbox[2]-1,dim_msgbox[3]-40,text,0);
 win_flush();
 cf_alt=set_clickable_func(clickable_msgok);
 while(inkey!=13&&inkey!=27)
 {
  inkey=win_input_get();
  if(mouse_in_field(inkey,dim_msgbox[0]+dim_msgbox[2]/2,dim_msgbox[1]+dim_msgbox[3]-20,
    dim_msgbox[0]+dim_msgbox[2],dim_msgbox[1]+dim_msgbox[3],0))
    {
     click_popup(dim_msgbox[0],dim_msgbox[1],dim_msgbox[2],dim_msgbox[3],
      get_string(STR_INFOBOX),0,0,get_string(STR_CONT),3);
     break;
    }
  entropy_add(inkey);
 }
 redraw_bg();    
 set_clickable_func(cf_alt);
}

static const unsigned short dim_sound[]={160,120,320,200};


static int CALLCONV clickable_snd(unsigned int x,unsigned int y)
{
 if(x<=dim_sound[0]||x>=dim_sound[0]+dim_sound[2]) return 0;
 if(y<=dim_sound[1]||y>=dim_sound[1]+dim_sound[3]) return 0;
 if(y>dim_sound[1]+dim_sound[3]-20&&x>dim_sound[0]+dim_sound[2]/2) return 1;
 if(y<dim_sound[1]+20&&x<dim_sound[0]+20) return 1;
 if(x>dim_sound[0]+30&&y>dim_sound[1]+100&&x<dim_sound[0]+30+200+32&&y<dim_sound[1]+100+16
    &&(x<dim_sound[0]+46||x>dim_sound[0]+30+200+16)) return 1; 
 if(x>dim_sound[0]+142&&y>dim_sound[1]+62&&x<dim_sound[0]+202+12&&y<dim_sound[1]+62+12
    &&(x<dim_sound[0]+142+12||x>dim_sound[0]+202)) return 1;   
 return 0;
}

/*Dialog für Geräusche oder MIDI*/
void dialog_sound(int musik)
{
 int xdst,ydst,dx,dy;
 uint32_t inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;
 cf_alt=set_clickable_func(clickable_snd);
 REDRAW:
 draw_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
    get_string(musik?STR_SETMIDI:STR_SETWAVE),1,NULL,get_string(STR_CONT),0);
 draw_frame(dim_sound[0],dim_sound[1]+20,dim_sound[2],dim_sound[3]-40,1,0,0);
 draw_frame(dim_sound[0]+1,dim_sound[1]+21,dim_sound[2]-2,dim_sound[3]-42,2,FARBE_HELL,FARBE_DUNKEL);
 shadow_print(dim_sound[0]+10,dim_sound[1]+60,110,20,get_string(musik?STR_MIDI:STR_WAVE),0);
 checkbox(dim_sound[0]+140,dim_sound[1]+60,musik?get_midi_onoff():get_wave_onoff());
 shadow_print(dim_sound[0]+160,dim_sound[1]+60,50,20,get_string(STR_ON),0);
 checkbox(dim_sound[0]+200,dim_sound[1]+60,!(musik?get_midi_onoff():get_wave_onoff()));
 shadow_print(dim_sound[0]+220,dim_sound[1]+60,50,20,get_string(STR_OFF),0);
 draw_scrollbar(dim_sound[0]+30,dim_sound[1]+100,200,2,musik?get_midi_volume():get_wave_volume(),0);
 win_flush();
 inkey=0;
 while(inkey!=27&&inkey!=13)
 {
  inkey=win_input_get();
  if(mouse_in_field(inkey,dim_sound[0]+dim_sound[2]/2,dim_sound[1]+dim_sound[3]-20,
    dim_sound[0]+dim_sound[2],dim_sound[1]+dim_sound[3],0))
    {
     click_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
      get_string(musik?STR_SETMIDI:STR_SETWAVE),1,NULL,get_string(STR_CONT),3);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_sound[0],dim_sound[1],
    dim_sound[0]+20,dim_sound[1]+20,0))
    {
     click_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
      get_string(musik?STR_SETMIDI:STR_SETWAVE),1,NULL,get_string(STR_CONT),1);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_sound[0]+30,dim_sound[1]+100,
    dim_sound[0]+46,dim_sound[1]+116,0))
    {
     xdst=musik?get_midi_volume():get_wave_volume();     
     xdst=xdst>10?xdst-10:0;
     if(musik)set_midi_volume(xdst);
     else 
     {
      set_wave_volume(xdst);
      play_wave(wav_ptrs[0],wav_lens[0]);
     }
     click_scrollbar(dim_sound[0]+30,dim_sound[1]+100,200,2,xdst,1);    
    }
  if(mouse_in_field(inkey,dim_sound[0]+30+200+16,dim_sound[1]+100,
    dim_sound[0]+46+200+16,dim_sound[1]+116,0))
    {
     xdst=musik?get_midi_volume():get_wave_volume();     
     xdst=xdst<90?xdst+10:100;
     if(musik)set_midi_volume(xdst);
     else 
     {
      set_wave_volume(xdst);
      play_wave(wav_ptrs[0],wav_lens[0]);
     }
     click_scrollbar(dim_sound[0]+30,dim_sound[1]+100,200,2,xdst,2);    
    }
  if(mouse_in_field(inkey,dim_sound[0]+140-3,dim_sound[1]+60-3,
    dim_sound[0]+140+12+3,dim_sound[1]+60+12+3,0))
    {
     if(musik)set_midi_onoff(1);              
     else set_wave_onoff(1);  
/*     printf("ON\n");*/
     goto REDRAW;            
    }
  if(mouse_in_field(inkey,dim_sound[0]+200-3,dim_sound[1]+60-3,
    dim_sound[0]+200+12+3,dim_sound[1]+60+12+3,0))
    {
     if(musik)set_midi_onoff(0);              
     else set_wave_onoff(0);              
/*     printf("OFF\n");*/
     goto REDRAW;            
    }
  entropy_add(inkey);
  while(win_input_ready()) entropy_add(win_input_get());
 }
 redraw_bg();     
 set_clickable_func(cf_alt);
}

/*Dialog zum Beenden*/
void CALLCONV menue_schliesen(void)
{
 if(msgbox_janein(get_string(STR_QUIT)))
  exit(0); 
}

/*Dialog für Geräusche*/
void CALLCONV menue_sound(void)
{
 dialog_sound(0);
}

/*Dialog für Musik*/
void CALLCONV menue_musik(void)
{
 dialog_sound(1);
}

/*Fenster Minimieren*/
void CALLCONV menue_minimieren(void)
{
 printf("Minimieren\n");
 minimize_window();
}

static int CALLCONV clickable_bild(unsigned int x,unsigned int y)
{
 if(x<=dim_sound[0]||x>=dim_sound[0]+dim_sound[2]) return 0;
 if(y<=dim_sound[1]||y>=dim_sound[1]+dim_sound[3]) return 0;
 if(y>dim_sound[1]+dim_sound[3]-20&&x>dim_sound[0]+dim_sound[2]/2) return 1;
 if(y<dim_sound[1]+20&&x<dim_sound[0]+20) return 1;
 if(x<dim_sound[0]+52||x>dim_sound[0]+52+12) return 0;
 y-=dim_sound[1]+62;
 if(y>60+12) return 0;
 if(y%20<12) return 1;
 return 0;
}


extern int algo;

/*Dialog zur Bildeinstellung*/
void CALLCONV menue_bild(void)
{
 uint32_t inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;
 cf_alt=set_clickable_func(clickable_bild);
REDRAW:
 printf("draw_popup\n");
 draw_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
    get_string(STR_BILD),1,NULL,get_string(STR_CONT),0);
 draw_frame(dim_sound[0],dim_sound[1]+20,dim_sound[2],dim_sound[3]-40,1,0,0);
 draw_frame(dim_sound[0]+1,dim_sound[1]+21,dim_sound[2]-2,dim_sound[3]-42,2,FARBE_HELL,FARBE_DUNKEL);
 shadow_print(dim_sound[0]+40,dim_sound[1]+35,200,20,get_string(STR_SCALE),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+60,algo==1);
 shadow_print(dim_sound[0]+70,dim_sound[1]+60,200,20,get_string(STR_SCNO),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+80,algo==2);
 shadow_print(dim_sound[0]+70,dim_sound[1]+80,200,20,get_string(STR_SCNN),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+100,algo==3);
 shadow_print(dim_sound[0]+70,dim_sound[1]+100,200,20,get_string(STR_SCLI),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+120,algo==4);
 shadow_print(dim_sound[0]+70,dim_sound[1]+120,200,20,get_string(STR_SCHQ),0);
 printf("win_flush()\n");
 win_flush();
 inkey=0;
 while(inkey!=27&&inkey!=13)
 {
  inkey=win_input_get();
  if(mouse_in_field(inkey,dim_sound[0]+dim_sound[2]/2,dim_sound[1]+dim_sound[3]-20,
    dim_sound[0]+dim_sound[2],dim_sound[1]+dim_sound[3],0))
    {
     click_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
      get_string(STR_BILD),1,NULL,get_string(STR_CONT),3);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_sound[0],dim_sound[1],
    dim_sound[0]+20,dim_sound[1]+20,0))
    {
     click_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
      get_string(STR_BILD),1,NULL,get_string(STR_CONT),1);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_sound[0]+50,dim_sound[1]+60,
    dim_sound[0]+50+20,dim_sound[1]+60+20,0))
    {
     algo=1;
ALGO_ANDERS:
     printf("win_init(0)\n");
     win_init(0);
     printf("reset_bitmap_slots()\n");
     reset_bitmap_slots();
     printf("win_init(algo)\n");
     win_init(algo);
     printf("init_fonts()\n");
     init_fonts();
     printf("redraw_bg()\n");
     redraw_bg();
     goto REDRAW;
    }
  if(mouse_in_field(inkey,dim_sound[0]+50,dim_sound[1]+80,
    dim_sound[0]+50+20,dim_sound[1]+80+20,0))
    {
     algo=2;
     goto ALGO_ANDERS;                
    }
  if(mouse_in_field(inkey,dim_sound[0]+50,dim_sound[1]+100,
    dim_sound[0]+50+20,dim_sound[1]+100+20,0))
    {
     algo=3;
     goto ALGO_ANDERS;                
    }
  if(mouse_in_field(inkey,dim_sound[0]+50,dim_sound[1]+120,
    dim_sound[0]+50+20,dim_sound[1]+120+20,0))
    {
     algo=4;
     goto ALGO_ANDERS;                
    }
 }
 redraw_bg();
 set_clickable_func(cf_alt);
}

static int CALLCONV clickable_vmen(unsigned int x,unsigned int y)
{
 if(x<=dim_sound[0]||x>=dim_sound[0]+dim_sound[2]) return 0;
 if(y<=dim_sound[1]||y>=dim_sound[1]+dim_sound[3]) return 0;
 if(y>dim_sound[1]+dim_sound[3]-20&&x>dim_sound[0]+dim_sound[2]/2) return 1;
 if(y<dim_sound[1]+20&&x<dim_sound[0]+20) return 1;
 
/* if(x<dim_sound[0]+202||x>dim_sound[0]+282+12) return 0;*/
 x-=dim_sound[0]+202;
 if(x>80+12) return 0;
 if(x>12&&x<80) return 0;
 y-=dim_sound[1]+82;
 if(y%20<12) return 1;
 return 0;
}

static unsigned short vmen_txt[3]={STR_SCROLL,STR_AUTOCON,STR_FFIGHT};
static uint8_t *(vmen_ptr[3])={&(welt.scroll),&(welt.auto_weiter),&(welt.fast_fight)};

/*Dialog zur Geschwindigkeitseinstellung*/
void CALLCONV menue_v(void)
{
 int a;
 uint32_t inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;
 cf_alt=set_clickable_func(clickable_vmen);
REDRAW:
 draw_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
    get_string(STR_SETV),1,NULL,get_string(STR_CONT),0);
 draw_frame(dim_sound[0],dim_sound[1]+20,dim_sound[2],dim_sound[3]-40,1,0,0);
 draw_frame(dim_sound[0]+1,dim_sound[1]+21,dim_sound[2]-2,dim_sound[3]-42,2,FARBE_HELL,FARBE_DUNKEL);

 shadow_print(dim_sound[0]+180,dim_sound[1]+60,60,20,get_string(STR_ON),1);
 shadow_print(dim_sound[0]+260,dim_sound[1]+60,60,20,get_string(STR_OFF),1);
 for(a=0;a<3;a++)
 {
  shadow_print(dim_sound[0]+10,dim_sound[1]+80+20*a,200,20,get_string(vmen_txt[a]),0);
  checkbox(dim_sound[0]+200,dim_sound[1]+80+20*a,*(vmen_ptr[a]));
  checkbox(dim_sound[0]+280,dim_sound[1]+80+20*a,!(*(vmen_ptr[a])));
 }
/* shadow_print(dim_sound[0]+70,dim_sound[1]+60,200,20,get_string(STR_SCNO),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+80,algo==2);
 shadow_print(dim_sound[0]+70,dim_sound[1]+80,200,20,get_string(STR_SCNN),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+100,algo==3);
 shadow_print(dim_sound[0]+70,dim_sound[1]+100,200,20,get_string(STR_SCLI),0);
 checkbox(dim_sound[0]+50,dim_sound[1]+120,algo==4);
 shadow_print(dim_sound[0]+70,dim_sound[1]+120,200,20,get_string(STR_SCHQ),0);*/
 win_flush();
 inkey=0;
 while(inkey!=27&&inkey!=13)
 {
  inkey=win_input_get();
  if(mouse_in_field(inkey,dim_sound[0]+dim_sound[2]/2,dim_sound[1]+dim_sound[3]-20,
    dim_sound[0]+dim_sound[2],dim_sound[1]+dim_sound[3],0))
    {
     click_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
      get_string(STR_BILD),1,NULL,get_string(STR_CONT),3);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_sound[0],dim_sound[1],
    dim_sound[0]+20,dim_sound[1]+20,0))
    {
     click_popup(dim_sound[0],dim_sound[1],dim_sound[2],dim_sound[3],
      get_string(STR_BILD),1,NULL,get_string(STR_CONT),1);
     inkey=27;
    }
  for(a=0;a<3;a++)
  {
   if(mouse_in_field(inkey,dim_sound[0]+200,dim_sound[1]+80+20*a,
    dim_sound[0]+200+16,dim_sound[1]+80+20*a+16,0))
    {
     *(vmen_ptr[a])=1;
     goto REDRAW;                
    }
   if(mouse_in_field(inkey,dim_sound[0]+280,dim_sound[1]+80+20*a,
    dim_sound[0]+280+16,dim_sound[1]+80+20*a+16,0))
    {
     *(vmen_ptr[a])=0;
     goto REDRAW;                
    }
  }
 }
 redraw_bg();
 set_clickable_func(cf_alt);
}

static unsigned short dim_about[4]={320-220,240-180,440,380};
static char text_about[]="        MAGNETIC PLANET 32 (Version " 
ENGVERSION 
") for "
ENGTARGET
":\n\n"
"   Original Game:\nKIWI - Kooperation fuer Interaktive Werbung und \nInformation "
"(http://www.kiwi.de) (Karl L. von Wendt;\nLars Hammer;Stefan Beyer)\n\n"
"   Original Publisher:\n(C) 1996 TDK Magnetics Europe GmbH (as Freeware)\n\n"
"   Remake Game Engine:\n2019 Christian Klamt\n\n"
"   Reverse Engineering:\nMathias Bockwoldt (https://www.frunit.de/qpop);\nChristian Klamt\n\n"
"   Spanish Translation:\nAriel Plomer\n\n"
"   HQX-Scaling Library:\nMaxim Stepin;Cameron Zemek;Francois Gannaz"
" - under LGPL\n - Source Code comes with Game\n\n"
#ifdef __WIN32__
"   MIDI-Decoder for Windows:\nBased on Public Domain Code by David J. Rager\n\n"
#endif
;

static int CALLCONV clickable_about(unsigned int x,unsigned int y)
{
 return 0;
}

/*Dialog für Credits*/
void CALLCONV menue_about(void)
{
 int a,b;
 char *cptr,*cptr2;
 uint32_t inkey=0;
 int (*CALLCONV cf_alt)(unsigned int,unsigned int)=NULL;
 cf_alt=set_clickable_func(clickable_about);
REDRAW:
 draw_popup(dim_about[0],dim_about[1],dim_about[2],dim_about[3],
    get_string(STR_ABOUT),1,NULL,get_string(STR_CONT),0);
 draw_frame(dim_about[0],dim_about[1]+20,dim_about[2],dim_about[3]-40,1,0,0);
 draw_frame(dim_about[0]+1,dim_about[1]+21,dim_about[2]-2,dim_about[3]-42,2,FARBE_HELL,FARBE_DUNKEL);


 cptr=text_about;
 a=2;
 while(*cptr)
 {
  cptr2=cptr;
  while(*cptr2!=0&&*cptr2!='\n') cptr2++;
  b=*cptr2;
  *cptr2=0;
  shadow_print(dim_about[0]+4,dim_about[1]+a*12,dim_about[2]-8,18,cptr,0);
  *cptr2=b;
  cptr=cptr2;
  if(*cptr) cptr++;
  a++;
 }
 win_flush();
 inkey=0;
 while(inkey!=27&&inkey!=13)
 {
  inkey=win_input_get();
  if(mouse_in_field(inkey,dim_about[0]+dim_about[2]/2,dim_about[1]+dim_about[3]-20,   
     dim_about[0]+dim_about[2],dim_about[1]+dim_about[3],0))
    {
     click_popup(dim_about[0],dim_about[1],dim_about[2],dim_about[3],
      get_string(STR_ABOUT),1,NULL,get_string(STR_CONT),3);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_about[0],dim_about[1],
    dim_about[0]+20,dim_about[1]+20,0))
    {
     click_popup(dim_about[0],dim_about[1],dim_about[2],dim_about[3],
      get_string(STR_ABOUT),1,NULL,get_string(STR_CONT),1);
     inkey=27;
    }
 }
 redraw_bg();
 set_clickable_func(cf_alt);
}

static const unsigned short dim_sprache[]={160,120,320,240};

extern int language;
extern int codepage;

/*Sprache einstellen*/
void CALLCONV menue_sprache(void)
{
 unsigned int a,b;
 int x,y,dx,dy;
 int language_max;
 uint32_t f;
 char *lan,*code,*filename;
 uint32_t inkey=0;
REDRAW:
 draw_popup(dim_sprache[0],dim_sprache[1],dim_sprache[2],dim_sprache[3],
    get_string(STR_SETLANG),1,NULL,get_string(STR_CONT),0);
 draw_frame(dim_sprache[0],dim_sprache[1]+20,dim_sprache[2],dim_sprache[3]-40,1,0,0);
 draw_frame(dim_sprache[0]+1,dim_sprache[1]+21,dim_sprache[2]-2,dim_sprache[3]-42,2,FARBE_HELL,FARBE_DUNKEL);
 language_max=0;
 for(a=0;a<20;a++)
 {
  b=language_info(&code,&lan,NULL,a);
  if(!b) break;
  language_max=a+1;
  x=dim_sprache[0]+(dim_sprache[2]/2-1)*(a&1);
  dx=/*x+*/dim_sprache[2]/2+1;
  y=dim_sprache[1]+20+20*(a>>1);
  dy=/*y+*/21;
/*  f=(a==language)?0xFF0000:0x333333;
  fill_rectangles(1,&x,&y,&dx,&dy,&f);*/
  draw_frame(x,y,dx,dy,1,0,0);
  draw_frame(x+1,y+1,dx-2,dy-2,2,(a==language)?FARBE_DUNKEL:FARBE_HELL,(a==language)?FARBE_HELL:FARBE_DUNKEL);
  printf("%s %s\n",code,lan);
  set_codepage(b);
  shadow_print(x+3,y,dx,dy,code,0);
  shadow_print(x+11*(1+strlen(code)),y,dx,dy,lan,0);
 }
 win_flush();
 set_codepage(codepage);
 while(inkey!=27&&inkey!=13)
 {
  inkey=win_input_get();                
  if(mouse_in_field(inkey,dim_sprache[0]+dim_sprache[2]/2,dim_sprache[1]+dim_sprache[3]-20,
    dim_sprache[0]+dim_sprache[2],dim_sprache[1]+dim_sprache[3],0))
    {
     click_popup(dim_sprache[0],dim_sprache[1],dim_sprache[2],dim_sprache[3],
      get_string(STR_SETLANG),1,NULL,get_string(STR_CONT),3);
     inkey=27;
    }
  if(mouse_in_field(inkey,dim_sprache[0],dim_sprache[1],
    dim_sprache[0]+20,dim_sprache[1]+20,0))
    {
     click_popup(dim_sprache[0],dim_sprache[1],dim_sprache[2],dim_sprache[3],
      get_string(STR_SETLANG),1,NULL,get_string(STR_CONT),1);
     inkey=27;
    }
  for(a=0;a<language_max;a++)
   if(mouse_in_field(inkey,dim_sprache[0]+(a&1)*dim_sprache[2]/2,dim_sprache[1]+20*(1+a/2),
    dim_sprache[0]+(a&1)*dim_sprache[2]/2+dim_sprache[2]/2,dim_sprache[1]+20*(1+a/2)+20,0))
   {
    language=a;
    language_info(NULL,NULL,&filename,language);
    codepage=parse_language_file(filename,1);
    set_codepage(codepage);        
    printf("%d\n",a);    
    goto REDRAW;
   }
 }     
 redraw_bg();
}

/*static const char spiel_name[]="Magnetic Planet 32";*/
/*#define BALKEN_LEN 499*/
#define BALKEN_LEN 459
#define LANG_LEN 40
static const unsigned short knopf_info[8][8]=
{
 {0,0,20,20,BM_SCHLIESEN,5,8,0},     
 {519,0,20,20,/*BM_HILFE*/BM_SANDUHR,5,4,40},      
 {539,0,20,20,BM_SANDUHR,5,3,0},      
 {559,0,20,20,/*BM_BILDSCHIRM*/BM_SANDUHR,5,3,20},
 {579,0,20,20,BM_LAUTSPRECHER,5,4,0},      
 {599,0,20,20,BM_NOTE,5,4,0},      
 {619,0,20,20,BM_MINI,5,6,0}
};

static const unsigned short knopf_spr[4]=
{20+BALKEN_LEN,0,LANG_LEN,20};

void (*CALLCONV menu_func[7])(void)={menue_schliesen,menue_about,menue_v,menue_bild,menue_sound,menue_musik,menue_minimieren};

/*Hintergrund und oberen Balken zeichnen*/
void menue_init(void)
{
 int a;
 TEXTURE_BACKGROUND(0,0,640,480);
 /*texture_rectangle(0,0,640,480,get_bitmap_slot(BM_WINDOW_MAIN),0,0,8,480);*/
 button_text(knopf_info[0][0]+knopf_info[0][2],knopf_info[0][1],BALKEN_LEN,knopf_info[0][3],get_string(STR_APPNAME),0);
 button_text(knopf_spr[0],knopf_spr[1],knopf_spr[2],knopf_spr[3],get_string(STR_LANCODE),0);
 for(a=0;a<7;a++) 
  button_icon(knopf_info[a][0],knopf_info[a][1],knopf_info[a][2],knopf_info[a][3],
    get_bitmap_slot(knopf_info[a][4]),knopf_info[a][5],knopf_info[a][6],knopf_info[a][7],0);
 draw_frame(0,HUD_Y,640-1,460-1,2,FARBE_HELL,FARBE_DUNKEL);    
}

/*"Weiter"-Knopf einzeichnen*/
void weiter_knopf(int down)
{
 if(down)
 {
  button_text(HUD_X,480-22,640-HUD_X,22,get_string(STR_CONT),1);
  win_flush();
  clk_wait((CLK_TCK/8)|1); 
 }
 button_text(HUD_X,480-22,640-HUD_X,22,get_string(STR_CONT),0);     
 if(down)
  win_flush();
}

/*Eingabe verarbeiten wenn sie für das Menü ist (notzero, wenn ja)*/
volatile int process_menu_input(uint32_t k)
{
 int a;
 if(k==3) {menu_func[0]();return 1;}
 if(k==27) k=MOUSE_FLAG; /*Bei ESC linke obere Ecke*/
 /*Sprache*/
 if(mouse_in_field(k,knopf_spr[0],knopf_spr[1],knopf_spr[0]+knopf_spr[2],knopf_spr[1]+knopf_spr[3],0))
 {
  click_button_text(knopf_spr[0],knopf_spr[1],knopf_spr[2],knopf_spr[3],get_string(STR_LANCODE)); 
  while(win_input_ready())win_input_get();  
  menue_sprache();
  return 1;
 }
 /*Knöpfe*/
 for(a=0;a<7;a++)
 {
  if(k&MOUSE_FLAG)
  {
   if(!(k&MOUSE_UP))
   {
    if(mouse_in_field(k,knopf_info[a][0],knopf_info[a][1],
      knopf_info[a][0]+knopf_info[a][2],
      knopf_info[a][1]+knopf_info[a][3],0))
    {
     click_button_icon(knopf_info[a][0],knopf_info[a][1],knopf_info[a][2],knopf_info[a][3],
       get_bitmap_slot(knopf_info[a][4]),knopf_info[a][5],knopf_info[a][6],knopf_info[a][7]);
     while(win_input_ready())win_input_get();  
     menu_func[a]();
     return 1;
    }
   }
  }   
 }
 return 0;   
}

int menue_clickable(int x)
{
 /*0 Wenn auf Schriftzug*/
 if(x<knopf_info[0][0]+knopf_info[0][2]||x>knopf_info[0][0]+knopf_info[0][2]+BALKEN_LEN)
  return 1;
 return 0;
}

static volatile int game_key_ok=0;
static volatile uint32_t game_key;

/*win_input_ready ohne Menü*/
volatile int game_input_ready(void)
{
 if(game_key_ok) return 1;
 while(win_input_ready())
 {
  game_key=win_input_get();
  entropy_add(game_key);
  if(!process_menu_input(game_key))
  {
   game_key_ok=1;
   return 1;
  }  
 }
 return 0;        
}

/*win_input_get ohne Menü*/
volatile uint32_t game_input_get(void)
{
 if(game_key_ok)
 {
  game_key_ok=0;
  return game_key; 
 }
 do{
  game_key=win_input_get();
  entropy_add(game_key);
 }while(process_menu_input(game_key));
 return game_key;
}
