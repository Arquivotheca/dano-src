
#include "Cinepak.h"

/*	Draw an intraframe		*/

void cpDrawIntraframe(Byte *baseAddr,long rowBytes, long width, long height, long *codebook, Byte *data)
{
	long	*line0,*line1,*line2,*line3,*d;
	long	makeup,bits,bitsleft,ss;
	long	*smooth,*s;
	short	h,v,n;
	
	smooth = codebook + 256*4;
	line0 = (long *)baseAddr;
	line1 = line0 + (rowBytes >> 2);
	line2 = line1 + (rowBytes >> 2);
	line3 = line2 + (rowBytes >> 2);
	makeup = rowBytes - width;
	
	bitsleft = 0;
	for (v = (height >> 2); v--;) {
		for (h = (width >> 2); h--;) {
			if (!bitsleft) {
				for (bits = 0, n = 4; n--;)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				d = codebook + (*data++ << 2);
				*line0++ = *d++;
				*line0++ = *d++;
				*line1++ = *d++;
				*line1++ = *d++;

				d = codebook + (*data++ << 2);
				*line0++ = *d++;
				*line0++ = *d++;
				*line1++ = *d++;
				*line1++ = *d++;

				d = codebook + (*data++ << 2);
				*line2++ = *d++;
				*line2++ = *d++;
				*line3++ = *d++;
				*line3++ = *d++;

				d = codebook + (*data++ << 2);
				*line2++ = *d++;
				*line2++ = *d++;
				*line3++ = *d++;
				*line3++ = *d++;
			} else {
				s = smooth + (*data++ << 2);
				ss = *s++;
				*line0++ = ss;
				*line0++ = ss;
				*line1++ = ss;
				*line1++ = ss;

				ss = *s++;
				*line0++ = ss;
				*line0++ = ss;
				*line1++ = ss;
				*line1++ = ss;

				ss = *s++;
				*line2++ = ss;
				*line2++ = ss;
				*line3++ = ss;
				*line3++ = ss;

				ss = *s++;
				*line2++ = ss;
				*line2++ = ss;
				*line3++ = ss;
				*line3++ = ss;	
			}
			
			bits = (bits << 1);
			bitsleft--;
		}
		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
}

/*	Draw an interframe	*/

void cpDrawInterframe(Byte *baseAddr,long rowBytes,long width,long height,long *codebook, Byte *data)
{
	long	*line0,*line1,*line2,*line3,*d;
	long	*smooth,*s;
	long	makeup,bits,bitsleft,ss;
	short	h,v,n;
	
	smooth = codebook + 1024;
	line0 = (long *)baseAddr;
	line1 = line0 + (rowBytes >> 2);
	line2 = line1 + (rowBytes >> 2);
	line3 = line2 + (rowBytes >> 2);
	makeup = rowBytes - width;
	
	bitsleft = 0;
	for (v = 0; v < (height >> 2); v++)	{
		for (h = 0; h < (width >> 2); h++)	{
			if (!bitsleft) {
				for (bits = n = 0; n < 4; n++)	{
					bits = (bits << 8) | *data++;
				}
				bitsleft = 32;
			}
			
			if (bits < 0) {	
			
				bits = (bits << 1);
				bitsleft--;
				if (!bitsleft) {
					for (bits = n = 0; n < 4; n++)	{
						bits = (bits << 8) | *data++;
					}
					bitsleft = 32;
				}

				if (bits < 0) {	
					d = codebook + (*data++ << 2);
					*line0++ = *d++;
					*line0++ = *d++;
					*line1++ = *d++;
					*line1++ = *d++;
	
					d = codebook + (*data++ << 2);
					*line0++ = *d++;
					*line0++ = *d++;
					*line1++ = *d++;
					*line1++ = *d++;
	
					d = codebook + (*data++ << 2);
					*line2++ = *d++;
					*line2++ = *d++;
					*line3++ = *d++;
					*line3++ = *d++;
	
					d = codebook + (*data++ << 2);
					*line2++ = *d++;
					*line2++ = *d++;
					*line3++ = *d++;
					*line3++ = *d++;
				} else {
					s = smooth + (*data++ << 2);
					ss = *s++;
					*line0++ = ss;
					*line0++ = ss;
					*line1++ = ss;
					*line1++ = ss;
	
					ss = *s++;
					*line0++ = ss;
					*line0++ = ss;
					*line1++ = ss;
					*line1++ = ss;
	
					ss = *s++;
					*line2++ = ss;
					*line2++ = ss;
					*line3++ = ss;
					*line3++ = ss;
	
					ss = *s++;
					*line2++ = ss;
					*line2++ = ss;
					*line3++ = ss;
					*line3++ = ss;	
				}
				bits = (bits << 1);
				bitsleft--;
				
			}	else	{
			
				line0 += 4;
				line1 += 4;
				line2 += 4;
				line3 += 4;
				bits = (bits << 1);
				bitsleft--;
				
			}
		}
		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
}
