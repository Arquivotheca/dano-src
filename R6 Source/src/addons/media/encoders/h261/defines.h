#ifndef __INCLUDE_defines_h
#define __INCLUDE_defines_h


/*
definition of BB_INT (bit buffer int), either 32 or 64 bits, depending on the running CPU
*/
#ifdef INT_64
	#define NBIT 	64	
	#define BB_INT 	INT_64
#else
	#define NBIT 	32
	#define BB_INT 	u_int
#endif

/*
storing and loading of the bit buffer (take care of endianess)
*/
#if BYTE_ORDER == LITTLE_ENDIAN
	#if NBIT == 64
		#define STORE_BITS(bb, bc) \
			bc[0] = bb >> 56; \
			bc[1] = bb >> 48; \
			bc[2] = bb >> 40; \
			bc[3] = bb >> 32; \
			bc[4] = bb >> 24; \
			bc[5] = bb >> 16; \
			bc[6] = bb >> 8; \
			bc[7] = bb;
		#define LOAD_BITS(bc) \
			((BB_INT)bc[0] << 56 | \
			 (BB_INT)bc[1] << 48 | \
			 (BB_INT)bc[2] << 40 | \
			 (BB_INT)bc[3] << 32 | \
			 (BB_INT)bc[4] << 24 | \
			 (BB_INT)bc[5] << 16 | \
			 (BB_INT)bc[6] << 8 | \
			 (BB_INT)bc[7])
	#else
		#define STORE_BITS(bb, bc) \
			bc[0] = bb >> 24; \
			bc[1] = bb >> 16; \
			bc[2] = bb >> 8; \
			bc[3] = bb;
		#define LOAD_BITS(bc) (ntohl(*(BB_INT*)(bc)))
	#endif
#else
	#define STORE_BITS(bb, bc) *(BB_INT*)bc = (bb);
	#define LOAD_BITS(bc) (*(BB_INT*)(bc))
#endif

/*
putting bits within the bit buffer
*/
#define PUT_BITS(bits, n, nbb, bb, bc) \
{ \
	nbb += (n); \
	if (nbb > NBIT)  { \
		u_int extra = (nbb) - NBIT; \
		bb |= (BB_INT)(bits) >> extra; \
		STORE_BITS(bb, bc) \
		bc += sizeof(BB_INT); \
		bb = (BB_INT)(bits) << (NBIT - extra); \
		nbb = extra; \
	} else \
		bb |= (BB_INT)(bits) << (NBIT - (nbb)); \
}


#endif
