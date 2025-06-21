#ifndef BMCACHE_H
#define BMCACHE_H
extern int register_bitmap(unsigned int n,
 void *hdr,void *pal,void *dat,
 void *hdrm,void *palm,void *datm);
extern int register_BX(unsigned int n,void *hdr,const char *name,unsigned transcol);
extern int set_bitmap_tilesize(unsigned int n,unsigned int a);
extern int set_bitmap_scaling(unsigned int n,int on);
extern const char *set_bitmap_name(unsigned int n,const char *name);
extern int get_bitmap_slot(unsigned int n);
extern int bitmap_name_to_number(const char *name);
extern unsigned int bitmap_get_size(unsigned int *y,unsigned int n);
extern void reset_bitmap_slots(void);
#endif /*!BMCACHE_H*/
