//========================================================================
//	MAIFFPlayer.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MAIFFPLAYER_H
#define _MAIFFPLAYER_H

#include "IDEConstants.h"

class MBufferPlayer;


const uint32 AIFFMagicWord = MW_FOUR_CHAR_CODE('FORM');
const uint32 AIFFFileType = MW_FOUR_CHAR_CODE('AIFF');
const uint32 COMMType = MW_FOUR_CHAR_CODE('COMM');
const uint32 SSNDType = MW_FOUR_CHAR_CODE('SSND');

struct AIFFFileHeader
{
	uint32		magicWord;	// must be FORM
	int32		fileSize;
	uint32		fileType;	// must be AIFF
};

struct ChunkHeader
{
	uint32		chunkType;	// whatever
	int32		chunkSize;	// doesn't include header
};

struct COMMChunk 
{
	uint32			chunkType;	// COMM
	int32			chunkSize;	// 18, doesn't include first two fields
	short			channels;
	short			frames1;	// in two pieces in the struct 
	short			frames2;	// because of padding
	short			bits;		// bits per sample
	unsigned char	rate[10];	// ieee extended
};

struct SSNDChunk 
{
	uint32		chunkType;	// SSND
	int32		chunkSize;	// doesn't include first two fields
	int32		offset;
	int32		blocksize;
};


class MAIFFPlayer
{
public:
								MAIFFPlayer(
									const void *	inData,
									int32			inLength);
								~MAIFFPlayer();
								
	void						Play();
								
static void 					ConvertToIeeeExtended(
									double num, 
									unsigned char * bytes);
static double 					ConvertFromIeeeExtended(
									const unsigned char *		bytes);

private:
		
		MBufferPlayer*			fPlayer;
		const COMMChunk *		fCommChunk;
		const SSNDChunk * 		fSoundChunk;

	bool						ProcessAChunk(
									const char *&	ioData);
};

#endif
