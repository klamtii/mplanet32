#ifndef _GUI_H_
#define _GUI_H_
void shadow_print(int x,int y,int dx,int dy,const char *text,int center);
void draw_frame(int x,int y,int dx,int dy,int w,uint32_t fh,uint32_t fd);
void texture_rectangle(int xdst,int ydst,int dxdst,int dydst,
                  int bm,int xsrc,int ysrc,int dxsrc,int dysrc);
void button_text(int x,int y,int dx,int dy,const char *text,int down);
void click_button_text(int x,int y,int dx,int dy,const char *text);
void button_icon(int x,int y,int dx,int dy,int bm,int offx,int offy,int ysrc,int down);
void click_button_icon(int x,int y,int dx,int dy,int bm,int offx,int offy,int ysrc);
void button_plusminus(int x,int y,int dx,int dy,int typ,int down);
void click_button_plusminus(int x,int y,int dx,int dy,int typ);
void checkbox(int x,int y,int check);
void scrollbar(int x,int y,int delta,int zustand,unsigned int farbe);
#endif /*!_GUI_H_*/
