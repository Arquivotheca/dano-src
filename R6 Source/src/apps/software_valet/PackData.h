#ifndef _PACKDATA_H_
#define _PACKDATA_H_

#include <SupportDefs.h>
class BPositionIO;

// PackData.h		mkk@starcode.com

typedef struct {
	int32	what;
	int16	flags;
	uchar	type;
} record_header;

enum {
	END_TYPE = 0,
	INT_TYPE,			// integer type
	STR_TYPE,			// string type (2^32 max)
	BIN_TYPE,			// binary type (2^64 max + segmenting)
	LIS_TYPE,			// list type
	BUF_TYPE,			// flat buffer type (2^32 max)
	EXT_TYPE = 127		// extension types, for future
};


typedef		status_t (*code_callback)(size_t inDelta, size_t outDelta, void *data);


class PCodec;

class PackData
{
public:
	PackData();
	virtual		~PackData();
	
	status_t	ReadRecordHeader(BPositionIO *file, record_header *header);
	status_t	WriteRecordHeader(BPositionIO *file, int32 id, uchar type, int16 flags);
	status_t	WriteRecordHeader(BPositionIO *file, const record_header *header);
	status_t	WriteEndHeader(BPositionIO *);
	
	status_t	ReadInt64(BPositionIO *file, int64 *result);
	status_t	WriteInt64(BPositionIO *file, int64 value, int32 id = -1);
	status_t	ReadInt32(BPositionIO *file, int32 *result);
	status_t	WriteInt32(BPositionIO *file, int32 value, int32 id = -1);
	
	status_t	ReadString(BPositionIO *file, char **str, size_t limit = LONG_MAX);
	status_t	ReadString(BPositionIO *file, char *str, size_t limit);
	status_t	WriteString(BPositionIO *file, const char *str, int32 id = -1);
	
	// cover for WriteString with fixed size
	//status_t	ReadBuf(BPositionIO *file, char **str, size_t limit = LONG_MAX);
	status_t	ReadBuf(BPositionIO *file, char *byte, size_t limit);
	status_t	WriteBuf(BPositionIO *file, char *byte, size_t size, int32 id = -1);
	
	status_t	WriteBin(	BPositionIO *dstfile,
							BPositionIO *srcfile,
							off_t *encoded_size,
							off_t *decoded_size,
							int *flags,
							code_callback func = NULL,
							int32 id = -1);
							
	status_t	ReadBin(BPositionIO *dstfile,
						BPositionIO *srcfile,
						int flags,
						code_callback func = NULL);
	
	status_t	SkipString(BPositionIO *file);
	status_t	SkipData(BPositionIO *file, record_header *header);
	status_t	SkipBin(BPositionIO *file);
	
	status_t			SetCurrentCodec(PCodec *nCodec);
	PCodec				*CurrentCodec() const;

	void				SetCallbackData(void *p);

	enum {
		ENCODE_ALL = -1
	};
	enum {
		R_CONTINUED_TO		=	0x0001,
		R_CONTINUED_FROM	=	0x0002,
		R_RESERVED1			=	0x0004,
		R_RESERVED2			=	0x0008,
		
		R_DELETED			=	0x0010
	};
	enum {
		RECORD_HEADER_SZ = 7,		// 4 + 2 + 1 bytes
		INT_SZ = 8
	};
private:
	PCodec		*fCodec;

	PCodec		*defCodec;
	enum {
		D_ERROR = -1,
		D_NO_ERROR = 0
	};	
	
	void		*fCallData;
	
	const size_t		BUF_SIZ;
	unsigned char		*dataBuf;
};

#endif

