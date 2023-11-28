
#if !defined(prefs_h)
#define prefs_h

struct prefs_struct {
	float x;
	float y;
};
extern prefs_struct prefs;

void get_prefs();
void put_prefs();

#endif	//	prefs_h
