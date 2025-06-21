#ifndef _SPIELDAT_H_
#define _SPIELDAT_H_
#include <stdint.h>

#ifndef _EDITION_H_
#include "EDITION.H"
#endif

#define SPIELFELD_X     0
#define SPIELFELD_Y     20
#define HUD_X           (6+6+447)
#define HUD_Y           20

#define RAUBTIER_MAX    0xE0

#define WAVE_MAX        32

#define FONT_NORMAL       0
#define FONT_GROSS        1
#define FONT_RUNDE        2

struct WELT
{
 uint8_t wasser;
 uint8_t temperatur;
 uint8_t feuchtigkeit;
 uint8_t runde;
 uint8_t runde_max;
 uint8_t raumschiff;
 uint8_t allein;
 uint8_t scroll;
 uint8_t auto_weiter;
 uint8_t fast_fight;
};

struct EVOLUTION
{
 uint16_t punkte_gesamt;
 uint16_t einheiten;
 uint8_t typ;
 uint8_t rang;
 uint8_t punkte_runde;
 uint8_t eigenschaften[13];
 uint8_t tot;
 uint8_t iq;
};


#define SPIELERTYP_AUS       0
#define SPIELERTYP_MENSCH    1
#define SPIELERTYP_COMPUTER  2

struct STELLUNG
{
 uint16_t pflanzen[6]; /*Einheiten auf Pflanze 0-5*/
 uint16_t nachbarn[6]; /*Nachbarschaft zu Spieler 0-5 (auch selbst (Weibchen!))*/
 uint16_t wasser;      /*Nachbarschaft zu Wasser*/
 uint16_t gebirge;     /*Nachbarschaft zu Gebirge*/
 uint16_t leer;        /*Nachbarschaft zu freien Feldern*/
};

struct SAMMELRUNDE
{
 uint16_t zeit;
 uint16_t nahrung;
 uint8_t paarungen;
 uint8_t tote;
 uint8_t revierkampf[6];
 uint8_t kraftfutter;
 uint8_t kinder;
 uint8_t bewegung;
 uint8_t sterblichkeit;
};

struct SPIELER
{
 struct EVOLUTION evol;
 struct STELLUNG stell;
 struct SAMMELRUNDE sammel;      
};

struct RAUBTIER
{
 uint8_t xpos,ypos; /*Position in Kästchen*/
 uint8_t xdst,ydst; /*Bewegungsziel*/
 int8_t xoffs,yoffs; /*Abweichung in Pixel (für Bewegung)*/
 uint8_t anim; /*Animationsnummer*/
 uint8_t frame;  /*Animationsframe*/
 uint8_t typ; 
 uint8_t tot;
 uint16_t last_frame;
};

#define tile_t uint16_t

#define TILEMASK_ANIM    0x00FF /*Nächster Frame (Animiert oder Fressen)*/
#define TILE_PFL1        0x0100 /* Pflanze 1-6*/
#define TILE_PFL2        0x0200
#define TILE_PFL3        0x0300
#define TILE_PFL4        0x0400
#define TILE_PFL5        0x0500
#define TILE_PFL6        0x0600
#define TILEDIV_PFLANZEN 0x0100
#define TILE_KRAFT       0x0700 /* Kraftfutter */
#define TILE_BERG        0x0800 /*Im Gebirge*/
#define TILE_MEER        0x0900 /*Am Meer*/
#define TILE_BASIS       0x0A00 /*Basis der Menschen (Zentral)*/
#define TILE_MENSCH      0x0B00 /*Wenn Mensch da ist (Fass)*/
#define TILE_ELEK        0x0C00 /*Elektroschlange kann erscheinen*/
#define TILE_NOGEN       0x0D00 /*Nicht zufällig erzeugen*/
#define TILE_AUGE        0x0E00 /*Auge in Q-Pop*/
#define TILEMASK_TYP     0x0F00
#define TILEF_PICKUP   0x1000 /*Kann aufgesammelt werden*/
#define TILEF_SOLID    0x2000 /*Kann nicht betreten werden*/
#define TILEF_EDGE     0x4000 /*Rand der Karte*/
#define TILEF_SOUND    0x8000 /*macht Geräusch*/


#define SPR_WEIBCHEN   0xFE
#define SPR_JUNGES     0xFF
#define SPR_GEGNER(a)     (0xF0+(a))
#define SPR_GEGNER_TOT(a) (0xF6+(a))
#define SPR_EI         0xEF /*Kiwiopterix schlüpft*/

#define anim_t uint8_t
#define ANIM_FRAME(a) (3+(a))
#define ANIM_LEN      2
#define ANIM_BITMAP   1
#define ANIM_SOUND    0

#define SPCANM_STEHEN        0 /*Muss so viel Frames wie Ei haben !*/
#define SPCANM_SUEDEN        1
#define SPCANM_NORDEN        2
#define SPCANM_WESTEN        3
#define SPCANM_OSTEN         4
#define SPCANM_PICKUP        5
#define SPCANM_TREIBSAND     6
#define SPCANM_ELEKTRO       7
#define SPCANM_SIEG          8
#define SPCANM_KOTZEN        9
#define SPCANM_KRAFT         10
#define SPCANM_WEIBCHEN      11
#define SPCANM_EI            12 /*Kiwiopterix schlüpft*/
#define SPCANM_KIND          13
#define SPCANM_FEIND         14
#define SPCANM_TOT           15
#define SPCANM_WAR_H         16
#define SPCANM_WAR_V         17
#define SPCANM_LOVE_H        18
#define SPCANM_LOVE_V        19
#define SPECIES_ANIM_NUM     20

#define PRDANM_STEHEN        0
#define PRDANM_SUEDEN        1
#define PRDANM_NORDEN        2
#define PRDANM_WESTEN        3
#define PRDANM_OSTEN         4
#define PRDANM_ANGRIFF_S     5
#define PRDANM_ANGRIFF_N     6
#define PRDANM_ANGRIFF_O     7
#define PRDANM_ANGRIFF_W     8
#define PRDANM_SIEG          9
#define PRDANM_TOT1         10
#define PRDANM_TOT2         11
#define PRDANM_TOT3         12
#define PREDATOR_ANIM_NUM   13

#define MOUSE_FLAG           0x80000000
#define MOUSE_UP             0x40000000
#define MOUSE_KEYMASK        0x0000C000
#define MOUSE_KEYSHIFT       14
#define MOUSE_KEYL           0x00000000
#define MOUSE_KEYM           0x00004000
#define MOUSE_KEYR           0x00008000
#define MOUSE_X(a)           ((a)&0x3FFF)
#define MOUSE_Y(a)           (((a)>>16)&0x3FFF)

#define EVO_PFLANZE(a)      (a)
#define EVO_VERMEHR          6
#define EVO_ANGRIFF          7
#define EVO_VERTEID          8
#define EVO_TARNUNG          9
#define EVO_V               10
#define EVO_SINN            11
#define EVO_INT             12
#define EVO_NUM             13

#define KAT_HITZE           0
#define KAT_KAELTE          1
#define KAT_METEOR          2
#define KAT_VIREN           3
#define KAT_VULKAN          4
#define KAT_FLUT            5
#define KAT_BEBEN           6
#define KAT_MENSCH          7
#define KAT_KSTRAHL         8

#ifdef GAME_MAGNETIC_PLANET
#define MAX_FOOD           800
#define FOOD_UNIT          80
#define KAT_NUM            7
#define KAT_SIEG           7
#define KAT_TOT            8
#endif

#ifdef GAME_Q_POP
#define MAX_FOOD           1600
#define FOOD_UNIT          40
#define KAT_NUM            9
#endif

#endif /*!_SPIELDAT_H_*/
