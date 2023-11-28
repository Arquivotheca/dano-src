#include "rico.h"

/********* public **********/
#include "mkralloc_pub.h"


#ifdef	__cplusplus
extern "C"
{
#endif

void				*malloc(size_t n);
void				*calloc( size_t n, size_t s);
void				free( void *p);
void				*realloc( void *old, size_t n);

void				*memalign(size_t alignment, size_t num_bytes);
void 				*valloc(size_t num_bytes);
void				*malloc_find_object_address(void *ptr);

/************** private **************/

static void			*alloc(char *f,size_t n);
static	void		checkbp( struct m *mp);
static struct m		*getmem( char *f);
static	void			doom( char *f);
static 	void			record( struct m *mp);
static struct m		*lookup( char *f, void *p);
static struct m		*detach( char *f, void *p);
static 	void			draconiancheck( );
static 	void			setborder( struct m *mp);
static 	void			checkborder( char *f, struct m *mp);
static	void			sowsalt( struct m *mp);
static 	void			checksalt( char *f, struct m *mp);
static struct m		*onlist( struct m *mp, void *p);
static 	void			trashout( char *f, struct m *mp);

static 	void			badpointer( char *f, void *p);
static void croak( char *f, char *mesg, ...);
static lock( bool acquire);
static initialize( );
static char	*getword( char **pp);

#ifdef	__cplusplus
}
#endif