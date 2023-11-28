/*
 * Tabstops set to 8
 */
/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*
 * This file contains routines which build tables used in block data decode
 */


#ifdef DEBUG
static char rcsid[] = "@(#) $Id: blktabs.c 1.56 1995/05/17 08:17:38 tw Exp $";
static char vcsid[] = "@(#) $Workfile:   blktabs.c  $ $Revision:   1.1  $";
#endif /* DEBUG */

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "cpk_xfrm.h"
#include "yrgb8.h"


static I32 BSQuantToScanOrder(BLOCKCONTEXT *context);			/* PROTO */
I32 QuantToScanOrder(BLOCKCONTEXT *context);			/* PROTO */
I32 BuildRVTables(RVCODETAB *);					/* PROTO */
I32 BuildPBHuffTab(HUFFTAB *);					/* PROTO */
BLOCKCONTEXT *AllocContext(void);				/* PROTO */
void FreeContext(BLOCKCONTEXT *);				/* PROTO */


/* Dequantization table configuration parameters.  */
#define MAXQCBITS	13		/* not counting sign */
#define MAXESCBITS	MAXQCBITS
#define LOG2MAXQUANT	8
#define MAXQUANT	1<<LOG2MAXQUANT
#if	(MAXQCBITS == 13) && (LOG2MAXQUANT == 9)
#define PRODUCTS 111218
#elif	(MAXQCBITS == 13) && (LOG2MAXQUANT == 8)
#define PRODUCTS 100124
#elif	(MAXQCBITS == 13) && (LOG2MAXQUANT == 7)
#define PRODUCTS 88922
#elif	(MAXQCBITS == 12) && (LOG2MAXQUANT == 9)
#define PRODUCTS 55370
#elif	(MAXQCBITS == 12) && (LOG2MAXQUANT == 8)
#define PRODUCTS 49944
#elif	(MAXQCBITS == 12) && (LOG2MAXQUANT == 7)
#define PRODUCTS 44406
#elif	(MAXQCBITS == 11) && (LOG2MAXQUANT == 9)
#define PRODUCTS 27438
#elif	(MAXQCBITS == 11) && (LOG2MAXQUANT == 8)
#define PRODUCTS 24842
#elif	(MAXQCBITS == 11) && (LOG2MAXQUANT == 7)
#define PRODUCTS 22134
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 9)
#define PRODUCTS 13500
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 8)
#define PRODUCTS 12306
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 7)
#define PRODUCTS 11010
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 6)
#define PRODUCTS 9658
#elif
#undef PRODUCTS
#error
#endif
#define MULSIZE (PRODUCTS * sizeof(I32))

/* DQTable memory-mapped file name*/
#define DQTABLENAME "IVI50_DQTable"

/* Module global data */
static I32 *multiply[MAXQUANT+1];

BLOCKCONTEXT *
AllocContext(void){

	BLOCKCONTEXT *p;
	extern void *HiveGlobalAllocPtr();

	if((p=(BLOCKCONTEXT*)HiveGlobalAllocPtr(sizeof(BLOCKCONTEXT),0))!=NULL){
		p->initialized = 0;
	}
	return p;
}

void
FreeContext(BLOCKCONTEXT *p){
	extern I32 HiveGlobalFreePtr();
	if(p)
		HiveGlobalFreePtr(p);
	return;
}



CTRANSFORM ctransforms[] = {
    { InvSlant8x8,	SmearSlant8x8,	"C Inv Slant 8x8",	TT_DUAL|TT_SAME|TT_DCPCM },
    { InvSlant1x8,	SmearSlant1x8,	"C Inv Slant 1x8",	TT_DUAL|TT_SAME },
    { InvSlant8x1,	SmearSlant8x1,	"C Inv Slant 8x1",	TT_DUAL|TT_SAME },
    { InvNada8x8,	SmearNada8x8,	"C No Xfrm 8x8",	TT_DUAL|TT_SAME },
    { InvSlant4x4,	SmearSlant4x4,	"C Inv Slant 4x4",	TT_DUAL|TT_SAME|TT_4IN8|TT_DCPCM },
    { InvSlant1x4,	SmearSlant1x4,	"C Inv Slant 1x4",	TT_DUAL|TT_SAME|TT_4IN8 },
    { InvSlant4x1,	SmearSlant4x1,	"C Inv Slant 4x1",	TT_DUAL|TT_SAME|TT_4IN8 },
    { InvNada4x4,	SmearNada4x4,	"C No Xfrm 4x4",	TT_DUAL|TT_SAME|TT_4IN8 },
    { NULL,			NULL,			"End Marker",		0 }
};	/* ctransforms */


/*
 * BuildRVTables();
 *
 * Function:
 * Build a table which maps from source symbol number to
 * run/val pair.
 *
 * We call these source symbols 'RVCODES'.  These are built
 * using an RV codebook descriptor historically called 'nabsval[]',
 * which is the number of distinct absolute values to include for
 * each successive run length.  The codebook descriptor '22, 12, 5 ...'
 * describes run-val pairs { (0,-22), (0,-21), (0,-20), ... (0,-1),
 * (0,1), ... (0,21), (0,22), (0,-12), (0,-11), ... (0,12), (0,-5) ... }
 * Which are then encoded as RVCODE's #2 - n, where N is 2 times the
 * sum of the nabsval[] entries plus 2.  The 'plus 2' is due to the
 * codebook descriptor being overloaded to express the presence of 2
 * magic entries, EOB and ESC, which are always source symbol 0 and 1.
 * Since source symbol order is not statistically useful, we reorder
 * each of the tables according to frequency.
 *
 * The vals which we look up in the table are encoded with the
 * 'tounsigned()' mapping as used in escaped values.
 *
 * Input: nabsval, freq; nabsval is 0 terminated.
 *
 * Output: runtab, valtab
 *
 * Returns:	-1 error	- more than 256 codes
 * 		 0 no error	- not more than 256 codes
 *
 */
I32
BuildRVTables(RVCODETAB *codetab){

	I32 symbol, *valtab, val, numrvcodes;
	U8 *runtab, run;

	FREQUENCY_TYPE freq[256];	/* only needed temporarily */
	NABSVAL_TYPE *nabsval, *foo;

	int i;
	FREQUENCY_TYPE *ifreq;

	nabsval = codetab->nabsval;

	foo = nabsval;		/* compute number of valid frequency entries */
	foo++;
	numrvcodes = 0;
	while(*foo)
	  numrvcodes += *foo++;
	numrvcodes *= 2;	/* positive and negative */
	numrvcodes += 2;	/* EOB and ESC */

	runtab = codetab->runtab;
	valtab = codetab->valtab;

	ifreq = codetab->invfreq;
	for(i = 0; i < numrvcodes; i++)
		freq[ifreq[i]] = i;

	run = 0;

	nabsval += 1;		/* skip bogus entry */
	symbol = 2;
	runtab[freq[0]] = 0;	/* EOB */
	valtab[freq[0]] = 0;
	runtab[freq[1]] = 0;	/* ESC */
	valtab[freq[1]] = 0;
	codetab->eob = (U8)freq[0];
	codetab->esc = (U8)freq[1];


	while(*nabsval){

		run++;

	/*	Negative Values */
		for(val = *nabsval; val > 0; val--){
			runtab[freq[symbol]] = run;
			valtab[freq[symbol]] = val<<1;
			symbol++;
		}
	/*	Positive Values */
		for(val = 1; val <= *nabsval; val++){
			runtab[freq[symbol]] = run;
			valtab[freq[symbol]] = (val<<1)-1;
			symbol++;
		}

		nabsval++;
	}

	if(symbol > 256)
		return -1;

	return 0;
}

/*
 *
 * BuildPBHuffTab():
 *
 * Function:
 * Build an old or modified PB style huffman code decode
 * table from a PB style table description.  The 'modified'
 * table has a last group or row with a prefix which is the
 * same length as that of the prior group, but which has a
 * one bit instead of a 0 bit as the last bit prior to the
 * 'variable' bits.
 *
 * Approach:
 * for each entry in varbits
 *   build prefix, modified if *(varbits+1) is 0...
 *   for each code in group
 *     build this code
 *     for each of 2^extra bits iterations
 *       read table[code] to preload non write allocate cache
 *       store decode value at table[code]
 *       code += 2^this code length
 *     end for
 *   end for
 * end for
 *
 * Implementation notes:
 * The dummy read for allocation is probably the optimal strategy for
 * tables which are larger than L1.  If the table is less than or equal
 * to L1 size, a preread of the table is optimal.  The C compiler will
 * optimize away the dummy read.  One can add the modifier 'volatile',
 * but that is overkill.  Perhaps or'ing from the table into 'dummy',
 * and storing the end result outside the loop to a volatile?
 *
 * Since a 0 terminates the row descriptions, each row description is
 * one greater than expected: a row or group with one code is specified
 * as 1, one with 4 as 3, one with 8 as 4, etc.
 *
 * The bit reversal tables allow lookup of the bit reversal of a number
 * within a given field width.  These are used to quickly generate the
 * 'suffix bit pattern'.  All other fields are more quickly generated
 * through computation.
 * The product can generate these on the fly, reducing code image size.
 * The first table is '0'.  Each successive table is two halves.  The first
 * half is the double of the prior table's entries, the second half is the
 * double + 1 of the prior table's entries (the new reset or set high order
 * bit in the low order bit position).
 *
 * More than 256 codes, while not yet useful, are allowed.  Codes will wrap,
 * since the table contains U8 values.
 *
 * Input: varbits
 *
 *	int * - pointer to array, each element of which specifies the number
 *	of suffix bits in a group of codes plus one.  An element with the
 *	value 0 terminates the input.
 *
 * Output: decodetable, numbits
 *
 *	unsigned char * - pointer to array of at least 2^maxcodelen size to
 *	store the output decoding table.
 *
 *	unsigned char * - pointer to array of at least number of source
 *	symbols size, each element of which will hold the number of bits
 *	in the corresponding symbols' huffman code.
 *
 *	If no error is detected, initializes the decode table such that
 *	*(decodetable + i) contains the source symbol encoded in the first
 *	codeword found in 'i'.
 *	If no error is detected, initializes the numbits table with huffman
 *	code bit counts.
 *
 * Returns: 	-1 error	- >= 16 bit codes, > 8 'suffix' bits
 *		 0 no error
 *
 */
#ifndef QT_FOR_MAC
#pragma data_seg(".sdata")	/* following is data shared among instances */
#endif
static U8 bitrev0[] = { 0 };
static U8 bitrev1[] = { 0, 1 };
static U8 bitrev2[] = {0, 2, 1, 3 };
static U8 bitrev3[] = {0,  4,  2,  6, 1,  5,  3,  7 };
static U8 bitrev4[] = {0,  8,  4, 12, 2, 10,  6, 14, 1,  9,  5, 13, 3, 11,  7, 15 };

static U8 bitrev5[] = {0, 16,  8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
		 1, 17,  9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31 };

static U8 bitrev6[] = {0, 32, 16, 48,  8, 40, 24, 56,  4, 36, 20, 52, 12, 44, 28, 60,
		 2, 34, 18, 50, 10, 42, 26, 58,  6, 38, 22, 54, 14, 46, 30, 62,

		 1, 33, 17, 49,  9, 41, 25, 57,  5, 37, 21, 53, 13, 45, 29, 61,
		 3, 35, 19, 51, 11, 43, 27, 59,  7, 39, 23, 55, 15, 47, 31, 63};

static U8 bitrev7[] = {0, 64, 32, 96, 16, 80, 48,112,  8, 72, 40,104, 24, 88, 56,120,
		 4, 68, 36,100, 20, 84, 52,116, 12, 76, 44,108, 28, 92, 60,124,
		 2, 66, 34, 98, 18, 82, 50,114, 10, 74, 42,106, 26, 90, 58,122,
		 6, 70, 38,102, 22, 86, 54,118, 14, 78, 46,110, 30, 94, 62,126,

		 1, 65, 33, 97, 17, 81, 49,113,  9, 73, 41,105, 25, 89, 57,121,
		 5, 69, 37,101, 21, 85, 53,117, 13, 77, 45,109, 29, 93, 61,125,
		 3, 67, 35, 99, 19, 83, 51,115, 11, 75, 43,107, 27, 91, 59,123,
		 7, 71, 39, 103,23, 87, 55,119, 15, 79, 47,111, 31, 95, 63,127};

static U8 bitrev8[] = {0,128, 64,192, 32,160, 96,224, 16,144, 80,208, 48,176,112,240,
		 8,136, 72,200, 40,168,104,232, 24,152, 88,216, 56,184,120,248,
		 4,132, 68,196, 36,164,100,228, 20,148, 84,212, 52,180,116,244,
		12,140, 76,204, 44,172,108,236, 28,156, 92,220, 60,188,124,252,
		 2,130, 66,194, 34,162, 98,226, 18,146, 82,210, 50,178,114,242,
		10,138, 74,202, 42,170,106,234, 26,154, 90,218, 58,186,122,250,
		 6,134, 70,198, 38,166,102,230, 22,150, 86,214, 54,182,118,246,
		14,142, 78,206, 46,174,110,238, 30,158, 94,222, 62,190,126,254,

		 1,129, 65,193, 33,161, 97,225, 17,145, 81,209, 49,177,113,241,
		 9,137, 73,201, 41,169,105,233, 25,153, 89,217, 57,185,121,249,
		 5,133, 69,197, 37,165,101,229, 21,149, 85,213, 53,181,117,245,
		13,141, 77,205, 45,173,109,237, 29,157, 93,221, 61,189,125,253,
		 3,131, 67,195, 35,163, 99,227, 19,147, 83,211, 51,179,115,243,
		11,139, 75,203, 43,171,107,235, 27,155, 91,219, 59,187,123,251,
		 7,135, 71,199, 39,167,103,231, 23,151, 87,215, 55,183,119,247,
		15,143, 79,207, 47,175,111,239, 31,159, 95,223, 63,191,127,255};

static I32 negpowersoftwo[] = {-1, -2, -4, -8, -16, -32, -64, -128, -256,
			-512, -1024, -2048, -4096, -8192, -16384,
			-32768, -65536, -131072, -262144, -524288 };
#ifndef QT_FOR_MAC
#pragma data_seg()	/* end shared data */
#endif

static U8 *bitrevs[] = { bitrev0, bitrev1, bitrev2,
		   bitrev3, bitrev4, bitrev5,
		   bitrev6, bitrev7, bitrev8 };


I32
PrivateBuildPBHuffTab(HUFFTAB *hufftab){



  I32 codesingroup, prefix, prefixlength;
  I32 randominit, sourcesymbol, sourcesymbolbase;
  I32 currbits, minbits, maxbits, countp;
  U8 *decodetable, *numbits, *b;

  PBDESCRIPTOR_TYPE *p;

  I32 suffixbitpattern, randombyone, randomcounter, thiscode;
  /* volatile */ unsigned char dummy; /* problematic */

  p = hufftab->descriptor;
  numbits = hufftab->numbits;
  decodetable = hufftab->maintable;

  maxbits = 0;
  prefixlength = 1;
  sourcesymbol = 0;
  countp = *p++;
  minbits = prefixlength + *p;

  while(countp--){

    if(!countp) prefixlength--;

    if(*p >= (sizeof(bitrevs)/sizeof(bitrevs[0])))	/* no bitrev table? */
      return -1;

    currbits = prefixlength + *p;

    if(currbits > maxbits)
      maxbits = currbits;

    if(currbits < minbits)
      minbits = currbits;

    codesingroup = 1<<*p;

    while(codesingroup--){
      numbits[sourcesymbol] = (unsigned char) currbits;
      if(sourcesymbol < 255)
    	sourcesymbol++;
    }

    p++;					/* next */

    prefixlength++;				/* prefix bit increase */

  }
  hufftab->minbits = (U8) minbits;
  hufftab->maxbits = (U8) maxbits;
  hufftab->maxbitsmask = (U32) ((1<<maxbits)-1 - (maxbits == 32) );

  /* the following is a 'soft' restriction, and can be eased by using the
   * 'trimmed' secondary lookup table, which is smaller by 2^k than the
   * full decoder table, where 'k' is the number of shared leading bits for
   * the codes decoded using that table. i.e: A primary table which fully
   * decodes all N bit and less codes (let N bit codes have prefix length L)
   * incidentally decodes the first L bits of all codes longer than N bits.
   * e.g.: an IRV like codebook, but expanded to 9 groups as in
   *   2,3,4,5,6,6,6,7,8 - max code length 17 bits, 636 codewords.
   *
   *   after finding no match in the primary size 2^MH_BITS table, we have
   *   learned that all the codes left to check have the first 4 bits == 1,
   *   and that if we trim these bits off, the remaining bits of the code
   *   still have group separation information: 0/10/110/1110/11110/, and
   *   the final decoding of them can be done with an access to a table of
   *   2^13 entries.  That makes about 6% of the codes decoded via the
   *   secondary table, with a table which while not in L1, is only just
   *   the size of L1.  This can be extended if performance demands, with
   *   a table which (further) partially decodes 13-17 bit codes and fully
   *   decodes 11 and 12 bit codes, or the primary table could be changed to
   *   encompass all codes up to and including 11 bits, although that is not
   *   a good idea for the parallel decoder, due to the size of the table
   *   elements, unless the average code size is 5.5 bits rather than 5...
   */
  if(maxbits > H_MAXCODELEN){
	return -1;
  }

  sourcesymbolbase = 0;

  p = hufftab->descriptor;
  prefixlength = 1;

  countp = *p++;

  while(countp--){

    prefix = (1 << (prefixlength - 1)) - 1;	/* leading 1's */

    if(!countp) prefixlength--;		/* modifed last prefix */

    codesingroup = 1 << *p;

    sourcesymbol = 0;

    b = bitrevs[*p];

    randominit = 1 << maxbits;			/* top plus one */

    while(codesingroup){			/* for each code */

      if((sourcesymbol + sourcesymbolbase) > 255)
        break;

      suffixbitpattern = *(b+sourcesymbol) << prefixlength;

      thiscode = prefix | suffixbitpattern;	/* code */

      randomcounter = randominit;		/* top plus one */
      randombyone = 1 << (*p + prefixlength);

      while(randomcounter){			/* for each don't care */
        randomcounter -= randombyone;		/* this don't care */
        dummy = decodetable[thiscode|randomcounter]; /* preload */
        decodetable[thiscode|randomcounter] = (unsigned char)(sourcesymbol+sourcesymbolbase); /* this symbol */
      }
      codesingroup--;				/* done? */
      sourcesymbol++;				/* next code in group */
    }

    sourcesymbolbase += 1<<*p;			/* next group base */
    p++;					/* next suffix length */
    prefixlength++;				/* next prefix length */
  }

  return 0;
}

/* Assuming the external client is the first caller each frame... */
I32
BuildPBHuffTab(HUFFTAB *hufftab){

  I32 retval;

  retval = PrivateBuildPBHuffTab(hufftab);
  return retval;
}

