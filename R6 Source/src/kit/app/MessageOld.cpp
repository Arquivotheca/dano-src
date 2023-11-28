//*****************************************************************************
//
//	File:		MessageOld.cpp
//
//	
//	Cut&Paster:	Dianne Hackborn
//
//				Implement reading of old style flattened messages.
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#include "MessageBody.h"

#include <ByteOrder.h>
#include <Debug.h>
#include <Message.h>
#include <StreamIO.h>
#include <String.h>

#define D DEBUG_ONLY
//#define D(x)
//#define D(x) x

#define NOISY(x)
//#define NOISY(x) x

#define DOUT BSer

static void copy_into_long(int32 *dst, const unsigned char *src,
	bool swap = false)
{
	if (swap)
		*dst = B_BYTE_SWAP_INT32(*((uint32 *) src));
	else
		*dst = *((uint32 *) src);
}

#if 0
static void copy_from_long(unsigned char *dst, int32 src)
{
	*((uint32 *) dst) = src;
}
#endif

/* ---------------------------------------------------------------- */

static uint32 _checksum_(const uchar *buf, int32 size)
{
	uint32 sum = 0;
	uint32 temp = 0;

//+	PRINT(("_check_sum_(%x, %d)\n", buf, size));

	while (size > 3) {
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
		sum += B_BYTE_SWAP_INT32(*(int *)buf);
#else
		sum += *(int *)buf;
#endif
		buf += 4;
		size -= 4;
	}

	while (size > 0) {
		temp = (temp << 8) + *buf++;
		size -= 1;
		sum += temp;
	}

	return sum;
}

/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */
/* --------------------------------------------------------------------- */

#define	FOB_WHAT			'FOB1'

//
// flags for the overall message (the bitfield is 1 byte)
//
#define FOB_BIG_ENDIAN		0x01
#define FOB_INCL_TARGET		0x02
#define FOB_INCL_REPLY		0x04
#define FOB_SCRIPT_MSG		0x08

//
// flags for each entry (the bitfield is 1 byte)
//
#define FOB_VALID			0x01
#define FOB_MINI_DATA		0x02
#define FOB_FIXED_SIZE		0x04
#define FOB_SINGLE_ITEM		0x08

#define END_OF_ENTRIES		0x0

extern const char *_CUR_SPECIFIER_ENTRY_; //	= "_cur_specifier_";

static const int32	PREFERRED_TOKEN = -2;

struct var_chunk {
	int32	fDataSize;				
	char	fData[1];
};

static int32 da_chunk_hdr_size()
{
	return sizeof(int32);
}

static int32 da_chunk_size(var_chunk *v)
{
	return (v->fDataSize + da_chunk_hdr_size() + 7) & ~7;
}

static var_chunk* da_next_chunk(var_chunk *v)
{
	return (var_chunk *) (((char*) v) + da_chunk_size(v));
}

// This function is defined in MessageBody.cpp.

BMessageBody* BPrivate::unflatten_old_message(BDataIO* stream,
											  header_args* outArgs,
											  message_target* outTarget,
											  status_t* outResult)
{
	// Very blatantly, highly inefficient.
	
	// min header size contents
	//	size = 4;			// signature
	//	size += 4;			// checksum of header
	//	size += 4;			// size itself
	//	size += 4;			// what field
	//	size += 1;			// flags field
	static const ssize_t	kMinHdrSize = 17;
	
	ssize_t		error;
	uint32		sig = 0;
	int32		rsize = -1;
	int32		size = -1;
	uchar		mflags;
	uint32		ocheck_sum;
	uint32		ncheck_sum;
	int32		hdr_size;
	
	uint32		*buffer = NULL;
	unsigned char* dataBuffer = NULL;
	BMessageBody *msg = NULL;
	
	unsigned char* buf;
	unsigned char* start;
	unsigned char* expected_end;
	
	bool		swap = false;
	bool		look_for_specifiers = false;
	
	error = stream->Read(&sig, 4);
	if (error < B_OK) goto other_error;
	if (error != 4) goto bad_data;
	
	NOISY(DOUT << "BMessageOld: got signature=" << sig << endl);
	
	if (sig == '1BOF') {
		D(DOUT << "BMessageOld: swapping!" << endl);
		swap = true;
	}
	
	if (swap)
		sig = B_BYTE_SWAP_INT32(sig);

	if (sig != FOB_WHAT) {
		*outResult = B_NOT_A_MESSAGE;
		return NULL;
	}

	error = stream->Read(&ocheck_sum, 4);
	if (error < B_OK) goto other_error;
	if (error != 4) goto bad_data;
	if (swap)
		ocheck_sum = B_BYTE_SWAP_INT32(ocheck_sum);

	NOISY(DOUT << "BMessageOld: got checksum=" << ocheck_sum << endl);
	
	error = stream->Read(&rsize, 4);
	if (error < B_OK) goto other_error;
	if (error != 4) goto bad_data;
	if (swap)
		size = B_BYTE_SWAP_INT32(rsize);
	else
		size = rsize;
		
	NOISY(DOUT << "BMessageOld: got size=" << size << endl);
	
	if (size < kMinHdrSize) goto bad_data;

//+	PRINT(("unflat size=0x%x, cksum=0x%x, sig=%.4s, %.4s\n",
//+		rsize, check_sum, (char *) &rsig, (char *) &sig));
	buffer = (uint32 *) malloc(size-8);
	buffer[0] = rsize;
	
	NOISY(DOUT << "BMessageOld: allocated buffer=" << buffer << endl);
	
	if (!buffer) {
		error = B_NO_MEMORY;
		goto other_error;
	}

	error = stream->Read(buffer+1, size-12);		// read the remainder
	if (error < B_OK) goto other_error;
	if (error != size-12) goto bad_data;

	NOISY(DOUT << "BMessageOld: unflattening " << size << " bytes of data: "
		 << BHexDump(buffer, size-8, 16, "\t") << endl);
	
	buf = (unsigned char*)(buffer+1);
	start = buf;
	
	expected_end = buf + (size-12);
	NOISY(DOUT << "BMessageOld: flat data ends at " << expected_end
		 << " (" << (void*)(expected_end-start+4) << ")" << endl);
	
	copy_into_long((long*)&(outArgs->what), buf);			buf += 4;
	if (swap) {
		outArgs->what = B_BYTE_SWAP_INT32(outArgs->what);
	}
	
	NOISY(DOUT << "BMessageOld: got what=" << outArgs->what << endl);
	
	mflags = *buf;											buf += 1;
	
	NOISY(DOUT << "BMessageOld: got flags=" << (int32)mflags << endl);
	
	{
		// Read target information from message.
		message_target dummyTarget;
		message_target& t = outTarget ? *outTarget : dummyTarget;
		
		if ((mflags & FOB_INCL_TARGET) != 0) {
			copy_into_long(&(t.target), buf, swap);			buf += 4;
			if (t.target == PREFERRED_TOKEN) {
				t.target = NO_TOKEN;
				t.flags |= MTF_PREFERRED_TARGET;
			}
			NOISY(DOUT << "BMessageOld: found target=" << t.target << endl);
		}
	
		// If we have a script message then look for the interesting
		// entries as they get rehydrated.
		if (mflags & FOB_SCRIPT_MSG)
			look_for_specifiers = true;
	
		if ((mflags & FOB_INCL_REPLY) != 0) {
			copy_into_long(&(t.reply_port), buf, swap);		buf += 4;
			copy_into_long(&(t.reply_target), buf, swap);	buf += 4;
			copy_into_long(&(t.reply_team), buf, swap);		buf += 4;
			t.flags |= MTF_DELIVERED;
			if (*buf) t.flags |= MTF_PREFERRED_REPLY;		buf += 1;
			if (*buf) t.flags |= MTF_SOURCE_IS_WAITING;		buf += 1;
			if (*buf) t.flags |= MTF_REPLY_SENT;			buf += 1;
			if (*buf) t.flags |= MTF_IS_REPLY;				buf += 1;
			NOISY(DOUT << "BMessageOld: found reply port=" << t.reply_port
				<< ", target=" << t.reply_target << ", team=" << t.reply_team
				<< ", flags=" << t.flags << endl);
		}
	}

	// done with header. Calc a new check_sum of actual data and see if
	// it matches what was saved
	hdr_size = buf-(const unsigned char*)buffer;
	ncheck_sum = _checksum_((const uchar*)buffer, hdr_size);

	if (ncheck_sum != ocheck_sum) {
#if SUPPORTS_STREAM_IO
		DOUT << "BMessageOld: checksum mismatch n=" << ncheck_sum
				<< ", o=" << ocheck_sum << endl;
#endif
		goto bad_data;
	}

	while (1) {
		uchar	flags;
		uint32	type;
		bool	is_fixed_size;
		bool	single_item;
		int32	count;
		int32	chunk_size;
		int32	logical;
		uchar	name_length;

		NOISY(DOUT << "Getting next field starting at "
			 << (void*)(buf-start+4) << ": "
			 << BHexDump(buf, (expected_end-buf), 16, "\t") << endl);
		
		if (buf+1 > expected_end) {
#if SUPPORTS_STREAM_IO
			DOUT << "BMessageOld: Field start past end of data" << endl;
#endif
			goto bad_data;
		}

		flags = *buf;										buf += 1;
		NOISY(DOUT << "Got field flags: " << (int32)flags
			 << ", next is " << (void*)(buf-start+4) << endl);
		if (flags == END_OF_ENTRIES)
			break;

		// Create a new message body into which we will add the data.
		if (!msg) {
			msg = BMessageBody::Create();
			if (!msg) {
				error = B_NO_MEMORY;
				goto other_error;
			}
			msg->Acquire();
			NOISY(DOUT << "BMessageOld: created body=" << msg << endl);
		}
		
		if (buf+4 > expected_end) {
#if SUPPORTS_STREAM_IO
			DOUT << "BMessageOld: Field type past end of data" << endl;
#endif
			goto bad_data;
		}

		copy_into_long((long*) &type, buf, swap);			buf += 4;
		NOISY(DOUT << "Got field type: " << (int32)type
			 << ", next is " << (void*)(buf-start+4) << endl);

		is_fixed_size = ((flags & FOB_FIXED_SIZE) != 0);
		single_item = ((flags & FOB_SINGLE_ITEM) != 0);

		if ((flags & FOB_MINI_DATA) != 0) {
			if (single_item) {
				count = 1;
			} else {
				if (buf+1 > expected_end) {
#if SUPPORTS_STREAM_IO
					DOUT << "BMessageOld: Mini count past end of data" << endl;
#endif
					goto bad_data;
				}
				count = *buf;								buf += 1;
			}
			if (buf+1 > expected_end) {
#if SUPPORTS_STREAM_IO
				DOUT << "BMessageOld: Mini size past end of data" << endl;
#endif
				goto bad_data;
			}
			logical = *buf;									buf += 1;
		} else {
			if (single_item) {
				count = 1;
			} else {
				if (buf+4 > expected_end) {
#if SUPPORTS_STREAM_IO
					DOUT << "BMessageOld: Data count past end of data" << endl;
#endif
					goto bad_data;
				}
				copy_into_long((long*) &count, buf, swap);	buf += 4;
			}
			if (buf+4 > expected_end) {
#if SUPPORTS_STREAM_IO
				DOUT << "BMessageOld: Data size past end of data" << endl;
#endif
				goto bad_data;
			}
			copy_into_long((long*) &logical, buf, swap);	buf += 4;
		}
		
		if (buf+1 > expected_end) {
#if SUPPORTS_STREAM_IO
			DOUT << "BMessageOld: Name length past end of data" << endl;
#endif
			goto bad_data;
		}
		name_length = *buf;									buf += 1;
		NOISY(DOUT << "Got name length: " << (int32)name_length
			 << ", next is " << (void*)(buf-start+4) << endl);

		if ((buf + logical + name_length > expected_end) || (logical < 0)) {
//+			SERIAL_PRINT(("BD: %.4s,%.4s (%x,%x): s=%x, chk=%x, nlen=%x, dlen=%x, f=%x\n",
//+				 (char*) &what, (char*) &type, what, type,
//+				 size, chunk_size, name_length, logical, flags));
#if SUPPORTS_STREAM_IO
			DOUT << "BMessageOld: Field items past end of data" << endl;
#endif
			goto bad_data;
		}
		if ((count <= 0) || (logical < count)) {
#if SUPPORTS_STREAM_IO
			DOUT << "BMessageOld: Bad value, count=" << count
				 << ", logical=" << logical << endl;
#endif
			goto bad_data;
		}

//+		PRINT(("	count=%d, type=%.4s, logical=%d, is_fixed=%d, namelen=%d\n",
//+			count, (char *) &type, logical, is_fixed_size, name_length));

		if (is_fixed_size)
			chunk_size = logical / count;
		else
			chunk_size = 0;

		BString name;
		name.SetTo((const char*)buf, name_length);			buf += name_length;
		BMessageBody::data_info info;
		info.name = name.String();
		info.type = type;
		
		// This is the easiest may to make sure data is aligned.
		if (logical < 0)
			goto bad_data;
		dataBuffer = (unsigned char*)realloc(dataBuffer, logical);
		if (!dataBuffer) {
#if SUPPORTS_STREAM_IO
			DOUT << "BMessageOld: Out of memory for data buffer" << endl;
#endif
			error = B_NO_MEMORY;
			goto other_error;
		}
		memcpy(dataBuffer, buf, logical);					buf += logical;
		
		if (!is_fixed_size) {
			var_chunk* vc = (var_chunk*)dataBuffer;
			for (int32 i=0; i<count; i++, vc = da_next_chunk(vc)) {
			
				if ((unsigned char*)vc < dataBuffer ||
						(unsigned char*)vc >= (dataBuffer+logical)) {
#if SUPPORTS_STREAM_IO
					DOUT << "BMessageOld: Bad format of variable-length array "
						 << info.name << endl;
#endif
					D(DOUT << "Message: " << BHexDump(buffer, size-8) << endl);
					D(DOUT << "Field: " << BHexDump(dataBuffer, logical) << endl);
					D(DOUT << "Currently at #" << i << " (" << vc << "), start="
						 << dataBuffer << endl);
					goto bad_data;
				}
				
				if (swap) {
					vc->fDataSize = B_BYTE_SWAP_INT32(vc->fDataSize);
				}
				
				info.size = vc->fDataSize;
				info.data = vc->fData;
				if (swap && info.type == B_REF_TYPE) {
					entry_ref_swap((char*)info.data, info.size);
				}
				NOISY(DOUT << "BMessageOld: adding variable size name=" << info.name
					 << ", type=" << info.type << ", " << info.size << " bytes: "
					 << BHexDump(info.data, info.size, 16, "\t") << endl);
				msg = msg->AddData(info.name, &info, false, &error);
				if (error != B_OK) {
#if SUPPORTS_STREAM_IO
					DOUT << "BMessageOld: Error returned by AddData()" << endl;
#endif
					goto other_error;
				}
				
			}
		} else {
			info.size = chunk_size;
			if (swap) {
				swap_data(info.type, dataBuffer, count*chunk_size, B_SWAP_ALWAYS);
			}
			bool isWhen = (info.type == B_INT64_TYPE &&
							strcmp(info.name, "when") == 0 &&
							info.size == sizeof(int64));
			if (isWhen && count == 1) {
				memcpy(&(outArgs->when), dataBuffer, sizeof(int64));
				outArgs->has_when = true;
			} else {
				for (int32 i=0; i<count; i++) {
					info.data = dataBuffer + i*chunk_size;
					NOISY(DOUT << "BMessageOld: adding fixed size name=" << info.name
						 << ", type=" << info.type << ", " << info.size << " bytes: "
						 << BHexDump(info.data, info.size, 16, "\t") << endl);
					msg = msg->AddData(info.name, &info, true, &error);
					if (error != B_OK) {
#if SUPPORTS_STREAM_IO
						DOUT << "BMessageOld: Error returned by AddData()" << endl;
#endif
						goto other_error;
					}
				}
			}
		}
	}

	if (look_for_specifiers && msg) {
		BMessageBody::data_info info;
		if (msg->FindData(B_SPECIFIER_ENTRY, &info) == B_OK) {
			atomic_or(&(msg->fHeader.fFlags), BMessageBody::MBF_HAS_SPECIFIERS);
		}
		if (msg->CopyData(_CUR_SPECIFIER_ENTRY_, B_INT32_TYPE, 0,
						  &(outArgs->cur_specifier), sizeof(int32)) != B_OK) {
			outArgs->cur_specifier = -1;
		} else {
			msg = msg->FreeData(_CUR_SPECIFIER_ENTRY_, -1, outResult);
		}
	}
	
//+	ASSERT((buf - start) == size);
	if ((buf - start + 12) != size) {
#if SUPPORTS_STREAM_IO
		DOUT << "BMessageOld: expected size=" << size
			 << ", got=" << (buf-start+12) << endl;
#endif
		goto bad_data;
	}

	free(dataBuffer);
	free(buffer);
	
	*outResult = B_OK;
	return msg;

bad_data:
	error = B_NOT_A_MESSAGE;
other_error:

	*outResult = error < B_OK ? error : B_NOT_A_MESSAGE;
	
#if SUPPORTS_STREAM_IO
	DOUT << "BMessageOld: error unflattening old message: "
			<< strerror(*outResult) << endl;
#endif
			
	free(dataBuffer);
	if (msg) msg->Release();
	free(buffer);
	return NULL;
}

/* --------------------------------------------------------------------- */

static inline int32 da_pad_8(int32 val) { return (val + 7) & ~7; }

static ssize_t flatten_old_field(uint8* data, BMessageBody::data_info info,
								 data_off cookie, const BMessageBody* body)
{
	ssize_t pos = 0;
	ssize_t error;
	
	// Write this field's header.
	uint8 flags = FOB_VALID;
	if (info.count == 1) flags |= FOB_SINGLE_ITEM;
	if (info.size != NULL_SIZE) flags |= FOB_FIXED_SIZE;
	if (data) {
		*(data+pos) = flags;
		*(type_code*)(data+pos+1) = info.type;
	}
	pos += sizeof(flags)+sizeof(info.type);
	
	if (!(flags&FOB_SINGLE_ITEM)) {
		if (data) *(size_t*)(data+pos) = info.count;
		pos += sizeof(info.count);
	}
	
	// This is where we write the total number of bytes of data.
	// We'll go back and actually write it at the end.
	const ssize_t logicalPos = pos;
	pos += sizeof(size_t);
	
	const uint8 nameLen = strlen(info.name);
	if (data) {
		*(data+pos) = nameLen;
		memcpy(data+pos+1, info.name, nameLen);
	}
	pos += nameLen + 1;
	
	// This is where the actual data starts.
	const ssize_t dataPos = pos;
	
	// For every index in each field...
	for (size_t i=0; i<info.count; i++) {
	
		// If this is an array, we need to retrieve the actual data
		// for the current index.  Otherwise, we can just directly use
		// whatever was in the original data_info struct.  (Doing this
		// check allows us to write non-array data items that aren't
		// actually in a message.)
		if (info.count > 1) {
			data_off tempCookie = cookie;
			error = body->FindData(NULL, B_ANY_TYPE, 0, &info, i, &tempCookie);
			if (error < B_OK) {
				TRESPASS();
				return error;
			}
		}
		
		if (info.type == B_MESSAGE_TYPE) {
			// Need to recursively convert the embedded message.
			BMessage msg;
			info.size = 0;		// in case of error
			error = msg.Unflatten((const char*)info.data);
			if (error < B_OK) {
				TRESPASS();
				return error;
			}
			error = msg.FlattenedSize(B_MESSAGE_VERSION_1);
			if (error < B_OK) {
				TRESPASS();
				return error;
			}
			info.size = (size_t)error;
			if (!(flags&FOB_FIXED_SIZE)) {
				if (data) *(size_t*)(data+pos) = info.size;
				pos += sizeof(size_t);
			}
			if (data) {
				error = msg.Flatten(B_MESSAGE_VERSION_1, (char*)data+pos, info.size);
				if (error < B_OK) {
				TRESPASS();
					return error;
				}
			}
			pos += info.size;
		} else {
			// Can directly write this field's data.
			if (!(flags&FOB_FIXED_SIZE)) {
				if (data) *(size_t*)(data+pos) = info.size;
				pos += sizeof(size_t);
			}
			if (data) memcpy(data+pos, info.data, info.size);
			pos += info.size;
		}

		if (!(flags&FOB_FIXED_SIZE)) {
			// Variable length fields need to be aligned on
			// 8-byte boundaries.
			int32 pad = da_pad_8(info.size+sizeof(info.size))
					  - info.size - sizeof(info.size);
			if (pad > 0) {
				if (data) memset(data+pos, 0, pad);
				pos += pad;
			}
		}
	}
	
	// Now write the total data size back into the header.
	if (data) *(ssize_t*)(data+logicalPos) = (pos - dataPos);
	
	return pos;
}

static ssize_t flatten_old_field(uint8* data, const char* name, type_code type,
								 const void* fieldData, size_t fieldSize)
{
	BMessageBody::data_info info;
	info.pos = 0;
	info.name = name;
	info.type = type;
	info.count = 1;
	info.size = fieldSize;
	info.data = fieldData;
	return flatten_old_field(data, info, 0, NULL);
}

static ssize_t flatten_old_data(uint8* data, const BMessageBody* body)
{
	ssize_t pos = 0;
	
	if (body) {
		BMessageBody::data_info info;
		data_off cookie = 0;
		data_off lastCookie = cookie;
		
		// For every field in the message...
		while (body->FindData(NULL, B_ANY_TYPE, 0, &info, -1, &cookie) == B_OK) {
		
			ssize_t amount = flatten_old_field(data ? (data+pos) : NULL,
											   info, lastCookie, body);
			if (amount < B_OK) {
				TRESPASS();
				return amount;
			}
			
			pos += amount;
			lastCookie = cookie;
		}
	}
	
	return pos;
}

ssize_t BPrivate::flatten_old_message(	uint8* data,
										const BMessageBody* body,
										const header_args* args)
{
	ssize_t pos = 0;
	ssize_t amount;
	uint8 mflags = 0;
	
#ifdef B_HOST_IS_BENDIAN
	mflags |= FOB_BIG_ENDIAN;
#endif

	if (args->cur_specifier >= 0) {
		// If we're a scripting msg then give a hint by setting this flag.
		mflags |= FOB_SCRIPT_MSG;
	}

	if (data) *(uint32*)(data+pos) = FOB_WHAT;
	pos += sizeof(uint32);
	
	// This is where the checksum will be stored.
	const ssize_t checksumPos = pos;
	pos += sizeof(uint32);
	
	// This is where the size will be stored.
	const ssize_t sizePos = pos;
	pos += sizeof(size_t);
	if (data) *(uint32*)(data+pos) = args->what;
	pos += sizeof(uint32);
	if (data) *(data+pos) = mflags;
	pos++;
	
	// This is where the actual data starts.
	const ssize_t dataPos = pos;
	
	// If there is a timestamp in the message, write this as an
	// old 'when' field.
	if (args->has_when) {
		amount = flatten_old_field(data ? (data+pos) : NULL, "when",
								   B_INT64_TYPE, &args->when, sizeof(bigtime_t));
		if (amount < B_OK) return amount;
		pos += amount;
	}
	
	// If there is a specifier in the message, write this as an
	// old _CUR_SPECIFIER_ENTRY_ field.
	if (args->cur_specifier >= 0) {
		amount = flatten_old_field(data ? (data+pos) : NULL, _CUR_SPECIFIER_ENTRY_,
								   B_INT32_TYPE, &args->cur_specifier, sizeof(int32));
		if (amount < B_OK) return amount;
		pos += amount;
	}
	
	// Now write the actual body data.
	amount = flatten_old_data(data ? (data+pos) : NULL, body);
	if (amount < B_OK) return amount;
	pos += amount;
	
	// That's it!
	if (data) *(data+pos) = END_OF_ENTRIES;
	pos++;
	
	// Write the total size back up into the header.
	if (data) *(ssize_t*)(data+sizePos) = pos;
	
	// And finally compute and write the check_sum.
	if (data) {
		*(ssize_t*)(data+checksumPos) = _checksum_(data+sizePos, dataPos-sizePos);
	}
	
	return pos;
}
