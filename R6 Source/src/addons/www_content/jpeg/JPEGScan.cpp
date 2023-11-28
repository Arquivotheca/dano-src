// ===========================================================================
//	JPEGScan.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <sys/param.h>
#include	"JPEG.h"

static long DecodeComponentPro(JPEGDecoder *j, int count,CompSpec *cspec, short *blocks);
static JPEGError InitJPEGDecoder(JPEGDecoder* j);
static void ResetJPEGDecoder(JPEGDecoder *j);
static void EndJPEGImage(JPEGDecoder *j);
static void PurgeJPEGDecoder(JPEGDecoder* j);
static void ResetScan(JPEGDecoder* j);
static void SetupScan(JPEGDecoder* j);
static void BeginInterval(JPEGDecoder* j);
static JPEGError DecodeScan(JPEGDecoder *j, short marker);
static long SkipScan(JPEGDecoder* j, const uchar* data, long dataLength);
static long ScanWrite(JPEGDecoder* j, const uchar* data, long dataLength);
static long NextMarker(const uchar *data,long dataLength,uchar *rMarker,ushort *rSize);

//	Read a group of bits from the stream
//	Only used inside a scan, so we can go to town with stripping 0xFF00 stuffing
//	NOTE:	This is the only part of the code that uses VLC
#define	bmask(_bc)	((1<<(_bc))-1)

#define get_bits(_x) (((_x) <= j->bits) ? (j->last32 >> (j->bits -= (_x))) & bmask(_x) :  GetBitsRead(j,(_x)))
#define next_bits(_x) (((_x) <= j->bits) ? (j->last32 >> (j->bits - (_x))) & bmask(_x) : NextBitsRead(j,(_x)))

#define next32bits() ((32 <= j->bits) ? (j->last32 >> (j->bits - 32))  : NextBitsRead(j,32))

#define skip_bits(_x)	if ((j->bits -= _x) < 0) {			\
							j->bits += 32;					\
							j->last32 = j->next32;			\
							j->next32 =  LONG(j->data);		\
							j->data += 4;					\
						}


// register based for speed -- need to check compiler output
#define l_get_bits(_x)	\
if ((_x) <= bits)					\
	bits_result = (last32 >> (bits -= (_x))) & bmask(_x);		\
else 	{	\
	tcount = (_x) - bits;			\
	bits_result = (last32 & bmask(bits)) << tcount;					\
	last32 = next32;  next32 = *data++;	bits = 32 - tcount;					\
	bits_result |= (last32 >> bits) & bmask(tcount);			\
}


#define l_next_bits(_x) 	\
if  ((_x) <= bits) 	\
	bits_result = (last32 >> (bits - (_x))) & bmask(_x);	\
else {		\
	tcount = (_x) - bits;	\
	bits_result = ((last32 & bmask(bits)) << tcount)  | (next32 >> (32 - tcount))  & bmask(tcount);	\
}						

#define l_skip_bits(_x)			\
if ((bits -= _x) < 0) {			\
	bits += 32;					\
	last32 = next32;			\
	next32 = *data++;			\
}

static inline long GetBitsRead(JPEGDecoder *j, long count)
{
	long a,b;
	long bits = j->bits;
	count -= bits;
	if (count == 32)
		a = 0;
	else
		a = (j->last32 & bmask(bits)) << count;
	b = j->last32 = j->next32;
	j->next32 = LONG(j->data);
	j->data += 4;
	j->bits = 32 - count;
	if (count)
		return a | (b >> j->bits) & bmask(count);
	else
		return a;
}

static inline long NextBitsRead(JPEGDecoder *j,long count)
{
	count -= j->bits;
	if (count == 32)
		return	((j->last32 & bmask(j->bits))) | j->next32;
	else if (count)
		return	((j->last32 & bmask(j->bits)) << count) | (j->next32 >> (32 - count)) & bmask(count);
	else
		return	(j->last32 & bmask(j->bits));
	
}


//	Decode components within MDU
static short GetCoeff(JPEGDecoder *j,HuffmanTable *h, ushort& run)
{
	uint16	code;
	short	l,s,i;
	FastRec *f;

	code = next_bits(8);
	f = h->fast + code;
	if (f->length > 0) {			// Got a whole extended value
		skip_bits(f->length);
		run = f->run;
		return f->value;
	}
	if (f->length < 0) {			// Got a symbol
		skip_bits(-f->length);
		run = f->run;
		s = f->value;
		i = get_bits(s);
		return 	Extend(i,s);
	}

	//	Do the slow case, rare: < 5%, bad huffman codes show up here
	skip_bits(8);
	l = 8;
	while (code > h->maxcode[l]) {
		code = (code << 1) + get_bits(1);
		l++;
	}

	if (l > 16) {
		printf("Bad Huffman code in GetCoeff");
		j->error = kBadHuffmanErr;
		return 0;
	}

	i = h->valptr[l] + (code - h->mincode[l]);
	s = h->huffval[i];
	run = s >> 4;				// Run
	if (!(s &= 0xF)) return 0;	// Size
	i = get_bits(s);
	return 	Extend(i,s);
}

static short HuffmanDecode(JPEGDecoder *j, HuffmanTable *h,ushort &run)
{
	ushort	code;
	short	l;
	uchar	s;	
	FastRec *f;
	ulong bitbuffer;
	int bitcount = 32;
	short	v;

	bitbuffer = next32bits();
	code = (bitbuffer >> 24) & bmask(8);
	f = h->fast + code;
	l = f->length;

	if (l > 0) { 			// Got a whole extended value
		bitcount -= l;
		run = f->run;
		v = f->value;
	} else if (l < 0) {		// Got a symbol
		run = f->run;
		s = f->value;
		bitcount -= s-l;
		l = (bitbuffer >> bitcount) & bmask(s);
		v = Extend(l,s);
	} else {
		bitcount -= 8;
		l = 8;
		while (code > h->maxcode[l]) {
			code <<= 1;
			if (--bitcount < 0) {
				l = 17;
				break;
			}

			code += (bitbuffer >> bitcount) & bmask(1);
			l++;
		}
		
		if (l > 16) {		// bad huffman code
			run = 64;
			v = -1;
			printf("Invalid Huffman code");
			goto done;
		}
		
		if (code < h->mincode[l] || code > h->maxcode[l]) {
			j->error = kBadHuffmanErr;
			return -1;
		}

		s = h->huffval[ h->valptr[l] + (code - h->mincode[l]) ];
		run = s >> 4;
		v = s & 0xf;
		if (v) {
			bitcount -= v;
			code = (bitbuffer >> bitcount) & bmask(v);
			v = Extend(code,v);
		}
	}

done:
	skip_bits(32-bitcount);
	return v;
}


static long DecodeComponentPro(JPEGDecoder *j, int count, CompSpec *cspec, short *blocks)
{
	int				k,b;
	short			v;
	ushort			run;
	uchar			s;
	long			l;
	ushort			code;
	HuffmanTable 	*h = cspec->ACH;
	short	dcSAVal		 = 1<<cspec->dcBitPos;
	short	acSAVal		 = 1<<cspec->acBitPos;
	const	uchar 	*ZZ = j->ZZ;
	
	if (blocks == NULL)
		blocks = j->blocks;

	for (b = 0; b < count; b++) {
		// Progressive JPEG DC 
		if (cspec->specStart == 0) {
			// first or only scan 
			if (cspec->dcPrevBitPos == 0) {
				v = HuffmanDecode(j,cspec->DCH,run);
				cspec->DC += v;
				blocks[0] = cspec->DC << cspec->dcBitPos;
			} else {
				// successive approximation refinement scan
				if (get_bits(1)) 
					cspec->DC += dcSAVal;
			}
		} else { 
			// Progressive JPEG AC 
			// first or only scan for this band
			
			if (cspec->acPrevBitPos == 0) 
			{
				if (j->bandSkip != 0) 
					j->bandSkip--;
				else 
				{
					for (k = cspec->specStart; k <= cspec->specEnd; k++) 
					{
						// decode the next huffman code
						
						v = HuffmanDecode(j,cspec->ACH,run);

						// process the decoded value
						
						if (v) {
							// run of zeros followed by a value
							k += run;
							if (k > cspec->specEnd) 
							{
								printf("Coeff past end of band");
								j->error = kBadHuffmanErr;
								return -1;
							}
							blocks[ZZ[k]] = v << cspec->acBitPos;
						} else  {
							if (run == 15) {
								// run of zeros followed by zero
								k += run;
								if (k > cspec->specEnd) 
								{
									printf("Coeff run past end of band");
									j->error = kBadHuffmanErr;
									return -1;
								}
							}
							else 
							{	
								if (run > 14) {
									j->error = kBadHuffmanErr;
									return -1;
								}
								// is not a run but a end of band code
								j->bandSkip = (1<<run);
								if (run) 
									j->bandSkip += get_bits(run);
								j->bandSkip--;
								break;
							}
						}
					}
				}
			} else {
				short *zcoeffp;
				short zcoef;
			
				// successive approximation refinement scan for this band
				k = cspec->specStart;
				if (j->bandSkip == 0) {
					for (k = cspec->specStart; k <= cspec->specEnd; k++) {
						long bitcount = 16;
						ulong bitbuffer = next_bits(16);
						code = (bitbuffer >> 8) & bmask(8);
						FastRec *f = h->fast + code;
						l = f->length;
						if (l > 0) {			// Got a whole extended value
							bitcount -= l;
							run = f->run;
							v = f->value;
							if (v != 0) {
								if (v == Extend(1,1))
									v = acSAVal;
								else 
									v = -acSAVal;
							}
						} else if (l < 0) {			// Got a symbol
							run = f->run;
							s = f->value;
							bitcount -= s-l;
							if (s == 1)  {
								if ((bitbuffer >> bitcount) & bmask(1))
									v = acSAVal;
								else
									v = -acSAVal;
							} else 
								v = 0;
						} else {
							l = 8;
							bitcount -= l;
							while (code > h->maxcode[l]) {
								if (--bitcount < 0) {
									l = 17;	// force error
									break;
								}
								code <<= 1;
								code += (bitbuffer >> bitcount) & bmask(1);
								l++;
							}

							if (l > 16) {
								printf("Bad AC Successive Approximation Huffman code");
								j->error = kBadHuffmanErr;
								return -1;
							}

							if (code < h->mincode[l]
								|| code > h->maxcode[l]) {
								j->error = kBadHuffmanErr;
								return -1;
							}

							s = h->huffval[ h->valptr[l] + (code - h->mincode[l]) ];
							run = s >> 4;				
							s &= 0xf;
							if (s != 0) {
								if (s != 1) {
									j->error = kBadHuffmanErr;
									printf("Bad AC Successive Approximation Correction Huffman code");
									return -1;
								}

								bitcount--;
								if ((bitbuffer >> bitcount) & bmask(1))
									v = acSAVal;
								else
									v = -acSAVal;
							} else
								v = 0;
						}
						skip_bits(16-bitcount);
						if (v == 0) {
							if (run != 15) {
								if (run > 14) {
									j->error = kBadHuffmanErr;
									return -1;
								}

								j->bandSkip = (1<<run);
								if (run) 
									j->bandSkip += get_bits(run);
								break;
							}
						}
					
						// look for the next zero-history coefficient
						//
						// if run is non zero, skip that many zero-history coef
						//      before stopping
						// for all nonzero history coefficients passed over
						//		read correction bits from the stream
						if (run > (cspec->specEnd-k)+1) {
							j->error = kBadHuffmanErr;
							printf("Bad AC Successive Approximation run length code %d > %d-%d+1",run,cspec->specEnd,k);
							return -1;
						}
						
						while (k <= cspec->specEnd) {
							zcoeffp = blocks+ZZ[k];
							zcoef = *zcoeffp;
							if (zcoef) {
								// has history, read correction bit from stream
								if (get_bits(1)) 
								{		// needs correction 
									if (zcoef & acSAVal) 
										*zcoeffp = zcoef + (zcoef >= 0 ? acSAVal : -acSAVal);
								}
							} else {
								// end after we get to the first non-zero history
								// coefficient after the run is exhausted
								if (run-- == 0)
									break;
							}
							k++;
						}
						if (v) 
							blocks[ZZ[k]] = v;
					}
				}
			
				// continue processing correction bits till end of band
				if (j->bandSkip != 0) {
					for (; k <= cspec->specEnd; k++) {
						zcoeffp = blocks + ZZ[k];
						if ((zcoef = *zcoeffp) != 0) {
							if (get_bits(1)) {
								if (zcoef & acSAVal) 
									*zcoeffp = zcoef + (zcoef >= 0 ? acSAVal : -acSAVal);
							}
						}
					}
					j->bandSkip--;
				}
			}
		}
		if (j->error)
			return -1;
		if (blocks)
			blocks += kMCUSize;
	}
	return 0;
}

long DecodeComponent(JPEGDecoder *j,short count,CompSpec *cspec,short *blocks)
{
	// progressive JPEG huffman decoding

	if (j->isProgressive) 
		return DecodeComponentPro(j,count,cspec,blocks);
	else {
		int	k,b;
		long	v;
		ushort	run;
		const	uchar 	*ZZ = j->ZZ;

		if (blocks == NULL) {
			for (b = 0; b < count; b++, blocks += kMCUSize) {
				// F.2.2.1 Huffman decoding of DC coefficients
				v = GetCoeff(j,cspec->DCH,run);
				cspec->DC += v;

				//	F.2.2.1 Huffman decoding of AC coefficients
				for (k = 1; k < kMCUSize; k++) {
					v = GetCoeff(j,cspec->ACH,run);
						k += run;
					if ((v|run) == 0) 
						break;
				}
			}
		} else {
			// sequential JPEG huffman decode
			ushort *QTable = cspec->QTable;
			for (b = 0; b < count; b++, blocks += kMCUSize) {
				// F.2.2.1 Huffman decoding of DC coefficients
				v = GetCoeff(j,cspec->DCH,run);
				if (run != 0) 
					goto fail;
				cspec->DC += v;
				blocks[0] = cspec->DC * QTable[0];
			  
				// F.2.2.1 Huffman decoding of AC coefficients
				for (k = 1; k < kMCUSize; k++) {
					v = GetCoeff(j,cspec->ACH,run);
						k += run;
					if (v) {
						if (k > kMCUSize)		// dont trash memory if we get bad data
							break;
						blocks[ZZ[k]] = v * QTable[k];
					} else {
						if (run == 0) 		// zero = end of block
							break;
					}
				}
			}
		}
		return 0;

fail:
		j->error = kBadHuffmanErr;
		printf("Bad Huffman code");
		return -1;
	}
}

void DumpJPEGDecode(JPEGDecoder*)
{
#if	0
	short	n;
	JPEGMessage(("Width: %d, Height: %d, Restart interval: %d", j->Width, j->Height, j->RstInterval));
	JPEGMessage(("First Slice: %d Last Slice %d", j->firstSlice,j->lastSlice));
	if (j->isProgressive) 
		JPEGMessage("Progressive JPEG");
	JPEGMessage(("Clip: %d %d %d %d", j->fClipRectangle.left, j->fClipRectangle.top,j->fClipRectangle.right,j->fClipRectangle.bottom));
	JPEGMessage(("MCUWidth: %d, MCUHeight: %d, MCU (%d x %d)", j->WidthMCU, j->HeightMCU, j->MCUHPixels,j->MCUVPixels));
	JPEGMessage((""));
	JPEGMessage(("Components in Frame: %d", j->CompInFrame));
	JPEGMessage(("Components in Scan: %d", j->CompInScan));
	for (n = 0; n < j->CompInFrame; n++) 
	{
		if (j->Comp[n].inScan) JPEGMessage(("In Scan"));
		JPEGMessage(("    Component identifier     (Ci): %d", j->Comp[n].Ci));
		JPEGMessage(("    Horizontal blocks in MCU (Hi): %d", j->Comp[n].Hi));
		JPEGMessage(("    Vertical blocks in MCU   (Vi): %d", j->Comp[n].Vi));
		JPEGMessage(("    Blocks in MCU     	   (Ta): %d", j->Comp[n].blocksMCU));
		JPEGMessage(("    Blocks in image     	   (Ta): %d", j->Comp[n].blockCount));
		JPEGMessage((""));
	}
#endif
}

static JPEGError InitJPEGDecoder(JPEGDecoder* j)
{
	uchar	*HTableData;
	uint16	*QTableData;
	int		n;
	
	static const uchar ZZ[kMCUSize] = {	
		 0, 1, 8,16, 9, 2, 3,10,
		17,24,32,25,18,11, 4, 5,
		12,19,26,33,40,48,41,34,
		27,20,13, 6, 7,14,21,28,
		35,42,49,56,57,50,43,36,
		29,22,15,23,30,37,44,51,
		58,59,52,45,38,31,39,46,
		53,60,61,54,47,55,62,63
	};
	
	memset((uchar *)j,0,sizeof(JPEGDecoder));
	j->ZZ = ZZ;
	
	// we could allocate these as we get them to save some memory 
															// Set everything to zero	
	QTableData = (uint16*)malloc(kMCUSize*kMaxQuantizationTables*sizeof(uint16));					// Up to 4 Q tables
	HTableData = (uchar*)malloc(sizeof(HuffmanTable)*kMaxHuffmanTables);	// 4 DC and 4 AC tables

	if (!HTableData || !QTableData) 
		return kLowMemory;
		
	for (n = 0; n < kMaxHuffmanTables; n++) 
	{											// Setup pointers to Q and H tables
		j->HTables[n] = HTableData;
		HTableData += sizeof(HuffmanTable);
	}
	for (n = 0; n < kMaxQuantizationTables; n++) 
	{											// Setup pointers to Q and H tables
		j->QTables[n] = QTableData;
		QTableData += kMCUSize;
	}
	j->inBuffer = 0;
	j->data = j->dataEnd = 0;
	j->CompInFrame = 0;
	j->CompInScan = 0;
	j->WriteBuffer = (uchar*)malloc(kJPEGBufferSize);
	if (j->WriteBuffer == NULL)
		return kLowMemory;
	return kNoError;
}


static void ResetJPEGDecoder(JPEGDecoder *j)
{
	j->data = j->dataEnd = 0;
	j->bits = 0;
	j->phase = kWaitingForNewImage;
	ResetScan(j);
}

static void EndJPEGImage(JPEGDecoder *j)
{
	ResetJPEGDecoder(j);
	j->phase = kSkippingToEnd;
}


JPEGDecoder* NewJPEGDecoder(const BRect* r, short thumbnail, DrawRowProcPtr drawProc, const BRect* invalid)
{
	JPEGDecoder* j;

	j = (JPEGDecoder*)malloc(sizeof(JPEGDecoder));
	if (j) 
	{ 
		if (InitJPEGDecoder(j) != kNoError) 
		{
			DisposeJPEGDecoder(j);
			return NULL;
		}
		j->mDrawRect = *r;
		j->thumbnail = thumbnail;
		j->SingleRowProc = drawProc;
		j->mClipRect = (invalid == NULL) ? *r : *invalid;
	}

	return j;
}


static void PurgeJPEGDecoder(JPEGDecoder* j)
{
	long i;

	if (j->QTables[0] != NULL)
		free(j->QTables[0]);
	if (j->HTables[0] != NULL)
		free(j->HTables[0]);

	for (i = 0; i < kMaxFrameComponents; i++) 
	{
		if (j->Comp[i].blockBufferData) {
			free(j->Comp[i].blockBufferData);
			j->Comp[i].blockBufferData = NULL;
		}
		j->Comp[i].blockBuffer = NULL;
	}
	for (i = 0; i < kNumDecompressBuffers; i++)
		if (j->buffer[i] != NULL)
			free(j->buffer[i]);
	if (j->WriteBuffer != NULL)
		free(j->WriteBuffer);
}

// Don't call this on an uninitalized JPEGDecoder
void	DisposeJPEGDecoder(JPEGDecoder* j)
{
	PurgeJPEGDecoder(j);
	free(j);
}

static void ResetScan(JPEGDecoder* j)
{
	j->inBuffer = 0;
	j->currentSlice = 0;
	j->MCUCount = 0;
	j->bandSkip = 0;
	j->skipToFirstSlice = false;
	j->SkipToRestartMarker = 0;
	j->didDraw = 0;
	j->Interval = 0;
}

static void SetupScan(JPEGDecoder* j)
{
	ResetScan(j);
	if (j->CompInScan == 1) 
	{	
		// non-interleaved unused MCU blocks not included
		
		long bh,bv;
		long sWidthMCU,sHeightMCU;
			
		bh = j->MCUHPixels;
		bv = j->MCUVPixels;
		
		if (j->thumbnail) 
		{
			bh <<= 3;
			bv <<= 3;
		}
		bh /= j->scanComps[0]->Hi;
		bv /= j->scanComps[0]->Vi;
		sWidthMCU = (j->Width + bh-1)/bh;
		sHeightMCU = (j->Height + bv-1)/bv;

		j->ScanMCUs = sWidthMCU * sHeightMCU;
		j->ScanWidth = sWidthMCU;
	}
	else 
	{
		// interleaved - full MCUs
		j->ScanWidth = j->WidthMCU;
		j->ScanMCUs = j->WidthMCU * j->HeightMCU;
	}
	j->multiPassScan++;
}

                                                                                                  

//	Setup at beginning of restart interval
static void BeginInterval(JPEGDecoder* j)
{
	short	i;
	CompSpec **cspec;

	cspec = j->scanComps;
	for (i = 0; i < j->CompInScan; i++, cspec++) 
		(*cspec)->DC = 0;		// Reset DC prediction
	j->bandSkip = 0;

	if (!(j->Interval = j->RstInterval))
		j->Interval = j->ScanMCUs;			// Restart interval counter (or full scan)

	j->last32 = LONG(j->data);	// Prime the data pump
	j->data += 4;
	j->next32 = LONG(j->data);	// Prime the data pump
	j->data += 4;
	j->bits = 32;
	j->RstMarker = 0;								// Reset restart marker
}


//		
//		DecodeScan draws as much data as is in the buffer.
//		If a restart marker is detected, DecodeScan can draw
//		until the end of the scan without worring about checking
//		for underflows
//
#define skip_bits(_x)	if ((j->bits -= _x) < 0) {			\
							j->bits += 32;					\
							j->last32 = j->next32;			\
							j->next32 =  LONG(j->data);		\
							j->data += 4;					\
						}

#define	BITCOUNT	(((j->data - dataStart - 8) <<3)  +  (32 - j->bits))


/*
	Convert the current MCU from DCT to pixels, dequantizing as we go.
	The mcu is assumed to be in interleaved order starting at src.
	if src is NULL thenmcuNum refers to the mcu index in the block buffer for each 
	component.

*/

#if __INTEL__ /* XXX */
# define INTEL_MODULO_COMPILER_BUG  1
#endif

static JPEGError DecodeScan(JPEGDecoder *j, short marker)
{
	long 		i;
	long		xMCU,yMCU;
	CompSpec	*cspec;
	short		restartMarker = j->RstMarker;
	bool		skipScan = false;
	bool		visible = true;
	bool		useBlockBuffer = j->isProgressive && !j->thumbnail;
	uchar* 		dataStart = j->data;
	long		startBits = 0;
	long		MCUsPerSlice = 0,MCUsTillSlice = 0;
	short		*b;
	long		blockbufsize;
	bool 	scanIsNonInterleaved = (j->CompInScan == 1 && j->CompInFrame > 1);
#if INTEL_MODULO_COMPILER_BUG
	long f;
	long g;
#endif	
	if (j->dataEnd - j->data == 0)
		return kNoError;

	// Start a new interval if there was a restart marker or start a new scan
	if (j->RstMarker) {
		if (j->RstMarker == kSOS)	
			SetupScan(j);

#ifdef	CHK_MIN_DATA
		if (j->dataEnd - j->data > 8)	// Need at least 8 bytes to start interval
			BeginInterval(j);
		else 
			return kNoError;			// Wait for more data to come
#else
		BeginInterval(j);
#endif

	}
	
	startBits = BITCOUNT;

	if (j->currentSlice > j->lastSlice) 
		skipScan = true;
	
	// the following may work with progressive but it's not tested
	if (!j->isProgressive) {
		/* clip an entire restart interval if possible */
		if ((restartMarker & 0xf8) == 0xd0 && (j->MCUCount + j->Interval) < j->firstMCU) {
			j->RstMarker = 0xd0 + ((restartMarker + 1) & 7);
			if (marker == 0) 
				j->SkipToRestartMarker = j->RstMarker;

			j->data = j->dataEnd;
			j->bits = 0;
			j->MCUCount += j->Interval;
			j->currentSlice = j->MCUCount / j->WidthMCU; 
			j->Interval = 0;
			j->bandSkip = 0;
			goto done;
		}
	}
	
	if (skipScan) {
		ResetScan(j);
		j->currentSlice = j->lastSlice+1;		// make sure we keep skipping
		j->phase = kSkippingScan;
		goto done;
	}
	
	//	We can decode until the end of image/restart interval,
	//	or until we run out of data

#if INTEL_MODULO_COMPILER_BUG
	f = j->MCUCount;
	g = j->ScanWidth;
	xMCU = f % g;
	yMCU = f / g;
	//	printf("this is a compiler bug\n");
#else
	xMCU = j->MCUCount % j->ScanWidth;
	yMCU = j->MCUCount / j->ScanWidth;
#endif

	if (scanIsNonInterleaved) {
		if (!j->isProgressive) 
			useBlockBuffer = false;

		MCUsPerSlice =  j->ScanWidth * j->scanComps[0]->Vi;
		MCUsTillSlice = j->MCUCount % MCUsPerSlice;
	}
	
	blockbufsize = 0;
	for (i=0; i < j->CompInFrame; i++) {
		cspec = &j->Comp[i];
		blockbufsize += cspec->blocksMCU * kMCUSize * sizeof(ushort);
		if (useBlockBuffer) {
			if (cspec->blockBufferData == NULL) {
				j->error = kGenericError;
				goto done;
			}

			cspec->blockBuffer = cspec->blockBufferData;
		}
	}
	
	while (j->Interval--) {			// Decode the next MCU
		bool completedSlice = false;
		if (!marker && j->dataEnd - j->data < kMDUMin)	{	
			j->Interval++;
			break;
		}

		// Huffman decode
		if (scanIsNonInterleaved) {
			// non interleaved scan (in interleaved frame)
			CompSpec *nicomp = j->scanComps[0];
			bool skipMCU = false;
			long mcuNum,mcuRow = j->MCUCount/j->ScanWidth;
			
			// calculate the mcu number (relative to place in frame) based on position in scan
			// NOTE: this only supports interleaves of 1 or 2...
			
			if (nicomp->Vi == 2)
				mcuNum = (mcuRow>>1)  * j->WidthMCU;
			else
				mcuNum = mcuRow  * j->WidthMCU;
			if (nicomp->Hi == 2)
				mcuNum += xMCU >> 1;
			else
				mcuNum += xMCU;
			
			if (!skipMCU) {
				if (!j->isProgressive) {
					b =j->blocks;
					memset(b,0,kMCUSize * sizeof(ushort));
					DecodeComponent(j,1,nicomp,b);
				} else {
					b = nicomp->blockBuffer + nicomp->blocksMCU * (mcuNum << kMCUSizeShift);
					if (nicomp->Hi == 2)
						b += (xMCU & 1) << kMCUSizeShift;
					if (nicomp->Vi == 2) 
						b += (nicomp->Hi * (mcuRow & 1)) << kMCUSizeShift;
					j->skipToFirstSlice = false;
					if (!j->isProgressive) 
						memset(b,0,kMCUSize * sizeof(ushort));
					DecodeComponent(j,1,nicomp,b);
				}
			} else {
				if (!j->skipToFirstSlice) {
					b = j->blocks;
					DecodeComponent(j,1,nicomp,b);
				}
			}

			if (++xMCU == j->ScanWidth) {
				xMCU = 0;
				yMCU++;
			}
		} else {
			// interleaved scan (or grayscale)
			long decodeMCU = j->MCUCount;
			visible = (j->currentSlice >= j->firstSlice && xMCU >= j->leftMCU && xMCU < j->rightMCU);
	
			b = NULL;
			if (!useBlockBuffer && visible) {
				b = j->blocks;
				memset(b,0,blockbufsize);
			}
			
			if (j->currentSlice == j->firstSlice) 
				j->skipToFirstSlice = false;
			if (!visible) {
				if (!j->skipToFirstSlice) 
				{
					for (i = 0; i < j->CompInFrame; i++) 
					{
						cspec = &j->Comp[i];
						if (cspec->inScan) 
							DecodeComponent(j,cspec->blocksMCU,cspec,NULL);
					}
				}
			} else {
				for (i = 0; i < j->CompInFrame; i++) 
				{
					cspec = &j->Comp[i];
					if (cspec->inScan) 
					{
						if (useBlockBuffer) 
							b = cspec->blockBuffer + ((decodeMCU * cspec->blocksMCU)<<kMCUSizeShift);
						DecodeComponent(j,cspec->blocksMCU,cspec,b);
						b += cspec->blocksMCU<<kMCUSizeShift;
					}
				}
			}
		}

		// in case we went nuts in huffman decode
		if (j->data-j->dataEnd > 8) {
			j->error = kBadHuffmanErr;
			goto done;
		}	

		if (j->error) { // Died or was cancelled
			printf("Scan error at mcu %li %li,%li with %li bytes left",j->MCUCount,xMCU,yMCU,j->dataEnd-j->data);	
			goto done;		
		}
		
		// Draw the decoded MCU  if necessary
		if (scanIsNonInterleaved) {
			// when we did enough to complete a slice of MCUs, then process it
			if (!j->isProgressive) {
				// special case for multi-scan sequential (just draw first scan directly)
				for (int ci=0; ci < j->CompInScan; ci++) {
					if (j->scanComps[ci]->Ci == j->Comp[0].Ci) {
						TransformBlocks(j,j->scanComps[ci],1,j->blocks,j->blocks);
						DrawMCUPiece(j,j->blocks,xMCU == 0 ? j->ScanWidth-1 : xMCU-1,yMCU);
						j->didDraw = true;
					}
				}
			}
			
			if (++MCUsTillSlice == MCUsPerSlice || j->MCUCount == (j->ScanMCUs-1)) {			// special case for possible partial slice at end of image
				MCUsTillSlice = 0;

				if (j->currentSlice >= j->firstSlice && j->isProgressive) 
				{
					long rowX;
					long dispMCU = j->currentSlice * j->WidthMCU;
					for (rowX= 0; rowX < j->WidthMCU; rowX++, dispMCU++) 
					{
						if (rowX >= j->leftMCU && rowX < j->rightMCU) 
						{
							DoTransform(j,dispMCU,NULL);
							DrawMCU(j,j->blocks,rowX,j->currentSlice);
							j->didDraw = true;
						}
					}
				}
				completedSlice = true;
			}
		} else {
			if (visible)  
			{
				if (!j->thumbnail) 
				{ 
					if (useBlockBuffer) 
					{
						long idispMCU = j->MCUCount;
						DoTransform(j,idispMCU,NULL);
					} else
						DoTransform(j,0,j->blocks);
				}
				DrawMCU(j,j->blocks,xMCU,yMCU);
				j->didDraw = true;
			}
			if (++xMCU == j->WidthMCU) 
			{
				xMCU = 0;
				yMCU++;
				completedSlice = true;
			}
		}
		
		j->MCUCount++;
		
		if (completedSlice) 
		{
			if (j->didDraw) 
			{
				if (j->SingleRowProc != NULL)
					 j->error = (j->SingleRowProc)(j);
				j->didDraw = false;
			}
			bool 	scanCompleted = false;
 
			if (j->currentSlice == j->lastSlice)  {
				if (j->multiPassScan == 0 || j->multiPassScan > j->scansProcessed)
					j->scansProcessed++;
				scanCompleted = true;
			}
			
			if (scanCompleted) 
			{
				if (j->thumbnail || marker == kEOI)
					j->phase =  kSkippingToEnd;	
				else
					j->phase =  kSkippingScan;		// next scan 
				break;
			}
			++j->currentSlice;
		}
		if (j->error)	
		{					// Died or was cancelled
			goto done;		
		}

		if (j->MCUCount > j->ScanMCUs) 
		{
			EndJPEGImage(j);
			break;
		}
	}
	j->RstMarker = marker;
done:
	for (i=0; i < j->CompInFrame; i++) {
		cspec = &j->Comp[i];
		cspec->blockBuffer = NULL;
	}
	return j->error;
}



static long SkipScan(JPEGDecoder* j, const uchar* data, long dataLength)
{	
	uchar marker;
	const uchar *dataStart = data;
	const uchar *dataEnd = dataStart + dataLength;
	
	// only handles one scan per image for non-interleaved sequential JPEG (rare)
	// those images need to be transcoded to see all the components 
	
	if (j->thumbnail || !j->isProgressive 
		||  (j->maxScansToDisplay != 0 && j->multiPassScan > j->maxScansToDisplay)
	) 
	{			
		j->phase = kSkippingToEnd;
		return dataLength;
	}
	if (dataLength == 0)
		return 0;
		
	while (data < dataEnd) 
	{
		if (*data++ == 0xff) 
		{
			// make sure a marker doesn't get split from its 0xff header
			// or else we wont recognize it next time around
			
			if (data == dataEnd) 
			{	
				return dataLength-1;		
			}
			
			// multiple fill bytes are allowed
			
			while ((marker=*data++) == 0xff) 
			{
				if (data == dataEnd) 
				{
					return dataLength-1;		// dont split markers
				}
			}
			
							
			if (marker != 0) 
			{		// if it's real
				if (marker == kEOI) 
				{
					j->phase =  kSkippingToEnd;	
					return data-dataStart;
				} 
				else 
				{	
					// ignore restarts
					if ((marker & 0xf8) != 0xd0) 
					{
						switch (marker) 
						{
						case kDQT:
						case kDHT:
						case kDRI:
						case kSOS:
						case kEOI:
						case kCOM:
							break;
						default:
							return -1;
						}
						j->phase = kWaitingForScan;
						return (data-2) - dataStart;
					}
				}
			}
		}
	}
	return dataLength;
}


//		ScanWrite writes vlc data, stripping huffman codes as it goes
//		We can always write into the start of our buffer, since the
//		caller is taking care of how much data is being used.
static long ScanWrite(JPEGDecoder* j,const  uchar* data, long dataLength)
{
	uchar	*dst;
	const  uchar* dataStart = data;
	short	marker = 0;
	JPEGError	error;
	long	count;

//	Write data until our buffer is full, they ran out of data or we hit a marker

	count = MIN(kJPEGBufferSize - j->inBuffer,dataLength);
	if (count <= 0)
		return -1;

	dst = j->WriteBuffer + j->inBuffer;

	while (count-- > 0) {
		if ((*dst++ = *data++) == 0xFF) 
		{
			// multiple fill bytes are allowed, but dont copy them
			do {
				if (!count--) 
				{
					data--;			// Didn't get this one, get it next time
					dst--;
					j->inBuffer = dst - j->WriteBuffer;
					return data-dataStart;
				}
			} while ((marker=*data++) == 0xff);

			// it's either a byte stuffed zero or a real marker

			if (marker != 0) 
			{
				if ((marker & 0xF8) == kRST) 
				{
					dst--;		// dont include the restart marker
					break;
				} 
				else 
				{
					if (marker == kEOI) 
					{
						j->phase = kSkippingToEnd;
						dst--;
						break;
					} 
					else 
					{
						dst--;			// dont want the 0xff
						data -= 2;		// process the marker next time	
						break;
					}
				}
			}
		}
	}

	if (j->SkipToRestartMarker) 
	{
		if (marker == j->SkipToRestartMarker)
			j->SkipToRestartMarker = 0;
		return data-dataStart;
	}
	
	if (dst == j->WriteBuffer) 
	{
		j->phase = kWaitingForScan;
		ResetScan(j);
		return 0;
	} 
	else 
	{
		if (marker == 0 && (dst-j->WriteBuffer) < kMDUMin) 
		{
			j->inBuffer = dst - j->WriteBuffer;
			return data-dataStart;
		}
		
		j->data = j->WriteBuffer;	// Read from here ...
		j->dataEnd = dst;			// use to here
	
		//	Data is all set up, use it
	
		if ((error = DecodeScan(j,marker)) != kNoError)
		{
			printf("JPEG DecodeScan: Error %d", (int)error);
			ResetJPEGDecoder(j);
			return error;				// Error
		}
		
		//	if we dont need the rest of the scan (or the image)
		//	just skip over the rest of the data here

		if (j->phase == kSkippingScan)
		{
			if (marker == kEOI)
				j->phase =  kSkippingToEnd;	
			else 
			{
				ResetScan(j);
				long skipped = 0;
				long used = (data - dataStart);
				if ((dataLength-used)  > 0 && used > 0)
				{
					skipped = SkipScan(j,data,dataLength - used);
					if (skipped < 0)
						return skipped;
				}
				return used + skipped;
			}
		}
	
		if (marker == kEOI || j->phase == kSkippingToEnd) 
		{
			ResetScan(j);
			j->phase =  kSkippingToEnd;	
			return dataLength;
		}

		// Copy Unused data to buffer head
		if (marker) 
			j->inBuffer = 0;
		else {
			j->inBuffer = j->dataEnd - j->data;
			if (j->inBuffer < 0)
				return -1;

			if (j->inBuffer > kJPEGBufferSize)
				return -1;

			if (j->inBuffer)
				memcpy(j->WriteBuffer,j->data,j->inBuffer);
		}
	}
	
	return data - dataStart;	// Return how much we used
}

static long NextMarker(const uchar *data,long dataLength,uchar *rMarker,ushort *rSize)
{
	long used = 0;
	uchar marker = 0;
	bool head = false;
	ushort s;

	*rMarker = 0;
	*rSize = 0;
	if (dataLength <= 2) 
		return 0;

	while (used < dataLength) {
		if (marker == 0) {
			if (head) {
				if (*data != 0xff)
					marker = *data;
				data++;
				used++;
			} else {
				if (*data++ == 0xff) 
					head = true;

				used++;
			}
		} else {
			if (marker == kSOI || marker == kEOI) {
				*rSize = 0;
				*rMarker = marker;
				break;
			} else if ((marker & 0xf8) == kRST) {
				marker = 0;
				head = false;
				continue;
			} else {
				if (dataLength-used < 2) 
					return used-2;		// reget from start of marker

				used += 2;
				s = *data++;
				s <<= 8;
				s |= *data++;
				*rSize = s;
				*rMarker = marker;
				break;
			}
		}
	}

	return used;
}

//
//		JPEGWrite
//			Write data into the decoder
//			Returns number of bytes used or
//			0 if size is to small or
//			-ve if there has been an error
long JPEGWrite(JPEGDecoder* j, const uchar* data, long dataLength, BRect *)
{
	uchar	marker;
	ushort	s;
	JPEGError	error = kNoError;
	long	l,bytesConsumed = 0;
	long	markerOffset = 0;
	JPEGDecodePhase thePhase;
	long	result = 0;
	
	marker = 0;
	
	if  (j->phase == kSkippingToEnd) {
		ResetJPEGDecoder(j);
		j->phase = kWaitingForNewImage;
	}
	
	if (dataLength == 0)
		goto done;
	
	// we have to deal with skipping larger markers we dont care about like
	// comments and or app markers 
	

	if (j->unusedSkip) {
		if (dataLength > j->unusedSkip)
			j->unusedSkip = 0;
		else
			goto done;
	}


newPhase:

	if (j->phase == kSkippingToEnd) 
		goto completed;

	thePhase = j->phase;

	if (dataLength < 8 && bytesConsumed != 0) {
		if (bytesConsumed < 0)
			result = kGenericError;
		else
			result = bytesConsumed;

		goto done;
	}

	if (dataLength <= 0) {
		result = kGenericError;
		goto done;
	}

	switch (j->phase) {
	case kSkippingScan:
		l = SkipScan(j,data,dataLength);
		if (l < 0) {
			ResetJPEGDecoder(j);
			result = l;
			goto done;
		}

		bytesConsumed += l;
		dataLength -= l;
		data += l;
			
		if (j->phase != thePhase)	
			goto newPhase;
		if (bytesConsumed < 0)
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
		
	case kProcessingScan:
		if (j->lastSlice < j->firstSlice || j->rightMCU  <= j->leftMCU) {
			j->phase = kSkippingScan;
			goto newPhase;
		}

		l = ScanWrite(j,data,dataLength);
		if (l < 0) {
			ResetJPEGDecoder(j);
			result = l;
			goto done;
		}
		bytesConsumed += l;
		dataLength -= l;
		data += l;
		if (/*l != 0 && */ j->phase != thePhase)	
			goto newPhase;
		if (bytesConsumed < 0)
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
			
	case kSkippingToEnd:	// Image is complete, accept no more data
completed:
		ResetJPEGDecoder(j);
		j->phase = kWaitingForNewImage;
		if (j->error == 0) 
		{
			if (j->anotherPass) 
			{					// requires another display pass on full stream
				j->phase = kWaitingForNewImage;
				//result = kJPEGCompletedPass;		// magic code
				result = bytesConsumed;
				goto done;
			}
		} else
			j->anotherPass = 0;
		//result = kJPEGCompletedPass;
		result = bytesConsumed;
		goto done;
		
	case kWaitingForNewImage:
		l = NextMarker(data,dataLength,&marker,&s);
		markerOffset = l;
		bytesConsumed += l;
		if (l > dataLength) {
			result = kGenericError;
			goto done;
		}
		if (marker == 0) {
			result = bytesConsumed;
			goto done;
		}
		else if (marker != kSOI) {				// Need to start with SOI
			result = kNoSOIMarkerErr;
			goto done;
		}
		j->phase = kWaitingForFrame;
		if (bytesConsumed < 0)
			result = kGenericError;
		else
			result = bytesConsumed;
		goto done;
	
	case kWaitingForFrame:
	case kWaitingForScan:
		markerOffset = l = NextMarker(data,dataLength,&marker,&s);
		if (l > dataLength) {
			result = kGenericError;
			goto done;
		}

		bytesConsumed += l;
		data += l;
		dataLength -= l;
		if (marker == 0) {
			if (bytesConsumed < 0)
				result = kGenericError;
			else
				result = bytesConsumed;

			goto done;
		}
	
		if (marker < kSOF0 || s <= 0) {
			result = kGenericError;
			goto done;
		}
	
		if (s > (dataLength-2)) {
			j->unusedSkip = s;
	
			// go back to just before the marker
			result = bytesConsumed - markerOffset;
			goto done;
		}
		
		// Interpret the marker
		switch (marker) {
			case kEOI:		EndJPEGImage(j); break;
			case kSOF0:	  error = InterpretSOF(j,data,s);	break;	// Baseline DCT
			case kSOF0+1: error = InterpretSOF(j,data,s);	break;	// Baseline DCT
			case kSOF2:  
					j->isProgressive = true;
					error = InterpretSOF(j,data,s);
					break;	// Progressive DCT
			case kSOS:	  error = InterpretSOS(j,data,s);	break;	// Start of Scan
			case kDHT:	  error = InterpretDHT(j,data,s);	break;	// Define Huffman tables
			case kDQT:	  error = InterpretDQT(j,data,s);	break;	// Define quantization tables
			case kDRI:	  error = InterpretDRI(j,data,s);	break;	// Define restart intervals
			case kCOM:	  
					if (dataLength < (s-2)) {			// marker doesnt fit	
					//	j->unusedSkip = s-2;
						result = l;
						goto done;
					}

					error = InterpretCOM(j,data,s);	
					break;	// Comment
			default:	
					if ((marker >= kAPP) && (marker <= (kAPP + 0xF))) {	// Application specific
						if (dataLength < (s-2)) {	// marker doesnt fit	
							result = l;
							goto done;
						}

						error = InterpretAPP(j,data,s);
					} else
						error = kBadMarkerErr;

					break;
		}

		if (error != kNoError) {
			ResetJPEGDecoder(j);
			result = error;
			goto done;
		}

		bytesConsumed += (s-2);
		if (bytesConsumed < 0)
			result = kGenericError;
		else
			result = bytesConsumed;		// Not even enough data for a marker and size!

		goto done;

	default:
		printf("bad phase");
		break;
	}

	result = 0;

done:
	return result;
}
