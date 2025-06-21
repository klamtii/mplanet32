#ifndef _WELT_H_
#define _WELT_H_
extern struct WELT welt;
extern struct SPIELER spieler[6];
extern uint8_t welt_tilemap[28*28];
extern uint8_t welt_einheiten[28*28];
extern uint8_t welt_relief[28*28];

void welt_punkte_verteilen(void);
void welt_make_tilemap(void);
void welt_bildaufbau(int alles);
void welt_stat_info(int n);
void relief_erzeugen(int neu);
void dump_relief(int xdst,int ydst);
void ausbreitungsrunde(int spieler_num);
void ausbreitungsrunde_ki(int spieler_num);
int katastrophe_bild(int num);
int katastrophe(int num);
void welt_ende_pruefen(void);
int welt_sterben_nach_sammeln(int sp);
#endif /*!_WELT_H_*/
