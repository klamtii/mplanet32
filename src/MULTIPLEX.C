#include <stddef.h>
#include <stdint.h>

/* Längen der einzelnen WAV- und MIDI-Dateien finden*/

/*Geladenen Block aus WAV-Dateien zerlegen Längen in *lens eintragen*/
int multiplex_wav(uint32_t *lens,int n_max,const void *_src,int src_len)
{
 const uint8_t *src8=(uint8_t*)_src;
 const uint32_t *src32;
 int offset=0;
 int r=0;
 if(lens==NULL||_src==NULL) return 0;
 while(offset+8<src_len&&r<n_max)
 {
  src32=(uint32_t*)(&(src8[offset]));
  if(src32[0]!='FFIR') break;
  if(src32[1]+offset+8>src_len) break;
  lens[r++]=src32[1]+8;
  offset+=src32[1]+8;
 }       
 return r;   
}

/*Geladenen Block aus MIDI-Dateien zerlegen Längen in *lens eintragen*/
int multiplex_midi(uint32_t *lens,int n_max,const void *_src,int src_len)
{
 int a,b;
 const uint8_t *src8=(uint8_t*)_src;
 uint32_t len;
 int offset=0;
 int r=0;
 if(lens==NULL||_src==NULL) return 0;
 while(offset+8<src_len&&r<n_max)
 {
  if(((uint32_t*)src8)[0]!='dhTM') break;
  len=14;
  a=src8[10];
  a=a*256+src8[11];
  while(a--)
  {
   if(len+8+offset>src_len) break;
   b=src8[len+4];         
   b=b*256+src8[len+5];
   b=b*256+src8[len+6];
   b=b*256+src8[len+7]+8;
   len+=b;
  }
  if(len+offset>src_len) break;
  lens[r++]=len;
  src8+=len;
  offset+=len;
 }       
 return r;   
}

/*Aus liste von Längen (die beiden Funktionen darüber)
 Zeiger erzeugen*/
void multiplex_pointers(void **ptrs,void *src,const uint32_t *lens,int n)
{
 uint8_t *ptr=(uint8_t*)src;
 if(n<1||ptrs==NULL||src==NULL||lens==NULL) return;
 while(n--)
 {
  *(ptrs++)=ptr;
  ptr+=*(lens++);
 }    
}
