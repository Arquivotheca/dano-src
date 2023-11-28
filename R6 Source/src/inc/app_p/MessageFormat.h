/******************************************************************************
/
/	File:			MessageFormat.h
/
/	Description:	Definition of BMessage format.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MESSAGE_FORMAT_H
#define _MESSAGE_FORMAT_H

#include <BeBuild.h>
#include <OS.h>
#include <SupportDefs.h>

#if defined(__cplusplus)
#include <token.h>
#endif

#if defined(__cplusplus)
namespace BPrivate {
#endif

// ---------------------------------------------------------------------------

typedef size_t data_off;

static const size_t NULL_SIZE = 0xffffffff;
static const size_t NULL_OFF = 0xffffffff;

	// for update messages sent by the app_server
#define UPDATE_MSG_CODE			0	

	// for standard messages sent by the app_server
#define APP_SERVER_MSG_CODE		1

	// the standard for sending messages via messengers.
#define STD_SEND_MSG_CODE		'pjpp'

	// for standard mini messages ('what' field only)
#define STD_MINI_SEND_MSG_CODE	'jahh'


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

enum {
	MESSAGE_SIGNATURE = 'FOB2',
	SWAPPED_MESSAGE_SIGNATURE = '2BOF'
};

// This is what appears at the front of a flattened message.
struct message_header {
	uint32 signature;
	uint32 size;
	uint32 what;
	uint32 reserved0;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

// Standard chunk codes.
enum {
	OFFSETS_CODE		= 'of',		// Chunks that appear in the header
	SCRIPT_CODE			= 'sc',
	WHEN_CODE			= 'wh',
	TARGET_CODE			= 'tr',
	PAD_CODE			= 'pa',
	
	DATA_CODE			= 'Da',		// Chunks that appear in the body
	INDEX_CODE			= 'In',
	END_CODE			= 'En'
};

// These are generic subcodes for chunks that don't care about them.
enum {
	OFFSETS_SUBCODE		= 'ST',
	SCRIPT_SUBCODE		= 'RT',
	WHEN_SUBCODE		= 'EN',
	TARGET_SUBCODE		= 'GT',
	PAD_SUBCODE			= 'DD',
	INDEX_SUBCODE		= 'DX',
	END_SUBCODE			= 'DD'
};

#if defined(__cplusplus)
static inline uint32 chunk_align(const uint32 size) { return ((size+0x7)&~0x7); }
#endif

struct chunk_header {
	uint16 code;			// kind of data in this chunk
							// if the first character is lower case this is
							// a header chunk
	uint16 subcode;			// specific type for chunk
	uint32 size;			// total size of chunk, including this header
#if defined(__cplusplus)
	inline const chunk_header* next() const	{ return (const chunk_header*)(((const uint8*)this)+chunk_align(size)); }
	inline chunk_header* next()				{ return (chunk_header*)(((uint8*)this)+chunk_align(size)); }
#endif
};

// ---------------------------------------------------------------------------

struct offsets_chunk {
	struct chunk_header header;
	uint32 data_end;
	uint32 body_end;
	int32 atom_count;
	uint32 reserved0;
};

// ---------------------------------------------------------------------------

struct script_chunk {
	struct chunk_header header;
	int32 cur_specifier;
	int32 reserved0;
};

// ---------------------------------------------------------------------------

struct when_chunk {
	struct chunk_header header;
	bigtime_t when;
};

// ---------------------------------------------------------------------------

enum {
	MTF_PREFERRED_TARGET	= (1<<31),	// target is preferred handler
	
	MTF_DELIVERED			= (1<<30),	// reply information is valid
	MTF_PREFERRED_REPLY		= (1<<29),	// reply to preferred handler
	MTF_SOURCE_IS_WAITING	= (1<<28),	// sender is waiting for reply
	MTF_REPLY_REQUIRED		= (1<<27),	// sender requires a reply
	MTF_REPLY_SENT			= (1<<26),	// a reply has been sent
	MTF_IS_REPLY			= (1<<25),	// this is a reply to another message
	
	MTF_MASK				= MTF_PREFERRED_TARGET
							| MTF_DELIVERED
							| MTF_PREFERRED_REPLY
							| MTF_SOURCE_IS_WAITING
							| MTF_REPLY_REQUIRED
							| MTF_REPLY_SENT
							| MTF_IS_REPLY
};

// note: this struct is included as-is in messages, so it probably
// should not change  (except it is only included in messages sent
// through ports, so maybe it can change...)
struct message_target {
	int32 target;			// who message is being set to
	int32 reply_port;		// reply information for received message
	int32 reply_target;		// token or NO_TOKEN
	int32 reply_team;
	uint32 flags;			// MTF_* flags above
	int32 reserved0;		// pad to 8-byte boundary
	
#if defined(__cplusplus)
	inline message_target()
		: target(NO_TOKEN),
		  reply_port(-1), reply_target(NO_TOKEN), reply_team(-1),
		  flags(0), reserved0(0)
	{
	}
#endif
};

struct target_chunk {
	struct chunk_header header;
	struct message_target target;
};

// ---------------------------------------------------------------------------

// This is the size for the message header and all possible header
// chunks.
enum {
	MAX_HEADER_SIZE = sizeof(struct message_header)	// signature
					+ sizeof(struct offsets_chunk)	// offsets
					+ sizeof(struct script_chunk)	// script
					+ sizeof(struct when_chunk)		// timestamp
					+ sizeof(struct target_chunk)	// target
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

// Standard data chunk subcodes.
enum {
	SINGLE_DATA_SUBCODE		= 'SG',
	FIXED_ARRAY_SUBCODE		= 'FA',
	VARIABLE_ARRAY_SUBCODE	= 'VA',
	MESSAGE_REF_SUBCODE		= 'MS'
};

enum {
	kDataChunkSize = sizeof(struct chunk_header)+10
};

struct data_chunk {
	struct chunk_header header;
	type_code type;			// type of data in chunk
	uint32 entry_size;		// chunks per data entry, or 0 if variable_array
	uint8 name_length;		// length of entry name, excluding \0-terminator
	char name[1];			// name goes here, \0-terminated
	// data structure goes here, chunk-aligned
#if defined(__cplusplus)
	inline void* data_struct()				{ return ((uint8*)this) + chunk_align(kDataChunkSize + name_length); }
	inline const void* data_struct() const	{ return ((uint8*)this) + chunk_align(kDataChunkSize + name_length); }
#endif
};

struct fixed_array {
	uint32 count;				// number of entries in the array
	uint32 reserved0;			// always set to zero
	// 'size' bytes of data goes here, 'count' times, total is chunk-aligned
#if defined(__cplusplus)
	inline uint8* data(size_t size, uint32 i)				{ return ((uint8*)this) + sizeof(fixed_array) + i*size; }
	inline const uint8* data(size_t size, uint32 i) const	{ return ((uint8*)this) + sizeof(fixed_array) + i*size; }
#endif
};

struct variable_array {
	uint32 count;				// number of entries in the array
	uint32 total_size;			// offset past data to index
	// array of chunk-aligned data goes here, all followed by an
	// array of data_off indicies containing the -end- of each entry
#if defined(__cplusplus)
	inline uint8* data() {
		return (uint8*)(this+1);
	}
	inline const uint8* data() const {
		return (const uint8*)(this+1);
	}
	inline uint8* data(uint32* outSize, uint32 i) {
		uint8* base = data();
		data_off pos = (i==0 ? 0 : chunk_align(((data_off*)(base+total_size))[i-1]));
		data_off end = ((data_off*)(base+total_size))[i];
		*outSize = end-pos;
		return base + pos;
	}
	inline const uint8* data(uint32* outSize, uint32 i) const {
		const uint8* base = data();
		data_off pos = (i==0 ? 0 : chunk_align(((const data_off*)(base+total_size))[i-1]));
		data_off end = ((const data_off*)(base+total_size))[i];
		*outSize = end-pos;
		return base + pos;
	}
#endif
};

// ---------------------------------------------------------------------------

#if defined(__cplusplus)
}

using namespace BPrivate;
#endif

#endif /* _MESSAGE_FORMAT_H */

