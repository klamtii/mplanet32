/*Ursprüngliche Main-Funktion*/
#if 0
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "MULTIPLEX.H"
#include "SYSTEMIO.H"
#include "RANDOM.H"
#include "BMCACHE.H"
#include "MPBITMAP.H"
#include "WELT.H"
#include "SPIELDAT.H"
#include "SAMMEL.H"
#include "GUI.H"
#include "EVOL.H"
#include "BEWERT.H"


int algo=4;

const char algo_name[5][50]=
{
 "kein Fenster",
 "keine Vergrößerung",
 "Nearest Neighbour Interpolation",
 "Bilineare Interpolation",
 "HQX Interpolation"
};

char textpuffer[64];

void relief_x(void)
{
 int a,b;
 for(a=0;a<6;a++)
 {
  relief_erzeugen(1);    
  for(b=0;b<6;b++)
  {   
   dump_relief(64*7+6+b*28,6+a*28);
   relief_erzeugen(0);    
  }  
 }   
}


int main(int argc,char **argv)
{
 char *bitmap1=NULL,*bitmap2=NULL,*bitmap3=NULL;
 char *midi=NULL;
 char *wave=NULL;
 uint32_t midi_lens[16],wave_lens[16];
 void *midi_ptrs[16],*wave_ptrs[16];
 uint32_t len=0;
 int a,b,c,num_songs=0;
 clock_t t1=0,t2=0;
 win_init(algo);

 if(load_magnetic_planet()) 
 {
  win_input_get();
  win_init(0);
  return 1;
 }
 set_midi_volume(50);
 play_song(0);
 yield_thread();
 menue_init();
 welt.wasser=20;
 welt.temperatur=50;
 welt.feuchtigkeit=50;
 qp_randomize();
 relief_erzeugen(1);
 welt_make_tilemap();
/*NOCHMAL:
 relief_erzeugen(1);
 welt_bildaufbau(1);
 welt_stat_info(0);
 win_flush();
 if(win_input_get()==13) goto NOCHMAL;*/
 memset(spieler,0,sizeof(spieler));
 for(len=0;len<6;len++)
 {
  spieler[len].sammel.kinder=10;
  spieler[len].sammel.bewegung=5;
  spieler[len].evol.iq=2;
  spieler[len].evol.typ=SPIELERTYP_COMPUTER;
  for(a=0;a<13;a++)
   spieler[len].evol.eigenschaften[a]=10;
 }
 spieler[0].evol.typ=SPIELERTYP_MENSCH;
 ausbreitungsrunde(0);
 for(len=1;len<6;len++)
  ausbreitungsrunde_ki(len);
 game_input_get();
 for(len=0;len<KAT_NUM;len++)
 {
  katastrophe_bild(len);
  katastrophe(len);
  game_input_get();  
 }
 welt_punkte_verteilen();
 bewertungsrunde();
 a=0;b=500;
/*draw_bitmaps(1,get_bitmap_slot(BM_TILESET_SURVIVAL),&a,&a,&b,&b,&a,&a);
 draw_bitmaps(1,get_bitmap_slot(BM_TOT_X),&a,&a,&b,&b,&a,&a);
 win_flush();
 win_input_get();*/
 printf("Einheiten %d\n",spieler[0].evol.einheiten);
/* spieler[0].evol.einheiten=50;
 spieler[0].stell.nachbarn[3]=50;
 spieler[0].stell.nachbarn[0]=300;
 spieler[0].stell.nachbarn[1]=50;
 spieler[0].stell.pflanzen[0]=5;
 spieler[0].stell.pflanzen[1]=45;
 spieler[0].stell.leer=0;
 spieler[0].stell.gebirge=30;
 spieler[0].stell.wasser=3;*/
 spieler[0].evol.punkte_runde=100;
 evolutionsrunde(0);
 sammelrunde_vorbereiten(0,&(spieler[0]),0);
/* relief_x();*/
 
 win_flush();
 a=0;c=0; len=0;

 sammelrunde(0);

 if(midi!=NULL)
 {
  unregister_song(1);
  free(midi);
 }

 win_init(0);
 if(bitmap1!=NULL) free(bitmap1);
 if(bitmap2!=NULL) free(bitmap2);
 if(bitmap3!=NULL) free(bitmap3); 
 return 0;   
}

#if 0
 {
  FILE *file=NULL;
  file=fopen("palette.txt","wt");
  if(file!=NULL)
  {
   uint32_t *dptr=(uint32_t*)(bitmap1+14+40);
   for(a=0;a<256;a++)
   {
    fprintf(file,"0x%X,",(int)*(dptr++));
    if((a&7)==7) fprintf(file,"\n");
   }
   
   fclose(file);              
  }
 }
#endif
#endif
