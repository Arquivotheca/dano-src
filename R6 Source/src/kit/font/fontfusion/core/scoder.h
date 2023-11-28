/*
 * Scoder.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#ifndef __T2K_SCODER__
#define __T2K_SCODER__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/* private */
#define No_of_chars 256                 /* Number of character symbols      */

typedef struct {
	/* private */
	tsiMemObject *mem;
	unsigned char *numBitsUsed;
	unsigned long numEntries, maxBits;
	unsigned char *LookUpSymbol;	/* maps a bitpattern to a symbol */
	unsigned short *LookUpBits;		/* maps a symbol the bitpattern  */

	/* public */
} SCODER;

/* Private methods. */
/*
 * Internal function used for sequencing the look-up table.
 */
void SCODER_SequenceLookUp( SCODER *t );

/* Public methods. */

/* Two different constructors. */
/* count is 256 entries large array with event counts for all the bytes. */
/* codingCost is an informative output from this constructor. */
#ifdef ENABLE_WRITE
SCODER *New_SCODER( tsiMemObject *mem, long *count, long *codingCost);

/*
 * This method saves an SCODER model to the stream, so that it can later be
 * recreated with New_SCODER_FromStream().
 */
void SCODER_Save( SCODER *t, OutputStream *out );

/*
 * Write a symbol to the output stream.
 */
int SCODER_EncodeSymbol( SCODER *t, OutputStream *out, unsigned char symbol );

#endif /* ENABLE_WRITE */

/*
 * This standard constructor recreates the SCODER object from a stream.
 */
SCODER *New_SCODER_FromStream( tsiMemObject *mem, InputStream *in );


/*
 * Read a symbol from the input stream.
 */
unsigned char SCODER_ReadSymbol( SCODER *t, InputStream *in );


/*
 * The destructor.
 */
void Delete_SCODER( SCODER *t );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_SCODER__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/scoder.h 1.3 1999/09/30 15:11:37 jfatal release $
 *                                                                           *
 *     $Log: scoder.h $
 *     Revision 1.3  1999/09/30 15:11:37  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:33  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
