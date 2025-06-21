#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include "EDITION.H"
#include "NSTRING.H"
#include "SYSTEMIO.H"

#if 0
/*String aus verketteten Strings auswählen*/
static const char *select_string(const char *str,unsigned short n)
{
 if(str==NULL) return "NULL";
 while(n--&&*str)
  str+=strlen(str)+1;       
 return str;       
}
#endif

int add_to_string(char **str,const char *fstr,...)
{
 int a=1024;
 char *tmpstr;
 va_list va;
 a+=strlen(fstr)*2;
 tmpstr=malloc(a);
 if(tmpstr==NULL) return 0; 
 va_start(va,fstr);
 vsprintf(tmpstr,fstr,va);
 va_end(va);
 if(*str==NULL)
  *str=strdup(tmpstr);
 else
 {
  a=strlen(*str)+4+strlen(tmpstr);
  *str=realloc(*str,a);
  strcat(*str,tmpstr);     
 }
 free(tmpstr);
 return a;    
}


struct GAME_STRING
{
 unsigned int num;
 const char name[8];
 const char *string0;
 char *string;
};

int encoding=-1;

static struct GAME_STRING strings[STR_MAX]=
{
 {STR_LANNAME,"LANNAME","Deutsch",NULL},
 {STR_LANCODE,"LANCODE","DE",NULL},
 {STR_GAMNAME,"GAMNAME","Magnetic Planet",NULL},
 {STR_APPNAME,"APPNAME","Magnetic Planet 32",NULL},
 {STR_SPEC1,"SPEC1","Rot",NULL},
 {STR_SPEC2,"SPEC2","Grün",NULL},
 {STR_SPEC3,"SPEC3","Gelb",NULL},
 {STR_SPEC4,"SPEC4","Blau",NULL},
 {STR_SPEC5,"SPEC5","Weiß",NULL},
 {STR_SPEC6,"SPEC6","Schwarz",NULL},
 {STR_PLANT1,"PLANT1","Anpassung Birnen",NULL},
 {STR_PLANT2,"PLANT2","Anpassung Schrauben",NULL},
 {STR_PLANT3,"PLANT3","Anpassung Isolierkraut",NULL},
 {STR_PLANT4,"PLANT4","Anpassung Kristalle",NULL},
 {STR_PLANT5,"PLANT5","Anpassung Widerstände",NULL},
 {STR_PLANT6,"PLANT6","Anpassung Stecker",NULL},
 {STR_FERTIL,"FERTIL","Duplizierung",NULL},
 {STR_ATTACK,"ATTACK","Angriffsstärke",NULL},
 {STR_DEFEND,"DEFEND","Verteidigungsstärke",NULL},
 {STR_CAMOU,"CAMOU","Tarnung",NULL},
 {STR_SPEED,"SPEED","Geschwindigkeit",NULL},
 {STR_SCAN,"SCAN","Scannerreichweite",NULL},
 {STR_INT,"INT","Neuronale Matrix",NULL},
 {STR_EVOL,"EVOL","Entwicklungspunkte",NULL},
 {STR_CONT,"CONT","Weiter",NULL},
 {STR_BACK,"BACK","Zurück",NULL},
 {STR_LOAD,"LOAD","Lade Spielstand",NULL},
 {STR_SAVE,"SAVE","Speichere Spielstand",NULL},
 {STR_YES,"YES","Ja",NULL},
 {STR_NO,"NO","Nein",NULL},
 {STR_ON,"ON","An",NULL},
 {STR_OFF,"OFF","Aus",NULL},
 {STR_WAVE,"WAVE","Geräusche",NULL},
 {STR_SETWAVE,"SETWAVE","Geräusche einstellen",NULL},
 {STR_MIDI,"MIDI","Musik",NULL},
 {STR_SETMIDI,"SETMIDI","Musik einstellen",NULL},
 {STR_SETLANG,"SETLANG","Sprache einstellen",NULL},
 {STR_SETV,"SETV","Geschwindigkeit einstellen",NULL},
 {STR_MSGBOX,"MSGBOX","Sicherheitsabteilung",NULL},
 {STR_INFOBOX,"INFOBOX","Trikorderanalyse ergab",NULL},
 {STR_DEAD,"DEAD","Ihre Modellreihe konnte sich nicht durchsetzen und wird daher eingestampft",NULL},
 {STR_AIDEAD,"AIDEAD","Eine der computergesteuerten Modellreihen konnte sich nicht durchsetzen, und wird daher eingestampft.",NULL},
 {STR_QUIT,"QUIT","Wollen Sie das Spiel wirklich beenden ?",NULL},
 {STR_CONF,"CONF","Ist ihr Zug wirklich beendet ?",NULL},
 {STR_BILD,"BILD","Bild einstellen",NULL},
 {STR_SCALE,"SCALE","Vergrößerungsalgorithmus",NULL},
 {STR_SCNO,"SCNO","Keine Vergrößerung",NULL},
 {STR_SCNN,"SCNN","Nearest Neighbour",NULL},
 {STR_SCLI,"SCLI","Bilineare Interpolation",NULL},
 {STR_SCHQ,"SCHQ","HQX",NULL},
 {STR_HEAT,"HEAT","Hitzewelle",NULL},
 {STR_COLD,"COLD","Kälteeinbruch",NULL},
 {STR_METEOR,"METEOR","Meteoriteneinschlag",NULL},
 {STR_VIRUS,"VIRUS","Virenangriff",NULL},
 {STR_VULKAN,"VULKAN","Vulkanausbruch",NULL},
 {STR_FLOOD,"FLOOD","Flutwelle",NULL},
 {STR_QUAKE,"QUAKE","Erdbeben",NULL},
 {STR_ROUND,"ROUND","Runde",NULL},
 {STR_ROUNDS,"ROUNDS","Spieldauer",NULL},
 {STR_ROUNDS1,"ROUNDS1","KURZ (5 Runden)",NULL},
 {STR_ROUNDS2,"ROUNDS2","MITTEL (10 Runden)",NULL},
 {STR_ROUNDS3,"ROUNDS3","LANG (20 Runden)",NULL},
 {STR_ROUNDS4,"ROUNDS4","BIS ZUM BITTEREN ENDE",NULL},
 {STR_VICTORY,"VICTORY","Hurra Gewonnen",NULL},
 {STR_DEFEAT,"DEFEAT","Leider verloren",NULL},
 {STR_ALONE,"ALONE","Möchten Sie allein weiterspielen ?",NULL},
 {STR_DIFFIC,"DIFFIC","Schwierigkeitsgrad:",NULL},
 {STR_DIFF1,"DIFF1","Hammerhart",NULL},
 {STR_DIFF2,"DIFF2","Gar nicht ohne",NULL},
 {STR_DIFF3,"DIFF3","So la la",NULL},
 {STR_DIFF4,"DIFF4","Leicht",NULL},
 {STR_POFF,"POFF","^  Aus",NULL},
 {STR_PHUMAN,"PHUMAN","^ Mensch",NULL},
 {STR_PCOMPU,"PCOMPU","^Computer",NULL},
 {STR_SINGLEP,"SINGLEP","Einzelspieler",NULL},
 {STR_MULTIP,"MULTIP","Mehrspieler",NULL},
 {STR_SUICIDE,"SUICIDE","Möchten Sie wirklich die Selbstzerstörung aktivieren ?",NULL},
 {STR_SCROLL,"SCROLL","Scrollen",NULL},
 {STR_AUTOCON,"AUTOCON","Automatisch Weiter",NULL},
 {STR_FFIGHT,"FFIGHT","Schneller Kampf",NULL}, 
 {STR_ABOUT,"ABOUT","Über...",NULL}, 
};

/*Zeiger auf String zurückliefern*/
const char *get_string(unsigned short n)
{
 unsigned int a;
 for(a=0;a<STR_MAX;a++)
  if(strings[a].num==n)
  {
   if(strings[a].string!=NULL)
    return strings[a].string;
   if(strings[a].string0!=NULL)
    return strings[a].string0;   
  } 
 return "STRING NOT FOUND";
}

/*Geladenen String löschen*/
static void reset_string(unsigned int n)
{
 if(strings[n].string!=NULL)
 {
  free(strings[n].string);
  strings[n].string=NULL;                             
 }              
}

/*String ändern (für Übersetzungen)*/
static void set_string(unsigned int n,const char *str)
{
 reset_string(n);
 if(str==NULL) return;      
 if(*str==0) return;
 strings[n].string=strdup(str);      
}

/*Alle geladenen Strings löschen*/
void reset_strings(void)
{
 unsigned int a;
 for(a=0;a<STR_MAX;a++)
  reset_string(a);
}

/*Zeile bis LF (10) aus Datei lesen*/
static unsigned int read_line(char *puffer,int lmax,FILE *file)
{
 unsigned int r=0,a=0;
 puffer[0]=0;
 while(a!=10)
 {
  a=getc(file);            
  if(a>255||feof(file))
  {
   if(r<1) return -1;
   break;
  }             
  if(a==13||a==0) continue;
  if(a==10) break;
  if(r+1<lmax)
  {
   puffer[r++]=a;
   puffer[r]=0;
  }
 }
 return r;      
}

/*Überflüssige Leerzeichen entfernen*/
static void purge_line(char *str0)
{
 int flag=0;
 char *str,*str2;
 str2=str=str0;;
 while(*str2==32||*str2==9) str2++;
 while(*str2)
 {
  if(*str2==32||*str2==9||(((*str2)&0x80)&&str==str0) )
  {
   if(flag)
    *(str++)=32;
   flag=0;           
  }
  else
  {
   *(str++)=*str2;
   if(*str2!='=')
    flag=1;    
  }
  str2++;
 }      
 while(str>str0&&str[-1]==32) str--;            
 *str=0;
}

/*String auf Variablennamen, gefolgt von '=' prüfen*/
static int keyword_match(char *str,const char *k)
{
 int r=0;
 while(*str&&*k)
 {
  if(*str!=*k&&((*str)+'A'-'a')!=*k) return 0;      
  str++; k++; r++;
 }
 while(*str==32&&*str==9) {str++;r++;}
 if(*str!='=') return 0;
 str++; r++;
 while(*str==32&&*str==9) {str++;r++;} 
 return r;
}

#ifdef GAME_Q_POP
static const char signatur[]="QPOPLANGUAGE";
#endif
#ifdef GAME_MAGNETIC_PLANET
static const char signatur[]="MAGNETICPLANETLANGUAGE";
#endif

/*Sprachdatei lesen*/
int parse_language_file(const char *name,int really)
{
 int a,b,r=0;
 FILE *file=NULL;
 char *puffer=NULL;
 int lanname_ok=0,lancode_ok=0;
 file=fopen(name,"rb");
 if(file==NULL) return 0;
 puffer=malloc(1024);
 if(puffer==NULL) {fclose(file);return 0;}
 do
 {
  a=1;
  *puffer=0;
  read_line(puffer,1024,file);
  printf("%s\n",puffer);
  purge_line(puffer);
  if(*puffer!=0)
   a=memcmp(puffer,signatur,strlen(signatur));  
  if(!a) break;
 }while(*puffer==0&&!feof(file));   
 if(!a)
 { 
  printf("Sprachdatei: %s\n",name);
  r=-1; 
  while(!feof(file))
  {
   read_line(puffer,1024,file);
   purge_line(puffer);
   if(puffer[0]==0) continue;
   if(puffer[0]==';') continue;
   b=keyword_match(puffer,"ENCODING");
   if(b)
   {
    if(!strcmp(puffer+b,"ASCII")) r=437;
    if(!strcmp(puffer+b,"UTF7")) r=65000;
    if(!strcmp(puffer+b,"UTF8")) r=65001;
    if(!memcmp(puffer+b,"CP",2))
     sscanf(puffer+b+2,"%u",&r);
   }
   else 
    for(a=0;a<STR_MAX;a++)
    {
     b=keyword_match(puffer,strings[a].name);
     if(b)
     {
      if(strings[a].num==STR_LANNAME) lanname_ok=1;
      if(strings[a].num==STR_LANCODE) lancode_ok=1;      
      if(really||strings[a].num==STR_LANNAME||strings[a].num==STR_LANCODE)
       set_string(a,puffer+b);              
     }
    }
   if(!really&&lanname_ok&&lancode_ok)
    goto ENDE;
  }
  if(!lancode_ok) {printf(" LANCODE fehlt !\n");return 0;}
  if(!lanname_ok) {printf(" LANNAME fehlt !\n");return 0;}
 }
 ENDE:
 free(puffer);
 fclose(file);   
 return r;
}

struct GAME_LANGUAGE
{
 int codepage;
 char *_str; /*Eigentlicher Speicherblock*/
 char *code;
 char *name; 
 char *filename;
};

static struct GAME_LANGUAGE *language_table=NULL;
static unsigned int language_num=0;

void free_language_table(void)
{
 unsigned int a;      
 if(language_table==NULL) return;
 for(a=0;a<language_num;a++)
  free(language_table[a]._str);
 free(language_table);
 language_table=NULL;
 language_num=0;      
}

int make_language_table(void)
{
 int a;
 char *dir=NULL;
 char *nameptr=NULL;
 const char *code,*name;
 struct GAME_LANGUAGE *tptr;
 free_language_table();
 dir=find_filenames("*.INI");
 if(dir==NULL) return 0;
 nameptr=dir;
 while(*nameptr)
 {
  a=parse_language_file(nameptr,0);
  if(a)
  {
   if(language_table==NULL)
   {
    language_table=malloc(sizeof(struct GAME_LANGUAGE));
    if(language_table==NULL) break;                       
   }
   else
   {
    tptr=realloc(language_table,sizeof(struct GAME_LANGUAGE)*(language_num+1));
    if(tptr==NULL) break;                           
    language_table=tptr;
   }
   tptr=language_table+language_num;
   tptr->codepage=a;
   code=get_string(STR_LANCODE);
   name=get_string(STR_LANNAME);
   tptr->_str=malloc( strlen(code)+strlen(name)+strlen(nameptr)+4);
   if(tptr->_str==NULL) break;
   tptr->code=tptr->_str;
   strcpy(tptr->code,code);
   tptr->name=tptr->code+strlen(code)+1;
   strcpy(tptr->name,name);
   tptr->filename=tptr->name+strlen(name)+1;
   strcpy(tptr->filename,nameptr);         
   language_num++;   
  }
  nameptr+=strlen(nameptr)+1;
 }
 free(dir);
 return language_num;
}

/*Informationen zu Sprache finden*/
unsigned int language_info(char **code,char **name,char **filename,unsigned int i)
{
 int r=0;
 char *c=NULL,*n=NULL,*f=NULL;
 if(i<language_num&&language_table!=NULL)
 {
  c=language_table[i].code;
  n=language_table[i].name;
  f=language_table[i].filename;
  r=language_table[i].codepage;
 }  
 if(code!=NULL) *code=c;
 if(name!=NULL) *name=n;
 if(filename!=NULL) *filename=f;
 return r;
}

/*ISO-Sprachcode zu Sprachdatei umwandeln*/
unsigned int language_codetofile(const char *code)
{
 int a;
 if(language_table==NULL||language_num<1) return -1;
 for(a=0;a<language_num;a++)
 {
  if(!stricmp(language_table[a].code,code))
   return a;
 }      
 return -1;
}
