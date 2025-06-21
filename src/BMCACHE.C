#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include "EDITION.H"
#include "SYSTEMIO.H"

struct REG_BITMAP
{
 struct DIB_HEADER *hdr;
 void *pal;
 void *dat;
 void *hdrm;
 void *palm;      
 void *datm;
 int slot;
 int registered;
 unsigned int tilesize;
 int noscale;
 const char *name;
 uint32_t sizeimage;
};

static struct REG_BITMAP *registered_bitmaps=NULL;
static unsigned int num_registered_bitmaps=0;
static int slot_to_bitmap[MAX_BITMAPS];
static unsigned int last_reference[MAX_BITMAPS];
static int reference_time=0;

static void set_bitmap_num(unsigned int n)
{
 unsigned int a;
 if(!num_registered_bitmaps)
 {
  for(a=0;a<MAX_BITMAPS;a++)
   slot_to_bitmap[a]=-1;
  if(!n) return;
  registered_bitmaps=malloc(sizeof(struct REG_BITMAP)*n);
 }
 else
 {
  if(!n)
  {
   free(registered_bitmaps);
   registered_bitmaps=NULL;
   num_registered_bitmaps=0;      
   return;
  }
  registered_bitmaps=realloc(registered_bitmaps,sizeof(struct REG_BITMAP)*n);
 }
 assert(registered_bitmaps!=NULL);/*Out of Memory*/
 for(a=num_registered_bitmaps;a<n;a++)
 {
#define RBA registered_bitmaps[a]
  RBA.registered=0;
  RBA.slot=-1;
  RBA.dat=RBA.pal=RBA.hdr=NULL;
  RBA.datm=RBA.palm=RBA.hdrm=NULL;
  RBA.name=NULL;
  RBA.sizeimage=0;
#undef RBA
 }
 num_registered_bitmaps=n;
}


/* registrierte Bitmaps können bei Bedarf in eine DDB geladen werden*/
/* um Registrierung Rückgängig zu machen mit hdr=NULL aufrufen*/
int register_bitmap(unsigned int n,
 void *hdr,void *pal,void *dat,
 void *hdrm,void *palm,void *datm)
{
 unsigned int a;
 struct REG_BITMAP *rb;
 if(n>=num_registered_bitmaps)
  set_bitmap_num(n+1);     
 rb=&(registered_bitmaps[n]);
 if(rb->slot>-1)
 {
  slot_to_bitmap[rb->slot]=-1;
  free_bitmap(rb->slot);
 }
 rb->slot=-1; rb->registered=1;
 rb->hdr=hdr; rb->pal=pal; rb->dat=dat;     
 rb->hdrm=hdrm; rb->palm=palm; rb->datm=datm;
 rb->tilesize=0;
 rb->noscale=0;
 rb->name=NULL;
 if(hdr==NULL||pal==NULL||dat==NULL)
  rb->registered=0;
 n=0;
 for(a=0;a<num_registered_bitmaps;a++)
  if(registered_bitmaps[a].registered) n=a+1;
 if(n<num_registered_bitmaps)
  set_bitmap_num(n);
 
 return num_registered_bitmaps;
}


/*Größe der Kästchen einer Tilemap festlegen
 - damit vergrößerungsalgorithmen am Rand aufhören,
 und Tiles sich nicht gegenseitig beeinflussen
*/
int set_bitmap_tilesize(unsigned int n,unsigned int a)
{
 int r=-1; 
 if(n>=num_registered_bitmaps) return r;
 r=registered_bitmaps[n].tilesize;
 registered_bitmaps[n].tilesize=a;
 return r;    
}

int set_bitmap_scaling(unsigned int n,int on)
{
 int r=-1; 
 if(n>=num_registered_bitmaps) return r;
 r=!registered_bitmaps[n].noscale;
 registered_bitmaps[n].noscale=!on;
 return r;        
}

const char *set_bitmap_name(unsigned int n,const char *name)
{
 const char  *r=NULL; 
 if(n>=num_registered_bitmaps) return r;
 r=registered_bitmaps[n].name;
 registered_bitmaps[n].name=name;
 return r;              
}

/* DIB zu DDB umwandeln, und für draw_bitmaps() zur Verfügung stellen
 n=Registrierungsnummer, Rückgabe=Slot der DDB*/
int get_bitmap_slot(unsigned int n)
{
 int a,b;
 struct REG_BITMAP *rb;
 uint32_t sizeimage=0;
 if(n>=num_registered_bitmaps) return -1;
 rb=&(registered_bitmaps[n]);
 if(!rb->registered) return -1;
 reference_time++;
 a=rb->slot;
 if(a>-1) 
 {
  last_reference[a]=reference_time;
  return a;
 }
 a=-1;
 for(b=0;b<MAX_BITMAPS;b++)
 {
  if(slot_to_bitmap[b]==-1) {a=b;break;}
  if(a==-1)a=b;
  else if(last_reference[b]<last_reference[a]) a=b;
 }
 b=slot_to_bitmap[a];
 if(slot_to_bitmap[a]>-1)
 {
  registered_bitmaps[slot_to_bitmap[a]].slot=-1;
  free_bitmap(a);
 }
 printf("Bitmap slot %d: %d->%d\n",a,b,n);

 if(rb->sizeimage)
 {
  sizeimage=rb->hdr->sizeimage;
  rb->hdr->sizeimage=rb->sizeimage;
 }

 alloc_bitmap(a,rb->hdr,rb->pal,rb->dat,rb->hdrm,rb->palm,rb->datm,rb->tilesize,rb->noscale);

 if(rb->sizeimage)
  rb->hdr->sizeimage=sizeimage;

 rb->slot=a;
 slot_to_bitmap[a]=n;
 last_reference[a]=reference_time;
 return a;
}

int bitmap_name_to_number(const char *name)
{
 int a;
 for(a=0;a<num_registered_bitmaps;a++)
 {
  if(registered_bitmaps[a].name!=NULL)
  {
   if(!stricmp(name,registered_bitmaps[a].name))
    return a;                        
  } 
 }  
 return -1;
}

unsigned int bitmap_get_size(unsigned int *y,unsigned int n)
{
 int x=0;
 if(n>=num_registered_bitmaps) return x;
 if(registered_bitmaps[n].hdr==NULL) return x;
 if(y!=NULL) *y=abs(registered_bitmaps[n].hdr->height);
 return registered_bitmaps[n].hdr->width;
}

/*Alle DDBs löschen
 um GDI-Handles vor dem Beenden freizugeben
*/
void reset_bitmap_slots(void)
{
 unsigned int a;
 if(!num_registered_bitmaps) return;
 for(a=0;a<MAX_BITMAPS;a++)
  if(slot_to_bitmap[a]>=0)
  {
   free_bitmap(a);
   registered_bitmaps[slot_to_bitmap[a]].slot=-1;
   slot_to_bitmap[a]=-1;
  }     
}

#pragma pack(push,1)
struct BX_FRAME
{
 uint32_t sizeimage;
 char bild_bmp[12];
 uint32_t dw_256;    
};
#pragma pack(pop)

static uint32_t mask_pal[256];
static int mask_pal_init=0;

int register_BX(unsigned int n,void *hdr,const char *name,unsigned transcol) /*hdr auf 40, nicht bx*/
{
 unsigned int a,b;
 int r=0;
 struct DIB_HEADER *dib=NULL;
 uint32_t *filehdr,*pal;
 uint8_t *dat;
 struct BX_FRAME *frame;
 if(!mask_pal_init)
 {
  for(a=1;a<256;a++)
   mask_pal[a]=0;
  mask_pal[255]=0xFFFFFF;
 }
 mask_pal[transcol]=0xFFFFFF;
 
 if(hdr==NULL) return 0;
 dib=hdr;
 filehdr=((uint32_t*)hdr)-3;
 dat=hdr;
 dat-=14;
 dat+=filehdr[2];
 pal=(uint32_t*)(((uint8_t*)hdr)+dib->size);
 frame=(void*) (pal+256);
 pal[transcol]=0;
 printf("%s",frame->bild_bmp);
 while(/*!memcmp(frame->bild_bmp,"BILD%D.BMP",10)*/frame->bild_bmp[0]<='Z'&&frame->bild_bmp[0]>='A')
 {
  register_bitmap(n+r,dib,pal,dat,dib,mask_pal,dat);
  registered_bitmaps[n+r].sizeimage=frame->sizeimage;
  dat+=frame->sizeimage;
  r++;
  frame++;
 }  
 if(name!=NULL)
  set_bitmap_name(n,name);
 printf("r=%d",r);
 return r;
}
