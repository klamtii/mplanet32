#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <mem.h>
#include <time.h>
#include <stdio.h>

#include "AUSWAHL.H"
#include "EDITION.H"
#include "SYSTEMIO.H"
#include "SPIELDAT.H"
#include "BMCACHE.H"
#include "WELT.H"
#include "MENUE.H"
#include "NSTRING.H"
#include "STRINGS.H"
#ifdef GAME_MAGNETIC_PLANET
#include "MPLANET.H"
#include "MPBITMAP.H"
#endif

static char textpuffer[16];

static const unsigned short ev_strings[13]=
{
 STR_PLANT1,STR_PLANT2,STR_PLANT3,STR_PLANT4,STR_PLANT5,STR_PLANT6,
 STR_FERTIL,STR_ATTACK,STR_DEFEND,STR_CAMOU,STR_SPEED,STR_SCAN,STR_INT
};

static void eigenschaft_zeichnen(struct SPIELER *sp_str,int n,int down)
{
 int a=10-(sp_str->evol.eigenschaften[n]/10);
 if(a<1) a=1;
 if(a>3) a--;
 scrollbar(SPIELFELD_X+250,SPIELFELD_Y+91+26*n,300,sp_str->evol.eigenschaften[n]*3,a);
 shadow_print(SPIELFELD_X+15,SPIELFELD_Y+86+26*n,205,20,get_string(ev_strings[n]),0);
 sprintf(textpuffer,"%3d%%",sp_str->evol.eigenschaften[n]);
 shadow_print(SPIELFELD_X+575,SPIELFELD_Y+86+26*n,40,20,textpuffer,0);
 {
  int b,c,d,e;
  uint32_t f;
  a=SPIELFELD_X+200;
  b=SPIELFELD_Y+86+26*n;
  c=24;
  d=n*24;
  e=96;                                                                                    
  draw_bitmaps(1,get_bitmap_slot(BM_ENTWICKL_DAT),&a,&b,&c,&c,&d,&e);
  if(n<6) /*Anzeige der Pflanzen*/
  {
   if(sp_str->stell.pflanzen[n])
   {
    a+=27;
    d=b+21;
    c=a+4;
    b=d-(sp_str->stell.pflanzen[n]*20)/sp_str->evol.einheiten;
    f=0;
    fill_rectangles(1,&a,&b,&c,&d,&f);
    a--;b--;c--;d--;
    f=0xFFFFFF;
    fill_rectangles(1,&a,&b,&c,&d,&f);   
   }
  }
 }
 if(down==1) click_button_plusminus(SPIELFELD_X+235,SPIELFELD_Y+91+26*n,20,20,-1);       
 else button_plusminus(SPIELFELD_X+235,SPIELFELD_Y+91+26*n,20,20,-1,0);       
 if(down==2) click_button_plusminus(SPIELFELD_X+549,SPIELFELD_Y+91+26*n,20,20,1);       
 else button_plusminus(SPIELFELD_X+549,SPIELFELD_Y+91+26*n,20,20,1,0);
 weiter_knopf(0);                                                                                   
}

static void CALLCONV redraw_func( void*p)
{
 int a,b,c,d;
 struct SPIELER *sp_str;
 sp_str=spieler+((int)p);
 draw_frame(SPIELFELD_X-1,SPIELFELD_Y-1,640-SPIELFELD_X-2,75,1,0,0);
 draw_frame(SPIELFELD_X,SPIELFELD_Y,639-SPIELFELD_X-1,75-2,2,FARBE_HELL,FARBE_DUNKEL);
 draw_frame(SPIELFELD_X,SPIELFELD_Y+75,639-SPIELFELD_X-1,480-(SPIELFELD_Y+75)-2,2,FARBE_HELL,FARBE_DUNKEL);
 a=SPIELFELD_X+24; b=SPIELFELD_Y+4;
 c=64;d=0;
 draw_bitmaps(1,get_bitmap_slot(BM_SPECIES1+(int)p),&a,&b,&c,&c,&d,&d);
 shadow_print(SPIELFELD_X+90,SPIELFELD_Y+35,160,20,get_string(STR_EVOL),0);
 scrollbar(SPIELFELD_X+250,SPIELFELD_Y+40,300,sp_str->evol.punkte_runde*3,0);
 sprintf(textpuffer,"%3d",sp_str->evol.punkte_runde);
 shadow_print(SPIELFELD_X+575,SPIELFELD_Y+35,30,20,textpuffer,0);
 for(a=0;a<13;a++)
  eigenschaft_zeichnen(sp_str,a,0);
}

static int CALLCONV clickable_func(unsigned int x,unsigned int y)
{
 if(y<21) return menue_clickable(x);
 if(y>480-20&&x>HUD_X) return 1;
 if(x<SPIELFELD_X+235||x>SPIELFELD_X+549+16) return 0;
 if(x<SPIELFELD_X+549-100&&x>SPIELFELD_X+235+16+100) return 0;
 y-=SPIELFELD_Y+91;
 if(y>26*13) return 0;
 if(y%26<16) return 1;
 return 0;
}

/*Evolutionsrunde*/
int evolutionsrunde(int spieler_num)
{
 int a,b,c;
 struct SPIELER *sp_str;
 uint32_t inkey=0;
 uint8_t eigenschaften_alt[13];
 if(spieler_num<0||spieler_num>=6) return 0;
 sp_str=spieler+spieler_num;
 set_redraw_func(redraw_func,(void*)spieler_num);
 menue_init();
 redraw_func((void*)spieler_num);
 win_flush();
 set_clickable_func(clickable_func);
 memcpy(eigenschaften_alt,sp_str->evol.eigenschaften,13);
 while(inkey!=13)
 {
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,25+437,640,480,0)) 
  {
   weiter_knopf(1);
   inkey=13;
  }                   
  if(inkey==13&&sp_str->evol.punkte_runde)
  {
   if(!msgbox_janein(get_string(STR_CONF))) inkey=0;                                                 
  }  
  for(a=0;a<13;a++)
  {
   if(mouse_in_field(inkey,SPIELFELD_X+235,SPIELFELD_Y+91+26*a,
      SPIELFELD_X+235+16,SPIELFELD_Y+91+26*a+16,0))
    {
     if(sp_str->evol.eigenschaften[a]>eigenschaften_alt[a])
     {
      sp_str->evol.eigenschaften[a]--;
      sp_str->evol.punkte_runde++;
      eigenschaft_zeichnen(sp_str,a,1);
      menue_init();
      redraw_func((void*)spieler_num);
      win_flush();
     }
    }
   if(mouse_in_field(inkey,SPIELFELD_X+549,SPIELFELD_Y+91+26*a,
      SPIELFELD_X+549+16,SPIELFELD_Y+91+26*a+16,0))
    {
     if(sp_str->evol.eigenschaften[a]<100&&sp_str->evol.punkte_runde>0)
     {
      sp_str->evol.eigenschaften[a]++;
      sp_str->evol.punkte_runde--;
      eigenschaft_zeichnen(sp_str,a,2);
      menue_init();
      redraw_func((void*)spieler_num);
      win_flush();
     }    
    }
   if(mouse_in_field(inkey,SPIELFELD_X+235+16,SPIELFELD_Y+91+26*a,
      SPIELFELD_X+235+16+100,SPIELFELD_Y+91+26*a+16,0))
    {
     if(sp_str->evol.eigenschaften[a]>eigenschaften_alt[a])
     {
      b=10;
      if(sp_str->evol.eigenschaften[a]-eigenschaften_alt[a]<b)
       b=sp_str->evol.eigenschaften[a]-eigenschaften_alt[a];
      sp_str->evol.eigenschaften[a]-=b;
      sp_str->evol.punkte_runde+=b;
      eigenschaft_zeichnen(sp_str,a,1);
      menue_init();
      redraw_func((void*)spieler_num);
      win_flush();
     }
    }
   if(mouse_in_field(inkey,SPIELFELD_X+549-100,SPIELFELD_Y+91+26*a,
      SPIELFELD_X+549,SPIELFELD_Y+91+26*a+16,0))
    {
     if(sp_str->evol.eigenschaften[a]<100&&sp_str->evol.punkte_runde>0)
     {
      b=10;
      if(100-sp_str->evol.eigenschaften[a]<b)
       b=100-sp_str->evol.eigenschaften[a];
      if(sp_str->evol.punkte_runde<b)
       b=sp_str->evol.punkte_runde;
      sp_str->evol.eigenschaften[a]+=b;
      sp_str->evol.punkte_runde-=b;
      eigenschaft_zeichnen(sp_str,a,2);
      menue_init();
      redraw_func((void*)spieler_num);
      win_flush();
     }    
    }
  }
  
 }
 return 1;    
}

#if 0
/*Priorität von Pflanzen in Abhängigkeit der besetzten Pflanzen
  aktuelle Pflanzen sind am Wichtigsten, aber Pflanzen, die
  durch Klimaveränderungen entstehen können sind auch wichtig*/
static const uint8_t klima_zu_pflanze[6][6]=
{
 {5,1,1,0,0,1},
 {1,5,0,1,0,0},
 {1,0,5,0,1,0},
 {0,1,0,5,0,1},
 {0,0,1,0,5,1},
 {1,0,0,1,1,5}
};


int evolutionsrunde_ki(int spieler_num)
{
 auto int evol_prop[13];
 auto long evol_soll[13];
 long a,b,c;
 struct SPIELER *sp_str;
 if(spieler_num<0||spieler_num>=6) return 0;
 sp_str=spieler+spieler_num;
 memset(evol_prop,0,sizeof(evol_prop));
 memset(evol_soll,0,sizeof(evol_soll));
 qp_randomize();
 menue_init();
 /*Anpassung an Pflanzen, wenn man darauf steht, oder
  die Pflanzen durch Klimaveränderungen entstehen könnten*/
 for(a=0;a<6;a++)
  for(b=0;b<6;b++)
   evol_soll[b]+=(int)klima_zu_pflanze[a][b]*(int)sp_str->stell.pflanzen[a];   
 for(a=0;a<6;a++)
   evol_soll[a]/=4;
 /*Vermehrung, wenn Nachbarschaft zu sich selbst niedrig*/
 evol_soll[EVO_VERMEHR]=(sp_str->evol.einheiten*8-sp_str->stell.nachbarn[spieler_num])/8;
 if(evol_soll[EVO_VERMEHR]<10) evol_soll[EVO_VERMEHR]=10;
 b=0;
 for(a=0;a<6;a++)
 {
  if(a==spieler_num) continue;
  b+=sp_str->stell.nachbarn[a];
 }
 /*Angriff, wenn man Gegner als Nachbarn hat*/
 evol_soll[EVO_ANGRIFF]=b/8;
 /*Verteidigung, wenn man Gegner als Nachbarn hat, oder eine hohe Bevölkerung
  (Raubtiere !) hat*/
 a=(sp_str->evol.einheiten*sp_str->evol.einheiten)/50;
 evol_soll[EVO_VERTEID]=(b+a)/9;
 /*Inteligenz, wenn man Gegner als Nachbarn hat, oder eine hohe Bevölkerung
  (Raubtiere !) hat (mehr Gewicht auf Raubtiere)*/
 evol_soll[EVO_INT]=(b+a*8)/16;
 /*Tarnung nur gegen Raubtiere*/
 evol_soll[EVO_TARNUNG]=a;
 /*Geschwindigkeit bei schlechter Anpassung an Pflanzen*/
 c=b=0; /*Beste Anpassung*/
 for(a=0;a<6;a++)
  if(sp_str->evol.eigenschaften[a]>b) b=sp_str->evol.eigenschaften[a];
 if(b<0) b=1;
 c=0;
 for(a=0;a<6;a++)
 {
  printf("%d %d %d\n",sp_str->stell.pflanzen[a],sp_str->evol.eigenschaften[a],c);
  c+=sp_str->stell.pflanzen[a]*sp_str->evol.eigenschaften[a]; 
 }
 evol_soll[EVO_V]=sp_str->evol.einheiten-(c/b);
 printf("b=%d c=%d %d\n",b,c,(int)evol_soll[EVO_V]);
 if(evol_soll[EVO_V]<0) evol_soll[EVO_V]=0;
 /*Sinnesorgane ? kein Ahnung ...*/
 evol_soll[EVO_SINN]=qp_random(sp_str->evol.einheiten);
 if(sp_str->evol.einheiten<1) return 0;
 for(a=0;a<13;a++)
 {
  evol_soll[a]=(evol_soll[a]*100)/sp_str->evol.einheiten;
  printf("%d ",(int)evol_soll[a]);
#ifdef GAME_MAGNETIC_PLANET
  if(evol_soll[a]<10) evol_soll[a]=10;
#else
  if(evol_soll[a]<15) evol_soll[a]=15;
#endif
  if(evol_soll[a]>100) evol_soll[a]=100;  
 }
 for(a=0;a<13;a++)
 {
  evol_prop[a]=evol_soll[a]-sp_str->evol.eigenschaften[a];
  if(evol_prop[a]<0) evol_prop[a]=0;
  if(sp_str->evol.eigenschaften[a]<100)
  {
   if(evol_prop[a]<1) evol_prop[a]=1;
  }
  else evol_prop[a]=0;
 } 
 if(auswahl_vorbereiten(evol_prop,13))
 {
  if(sp_str->evol.punkte_runde<1) return;
  printf("%d\n",sp_str->evol.punkte_runde);
  while(sp_str->evol.punkte_runde>0)
  {
   a=auswahl_treffen(evol_prop,13);
   if(sp_str->evol.eigenschaften[a]<100)
    sp_str->evol.eigenschaften[a]++;
   sp_str->evol.punkte_runde--;                                         
  }                                     
 }
 return 1; 
}
#endif

/*Evolutionsrunde für Computergegner
 (wird nicht angezeigt)*/
int evolutionsrunde_ki(int spieler_num)
{
 long a,b,c,n=0;
 int evol_prop[13];
 struct SPIELER *sp_str;
 if(spieler_num<0||spieler_num>=6) return 0;
 sp_str=spieler+spieler_num;
 if(sp_str->evol.einheiten<1) return 0;
 qp_randomize();
 while(sp_str->evol.punkte_runde>0)
 {
  if(n++>1000) break;
  memset(evol_prop,0,sizeof(evol_prop));
  sp_str->evol.punkte_runde--;
  for(b=0,a=0;b<6;b++)
  {  
   a+=sp_str->evol.eigenschaften[b];
  }
  for(b=6,c=0;b<13;b++)
  {  
   c+=sp_str->evol.eigenschaften[b];
  }  
  if((qp_random(100)<60||c>=700)&&a<600)
  {
   for(b=0;b<6;b++)
   {
    if(sp_str->evol.eigenschaften[b]<100)
     evol_prop[b]=sp_str->stell.pflanzen[b]+1;    
   }
  }
  else
  {
   for(b=6;b<13;b++)
   {  
    if(sp_str->evol.eigenschaften[b]<100)
     evol_prop[b]=10+qp_random(5);    
   }  
  }
  if(auswahl_vorbereiten(evol_prop,13))
  {
   a=auswahl_treffen(evol_prop,13);
   if(sp_str->evol.eigenschaften[a]<100)
    sp_str->evol.eigenschaften[a]++;
  }                                         
 }
 return 1;
}
