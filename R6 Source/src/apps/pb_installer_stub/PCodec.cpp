// PCodec.cpp			mkk@starcode.com

#include "PCodec.h"

#include "PackData.h"


PCodec::PCodec()
{
}

PCodec::~PCodec()
{
}

status_t	PCodec::BeginEncode()
{
	return 0;
}

status_t	PCodec::EndEncode()
{
	return 0;
}

status_t	PCodec::BeginDecode()
{
	return 0;
}

status_t	PCodec::EndDecode()
{
	return 0;
}

void	*PCodec::Info()
{
	return NULL;
}

status_t	PCodec::ID()
{
	return 1;
}

// encode bytes in buf and output to stream
ssize_t	PCodec::EncodeBytes(BPositionIO *fileRep,
							ssize_t *amount,
							unsigned char *buf,
							bool final)
{
	final;
	return fileRep->Write(buf, *amount);
}

// decode bytes in buf and output to stream
ssize_t	PCodec::DecodeBytes(BPositionIO *fileRep,
							ssize_t *amount,
							unsigned char *buf,
							bool final)
{
	final;
	return fileRep->Write(buf, *amount);
}
