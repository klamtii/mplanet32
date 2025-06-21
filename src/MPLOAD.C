#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <mem.h>
#include "EDITION.H"
#ifdef GAME_MAGNETIC_PLANET
#include "SPIELDAT.H"
#include "SYSTEMIO.H"
#include "BMCACHE.H"
#include "MPBITMAP.H"
#include "MULTIPLEX.H"

#define LOCAL static _stdcall

static unsigned int error_line=5;

static int printf_error(const char *fstr,...)
{
 int r;
 va_list va;
 char *buffer;     
 if(fstr==NULL) return 0;
 if(!*fstr) return 0; 
 buffer=malloc(256+strlen(fstr)*2);
 if(buffer!=NULL)
 {
  va_start(va,fstr);  
  vsprintf(buffer,fstr,va);
  va_end(va);
  shadow_print(10,error_line,620,18,buffer,0);
  free(buffer);
  error_line+=15;
  win_flush();
 }
 return r;
}

void *intro_video=NULL;

uint32_t wav_lens[WAVE_MAX];
void *wav_ptrs[WAVE_MAX];
int num_wav=0;

static uint32_t mp_palette1[256]=
{0x0,0x800000,0x8000,0x808000,0x80,0x800080,0x8080,0xC0C0C0,
0x69696D,0xA6CAF0,0x101010,0x202020,0x303030,0x404040,0x505050,0x606060,
0x707070,0x909090,0xA0A0A0,0xB0B0B0,0xD0D0D0,0xE0E0E0,0xF0F0F0,0x4C281C,
0x4C1C28,0x284C1C,0x1C4C28,0x281C4C,0x1C284C,0x6C5820,0xA07C58,0xE8A868,
0x543010,0x5C3828,0x202010,0x482010,0x702010,0x982010,0xC02010,0xE82010,
0x202048,0x482048,0x702048,0x982048,0xC02048,0xE82048,0x202080,0x482080,
0x702080,0x982080,0xC02080,0xE82080,0x2020B8,0x4820B8,0x7020B8,0x9820B8,
0xC020B8,0xE820B8,0x2020F0,0x4820F0,0x7020F0,0x9820F0,0xC020F0,0xE820F0,
0x7C4C30,0x8C5438,0x204810,0x484810,0x704810,0x984810,0xC04810,0xE84810,
0x204848,0x484848,0x704848,0x984848,0xC04848,0xE84848,0x204880,0x484880,
0x704880,0x984880,0xC04880,0xE84880,0x2048B8,0x4848B8,0x7048B8,0x9848B8,
0xC048B8,0xE848B8,0x2048F0,0x4848F0,0x7048F0,0x9848F0,0xC048F0,0xE848F0,
0x985C40,0xA8644C,0x207010,0x487010,0x707010,0x987010,0xC07010,0xE87010,
0x207048,0x487048,0x707048,0x987048,0xC07048,0xE87048,0x207080,0x487080,
0x707080,0x987080,0xC07080,0xE87080,0x2070B8,0x4870B8,0x7070B8,0x9870B8,
0xC070B8,0xE870B8,0x2070F0,0x4870F0,0x7070F0,0x9870F0,0xC070F0,0xE870F0,
0xB07460,0xBC8868,0x209810,0x489810,0x709810,0x989810,0xC09810,0xE89810,
0x209848,0x489848,0x709848,0x989848,0xC09848,0xE89848,0x209880,0x489880,
0x709880,0x989880,0xC09880,0xE89880,0x2098B8,0x4898B8,0x7098B8,0x9898B8,
0xC098B8,0xE898B8,0x2098F0,0x4898F0,0x7098F0,0x9898F0,0xC098F0,0xE898F0,
0xC8906C,0xD0A074,0x20C010,0x48C010,0x70C010,0x98C010,0xC0C010,0xE8C010,
0x20C048,0x48C048,0x70C048,0x98C048,0xC0C048,0xE8C048,0x20C080,0x48C080,
0x70C080,0x98C080,0xC0C080,0xE8C080,0x20C0B8,0x48C0B8,0x70C0B8,0x98C0B8,
0xC0C0B8,0xE8C0B8,0x20C0F0,0x48C0F0,0x70C0F0,0x98C0F0,0xC0C0F0,0xE8C0F0,
0xD8A880,0xE0B090,0x20E810,0x48E810,0x70E810,0x98E810,0xC0E810,0xE8E810,
0x20E848,0x48E848,0x70E848,0x98E848,0xC0E848,0xE8E848,0x20E880,0x48E880,
0x70E880,0x98E880,0xC0E880,0xE8E880,0x20E8B8,0x48E8B8,0x70E8B8,0x98E8B8,
0xC0E8B8,0xE8E8B8,0x20E8F0,0x48E8F0,0x70E8F0,0x98E8F0,0xC0E8F0,0xE8E8F0,
0xE8C09C,0xF0D4B8,0x3800,0x6000,0xA000,0xD000,0xD0D000,0xA0A000,
0x606000,0x383800,0x380000,0x600000,0xA00000,0xD00000,0xD000D0,0xA000A0,
0x600060,0x380038,0x38,0x60,0xA0,0xD0,0xCFCFD1,0xA0A0A4,
0x808080,0xFF0000,0xFF00,0xFFFF00,0xFF,0xFF00FF,0xFFFF,0xFFFFFF};

static uint32_t mp_palette2[256];
static uint32_t mp_palette3[256];
static uint32_t mp_palette4[256];
static uint32_t mp_palette5[256];
static uint32_t mp_palette6[256];

static uint32_t *(pals[6])={mp_palette1,mp_palette2,mp_palette3,mp_palette4,mp_palette5,mp_palette6};

#define CSUC const static unsigned char

#define SWPCOL      8
CSUC pal_swap1[SWPCOL]={45,141,38,109/*!*/,103,36,77,37};
CSUC pal_swap2[SWPCOL]={228,209,227,203,203,226,203,66};
CSUC pal_swap3[SWPCOL]={251,217/*!*/,135,251/*!*/,251,103,251,135};
CSUC pal_swap4[SWPCOL]={117,188,79,157,157,40,189,79};
CSUC pal_swap5[SWPCOL]={20,255/*!*/,18,22,22/*!*/,34,22,18};
CSUC pal_swap6[SWPCOL]={13,17,11,15,15,0/*!*/,30,11};
static char pal_ok[6]={1,0,0,0,0,0};
CSUC *(pal_swaps[6])={pal_swap1,pal_swap2,pal_swap3,pal_swap4,pal_swap5,pal_swap6};

static const char txt_datei_fehlt[]="FEHLER:Kann %s nicht laden!\n";

static struct DIB_HEADER dib_icon=
{sizeof(struct DIB_HEADER),8,-52,1,8,0,10*52,1024,1024,256,256};

static uint8_t icon_sanduhr[10*52]=
{
 0xFF,0x00, 0x00,0x00,0x00,0x00, 0x00,0xFF,
 0xFF,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0xFF,
 0xFF,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0xFF,
 0xFF,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0xFF,

 0xFF,0xFF, 0x00,0xFF,0xFF,0x00, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0x00,0xFF,0xFF,0x00, 0xFF,0xFF,

 0xFF,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0xFF,
 0xFF,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0xFF,
 0xFF,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0xFF,
 0xFF,0x00, 0x00,0x00,0x00,0x00, 0x00,0xFF,
/*};

static uint8_t icon_bildschirm[12*12]=
{*/
#define WHITE_LINE 0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF
 WHITE_LINE,WHITE_LINE,WHITE_LINE,WHITE_LINE,
 WHITE_LINE,WHITE_LINE,WHITE_LINE,WHITE_LINE,

 0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,
 0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,
 0x00,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0x00,

 0x00,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0x00,
 0x00,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0x00,
 0x00,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0x00,
 0x00,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0x00,

 0x00,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0x00,
 0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,
 0xFF,0x00, 0x00,0x00,0x00,0x00, 0x00,0xFF,
/*};

static uint8_t icon_hilfe[12*12]=
{*/
 WHITE_LINE,WHITE_LINE,WHITE_LINE,WHITE_LINE,
 WHITE_LINE,WHITE_LINE,WHITE_LINE,WHITE_LINE,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0x00,0x00,0x00,0x00, 0xFF,0xFF,
 0xFF,0x00, 0x00,0xFF,0xFF,0x00, 0x00,0xFF,
 0x00,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0x00,

 0x00,0x00, 0xFF,0xFF,0xFF,0xFF, 0x00,0x00,
 0xFF,0xFF, 0xFF,0xFF,0xFF,0x00, 0x00,0xFF,
 0xFF,0xFF, 0xFF,0xFF,0x00,0x00, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,

 0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0x00,0x00,0xFF, 0xFF,0xFF,
 0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,
};



LOCAL int _load_file(char **ptr,int lz,char *name,unsigned int len)
{
 uint32_t load_len=0;
 printf("%s\n",name);
 if(lz)
  *ptr=(char*)load_file_lzexpand(&load_len,name);
 else
  *ptr=(char*)load_file_stdio(&load_len,name);
 if(*ptr==NULL||load_len==0)
 {
  printf_error(txt_datei_fehlt,name);
  return 1;
 }
 if(load_len<len)
 {
  printf_error("FEHLER:%s ist zu kurz (%u byte statt %u byte)!\n",name,(unsigned int)load_len,len);
  return 2;
 } 
 return 0;
}

LOCAL int _register_bitmap40(int n,unsigned int npal,char *hdr)
{
 if(npal>5) npal=0;
 if(hdr==NULL) return;
 return register_bitmap(n,hdr,pals[npal],hdr+40,NULL,NULL,NULL);
}

static char *mplanet_exe=NULL;
static char *back_dat=NULL;
static char *backgrou_dat=NULL;
static char *bewert_dat=NULL;
static char *entwickl_dat=NULL;
static char *init_dat=NULL;
static char *lovewar_dat=NULL;
static char *meer_dat=NULL;
static char *music_dat=NULL;
static char *rand_dat=NULL;
static char *stand_dat=NULL;
static char *tot_dat=NULL;
static char *wave_dat=NULL;
static char *species_0=NULL;
static char *species_1=NULL;
static char *predator_1=NULL;
static char *predator_1m=NULL;
static char *predator_2=NULL;
static char *predator_2m=NULL;

static char *(katastrophen)[9]=
{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

/*Katastrophen (Standbilder) umfärben*/
void recolour_disasters(unsigned int n)
{
 unsigned int a;
 for(a=0;a<9;a++)
  _register_bitmap40(BM_KAT_HITZE+a,n,katastrophen[a]);     
/* register_bitmap(BM_WINDOW_INIT,init_dat,pals[n],init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);*/
}

/*Schriftarten festlegen*/
void init_fonts(void)
{
 alloc_font(FONT_NORMAL,"Arial",8,14,1);
 alloc_font(FONT_RUNDE,"Times New Roman",10,20,0);
 alloc_font(FONT_GROSS,"Times New Roman",20,40,1);     
}

/*Alle Dateien für Magnetic Planet laden*/
int load_magnetic_planet(void)
{
 unsigned int a,b;
 int r=0;
 uint32_t midi_len=0,wav_len=0;
 
 init_fonts();
#define LOAD_FILE_LZEX(pointer,name,len) r|=_load_file((pointer),1,(name),(len))
#define LOAD_FILE_STDIO(pointer,name,len) r|=_load_file((pointer),0,(name),(len))
 LOAD_FILE_STDIO(&mplanet_exe,"MPLANET.EXE",519168);
 LOAD_FILE_LZEX(&back_dat,"BACK.DAT",1178 );
 LOAD_FILE_LZEX(&backgrou_dat,"BACKGROU.DAT",55840);
 LOAD_FILE_LZEX(&bewert_dat,"BEWERT.DAT",22343);
 LOAD_FILE_LZEX(&entwickl_dat,"ENTWICKL.DAT",10811);
 LOAD_FILE_LZEX(&init_dat,"INIT.DAT",7033 );
 LOAD_FILE_LZEX(&lovewar_dat,"LOVEWAR.DAT",19131);
 LOAD_FILE_LZEX(&meer_dat,"MEER.DAT",14233);
 LOAD_FILE_LZEX(&rand_dat,"RAND.DAT",16500);
 LOAD_FILE_LZEX(&stand_dat,"STAND.DAT",4901);
 LOAD_FILE_LZEX(&tot_dat,"TOT.DAT",4797);
 LOAD_FILE_LZEX(&species_0,"SPECIES.0",14617);
 LOAD_FILE_LZEX(&species_1,"SPECIES.1",36253);
 LOAD_FILE_LZEX(&predator_1,"PREDATOR.1",24752);
 LOAD_FILE_LZEX(&predator_1m,"PREDATOR.1M",8773);
 LOAD_FILE_LZEX(&predator_2,"PREDATOR.2",20929);
 LOAD_FILE_LZEX(&predator_2m,"PREDATOR.2M",8315);
 LOAD_FILE_LZEX(&(katastrophen[0]),"KATASTR.0",12567);
 LOAD_FILE_LZEX(&(katastrophen[1]),"KATASTR.1",11273);
 LOAD_FILE_LZEX(&(katastrophen[2]),"KATASTR.2",15762);
 LOAD_FILE_LZEX(&(katastrophen[3]),"KATASTR.3",12973);
 LOAD_FILE_LZEX(&(katastrophen[4]),"KATASTR.4",12642);
 LOAD_FILE_LZEX(&(katastrophen[5]),"KATASTR.5",11338);
 LOAD_FILE_LZEX(&(katastrophen[6]),"KATASTR.6",12424);
 LOAD_FILE_LZEX(&(katastrophen[7]),"KATASTR.7",18300);
 LOAD_FILE_LZEX(&(katastrophen[8]),"KATASTR.8",14950);
 wave_dat=(char*)load_file_stdio(&wav_len,"WAVE.DAT");
 if(wave_dat==NULL||wav_len<8)
 {
  printf_error(txt_datei_fehlt,"WAVE.DAT");
  r|=1;
 }
 music_dat=(char*)load_file_lzexpand(&midi_len,"MUSIC.DAT");
 if(music_dat==NULL||midi_len<8)
 {
  printf_error(txt_datei_fehlt,"MUSIC.DAT");
  r|=1;
 }
#undef LOAD_FILE_LZEX 
#undef LOAD_FILE_STDIO
 if(r&1)
 {
  printf_error("Spiel kann nicht gestartet werden - mindestens eine Datei fehlt!\n");
  return 1;
 }
 if(r)
 {
  printf_error("Mindestens eine Datei ist zu kurz\n");
  return 2;
 }
#define REGBM(n,d) _register_bitmap40((n),0,(d))
 {
  unsigned int c,d,npal;
  for(npal=1;npal<6;npal++)
  {   
   memcpy(pals[npal],pals[0],256*4);
   for(a=0;a<SWPCOL;a++)
   {
    b=pals[0][pal_swaps[npal][a]];
    c=pals[0][pal_swaps[0][a]];
    d=pal_swaps[npal][a];
    if(d!=0&&d!=251&&d!=255&&d!=217/*&&d!=109*/&&d!=22&&d!=157) pals[npal][pal_swaps[npal][a]]=c;
    d=pal_swaps[0][a];
    if(d!=0&&d!=251&&d!=255&&d!=217/*&&d!=109*/&&d!=22&&d!=157) pals[npal][pal_swaps[0][a]]=b;
   }
  }
 }
 REGBM(BM_WINDOW_MAIN,back_dat);
/* set_bitmap_scaling(BM_WINDOW_MAIN,0);*/
/* REGBM(BM_WINDOW_INIT,init_dat);*/
 register_bitmap(BM_WINDOW_INIT0,init_dat,mp_palette1,init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);
 register_bitmap(BM_WINDOW_INIT1,init_dat,mp_palette2,init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);
 register_bitmap(BM_WINDOW_INIT2,init_dat,mp_palette3,init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);
 register_bitmap(BM_WINDOW_INIT3,init_dat,mp_palette4,init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);
 register_bitmap(BM_WINDOW_INIT4,init_dat,mp_palette5,init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);
 register_bitmap(BM_WINDOW_INIT5,init_dat,mp_palette6,init_dat+40,
          mplanet_exe+0x78F00,mplanet_exe+0x78F00+40,mplanet_exe+0x78F00+40+16*4);
 REGBM(BM_MEER_DAT,meer_dat);
 set_bitmap_tilesize(BM_MEER_DAT,8);
 REGBM(BM_TILESET_WORLD,meer_dat);
/* REGBM(BM_BEWERT_DAT,bewert_dat);*/
 register_bitmap(BM_BEWERT_DAT,bewert_dat,mp_palette1,bewert_dat+40,
       mplanet_exe+0x60E00,mplanet_exe+0x60E00+40,mplanet_exe+0x60E00+40+16*4);
 REGBM(BM_RAND_DAT,rand_dat);
 REGBM(BM_TILESET_SURVIVAL,backgrou_dat);
/* set_bitmap_tilesize(BM_TILESET_SURVIVAL,64);*/
 register_bitmap(BM_PREDATOR1,predator_1,mp_palette1,predator_1+40,
       predator_1m+14,predator_1m+14+40,predator_1m+14+40+16*4);
 register_bitmap(BM_PREDATOR2,predator_2,mp_palette1,predator_2+40,
       predator_2m+14,predator_2m+14+40,predator_2m+14+40+16*4);
 recolour_disasters(0); 
/* _register_bitmap40(BM_TOT,0,tot_dat);
 _register_bitmap40(BM_TOT_X,3,tot_dat);*/
 register_bitmap(BM_TOT,tot_dat,mp_palette1,tot_dat+40,
              mplanet_exe+0x57200,mplanet_exe+0x57200+40,mplanet_exe+0x57200+104);                        
 register_bitmap(BM_TOT_X,tot_dat,mp_palette4,tot_dat+40,
              mplanet_exe+0x57200,mplanet_exe+0x57200+40,mplanet_exe+0x57200+104);                        
#define REGBM16(a,b) register_bitmap((a),(b),(b)+40,(b)+104,NULL,NULL,NULL)
 REGBM16(BM_SCANNER_MASKE,mplanet_exe+0x75400);
 REGBM16(BM_SCANNER_BLATT,mplanet_exe+0x78B00);
 REGBM16(BM_SCANNER_HERZ,mplanet_exe+0x78C00);
 REGBM16(BM_SCANNER_TOT,mplanet_exe+0x78D00);
 REGBM16(BM_SCANNER_FEIND,mplanet_exe+0x78E00);
 register_bitmap(BM_STAND_DAT,stand_dat,mp_palette1,stand_dat+40,
              mplanet_exe+0x49600,mplanet_exe+0x49600+40,mplanet_exe+0x49600+104);
 register_bitmap(BM_LOVEWAR,lovewar_dat,mp_palette1,lovewar_dat+40,
                 mplanet_exe+0x49F00,mplanet_exe+0x49F00+40,mplanet_exe+0x49F00+40+64);
/* REGBM16(BM_TEST,mplanet_exe+0x38D0E);
 register_bitmap(BM_TEST,mplanet_exe+0x38D0E,mplanet_exe+0x38D0E +40,mplanet_exe+0x38D0E +40+1024,NULL,NULL,NULL);*/
/* REGBM(BM_TEST,mplanet_exe+0x38D00);*/
 REGBM16(BM_MAUSFINGER,mplanet_exe+0x48C04);
 REGBM16(BM_PLUS,mplanet_exe+0x48E00);
 REGBM16(BM_PLUSX,mplanet_exe+0x48F00);
 REGBM16(BM_MINUS,mplanet_exe+0x49000);
 REGBM16(BM_MINUSX,mplanet_exe+0x49100);
/* REGBM16(BM_DNS,mplanet_exe+0x49200);
 REGBM16(BM_DNS_MASKE,mplanet_exe+0x49300);*/
 register_bitmap(BM_DNS,mplanet_exe+0x49200,mplanet_exe+0x49200+40,mplanet_exe+0x49200+40+4*16,
  mplanet_exe+0x49300,mplanet_exe+0x49300+40,mplanet_exe+0x49300+40+4*16);
/* REGBM16(BM_KRANZ,mplanet_exe+0x49400);
 REGBM16(BM_KRANZ_MASKE,mplanet_exe+0x49500);*/
 register_bitmap(BM_KRANZ,mplanet_exe+0x49400,mplanet_exe+0x49400+40,mplanet_exe+0x49400+40+4*16,
  mplanet_exe+0x49500,mplanet_exe+0x49500+40,mplanet_exe+0x49500+40+4*16);
 REGBM16(BM_STAND_MASKE,mplanet_exe+0x49600);
/* REGBM16(BM_LOVEWAR_MASKE,mplanet_exe+0x49F00);
 REGBM16(BM_TOT_MASKE,mplanet_exe+0x57200);*/
 REGBM16(BM_WASS_MASKE,mplanet_exe+0x57B00);
 REGBM16(BM_WASSER,mplanet_exe+0x58400);
 REGBM16(BM_WASS_FRONT,mplanet_exe+0x58D00);
 REGBM16(BM_WASS_FRONT_MASKE,mplanet_exe+0x5B100);
 REGBM16(BM_TEMP_MASKE,mplanet_exe+0x59600);
 REGBM16(BM_TEMPERATUR,mplanet_exe+0x59F00);
 REGBM16(BM_TEMP_FRONT,mplanet_exe+0x5A800);
/* REGBM16(BM_KREUZ,mplanet_exe+0x5BD00);
 REGBM16(BM_KREUZ_MASKE,mplanet_exe+0x5BE00);*/
 register_bitmap(BM_KREUZ,mplanet_exe+0x5BD00,mplanet_exe+0x5BD00+40,mplanet_exe+0x5BD00+40+64,
                 mplanet_exe+0x5BE00,mplanet_exe+0x5BE00+40,mplanet_exe+0x5BE00+40+64);
 register_bitmap(BM_ENTWICKL_DAT,entwickl_dat,mp_palette1,entwickl_dat+40,
                 mplanet_exe+0x5BF00,mplanet_exe+0x5BF00+40,mplanet_exe+0x5BF00+40+64);
/* REGBM16(BM_ENTWICK_MASKE,mplanet_exe+0x5BF00);*/
 REGBM16(BM_LAUTSPRECHER,mplanet_exe+0x60D00);
 REGBM16(BM_NOTE,mplanet_exe+0x5BB00);
 REGBM16(BM_MINI,mplanet_exe+0x5BA00);
 REGBM16(BM_UNBEKANNT1,mplanet_exe+0x60C00);
 REGBM16(BM_SCHLIESEN,mplanet_exe+0x60B00);
 register_bitmap(BM_SANDUHR,&dib_icon,mp_palette1,icon_sanduhr,NULL,NULL,NULL);
/* register_bitmap(BM_BILDSCHIRM,&dib_icon,mp_palette1,icon_bildschirm,NULL,NULL,NULL);
 register_bitmap(BM_HILFE,&dib_icon,mp_palette1,icon_hilfe,NULL,NULL,NULL);*/

 /*Animation : MPLANET.EXE +0x28D00 ... 0x2A800*/
 intro_video=mplanet_exe+0x28D00;



 
/*extern int register_BX(unsigned int n,void *hdr,const char *name);*/

#define REG_BX(a,b,c,d) register_BX(a,mplanet_exe+14+(b*256),c,d);

/* REG_BX(BM_STARS,0x36A,"STARS");*/
/* REG_BX(BM_STARS,0x36A00);*/
 register_bitmap(BM_STARS,mplanet_exe+14+0x36A00,mp_palette1,mplanet_exe+14+0x36E4A,NULL,NULL,NULL);
 set_bitmap_name(BM_STARS,"STARS");
 REG_BX(BM_BUM,0x296,"BUM",255);
 REG_BX(BM_SCHRIFT,0x322,"SCHRIFT",253); 
 REG_BX(BM_KIWI,0x2B2,"KIWI",255); 
 REG_BX(BM_SCHIFF,0x2f2,"SCHIFF",255);
 REG_BX(BM_STERN,0x386,"STERN",250); 
 REG_BX(BM_TITEL,0x38D,"TITELMP",252);
 REG_BX(BM_FLASH,0x2A8,"FLASH",252);
 
/* REG_BX(BM_SCHRIFT,0x38D00);
 set_bitmap_name(BM_SCHRIFT,"SCHRIFT");*/
 
 for(a=0;a<6;a++)
  register_bitmap(BM_SPECIES1+a,species_1,pals[a],species_1+40,
       species_0+14,species_0+14+40,species_0+14+40+16*4);
 {
  num_wav=multiplex_wav(wav_lens,10,wave_dat,wav_len);
  multiplex_pointers(wav_ptrs,wave_dat,wav_lens,num_wav);
 }
 {
  uint32_t mid_lens[8];
  void *mid_ptrs[8];
  a=multiplex_midi(mid_lens,8,music_dat,midi_len);
  multiplex_pointers(mid_ptrs,music_dat,mid_lens,a);
  for(b=0;b<a;b++)
   register_song(b,mid_ptrs[b],mid_lens[b]);
 }
 init_fonts(); 
 
 return 0;
}

void make_mp16_species(void)
{
 int a,b,c;
 FILE *file;
 char *sp1_dat=NULL;
 char *cptr;
 char f;
 uint32_t sp1_len;
 char puffer[12]="SPECIES.1";
 
 printf("Lade %s",puffer);

 if(_load_file(&sp1_dat,1,puffer,36253))
 {
  if(sp1_dat!=NULL) free(sp1_dat);
  return;                                            
 }
 printf("OK\n");
 for(a=1;a<6;a++)
 {
  puffer[8]++;
  printf("Erzeuge %s:",puffer);
  file=fopen(puffer,"wb");
#if BUFSIZ<4096
  setvbuf(file,NULL,_IOFBF,4096);
#endif  
  if(file==NULL)
  {
   printf("Kann Datei nicht anlegen\n");
   continue;
  }
  fwrite(sp1_dat,1,40,file);
  cptr=sp1_dat+40;
  for(b=36253-40;b>0;b--)
  {
   f=*(cptr++);
   for(c=0;c<SWPCOL;c++)
    if(pal_swaps[0][c]==f)
     {f=pal_swaps[a][c];break;}
   fwrite(&f,1,1,file);                          
  }  
  fclose(file);  
  printf("OK\n");
 }
 free(sp1_dat);
}
#endif /*GAME_MAGNETIC_PLANET*/
