#include "SPIELDAT.H"
#include "MPBITMAP.H"

#define NUM_TILES    60

#define TPFL1 TILE_PFL1|TILEF_PICKUP
#define TPFL2 TILE_PFL2|TILEF_PICKUP
#define TPFL3 TILE_PFL3|TILEF_PICKUP
#define TPFL4 TILE_PFL4|TILEF_PICKUP
#define TPFL5 TILE_PFL5|TILEF_PICKUP
#define TPFL6 TILE_PFL6|TILEF_PICKUP
#define TBERG  TILEF_SOLID|TILE_BERG
#define TMEER  TILEF_SOLID|TILE_MEER

tile_t tile_info[NUM_TILES+1]=
{
/*0*/  TILE_NOGEN,
/*1*/  TILE_ELEK,             1|TPFL1,       2|TPFL1,       3|TPFL1,      4|TPFL1, 
/*6*/  5|TPFL1,       1|TPFL2,       7|TPFL2,       8|TPFL2,       9|TPFL2, 
/*11*/ 10|TPFL2,      1|TPFL3,      12|TPFL3,      13|TPFL3,      14|TPFL3, 
/*16*/ 15|TPFL3,      1|TPFL4,      17|TPFL4,      18|TPFL4,      19|TPFL4, 
/*21*/ 20|TPFL4,      1|TPFL5,      22|TPFL5,      23|TPFL5,      24|TPFL5, 
/*26*/ 25|TPFL5,      1|TPFL6,      27|TPFL6,      28|TPFL6,      29|TPFL6, 

/*31*/  30|TPFL6,
  TILEF_SOLID,
  TBERG|TILEF_EDGE,
  TBERG|TILEF_EDGE,
  TBERG|TILEF_EDGE,

/*36*/  TBERG,        TILEF_SOLID,   TMEER,       TILE_BERG,    TILEF_SOLID,
/*41*/  TILEF_SOLID,  TBERG,        TILEF_SOLID, TILEF_SOLID,TBERG|TILEF_EDGE,

/*46*/  TMEER, 
  TBERG|TILEF_EDGE,
  TILEF_PICKUP|TILE_KRAFT|56,
  TILEF_SOLID|TILEF_EDGE|TILE_NOGEN,
  TILE_BERG,
  
/*51*/  TILEF_PICKUP|51,TILEF_PICKUP|52,TILEF_PICKUP|53,TILE_ELEK,TILE_ELEK,
/*56*/  TILE_NOGEN,TILE_NOGEN,TILE_NOGEN,TILE_NOGEN,TILE_NOGEN  
};

#undef TPFL1
#undef TPFL2
#undef TPFL3
#undef TPFL4
#undef TPFL5
#undef TPFL6
#undef TBERG
#undef TMEER

static anim_t roboter_steht[]={0,BM_SPECIES1,3,1,1,1};
static anim_t roboter_sueden[]={0,BM_SPECIES1,4,2,3,4,5};
static anim_t roboter_norden[]={0,BM_SPECIES1,4,6,7,8,9};
static anim_t roboter_westen[]={0,BM_SPECIES1,4,10,11,12,13};
static anim_t roboter_osten[]= {0,BM_SPECIES1,4,14,15,16,17};
static anim_t roboter_pickup[]= {SOUND_PICKUP,BM_SPECIES1,8,20,20,18,18,19,19,20,20};
static anim_t roboter_falltuer[]=
 {0,BM_SPECIES1,11,21,22,23,24,25,26,27,28,29,30,31};
static anim_t roboter_sieg[]={SOUND_SIEG,BM_SPECIES1,12,32,33,34,35,36,37,32,33,34,35,36,37};
static anim_t roboter_oel[]={SOUND_OEL,BM_SPECIES1,16,38,39,40,41,42,43,44,45,38,39,40,41,42,43,44,45};
static anim_t roboter_diskette[]={SOUND_DISKETTE,BM_SPECIES1,10,46,1,46,1,46,1,46,1,46,1};
static anim_t roboter_kind[]={0,BM_SPECIES1,3,47,48,49};
static anim_t roboter_baukasten[]={0,BM_SPECIES1,1,50};
static anim_t roboter_tot[]={0,BM_TOT,1,1};
static anim_t roboter_war_h[]={SOUND_KAMPF_S,BM_LOVEWAR,23,
 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,5,6,7};
static anim_t roboter_war_v[]={SOUND_KAMPF_S,BM_LOVEWAR,23,
 9,10,11,12,9,10,11,12,9,10,11,12,9,10,11,12,9,10,11,12,13,14,15};
static anim_t roboter_love_h[]={SOUND_BAUSATZ,BM_LOVEWAR,23,
 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4,5,6,7};
static anim_t roboter_love_v[]={SOUND_BAUSATZ,BM_LOVEWAR,23,
 9,10,11,12,9,10,11,12,9,10,11,12,9,10,11,12,9,10,11,12,13,14,15};

anim_t *(species_anim[SPECIES_ANIM_NUM])=
{
 roboter_steht,roboter_sueden,roboter_norden,roboter_westen,
 roboter_osten,roboter_pickup,roboter_falltuer,roboter_falltuer,
 roboter_sieg,roboter_oel,roboter_diskette,roboter_baukasten,
 roboter_kind,roboter_kind,roboter_steht,roboter_tot,
 roboter_war_h,roboter_war_v,roboter_love_h,roboter_love_v
};

/*Bitmaps für verschiedene Roboter umfärben*/
void roboter_farbe_setzen(int f)
{
 int a;
 static int f_alt=-1;
 if(f==f_alt) return;
 f_alt=f;
/* roboter_steht[ANIM_FRAME(0)]=1+f;*/
 for(a=SPCANM_STEHEN;a<SPCANM_FEIND;a++)
  species_anim[a][ANIM_BITMAP]=BM_SPECIES1+f;
 if(f)
 {
  roboter_tot[ANIM_BITMAP]=BM_TOT;
  roboter_tot[ANIM_FRAME(0)]=f;     
 }
 else
 {
  roboter_tot[ANIM_BITMAP]=BM_TOT_X;
  roboter_tot[ANIM_FRAME(0)]=3;     
 }
}


static anim_t hund_steht[]={0,BM_PREDATOR1,1,1};
static anim_t hund_osten[]={0,BM_PREDATOR1,4,2,3,4,5};
static anim_t hund_westen[]={0,BM_PREDATOR1,4,6,7,8,9};
static anim_t hund_norden[]={0,BM_PREDATOR1,4,10,11,12,13};
static anim_t hund_sueden[]={0,BM_PREDATOR1,4,14,15,16,17};
static anim_t hund_sieg[]={SOUND_ROBODOG,BM_PREDATOR1,8,18,19,18,19,18,19,18,19};
static anim_t hund_tot[]={0,BM_PREDATOR1,1,20};
static anim_t hund_ang_w[]={0,BM_PREDATOR1,4,21,21,22,22};
static anim_t hund_ang_s[]={0,BM_PREDATOR1,4,23,23,24,24};
static anim_t hund_ang_n[]={0,BM_PREDATOR1,4,25,25,26,26};
static anim_t hund_ang_o[]={0,BM_PREDATOR1,4,1,1,27,27};

static anim_t hydrant_steht[]={0,BM_PREDATOR2,1,1};
static anim_t hydrant_westen[]={0,BM_PREDATOR2,5,2,3,4,5,6};
static anim_t hydrant_osten[]={0,BM_PREDATOR2,5,7,8,9,10,11};
static anim_t hydrant_sueden[]={0,BM_PREDATOR2,5,13,14,15,16,17};
static anim_t hydrant_norden[]={0,BM_PREDATOR2,5,19,20,21,22,23};
static anim_t hydrant_sieg[]={SOUND_HYDRANT,BM_PREDATOR2,8,26,27,26,28,26,27,26,28};
static anim_t hydrant_tot[]={0,BM_PREDATOR2,1,25};
static anim_t hydrant_ang_w[]={0,BM_PREDATOR2,2,1,1};
static anim_t hydrant_ang_o[]={0,BM_PREDATOR2,2,11,11};
static anim_t hydrant_ang_s[]={0,BM_PREDATOR2,2,18,18};
static anim_t hydrant_ang_n[]={0,BM_PREDATOR2,2,24,24};

anim_t *(raubtier_anim[PREDATOR_ANIM_NUM*2])=
{
 hund_steht,hund_sueden,hund_norden,hund_westen,
 hund_osten,hund_ang_s,hund_ang_n,hund_ang_o,
 hund_ang_w,hund_sieg,hund_tot,hund_tot,
 hund_tot,

 hydrant_steht,hydrant_sueden,hydrant_norden,hydrant_westen,
 hydrant_osten,hydrant_ang_s,hydrant_ang_n,hydrant_ang_o,
 hydrant_ang_w,hydrant_sieg,hydrant_tot,hydrant_tot,
 hydrant_tot
};

