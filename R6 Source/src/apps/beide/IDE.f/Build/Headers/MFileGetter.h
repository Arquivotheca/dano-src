//========================================================================
//	MFileGetter.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFILEGETTER_H
#define _MFILEGETTER_H

class MFileGetter
{
public:
	
	virtual	off_t				FileSize(
									const char*	inName,
									bool		inSystemTree) = 0;
	virtual	long				WriteFileToBlock(
									void*		inBlock,
									off_t&		ioSize,
									const char*	inName,
									bool		inSystemTree) = 0;
};

#endif
