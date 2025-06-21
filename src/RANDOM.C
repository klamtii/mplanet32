#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define MAX_ENTROPY 16

static uint32_t entropy[MAX_ENTROPY];
static unsigned int entropy_index=0;

/*Entropie für Zufallszahlen hinzufügen*/
void entropy_add(uint32_t a)
{
 a+=entropy[(entropy_index++)%MAX_ENTROPY]*251;
 a^=a>>7;
 entropy[(entropy_index)%MAX_ENTROPY]+=a;
}

/*Entropy für Zufallszahlen lesen*/
uint32_t entropy_get(void)
{
 uint32_t a=0xDEADBEEF;
 a+=entropy[(entropy_index++)%MAX_ENTROPY]*19;
 a^=a>>11; a*=12345;
 a^=a>>13; a*=54321;
 entropy[(entropy_index)%MAX_ENTROPY]^=a;
 return a;
}

/*Zufallsgenerator mit Zeit initialisieren*/
void qp_randomize(void)
{
 volatile int a;
 entropy_add(time(NULL));
 entropy_add(clock());
 entropy_add(a);
 for(a=0;a<MAX_ENTROPY;a++)
  entropy_add(a+0xDEADBEEF);
}

/*Zufallszahl bis Max lesen
 - kann negativ sein (Zufallszahlen sind dann auch negativ)
 - kann 0 sein (ohne Division by Zero)*/
int qp_random(int max)
{
/* int32_t a;
 a=entropy_get()&0xFFFF;
 a*=max;
 return a/65536;*/
 return (((int32_t)(entropy_get()&0xFFFF))*max)/65536;
/* if(max<1) return 0;
 return entropy_get()%max;*/
}

/*Teile a/b Zufällig Runden je nach Rest*/
int random_div(int a,int b)
{
 int r;
 int sign=0;
 if(!b) return a;
 if(a<0) {sign++;a=-a;}
 if(b<0) {sign++;b=-b;}
 r=a/b; a=a%b;
 if(qp_random(b)<a) r++;
 return(sign&1)?-r:r;
}
