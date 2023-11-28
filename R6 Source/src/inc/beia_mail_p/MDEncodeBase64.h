#ifndef __MD_ENCODE_BASE64_H__
#define __MD_ENCODE_BASE64_H__

#include <SupportDefs.h>

class BMallocIO;

class MDEncodeBase64
{
public:
		MDEncodeBase64();
virtual	~MDEncodeBase64();

void	decode_buffer(BMallocIO *);

private:
int16 decode_table[256];
int32 gotchars;
uint8 c1,c2,c3,c4;
};

#endif
