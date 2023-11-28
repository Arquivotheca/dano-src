#ifndef _PCODEC_H_
#define _PCODEC_H_

#include <DataIO.h>

// PCodec.h		mkk@starcode.com

class	PackData;

class PCodec
{
public:
	PCodec();
	virtual ~PCodec();

	// virtual status_t	SetDataIO(PackData *p);
	
	virtual status_t	BeginEncode();
	virtual status_t	EndEncode();
	
	virtual status_t	BeginDecode();
	virtual status_t	EndDecode();
	
	virtual void *		Info();
	virtual	int32		ID();		// the unique id number for the codec	
	
	// encode bytes in buf and output to stream
	virtual ssize_t		EncodeBytes(BPositionIO *fileRep,
								ssize_t *amount,
								unsigned char *buf,
								bool final = FALSE);
	// decode bytes in buf and output to stream
	virtual ssize_t		DecodeBytes(BPositionIO *fileRep,
								ssize_t *amount,
								unsigned char *buf,
								bool final = FALSE);
};

#endif
