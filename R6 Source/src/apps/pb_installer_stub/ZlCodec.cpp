// ZlCodec.cpp			mkk@starcode.com

#include "ZlCodec.h"
#include "PackData.h"

#define DEBUG 0
#include <Debug.h>

ZlCodec::ZlCodec()
	:	CBUFSIZ(8192)
{
	c_stream.zalloc = (alloc_func)0;
    c_stream.zfree = (free_func)0;
    c_stream.opaque = (voidpf)0;
    
   	c_stream.avail_in = 0;
	c_stream.avail_out = 0;
	c_stream.next_in = NULL;
	c_stream.next_out = NULL;
	
	fBuf = (char *)malloc(CBUFSIZ*sizeof(char));

	fPrevMode = fMode;	
	fMode = M_NONE;
	fErrFunc = NULL;
}

ZlCodec::~ZlCodec()
{
	free(fBuf);
	
	if (fPrevMode == M_ENCODE) {
		PRINT(("deflate end\n"));
		deflateEnd(&c_stream);
	}
	else if (fPrevMode == M_DECODE) {
		PRINT(("inflate end\n"));
		inflateEnd(&c_stream);
	}
	else {
		PRINT(("no zlib end\n"));
	}
}

status_t	ZlCodec::BeginEncode()
{
	int err;

	//if (fMode != M_NONE)
	//	return CodecError(B_ERROR);
			
	c_stream.next_out = (Bytef *)fBuf;
	c_stream.avail_out = CBUFSIZ;


	if (fMode == M_NONE) {
		err = deflateInit(&c_stream,Z_DEFAULT_COMPRESSION);
	}
	else {
		err = deflateReset(&c_stream);
	}

	if (err != Z_OK) {
		return CodecError(err,FALSE);
	}
	fPrevMode = fMode;
	fMode = M_ENCODE;
	
	return B_NO_ERROR;
}

status_t	ZlCodec::EndEncode()
{
	//int err;
	
	/***
	if (fMode != M_ENCODE)
		return CodecError(B_ERROR);
	
	c_stream.avail_out = CBUFSIZ;
	c_stream.next_out = (Bytef *)fBuf;
	
	c_stream.avail_in = 0;
	c_stream.next_in = (Bytef *)NULL;
	
	err = deflateReset(&c_stream);
	if (err != Z_OK) {
		return CodecError(err,FALSE);
	}
	***/
	//fPrevMode = fMode;
	//fMode = M_NONE;
	
	return B_NO_ERROR;
}

void *		ZlCodec::Info()
{
	return &c_stream;
}

status_t	ZlCodec::ID()
{
	return 2;
}

// encode bytes in buf and output to stream
// return encoded size written
// or error
ssize_t		ZlCodec::EncodeBytes(BPositionIO *fileRep,
							ssize_t *amount,
							unsigned char *buf,
							bool final)
{
	ssize_t err;
	ssize_t	encSize = 0;

	if (fMode != M_ENCODE)
		return CodecError(B_ERROR);

	c_stream.next_in = (Bytef *)buf;
	c_stream.avail_in = *amount;
	
	while (c_stream.avail_in > 0) {
		if (c_stream.avail_out == 0) {
			// write a full buffer
			c_stream.next_out = (Bytef *)fBuf;
			err = fileRep->Write(fBuf, CBUFSIZ);
			encSize += err;
			if (err != CBUFSIZ)
				break;
			c_stream.avail_out = CBUFSIZ;
		}
		err = deflate(&c_stream,Z_NO_FLUSH);
		if (err != Z_OK) {
			CodecError(err,FALSE);
			break;
		}
	}
	if (final) {
		// perform a flush
		bool done = FALSE;
		for (;;) {
			// get amount sitting in avail_out
			long len = CBUFSIZ - c_stream.avail_out;
			
			// clear it
			if (len > 0) {
				err = fileRep->Write(fBuf,len);
				encSize += err;
				if (err != len)
					return B_ERROR;
				c_stream.next_out = (Bytef *)fBuf;
				c_stream.avail_out = CBUFSIZ;
			}
			if (done)
				break;
			err = deflate(&c_stream,Z_FINISH);
			if (c_stream.avail_out != 0 || err == Z_STREAM_END)
				done = TRUE;
			if (err != Z_OK && err != Z_STREAM_END) {
				CodecError(err,FALSE);
				break;
			}
		}
	}
	// update amount consumed
	*amount = *amount - c_stream.avail_in;
	
	// return encoded amount written
	return encSize;
}

status_t	ZlCodec::BeginDecode()
{
	int err;
	
	c_stream.next_out = (Bytef *)fBuf;
	c_stream.avail_out = CBUFSIZ;

	if (fMode == M_NONE) {
		err = inflateInit(&c_stream);
	}
	else {
		err = inflateReset(&c_stream);
	}
	
	if (err != Z_OK) {
		return CodecError(err,FALSE);
	}
	fPrevMode = fMode;
	fMode = M_DECODE;
	
	return B_NO_ERROR;
}

status_t	ZlCodec::EndDecode()
{
	//int err;
	
	//if (fMode != M_DECODE)
	//	return CodecError(B_ERROR);
	
	//c_stream.avail_out = CBUFSIZ;
	//c_stream.next_out = (Bytef *)fBuf;
	
	//c_stream.avail_in = 0;
	//c_stream.next_in = (Bytef *)NULL;
	
	//err = inflateReset(&c_stream);
	//if (err != Z_OK) {
	//	return CodecError(err,FALSE);
	//}
	//fPrevMode = fMode;
	//fMode = M_NONE;
	
	return B_NO_ERROR;
}

ssize_t		ZlCodec::DecodeBytes(BPositionIO *fileRep,
							ssize_t *amount,
							unsigned char *buf,
							bool final)
{
	ssize_t err;
	ssize_t	encSize = 0;

	if (fMode != M_DECODE)
		return CodecError(B_ERROR);
		
	// decode *amount bytes of data
	// return amount written to fileRep
	
	c_stream.next_in = (Bytef *)buf;
	c_stream.avail_in = *amount;
	
	while (c_stream.avail_in > 0) {
		PRINT(("decode\n"));
		if (c_stream.avail_out == 0) {
			// write a full buffer
			c_stream.next_out = (Bytef *)fBuf;
			err = fileRep->Write(fBuf, CBUFSIZ);
			encSize += err;
			if (err != CBUFSIZ)
				break;
			c_stream.avail_out = CBUFSIZ;
		}
		err = inflate(&c_stream,Z_NO_FLUSH);
		if (err != Z_OK && err != Z_STREAM_END) {
			CodecError(err,FALSE);
			break;
		}
		if (err == Z_STREAM_END) {
			PRINT(("stream end in first loop\n"));
			final = TRUE;
			break;
		}
	}
	if (final) {
		PRINT(("final flush\n"));
		// perform a flush, no more input
		bool done = FALSE;
		for (;;) {
			// get amount sitting in avail_out
			long len = CBUFSIZ - c_stream.avail_out;
			
			// clear it
			PRINT(("flush loop, len %d\n",len));
			if (len > 0) {
				err = fileRep->Write(fBuf,len);
				encSize += err;
				if (err != len)
					return B_ERROR;
				c_stream.next_out = (Bytef *)fBuf;
				c_stream.avail_out = CBUFSIZ;
			}
			if (done)
				break;
			err = inflate(&c_stream,Z_FINISH);
			if (c_stream.avail_out != 0 || err == Z_STREAM_END)
				done = TRUE;
			if (err != Z_OK && err != Z_STREAM_END) {
				CodecError(err,FALSE);
				break;
			}
		}
	}
	// update amount consumed
	*amount = *amount - c_stream.avail_in;
	
	// return encoded amount written
	return encSize;
}

void	ZlCodec::SetErrorHook(error_hook foo)
{
	fErrFunc = foo;
}

status_t	ZlCodec::CodecError(status_t	err, bool isOSErr)
{
	if (isOSErr)
		return err;

	if (err != Z_OK) {
		if (fErrFunc) {
			switch (err) {
				case Z_ERRNO:
					fErrFunc("The compression library encountered a file-system error");
					break;
				case Z_STREAM_ERROR:
					fErrFunc("Compression stream error.");
					break;
				case Z_DATA_ERROR:
					fErrFunc("Compression data error.");
					break;
				case Z_MEM_ERROR:
					fErrFunc("Memory error in compression library");
					break;
				case Z_BUF_ERROR:
					fErrFunc("Compression buffer error");
					break;
				case Z_VERSION_ERROR:
					fErrFunc("The compressed stream is not the correct version");
					break;
				case Z_STREAM_END:
					fErrFunc("Stream end.");
					break;
				default:
					fErrFunc("An error occured in data compression.");
					break;
			}
		}
		return B_ERROR;
	}
	
	return B_NO_ERROR;
}
