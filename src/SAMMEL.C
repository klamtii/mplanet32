#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <mem.h>
#include <time.h>

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

#define SAMMEL_OVERSCAN 3

static const short raubtier_scanner[3]={100,70,150};
static const short raubtier_angriff[3]={250,350,150};

static uint8_t _sammel_tilemap[100*(100+SAMMEL_OVERSCAN*2)];
static uint8_t _sammel_sprites[100*(100+SAMMEL_OVERSCAN*2)];
uint8_t *sammel_tilemap=_sammel_tilemap+100*SAMMEL_OVERSCAN;
uint8_t *sammel_sprites=_sammel_sprites+100*SAMMEL_OVERSCAN;

struct RAUBTIER raubtiere[RAUBTIER_MAX];

struct RAUBTIER spielfigur;

unsigned int treibsand[TREIBSAND_MAX],treibsand_num;

#ifdef GAME_QPOP
struct RAUBTIER elektro;
#endif

static int raubtier_setzen(struct RAUBTIER *dst)
{
 unsigned int a,b;   
 dst->yoffs=dst->xoffs=0;
 dst->frame=dst->anim=0;
 dst->tot=0;
 for(a=0;a<100;a++)
 {
  dst->xdst=dst->xpos=qp_random(94)+3;
  dst->ydst=dst->ypos=qp_random(94)+3;
  b=dst->ypos*100+dst->xpos;
  if((!sammel_sprites[b])
   &&(!(tile_info[sammel_tilemap[b]]&TILEF_SOLID))
   &&(!(tile_info[sammel_tilemap[b]]&TILEF_PICKUP))
   &&(!(sammel_sprites[b-1]|sammel_sprites[b+1]|
        sammel_sprites[b-100]|sammel_sprites[b+100])))
  {                                                                
   return b;
  }
 }
 return 0;  
}

#define TILECLASS_ZUFALL                0
#define TILECLASS_KANTE                 1
#define TILECLASS_RESERVIERT            2
#ifdef GAME_QPOP
#define TILECLASS_GEBIRGE               3
#define TILECLASS_BASIS                 4
#define TILECLASS_MAX                   5
#else
#define TILECLASS_MAX                   3
#endif

unsigned int prop_tile[TILECLASS_MAX][NUM_TILES+1];

unsigned int prop_unit[7]; /*Gegner oder Weibchen*/


/*Karte erzeugen*/
int sammelrunde_vorbereiten(int spieler_num,int skill)
{
 int a,b,c,d;
 unsigned int dichte;
 uint16_t ti;
 const struct SPIELER *sp_str=&(spieler[spieler_num]);
 qp_randomize();
 if(sp_str->evol.einheiten<1) return 0;
 
 /*dichte:durchnittliche eigene Nachbarn pro einheit 0-7 (8 unmöglich)*/
 dichte=sp_str->stell.nachbarn[spieler_num]/sp_str->evol.einheiten;
 /*Warscheinlichkeiten berechnen*/
 for(a=0;a<NUM_TILES+1;a++)
 {
  for(b=0;b<TILECLASS_MAX;b++)
   prop_tile[b][a]=0;
  b=0;
  ti=tile_info[a];
  switch(ti&TILEMASK_TYP)
  {
   case 0:  case TILE_ELEK: case TILE_AUGE:
        /*Normale Kästchen frei oder Hindernis, mit und ohne Animation
         einschlieslich Schleim/Öl*/
        b=sp_str->evol.einheiten; /*  ~ Summe der Pflanzen*/
        if(!(ti&(TILEF_SOLID|TILEF_PICKUP))) b=b*4; 
        /*mehrere Frames würden Warscheinlichkeit zu sehr erhöhen*/
        if(ti&TILEMASK_ANIM) b=(b+1)/2; 
        break;
   case TILE_PFL1:  case TILE_PFL2:  case TILE_PFL3:
   case TILE_PFL4: case TILE_PFL5: case TILE_PFL6:
        /*Kästchen mit Pflanze, 0-5 mal Fressen*/
        b=(ti&TILEMASK_TYP)/TILEDIV_PFLANZEN;
        b=sp_str->stell.pflanzen[b-1];
#ifdef GAME_QPOP        
        if(!(ti&TILEF_PICKUP))
        {
         /*Bei hoher Bevölkerungsdichte weniger Nahrung
           stattdessen abgefressene Pflanzen*/
         if(b)
          b=(b*dichte)/8+1;
        }
        else
#endif
        {
         if(b)
          b=(b*(8-dichte))/8+1;
        }
        break;
   case TILE_KRAFT:   
         /*Kraftfutter/Diskette*/           
        b=sp_str->evol.einheiten; /*~~~Summe der Pflanzen*/
        /*Warscheinlichkeit 1/14-1/11 Einheiten (je nach IQ)*/
        c=16+skill;
        b/=c;
        break;
   case TILE_BERG:
        /*Felsen-> Warscheilichkeit nimmt bei Nachbarschaft zum Gebirge zu*/
        b=sp_str->evol.einheiten/2;
        b+=(sp_str->stell.gebirge)/2; /*~2 Einheiten:Nachbarsch. 8-fach gewichtet !!*/
        /*Warscheinlichkeit 0,5 bis 4,5 Einheiten*/
        break;
   case TILE_MEER:
        /*Pfützen-> Warscheilichkeit nimmt bei Nachbarschaft zum Wasser zu*/
        b=sp_str->evol.einheiten/2;
        b+=(sp_str->stell.wasser)/4;  /*~2 Einheiten:Nachbarsch. 8-fach gewichtet !!*/
        /*Warscheinlichkeit 0,5 bis 2,5 Einheiten*/
        break;
   case TILE_MENSCH:           
        /*Fässer (und Außenposten ?)
         hängt vom Menschen ab, gehört aber nicht zur Basis*/
        if(welt.raumschiff) b=sp_str->evol.einheiten/2;
        break;
   case TILE_NOGEN: case TILE_BASIS:
        /*Nicht zufällig erzeugt
        Ende-Der-Welt-Schild, leere Diskette, Menschenbasis*/
        break;
  }
  if(a==TILE_UNTERGRUND) 
  {
   b*=2;
#ifdef GAME_MAGNETIC_PLANET
   b+=dichte*sp_str->evol.einheiten*2;
#endif
  }
  if(ti&TILEF_SOLID)
   prop_tile[TILECLASS_ZUFALL][a]=b;
  else
   prop_tile[TILECLASS_ZUFALL][a]=b*2;
  if(ti&TILEF_EDGE)
  {
   prop_tile[TILECLASS_KANTE][a]=b?250:100;
  }
#ifdef GAME_QPOP
  if(ti&TILEF_SOLID)
   prop_tile[TILECLASS_GEBIRGE][a]=b;
  if((ti&TILEMASK_TYP)==TILE_BASIS)
   prop_tile[TILECLASS_BASIS][a]=100;
#endif  
  if((!(ti&TILEF_SOLID))&&((ti&TILEMASK_TYP)!=TILE_KRAFT))
   prop_tile[TILECLASS_RESERVIERT][a]=b;
 }
 /*Warscheinlichkeiten aufaddieren*/
 for(b=0;b<TILECLASS_MAX;b++)
  auswahl_vorbereiten(prop_tile[b],NUM_TILES+1);

#if 0 /* ------ */
 /*Warscheinlichkeiten für Weibchen und Gegner*/
 for(a=0;a<6;a++)
 {
  prop_unit[a]=sp_str->stell.nachbarn[a];
  if(prop_unit[a]&&a!=spieler_num) prop_unit[a]+=sp_str->evol.einheiten;
 }
 /*letzter Eintrag-> weder Weibchen noch Gegner */
 prop_unit[6]=(sp_str->evol.einheiten)*(20+skill);
 /*Einsame Einheit bekommen Weibchen auch ohne Nachbarschaft zu sich selbst*/
#ifdef GAME_MAGNETIC_PLANET
 /*Paarung (bzw. Zusammenbauen) zwingend erforderlich*/
 prop_unit[spieler_num]+=2;
#else
 /*Fortpflanzung geht auch durch genug Nahrung möglich*/
 prop_unit[spieler_num]++;
#endif
 auswahl_vorbereiten(prop_unit,7);

 if(sp_str->evol.typ==SPIELERTYP_COMPUTER)
 {
  /* Sammelrunde für Gegner simulieren */
  return 1;
 }
#endif /*0*/

 memset(_sammel_tilemap,TILECLASS_ZUFALL,100*(100+SAMMEL_OVERSCAN*2));
 memset(_sammel_sprites,0,100*(100+SAMMEL_OVERSCAN*2));
 memset(raubtiere,0,RAUBTIER_MAX*sizeof(struct RAUBTIER));
 /*Kante*/
 for(a=0;a<3;a++)
  for(b=0;b<100;b++)
  {
   sammel_tilemap[a*100+b]=TILECLASS_KANTE;
   sammel_tilemap[a+b*100]=TILECLASS_KANTE;
   sammel_tilemap[(99-a)*100+b]=TILECLASS_KANTE;
   sammel_tilemap[(99-a)+b*100]=TILECLASS_KANTE;
  }  
 b=50+(sp_str->evol.einheiten*300)/(sp_str->stell.gebirge+1);
 if(b>500) b=500;
 while(b--)
 {
  a=qp_random(100*100);
  if(sammel_tilemap[a]==TILECLASS_ZUFALL)
   sammel_tilemap[a]=TILECLASS_RESERVIERT;
 }
#ifdef GAME_QPOP /*mehr Gebirge*/
 b=50+(sp_str->stell.gebirge*300)/(sp_str->evol.einheiten)
 if(b>350) b=350;
 while(b--)
 {
  a=qp_random(100*100);
  if(sammel_tilemap[a]==TILECLASS_ZUFALL)
   sammel_tilemap[a]=TILECLASS_GEBIRGE;
 }
#endif

/*Mondbasis*/
#ifdef GAME_QPOP
#endif
 

 /*Klassen in Sinnvolle Einträge umwandeln*/
 for(a=0;a<100;a++)
  for(b=0;b<100;b++)
  {
   c=a*100+b;
   d=sammel_tilemap[c];
   switch(d)
   {
    case TILECLASS_KANTE:
#ifdef GAME_QPOP    
    case TILECLASS_BASIS:
#endif
     sammel_tilemap[c]=
      auswahl_treffen(prop_tile[d],NUM_TILES+1);
     break;
    case TILECLASS_ZUFALL:
    case TILECLASS_RESERVIERT:
#ifdef GAME_QPOP    
    case TILECLASS_GEBIRGE:
#endif
     sammel_tilemap[c]=d=auswahl_treffen(prop_tile[d],NUM_TILES+1);
     break;         
   }
  }
 /*Weibchen setzen*/  
 b=20+(1<<dichte)+10*dichte;
 if(b>200) b=200;
 printf("%d Weibchen\n",b);
 while(b--)
 {
  raubtier_setzen(&spielfigur);
  sammel_sprites[spielfigur.xpos+spielfigur.ypos*100]=SPR_WEIBCHEN;
 }
 for(a=0;a<6;a++)
 {
  prop_unit[a]=sp_str->stell.nachbarn[a];
  if(a==spieler_num) prop_unit[a]=0;
 }
 if(auswahl_vorbereiten(prop_unit,6))
 {
  for(a=0;a<100;a++)
  {
   raubtier_setzen(&spielfigur);
   sammel_sprites[spielfigur.xpos+spielfigur.ypos*100]=
    SPR_GEGNER(auswahl_treffen(prop_unit,6));
  }
 }
 /*Spieler auf Karte setzen*/
 spielfigur.xpos=0;
 a=0;
 while(spielfigur.xpos<10||spielfigur.ypos<10||spielfigur.xpos>89||spielfigur.ypos>89)
 {
  raubtier_setzen(&spielfigur);
  if(a++>100) return 0;
 }
 spielfigur.typ=spieler_num;
 /*Falltüren/Treibsand auf Karte setzen*/
 for(a=0;a<TREIBSAND_MAX;a++) treibsand[a]=0;
 a=TREIBSAND_MIN+skill+qp_random(3);
 if(a>=TREIBSAND_MAX) a=TREIBSAND_MAX;
 for(b=0;b<a;b++)
 { 
  struct RAUBTIER rt;
  c=0;do
  {
   raubtier_setzen(&rt);
   if(c++>20-skill) break;
  }while(abs(rt.xpos-spielfigur.xpos)<(30-skill*2)&&
         abs(rt.ypos-spielfigur.ypos)<(30-skill*2));
  treibsand[b]=rt.ypos*100+rt.xpos;
/*  sammel_tilemap[treibsand[b]]=0;  */
 }
 treibsand_num=a;
#ifdef GAME_QPOP
 /*Elektroschlange startet in der entferntesten Ecke*/
 elektro.xpos=(spielfigur.xpos>50)?3:96;
 elektro.ypos=(spielfigur.ypos>50)?3:96;
#endif 
 /*Anzahl der Raubtiere */
 a=30+((unsigned int)(sp_str->evol.einheiten)*(skill+1));
 if(welt.raumschiff) a+=10;
/* if(a>64) a=(a-64)/2+64;*/
 if(a>=RAUBTIER_MAX) a=RAUBTIER_MAX-1;
 /*Raubtiere auf Karte setzen*/
 for(b=1;b<a;b++) /*0 bleibt frei*/
 {
  c=0;do
  {
   d=raubtier_setzen(raubtiere+b);
   if(c++>20-skill) break;
  }while(abs(raubtiere[b].xpos-spielfigur.xpos)+abs(raubtiere[b].ypos-spielfigur.ypos)<5-skill);
  sammel_sprites[d]=b;
  
#ifdef GAME_QPOP
  if(raumschiff)raubtiere[b].typ=(b%3);
  else
#endif
   raubtiere[b].typ=(b&1);
 }
 return 1;
}

    

/*Koordinaten der Wolken (kein 64x64 Raster) berechnen*/
static void lovewar_index(int *xdst,int *ydst,int *dx,int *dy,int *xsrc,int *ysrc,int n)
{
 if(n&8) /* Senkrecht ?*/
 {
  *dx=64; *dy=100;
  *ysrc=128; *xsrc=(n&7)*64;
  *ydst+=14;
 }
 else
 {
  *dx=100; *dy=64;
  *ysrc=(n&4)<<4; *xsrc=(n&3)*100;    
  *xdst+=14;
 }
 if(n&16) *ysrc+=228;       
}

/*Sprite Zeichnen
 gibt 1 zurück wenn Animation am Ende*/
static int render_sprite
  (int *at_sound,uint16_t xscr,uint16_t yscr,uint16_t xcam,uint16_t ycam,
  struct RAUBTIER *spr,anim_t **anim)
{
 int frame_n,r=0;
 int xdst,ydst,dx,dy,xsrc,ysrc;
 xdst=(spr->xpos-(xcam>>6));
 ydst=(spr->ypos-(ycam>>6));
 if(xdst<-3||ydst<-3||xdst>11||ydst>11) return 1;
 xdst=(xdst*64)+spr->xoffs-(xcam&63);  ydst=(ydst*64)+spr->yoffs-(ycam&63);
 if(spr->frame>=anim[spr->anim][ANIM_LEN])
 {
  spr->frame=0;
  return 1;                            
 }
 frame_n=anim[spr->anim][ANIM_FRAME(spr->frame++)]-1;
 if(spr->frame>=anim[spr->anim][ANIM_LEN])
 {
  spr->frame=0;
  r=1;                            
 } 
 if(xdst<=-64||ydst<=-64||xdst>=64*7||ydst>=64*7) return r;
 dy=dx=64;
 xsrc=(frame_n%10)*64;
 ysrc=(frame_n/10)*64;
 if(xdst<0) {xsrc-=xdst;dx+=xdst;xdst=0;}
 if(ydst<0) {ysrc-=ydst;dy+=ydst;ydst=0;}
 if(xdst>64*6) dx-=(xdst&63);
 if(ydst>64*6) dy-=(ydst&63);
 xdst+=xscr;
 ydst+=yscr;
 /*Sonderbehandlung für Wolken aus LOVEWAR.DAT (größer als 64x64)*/
 if(anim[spr->anim][ANIM_BITMAP]==BM_LOVEWAR)
  lovewar_index(&xdst,&ydst,&dx,&dy,&xsrc,&ysrc,frame_n);
 draw_bitmaps(1,get_bitmap_slot(anim[spr->anim][ANIM_BITMAP]),
  &xdst,&ydst,&dx,&dy,&xsrc,&ysrc);
 if(spr->anim==PRDANM_SIEG) printf("%d!\n",anim[spr->anim][ANIM_SOUND]);
 if((anim[spr->anim][ANIM_SOUND])&&(at_sound!=NULL))
  *at_sound=anim[spr->anim][ANIM_SOUND];
 return r;
}

/*Zeichner Bildausschnitt*/
int sammelrunde_bildaufbau
 (unsigned int xcam,unsigned int ycam)
{
 static unsigned int frame_num=0;
 int a,b,c,d;
 uint32_t f[8];
 unsigned int tile_x,tile_y,offs_x,offs_y;
 int sound_num=0;
 unsigned int xscr=SPIELFELD_X+5, yscr=SPIELFELD_Y+5;
 
 frame_num++;
 tile_x=xcam/64; tile_y=ycam/64;
 offs_x=xcam%64; offs_y=ycam%64;
 for(a=0;a<8;a++)
 {
  int xdst[8],ydst[8],dx[8],dy[8],xsrc[8],ysrc[8];
  if(a==7&&offs_y==0) break;
  for(b=0;b<8;b++)
  {
   xdst[b]=b*64-offs_x+xscr;
   ydst[b]=a*64-offs_y+yscr;
   dy[b]=dx[b]=64;
   c=tile_x+b;if(c>99) c=99;
   d=tile_y+a;if(d>99) d=99;
   c+=d*100;
   c=sammel_tilemap[c];
   d=tile_info[c];
   if(d&TILEF_SOLID) f[b]=0xFFFFFF;
   if(d&TILEF_SOUND)
   {
    if(!sound_num||(entropy_get()&4))
    {
     ; /*QPOP Geräusche für Tiles*/                                 
    }
   }
   if(d&TILEMASK_ANIM)
   {
    if(!(d&TILEF_PICKUP))
    {
     if((d&TILEMASK_TYP)==TILE_AUGE)
     {;
      /*QPOP Auge*/
     }
     else sammel_tilemap[c]=d&TILEMASK_ANIM;                     
    }                   
   }
   c--;
   f[b]=0;
   if(c<0) c=59;
   d=c/10; c%=10;   
   xsrc[b]=c*64;
   ysrc[b]=d*64;
   if(!b) /*Rand links abschneiden*/
   {
    xdst[b]+=offs_x;      
    xsrc[b]+=offs_x;
    dx[b]-=offs_x;
   }
   if(!a) /*Rand oben abschneiden*/
   {
    ydst[b]+=offs_y;      
    ysrc[b]+=offs_y;
    dy[b]-=offs_y;
   }
   if(b==7) /*Rand rechts abschneiden*/
    dx[b]=offs_x;
   if(a==7) /*Rand unten abschneiden*/
    dy[b]=offs_y;
   c=tile_info[c];

  }
  draw_bitmaps(offs_x?8:7,get_bitmap_slot(BM_TILESET_SURVIVAL),
    xdst,ydst,dx,dy,xsrc,ysrc);
  for(b=0;b<8;b++)
  {
   dx[b]=xdst[b]+1;
   dy[b]=ydst[b]+1;
  }
/*  fill_rectangles(offs_x?8:7,xdst,ydst,dx,dy,f);*/
  
 }         
 for(a=-1;a<8;a++)
 {
  for(b=-1;b<8;b++)
  {
   struct RAUBTIER *spr=NULL;
   anim_t **anim=NULL;
   c=tile_x+b;if(c>99||c<0) continue;
   d=tile_y+a;if(d>99||d<0) continue;
   c+=d*100;
   c=sammel_sprites[c];
   if(c)
   {
    struct RAUBTIER sprtmp;
    if(c<RAUBTIER_MAX)
    {
     spr=raubtiere+c;
     if(spr->xpos==tile_x+b&&spr->ypos==tile_y+a)
      anim=raubtier_anim+PREDATOR_ANIM_NUM*spr->typ;
/*     switch(spr->typ)
     {
      case 0: anim=raubtier_hund;break;
      case 1: anim=raubtier_hydrant;break;                    
     }*/
/*     {
      struct RAUBTIER spr2;
      memcpy(&spr2,spr,sizeof(struct RAUBTIER));
      spr2.xpos=tile_x+b;
      spr2.ypos=tile_y+a;
      render_sprite(&c,xscr,yscr,xcam,ycam,&spr2,anim);
     }*/
    }
    else
    {
     sprtmp.xpos=tile_x+b;
     sprtmp.ypos=tile_y+a;
     sprtmp.yoffs=sprtmp.xoffs=0;
     spr=&sprtmp;        
     switch(c)
     {
      case SPR_WEIBCHEN:
       anim=species_anim;
       sprtmp.anim=SPCANM_WEIBCHEN;
       roboter_farbe_setzen(spielfigur.typ);
       break;
      case SPR_EI:
       anim=species_anim;
       sprtmp.anim=SPCANM_EI;
       roboter_farbe_setzen(spielfigur.typ);
       sprtmp.frame=spielfigur.frame;
       break;
      case SPR_JUNGES:
       anim=species_anim;
       sprtmp.anim=SPCANM_KIND;
       roboter_farbe_setzen(spielfigur.typ);
       break;
      case SPR_GEGNER(0):case SPR_GEGNER(1):case SPR_GEGNER(2):
      case SPR_GEGNER(3):case SPR_GEGNER(4):case SPR_GEGNER(5):
       anim=species_anim;
       sprtmp.anim=SPCANM_FEIND;
       roboter_farbe_setzen(c-SPR_GEGNER(0));
       break;
      case SPR_GEGNER_TOT(0):case SPR_GEGNER_TOT(1):case SPR_GEGNER_TOT(2):
      case SPR_GEGNER_TOT(3):case SPR_GEGNER_TOT(4):case SPR_GEGNER_TOT(5):
       anim=species_anim;
       sprtmp.anim=SPCANM_TOT;
       roboter_farbe_setzen(c-SPR_GEGNER_TOT(0));
       break;
     }
     if(anim!=NULL)
     switch(c)
     {
      case SPR_WEIBCHEN: case SPR_EI: case SPR_JUNGES:            
      case SPR_GEGNER(0):case SPR_GEGNER(1):case SPR_GEGNER(2):
      case SPR_GEGNER(3):case SPR_GEGNER(4):case SPR_GEGNER(5):
      case SPR_GEGNER_TOT(0):case SPR_GEGNER_TOT(1):case SPR_GEGNER_TOT(2):
      case SPR_GEGNER_TOT(3):case SPR_GEGNER_TOT(4):case SPR_GEGNER_TOT(5):
       d=(anim[sprtmp.anim])[ANIM_LEN];
       if(d<1) d+=1;
       sprtmp.frame=frame_num%d;
       if(c==SPR_EI)
        sprtmp.frame=spielfigur.frame; /*Ei wird mit Spieler animiert*/
       break;
     }
    }
    if(anim!=NULL&&spr!=NULL)
    {
     c=0;     
     render_sprite(&c,xscr,yscr,xcam,ycam,spr,anim);
     if(spr->anim==PRDANM_SIEG)printf("%d %d ",c,sound_num);
     if(c)
     {
      if(sound_num>-1||(entropy_get()&8))
       sound_num=-c;       
     }
     if(spr->anim==PRDANM_SIEG)printf("%d %d\n",c,sound_num);
    }
   }
  }
 }
 roboter_farbe_setzen(spielfigur.typ);
 c=0;
 if(!spielfigur.tot)
 {
  if(spielfigur.frame==0)
  {
   render_sprite(&c,xscr,yscr,xcam,ycam,&spielfigur,species_anim);
   if(c) sound_num=c;  
  }
  else 
   render_sprite(&c,xscr,yscr,xcam,ycam,&spielfigur,species_anim);
 }
 if(sound_num)
 {
  sound_num=abs(sound_num);
  printf("%d %d\n",c,sound_num);
  if(sound_num<=num_wav&&((!wave_is_playing())||c))
  {
   play_wave(wav_ptrs[sound_num-1],wav_lens[sound_num-1]);
  }
  else printf("%d\n",sound_num);
 }
}

/*Koordinaten (in Pixel) eines Sprites an Frame anpassen*/
static void move_sprite_anim(struct RAUBTIER *spr,int dist) 
/*Dist 0-64 von xpos/ypos zu xdst/ydst setzt xoffs/yoffs */
{
 if(spr->xpos>spr->xdst)
  spr->xoffs=-dist;
 else if(spr->xpos<spr->xdst)
  spr->xoffs=dist;
 else
  spr->xoffs=0;
 if(spr->ypos>spr->ydst)
  spr->yoffs=-dist;
 else if(spr->ypos<spr->ydst)
  spr->yoffs=dist; 
 else
  spr->yoffs=0;             
}

/*Sprite-Daten nach abgeschlossener Bewegung ändern*/
static int move_sprite_final(struct RAUBTIER *spr) 
/*setzt xpos/ypos*/
{
 unsigned int num;
 unsigned int index;
 index=spr->ypos*100+spr->xpos;
 num=sammel_sprites[index];
 sammel_sprites[index]=0;
/* if(!num&&spr!=&spielfigur) {printf("Sprite Nummer 0 ?\n");return;}*/
 if(spr->xpos>spr->xdst)
  spr->xpos--;
 else if(spr->xpos<spr->xdst)
  spr->xpos++;
 if(spr->ypos>spr->ydst)
  spr->ypos--;
 else if(spr->ypos<spr->ydst)
  spr->ypos++;
 spr->yoffs=spr->xoffs=0;
 index=spr->ypos*100+spr->xpos; 
 sammel_sprites[index]=num;
 if(!spr->tot) {spr->anim=0;spr->frame=0;}
 return spr->xpos==spr->xdst&&spr->ypos==spr->ydst;
}

/*move_sprite_anim() für alle*/
void move_all_sprites_anim(int xcam,int ycam,int dist)
{
 int a;
 int x,y;
/* x=spielfigur->xpos; y=spielfigur->ypos;*/
 x=xcam+3;y=ycam+3;
 if(x<0||y<0||x>99||y>99) return;
 for(a=1;a<RAUBTIER_MAX;a++)
 {
  if(abs(raubtiere[a].xpos-x)<5&&abs(raubtiere[a].ypos-y)<5)
   move_sprite_anim(raubtiere+a,dist);  
 }   
 if(abs(spielfigur.xpos-x)<5&&abs(spielfigur.ypos-y)<5)
  move_sprite_anim(&spielfigur,dist); 
}

/*move_sprite_final() für alle*/
void move_all_sprites_final(void)
{
 int a;
/* int x,y;*/
/* x=spielfigur->xpos; y=spielfigur->ypos;*/
/* x=xcam+3;y=ycam+3;
 if(x<0||y<0||x>99||y>99) return;*/
 for(a=1;a<RAUBTIER_MAX;a++)
 {
/*  if(abs(raubtiere[a].xpos-x)<5&&abs(raubtiere[a].ypos-y)<5)*/
   move_sprite_final(raubtiere+a);  
 }   
/* if(abs(spielfigur.xpos-x)<5&&abs(spielfigur.ypos-y)<5)*/
  move_sprite_final(&spielfigur); 
}

/*Zug für ein Raubtier berechnen*/
void raubtier_ki(struct RAUBTIER *spr,unsigned int tarnung)
{                    
 int a,b,c;
 static const int delta[4]={1,-1,100,-100};
 unsigned int prop[4]={0,0,0,0}; /*O W S N*/
 unsigned int abstand,index;
 spr->xdst=spr->xpos;
 spr->ydst=spr->ypos;
 spr->yoffs=spr->xoffs=0;
 if(spr->tot) return;
 abstand=abs((int)(spr->xpos-spielfigur.xpos));
 a=abs((int)(spr->ypos-spielfigur.ypos));
 if(a>abstand) abstand=a;
 if(abstand>20) return;
 index=spr->ypos*100+spr->xpos; 
 for(b=0;b<4;b++)
  if(!(tile_info[sammel_tilemap[index+delta[b]]]&TILEF_SOLID)&&
     !sammel_sprites[index+delta[b]])
   prop[b]=10;
 if(abstand<4)
 {
  a=raubtier_scanner[spr->typ];                
  switch(abstand)
   {case 1:a*=100;case 2:a*=2;case 3:a*=5; break;}
  printf("Scentchance qp_random(%d)>%d\n",a,tarnung);
  if(qp_random(a)>tarnung)
  { 
/*   prop[3]=prop[2]=prop[1]=prop[0]=0;*/
   a=(int)spielfigur.xpos-(int)spr->xpos;
   b=(int)spielfigur.ypos-(int)spr->ypos;  
   c=100; if(abs(a)>=abs(b)) c*=3;
   if(a>0) {prop[0]=prop[0]*10+c;prop[1]=0;}
   if(a<0) {prop[1]=prop[1]*10+c;prop[0]=0;}
   c=100; if(abs(b)>=abs(a)) c*=3;
   if(b>0) {prop[2]=prop[2]*10+c;prop[3]=0;}
   if(b<0) {prop[3]=prop[3]*10+c;prop[2]=0;}
  }
 } 
 auswahl_vorbereiten(prop,4);
 a=auswahl_treffen(prop,4);
 switch(a)
 {
  case 0:spr->anim=PRDANM_OSTEN;spr->xdst++;break;
  case 1:spr->anim=PRDANM_WESTEN;spr->xdst--;break;
  case 2:spr->anim=PRDANM_SUEDEN;spr->ydst++;break;
  case 3:spr->anim=PRDANM_NORDEN;spr->ydst--;break;
 }
 a=0;
 b=sammel_sprites[index]; /*eigene Nummer*/
 index=spr->ydst*100+spr->xdst;
 if(tile_info[sammel_tilemap[index]]&TILEF_SOLID) a=1;
 if(sammel_sprites[index]) a=1;
 if((spr->xdst==spielfigur.xpos&&spr->ydst==spielfigur.ypos)||
    /* Wenn Spieler herkommt, einfach warten*/
    (spr->xdst==spielfigur.xdst&&spr->ydst==spielfigur.ydst)) a=1;
 if(a)
 {
  spr->xdst=spr->xpos;
  spr->ydst=spr->ypos;
  spr->anim=PRDANM_STEHEN;      
 }
 else
  sammel_sprites[index]=b; /*Kästchen mit Nummer reservieren*/
}

#define SCAN_BLATT          1
#define SCAN_HERZ           2
#define SCAN_FEIND          4
#define SCAN_TOT            8

#define SCANNER_KANTE          19
#define SCANNER_MITTE          9
#define SCANNER_PIXEL          8


static uint8_t scanner_dat[SCANNER_KANTE*SCANNER_KANTE];

/*Daten in scanner_dat[] eintragen*/
static void scanner_setzen(int xpos,int ypos,struct EVOLUTION *ev) 
{
 int a,b,c,d,e,f,g;
 unsigned int weite2;
 memset(scanner_dat,0,SCANNER_KANTE*SCANNER_KANTE);
 weite2=ev->eigenschaften[EVO_SINN]+30;
 if(weite2>130) weite2=130;
 weite2=(weite2*weite2)/100; /*Radius^2 der Reichweite*/
 for(a=0;a<SCANNER_KANTE;a++)
 {
  c=a-SCANNER_MITTE;
  e=c+ypos;
  if(e<0||e>99) continue;
  e*=100;
  for(b=0;b<SCANNER_KANTE;b++)
  {
   d=b-SCANNER_MITTE;
   if(c*c+d*d>weite2) continue;
   d+=xpos;
   if(d<0||d>99) continue;
   d+=e;
   if(sammel_sprites[d]==SPR_WEIBCHEN)
    scanner_dat[a*SCANNER_KANTE+b]=SCAN_HERZ;
   else if(sammel_sprites[d]>=SPR_GEGNER(0)&&sammel_sprites[d]<SPR_GEGNER(6))
    scanner_dat[a*SCANNER_KANTE+b]=SCAN_FEIND;
   else if(sammel_sprites[d]>0&&sammel_sprites[d]<RAUBTIER_MAX)
    if(!raubtiere[sammel_sprites[d]].tot)
     scanner_dat[a*SCANNER_KANTE+b]=SCAN_TOT;
   d=sammel_tilemap[d];
   f=tile_info[d];
   if(f&TILEF_PICKUP)
   {
    f=((f&TILEMASK_TYP)/TILEDIV_PFLANZEN)-1;
    if(f>=0&&f<6)
    {
     f=ev->eigenschaften[f];
     if(f>0)
     {
      g=0;
      while(tile_info[d]&TILEF_PICKUP)
      {
       g+=f; if(g>200) break;
       d=tile_info[d]&TILEMASK_ANIM;                                
      }
      if(g>=125)
       scanner_dat[a*SCANNER_KANTE+b]|=SCAN_BLATT;
     }
    }
   }
  }     
 } 
}

/*Eine Art Bilder (Blatt,Herz,...) Aus scanner_dat Zeichnen*/
static void scanner_bitmap(int scrx,int scry,int typ,int bitmap)
{
 int a,b,c,d=0;
 int src[10],xdst[10],ydst[10],delta[10];
 for(a=0;a<SCANNER_KANTE;a++)
 {
  c=a*SCANNER_KANTE;
  for(b=0;b<SCANNER_KANTE;b++)
  {
   if(scanner_dat[c+b]&typ)
   {    
    src[d]=0; delta[d]=8;
    ydst[d]=scry+8*a+7;                        
    xdst[d++]=scrx+8*b+7;
    if(d>=10)
    {
     draw_bitmaps(d,get_bitmap_slot(bitmap),xdst,ydst,delta,delta,src,src);
     d=0;
    }
   }
  }
 }      
 if(d)
  draw_bitmaps(d,get_bitmap_slot(bitmap),xdst,ydst,delta,delta,src,src);                        
}

/*Scanner vollständig Zeichnen*/
void scanner_zeichnen(int xpos,int ypos,struct EVOLUTION *ev)
{
 int a,b;
 int scrx=HUD_X+6, scry=HUD_Y+6;
#ifdef GAME_QPOP
 uint32_t f;
 a=scrx+166;b=scry+166;f=0;
 fill_rectangles(1,&scrx,&scry,&a,&b,&f);
#endif
#ifdef GAME_MAGNETIC_PLANET
 int c;
 a=166;b=0;c=437;
 draw_bitmaps(1,get_bitmap_slot(BM_RAND_DAT),&scrx,&scry,&a,&a,&b,&c); 
#endif
 scanner_setzen(xpos,ypos,ev);
 scanner_bitmap(scrx,scry,SCAN_BLATT,BM_SCANNER_BLATT);
 scanner_bitmap(scrx,scry,SCAN_HERZ,BM_SCANNER_HERZ);
 scanner_bitmap(scrx,scry,SCAN_FEIND,BM_SCANNER_FEIND);
 scanner_bitmap(scrx,scry,SCAN_TOT,BM_SCANNER_TOT);
#ifdef GAME_MAGNETIC_PLANET
 a=166;b=0;c=0;
 draw_bitmaps_x(1,get_bitmap_slot(BM_SCANNER_MASKE),&scrx,&scry,&a,&a,&b,&c,-1); 
 a=166;b=0;c=437;
 draw_bitmaps_x(1,get_bitmap_slot(BM_RAND_DAT),&scrx,&scry,&a,&a,&b,&c,1); 
#endif
}

/*Balken (für Züge) zeichnen*/
void balken_zeichnen(int x,int y,int dx,int dy,int w,uint32_t _f)
{
 int x1[2],y1[2],x2[2],y2[2],f[2],n=0;
 if(w>0)
 {
  x1[n]=x; y1[n]=y;
  x2[n]=x+w-1; y2[n]=y+dy-1;
  f[n++]=_f;
 }
 if(w<dx)
 {
  x1[n]=x+w; y1[n]=y;
  x2[n]=x+dx; y2[n]=y+dy-1;
  f[n++]=0;
 }
 if(n)
  fill_rectangles(n,x1,y1,x2,y2,f);
}

/*Mausklick auf Tile für Sammelrunde prüfen*/
static int mouse_in_tile(uint32_t m,int x,int y,int rechts)
{
 return mouse_in_field(m,4+x*64,26+y*64,4+(x+1)*64,26+(y+1)*64,
  rechts?MOUSE_KEYR:MOUSE_KEYL);
}

static int idle_function(void)
{
 clk_wait(CLK_TCK/100+1);      
}

/*Punkt für anzeige Zeichnen*/
void punkt_zeichnen(unsigned short typ,unsigned short pos)
{
 int xdst,ydst,dx,dy,xsrc,ysrc;
 if(typ<3)
 {
  dy=dx=7; xsrc=166; ysrc=437+typ*7;
  xdst=50+HUD_X+pos*12; 
  ydst=266+37*typ+HUD_Y;
  draw_bitmaps(1,get_bitmap_slot(BM_RAND_DAT),&xdst,&ydst,&dx,&dy,&xsrc,&ysrc);
 }
}

/*Aufrufen, wenn Spieler tot ist*/
void spieler_tot(struct SPIELER *sp_str)
{
/* int xdst,ydst,delta,xsrc,ysrc;*/
 /*roten Punkt zeichnen */
 if(sp_str->sammel.tote<10)
 {
  punkt_zeichnen(2,sp_str->sammel.tote);
  
/*  delta=7; xsrc=166; ysrc=451;
  xdst=50+HUD_X+sp_str->sammel.tote*12; 
  ydst=340+HUD_Y;
  draw_bitmaps(1,get_bitmap_slot(BM_RAND_DAT),&xdst,&ydst,&delta,&delta,&xsrc,&ysrc);*/
 }  
 sp_str->sammel.tote++;
 raubtier_setzen(&spielfigur);
 spielfigur.tot=0;
 scanner_zeichnen(spielfigur.xpos,spielfigur.ypos,&(sp_str->evol));
}

static void CALLCONV redraw_func( void*p)
{
 int skill;
 unsigned int zeit;
 int xdst,ydst,dx,dy,xsrc,ysrc;
 struct SPIELER *sp_str;
 sp_str=&(spieler[(int)p]);
/* switch(sp_str->evol.typ)
 {
  case SPIELERTYP_MENSCH: skill=4-sp_str->evol.iq;break;
  case SPIELERTYP_COMPUTER: skill=sp_str->evol.iq-1;break;
  default: return;
 }*/
 zeit=sp_str->sammel.zeit;
 skill=4-sp_str->evol.iq;
 xdst=HUD_X;  ydst=HUD_Y;
 dx=180;        dy=437;
 xsrc=0;        ysrc=0;
 draw_bitmaps(1,get_bitmap_slot(BM_RAND_DAT),&xdst,&ydst,&dx,&dy,&xsrc,&ysrc);
 draw_frame(xdst-1,ydst-1,dx+1,dy+2,1,0,0);
 draw_frame(SPIELFELD_X,SPIELFELD_Y,64*7+10,64*7+10,2,FARBE_HELL,FARBE_DUNKEL);
 draw_frame(SPIELFELD_X+4,SPIELFELD_Y+4,64*7+2,64*7+2,2,0,0);
 sammelrunde_bildaufbau((spielfigur.xpos-3)*64,(spielfigur.ypos-3)*64);
 scanner_zeichnen(spielfigur.xpos,spielfigur.ypos,&(sp_str->evol));
 balken_zeichnen(HUD_X+52,HUD_Y+191,119,23,(119*zeit)/(80-skill*10-1),0xFF1111);
 xdst=sp_str->sammel.nahrung/80;
 for(ydst=0;ydst<xdst;ydst++)
  punkt_zeichnen(0,ydst);
 xdst=sp_str->sammel.paarungen;
 for(ydst=0;ydst<xdst;ydst++)
  punkt_zeichnen(1,ydst);
 xdst=sp_str->sammel.tote;
 for(ydst=0;ydst<xdst;ydst++)
  punkt_zeichnen(2,ydst);
 weiter_knopf(0);
}

static int CALLCONV clickable_func(unsigned int x,unsigned int y)
{
 if(y<21) return menue_clickable(x);
 if(y>480-20&&x>HUD_X) return 1;
 return 0;
}

/*Auswirkungen der Sammelrunde auf die Weltkarte (Spieler oder KI)*/
void sammelrunde_auswerten(struct SPIELER *sp_str)
{
 int a,b;
 uint8_t *unitptr=NULL;
 /*Sterblichkeit*/
 a=sp_str->sammel.tote*5;
 if(sp_str->sammel.nahrung<800)
  a+=random_div(800-sp_str->sammel.nahrung,8);
 if(a>90) a=90;
 sp_str->sammel.sterblichkeit=a;
 /*Kinder*/
 a=sp_str->sammel.paarungen;
 if(sp_str->sammel.nahrung>800)
  a+=random_div(sp_str->sammel.nahrung-800,400);
 b=random_div(a*sp_str->evol.eigenschaften[EVO_VERMEHR],20);
 if(b<sp_str->sammel.paarungen) b=sp_str->sammel.paarungen;
 if(b>20) b=20;
 sp_str->sammel.kinder=b;
 /*Bewegung*/
 sp_str->sammel.bewegung=random_div(sp_str->evol.eigenschaften[EVO_V],5); 
}

/*Sammelrunde für Spieler*/
int sammelrunde(int spieler_num)
{        
 int a,b,c;
 int xdst,ydst,dx,dy,xsrc,ysrc;
 int skill;
 int nahrung_icons=0;
 uint16_t this_frame=1;
 uint32_t f,inkey=0;
 clock_t t1,t2,t3;
 unsigned int zeit;
 clock_t zugzeit,framezeit;
 uint16_t pickup=0;
 struct SPIELER *sp_str;
 static const short lovewar_delta[4]={-100,-1,100,1};
 static const short lovewar_offx[4]={0,-1,0,0};
 static const short lovewar_offy[4]={-1,0,0,0};
 static const short love_anim[4]={SPCANM_LOVE_V,SPCANM_LOVE_H,SPCANM_LOVE_V,SPCANM_LOVE_H};
 static const short war_anim[4]={SPCANM_WAR_V,SPCANM_WAR_H,SPCANM_WAR_V,SPCANM_WAR_H};
 static const short raubtier_anim[4]={PRDANM_ANGRIFF_S,PRDANM_ANGRIFF_O,PRDANM_ANGRIFF_N,PRDANM_ANGRIFF_W};

 if(spieler_num<0||spieler_num>=6) return 0;
 sp_str=spieler+spieler_num;
 memset(&(sp_str->sammel),0,sizeof(struct SAMMELRUNDE));

 switch(sp_str->evol.typ)
 {
  case SPIELERTYP_MENSCH: skill=4-sp_str->evol.iq;break;
  case SPIELERTYP_COMPUTER: skill=sp_str->evol.iq-1;break;
  default: return;
 }
 if(skill<0) skill=0; if(skill>3) skill=3;
 sammelrunde_vorbereiten(spieler_num,skill);
 zeit=80-skill*10;
 zugzeit=CLK_TCK*2;
 framezeit=CLK_TCK/10;
 if(framezeit<1) framezeit=1;
 /* .... */
 if(sp_str->evol.typ!=SPIELERTYP_MENSCH) return 0;    
 /* Gui am Rand zeichnen*/
 move_all_sprites_anim(spielfigur.xpos,spielfigur.ypos,0);
 move_all_sprites_final();
 xdst=HUD_X;  ydst=HUD_Y;
 dx=180;        dy=437;
 xsrc=0;        ysrc=0;
 win_lock();
 set_redraw_func(redraw_func,(void*)spieler_num);
 menue_init();
 redraw_func((void*)spieler_num);
 win_flush();
 set_clickable_func(clickable_func);
 while(game_input_ready()) game_input_get();
 while(zeit&&inkey!=13)
 {
  this_frame++;
  t2=t1=clock();
  t2+=zugzeit;
  sp_str->sammel.zeit=zeit;
  balken_zeichnen(HUD_X+52,HUD_Y+191,119,23,(119*zeit)/(80-skill*10-1),0xFF1111);
WARTEN_AUF_ZUG:
  spielfigur.anim=SPCANM_STEHEN;
  spielfigur.frame=0;
  inkey=' '; /*Automatisch fressen*/
  while(t1<t2&&t2-t1<=zugzeit/*Überlauf*/&&!game_input_ready())  
  {
   t3=t1=clock();
   t3+=framezeit;
   win_lock();
   sammelrunde_bildaufbau((spielfigur.xpos-3)*64,(spielfigur.ypos-3)*64);
   win_flush();
   while(t1<t3&&t3-t1<=framezeit)
   {
    t1=clock();
    idle_function();
   }
   while(window_minimized())
   {
    clk_wait(CLK_TCK/5+1);
    t1+=CLK_TCK/5+1; 
    t2+=CLK_TCK/5+1; /* Zeit bleibt stehen wenn minimiert*/                
   }                                
   if(!game_input_ready()) /*Treibsand/Falltür ?*/
   {
    if(
     (tile_info[sammel_tilemap[spielfigur.ypos*100+spielfigur.xpos]]
     &TILEMASK_TYP)==TILE_ELEK)
    {
     printf("Falltür ?...");
     if(qp_random(1000)==1&&sp_str->sammel.kraftfutter==0)
     {
      spielfigur.anim=SPCANM_TREIBSAND;                  
      spielfigur.frame=0;                      
      /*Animieren*/
      do
      {
       t3=t1=clock();
       t3+=framezeit;
       while(game_input_ready()) game_input_get();
       win_lock();
       sammelrunde_bildaufbau((spielfigur.xpos-3)*64,
                           (spielfigur.ypos-3)*64);
       win_flush();
       while(t1<t3&&t3-t1<=framezeit)
       {
        t1=clock();
        idle_function();
       }
      }while(spielfigur.frame);
      spieler_tot(sp_str);                                               
     }
    }          
   }
  }
  if(game_input_ready()) inkey=game_input_get();
  if(inkey&MOUSE_FLAG)
  {
   /*Mausklicks übersetzen*/
   if(mouse_in_tile(inkey,4,3,0))inkey='6';
   if(mouse_in_tile(inkey,2,3,0))inkey='4';
   if(mouse_in_tile(inkey,3,4,0))inkey='2';
   if(mouse_in_tile(inkey,3,2,0))inkey='8';
   if(mouse_in_tile(inkey,3,3,0))inkey=' ';
   if(mouse_in_tile(inkey,3,3,1))inkey='K';
   if(mouse_in_field(inkey,64*7+8,26+437,640,480,0)) 
   {
    weiter_knopf(1);
    inkey=13;
   }
  }
  if(inkey==13&&zeit)
  {
   if(!msgbox_janein(get_string(STR_CONF))) inkey=0;                                                 
  }
  if(inkey==13) break;  
  spielfigur.xdst=spielfigur.xpos;
  spielfigur.ydst=spielfigur.ypos;
  spielfigur.anim=SPCANM_STEHEN;
  switch(inkey)
  {
   case ' ':break;
   case '6':spielfigur.xdst++;spielfigur.anim=SPCANM_OSTEN;break;
   case '4':spielfigur.xdst--;spielfigur.anim=SPCANM_WESTEN;break;
   case '2':spielfigur.ydst++;spielfigur.anim=SPCANM_SUEDEN;break;
   case '8':spielfigur.ydst--;spielfigur.anim=SPCANM_NORDEN;break;
   case 'K':
    if(msgbox_janein(get_string(STR_SUICIDE)))
     spieler_tot(sp_str);
   default:
    goto WARTEN_AUF_ZUG;      
  }
  a=spielfigur.ydst*100+spielfigur.xdst;
  pickup=0;
  if(inkey!=' ')
  {
   if(spielfigur.xdst<3||spielfigur.xdst>96)  goto WARTEN_AUF_ZUG;
   if(spielfigur.ydst<3||spielfigur.ydst>96)  goto WARTEN_AUF_ZUG;
   if(tile_info[sammel_tilemap[a]]&TILEF_SOLID) goto WARTEN_AUF_ZUG;
   if(sammel_sprites[a]) 
   {
    printf("Sprite %d steht im Weg !\n",sammel_sprites[a]);
    goto WARTEN_AUF_ZUG;   
   }
  }
  else
  {   
   pickup=tile_info[sammel_tilemap[a]];
   if(pickup&TILEF_PICKUP)
   {
    if(pickup&TILEMASK_ANIM) sammel_tilemap[a]=pickup&TILEMASK_ANIM;
    spielfigur.anim=SPCANM_PICKUP;
   }
   else pickup=0;
  }
  /*Raubtiere Ziehen*/
  for(a=spielfigur.ypos-SCANNER_KANTE;a<=spielfigur.ypos+SCANNER_KANTE;a++)
  {
   if(a<0||a>=100) continue;
   for(b=spielfigur.xpos-SCANNER_KANTE;b<=spielfigur.xpos+SCANNER_KANTE;b++)
   {
    if(b<0||b>=100) continue;
    c=sammel_sprites[a*100+b];
    if(c>0&&c<RAUBTIER_MAX)
    {
     /*Raubtiere reservieren Felder mit ihrer Nummer, dürfen aber nur
      einmal bewegt werden !*/
     if(raubtiere[c].last_frame!=this_frame)
     {
      raubtier_ki(raubtiere+c,sp_str->evol.eigenschaften[EVO_TARNUNG]*4+
       sp_str->evol.eigenschaften[EVO_V]*2+sp_str->evol.eigenschaften[EVO_INT]);
/*tarnung * 4 + geschwindigkeit * 2 + intelligenz*/
      raubtiere[c].last_frame=this_frame;
     }
    }
   }   
  }
  spielfigur.frame=0;
  /* Zuganimation */  
  for(a=0;a<64;a+=8)
  {
   t3=t1=clock();
   t3+=framezeit/2;
   while(game_input_ready()) entropy_add(game_input_get());
   move_all_sprites_anim(spielfigur.xpos-3,spielfigur.ypos-3,a);
   win_lock();
   sammelrunde_bildaufbau((spielfigur.xpos-3)*64+spielfigur.xoffs,
                          (spielfigur.ypos-3)*64+spielfigur.yoffs);
   win_flush();
   while(t1<t3&&t3-t1<=framezeit)
   {
    t1=clock();
    idle_function();
   }
   if(!welt.scroll) break;
  }
  move_all_sprites_final();
/*stop_wave();*/
  spielfigur.anim=SPCANM_STEHEN;
  move_all_sprites_anim(spielfigur.xpos,spielfigur.ypos,0);
  /*Was wurde gefressen ?*/
  if(pickup&TILEF_PICKUP)
  {
   pickup=(pickup&TILEMASK_TYP);
   spielfigur.frame=0;
    /*Kraftfutter*/
   if(pickup==TILE_KRAFT)
   {
    spielfigur.anim=SPCANM_KRAFT; /*Animieren*/
    sp_str->sammel.kraftfutter++;
    pickup=1; /*Animation*/
   }
   else if(pickup>=TILE_PFL1&&pickup<=TILE_PFL6)
   {
    spielfigur.anim=SPCANM_STEHEN;
    sp_str->sammel.nahrung+=sp_str->evol.eigenschaften[(pickup/TILEDIV_PFLANZEN)-1];
    /*gelbe Punkte zeichnen*/
    if(sp_str->sammel.nahrung>MAX_FOOD)sp_str->sammel.nahrung=MAX_FOOD;
    a=sp_str->sammel.nahrung/FOOD_UNIT;
    /*win_lock();*/
    while(nahrung_icons<a)
    {
     punkt_zeichnen(0,nahrung_icons);
     nahrung_icons++;
    }
    pickup=0; /*keine Animation*/
   }
   else
   {
    spielfigur.anim=SPCANM_KOTZEN;       
    pickup=1; /*Animation*/
   }
  }
  /*Scanner neu Zeichnen*/
  scanner_zeichnen(spielfigur.xpos,spielfigur.ypos,&(sp_str->evol));
  win_flush();  
  /*Animation nach Fressen*/
  if(pickup==1) do
  {
   t3=t1=clock();
   t3+=framezeit;
   while(game_input_ready()) entropy_add(game_input_get());
   win_lock();
   sammelrunde_bildaufbau((spielfigur.xpos-3)*64+spielfigur.xoffs,
                          (spielfigur.ypos-3)*64+spielfigur.yoffs);
   win_flush();
   while(t1<t3&&t3-t1<=framezeit)
   {
    t1=clock();
    idle_function();
   }                                                   
  }while(spielfigur.frame);
  
  /*Kampf/Paarung mit umliegenden Sprites*/
  for(a=0;a<4;a++)
  {
   uint8_t *wen_animieren=&(spielfigur.frame); /*Raubtier oder Spieler siegt*/
   b=spielfigur.ypos*100+spielfigur.xpos+lovewar_delta[a];
   c=sammel_sprites[b];
   if(c==SPR_WEIBCHEN) /*Weibchen*/ 
    spielfigur.anim=love_anim[a];
   else if(c>=SPR_GEGNER(0)&&c<=SPR_GEGNER(5))
    spielfigur.anim=war_anim[a];
   else if(c>0&&c<=RAUBTIER_MAX)
   {
    /*Raubtier*/
    if(!raubtiere[c].tot) /*Lebt es noch ?*/
    {
     raubtiere[c].anim=raubtier_anim[a];
     raubtiere[c].frame=0;
     /*Angriffsanimation*/
     do
     {
      t3=t1=clock();
      t3+=framezeit;
      while(game_input_ready()) game_input_get();
      win_lock();
      sammelrunde_bildaufbau((spielfigur.xpos-3)*64,
                          (spielfigur.ypos-3)*64);
      win_flush();
      while(t1<t3&&t3-t1<=framezeit)
      {
       t1=clock();
       idle_function();
      }    
     }while(raubtiere[c].frame);
     
     spielfigur.anim=war_anim[a];
    }
    else c=0;
   }
   else c=0;
   if(c)
   {
    sammel_sprites[b]=0; /*Sprite entfernen, Spielfigur wird Wolke*/
    spielfigur.xpos+=lovewar_offx[a];
    spielfigur.ypos+=lovewar_offy[a];
    spielfigur.frame=0;
    /*Wolke Zeichnen*/             
    do
    {
     t3=t1=clock();
     t3+=framezeit;
     while(game_input_ready()) game_input_get();
     win_lock();
     sammelrunde_bildaufbau((spielfigur.xpos-3-lovewar_offx[a])*64,
                          (spielfigur.ypos-3-lovewar_offy[a])*64);
     win_flush();
     while(t1<t3&&t3-t1<=framezeit)
     {
      t1=clock();
      idle_function();
     }
    }while(spielfigur.frame);
    /*Spielfigur zurück*/
    spielfigur.xpos-=lovewar_offx[a];
    spielfigur.ypos-=lovewar_offy[a];
    
    if(c==SPR_WEIBCHEN) /*Weibchen*/ 
    {
     spielfigur.anim=SPCANM_STEHEN;
     sammel_sprites[b]=SPR_EI;
     /*blauen Punkt zeichnen */
     if(sp_str->sammel.paarungen<10)
     {
      punkt_zeichnen(1,sp_str->sammel.paarungen);
     }
     sp_str->sammel.paarungen++;
    }
    else if(c>=SPR_GEGNER(0)&&c<=SPR_GEGNER(5))
    { /*Gegner*/
     stop_wave();
     spielfigur.anim=SPCANM_SIEG;
     sammel_sprites[b]=c-(SPR_GEGNER(0)-SPR_GEGNER_TOT(0));
     sp_str->sammel.revierkampf[c-SPR_GEGNER(0)]++;
    }
    else 
    {
     /*Raubtier*/
     stop_wave();
     if( ((int)(sp_str->evol.eigenschaften[EVO_VERTEID])
        >(int)qp_random((int)raubtier_angriff[raubtiere[c].typ]))||sp_str->sammel.kraftfutter)  
      /* Gewinnt der Spieler ?*/
      { /*Gewonnen*/
       spielfigur.anim=SPCANM_SIEG;
       sammel_sprites[b]=c;
       raubtiere[c].tot=1;
       raubtiere[c].anim=PRDANM_TOT1+qp_random(3);
       sp_str->sammel.revierkampf[spieler_num]++;                
      }
      else
      { /*Verloren*/
       sammel_sprites[spielfigur.ypos*100+spielfigur.xpos]=c;
       raubtiere[c].anim=PRDANM_SIEG;
       spielfigur.tot=1; 
       spielfigur.anim=SPCANM_STEHEN;
       spielfigur.frame=0;
       raubtiere[c].xpos=spielfigur.xpos;
       raubtiere[c].ypos=spielfigur.ypos;
       wen_animieren=&(raubtiere[c].frame);
      }
      raubtiere[c].frame=0;
    }
    spielfigur.frame=0;
    /*Ergebnis animieren*/
    do
    {
     t3=t1=clock();
     t3+=framezeit;
     while(game_input_ready()) entropy_add(game_input_get());
     printf("*wen_animieren=%d\n",*wen_animieren);
     win_lock();
     sammelrunde_bildaufbau((spielfigur.xpos-3)*64,
                          (spielfigur.ypos-3)*64);
     win_flush();
     while(t1<t3&&t3-t1<=framezeit)
     {
      t1=clock();
      idle_function();
     }                                                   
    }while(*wen_animieren);
    if(c==SPR_WEIBCHEN) /*Weibchen*/ 
     sammel_sprites[b]=SPR_JUNGES; /*jetzt ist der Kiwi geschlüpft*/

    scanner_zeichnen(spielfigur.xpos,spielfigur.ypos,&(sp_str->evol));

    if(spielfigur.tot)
    {
     spieler_tot(sp_str);
     break;                  
    }    
   }
  } 
#if 0 
  /*Treibsand*/
   b=spielfigur.ypos*100+spielfigur.xpos+lovewar_delta[a];
   for(a=0;a<treibsand_num;a++)
   {
    if(treibsand[a]==b)
    {
     if(!sp_str->sammel.kraftfutter) /*kein T.s. wenn Unverwundbar*/
      spielfigur.anim=SPCANM_TREIBSAND;                  
     else
      spielfigur.anim=SPCANM_KRAFT;                  
     spielfigur.frame=0;                      
     /*Animieren*/
     do
     {
      t3=t1=clock();
      t3+=framezeit;
      while(game_input_ready()) game_input_get();
      win_lock();
      sammelrunde_bildaufbau((spielfigur.xpos-3)*64,
                          (spielfigur.ypos-3)*64);
      win_flush();
      while(t1<t3&&t3-t1<=framezeit)
      {
       t1=clock();
       idle_function();
      }
     }while(spielfigur.frame);
     if(!sp_str->sammel.kraftfutter) /*kein T.s. wenn Unverwundbar*/
      spieler_tot(sp_str);
 /*    treibsand[a]=0;*/
     break;
    }   
   }
#endif      
  zeit--;
 }
 sammelrunde_auswerten(sp_str);
 welt_sterben_nach_sammeln(spieler_num);
 return 1;
}

/*Sammelrunde für Computergegner*/
int sammelrunde_ki(int spieler_num)
{
 int a,b,c;
 unsigned long nahrung;
 struct SPIELER *sp_str=NULL;

 if(spieler_num<0||spieler_num>=6) return 0;
 sp_str=spieler+spieler_num;
 memset(&(sp_str->sammel),0,sizeof(struct SAMMELRUNDE));

 if(!sp_str->evol.einheiten) return 0;
 /*Fressen*/  
 nahrung=0;
 for(a=0;a<6;a++)
  nahrung+=
    ((unsigned int)(sp_str->stell.pflanzen[a]))*
    ((unsigned int)(sp_str->evol.eigenschaften[a]));       
 nahrung/=sp_str->evol.einheiten;
 /*0-100*/
 a=(7-sp_str->evol.iq)*20;
 a+=random_div(sp_str->evol.eigenschaften[EVO_SINN],5);
 a+=random_div(sp_str->evol.eigenschaften[EVO_INT],10);
 nahrung=random_div(nahrung*a,5);
 /*0-1800*/
 if(nahrung>MAX_FOOD) nahrung=MAX_FOOD;
 sp_str->sammel.nahrung=nahrung;
 /*Sterben*/ 
 a=qp_random(sp_str->evol.einheiten)/10+5;
 while(a--)
 {
  if(qp_random(600)<sp_str->evol.eigenschaften[EVO_V]) continue;
  if(qp_random(300)<sp_str->evol.eigenschaften[EVO_TARNUNG]) continue;
  if(qp_random(1000)<sp_str->evol.eigenschaften[EVO_INT]) continue;
  if(qp_random(7)<6-sp_str->evol.iq) continue;
   if(qp_random(2000)<sp_str->evol.eigenschaften[EVO_SINN]) continue;
  if(qp_random(600)<sp_str->evol.eigenschaften[EVO_VERTEID])
   {sp_str->sammel.revierkampf[spieler_num]++; continue;}
  sp_str->sammel.tote++;
 }
 /*Paarung*/
 a=(7-sp_str->evol.iq)*2+random_div(sp_str->stell.nachbarn[spieler_num],sp_str->evol.einheiten);
 printf("a=%d ",a);
 sp_str->sammel.paarungen=qp_random(a);
 /*Revierkämpfe*/ 
 for(a=0;a<6;a++)
  if(a!=spieler_num) sp_str->sammel.revierkampf[a]=random_div(sp_str->stell.nachbarn[a],sp_str->evol.einheiten);
 sammelrunde_auswerten(sp_str);
 welt_sterben_nach_sammeln(spieler_num);
 return 1;  
}

