/*
	HStream
*/

#ifndef HSTREAM_H
#define HSTREAM_H

#include "HError.h"

#include <SupportDefs.h>
#include <DataIO.h>

template <class S>
class HStream
{
	typedef S stream_type;

  public:
	HStream(stream_type& s)
		: fStream(s) {}

	template <class D>
	HStream& operator>> (D& d)
	{
		fStream.Read(&d, sizeof(D));
		return *this;
	}
	
	HStream& operator>> (char* s)
	{
		unsigned char t;
		fStream.Read(&t, 1);
		fStream.Read(s, t);
		s[t] = 0;
		return *this;
	}
	
	template <class D>
	HStream& operator<< (const D& d)
	{
		fStream.Write(&d, sizeof(D));
		return *this;
	}
	
	HStream& operator<< (const char* s)
	{
		unsigned char t = strlen(s);
		fStream.Write(&t, 1);
		fStream.Write(s, t);
		return *this;
	}
	
	void seekp (off_t position, uint32 seek_mode = SEEK_SET)
	{
		fStream.Seek(position, seek_mode);
	}
	
	off_t tellp ()
	{
		return fStream.Position();
	}
	
	off_t size ()
	{
		off_t size, pos;
		pos = fStream.Position();
		size = fStream.Seek(0, SEEK_END);
		fStream.Seek(pos, SEEK_SET);
		return size;
	}
	
  private:
	stream_type& fStream;
};

void ReadCString(BPositionIO& stream, int maxLen, char *s);

template<class D>
inline BPositionIO& operator>>(BPositionIO& s, D& d)
{
	if (s.Read(&d, sizeof(D)) != sizeof(D))
		throw HErr("Error reading");
	return s;
} /* operator>> */

inline BPositionIO& operator>>(BPositionIO& stream, char *string)
{
	ReadCString(stream, 255, string);
	return stream;
} /* operator>> */

template<class D>
inline BPositionIO& operator<<(BPositionIO& s, const D& d)
{
	if (s.Write(&d, sizeof(D)) != sizeof(D))
		throw HErr("Error writing");
	return s;
} /* operator<< */

inline BPositionIO& operator<<(BPositionIO& stream, const char *string)
{
	int sl = strlen(string) + 1;
	if (stream.Write(string, sl) != sl)
		throw HErr("Error writing");
	return stream;
} /* operator<< */

#endif
