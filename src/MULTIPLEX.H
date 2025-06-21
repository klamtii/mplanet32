#ifndef _MULTIPLEX_H_
#define _MULTIPLEX_H_
#include <stdint.h>
int multiplex_wav(uint32_t *lens,int n_max,const void *_src,int src_len);
int multiplex_midi(uint32_t *lens,int n_max,const void *_src,int src_len);
void multiplex_pointers(void **ptrs,void *src,const uint32_t *lens,int n);
#endif /*!_MULTIPLEX_H_*/
