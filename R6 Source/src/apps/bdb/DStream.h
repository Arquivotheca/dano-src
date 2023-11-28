/*	$Id: DStream.h,v 1.1 1998/10/21 12:03:07 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 29 September, 1998 10:58:05
*/

#ifndef DSTREAM_H
#define DSTREAM_H

class DStream
{
  public:
	DStream (const char *ptr, size_t sz)
		: data (ptr), size (sz), offset (0)
	{};
	
	void Read (char *dt, size_t sz)
	{
		memcpy(dt, data + offset, sz);
		offset += sz;
	};

	void Reset ()
	{
		offset = 0;
	}

  private:
  	char	*data;
  	size_t	size;
  	uint32	offset;
};

template <class T>
const DStream& operator >> (const DStream& s, T& t)
{
	s.Read(&t, sizeof(T));
} // operator >>



#endif
