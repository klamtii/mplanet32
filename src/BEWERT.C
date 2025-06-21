#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "EDITION.H"
#include "SPIELDAT.H"
#include "SYSTEMIO.H"
#include "MPBITMAP.H"
#include "MPLANET.H"
#include "NSTRING.H"
#include "STRINGS.H"
#include "WELT.H"

/*Dateisignatur für Savegames*/
static char signatur[]=
#ifdef GAME_Q_POP
"Q-POP Savegame\0\0";
#endif
#ifdef GAME_MAGNETIC_PLANET
"Magnetic Planet Savegame\0\0";
#endif

extern int algo;

static char puffer[64];

/*Spiel Speichern*/
int save_game(int autosave)
{
 int a,b,c;
 const char *name;
 FILE *file;
 uint8_t byte[4];
 uint16_t word[2];
 uint32_t dword;

 name=get_filename(autosave?2:1);
 if(name==NULL) return 0;
 if(*name==0) return 0;
 printf("Speichere als %s\n",name);
 file=fopen(name,"wb");
 if(file==NULL) return 0;   
 printf("Öffnen OK\n",name);
 /*Signatur*/ 
 fwrite(signatur,1,strlen(signatur)+2,file);
 /*Musik*/
 byte[0]=!!get_midi_onoff();
 byte[1]=get_midi_volume();
 fwrite(byte,1,2,file);
 /*Geräusche*/
 byte[0]=!!get_wave_onoff();
 byte[1]=get_wave_volume();
 fwrite(byte,1,2,file);
 /*Spieler: Typ und IQ*/
 for(a=0;a<6;a++)
 {
  switch(spieler[a].evol.typ)
  {
   default: byte[0]=3;break;
   case SPIELERTYP_MENSCH: byte[0]=1;break;
   case SPIELERTYP_COMPUTER: byte[0]=2;break;
  }                
  byte[1]=spieler[a].evol.iq;
  fwrite(byte,1,2,file);  
 }
 /*Spieler Eigenschaften*/
 for(a=0;a<6;a++)
 {
  fwrite(&(spieler[a].evol.eigenschaften[EVO_ANGRIFF]),1,1,file);
  fwrite(&(spieler[a].evol.eigenschaften[EVO_VERTEID]),1,1,file);
  fwrite(&(spieler[a].evol.eigenschaften[EVO_VERMEHR]),1,1,file);
  fwrite(&(spieler[a].evol.eigenschaften[EVO_TARNUNG]),1,1,file);
  fwrite(&(spieler[a].evol.eigenschaften[EVO_V]),1,1,file);
  fwrite(&(spieler[a].evol.eigenschaften[EVO_SINN]),1,1,file);
  fwrite(&(spieler[a].evol.eigenschaften[EVO_INT]),1,1,file);
  fwrite(&(spieler[a].sammel.tote),1,1,file);
  byte[0]=0;
  for(b=0;b<6;b++)
   byte[0]+=spieler[a].sammel.revierkampf[b];
  fwrite(byte,1,1,file);
  byte[0]=spieler[a].sammel.nahrung/8;
  fwrite(byte,1,1,file);  
  fwrite(&(spieler[a].evol.einheiten),1,1,file);
  fwrite(&(spieler[a].evol.punkte_runde),1,1,file);  
  fwrite(&(spieler[a].sammel.kinder),1,1,file);  
  fwrite(&(spieler[a].sammel.bewegung),1,1,file);  
  fwrite(&(spieler[a].evol.punkte_gesamt),1,2,file);  
  fwrite(&(spieler[a].evol.eigenschaften[0]),1,6,file);  
  byte[0]=spieler[a].evol.einheiten?0:1;
  fwrite(byte,1,1,file);  
 } 
 /*Runde*/
 word[0]=welt.runde;
 fwrite(word,1,2,file); 
#ifdef GAME_Q_POP
 fwrite(&welt.runde_max,1,1,file);
 fwrite(&welt.rraumschiff,1,1,file); 
#endif
 word[0]=0;
 fwrite(word,1,2,file); /* 0 */
 byte[0]=qp_random(3); /* ? */
 fwrite(byte,1,1,file); 
 word[0]=welt.wasser;
 fwrite(word,1,2,file); 
 word[0]=welt.feuchtigkeit;
 fwrite(word,1,2,file); 
 word[0]=welt.temperatur;
 fwrite(word,1,2,file); 
 /*Tilemap*/
 for(a=0;a<28*28;a++)
 {
  b=welt_tilemap[a];
  if(b<47) byte[0]=0;
  else if(b==47) byte[0]=7;
  else if(b<54) byte[0]=b-48;
  else if(b==54) byte[0]=8;
  else if(b==55) byte[0]=9;
  else if(b==56) byte[0]=10;
  fwrite(byte,1,1,file); 
 }
 /* Relief */
 for(a=0;a<28;a++)
  for(b=0;b<28;b++)
  {
   fwrite(&(welt_relief[b*28+a]),1,1,file); 
  }
 /* Einheiten */
 fwrite(welt_einheiten,28,28,file);
 /* ungenutzte Karten ?*/
 for(a=0;a<(28*28*2)/4;a++)
 {
  switch(a) 
  {
   default: dword=0;break;         
    /*Speicherstände für Erweiterung*/
   /*case 0:  dword=0;break;*/
   case 1:  dword=0x01010101;break; /*Signatur*/
   case 2:  dword=algo;break; /*Samplingalgorithmus*/
  }                         
  fwrite(&dword,1,4,file);                            
 }
 /*Mutationswerte*/
 for(a=0;a<23;a++)
 {
  if(a<8||(a>15&&a<22)) byte[0]=10;
  else byte[0]=0;
  fwrite(byte,1,1,file);                 
 }
 /*??? Sprites*/ 
 dword=0;
 for
#ifdef GAME_Q_POP
 (a=0;a<9;a++)
#endif
#ifdef GAME_MAGNETIC_PLANET
 (a=0;a<8;a++)
#endif
  fwrite(&dword,1,4,file);

 /*Wer ist Tot ?*/
 b=0;
 for(a=0;a<6;a++)
 {
  byte[0]=spieler[a].evol.einheiten==0?1:0;
  fwrite(byte,1,1,file);                   
  b+=byte[0]; /*b=Anz. Tote */
 }
 
 /*Sonstiges*/
 byte[0]=0;
 byte[1]=welt.runde_max==255?1:0;
 byte[2]=b>4?1:0;
 byte[3]=welt.scroll; /*Scroll-Option*/
 fwrite(byte,1,4,file);                   

 fclose(file);
 return 1;
}

/*Spiel Laden*/
int load_game(void)
{
 int a,b,c;
 const char *name;
 FILE *file;
 uint8_t byte[4];
 uint16_t word[2];
 uint32_t dword;

 name=get_filename(0);
 if(name==NULL) return 0;
 if(*name==0) return 0;
 printf("Lade %s\n",name);
 file=fopen(name,"rb");
 if(file==NULL) return 0;   
 printf("Öffnen OK\n",name);

 /*Signatur*/ 
 a=fread(puffer,1,strlen(signatur)+2,file);
 if(a<strlen(signatur)||memcmp(signatur,puffer,strlen(signatur)+2))
 {
  printf("Signatur '%s' statt '%s'\n",puffer,signatur);
  fclose(file);
  return 0;
 }
 printf("Signatur OK\n",name);
 /*Musik*/
 fread(byte,1,2,file);
 set_midi_onoff(byte[0]);
 set_midi_volume(byte[1]);
 /*Geräusche*/
 fread(byte,1,2,file);
 byte[0]=get_wave_onoff();
 byte[1]=get_wave_volume();
 /*Spieler: Typ und IQ*/
 for(a=0;a<6;a++)
 {
  fread(byte,1,2,file);
  switch(byte[0])
  {
   default: spieler[a].evol.typ=0;break;
   case 1:spieler[a].evol.typ=SPIELERTYP_MENSCH;break;
   case 2:spieler[a].evol.typ=SPIELERTYP_COMPUTER;break;
  }                
  spieler[a].evol.iq=byte[1];
 }
 /*Spieler Eigenschaften*/
 for(a=0;a<6;a++)
 {
  fread(&(spieler[a].evol.eigenschaften[EVO_ANGRIFF]),1,1,file);
  fread(&(spieler[a].evol.eigenschaften[EVO_VERTEID]),1,1,file);
  fread(&(spieler[a].evol.eigenschaften[EVO_VERMEHR]),1,1,file);
  fread(&(spieler[a].evol.eigenschaften[EVO_TARNUNG]),1,1,file);
  fread(&(spieler[a].evol.eigenschaften[EVO_V]),1,1,file);
  fread(&(spieler[a].evol.eigenschaften[EVO_SINN]),1,1,file);
  fread(&(spieler[a].evol.eigenschaften[EVO_INT]),1,1,file);
  fread(&(spieler[a].sammel.tote),1,1,file);
  for(b=0;b<6;b++)
   spieler[a].sammel.revierkampf[b]=0;
  fread(&(spieler[a].sammel.revierkampf[a]),1,1,file);
  fread(byte,1,1,file);  
  spieler[a].sammel.nahrung=byte[0]*8;
  fread(&(spieler[a].evol.einheiten),1,1,file);
  fread(&(spieler[a].evol.punkte_runde),1,1,file);  
  fread(&(spieler[a].sammel.kinder),1,1,file);  
  fread(&(spieler[a].sammel.bewegung),1,1,file);  
  fread(&(spieler[a].evol.punkte_gesamt),1,2,file);  
  fread(&(spieler[a].evol.eigenschaften[0]),1,6,file);  
  fread(byte,1,1,file);  /*tot ? */
 } 
 /*Runde*/
 fread(word,1,2,file); 
 welt.runde=word[0];
#ifdef GAME_Q_POP
 fread(&welt.runde_max,1,1,file);
 fread(&welt.rraumschiff,1,1,file); 
#endif
 fread(word,1,2,file);  /*0 */
 fread(byte,1,1,file);  /* ? */

 fread(word,1,2,file);
 welt.wasser=word[0]; 
 fread(word,1,2,file);
 welt.feuchtigkeit=word[0]; 
 fread(word,1,2,file);
 welt.temperatur=word[0]; 
 /*Tilemap*/
 for(a=0;a<28;a++)
  for(b=0;b<28;b++)
  {
   fread(byte,1,1,file); 
   if(byte[0]==9) welt_relief[b*28+a]=101;
   else if(byte[0]==10) welt_relief[b*28+a]=102;
   else welt_relief[b*28+a]=0;
  }
 /* Relief */
 for(a=0;a<28;a++)
  for(b=0;b<28;b++)
  {
   fread(byte,1,1,file); 
   if(welt_relief[b*28+a]<=100)
    welt_relief[b*28+a]=byte[0];
  }
 /* Einheiten */
 fread(welt_einheiten,28,28,file);
 welt_make_tilemap();

 /* ungenutzte Karten ?*/
 b=0;
 for(a=0;a<(28*28*2)/4;a++)
 {
  fread(&dword,1,4,file);
  if(a==0&&dword==0) b++;
  if(a==1&&dword==0x01010101) b++;
  if(b==2){
  switch(a) 
  {
    /*Speicherstände für Erweiterung*/
   /*case 0:  dword=0;break;*/
   case 2:
    if(algo!=dword) /*Samplingalgorithmus*/
    {
     win_init(0);
     algo=dword;
     win_init(algo);               
    }
    break; 
  }
  }                         
 }
 /*Mutationswerte*/
 for(a=0;a<23;a++)
  fread(byte,1,1,file);                 

 /*??? Sprites*/ 
 for
#ifdef GAME_Q_POP
 (a=0;a<9;a++)
#endif
#ifdef GAME_MAGNETIC_PLANET
 (a=0;a<8;a++)
#endif
  fread(&dword,1,4,file);

 /*Wer ist Tot ?*/
 for(a=0;a<6;a++)
 {
/*  byte[0]=spieler[a].evol.einheiten==0?1:0;*/
  fread(byte,1,1,file);
 }

 /*Sonstiges*/
 fread(byte,1,4,file);                   
/* byte[0]=0;
 byte[1]=welt.runde_max==255?1:0;
 byte[2]=b>4?1:0;
 byte[3]=0;*/ /*Scroll-Option*/
 welt.scroll=byte[3];
 
 if(byte[1]) welt.runde_max=255;

 fclose(file);        
 return 1;
}

static int bew_letzte;

static int bew_x[6]={340,240,440,140,540,40};
static int bew_y[6]={107,146,170,193,219,242};

static void CALLCONV redraw_func( void*p)
{
 int a=(int)p,b,c;
 int xdst[6],ydst[6],dx[6],dy[6],xsrc[6],ysrc[6];
 char puffer[32];
 static const char proz_d[]="%d";
 menue_init();
 weiter_knopf(a&1);
 if(!bew_letzte)
 {
  button_text(0,480-22,HUD_X/2,22,get_string(STR_LOAD),a&2);
  button_text(HUD_X/2,480-22,HUD_X-HUD_X/2,22,get_string(STR_SAVE),a&4);
 }
 draw_frame(0,20,638,437,2,FARBE_HELL,FARBE_DUNKEL);
 /*Podest*/
 xdst[0]=30;ydst[0]=170;
 dx[0]=582; dy[0]=285;
 ysrc[0]=xsrc[0]=0;
 draw_bitmaps(1,get_bitmap_slot(BM_BEWERT_DAT),xdst,ydst,dx,dy,xsrc,ysrc);
 b=0;
 /*Roboter groß*/
 for(a=0;a<6;a++)
 {
  dy[a]=dx[a]=64;
  ysrc[a]=0;
  xsrc[spieler[a].evol.rang]=64*a;
  if(spieler[a].evol.rang+1>b&&spieler[a].evol.einheiten>0) b=spieler[a].evol.rang+1;
 }
 draw_bitmaps(b,get_bitmap_slot(BM_STAND_DAT),bew_x,bew_y,dx,dy,xsrc,ysrc);
 /*Maske für Roboter*/
 for(a=0;a<6;a++)
 {
  ydst[a]=373;
  dy[a]=dx[a]=16;
  ysrc[a]=16;
  xsrc[spieler[a].evol.rang]=272+16+32*a;                 
 }
 draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),bew_x,ydst,dx,dy,xsrc,ysrc,-1);
 /*Bitmap für Roboter*/
 for(a=0;a<6;a++)
  xsrc[a]-=16;
 draw_bitmaps_x(b,get_bitmap_slot(BM_MEER_DAT),bew_x,ydst,dx,dy,xsrc,ysrc,1);
 /* DNS-Strang */
 for(a=0;a<6;a++)
 {
  ydst[a]+=24;
  ysrc[a]=xsrc[a]=0;
 }
 draw_bitmaps(b,get_bitmap_slot(BM_DNS),bew_x,ydst,dx,dy,xsrc,ysrc);
 /* Kranz */
 for(a=0;a<6;a++)
 {
  ydst[a]+=24;
  ysrc[a]=xsrc[a]=0;
 }
 draw_bitmaps(b,get_bitmap_slot(BM_KRANZ),bew_x,ydst,dx,dy,xsrc,ysrc);
 for(a=0;a<6;a++)
 {
  if(spieler[a].evol.einheiten>0)
  {
   c=spieler[a].evol.rang;
   sprintf(puffer,proz_d,spieler[a].evol.einheiten);
   write_text(FONT_NORMAL,bew_x[c]+32,373,50,20,puffer,0xFFFFFF,0);
   write_text(FONT_NORMAL,bew_x[c]+32+1,373+1,50,20,puffer,0,0);
   sprintf(puffer,proz_d,spieler[a].evol.punkte_runde);
   write_text(FONT_NORMAL,bew_x[c]+32,373+24,50,20,puffer,0xFFFFFF,0);
   write_text(FONT_NORMAL,bew_x[c]+32+1,373+24+1,50,20,puffer,0,0);
   sprintf(puffer,proz_d,spieler[a].evol.punkte_gesamt);
   write_text(FONT_NORMAL,bew_x[c]+32,373+48,50,20,puffer,0xFFFFFF,0);
   write_text(FONT_NORMAL,bew_x[c]+32+1,373+48+1,50,20,puffer,0,0);
  } 
 }
 /*Überschrift*/
 sprintf(puffer,"%s %d",get_string(STR_ROUND),welt.runde);
 write_text(FONT_GROSS,20,30,600,40,puffer,0x303030,1);

}

static int CALLCONV clickable_bew(unsigned int x,unsigned int y)
{
 if(y<21) return menue_clickable(x);
 if(y>480-20)
 {
  if(!bew_letzte) return 1;
  if(x>HUD_X) return 1;
  else return 0;
 }
 return 0; 
}



/*Bewertungsrunde*/
void bewertungsrunde(int l)
{
 uint32_t inkey=0;
 
 bew_letzte=l;
 
 welt_statistik();
 welt_punkte_verteilen();
 if(!l)
  save_game(1);

 set_redraw_func(redraw_func,(void*)0);
 set_clickable_func(clickable_bew);
 menue_init();
 redraw_func((void*)0);    
 win_flush();
 
 while(inkey!=13)
 {
  inkey=game_input_get();
  if(mouse_in_field(inkey,64*7+8,26+437,640,480,0)) 
  {
   /*redraw_func((void*)1)*/
   weiter_knopf(1);
   inkey=13;
  }
  if(!l)
  {
   if(mouse_in_field(inkey,32*7+8,26+437,64*7+7,480,0)) 
   {
    redraw_func((void*)4);
    clk_wait(CLK_TCK/10);
    save_game(0);
   }
   if(mouse_in_field(inkey,8,26+437,32*7+7,480,0)) 
   {
    redraw_func((void*)2);
    clk_wait(CLK_TCK/10);
    load_game();
    welt_statistik();
    welt_punkte_verteilen();
    redraw_func(NULL);
    win_flush();
   }
  }
 }     
}
