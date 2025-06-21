#ifndef _EDITION_H_
#define _EDITION_H_
/*Version der Spielengine*/
#define ENGVERSION "1.0"
#ifdef __WIN32__
#define ENGTARGET "WIN32"
#define TARGET_WINGDI
/*#define TARGET_SDL1*/
#endif


/*
 Einstellen, ob Magnetic Planet oder Q-POP kompiliert werden soll
*/
#define GAME_MAGNETIC_PLANET
/*#define GAME_QPOP*/


#define FEATURE_HQX
/*#define FEATURE_DEBUG*/
#define FEATURE_UNICODE

/*Aufrufkonvention für Zeiger auf Funktionen*/
#define CALLCONV __cdecl

#ifndef FEATURE_DEBUG
#define printf
#endif

#endif /*!_EDITION_H_*/
