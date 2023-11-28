/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: inputfile.h
 *   project : MPEG Layer-3 Audio Encoder
 *   author  : Stefan Gewinner gew@iis.fhg.de
 *   date    : 1999-08-18
 *   contents/description: file reading function prototypes
 *
 * $Header: /home/cvs/menc/ciEncoder/common/Attic/inputfile.h,v 1.1.2.3 1999/08/18 15:50:05 gew Exp $
 *
\***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

unsigned int openInputFile (char *filename) ;
unsigned int getSampleRateFromInputFile (void) ;
unsigned int getChannelCountFromInputFile (void) ;
unsigned int getInputFileDataSize (void) ;
unsigned int readFromInputFile (void *buffer, unsigned int buflen) ;
unsigned int closeInputFile (void) ;

#ifdef __cplusplus
}
#endif
