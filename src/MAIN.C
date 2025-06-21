#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "EDITION.H"


#ifndef FEATURE_DEBUG
#include <windows.h>
#endif

#include "SPIELDAT.H"
#include "WELT.H"
#include "SAMMEL.H"
#include "EVOL.H"
#include "BEWERT.H"
#include "SYSTEMIO.H"
#include "MENUE.H"
#include "RANDOM.H"
#include "STRINGS.H"
#include "NEWGAME.H"
#include "VIDEO.H"

#include "hqx.h"


#ifdef GAME_Q_POP
#endif
#ifdef GAME_MAGNETIC_PLANET
#include "MPLANET.H"
#endif

#ifdef FEATURE_HQX
int cmd_hqx(int typ,volatile int argc, char **argv,volatile int argn)
{
 int r=0;
 int a,b,c,d;
 char *input_bmp,*output_bmp;
 uint32_t *in_buffer,*out_buffer,*dptr1,*dptr2;
 const char *in_name;
 char *out_name;
 uint32_t in_len=0,out_len=0;
 uint8_t *bptr;
 struct DIB_HEADER *in_hdr,*out_hdr;   
 static const char out_of_ram[]="Nicht genug Arbeitsspeicher !\n";
 FILE *out_handle=NULL;
 b=0;
 if(argn+1>=argc) b=1; 
 /*else if(argv[argn+1][0]=='-') b=2;*/ /* nächster ein Befehl ?*/
 if(b){printf("Dateiname für -hq%cx fehlt !\n",'0'+typ);return r;}
 r++;
 in_name=argv[argn+1];
 b=0;
 if(argn+2>=argc) b=1; 
 else if(argv[argn+2][0]=='-') b=2; /* nächster ein Befehl ?*/
 if(b)
 {
  out_name=malloc(strlen(argv[argn+1])+20);
  if(out_name!=NULL)
   sprintf(out_name,"hq%cx_%s",typ+'0',in_name);
 }
 else {out_name=strdup(argv[argn+2]);r++;}
 if(out_name==NULL)
  printf(out_of_ram);
 else
 {
  input_bmp=load_file_stdio(&in_len,in_name);
  if(input_bmp==NULL)  
   printf("Kann %s nicht laden!\n",in_name);
  else
  {
   if(in_len<54)
    printf("%s ist zu kurz (%d byte)\n",in_name,in_len);
   else if(input_bmp[0]!='B'||input_bmp[14]<40)
    printf("%s ist keine BMP-Datei\n",in_name);
   else
   {
    in_hdr=(void*)&(input_bmp[14]);
    if(in_hdr->planes!=1||(in_hdr->bitcount!=4&&in_hdr->bitcount!=8&&in_hdr->bitcount!=24))
     printf("Nur Farbtiefen 4, 8 und 24 bit unterstützt (keine Ebenen)\n");
    else if(in_hdr->compression)
     printf("Kompression %d nicht unterstützt\n",in_hdr->compression);
    else
    {
     a=(in_hdr->width*in_hdr->bitcount+31)/32;
     in_hdr->width=(a*32)/in_hdr->bitcount;
     in_buffer=malloc((abs(in_hdr->width)+1)*(abs(in_hdr->height)+1)*4+256);
     if(in_buffer==NULL)
      printf(out_of_ram);
     else
     {
      dptr1=(uint32_t*)&(input_bmp[14+in_hdr->size]); /*Palette*/
      dptr2=in_buffer; /*Puffer*/
      bptr=&(input_bmp[*((uint32_t*)(input_bmp+10))]); /*Daten*/
      a=abs(in_hdr->width)*abs(in_hdr->height);
      switch(in_hdr->bitcount)
      {
       case 4:
        a/=2;
        while(a--)
        {
         *(dptr2++)=dptr1[(*bptr)&15];
         *(dptr2++)=dptr1[(*(bptr++))>>4];
        }
        break;
       case 8:
        while(a--)
         *(dptr2++)=dptr1[*(bptr++)];
        break;
       case 24:            
        while(a--)
        {
         *(dptr2++)=(*((uint32_t*)bptr))&0xFFFFFF;
         bptr+=3;
        }
        break;
      }
      out_buffer=malloc((abs(in_hdr->width)+1)*(abs(in_hdr->height)+1)*4*typ*typ+256);
      if(out_buffer==NULL)
       printf(out_of_ram);
      else
      {
       b=abs(in_hdr->width);
       c=abs(in_hdr->height);
       hqxInit();
       switch(typ)
       {
        case 2: hq2x_32(in_buffer,out_buffer,b,c);break;
        case 3: hq3x_32(in_buffer,out_buffer,b,c);break;
        case 4: hq4x_32(in_buffer,out_buffer,b,c);break;
       }          
       output_bmp=malloc(14+40+b*c*3*typ*typ+1024);
       if(output_bmp==NULL)
        printf(out_of_ram);
       else
       {            
        memset(output_bmp,0,14+40);
        memcpy(output_bmp,"BM",2);
        dptr2=(uint32_t*)(output_bmp+2);
        dptr2[0]=abs(in_hdr->width)*abs(in_hdr->height)*typ*typ*3;
        dptr2[2]=40+14;
        out_hdr=(void*)(output_bmp+14);
        out_hdr->size=40;
        out_hdr->width=in_hdr->width*typ;
        out_hdr->height=in_hdr->height*typ;
        out_hdr->planes=1;
        out_hdr->bitcount=24;
        out_hdr->compression=0;
        out_hdr->sizeimage=dptr2[0];
        out_hdr->xpelspermeter=1000;
        out_hdr->ypelspermeter=1000;
        out_hdr->clrused=0;
        out_hdr->clrimportant=0;
        a=abs(in_hdr->width)*abs(in_hdr->height)*typ*typ;
        dptr1=out_buffer; /*Quelle (32bit)*/
        bptr=output_bmp+14+40; /*Ziel*/
        while(a--)
        {
         dptr2=(uint32_t*)bptr;
         bptr+=3;
         *dptr2=*(dptr1++);         
        }
        out_handle=fopen(out_name,"wb");
        if(out_handle==NULL)
         printf("Kann %s nicht anlegen!\n",out_name);
        else
        {
#if BUFSIZ<4096
         setvbuf(out_handle,NULL,_IOFBF,4096);
#endif
         fwrite(output_bmp,1,out_hdr->sizeimage+14+40,out_handle);
         fclose(out_handle);
         printf("%s->hq%cx->%s\n",in_name,typ+'0',out_name);         
        }
        free(output_bmp);  
       }       
       free(out_buffer);    
      }      
      free(in_buffer);
     }
    }
   }   
   free(input_bmp);   
  }  
  free(out_name);     
 }
 return r; 
}
#endif /*FEATURE_HQX*/


/*Algorithmus zum Vergrößern des Fensters
 0 - nicht vergrößern
 1 - Nearest Neighbour
 2 - Bilinear
 3 - HQX
*/
int algo=4;
int language=-1;
int codepage=437;
const char *language_code=NULL;

extern void *intro_video;


/*Allgemeine Mainfunktion im C-Stil*/
unsigned char game_main(int argc,char **argv)
{
 int a,b;

 b=0;
 for(a=1;a<argc;a++)
 {
  if(!strcmp(argv[a],"-exit"))
   b++;
#ifdef FEATURE_HQX
  else if(!strcmp(argv[a],"-hq2x"))
   a+=cmd_hqx(2,argc,argv,a);
  else if(!strcmp(argv[a],"-hq3x")) 
   a+=cmd_hqx(3,argc,argv,a);
  else if(!strcmp(argv[a],"-hq4x")) 
   a+=cmd_hqx(4,argc,argv,a);    
#endif /*FEATURE_HQX*/
#ifdef GAME_MAGNETIC_PLANET
/*  else if(!strcmp(argv[a],"-mp16species")) 
   make_mp16_species();*/
#endif  
  else if(!(strcmp(argv[a],"/?")&&strcmp(argv[a],"-?")&&
     strcmp(argv[a],"-help")&&strcmp(argv[a],"--help")))
     {
      printf("Parameter für %s:\n",argv[0]);
      printf("-exit = Programm nach Ausführung der Befehlszeile Beenden\n");      
      printf("/? -? -help --help = Hilfe Anzeigen\n");
      printf("-hq?x <Datei.BMP> [<Ziel.BMP>] = Datei.BMP um den Faktor ? mit hqx vergrößern.\n");                                                              
/*      printf("-mp16species = SPECIES.? Dateien erzeugen.\n");*/
      b=1;
     }
 }
 if(b)
 {
  return 1; 
 }
 language_code=strdup("DE");
 /* !! Sprache aus Umgebungsvariablen o.ä. lesen*/

 win_init(algo); 
 if(load_magnetic_planet()) 
 {
  win_input_get();
  win_init(0);
  return 1;
 }
 menue_init();
 yield_thread();
 set_midi_volume(50);
 play_song(0);
 yield_thread();
 write_text(0,20,20,320,32,"Loading Languages...",0,0);
 win_flush();   
 yield_thread();
 make_language_table();
 language_code=(char*)iso_language_code();
 printf("Sprache:%s\n",language_code);
 language=language_codetofile(language_code);
 if(language<0)
 {
  language=language_codetofile("EN");
 }
 if(language>-1)
 {
  char *sprache;
  language_info(NULL,NULL,&sprache,language);
  codepage=parse_language_file(sprache,1);
  set_codepage(codepage);     
 }
 yield_thread();
 /*Werte vorgeben*/
 welt.runde=0;
 welt.runde_max=255;
 welt.wasser=20;
 welt.temperatur=50;
 welt.feuchtigkeit=50;
 welt.allein=0;
 welt.scroll=1;
 welt.auto_weiter=0;
 welt.fast_fight=0;
 qp_randomize();
 relief_erzeugen(1);
 welt_make_tilemap();
 memset(spieler,0,sizeof(spieler));
 for(a=0;a<6;a++)
 {
  spieler[a].sammel.kinder=10;
  spieler[a].evol.iq=3;
  spieler[a].evol.typ=SPIELERTYP_COMPUTER;
  for(b=0;b<13;b++)
   spieler[a].evol.eigenschaften[b]=10;
 }
 spieler[0].evol.typ=SPIELERTYP_MENSCH;

 menue_init(); 
 play_video(intro_video);
 
 if(newgame(0)==2) goto LOAD_GAME;

 welt_punkte_verteilen();
 while(welt.runde<=welt.runde_max||welt.runde_max==255)
 {
  if(welt.runde)
  {
   for(a=0;a<6;a++)
   {
    if(!spieler[a].evol.tot)
    {
     if(spieler[a].evol.typ==SPIELERTYP_MENSCH)
      evolutionsrunde(a);
     if(spieler[a].evol.typ==SPIELERTYP_COMPUTER)
      evolutionsrunde_ki(a);
    }
    welt_ende_pruefen();
   }
   for(a=0;a<6;a++)
   {
    if(!spieler[a].evol.tot)
    {
     if(spieler[a].evol.typ==SPIELERTYP_MENSCH)
      sammelrunde(a);
     if(spieler[a].evol.typ==SPIELERTYP_COMPUTER)
     {
      sammelrunde_ki(a);
     }
    }
    welt_ende_pruefen();
   }
  }
  /*Ausbreitungsrunde*/
  for(a=0;a<6;a++)
  {
   if(!spieler[a].evol.tot)
   {
    if(spieler[a].evol.typ==SPIELERTYP_MENSCH)
     ausbreitungsrunde(a);
    if(spieler[a].evol.typ==SPIELERTYP_COMPUTER)
     ausbreitungsrunde_ki(a);
   }
   if(welt.runde)
    welt_ende_pruefen();
  }
  if(welt.runde)
  {
   /*Katastrophe*/
   a=qp_random(KAT_NUM);
   katastrophe_bild(a);
   katastrophe(a);
  }
  else 
   if(welt.auto_weiter)
   {
    katastrophe(-1);
   }
  welt_punkte_verteilen();
  welt_ende_pruefen();
LOAD_GAME:
  if(welt.runde)
  {
   /*Bewertungsrunde*/
   bewertungsrunde(0);
  }
  welt.runde++;
 }
 
/* bewertungsrunde(1);*/
 b=0;
 for(a=0;a<6;a++)
 {
  if(spieler[a].evol.rang<spieler[b].evol.rang)
   b=a;
 }
 if(spieler[b].evol.typ!=SPIELERTYP_MENSCH||(!(spieler[b].evol.einheiten)))
  katastrophe_bild(KAT_TOT);
 else
 {
  recolour_disasters(b);
  katastrophe_bild(KAT_SIEG);          
 }
 free_language_table(); 

 win_init(0);
 return 0; 
}

#ifdef FEATURE_DEBUG
/*Mainfunktion, mit Konsolenfenster
 muss mit gdi32.a gelinkt werden*/
int main(int argc,char **argv)
{
 return game_main(argc,argv);        
}
#else

#define ARGC_MAX  10

/*Befehlszeile im C-Stil in argv/argc zerlegen*/
static int parse_cmd(char **argv,char *arg)
{
 int argc=0;
 char *cptr; 
 cptr=(char*) arg;
 while(*cptr&&argc<ARGC_MAX)
 {
  while(*cptr==32||*cptr==9||*cptr==13||*cptr==10)
  {
   *cptr=0;
   cptr++;                  
  }
  if(!*cptr) break;
  argv[argc++]=cptr;
  while(*cptr!=32&&*cptr!=9&&*cptr!=13&&*cptr!=10&&*cptr!=0)
   cptr++;                                                
 }
 return argc;              
}

/*Mainfunktion ohne Konsolenfenster
 (Betriebssytemabhängig)*/
int WINAPI WinMain (HINSTANCE thisinst,HINSTANCE previnst,LPSTR arg,int funsterstil)
{
 char *argv[ARGC_MAX];
 int argc;
 argc=parse_cmd(argv,arg);
 return game_main(argc,argv);  
}
#endif
