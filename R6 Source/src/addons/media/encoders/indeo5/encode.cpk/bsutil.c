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
 * bsutil.c
 *
 * Description: 
 *		These routines support big and little endian processors
 *		without any conditional compilation (explicit or implicit).
 *		The routines in this file are for the management of the 
 *		bitstream -- reading from it and writing to it.
 * 		Bit Buffers can be written to or read from, but not both at the
 * 		same time.  A "flush" must be performed after writing in order
 * 		to be able to read (equivalent to a rewind).
 *
 * Functions:
 *		BitBuffAlloc		Allocates space for bit buffer structure
 *		BitBuffInit			Initializes a new bit buffer struct
 *		BitBuffFree			Free the bit buffer and all nodes 
 *		BitBuffWrite		Append bits to the bit buffer
 *		NpMBitBuffWrite		Append N [+M] bits to the bit buffer
 *		BitBuffOverWrite	Write to the buffer without checking positioning
 *		BitBuffFlush		"Rewind" after writes to allow reads
 *		BitBuffRead			Reads bits from the bit buffer
 *		BitBuffByteRead		Read a full byte
 *		BitBuff2Mem			Store entire buffer (and nodes) into memory
 *		BitBuffByteAlign	Guarantees next write will be on a byte boundary
 *		BitBuffSkip2ByteAlign Same for a read
 *		BitBuffSize			Returns the size of the bit buffer, in bytes
 *		BitBuffHist			Calculate the histogram of the bit buffer
 *		SkipPad2DWord		Pad to the next DWord
 *		HuffRead			Read one Huffman symbol from bitstream
 *		HuffEncBitBuff		Huffman encode a bitstream
 *		tosigned
 *		tounsigned
 */

#include <string.h>		/* for memcpy */
#include <setjmp.h>

#include "datatype.h"
#include "tksys.h"

#include "hufftbls.h"
#include "qnttbls.h"
#include "mc.h"

#include "ivi5bs.h"
#include "mbhuftbl.h"
#include "bkhuftbl.h"
#ifdef SIMULATOR
#include "encsmdef.h"
#endif /* SIMULATOR */
#include "bsutil.h"
#include "errhand.h"
#include "const.h"


/*
 * Local forward references
 */

static pBitBuffNodeSt NewBuffNode(I32 iInitBuffSize, jmp_buf jbEnv);

static const U8  MaxBits = 32;	/* Upper limit on bits per I/O call */
static const I32 MAX_BITS_IN_HUFF_CODE = 32;

/*
 * Create a new root node.  Take a pointer to an unintializied 
 * BitBuffSt, initialize it, and allocate memory for its root node.
 */
void BitBuffAlloc(pBitBuffSt pBitBuff, I32 iInitBuffSize, 
						jmp_buf jbEnv)
{
	pBitBuff->Write = TRUE;
	pBitBuff->BuffSize = iInitBuffSize;
	pBitBuff->BitsInByte = 0;
	pBitBuff->TotalBitsWritten = 0;
	pBitBuff->TotalBitsRead = 0;
	pBitBuff->RootNode = NewBuffNode(pBitBuff->BuffSize, jbEnv);
	pBitBuff->CurrentNode = pBitBuff->RootNode;
	pBitBuff->CurrentByte = pBitBuff->RootNode->Buff;

	return;
}

/*
 * Initialize a bit buffer structure:  reset pointers and set it up
 * for writing.
 */
void BitBuffInit(pBitBuffSt pBitBuff,jmp_buf jbEnv)
{
	pBitBuffNodeSt pTmpNode;

	if (pBitBuff != NULL) {
		pBitBuff->Write = TRUE;
		pBitBuff->BitsInByte = 0;
		pBitBuff->TotalBitsWritten = 0;
		pBitBuff->TotalBitsRead = 0;
		pTmpNode = pBitBuff->RootNode;

		while (pTmpNode != NULL) {
			if (pTmpNode->Buff != NULL) {
				pTmpNode->BitsInBuff = 0;
				pTmpNode = pTmpNode->NextNode;
			} else {
				longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_NULL_PARM << TYPE_OFFSET));
			}
		}

		pBitBuff->CurrentNode = pBitBuff->RootNode;
		if (pBitBuff->RootNode != NULL) {
			pBitBuff->CurrentByte = pBitBuff->RootNode->Buff;
		}

	} else {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	}

	return;
}

/*
 * Allocate and return a bit buffer node.  In order to reduce the 
 * number of mallocs, perform just one for both the buffer and the 
 * node it points to.
 */
static
pBitBuffNodeSt NewBuffNode(I32 iInitBuffSize, jmp_buf jbEnv)
{
	pBitBuffNodeSt pTmpNode = 
		(pBitBuffNodeSt) SysMalloc(sizeof(BitBuffNodeSt)+iInitBuffSize, jbEnv);

	pTmpNode->BitsInBuff = 0;
	pTmpNode->Buff = (PU8) pTmpNode + sizeof(BitBuffNodeSt);
	pTmpNode->NextNode = NULL;

	return pTmpNode;
}

/* Free up the bit buffer's allocated buffers */
void BitBuffFree(pBitBuffSt pBitBuff, jmp_buf jbEnv)
{
	pBitBuffNodeSt pCurrNode, pNextNode;

	if (pBitBuff != NULL) {				/* Sanity check */
		pCurrNode = pBitBuff->RootNode;
	} else {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	}

	while(pCurrNode != NULL) {	 /* Loop through the nodes, freeing as we go */
		pNextNode = pCurrNode->NextNode;
		SysFree((PU8) pCurrNode, jbEnv);
		pCurrNode = pNextNode;
	}

	return;
}

/*
 * Appends Count bits of Value to the bitbuffer.  More nodes are allocated
 * if required.  The internal counters are advanced in the bit buffer.
 */
void BitBuffWrite(
	pBitBuffSt 	pBitBuff,	/* buffer to append to */
	U32 		uValue,		/* contains the bit pattern to append */
	U8 			u8Count,	/* contains the number of bits in the pattern */
	jmp_buf 	jbEnv)
{
	I32 iMask = 1;		/* write low bit first */
	I32 iBit;

	/*
	 * Sanity checks
	 */
	if (!pBitBuff->Write) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BITSTREAM << TYPE_OFFSET));
	}
	if ((u8Count > MaxBits) ||
		((u8Count != MaxBits) && (uValue >= (U32) (1 << u8Count))) ) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}

	while (u8Count-- > 0) {
		iBit = (uValue & iMask) ? 1 : 0;
		if (pBitBuff->BitsInByte == 8) {	/* Is the byte full? */
			pBitBuff->TotalBitsWritten += 8;

			/*
			 * If the buffer is full then allocate a new one if we have to.
			 * Otherwise, use the existing node that's there.
			 */
			if(!((pBitBuff->TotalBitsWritten / 8) % pBitBuff->BuffSize)) {
				if (pBitBuff->CurrentNode->NextNode == NULL) {
					pBitBuff->CurrentNode->NextNode = 
									NewBuffNode(pBitBuff->BuffSize, jbEnv);
					pBitBuff->CurrentNode->NextNode->BitsInBuff = 0;
				}
				pBitBuff->CurrentNode = pBitBuff->CurrentNode->NextNode;
				pBitBuff->CurrentByte = pBitBuff->CurrentNode->Buff;
			} else {
				pBitBuff->CurrentByte++;	/* Advance to the next byte */
			}

			pBitBuff->BitsInByte = 0;	/* Reset bits in byte */
		}

		/*
		 * Be sure to clear the bit before setting it (in case it's zero)
		 */
		*pBitBuff->CurrentByte = (U8) ((*pBitBuff->CurrentByte &
									    ~(1 << pBitBuff->BitsInByte)) |
									    (iBit << pBitBuff->BitsInByte));
		pBitBuff->BitsInByte += 1;
		pBitBuff->CurrentNode->BitsInBuff += 1;
		iMask <<= 1;		/* Move mask to next bit */
	}
}


/*
 * Write out n+m bits to the bit buffer.
 *
 * The "n" part is always written.
 * The "m" part is optional, and is only written if it's length > 0
 */
void NpMBitBuffWrite(
	pBitBuffSt 	pBuff,		/* The bit buffer to write to */
	U32 		uFixedBits, /* Always write this value */
	U8 			u8FixedLen, /* Length of always-written value */
	U32 		uVarBits,   /* Possible write this value */
	U8 			u8VarLen,   /* Length of optional value; 0 -> no write */
	jmp_buf 	jbEnv)
{
        BitBuffWrite(pBuff, uFixedBits, u8FixedLen, jbEnv);
        if (u8VarLen > 0) {
                BitBuffWrite(pBuff, uVarBits, u8VarLen, jbEnv);
        }
        return;
}


/*
 * Overwrite bits to a bit buffer. 
 * This assumes that the bit buffer is positioned at the proper place
 * (from a prior read).
 */
void BitBuffOverWrite(
	pBitBuffSt  pBitBuff,	/* buffer to append to */
	U32 		uValue,		/* contains the bit pattern to append */
	U8  		u8Count,	/* contains the number of bits in the pattern */
	jmp_buf 	jbEnv)
{
	U32 uMask = 1;
	U32 uBit;

	if ((pBitBuff->Write) ||  		/* Sanity checks */
		(u8Count > MaxBits) ||
		((u8Count != MaxBits) && (uValue >= (U32) (1 << u8Count))) ) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}

	/*
	 * The semantics of BitsInByte of a "read" buffer are different than
	 * for a write buffer.   Adjust the value so it works for write.
	 */
	pBitBuff->BitsInByte = 8 - pBitBuff->BitsInByte;

	while (u8Count-- > 0) {
		uBit = (uValue & uMask) ? 1 : 0;
		if (pBitBuff->BitsInByte == 8) {   /* End of this byte? */
			if(	pBitBuff->CurrentByte + 1 >		/* Buffer full? */ 
				pBitBuff->CurrentNode->Buff + pBitBuff->BuffSize) {
				if (pBitBuff->CurrentNode->NextNode == NULL) {
					longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_NULL_PARM << TYPE_OFFSET));
				}
				pBitBuff->CurrentNode = pBitBuff->CurrentNode->NextNode;
				pBitBuff->CurrentByte = pBitBuff->CurrentNode->Buff;
			} else {
				pBitBuff->CurrentByte++;	/* Advance to the next byte */
			}

			pBitBuff->BitsInByte = 0;	/* Reset bits in byte */
		}

		/*
		 * Be sure to clear the bit before setting it (in case it's zero)
		 */
		*pBitBuff->CurrentByte = (U8) ((*pBitBuff->CurrentByte &
										~(1 << pBitBuff->BitsInByte)) |
										(uBit << pBitBuff->BitsInByte));
		pBitBuff->BitsInByte += 1;
		uMask <<= 1;		/* Move mask to next bit */
	}

	/*
	 * Restore the semantics of BitsInByte to work for a read buffer again
	 */
	pBitBuff->BitsInByte = 8 - pBitBuff->BitsInByte;
	return;
}

/* 	This function resets the pointer to the beginning of the buffers
 *	and prepares the structure for reading the buffers.
 */

void BitBuffFlush(pBitBuffSt pBitBuff)
{
	/*
	 * Don't add to the bit count if you don't have to.  This
	 * allows flush to be called more than once without undesired
	 * side effects.
	 */
	if (pBitBuff->Write == TRUE) {
		pBitBuff->TotalBitsWritten += pBitBuff->BitsInByte;
	}
	pBitBuff->Write = FALSE;
	pBitBuff->CurrentNode = pBitBuff->RootNode;
	pBitBuff->CurrentByte = pBitBuff->CurrentNode->Buff;
	pBitBuff->BitsInByte = 8;
	pBitBuff->TotalBitsRead = 0;
}

/*
 * Read Count bits out of the bit buffer.
 * The internal counters are advanced in the bit buffer.
 * Fatal error if trying to read more bits than remain in the buffer.
 */
U32 BitBuffRead(pBitBuffSt pBitBuff, U8 u8Count, jmp_buf jbEnv)
{
	U32 uVal = 0;
	I32 iShift;
	U32 uBit, uBitNo=0;
	I32 iTotalBits2Read = pBitBuff->TotalBitsWritten;
	
	if(pBitBuff->Write) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}
	while (u8Count-- > 0) {
		if (pBitBuff->BitsInByte == 0) {	/* byte is fully read */
			/* If the buffer is read then go to the next one in the list */
			if(!((pBitBuff->TotalBitsRead / 8) % pBitBuff->BuffSize)) {
				pBitBuff->CurrentNode = pBitBuff->CurrentNode->NextNode;
				if(pBitBuff->CurrentNode == NULL) {
					longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_NULL_PARM << TYPE_OFFSET));
 				}
				pBitBuff->CurrentByte = pBitBuff->CurrentNode->Buff;
			}
			else {
				pBitBuff->CurrentByte++;
			}
			pBitBuff->BitsInByte = 8;	/* now full */
		}
 		if(++pBitBuff->TotalBitsRead > pBitBuff->TotalBitsWritten){
			longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));
		}
		iShift = 8 - pBitBuff->BitsInByte;	/* start with LSB */
		uBit = (*pBitBuff->CurrentByte & (1 << iShift) )  ?  1 : 0;
		uVal |= uBit << uBitNo++;
		pBitBuff->BitsInByte--;
	}
	return uVal;
}


/*
 * Read a full byte from the bit buffer as a byte (versus 1 bit at a time).
 * The internal counters are advanced in the bit buffer.
 * Fatal error if trying to read more bits than remain in the buffer.
 */
U8 BitBuffByteRead(pBitBuffSt pBitBuff, jmp_buf jbEnv)
{
	U8 u8Val;
	
	if(pBitBuff->Write) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}
	if(pBitBuff->BitsInByte != 8 && pBitBuff->BitsInByte != 0) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}

	if (pBitBuff->BitsInByte == 0) {	/* is byte fully read?  */
		if(!((pBitBuff->TotalBitsRead / 8) % pBitBuff->BuffSize)) {
			pBitBuff->CurrentNode = pBitBuff->CurrentNode->NextNode;
			if(pBitBuff->CurrentNode == NULL) {
				longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_NULL_PARM << TYPE_OFFSET));
			}
			pBitBuff->CurrentByte = pBitBuff->CurrentNode->Buff;
		} else {
			pBitBuff->CurrentByte++;
		}
	}

	u8Val = *pBitBuff->CurrentByte;
	pBitBuff->TotalBitsRead += 8;
	pBitBuff->BitsInByte = 0;

	if(pBitBuff->TotalBitsRead > pBitBuff->TotalBitsWritten){
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}
	return u8Val;
}

/*
 * Unravel a bit buffer and copy it into memory
 * 
 * Assumes the destination is large enough to accommodate the bit buffer
 * Also assumes that the source bit buffer has been byte-aligned.
 * Return the number of bytes copied to memory
 */
U32 BitBuff2Mem(PU8 pu8MemTo, pBitBuffSt pBuffFrom, jmp_buf jbEnv)
{
	pBitBuffNodeSt pCurrNode;
	size_t to_write;
	U32 uWritten = 0;

	if (pBuffFrom == NULL) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	}
	if (pu8MemTo == NULL) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_NULL_PARM << TYPE_OFFSET));
	}

	pCurrNode = pBuffFrom->RootNode;

	while (pCurrNode != NULL) {
   		to_write = pCurrNode->BitsInBuff / 8;
		memcpy(pu8MemTo, pCurrNode->Buff, to_write);
		pu8MemTo += to_write;
		uWritten += to_write;
		pCurrNode = pCurrNode->NextNode;
	}

	if (uWritten != (U32) (pBuffFrom->TotalBitsWritten / 8)) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	}

	return(uWritten);
}

/*
 * Advances the internal pointers of a "write to" bit biffer so that an
 * integral number of bytes is used.  The next write to the bit buffer
 * will be on a byte boundary.
 */
void BitBuffByteAlign(pBitBuffSt pBitBuff, jmp_buf jbEnv)
{
	/* A read buff's BitsInByte of 0 corresponds to a Write Buff's 
	 * BitsInByte of 8.  So if BitsInByte is non-zero then need to
	 * byte align 
	 */
	if (pBitBuff->BitsInByte)	
		BitBuffWrite(pBitBuff, 0, (U8) (8 - pBitBuff->BitsInByte), jbEnv);
}

/*
 * Advances the internal pointers of a "read from" bit biffer so that an
 * integral number of bytes is used.  The next read from the bit buffer
 * will be from a byte boundary.
 */
void BitBuffSkip2ByteAlign(pBitBuffSt pBitBuff, jmp_buf jbEnv) {

	U8 u8Count = pBitBuff->BitsInByte;
	I32 iVal = 0;

	if(u8Count != 8 && u8Count != 0) {
		iVal = BitBuffRead(pBitBuff, u8Count, jbEnv);
	}
	if(iVal != 0) {
		longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BITSTREAM << TYPE_OFFSET));
	}
}


/*
 * Traverse the bit buffer list and add up all of the bytes
 */
U32 BitBuffSize(pBitBuffSt pBuf)
{
	U32				uLength;	/* accumulate number of bytes */
	pBitBuffNodeSt	p;			/* used to traverse through the list */

	uLength = 0;
	if (pBuf != NULL) {
		for (p = pBuf->RootNode; p != NULL; p = p->NextNode) {
			uLength +=  (p->BitsInBuff / 8) + 
						((p->BitsInBuff % 8 == 0) ? 0 : 1);
		}
	}

	return(uLength);
}

/*
 * Calculate the histogram of the values to be Huffman coded in a buffer
 * containing a mixture of both fixed length and values to be Huffman coded.
 */
void BitBuffHist(pBitBuffSt pPreHuffBuff, PI32 piHist, jmp_buf jbEnv)
{
	U32 uVal;

	BitBuffFlush(pPreHuffBuff);  /* Make sure the buffer is ready for reading */
	while(!EOBUFF(pPreHuffBuff)){
		uVal = BitBuffRead(pPreHuffBuff, IndicatorLen, jbEnv);
		if(uVal == FixedLenIndic) {
			/* Read in a fixed length value */
			uVal = BitBuffRead(pPreHuffBuff,FixLenLen, jbEnv); 
			uVal = BitBuffRead(pPreHuffBuff, (U8) uVal, jbEnv);	/* Skip this */
		}
		else if(uVal == HuffCodeIndic){
			/* Read in a value which will be Huffman coded */
			uVal = BitBuffRead(pPreHuffBuff, HuffLen, jbEnv);
			piHist[uVal]++;
		}
	}

	BitBuffFlush(pPreHuffBuff); /* Return the buffer readable */
	return;
}


/* Converts x from signed to unsigned with the mapping */
/*	of 0, 1, -1, 2, -2, 3, -3, 4 ...	*/

I32 tounsigned(I32 x)
{
	return (x > 0) ? 2*x-1 : 2*-x;
}

/* convert x from unsigned to signed, with the range:
 * (0, -1, 1, -2, 2, ...).
 */
I32 tosigned(I32 x)
{
	
	I32 r;
	
	r = (x + 1) >> 1;
	if (((x + 1) & 1) == 0)
		return(r);
	else
		return(-r);
}

/* Input is two bitbuffers.  One is the uncompressed buffer as a sequence
   of bytes and the other is the bitbuffer to which the bytes will be placed */
void HuffEncBitBuff(pBitBuffSt pBitBuff, pBitBuffSt pPreHuffBuff,
		PHuffTblSt pHuffTbl, pRVMapTblSt pRVMapTbl, jmp_buf jbEnv)
{
	HuffSymbolSt HuffSymbol;
	PHuffSymbolSt pHuffSym;
	U8 u8Len;
	U32 uVal;
	I32 iSinceLastEsc = 10; /* Just must be initialized to > 4 */

	BitBuffFlush(pPreHuffBuff);  /* Make sure the buffer is ready for reading */

	pHuffSym = pHuffTbl->HuffSym;	

	/* For each value in the buffer read it in, determine if it is a fixed
	   length value or one that needs a Huffman code, and write out the
	   corresponding value to the bit buffer.
	 */
	while(!EOBUFF(pPreHuffBuff)){

		uVal = BitBuffRead(pPreHuffBuff, IndicatorLen, jbEnv);
		if(uVal == FixedLenIndic) {
			/* Fixed Length -- no huffman coding so get the length, the value,
			 * and write it back out 
			 */
			u8Len = (U8) BitBuffRead(pPreHuffBuff,FixLenLen, jbEnv); 
			uVal = BitBuffRead(pPreHuffBuff, (U8) u8Len, jbEnv);	 
			BitBuffWrite(pBitBuff, uVal, u8Len, jbEnv);
		}
		else if(uVal == HuffCodeIndic){
			/* Read in a value which will be Huffman coded */
			uVal = BitBuffRead(pPreHuffBuff, HuffLen, jbEnv);

			if(pRVMapTbl != NULL) {
				/* RV codes are found before swapping is determined.  The code
				   below swaps the value if it was determined that it should be
				   swapped.  This works because the FreqOrdTbl is not swapped.  
				   So  the first statement gives back the original value and the
				   second statement gets the new RV code for that value.  Escaped
				   values are not swapped hence the the logic seen is only if you
				   are not processing an escape.
				 */
				if(iSinceLastEsc >= 4) {
					/* Restore the original value from the unswapped FreqOrdTbl. */
					uVal = pRVMapTbl->FreqOrdTbl[uVal];

					/* Find the new RV code with the swapped table. */
					uVal = pRVMapTbl->InvFreqOrdTbl[uVal];

					if(uVal == pRVMapTbl->InvFreqOrdTbl[ESC]) {
						/* Escape value */
						iSinceLastEsc = 0;
					}
				}
			}
			/* Make sure there is a defined huffman code for this value in the
			   Huffman code table.
			*/
			if (uVal >= (U32) pHuffTbl->NumSymbols) 
				longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BITSTREAM << TYPE_OFFSET));
			HuffSymbol = pHuffSym[uVal];
			BitBuffWrite(pBitBuff, HuffSymbol.Symbol, HuffSymbol.Len, jbEnv);
			iSinceLastEsc++;
		}
		else{
			/* Error condition */
			longjmp(jbEnv,  (BSUTIL << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BITSTREAM << TYPE_OFFSET));
		}
	}
}

/* Pads to the next DWord and then adds an extra DWord at the end */
void SkipPad2DWord(pBitBuffSt pBitBuff, jmp_buf jbEnv)
{
	U8 u8Bits2Read;

	u8Bits2Read = (U8) ((DWordSize * 8) - 
						(pBitBuff->TotalBitsRead % (DWordSize * 8)));
	BitBuffRead(pBitBuff, u8Bits2Read, jbEnv);
	if(u8Bits2Read <= DWordSize * 8) {
		BitBuffRead(pBitBuff, (U8) (DWordSize * 8), jbEnv);
	}
}

/* Huffman decoder routine.  Reads one symbol from the bitstream.
 * New faster method: Read MAX_BITS_IN_HUFF_CODE bits, then use this as an
 * index into two tables: the first table returns the first Huffman code
 * read, and the second table returns the number of bits
 * actually used. Then reset the input ptr by the number of bits read 
 * but not used for the current Huffman code.
 */
U32 HuffRead(pBitBuffSt pBitBuff, 
			 PNewDecHuffTblSt pNewDecHuffTbl, 
			 jmp_buf jbEnv)
{
	PU32 puTmp;
	U32 uIndex, Dummy, symbol;
	U8  ReadBits;

	/* Read a DWORD from bitstream */
	/* Use as an index into table */
	/* Reset bitstream by number of bits read */

	puTmp = (PU32) pBitBuff->CurrentByte;
	uIndex = *puTmp;
	uIndex = (uIndex >> (8 - pBitBuff->BitsInByte)) & MAXBITS_MASK;

	symbol   = pNewDecHuffTbl->DecHuffSym[uIndex];
	ReadBits = pNewDecHuffTbl->BitsUsed[symbol];
	Dummy = BitBuffRead(pBitBuff, ReadBits, jbEnv);
	return symbol;	
}


