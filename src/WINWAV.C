#include "EDITION.H"
#ifdef TARGET_WINGDI

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>

static clock_t wave_end_time=0;

static int wave_flag=1;

/*Wird gerade eine WAV-Datei abgespielt ?*/
int wave_is_playing(void)
{
 clock_t clk=clock();
 if(clk>=wave_end_time) return 0;
 if(wave_end_time-clk>CLK_TCK*10) return 0;
 return 1;
}

#pragma pack(push, 1)
struct RIFF_HEADER
{
 uint32_t sig_riff;
 uint32_t len;      
 uint32_t sig_wave;
};

struct CHUNK_HEADER
{
 uint32_t sig;
 uint32_t len;      
};

struct FMT_CHUNK
{
 uint16_t format;
 uint16_t channels;
 uint32_t sampling_rate;
 uint32_t data_rate;
 uint16_t block_align;
 uint16_t bits_per_sample;
};
#pragma pack(pop)

/*Wave-Datei Abspielen*/
int play_wave(void *dat,unsigned int len)
{
 unsigned long t=0; 
 char *cptr=(char*)dat,*endptr;
 struct RIFF_HEADER *riff_header=NULL;
 struct CHUNK_HEADER *chunk_header=NULL;
 struct FMT_CHUNK *fmt_chunk=NULL;

 if(len<32) return 1;
 /*Zeit berechnen ...*/
 riff_header=(struct RIFF_HEADER*)cptr;
 if(riff_header->sig_riff!='FFIR'||riff_header->sig_wave!='EVAW') return 2;
 if(len>riff_header->len+8) len=riff_header->len+8;
 endptr=(void*)(cptr+len);
 cptr+=12;
 while(cptr+8<endptr)
 {
  chunk_header=(struct CHUNK_HEADER*)cptr;
  if(chunk_header->sig==' tmf')
   fmt_chunk=(struct FMT_CHUNK*)(cptr+8);
  if(chunk_header->sig=='atad')
   t+=chunk_header->len; /*Bytes aller data chunks addieren*/
  cptr+=8+chunk_header->len;
 }
 if(fmt_chunk==NULL) return 3;
 if(t<1) return 4;
 if(fmt_chunk->data_rate<1) return 3; /*nicht durch 0 teilen*/
 t=(t*32)/fmt_chunk->data_rate;
 t=(t*CLK_TCK)/32; 
 if(wave_is_playing())
 {
  Sleep(10);
  while(wave_is_playing()) Sleep(20);
 }
 if(wave_flag)
  PlaySound((char*)dat,NULL,SND_MEMORY|SND_ASYNC);
 wave_end_time=clock()+t;
 return 0;
}

/*Abspielen abbrechen*/
int stop_wave(void)
{
 PlaySound(NULL,NULL,SND_MEMORY|SND_ASYNC);
 wave_end_time=clock()-1;
 return 0;   
}

/*Lautstärke setzen (eigentlich Global)*/
void set_wave_volume(unsigned short v) /*0-100*/
{
 DWORD tmp;
 tmp=v;
 tmp=(tmp*0xFFFF)/100;
 tmp|=tmp<<16;
 waveOutSetVolume((HWAVEOUT)0,tmp);
}

/*Lautstärke lesen*/
unsigned short get_wave_volume(void) /*0-100*/
{
 DWORD tmp;
 waveOutGetVolume((HWAVEOUT)0,&tmp);
/* waveOutSetVolume((HWAVEOUT)0,0x10001000);*/
 tmp=(tmp&0xFFFF)+((tmp>>16)&0xFFFF);
 tmp=(tmp*100)/0x1FFFE;
 return (unsigned short) tmp;
}

/*Wave-Ausgabe ein/ausschalten*/
void set_wave_onoff(int p)
{
 wave_flag=p;
}

/*Wave-Ausgabe ein/aus lesen*/
int get_wave_onoff(void)
{
 return wave_flag;     
}

/*Andere Threads zum Zug kommen lassen*/
void yield_thread(void)
{
 Sleep(0);    
}

/*Warten (Zeiteinheit ist CLK_TCK in time.h)*/
void clk_wait(clock_t t)
{
 Sleep(t);    
}

#endif /*TARGET_WINGDI*/
