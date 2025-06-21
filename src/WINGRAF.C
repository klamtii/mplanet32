#include "EDITION.H"
#ifdef TARGET_WINGDI

#include <stddef.h>
#include <stdlib.h>
#include <windows.h>
#include <stdint.h>
#include <malloc.h>
#include <mem.h>
#include <string.h>
#include <time.h>

#include "SPIELDAT.H"
#include "STRINGS.H"
#include "NSTRING.H"

#ifdef FEATURE_HQX
#include "hqx.h"
#endif

#define MAX_BITMAPS        32
#define DRAWWIN_X          640
#define DRAWWIN_Y          480
#define MAX_FONT           4

#define MAX_RINGPUFFER     64

HWND window_handle=NULL;
static HBITMAP hbitmap=NULL;
static HANDLE semaphore=NULL;
static int screen_locked=0;
static HANDLE event=NULL,thread=NULL;
static DWORD thread_id=0;
static int screen_ready=0;
static unsigned int faktor=1;
static unsigned int bilinear_flag=0;
#ifdef FEATURE_HQX
static unsigned int hqx_flag=0;
static unsigned int box_x1=0,box_y1=0,box_x2=640,box_y2=480;
#endif
static HCURSOR cursor_normal=NULL,cursor_hand=NULL;
static int cursor_flag=0; /*1 für Hand*/

#define LOCAL static pascal

inline void apply_cursor(int flg)
{
 if(cursor_flag&&!flg) SetCursor(cursor_normal);      
 if(!cursor_flag&&flg) SetCursor(cursor_hand);
 cursor_flag=flg;
}

/*Funktion um zu prüfen wo man klicken kann*/
volatile static int (*CALLCONV clickable_func)(unsigned int x,unsigned int y)=NULL;

inline int click_test(uint32_t lparam)
{
 if(clickable_func!=NULL)
  return clickable_func((lparam&0x3FFF)/faktor,((lparam>>16)&0x3FFF)/faktor);
 return 0;
}


/*Funktion, die Zeiger auf Funktion annimmt, und Zeiger auf Funktion
 zurückliefert*/
int (* CALLCONV set_clickable_func(int (*CALLCONV func)(unsigned int,unsigned int)))(unsigned int,unsigned int)
{
 int (*CALLCONV r)(unsigned int,unsigned int)=NULL;
 r=clickable_func;
 clickable_func=NULL;
 clickable_func=func;
 return r;
}

struct RINGPUFFER 
 {volatile DWORD p[MAX_RINGPUFFER];volatile WORD ein,aus;};

static volatile WORD maus_x=0,maus_y=0;
static struct RINGPUFFER rp_tast/*,rp_maus*/;

/*Rechteck, dass Veränderungen umfasst aktualisieren*/
LOCAL void update_box(int x1,int y1,int x2,int y2)
{
 if(x1<0) x1=0;
 if(y1<0) y1=0;
 if(x2>=DRAWWIN_X) x2=DRAWWIN_X-1;
 if(y2>=DRAWWIN_Y) y2=DRAWWIN_Y-1;
 if(box_x1>x1) box_x1=x1;     
 if(box_y1>y1) box_y1=y1;     
 if(box_x2<x2) box_x2=x2;     
 if(box_y2<y2) box_y2=y2;           
}

/*Rechteck mit Verändrungen zurücksetzen*/
LOCAL int reset_box(void)
{
 box_x1=DRAWWIN_X;     
 box_y1=DRAWWIN_Y;     
 box_y2=box_x2=0;
}

/*Gibt es was zum Zeichnen ?*/
LOCAL int box_has_content(void)
{
 return !!(box_x2>box_x1&&box_y2>box_y1);
}

/*Wert in Ringpuffer einfügen*/
LOCAL volatile int ringpuffer_ein(struct RINGPUFFER *rp,DWORD d)
{
 int r=-1;
 WORD a;
 a=(rp->ein+1)%MAX_RINGPUFFER;
 if(a!=rp->aus)
 {
  rp->p[rp->ein]=d;
  rp->ein=a;
  r=0;
 }
 return r;
}

/*Wert aus Ringpuffer lesen*/
LOCAL volatile int ringpuffer_aus(DWORD *d,struct RINGPUFFER *rp)
{
 int r=-1;
 if(rp->ein!=rp->aus)
 {
  *d=rp->p[rp->aus];
  rp->aus=(rp->aus+1)%MAX_RINGPUFFER;
  r=0;
 }
 return r;
}

/*Sind Werte im Ringpuffer ?*/
LOCAL volatile int ringpuffer_inhalt(const struct RINGPUFFER *rp)
{return (rp->ein!=rp->aus)?1:0;}

/* liefert mehrere kompatible HDCs (Handle Device Context) zurück*/
LOCAL int get_compatible_hdcs(HDC *hdcs,UINT n)
{
 UINT a;
 HDC hdcwin=NULL;
 if(n<1) return;
 for(a=0;a<n;a++) hdcs[a]=NULL;
 if(window_handle!=NULL)
  hdcwin=GetDC(window_handle); 
 if(hdcwin==NULL)
  hdcwin=GetDC(NULL);
 if(hdcwin==NULL) return 0;
 for(a=0;a<n;a++)
 {
  hdcs[a]=CreateCompatibleDC(hdcwin);
  if(hdcs[a]==NULL) break;
 }
 DeleteObject(hdcwin);
 return a;              
}

/*mehrere HDCs löschen*/
LOCAL int delete_hdcs(HDC *hdcs,UINT n)
{
 int r=0,a;
 for(a=0;a<n;a++)
 {
  if(hdcs[a]!=NULL)
  {
   DeleteObject(hdcs[a]);
   hdcs[a]=NULL;
   r++;                 
  }       
 }
 return r;     
}

/* Wenn Ausgabe gesperrt-> nichts tun, ansonsten Semaphor sperren*/
LOCAL int lock_semaphore(void)
{
 if(screen_locked) return 1;
 return (WaitForSingleObject(semaphore,INFINITE)==WAIT_OBJECT_0);
}

/*Semaphor freigeben*/
LOCAL void free_semaphore(void)
{
 if(screen_locked) return;
 ReleaseSemaphore(semaphore,1,NULL);
}

LOCAL int lock_screen(void) /*Semaphore bis flush sperren*/
{
 if(screen_locked) return 1;
 if(WaitForSingleObject(semaphore,INFINITE)==WAIT_OBJECT_0)
 {
  screen_locked=1;
  return 1;                                                          
 }
 return 0;
}

LOCAL void free_screen(void)
{
 if(screen_locked)
 {
  screen_locked=0;
  ReleaseSemaphore(semaphore,1,NULL);
 }
}

#define REPEAT_MAX 4

/*Callback-Funktion*/
static LRESULT CALLBACK 
window_callback (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
 static int maustaste=6;
 int a=0,b;
 switch(msg)
 {
  case WM_CREATE:
   if(semaphore!=NULL) 
   {
    CloseHandle(semaphore);
    printf("Warnung semaphore nicht gelöscht !\n");
   }
   semaphore=CreateSemaphoreA(NULL,1,1,NULL);
   if(semaphore!=NULL)
   {
    HDC hdc;
    RECT rc;
    screen_locked=0;
/*    if(faktor_fenster>1&&hqx_flag)
    {
     puffer_bitmap=calloc(DRAWWIN_X*faktor_bitmaps*DRAWWIN_Y*faktor_bitmaps,sizeof(uint32_t)*2);
     puffer_window=calloc(DRAWWIN_X*faktor_bitmaps*faktor_fenster*DRAWWIN_Y*faktor_bitmaps*faktor_fenster,sizeof(uint32_t));
     if(puffer_bitmap==NULL||puffer_window==NULL)
     {
      if(puffer_bitmap!=NULL) free(puffer_bitmap);
      if(puffer_window!=NULL) free(puffer_window);
      puffer_window=puffer_bitmap=NULL;
      hqx_flag=0;
     }
    }*/
    hdc=GetDC(hwnd);
    if(hdc!=NULL)    
    {
     if(hbitmap!=NULL) 
     {
      DeleteObject(hbitmap);
      printf("Warnung hbitmap nicht gelöscht !\n");
     }
     hbitmap=CreateCompatibleBitmap(hdc,DRAWWIN_X*faktor,DRAWWIN_Y*faktor);
     DeleteObject(hdc);
     if(hbitmap!=NULL)
     {      
      screen_ready=1;
      update_box(0,0,DRAWWIN_X,DRAWWIN_Y);
      break;
     }
    }
    CloseHandle(semaphore);
    semaphore=NULL;
   }
   DestroyWindow(hwnd);
   break;
  case WM_DESTROY:
   printf("WM_DESTROY\n");
   screen_ready=0;
   if(hbitmap!=NULL) DeleteObject(hbitmap);
   hbitmap=NULL;
   if(semaphore!=NULL) CloseHandle(semaphore);
   semaphore=NULL;
   PostQuitMessage(0);
   break;
  case WM_PAINT:
   if(WaitForSingleObject(semaphore,50)==WAIT_OBJECT_0)
   {
    PAINTSTRUCT ps;
    HDC hdc,hdcmem=NULL;        
    hdc=BeginPaint(hwnd,&ps);
    if(hdc!=NULL)
    {
     hdcmem = CreateCompatibleDC(hdc);
     if(hdcmem!=NULL)
     {
      HDC hdcold;
      hdcold=(HDC)SelectObject(hdcmem,hbitmap);
      BitBlt(hdc,0,0,DRAWWIN_X*faktor,DRAWWIN_Y*faktor,hdcmem,0,0,SRCCOPY);
      SelectObject(hdcmem,hdcold);
      DeleteObject(hdcmem);    
     }
     EndPaint(hwnd, &ps);
    }
    ReleaseSemaphore(semaphore,1,NULL);
    break;
   }
   else goto DEF_WIN_PROC;
  case WM_KEYDOWN:
   switch(wparam)
   {
    case VK_NUMPAD9: case VK_PRIOR: a++;
    case VK_NUMPAD8: case VK_UP:    a++;
    case VK_NUMPAD7: case VK_HOME:  a++;
    case VK_NUMPAD6: case VK_RIGHT: a++;
    case VK_NUMPAD5:                a++;
    case VK_NUMPAD4: case VK_LEFT:  a++;
    case VK_NUMPAD3: case VK_NEXT:  a++;
    case VK_NUMPAD2: case VK_DOWN:  a++;
    case VK_NUMPAD1: case VK_END:   a++;
    case VK_NUMPAD0: case VK_INSERT:
         a+='0';
         break;
    default:
      a=wparam;
      break;
   }
   if(a)
   {
    b=lparam&0xFFFF;
    if(b<1) b=1;
    if(b>REPEAT_MAX) b=REPEAT_MAX;
    if(WaitForSingleObject(semaphore,250)==WAIT_OBJECT_0)
    {
     while(b--)
      if(ringpuffer_ein(&rp_tast,a)) break;
     ReleaseSemaphore(semaphore,1,NULL);
    }
   }
   break;
  case WM_RBUTTONUP: a++;
  case WM_MBUTTONUP: a++;
  case WM_LBUTTONUP: a++;
  case WM_RBUTTONDOWN: a++;
  case WM_MBUTTONDOWN: a++;
  case WM_LBUTTONDOWN:
   {
    DWORD d=MOUSE_FLAG;
    /* Flag:0x80000000 Hoch:0x40000000 Tasten:0xC000 X:0x3FFF,Y:0x3FFF0000 */
    if(a>5) 
    {
     a=maustaste;
     if(a>2) break;
    }
    maustaste=a;
    if(a>2) {a-=3; d|=MOUSE_UP;}    
    d|=a<<MOUSE_KEYSHIFT; 
    a=(lparam&0x3FFF)/faktor;
    if(a>=DRAWWIN_X) break;
    d|=a;
    a=((lparam>>16)&0x3FFF)/faktor;
    if(a>DRAWWIN_Y) break;
    d|=a<<16;
    if(WaitForSingleObject(semaphore,100)==WAIT_OBJECT_0)
    {
     ringpuffer_ein(&rp_tast,d);
     ReleaseSemaphore(semaphore,1,NULL);
    }
   }
   break;
case WM_SETCURSOR:
   SetCursor(cursor_normal);
   cursor_flag=0;
   return DefWindowProc (hwnd, msg, wparam,lparam);
  case WM_MOUSEMOVE:
   apply_cursor(click_test(lparam));
   break;
  case WM_CLOSE: /* ALT+F4*/
#if 0
    if(WaitForSingleObject(semaphore,5000)==WAIT_OBJECT_0)
    {
     ringpuffer_ein(&rp_tast,3 /* 3=STRG+C*/);
     ReleaseSemaphore(semaphore,1,NULL);
    }
#endif
   exit(1);
   break;            
  default:
DEF_WIN_PROC: return DefWindowProc (hwnd, msg, wparam,lparam);
 }
 return 0;
}

/*Threadfunktion für Fenster (Nachrichtenpumpe)*/
static DWORD WINAPI pump_thread(LPVOID parameter)
{
 static MSG msg;     
 static const char name[8]="QPOP32";
 static ATOM atom=0;
 static WNDCLASS wndclass={CS_HREDRAW|CS_VREDRAW
  ,window_callback,0,0,NULL,NULL,NULL,
  (HBRUSH)COLOR_BACKGROUND+1,NULL,name};
  
 PeekMessageA(&msg,NULL,0,0,PM_NOREMOVE);/*make message queue*/
 if(atom==0)
 { 
  atom=RegisterClassA(&wndclass);
  /*Cursor Laden*/
  cursor_normal=LoadCursorA(NULL,IDC_ARROW);
  cursor_hand=LoadCursorA(NULL,IDC_HAND);
 }
 window_handle=NULL;
  
 if(GetSystemMetrics(SM_CXSCREEN)>=DRAWWIN_X*2&&
    GetSystemMetrics(SM_CYSCREEN)>=DRAWWIN_Y*2) 
 {
  int a,b;
  a=GetSystemMetrics(SM_CXSCREEN)/DRAWWIN_X;
  b=GetSystemMetrics(SM_CYSCREEN)/DRAWWIN_Y;
  if(a>b)a=b; if(a>4)a=4; if(a<1)a=1;
  switch((int)parameter)
  {
   default:faktor=1; bilinear_flag=hqx_flag=0;break;
   case 2:faktor=a; bilinear_flag=hqx_flag=0;break;
   case 3:faktor=a; bilinear_flag=1;hqx_flag=0;break;
   case 4:faktor=a; bilinear_flag=0;hqx_flag=1;break;
  }
 }
 if(hqx_flag) {hqxInit();printf("hqx init\n");}
 if(atom!=0)
 {
  window_handle= CreateWindowA(
   (char*)(atom&0xFFFF),get_string(STR_APPNAME),WS_BORDER|WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN,
   GetSystemMetrics(SM_CXSCREEN)/2-320*faktor,
   GetSystemMetrics(SM_CYSCREEN)/2-240*faktor,
   DRAWWIN_X*faktor,
   DRAWWIN_Y*faktor,
   NULL,NULL,NULL,NULL);     
 }
 SetEvent(event);
 if(window_handle==NULL) return 1;
 while (GetMessageA(&msg, NULL, 0, 0))
 {
  if(msg.hwnd==NULL)
  {
   if(msg.message==WM_DESTROY)
    DestroyWindow(window_handle);
   if(msg.message==WM_PAINT)
   {
    RECT rc;
    msg.hwnd=window_handle;
    GetClientRect(window_handle,&rc);
    InvalidateRect(window_handle,&rc,0);  
   }
  }
  DispatchMessageA(&msg);
 }   
/* exit(0);*/
 return 0;     
}

/*Fenster öffnen (>0=Vergrößerungsalgorithmus), 0=schliesen*/
int win_init(int mode)
{
 update_box(0,0,DRAWWIN_X,DRAWWIN_Y);
 if(mode)
 {
  if(screen_ready) win_init(0);
  event=CreateEventA(NULL,0,0,NULL);  
  if(event!=NULL)
  {
   thread=CreateThread(NULL,0,pump_thread,(void*)mode,0,&thread_id);
   WaitForSingleObject(event,INFINITE);         
   CloseHandle(event);
   event=NULL;
  }else return 1;
  if(!screen_ready) return 1;
 }
 else
 {
  if(!screen_ready) return 0;   
  PostThreadMessage(thread_id,WM_DESTROY,0,0);
  WaitForSingleObject(thread,INFINITE);
  CloseHandle((HANDLE)thread);
  thread=NULL;
  if(screen_ready) 
   printf("Warnung: screen_ready nach schließen des Fensters !\n");
 }
 return 0;  
}

/*Fenster aktualisieren, Sperre ggf. Entfernen*/
int win_flush(void)
{
 if(screen_ready==0||semaphore==NULL) return 1;
 free_screen();
 PostThreadMessage(thread_id,WM_PAINT,0,0);
 Sleep(0);
 return 0;   
}

/*Fenster Sperren*/
int win_lock(void)
{
 if(screen_ready==0||semaphore==NULL) return 1;
 lock_screen();
 return 0;
}

/*ist Eingabe bereit ? (kbhit())*/
volatile int win_input_ready(void)
{
 int r=0;
 if(hbitmap==NULL||semaphore==NULL) return 1;
 free_screen();
 if(WaitForSingleObject(semaphore,INFINITE)==WAIT_OBJECT_0)
 {
  r=ringpuffer_inhalt(&rp_tast);
  ReleaseSemaphore(semaphore,1,NULL);
 }
 else return 1;
 return r; 
}

/*Eingabe lesen (getch())*/
volatile uint32_t win_input_get(void)
{
 DWORD r=0;
 if(hbitmap==NULL||semaphore==NULL) return 0;
 free_screen();
 while(!win_input_ready())
  Sleep(40);
 if(WaitForSingleObject(semaphore,INFINITE)==WAIT_OBJECT_0)
 {
  if(ringpuffer_aus(&r,&rp_tast)) r=0;
  ReleaseSemaphore(semaphore,1,NULL);
 }
 return r;          
}

struct DDB_SLOT
{
 int x,y;
 HBITMAP bitmap;
 int xm,ym;
 HBITMAP maske;
};

/*Bitmap (DDB) mit Windowsfunktion vergrößern*/
LOCAL HBITMAP stretch_bitmap(HBITMAP bm,int x,int y,int faktor,int lin_interpol,int tilesize)
{
 HDC /*hdc1,hdc2*/ hdc[2];
 HBITMAP bmold1,bmold2;
 HBITMAP r=bm;
 if(faktor<2||bm==NULL||x<1||y<1) return bm;

 get_compatible_hdcs(hdc,2); 
 if(hdc[0]!=NULL&&hdc[1]!=NULL)
 {
  bmold1=SelectObject(hdc[0],bm);
  r=CreateCompatibleBitmap(hdc[0],x*faktor,y*faktor);
  if(r!=NULL)
  {
   bmold2=SelectObject(hdc[1],r);
   if(!lin_interpol) tilesize=0;
   lin_interpol=lin_interpol?HALFTONE:COLORONCOLOR;
   SetStretchBltMode(hdc[0],lin_interpol);
   SetStretchBltMode(hdc[1],lin_interpol);
   StretchBlt(hdc[1],0,0,x*faktor,y*faktor,hdc[0],0,0,x,y,SRCCOPY);   
   if(tilesize)
   {
    int a,b;
    for(a=0;a<y;a+=tilesize)
     for(b=0;b<x;b+=tilesize)
     {
      if(a+tilesize>y||b+tilesize>x) continue;
      StretchBlt(hdc[1],b*faktor,a*faktor,tilesize*faktor,tilesize*faktor,hdc[0],b,a,tilesize,tilesize,SRCCOPY);
     }
   }
   SelectObject(hdc[1],bmold2);   
   DeleteObject(bm);
  }else r=bm;
  SelectObject(hdc[0],bmold1);
 }
 DeleteObject(hdc[1]);
 DeleteObject(hdc[0]);
 return r; 
}

/*Bitmap (DDB) mit Windowsfunktion nebeneinander kopieren*/
LOCAL HBITMAP tile_bitmap(HBITMAP bm,int x,int y,int faktor)
{
 UINT a,b;
 HDC /*hdc1,hdc2*/ hdc[2];
 HBITMAP bmold1,bmold2;
 HBITMAP r=bm;
 if(faktor<2||bm==NULL||x<1||y<1) return bm;

 get_compatible_hdcs(hdc,2); 
 if(hdc[0]!=NULL&&hdc[1]!=NULL)
 {
  bmold1=SelectObject(hdc[0],bm);
  r=CreateCompatibleBitmap(hdc[0],x*faktor,y*faktor);
  if(r!=NULL)
  {
   bmold2=SelectObject(hdc[1],r);
   for(a=0;a<faktor;a++)
    for(b=0;b<faktor;b++)
     BitBlt(hdc[1],b*x,a*y,x,y,hdc[0],0,0,SRCCOPY);
   SelectObject(hdc[1],bmold2);   
   DeleteObject(bm);
  }else r=bm;
  SelectObject(hdc[0],bmold1);
 }
 DeleteObject(hdc[1]);
 DeleteObject(hdc[0]);
 return r; 
}

#ifdef FEATURE_HQX
/*Bitmap (und Maske) mit HQX vergrößern*/
LOCAL int hqx_sprite(struct DDB_SLOT *sprite,int faktor,int tilesize)
{
 int a,b,c;
 int r=0;
 BITMAPINFO bmi;
 HDC hdc=NULL;
 uint32_t *bm_src=NULL,*bm_dst=NULL,*msk_src=NULL,*msk_dst=NULL;
 uint32_t *bptr=NULL, *mptr=NULL;     
 if(faktor<2) return 1;
 if(faktor>4) faktor=4;

 /*Speicher anfordern*/
 if(sprite->bitmap==NULL||sprite->x<1||sprite->y<1) return 0;
 a=sprite->x*sprite->y*(faktor*faktor+1);
 bm_src=calloc(a+64,sizeof(uint32_t));
 if(bm_src==NULL) return 0;
 bm_dst=bm_src+sprite->x*sprite->y;
 r++; 
 if(sprite->maske!=NULL&&sprite->x>0&&sprite->y>0)
 {
  a=sprite->xm*sprite->ym*(faktor*faktor+1);
  msk_src=calloc(a+64,sizeof(uint32_t));
  if(msk_src==NULL) {free(bm_src);return 0;}
  msk_dst=msk_src+sprite->xm*sprite->ym;
  r++;
 }
 /*hdc anfordern*/
 hdc=GetDC(NULL);

 /*Daten lesen*/
 bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biPlanes = 1;
 bmi.bmiHeader.biCompression = BI_RGB;
 bmi.bmiHeader.biSizeImage = sprite->x*sprite->y*sizeof(uint32_t);
 bmi.bmiHeader.biWidth = sprite->x;
 bmi.bmiHeader.biHeight =-sprite->y; /* ! */
 bmi.bmiHeader.biClrUsed = 0;
 GetDIBits(hdc,sprite->bitmap,0,sprite->y,bm_src,&bmi,DIB_RGB_COLORS); 
 if(msk_src!=NULL)
 {
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = sprite->xm*sprite->ym*sizeof(uint32_t);
  bmi.bmiHeader.biWidth = sprite->xm;
  bmi.bmiHeader.biHeight =-sprite->ym; /*!*/
  bmi.bmiHeader.biClrUsed = 0;
  GetDIBits(hdc,sprite->maske,0,sprite->ym,msk_src,&bmi,DIB_RGB_COLORS);
 } 
 /*DDBs ändern*/ 
 DeleteObject(sprite->bitmap);
 sprite->bitmap=CreateCompatibleBitmap(hdc,sprite->x*faktor,sprite->y*faktor); 
 if(msk_src!=NULL)
 {
  DeleteObject(sprite->maske);
  sprite->maske=CreateCompatibleBitmap(hdc,sprite->xm*faktor,sprite->ym*faktor); 
 }
 /*hqx*/
 switch(faktor)
 {
  case 2: hq2x_32(bm_src,bm_dst,sprite->x,sprite->y); break;
  case 3: hq3x_32(bm_src,bm_dst,sprite->x,sprite->y); break;
  case 4: hq4x_32(bm_src,bm_dst,sprite->x,sprite->y); break;
 }
 if(tilesize)
 {
  for(a=0;a<sprite->y;a+=tilesize)
   for(b=0;b<sprite->x;b+=tilesize)
   {
    if(a+tilesize>sprite->y||b+tilesize>sprite->x) continue;
    switch(faktor)
    {
     case 2: hq2x_32_rb(bm_src+sprite->x*a+b,sprite->x*sizeof(uint32_t),
              bm_dst+(sprite->x*faktor*a+b)*faktor,sprite->x*faktor*sizeof(uint32_t),
              tilesize,tilesize); break;
     case 3: hq3x_32_rb(bm_src+sprite->x*a+b,sprite->x*sizeof(uint32_t),
              bm_dst+(sprite->x*faktor*a+b)*faktor,sprite->x*faktor*sizeof(uint32_t),
              tilesize,tilesize); break;
     case 4: hq4x_32_rb(bm_src+sprite->x*a+b,sprite->x*sizeof(uint32_t),
              bm_dst+(sprite->x*faktor*a+b)*faktor,sprite->x*faktor*sizeof(uint32_t),
              tilesize,tilesize); break;
    }    
   }          
 }
 
 if(msk_src!=NULL)
 {
  switch(faktor)
  {
   case 2: hq2x_32(msk_src,msk_dst,sprite->xm,sprite->ym); break;
   case 3: hq3x_32(msk_src,msk_dst,sprite->xm,sprite->ym); break;
   case 4: hq4x_32(msk_src,msk_dst,sprite->xm,sprite->ym); break;
  }
  /*Alphakanal zu Transparenzmaske*/
  a=sprite->ym*faktor; b=sprite->y*faktor;
  if(b<a) a=b;
  c=sprite->xm*faktor; b=sprite->x*faktor;
  if(b<c) c=b;  
  while(a--)
  {
   bptr=&(bm_dst[a*sprite->x*faktor]);
   mptr=&(msk_dst[a*sprite->xm*faktor]);
   for(b=0;b<c;b++)
   {
    if((int)((uint8_t*)mptr)[0]+(int)((uint8_t*)mptr)[1]+(int)((uint8_t*)mptr)[0]>40*3)
     {*mptr=0xFFFFFF; *bptr=0;}
    else
     *mptr=0;
    bptr++; mptr++;                    
   }
  }  
 } 
 /*Daten schreiben*/
 bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biPlanes = 1;
 bmi.bmiHeader.biCompression = BI_RGB;
 bmi.bmiHeader.biSizeImage = sprite->x*faktor*sprite->y*faktor*sizeof(uint32_t);
 bmi.bmiHeader.biWidth = sprite->x*faktor;
 bmi.bmiHeader.biHeight =-sprite->y*faktor; /* ! */
 bmi.bmiHeader.biClrUsed = 0;
 SetDIBits(hdc,sprite->bitmap,0,sprite->y*faktor,bm_dst,&bmi,DIB_RGB_COLORS);
 if(msk_src!=NULL)
 {
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = sprite->xm*faktor*sprite->ym*faktor*sizeof(uint32_t);
  bmi.bmiHeader.biWidth = sprite->xm*faktor;
  bmi.bmiHeader.biHeight =-sprite->ym*faktor; /* ! */
  bmi.bmiHeader.biClrUsed = 0;
  SetDIBits(hdc,sprite->maske,0,sprite->ym*faktor,msk_dst,&bmi,DIB_RGB_COLORS);
 } 
 /*HDC freigeben*/
 delete_hdcs(&hdc,1);  

 /*Speicher freigeben*/ 
 if(bm_src!=NULL) free(bm_src);
 if(msk_src!=NULL) free(msk_src);
 return r;
}
#endif

static int ddb_ok=0;
static struct DDB_SLOT ddb_slots[MAX_BITMAPS];

LOCAL void ddb_init(void)
{
 int a;
 if(ddb_ok) return;
 for(a=0;a<MAX_BITMAPS;a++) ddb_slots[a].x=-1;      
 ddb_ok=1;
}

/*Bitmap aus Slot freigeben*/
int free_bitmap(int n)
{
 int r=0;
 if(!ddb_ok) return 0;
 if(n<0)
 {
  for(n=0;n<MAX_BITMAPS;n++) r+=free_bitmap(n);
  return r;
 }
 if(n>=MAX_BITMAPS) return 0;
 if(ddb_slots[n].x==-1) return 0;
 ddb_slots[n].x=-1;
/* printf("Delete Bitmap %d\n",n);*/
 if(ddb_slots[n].bitmap!=NULL)
  DeleteObject(ddb_slots[n].bitmap);
 if(ddb_slots[n].maske!=NULL)
  DeleteObject(ddb_slots[n].maske);
 ddb_slots[n].maske=ddb_slots[n].bitmap=NULL;
 return 1;
}

/*Bitmapslot laden mit Bitmap (und Maske)*/
int alloc_bitmap(int n,void *hdr,void *pal,void *dat,
 void *hdrm,void *palm,void *datm,unsigned int tilesize,int noscale)
{
 int a,r=0;
 int maske=0;
 BITMAPINFO *bmi=NULL;
 HDC hdc;
 if(hdr==NULL||pal==NULL||dat==NULL||n<0||n>=MAX_BITMAPS) return 0;
 if(hdrm!=NULL&&palm!=NULL&&datm!=NULL) maske=1;

 if(!ddb_ok) ddb_init();
 else if(ddb_slots[n].x!=-1) free_bitmap(n);
 hdc=GetDC(NULL);
 if(hdc!=NULL)
 {
  a=sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256;
  bmi=malloc(a);
  if(bmi!=NULL)
  {
   memcpy(&(bmi->bmiHeader),hdr,sizeof(BITMAPINFOHEADER));
   memcpy(&(bmi->bmiColors),pal,sizeof(RGBQUAD)*256);
 /*  printf("Create Bitmap %d\n",n);*/
   ddb_slots[n].bitmap=
    CreateCompatibleBitmap(hdc,bmi->bmiHeader.biWidth,
    abs(bmi->bmiHeader.biHeight));
   if(ddb_slots[n].bitmap!=NULL)
   {
    SetDIBits(hdc,ddb_slots[n].bitmap,0,abs(bmi->bmiHeader.biHeight),
      dat,bmi,DIB_RGB_COLORS);
    ddb_slots[n].x=bmi->bmiHeader.biWidth;
    ddb_slots[n].y=abs(bmi->bmiHeader.biHeight);
    r++;
   }
   if(maske)
   {
    memcpy(&(bmi->bmiHeader),hdrm,sizeof(BITMAPINFOHEADER));
    memcpy(&(bmi->bmiColors),palm,sizeof(RGBQUAD)*256);
    ddb_slots[n].maske=
     CreateCompatibleBitmap(hdc,bmi->bmiHeader.biWidth,
     abs(bmi->bmiHeader.biHeight));
    if(ddb_slots[n].maske!=NULL)
    {
     SetDIBits(hdc,ddb_slots[n].maske,0,abs(bmi->bmiHeader.biHeight),
       datm,bmi,DIB_RGB_COLORS);
     ddb_slots[n].xm=bmi->bmiHeader.biWidth;
     ddb_slots[n].ym=abs(bmi->bmiHeader.biHeight);
     r++;
    }            
   }
   free(bmi);
  }
  DeleteObject(hdc);
 }
 if(maske&&ddb_slots[n].maske==NULL)
 {
  free_bitmap(n);
  return 0;
 }
 if(faktor>1)
 {
  if(noscale)
  {
   ddb_slots[n].bitmap=tile_bitmap(ddb_slots[n].bitmap,
   ddb_slots[n].x,ddb_slots[n].y,faktor);
  }
  else
  {
#ifdef FEATURE_HQX
   if(hqx_flag)
    hqx_sprite(ddb_slots+n,faktor,tilesize);
   else
#endif
   {
    if(ddb_slots[n].bitmap!=NULL)
     ddb_slots[n].bitmap=stretch_bitmap(ddb_slots[n].bitmap,
     ddb_slots[n].x,ddb_slots[n].y,faktor,bilinear_flag,tilesize);
    if(ddb_slots[n].maske!=NULL)
     ddb_slots[n].maske=stretch_bitmap(ddb_slots[n].maske,
      ddb_slots[n].x,ddb_slots[n].y,faktor,0,tilesize);
   }
  }
 }
 return r;     
}

/*Bitmaps Zeichnen
 Operation: 1 nur ODER mit Bitmap
           -1 nur UND mit Maske
            0 beides*/
int draw_bitmaps_x(int num,int slot,int *at_xdst,int *at_ydst,int *at_dx,int *at_dy,int *at_xsrc,int *at_ysrc,int operation)
{
 int xdst,ydst,dx,dy,xsrc,ysrc,r=0,maske=0;
 HDC hdc[3]={NULL,NULL,NULL};
 HBITMAP hbmold[3];
 if(hbitmap==NULL||semaphore==NULL||screen_ready==0) return 0;
 if(slot<0||slot>=MAX_BITMAPS) return 0;
 if(ddb_slots[slot].x<1) return 0;
 if(num<1) return 0;
 if(ddb_slots[slot].maske!=NULL) maske=1;
 
 if(operation>0) operation=SRCPAINT;
 else if(operation<0) operation=SRCAND;
 else operation=SRCCOPY;
 
 lock_semaphore();  /* -v- kritisch Anfang -v-*/
 xdst=2+maske;
 if(get_compatible_hdcs(hdc,xdst)==xdst)
 {
  hbmold[0]=(HBITMAP)SelectObject(hdc[0],hbitmap);
  hbmold[1]=(HBITMAP)SelectObject(hdc[1],ddb_slots[slot].bitmap);
  if(maske)
   hbmold[2]=(HBITMAP)SelectObject(hdc[2],ddb_slots[slot].maske);
  while(num--)
  {
   xdst=*(at_xdst++); ydst=*(at_ydst++);
   xsrc=*(at_xsrc++); ysrc=*(at_ysrc++);
   dx=*(at_dx++); dy=*(at_dy++);
   if(dx<0) {xdst+=dx;xsrc+=dx;dx=-dx;}   
   if(dy<0) {ydst+=dy;ysrc+=dy;dy=-dy;}
   if(xdst+dx>DRAWWIN_X) dx=DRAWWIN_X-xdst;
   if(ydst+dy>DRAWWIN_Y) dy=DRAWWIN_Y-ydst;
   if(xdst<0) {dx+=xdst;xsrc-=xdst;xdst=0;}
   if(ydst<0) {dy+=ydst;ysrc-=ydst;ydst=0;}
   if(dx<1||dy<1||xdst>=DRAWWIN_X||ydst>=DRAWWIN_Y) continue;
   if(xdst+dx<=0||ydst+dy<=0||xsrc+dx<=0||ysrc+dy<=0) continue;
   if(xsrc>=ddb_slots[slot].x||ysrc>=ddb_slots[slot].y) continue;    
   update_box(xdst,ydst,xdst+dx,ydst+dy);
   if(!maske)
    BitBlt(hdc[0],xdst*faktor,ydst*faktor,dx*faktor,dy*faktor,
     hdc[1],xsrc*faktor,ysrc*faktor,operation); 
   else
   {
    if(operation!=SRCPAINT)
     if(ddb_slots[slot].xm&&ddb_slots[slot].ym)
      BitBlt(hdc[0],xdst*faktor,ydst*faktor,dx*faktor,dy*faktor,hdc[2],
      (xsrc%ddb_slots[slot].xm)*faktor,(ysrc%ddb_slots[slot].ym)*faktor,SRCAND); 
    if(operation!=SRCAND)
     BitBlt(hdc[0],xdst*faktor,ydst*faktor,dx*faktor,dy*faktor,hdc[1],
     xsrc*faktor,ysrc*faktor,SRCPAINT); 
   } 
   r++;
  }
  ydst=2+maske;
  for(xdst=0;xdst<ydst;xdst++)
   SelectObject(hdc[xdst],hbmold[xdst]);
 }
 delete_hdcs(hdc,3);
 free_semaphore(); /* -^- kritisch Ende -^-*/    
}

/*Bitmaps Zeichnen (immer Operation 0)*/
int draw_bitmaps(int num,int slot,int *at_xdst,int *at_ydst,int *at_dx,int *at_dy,int *at_xsrc,int *at_ysrc)
{
 draw_bitmaps_x(num,slot,at_xdst,at_ydst,at_dx,at_dy,at_xsrc,at_ysrc,0);
}

struct FONTINFO {HFONT font;int x,y;};

static int font_ok=0;
static struct FONTINFO font_slots[MAX_FONT];

LOCAL void font_init(void)
{
 int a;
 if(font_ok) return;
 for(a=0;a<MAX_FONT;a++) font_slots[a].font=NULL;      
 font_ok=1;
}

/*Schriftart löschen*/
int free_font(int n)
{
 if((!font_ok)||n<0||n>=MAX_FONT) return 0;
 if(font_slots[n].font==NULL) return 0;
 DeleteObject(font_slots[n].font);
 font_slots[n].font=NULL;
 return 1;
}

/*Schriftart Erzeugen*/
int alloc_font(int n,const char *name,unsigned int x,unsigned int y,int bold)
{
 if(!font_ok) font_init();
 if(n<0||n>=MAX_FONT) return 0;
 if(font_slots[n].font)
  free_font(n);
 font_slots[n].font=CreateFontA(y*faktor,x*faktor,0,0,
   bold?800:FW_DONTCARE,0,0,0,OEM_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
   DEFAULT_QUALITY,FF_DONTCARE,name);
 font_slots[n].x=x;
 font_slots[n].y=y;
 return (font_slots[n].font==NULL)?0:1;
}

#ifdef FEATURE_UNICODE
#define UNICODE_BUFFER 512
static unsigned int codepage=/*1252*/ 1252;
static WCHAR utf16str[UNICODE_BUFFER];

unsigned int set_codepage(unsigned int cp)
{
 unsigned int r=codepage;
 codepage=cp;
 return r;
}
#endif /*FEATURE_UNICODE*/

/*Text ausgeben*/
int write_text(int n,int x,int y,int dx,int dy,const char *str,uint32_t f,int cent)
{
 int a;
 HDC hdc,hdcold;
 HFONT fontold;
 RECT rect,rect2;
 if((!font_ok)||n<0||n>=MAX_FONT||str==NULL) return 0;
 if(font_slots[n].font==NULL) return 0;
#ifdef FEATURE_UNICODE
 rect.left=strlen(str);
 if(rect.left>UNICODE_BUFFER-1) rect.left=UNICODE_BUFFER-1;
 /*Sollte eigentlich nur einmal aufgerufen werden*/
 rect.right=MultiByteToWideChar(codepage,0/*Flags*/,str,rect.left,utf16str,UNICODE_BUFFER-1);
 utf16str[rect.right]=0;
#endif /*FEATURE_UNICODE*/ 
 lock_semaphore(); /* -v- kritisch Anfang -v-*/
 get_compatible_hdcs(&hdc,1); 
 hdcold=(HDC)SelectObject(hdc,hbitmap);
 fontold=(HFONT)SelectObject(hdc,font_slots[n].font);
 SetBkMode(hdc,TRANSPARENT);
 SetTextColor(hdc,f);
 rect.left=x*faktor;
 rect.top=y*faktor;
 rect.right=(x+dx)*faktor;
 rect.bottom=(y+dy)*faktor;
 update_box(x,y,x+dx,y+dy);
 memcpy(&rect2,&rect,sizeof(RECT));
#ifdef FEATURE_UNICODE
 a=DrawTextW(hdc,utf16str,-1,&rect2,(cent?DT_CENTER:DT_LEFT)|DT_WORDBREAK|DT_CALCRECT);
#else
 a=DrawTextA(hdc,str,-1,&rect2,(cent?DT_CENTER:DT_LEFT)|DT_WORDBREAK|DT_CALCRECT);
#endif
 a-=(rect.bottom-rect.top);
 if(a<0)
  {a/=-2; rect.top+=a; rect.bottom+=a;}
#ifdef FEATURE_UNICODE
 DrawTextW(hdc,utf16str,-1,&rect,(cent?DT_CENTER:DT_LEFT)|DT_WORDBREAK);
#else
 DrawTextA(hdc,str,-1,&rect,(cent?DT_CENTER:DT_LEFT)|DT_WORDBREAK);
#endif
 SelectObject(hdc,hdcold);
 SelectObject(hdc,fontold);
 DeleteObject(hdc);
 free_semaphore(); /* -^- kritisch Ende -^-*/
 return 1;
}

#define XORSWAP(a,b) {(a)^=(b);(b)^=(a);(a)^=(b);}

/*Rechtecke Zeichnen*/
int fill_rectangles(unsigned int n,int *x1,int *y1,int *x2,int *y2,uint32_t *f)
{
 int r=0;
 HDC hdc,hdcold;
 HBRUSH brush=NULL,brushold;
 uint32_t f_alt=0xFF000000;
 HPEN penold;
 if(hbitmap==NULL||semaphore==NULL||n<1) return 0;
 lock_semaphore(); /* -v- kritisch Anfang -v-*/
 get_compatible_hdcs(&hdc,1); 
 hdcold=(HDC)SelectObject(hdc,hbitmap);
 penold=SelectObject(hdc,GetStockObject(NULL_PEN));
 brushold=SelectObject(hdc,NULL);
 while(n--)
 {
  if(*x1>*x2) XORSWAP(*x1,*x2);
  if(*y1>*y2) XORSWAP(*y1,*y2);
  if(*x2<0||*y2<0) goto INCR_RECT;
  if(*x1>=DRAWWIN_X||*y1>=DRAWWIN_Y) goto INCR_RECT;
  if(*x1<0) *x1=0; if(*y1<0) *y1=0;
  if(*x2>=DRAWWIN_X) *x2=DRAWWIN_X-1;if(*y2>=DRAWWIN_Y) *y2=DRAWWIN_Y-1;
  r++;
  if(brush!=NULL&&f_alt!=*f) {DeleteObject(brush);brush=NULL;}
  if(brush==NULL) brush=CreateSolidBrush(((*f&0xFF)<<16)|((*f&0xFF00))|((*f&0xFF0000)>>16));
  f_alt=*f;
  SelectObject(hdc,brush);
  update_box(*x1,*y1,*x2,*y2);
  Rectangle(hdc,(*x1)*faktor,(*y1)*faktor,(*x2+1)*faktor+1,(*y2+1)*faktor+1);
  SelectObject(hdc,brushold);
INCR_RECT:
  x1++;y1++;x2++;y2++;f++;
 }
 SelectObject(hdc,brushold);
 SelectObject(hdc,penold);
 SelectObject(hdc,hdcold); 
 if(brush!=NULL) DeleteObject(brush);
 DeleteObject(hdc);
 free_semaphore(); /* -^- kritisch Ende -^-*/    
 return r;
}

void minimize_window(void)
{ShowWindow(window_handle,SW_MINIMIZE);}

int window_minimized(void)
{
 if(window_handle==NULL) return 0;
 return IsIconic(window_handle);
}
#endif /*TARGET_WINGDI*/
