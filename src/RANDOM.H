#ifndef _RANDOM_H_
#define _RANDOM_H_
#include <stdint.h>
void entropy_add(uint32_t a);
uint32_t entropy_get(void);
void qp_randomize(void);
int qp_random(int max);
int random_div(int a,int b);
#endif
