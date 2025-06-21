#include <stddef.h>
#include "RANDOM.H"

/*Warscheinlichkeiten aufaddieren, und Summe zurückliefern*/
unsigned int auswahl_vorbereiten(unsigned int *p,unsigned int n)
{
 unsigned int r=0;
 while(n--)
 {        
  *(p++)=r=r+*p;
 }
 return r;       
}

/*Einen Eintrag anhand der aufaddierten Warscheinlichkeiten auswählen*/
unsigned int auswahl_treffen(unsigned int *p,unsigned int n)
{
 unsigned int a,rnd;
 if(!p[n-1]) return 0;
 rnd=qp_random(p[n-1]);
 for(a=0;a<n;a++)
 {
  if(*(p++)>rnd) return a;
 }
 return 0;        
}
