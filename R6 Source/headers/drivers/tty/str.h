

#define	EOF	(-1)

struct str {
	uchar	*buffer;
	uint	bufsize,
		count,
		tail;
	bool	allocated;
};

#define	scount( s)	((s)->count)
#define	sclear( s)	((s)->count = 0)

bool	salloc( struct str *, uint),
	sputb( struct str *, uint),
	sputs( struct str *, uchar [], uint);
uint	sgetb( struct str *),
	sunputb( struct str *),
	sseglen( struct str *);
uchar	*sgets( struct str *, uint);
void	sinit( struct str *, uchar [], uint),
	sfree( struct str *),
	sungets( struct str *, uint);
