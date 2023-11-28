

#define	isuword( u)	(_utype_tab[(u)/8] & 1<<(u)%8)

extern uchar	_utype_tab[];
