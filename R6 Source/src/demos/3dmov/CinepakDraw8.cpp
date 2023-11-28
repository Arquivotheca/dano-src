
#include "Cinepak.h"

/*	Draw an intraframe		*/

void cpDrawIntraframe8(Byte *baseAddr, long rowBytes, long width, long height, long *codebook, Byte *data)
{
	long	*line0,*line1,*line2,*line3,makeup;
	unsigned long	c0,c1;
	long	*smooth,*s;
	long	bits,bitsleft;
	short	h,v,n;
	
	makeup = rowBytes;
	rowBytes >>= 2;				// Convert to longs
	width >>= 2;
	
	smooth = codebook + 256;	// Detail codebook is 256*4 bytes, smooth is 256*16
	line0 = (long *)baseAddr;
	line1 = line0 + rowBytes;
	line2 = line1 + rowBytes;
	line3 = line2 + rowBytes;
	
	bitsleft = 1;
	for (v = (height >> 2); v--;) {
		for (h = 0; h < width; h++) {
			if (--bitsleft == 0) {
				for (bits = 0, n = 4; n--;)
					bits = (bits << 8) | *data++;
				bitsleft = 32;
			}
			
			if (bits < 0) {	
				
				c0 = codebook[data[0]];		// Detail position 0,1
				c1 = codebook[data[1]];
				line0[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
				line1[h] = (c0 << 16) | (c1 & 0x0FFFF);
				
				c0 = codebook[data[2]];		// Detail position 2,3
				c1 = codebook[data[3]];
				line2[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
				line3[h] = (c0 << 16) | (c1 & 0x0FFFF);
				data += 4;
				
			} else {
			
				s = smooth + (*data++ << 2);	// Smooth
				line0[h] = s[0];
				line1[h] = s[1];
				line2[h] = s[2];
				line3[h] = s[3];
				
			}
			
			bits = (bits << 1);
		}
		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
}

/*	Draw an interframe	*/

void cpDrawInterframe8(Byte *baseAddr,long rowBytes,long width,long height,long *codebook, Byte *data)
{
	long	*line0,*line1,*line2,*line3,makeup;
	unsigned long	c0,c1;
	long	*smooth,*s;
	long	bits,bitsleft;
	short	h,v,n;
	
	makeup = rowBytes;
	rowBytes >>= 2;				// Convert to longs
	width >>= 2;
	
	smooth = codebook + 256;	// Detail codebook is 256*4 bytes, smooth is 256*16
	line0 = (long *)baseAddr;
	line1 = line0 + rowBytes;
	line2 = line1 + rowBytes;
	line3 = line2 + rowBytes;
	
	bitsleft = 1;
	for (v = (height >> 2); v--;) {
		for (h = 0; h < width; h++) {
			if (--bitsleft == 0) {
				for (bits = 0, n = 4; n--;)
					bits = (bits << 8) | *data++;
				bitsleft = 32;
			}
			
			if (bits < 0) {	
			
				bits = (bits << 1);
				if (--bitsleft == 0) {
					for (bits = 0, n = 4; n--;)
						bits = (bits << 8) | *data++;
					bitsleft = 32;
				}

				if (bits < 0) {	
					
					c0 = codebook[data[0]];		// Detail position 0,1
					c1 = codebook[data[1]];
					line0[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
					line1[h] = (c0 << 16) | (c1 & 0x0FFFF);
					
					c0 = codebook[data[2]];		// Detail position 2,3
					c1 = codebook[data[3]];
					line2[h] = (c0 & 0xFFFF0000) | (c1 >> 16);
					line3[h] = (c0 << 16) | (c1 & 0x0FFFF);
					data += 4;
					
				} else {
				
					s = smooth + (*data++ << 2);	// Smooth
					line0[h] = s[0];
					line1[h] = s[1];
					line2[h] = s[2];
					line3[h] = s[3];
					
				}
			}
			
			bits = (bits << 1);
		}
		line0 += makeup;
		line1 += makeup;
		line2 += makeup;
		line3 += makeup;
	}
}
