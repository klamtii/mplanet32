#ifndef _MPLANET_H_
#define _MPLANET_H_
#include "EDITION.H"
#ifdef GAME_MAGNETIC_PLANET

extern uint32_t wav_lens[WAVE_MAX];
extern void *wav_ptrs[WAVE_MAX];
extern int num_wav;

void recolour_disasters(unsigned int n);
void init_fonts(void);
int load_magnetic_planet(void);

#define NUM_TILES    60
extern const tile_t tile_info[NUM_TILES+1];
#define TILE_UNTERGRUND 1

#define TREIBSAND_MAX   10
#define TREIBSAND_MIN   5

#define FARBE_HELL      0xDFDF80
#define FARBE_DUNKEL    0x808000

/*extern anim_t **species_roboter;*/
extern anim_t *(species_anim[SPECIES_ANIM_NUM*6]);
void roboter_farbe_setzen(int f);
extern anim_t *(raubtier_anim[PREDATOR_ANIM_NUM*2]);

extern void make_mp16_species(void);
#endif /*GAME_MAGNETIC_PLANET*/
#endif /*_MPLANET_H_*/
