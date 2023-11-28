#include "SharedCamera.h"


FilenameList::FilenameList()
{
}

FilenameList::~FilenameList()
{
	MakeEmpty();
}

void FilenameList::MakeEmpty()
{	
	uint8 *file;

	while((file = (uint8 *)RemoveItem((long int)0)) != NULL) {
		delete file;
	}
	BList::MakeEmpty();
}

uint8 calculate_checksum(uint8 *buf, int start_pos, int stop_pos)
{
	int i;
	uint8 chksum = 0;
	
	for(i=start_pos; i<=stop_pos; i++) {
		chksum += buf[i];
	}

	return chksum;
}

/*
#define clamp(x) \
{\
	if((x) > 255) x = 255;\
	if((x) < 0) x = 0;\
}	
*/

#define clamp(x) (x) = ((x) > 255) ? 255 : (((x) < 0) ? 0 : (x))



int YCCtoRGB32(uint8 *inbuf, uint8 *outbuf, uint32 inlen)
{
	int i;
	int outbuf_idx = 0;		
	float r_prime, g_prime, b_prime;
	float luma, chroma_b, chroma_r;
	
	if(inbuf == NULL || outbuf == NULL) {
		return B_ERROR;
	}
	
	// Convert from YCrCb format
	for(i=0; i<inlen; i+=4) {
		chroma_b = (float)inbuf[i+0];
		luma = (float)inbuf[i+1];
		chroma_r = (float)inbuf[i+2];
		// adjust ranges
		luma -= 14;
		chroma_b -= 128;
		chroma_r -= 128;
		// calc first pixel
		r_prime = 1.164 * luma + 1.596 * chroma_r;
		g_prime = 1.164 * luma - 0.813 * chroma_r - 0.392 * chroma_b;
		b_prime = 1.164 * luma + 2.017 * chroma_b;
		clamp(r_prime);
		clamp(g_prime);
		clamp(b_prime);
		outbuf[outbuf_idx++] = (uint8)b_prime;
		outbuf[outbuf_idx++] = (uint8)g_prime;
		outbuf[outbuf_idx++] = (uint8)r_prime;
		outbuf[outbuf_idx++] = 0;
		// stuff pixel data
		luma = (float)inbuf[i+3];
		luma -= 14;
		// calc second pixel
		r_prime = 1.164 * luma + 1.596 * chroma_r;
		g_prime = 1.164 * luma - 0.813 * chroma_r - 0.392 * chroma_b;
		b_prime = 1.164 * luma + 2.017 * chroma_b;
		clamp(r_prime);
		clamp(g_prime);
		clamp(b_prime);
		outbuf[outbuf_idx++] = (uint8)b_prime;
		outbuf[outbuf_idx++] = (uint8)g_prime;
		outbuf[outbuf_idx++] = (uint8)r_prime;
		outbuf[outbuf_idx++] = 0;
	}
	
	return B_NO_ERROR;				
}



