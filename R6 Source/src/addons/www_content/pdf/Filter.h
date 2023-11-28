#if !defined(_FILTER_H_)
#define _FILTER_H_

#include <size_t.h>

class BInputFilter {
public:
					BInputFilter(BInputFilter *source);
virtual				~BInputFilter();

					enum {
						ERROR = -1,
						END_OF_INPUT = -2
					};
virtual	ssize_t		Read(void *buffer, size_t size);
virtual void		Reset(void);

BInputFilter		*SetSource(BInputFilter *new_source);
const BInputFilter	*GetSource(void);

private:
BInputFilter		*m_source;
};

class BOutputFilter {
public:
					BOutputFilter(BOutputFilter *sink);
virtual				~BOutputFilter();

					enum {
						ERROR = -1,
						END_OF_OUTPUT = -2
					};
virtual ssize_t		Write(void *buffer, size_t size);
virtual void		Reset(void);

BOutputFilter		*SetSink(BOutputFilter *new_sink);
const BOutputFilter	*GetSink(void);

private:
BOutputFilter		*m_sink;
};

#endif
