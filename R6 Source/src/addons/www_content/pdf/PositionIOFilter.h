#if !defined(_POSITIONIOFILTER_H_)
#define _POSITIONIOFILTER_H_

#include "Filter.h"
#include <DataIO.h>
#include <limits.h>

class BPositionIOInputFilter : public BInputFilter {
public:
					BPositionIOInputFilter(BPositionIO *source, ssize_t limit = LONG_MAX);
virtual				~BPositionIOInputFilter();
					
virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

off_t				Seek(off_t position, uint32 seek_mode);
ssize_t				GetLimit(void);
ssize_t				SetLimit(ssize_t limit);
BPositionIO			*GetIO(void);
BPositionIO			*SetIO(BPositionIO *new_source);

private:
BPositionIO			*m_io;
off_t				m_resetPosition;
ssize_t				m_limit;
ssize_t				m_totalRead;
};

class BPositionIOOutputFilter : public BOutputFilter {
public:
					BPositionIOOutputFilter(BPositionIO *sink);
virtual				~BPositionIOOutputFilter();
					
virtual ssize_t		Write(void *buffer, size_t size);
virtual void		Reset(void);

private:
BPositionIO			*m_sink;
off_t				m_resetPosition;
};


#endif
