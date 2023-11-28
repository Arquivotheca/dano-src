
#include	<SupportDefs.h>
#include	"rico.h"
#include	<memory.h>
#include	<stdlib.h>
#include	"XString.h"


XString::XString( )
{

	bufsize = 1;
	buffer = (uchar *) malloc( bufsize);
	head = 0;
	tail = 0;
}


XString::XString( const XString &x)
{

	bufsize = x.bufsize;
	buffer = (uchar *) malloc( bufsize);
	memcpy( buffer, x.buffer, bufsize);
	head = x.head;
	tail = x.tail;
}


XString	&
XString::operator=( const XString &x)
{

	bufsize = x.bufsize;
	if (buffer)
		free( buffer);
	buffer = (uchar *) malloc( bufsize);
	memcpy( buffer, x.buffer, bufsize);
	head = x.head;
	tail = x.tail;
}


XString::~XString( )
{

	if (buffer)
		free( buffer);
}


bool
XString::putm( uchar *s, uint n)
{

	unless (buffer)
		return (FALSE);
	for (uint i=0; i<n; ++i) {
		uint h = (head+1) % bufsize;
		if (h == tail) {
			bufsize *= 2;
			unless (buffer = (uchar *)realloc( buffer, bufsize))
				return (FALSE);
			if (head < tail) {
				memcpy( buffer+bufsize/2, buffer, head);
				head += bufsize / 2;
			}
			h = (head+1) % bufsize;
		}
		buffer[head] = s[i];
		head = h;
	}
	return (TRUE);
}


bool
XString::putw( ushort w)
{

	return (putm( (uchar *)&w, sizeof w));
}


bool
XString::putb( uint c)
{
	uchar	s[]	= ".";

	s[0] = c;
	return (putm( s, 1));
}


uint
XString::count( )
{

	unless (head < tail)
		return (head - tail);
	return (bufsize-tail + head);
}


uint
XString::getw( )
{

	return (*(ushort *)getm( sizeof( ushort)));
}


uint
XString::getb( )
{

	if ((not buffer)
	or (tail == head))
		return (-1);
	uint c = buffer[tail];
	tail = (tail+1) % bufsize;
	return (c);
}


uchar	*
XString::getm( uint n)
{
	uchar	*p;

	unless (buffer)
		return (0);
	if (head < tail) {
		unless (p = (uchar *)malloc( head))
			return (0);
		memcpy( p, buffer, head);
		uint i = bufsize - tail;
		memmove( buffer, buffer+tail, i);
		memcpy( buffer+i, p, head);
		free( p);
		head += i;
		tail = 0;
	}
	p = buffer + tail;
	tail = (tail+n) % bufsize;
	return (p);
}


void
XString::ungetm( uint n)
{

	tail = (tail-n) % bufsize;
}


uint
XString::unputb( )
{

	if ((not buffer)
	or (tail == head))
		return (-1);
	head = (head-1) % bufsize;
	return (buffer[head]);
}


void
XString::clear( )
{

	head = 0;
	tail = 0;
}
