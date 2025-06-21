
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <mem.h>
#include <malloc.h>
#include <dir.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "EDITION.H"
#ifdef TARGET_WINGDI

#include <windows.h>
#include <lzexpand.h>

static int expand_puffer(void **p,DWORD l0,DWORD l)
{
 l0=(l0+511)&(~511);
 l=(l+511)&(~511);   
 if(l>l0)
 {
  if(*p!=NULL)
  {
   void *vptr=*p;
   *p=realloc(*p,l);
   if(*p==NULL) {free(vptr);*p=NULL;}
  }
  else *p=malloc(l);
 }
 if(*p==NULL) return 0;
 return 1;   
}

void print_errno(void)
{
 int e=errno;
 fprintf(stderr,"Fehler %d:%s!\n",e,strerror(e));
}

/*Datei mit stdio.h laden*/
void *load_file_stdio(uint32_t *len,const char *name)
{
 UINT a=64;
 DWORD l=0;
 FILE *file=NULL;
 BYTE *r=NULL;
 BYTE puffer[64]; 

 file=fopen(name,"rb");
 if(file==NULL)
 {
  fprintf(stderr,"Kann %s nicht öffnen!\n",name);
  print_errno();
  return NULL;              
 }
#if BUFSIZ<4096
 setvbuf(file,NULL,_IOFBF,4096);
#endif
 while(a==64)
 {
  a=fread(puffer,1,64,file);
  if(a>0)
  {
   if(expand_puffer((void**)&r,l,l+a))
   {
    memcpy(&(r[l]),puffer,a);
    l+=a;
   }
   else {a=0;break;}
  }
 }
 fclose(file);
 if(len!=NULL) *len=l;
 return r;
}

void print_lzexpand_error(int e)
{
 const char *cptr="Unbekannter Fehler";
 switch(e)
 {
  case LZERROR_BADINHANDLE: cptr="Ungültiges Lesehandle";break;
  case LZERROR_BADOUTHANDLE:	cptr="Ungültiges Schreibhandle";break;
  case LZERROR_READ:	cptr="Lesen Fehlgeschlagen";break;
  case LZERROR_WRITE: cptr="Schreiben Fehlgeschlagen";break;
  case LZERROR_GLOBALLOC: cptr="Nicht genug Arbeitsspeicher";break;
  case LZERROR_GLOBLOCK:	cptr="Semaphor gesperrt";break;
  case LZERROR_BADVALUE:	cptr="Ungültiger Wert";break;
  case LZERROR_UNKNOWNALG: cptr="Unbekannter Algoritmus";break;
 }
 fprintf(stderr,"LZEXPAND.DLL - Fehler %d:%s!\n",e,cptr);     
}

/*Datei mit lzexpand.h laden*/
void *load_file_lzexpand(uint32_t *len,/*const*/ char *name)
{
 UINT a=64;
 OFSTRUCT ofs;
 int lzhandle=0;
 DWORD l=0;
 BYTE *r=NULL;
 BYTE puffer[64];
 
 memset(&ofs,0,sizeof(ofs));
 /*Erster Parameter ohne const ? Was erlauben Microsoft !*/
 lzhandle=LZOpenFileA(name,&ofs,OF_READ);
 if(lzhandle<0)
 {
  fprintf(stderr,"Kann %s nicht öffnen!\n",name);
  print_lzexpand_error(lzhandle);
  return NULL;
 }
 while(a==64)
 {
  a=LZRead(lzhandle,(char*)puffer,64);
  if(a>0)
  {
   if(expand_puffer((void**)&r,l,l+a))
   {
    memcpy(&(r[l]),puffer,a);
    l+=a;
   }
   else {a=0;break;}
  }
 }
 LZClose(lzhandle);
 if(len!=NULL) *len=l;
 return r;
}

#endif /*TARGET_WINGDI*/
