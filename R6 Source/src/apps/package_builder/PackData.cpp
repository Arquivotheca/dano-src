#include <Be.h>
// PackData.cpp			mkk@starcode.com

#include "PackData.h"
#include "PCodec.h"

#define DEBUG 0
#include <Debug.h>

#ifndef LONGLONG_MAX
#define LONGLONG_MAX LONG_LONG_MAX
#endif

PackData::PackData()
	:	fCodec(NULL),
		BUF_SIZ(16384)
{
	defCodec = new PCodec();
	SetCurrentCodec(defCodec);
	SetCallbackData(NULL);
	dataBuf = (unsigned char *)malloc(BUF_SIZ);
}

PackData::~PackData()
{
	delete defCodec;
	free(dataBuf);
}

status_t	PackData::SetCurrentCodec(PCodec *p)
{
	if (p == NULL) {
		fCodec = defCodec;
	}
	else {
		fCodec = p;
	}
	return B_NO_ERROR;
}

PCodec	*PackData::CurrentCodec() const
{
	return fCodec;
}

void		PackData::SetCallbackData(void *p)
{
	fCallData = p;
}

status_t	PackData::ReadRecordHeader(BPositionIO *file, record_header *header)
{
	PRINT(("PackData::ReadRecordHeader, ENTER\n"));
	unsigned char buf[RECORD_HEADER_SZ];
	off_t res;
	
	res = file->Read(buf, RECORD_HEADER_SZ);
	if (res != RECORD_HEADER_SZ) {
		PRINT(("PackData::ReadRecordHeader, read returned %d\n",res));
		return D_ERROR;
	}

	PRINT(("PackData::ReadRecordHeader, got file data\n"));

	header->what = B_BENDIAN_TO_HOST_INT32(*(int32 *)buf);
	header->flags = B_BENDIAN_TO_HOST_INT16(*(int16 *)(buf+4));
	header->type = buf[6];
	// interpret flags if extra data???

	#if DEBUG
		char buff[5];
		memcpy(buff,&(header->what),4);
		buff[4] = 0;
		printf("HEADER->WHAT '%s'\n",buff);
	#endif
	return D_NO_ERROR;
}

status_t	PackData::WriteRecordHeader(BPositionIO *file, int32 id, uchar type, int16 flags)
{
	unsigned char buf[RECORD_HEADER_SZ];
	off_t res;

	*((int32 *)buf) = B_HOST_TO_BENDIAN_INT32(id);
	*((int16 *)(buf+4)) = B_HOST_TO_BENDIAN_INT16(flags);
	*(buf+6) = type;
	
	res = file->Write(buf,RECORD_HEADER_SZ);
	if (res != RECORD_HEADER_SZ)
		return D_ERROR;

	return D_NO_ERROR;
}

/***
evil and confusing
status_t	PackData::WriteRecordHeader(BPositionIO *file,
									const record_header *header)
{
	return WriteRecordHeader(file, header->what, header->type, header->flags);
}
****/

status_t	PackData::WriteEndHeader(BPositionIO *file)
{
	return WriteRecordHeader(file, END_TYPE,  END_TYPE,  END_TYPE);
}

status_t	PackData::ReadInt64(BPositionIO *file, int64 *result)
{
	unsigned char buf[INT_SZ];
	off_t res;
	
	res = file->Read(buf,INT_SZ);
	if (res != INT_SZ) return D_ERROR;

	*result = B_BENDIAN_TO_HOST_INT64(*((int64 *)buf));
	
	return D_NO_ERROR;
}

/*****

status_t	PackData::ReadVInt(BPositionIO *file, off_t *result)
{
	unsigned char	lenByte;
	int				datLen;
	int				shift;
	unsigned char	buf[sizeof(off_t)];
	off_t	res;
	
	res = file->Read(&lenByte,1);
	if (res != 1) return D_ERROR;
	
	datLen = lenByte;
	if (datLen > sizeof(off_t)) {
		// integer too large
		return D_ERROR;
	}	
	// read bytes
	res = file->Read(buf,&len);
	
	
	shift = datLen - sizeof(off_t);		// any difference in size (smaller)
	if (shift > 0) {
		// shift bytes down by difference so we have a complete word
		memmove(buf + shift, buf, datLen);
		if (buf[shift] & 0x80)	{	// top bit is set and there are extra bytes
			// sign extend
			memset(buf, shift, 0xFF);
		}
		else {
			// clear upper bytes
			memset(buf, shift, 0x00);
		}
	}

#if little_endian
	off_t tmpValue;
	for (i = 0; i < sizeof(off_t); i++)
		tmpValue = (tmpValue << 8) | buf[i];	// shift msb left
	}
	*value = tmpValue;
#else
	*value = *((off_t *)buf;
#endif

	return D_NO_ERROR;
}
***/

status_t	PackData::WriteInt64(BPositionIO *file, int64 value, int32 id)
{
	unsigned char buf[INT_SZ];
	off_t		res;
	status_t	err;
	
	if (id != -1) {
		// Write a header
		err = WriteRecordHeader(file,id,INT_TYPE,2);
		if (err != D_NO_ERROR)
			return err;
	}
	*((int64 *)buf) = B_HOST_TO_BENDIAN_INT64(value);

	res = file->Write(buf,INT_SZ);
	if (res != INT_SZ)
		return D_ERROR;
		
	return D_NO_ERROR;
}

status_t	PackData::ReadInt32(BPositionIO *file, int32 *result)
{
	PRINT(("PackData::ReadInt32, ENTER\n"));
	
	off_t res;
	res = file->Read(result,sizeof(int32));
	if (res != sizeof(int32)) {
	PRINT(("PackData::ReadInt32, read error %d\n",res));
	
		return D_ERROR;
	}
	*result = B_BENDIAN_TO_HOST_INT32(*result);
	
	PRINT(("PackData::ReadInt32, VALUE %d\n",*result));	
	
	return D_NO_ERROR;
}

status_t	PackData::WriteInt32(BPositionIO *file, int32 value, int32 id)
{
	status_t	err;
	off_t		res;
	
	if (id != -1) {
		// Write a header
		err = WriteRecordHeader(file,id,INT_TYPE,1);
		if (err != D_NO_ERROR)
			return err;
	}
	value = B_HOST_TO_BENDIAN_INT32(value);
	
	res = file->Write(&value,sizeof(int32));
	if (res != sizeof(int32))
		return D_ERROR;
	
	return D_NO_ERROR;
}

// malloc buffer
status_t	PackData::ReadString(BPositionIO *file, char **str, size_t limit)
{
	off_t		res;
	status_t	err;
	int32		len;
	
	// should be variable size
	// up to ssize_t (signed)
	err = ReadInt32(file,&len);
	
	// off_t	len
	// err = ReadInt(file, &len)
	// check for error (could be that integer is too large)
	
	if (err != D_NO_ERROR)
		return err;
		
	if (len+1 > limit)
		return D_ERROR;		// string too large
	
	*str = (char *)malloc(len+1);
	if (!*str)
		return D_ERROR;		// malloc error
		
	res = file->Read(*str,len);
	(*str)[len] = '\0';			// terminate string
	if (res != len)
		return D_ERROR;
	
	return D_NO_ERROR;
}

// buffer already alloced
status_t	PackData::ReadString(BPositionIO *file, char *str, size_t limit)
{
	off_t		res;
	status_t	err;
	int32		len;
	
	// should be variable size
	//
	err = ReadInt32(file,&len);
	
	if (err != D_NO_ERROR)
		return err;
		
	if (len+1 > limit) {
		//return D_ERROR;		// string too large
		len = limit-1;	
	}
		
	res = file->Read(str,len);
	str[len] = '\0';			// terminate string
	if (res != len)
		return D_ERROR;
	
	return D_NO_ERROR;
}

status_t	PackData::WriteString(BPositionIO *file, const char *str, int32 id)
{
	off_t		res;
	status_t	err;
	int32		len;
	
	if (id != -1) {
		// Write a header
		err = WriteRecordHeader(file,id,STR_TYPE,0);
		if (err != D_NO_ERROR)
			return err;
	}
	len = strlen(str);
	
	err = WriteInt32(file, len);
	if (err != D_NO_ERROR)
		return err;
		
	res = file->Write(str, len);
	if (res != len)
		return D_ERROR;
		
	return D_NO_ERROR;
}

status_t	PackData::WriteBuf(BPositionIO *file, char *byte, size_t size, int32 id)
{
	off_t		res;
	status_t	err;
	
	if (id != -1) {
		// Write a header
		err = WriteRecordHeader(file,id,BUF_TYPE,0);
		if (err != D_NO_ERROR)
			return err;
	}
	err = WriteInt32(file, size);
	if (err != D_NO_ERROR)
		return err;
		
	res = file->Write(byte, size);
	if (res != size)
		return D_ERROR;
		
	return D_NO_ERROR;
}

status_t	PackData::ReadBuf(BPositionIO *file, char *byte, size_t limit)
{
	off_t		res;
	status_t	err;
	int32		len;
	
	// should be variable size
	//
	err = ReadInt32(file,&len);
	
	if (err != D_NO_ERROR)
		return err;
		
	if (len > limit) {
		return D_ERROR;		// buffer too large
	}
		
	res = file->Read(byte,len);
	if (res != len)
		return D_ERROR;
	
	return D_NO_ERROR;
}

status_t	PackData::SkipString(BPositionIO *file)
{
	status_t	err;
	int32		len;
	
	// variable size
	err = ReadInt32(file,&len);
	
	if (err != D_NO_ERROR)
		return err;
	
	file->Seek(len,SEEK_CUR);	// check for bad seek
	
	return D_NO_ERROR;
}


status_t	PackData::WriteBin(BPositionIO *dstfile,
								BPositionIO *srcfile,
								off_t *encoded_size,
								off_t *decoded_size,
								int *flags,
								code_callback callback,
								int32 id)
{
	unsigned char *buf = dataBuf;
	int32		codec = 0;
	status_t	err = D_NO_ERROR;
	
	// space for params
	off_t	params = dstfile->Position();
	
	PRINT(("params start %Ld\n",params));
	
	if (id != -1)
		dstfile->Seek(RECORD_HEADER_SZ,SEEK_CUR);
	
	off_t foo = dstfile->Seek(sizeof(int64)*2 + sizeof(int32),SEEK_CUR);
	PRINT(("params end %Ld\n",foo));
	
	off_t amount = *decoded_size;
	off_t encodeMax = *encoded_size;
	if (amount == ENCODE_ALL) {
		// do the whole file
		amount = LONGLONG_MAX;
	}
	if (encodeMax == ENCODE_ALL) {
		// no limit on segment size
		encodeMax = LONGLONG_MAX;
	}
	
	if (!(*flags & R_CONTINUED_FROM)) {
		fCodec->BeginEncode();
	}
	
	off_t	inBytes = 0;
	off_t	outBytes = 0;
	bool		continued = FALSE;
	bool		final = FALSE;
	while (amount && !final) {
		ssize_t		inRes;
		ssize_t		outRes;
		ssize_t		readAmount;

		if (amount <= BUF_SIZ) {
			readAmount = amount;
			final = TRUE;
		}
		else {
			readAmount = BUF_SIZ;
			final = FALSE;
		}
		
		//PRINT(("attempting read of %d bytes from srcfile\n",readAmount));
		inRes = srcfile->Read(buf,readAmount);
				
		if (inRes < 0) {
			err = inRes;
			break;
		}
		else if (inRes < readAmount) {
			// could be zero
			PRINT(("inRes, %d < readAmount\n",inRes));
			final = TRUE;
		}
		
		inBytes += inRes;
		
		outRes = inRes;	
		
		//PRINT(("compress %d bytes read\n",inRes));
		
		outRes = fCodec->EncodeBytes(dstfile,&outRes,buf,final);
		if (outRes < 0) {
			err = outRes;
			break;
		}
		outBytes += outRes;
		if (outRes) {
			PRINT(("output %d encoded bytes to dstfile, inRes is %d\n",outRes,inRes));
		}
			
		// do callback function
		// in & out bytes, cancel check
		if (callback && (err = callback(inRes,outRes,fCallData)) < 0)
			break;
		
		if (outBytes >= encodeMax) {
			continued = TRUE;
			final = TRUE;
		}
	}
	if (!continued || err < D_NO_ERROR)
		fCodec->EndEncode();
	
	if (err >= D_NO_ERROR) {
		off_t cur = dstfile->Position();
		dstfile->Seek(params,SEEK_SET);
	
		if (continued) {
			*flags = *flags | R_CONTINUED_TO;
		}
		if (id != -1) {
			// Write a header
			err = WriteRecordHeader(dstfile,id,BIN_TYPE,*flags);
			if (err != D_NO_ERROR)
				return err;
		}
		
		err = WriteInt64(dstfile,outBytes);
		if (err != D_NO_ERROR) return err;
		err = WriteInt64(dstfile,inBytes);
		if (err != D_NO_ERROR) return err;
		
		codec = fCodec->ID();
		err = WriteInt32(dstfile,codec);
		if (err != D_NO_ERROR) return err;
		
		// write continued flag!!!
		
		dstfile->Seek(cur,SEEK_SET);
	}
	// info out, checksum, in out bytes, other stats
	*encoded_size = outBytes;
	*decoded_size = inBytes;
	return err;
}

status_t	PackData::ReadBin( BPositionIO *dstfile,
								BPositionIO *srcfile,
								int flags,
								code_callback callback)
{
	unsigned char *buf = dataBuf;
		
	off_t		amount;
	off_t		decodedSize;
	int32		codec;
	status_t	err;

	// should be variable size integers
	err = ReadInt64(srcfile,&amount);
	if (err != D_NO_ERROR) return err;
	err = ReadInt64(srcfile,&decodedSize);
	if (err != D_NO_ERROR) return err;
	err = ReadInt32(srcfile,&codec);
	if (err != D_NO_ERROR) return err;
	
	// codec id doesn't match
	if (codec != fCodec->ID())	return err;
	
	// this is a problem
	if (amount == ENCODE_ALL) {
		amount = LONGLONG_MAX;
	}
	PRINT(("encoded size is %d\n",(long)amount));
	PRINT(("decoded size is %d\n",(long)decodedSize));

	// decode the data into dstIO
	
	
	if (!(R_CONTINUED_FROM & flags)) {
		// if this is not a contunation then we need to init
		// the codec
		PRINT(("begin decode\n"));
		fCodec->BeginDecode();	
	}
	
	off_t		outBytes = 0;
	bool		final = FALSE;
	while (amount && !final) {
		ssize_t		res;
		ssize_t		readAmount;

		if (amount <= BUF_SIZ) {
			readAmount = amount;
			final = TRUE;
		}
		else {
			readAmount = BUF_SIZ;
			final = FALSE;
		}
		
		PRINT(("attempting read of %d bytes from srcfile\n",readAmount));
		res = srcfile->Read(buf,readAmount);
				
		if (res == 0) {
			break;
		}
		else if (res < 0) {
			err = res;
			break;
		}
		else if (res < readAmount) {
			final = TRUE;
		}
		amount -= res;
		
		ssize_t	read = res;
		PRINT(("decode %d bytes to dstfile\n",read));
		ssize_t decoded = fCodec->DecodeBytes(dstfile,&read,buf,final);
		if (decoded < 0) {
			// error condition
			err = res;
			break;
		}
		if (read < res) {
			// amount consumed was less than requested
			// meaning the codec was at an end
			// so rewind the input file
			
			// what if we cant rewind!!!???
			// do we define a block size???
		}
		
		outBytes += decoded;
		if (outBytes >= decodedSize) {
			final = TRUE;
		}
		PRINT(("output %d decoded bytes to dstfile\n",decoded));
		if (final)
			PRINT(("final is true\n"));
		else
			PRINT(("final is false\n"));
		if (callback && (err = callback(read,decoded,fCallData)) < 0)
			break;

	}
	if (!(R_CONTINUED_TO & flags)) {
		PRINT(("end decode\n"));
		fCodec->EndDecode();
	}
	
	PRINT(("ReadBin err result %d\n",err));
	return err;
}

status_t	PackData::SkipBin(BPositionIO *file)
{
	off_t		encodedSize;
	off_t		decodedSize;
	int32		codec;
	status_t	err;
	
	// should be variable size
	err = ReadInt64(file,&encodedSize);
	if (err != D_NO_ERROR) return err;
	err = ReadInt64(file,&decodedSize);
	if (err != D_NO_ERROR) return err;
	err = ReadInt32(file,&codec);
	if (err != D_NO_ERROR) return err;
	
	if (encodedSize == -1) {
		// need to decode to skip
	}
	
	file->Seek(encodedSize,SEEK_CUR);
	
	return D_NO_ERROR;
}

status_t	PackData::SkipData(BPositionIO *file, record_header *header)
{
	status_t err = D_NO_ERROR;
	switch(header->type) {
		case END_TYPE:  {
			PRINT(("End Type\n"));
			// nothing to do
			err = D_NO_ERROR;
			break;
		}
		case INT_TYPE: {
			PRINT(("Int Type, header flags == %d\n",header->flags));

			// read appropriate integer type
			//if (header->flags == 1)
			//	file->Seek(sizeof(int32),SEEK_CUR);
			//else if (header->flags == 2)
			//	file->Seek(2sizeof(int64),SEEK_CUR);
			//else
			
			file->Seek(sizeof(int32) * header->flags,SEEK_CUR);
			break;
		}
		case STR_TYPE: {
			PRINT(("String Type\n"));

			// skip string
			SkipString(file);
			break;
		}
		case BIN_TYPE: {
			PRINT(("Bin Type\n"));

			// skip bin
			SkipBin(file);
			break;
		}
		case LIS_TYPE: {
			PRINT(("List Type\n"));
			// later move this into its own function
			record_header	curHeader;
			while ((err = ReadRecordHeader(file, &curHeader)) == D_NO_ERROR) {
				// got a header
				err = SkipData(file,&curHeader);
				if (err != D_NO_ERROR)
					break;
					
				// if last, end of list
				if (curHeader.what == END_TYPE)
					break;			
			}
			break;
		}
		case BUF_TYPE: {
			PRINT(("Buf Type\n"));
			SkipString(file);
			break;
		}
		default: {
			PRINT(("unknown Type\n"));
			// unknown type
			err = D_ERROR;
			break;
		}
	}
	return err;
}


