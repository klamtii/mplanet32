#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "SPIELDAT.H"
#include "EDITION.H"
#include "SYSTEMIO.H"
#include "MPLANET.H"
#include "MPBITMAP.H"

/*Text mit Schatten (Weiß auf Schwarz) zeichnen*/
void shadow_print(int x,int y,int dx,int dy,const char *text,int center)
{
 write_text(FONT_NORMAL,x+1,y+1,dx,dy,text,0,center);    
 write_text(FONT_NORMAL,x,y,dx,dy,text,0xFFFFFF,center);         
}

/*Rahmen zeichnen, fh=Farbe links oben, fd=Farbe rechts unten*/
void draw_frame(int x,int y,int dx,int dy,int w,uint32_t fh,uint32_t fd)
{
 int x1[4],y1[4],x2[4],y2[4],f[4];
 if(w<1) w=1;
 if(w>1)
 {
  draw_frame(x,y,dx,dy,w/2,fh,fd);       
  draw_frame(x+w/2,y+w/2,dx-(w/2)*2,dy-(w/2)*2,w-w/2,fh,fd);       
  return;        
 }
 x1[3]=x1[1]=x1[0]=x;
 y1[2]=y1[1]=y1[0]=y;
 x2[3]=x2[2]=x2[0]=x+dx-1;
 y2[0]=y+w-1;
 f[1]=f[0]=fh;
 x2[1]=x+w-1;
 y2[3]=y2[2]=y2[1]=y+dy-1;
 x1[2]=x+dx-w;
 f[3]=f[2]=fd;
 y1[3]=y+dy-w;
 fill_rectangles(4,x1,y1,x2,y2,f);
}

/*Rechteck mit Textur füllen, (Bitmaps aneinanderreihen)*/
void texture_rectangle(int xdst,int ydst,int dxdst,int dydst,
                  int bm,int xsrc,int ysrc,int dxsrc,int dysrc)
{
 int a,b,n=0;
 int x[8],y[8],dx[8],dy[8],sx[8],sy[8];
 for(a=0;a<dydst;a+=dysrc)
  for(b=0;b<dxdst;b+=dxsrc)
  {
   x[n]=xdst+b; y[n]=ydst+a;
   dx[n]=dxsrc; if(b+dxsrc>=dxdst) dx[n]=dxdst-b;
   dy[n]=dysrc; if(a+dysrc>=dydst) dy[n]=dydst-a;
   sx[n]=xsrc;
   sy[n]=ysrc;
   n++;
   if(n>7)
   {
    draw_bitmaps(n,bm,x,y,dx,dy,sx,sy);
    n=0;
   }
  }           
 if(n)  
  draw_bitmaps(n,bm,x,y,dx,dy,sx,sy);
}

/*Knopf mit Text zeichnen*/
void button_text(int x,int y,int dx,int dy,const char *text,int down)
{
 draw_frame(x-1,y-1,dx+1,dy+1,1,0,0);
 dx--; dy--;
 if(down)
  draw_frame(x,y,dx,dy,2,FARBE_DUNKEL,FARBE_HELL);
 else
  draw_frame(x,y,dx,dy,2,FARBE_HELL,FARBE_DUNKEL);
/* write_text(FONT_NORMAL,x+1,y+1,dx,dy,text,0,1);
 write_text(FONT_NORMAL,x,y,dx,dy,text,0xFFFFFF,1);*/
 shadow_print(x,y,dx,dy,text,1);
}

/*Knopf mit Text einmal gedrückt Zeichnen*/
void click_button_text(int x,int y,int dx,int dy,const char *text)
{
 button_text(x,y,dx,dy,text,1);
 win_flush();
 clk_wait((CLK_TCK/8)|1); 
 button_text(x,y,dx,dy,text,0); 
 win_flush();
}

/*Knopf mit DDB (bereits geladen) Zeichnen*/
void button_icon(int x,int y,int dx,int dy,int bm,int offx,int offy,int ysrc,int down)
{
 draw_frame(x-1,y-1,dx+1,dy+1,1,0,0);
 dx--; dy--;
 if(down)
  draw_frame(x,y,dx,dy,2,FARBE_DUNKEL,FARBE_HELL);
 else
  draw_frame(x,y,dx,dy,2,FARBE_HELL,FARBE_DUNKEL);
 x+=offx; y+=offy; dx-=offx; dy-=offy;
 down=0;
 draw_bitmaps_x(1,bm,&x,&y,&dx,&dy,&down,&ysrc,-1);      
}

/*Knopf mit DDB (bereits geladen) Drücken*/
void click_button_icon(int x,int y,int dx,int dy,int bm,int offx,int offy,int ysrc)
{
 button_icon(x,y,dx,dy,bm,offx,offy,ysrc,1);
 win_flush();
 clk_wait((CLK_TCK/8)|1); 
 button_icon(x,y,dx,dy,bm,offx,offy,ysrc,0);
 win_flush();
}

/*Knopf mit Plus/Minus (komplett als Bitmap) zeichnen*/
void button_plusminus(int x,int y,int dx,int dy,int typ,int down)
{
 static const unsigned short bms[4]={BM_PLUS,BM_PLUSX,BM_MINUS,BM_MINUSX};
 typ=(typ<0)?2:0;
 if(down) typ++;
 down=0;
 draw_bitmaps(1,get_bitmap_slot(bms[typ]),&x,&y,&dx,&dy,&down,&down);
}

/*Knopf mit Plus/Minus (komplett als Bitmap) drücken*/
void click_button_plusminus(int x,int y,int dx,int dy,int typ)
{
 button_plusminus(x,y,dx,dy,typ,1);
 win_flush();
 clk_wait((CLK_TCK/8)|1); 
 button_plusminus(x,y,dx,dy,typ,0);
 win_flush();
}

/* Checkbox zeichnen (eigentlich als Radiobutton verwendet)*/
void checkbox(int x,int y,int check)
{
 uint32_t f=0x808080;
 int a,b;
 draw_frame(x,y,17,18,2,FARBE_DUNKEL,FARBE_HELL);
 draw_frame(x+2,y+2,13,14,1,0,0);    
 x+=3;y+=3;a=x+10;b=y+11;
 fill_rectangles(1,&x,&y,&a,&b,&f);
 f=0;a=11;b=12;
 if(check)
  draw_bitmaps(1,get_bitmap_slot(BM_KREUZ),&x,&y,&a,&b,&f,&f);
}

/*Scrollbar aus ENTWICKL.DAT zeichnen (Evolution,Einstellungen)*/
void scrollbar(int x,int y,int delta,int zustand,unsigned int farbe)
{
 uint32_t f=0x808080;
 int xdst[2],ydst[2],dx[2],dy[2],xsrc[2],ysrc[2];
 xdst[0]=x+delta; ydst[0]=y+15;
 fill_rectangles(1,&x,&y,xdst,ydst,&f);
 draw_frame(x,y,delta,16,1,0,0);
 xdst[0]=x; ydst[1]=ydst[0]=y; dy[1]=dy[0]=16;
 dx[0]=zustand/2;
 ysrc[1]=ysrc[0]=(farbe>>1)*16;
 xdst[1]=x+dx[0];
 dx[1]=zustand-dx[0]; 
 xsrc[0]=xsrc[1]=0;
 draw_bitmaps_x(2,get_bitmap_slot(BM_ENTWICKL_DAT),xdst,ydst,dx,dy,xsrc,ysrc,-1);
 xsrc[0]=(farbe&1)?300:0;
 xsrc[1]=300-dx[1]+xsrc[0];
 draw_bitmaps_x(2,get_bitmap_slot(BM_ENTWICKL_DAT),xdst,ydst,dx,dy,xsrc,ysrc,1);
}
