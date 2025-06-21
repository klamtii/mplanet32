#include "EDITION.H"
#ifdef TARGET_WINGDI

#include <stddef.h>
#include <string.h>
#include <dir.h>
#include <malloc.h>
#include <windows.h>

extern HWND window_handle;

/*Dialog zum Auswählen einer Datei (noch nicht fertig)*/
const char * get_filename(int save)
{
 static char puffer[512];
 int a;
 OPENFILENAME ofn;
 puffer[0]=0;
 if(save>1)
 {
  strcpy(puffer,"AUTOSAVE");
#ifdef GAME_Q_POP
  strcat(puffer,".QPP");
#endif
#ifdef GAME_MAGNETIC_PLANET
  strcat(puffer,".TDK");
#endif 
 }
 else 
 {
  memset(&ofn,0, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = window_handle;
#ifdef GAME_Q_POP
  ofn.lpstrFilter="Q-Pop Savegame (*.QPP)\0*.qpp\0"
#endif
#ifdef GAME_MAGNETIC_PLANET
  ofn.lpstrFilter="Magnetic Planet Savegame (*.TDK)\0*.tdk\0"
#endif 
  "Alle Dateien (*.*)\0*.*\0\0";
  ofn.lpstrFile = puffer;
  ofn.nMaxFile = 511;
#ifdef GAME_Q_POP
  ofn.lpstrDefExt = "QPP";
#endif
#ifdef GAME_MAGNETIC_PLANET
  ofn.lpstrDefExt = "TDK";
#endif 
  ofn.nFilterIndex=0;
  ofn.Flags=OFN_NOCHANGEDIR;
  if(save) /*==1*/
  {
   a=GetSaveFileName(&ofn);       
  }
  else
  {
   a=GetOpenFileName(&ofn);    
  }
  if(!a) puffer[0]=0;
 }
 return puffer;     
}

/* Liefert Dateinamen als 0-terminierte Strings zurück
 Ende 0 0; Ergebnis mit free() freigeben*/
char * find_filenames(const char *srch)
{
 int a,rlen=0;
 char *r=NULL;
 struct _finddata_t ff;
 long ff_handle;
 long ff_ret;
 if(srch==NULL) srch="*.*";
 ff_ret=ff_handle=_findfirst(srch,&ff);
 if(ff_handle!=-1)
 {
  while(ff_ret!=-1)
  {
   a=strlen(ff.name);
   if(a)
   {
    if(r!=NULL)
    {
     char *strtmp=realloc(r,rlen+a+4);
     if(strtmp==NULL) break;
     r=strtmp;
    }
    else
    {
     r=malloc(rlen+a+4);    
     if(r==NULL) break;
    }
    strcpy(r+rlen,ff.name);    
    rlen+=a+1;
    r[rlen]=0;
   }
   ff_ret=_findnext(ff_handle,&ff);
  }
  _findclose(ff_handle);
 }  
 return r;     
}

const char *iso_language_code(void)
{
 unsigned int a;
 static char puffer[16];
 GetLocaleInfoA(LOCALE_USER_DEFAULT,LOCALE_SISO639LANGNAME,puffer,16);
 for(a=0;a<16&&puffer[a];a++)
 {
  if(puffer[a]>='a'&&puffer[a]<='z')
   puffer[a]-='a'-'A';
 }
 return puffer;     
}

#endif /*TARGET_WINGDI*/
