#ifndef _MENUE_H_
#define _MENUE_H_
#include <stdint.h>
extern void set_redraw_func( void (*CALLCONV func)( void*p),void *p);
extern int msgbox_janein(const char *text);
extern void msgbox_ok(const char *text);
extern void menue_init(void);
extern void weiter_knopf(int down);
extern int menue_clickable(int x);
extern int mouse_in_field(uint32_t m,unsigned int x1,unsigned int y1,
  unsigned int x2,unsigned int y2,uint32_t flags);
extern volatile int game_input_ready(void);
extern volatile uint32_t game_input_get(void);
#endif
