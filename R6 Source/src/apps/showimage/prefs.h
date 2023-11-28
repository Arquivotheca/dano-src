/*	prefs.h	*/

#if !defined(PREFS_H)
#define PREFS_H

struct prefs {
	bool dither;
};


extern prefs g_prefs;

void init_prefs();
void save_prefs();


#endif /* PREFS_H */

