// ZlCodec.h			mkk@starcode.com

#include "PCodec.h"
#include "zlib.h"

#ifndef _ZICODEC_H
#define _ZICODEC_H

typedef		status_t (*error_hook)(const char *,status_t = 0);


class ZlCodec : public PCodec
{
public:
	ZlCodec();
	virtual ~ZlCodec();

	virtual status_t	BeginEncode();
	virtual status_t	EndEncode();
	
	virtual status_t	BeginDecode();
	virtual status_t	EndDecode();
	
	virtual void *		Info();
	virtual status_t	ID();
	// checksumming shit
	// virtual	
	
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
	virtual	void		SetErrorHook(error_hook);
	
	virtual status_t	CodecError(status_t	err, bool isOS = TRUE);	
private:
	const ssize_t		CBUFSIZ;
	char				*fBuf;
	
	z_stream			c_stream;
	
	int					fMode;	// encoding or decoing
	int					fPrevMode;
	bool				needFlush;
	error_hook			fErrFunc;
	
	enum {
		M_NONE, M_ENCODE, M_DECODE
	};
};

#endif
