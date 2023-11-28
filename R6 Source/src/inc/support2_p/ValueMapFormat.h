/******************************************************************************
/
/	File:			ValueMapFormat.h
/
/	Description:	Definition of BValueMap format.
/
/	Copyright 2001, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _SUPPORT2_VALUEMAPFORMAT_H
#define _SUPPORT2_VALUEMAPFORMAT_H

#include <support2/SupportDefs.h>

#include <kernel/OS.h>

#if defined(__cplusplus)
#include <support2/TokenSpace.h>
namespace B {
namespace Support2 {
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
	VALUE_MAP_SIGNATURE = 'VMAP',
	SWAPPED_VALUE_MAP_SIGNATURE = 'PAMV'
};

// This is what appears at the front of a flattened value map.
struct value_map_header {
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
	// Chunks that appear in the header
	OFFSETS_CODE		= 'of',		// Location of important chunks
	WHEN_CODE			= 'wh',		// Time stamp
	PAD_CODE			= 'pa',		// Whitespace
	
	// Chunks that appear in the body
	DATA_CODE			= 'Da',		// A key/value pair
	INDEX_CODE			= 'In',		// Sorted index of data chunks
	END_CODE			= 'En'		// End of value map
};

#if defined(__cplusplus)
static inline uint32 chunk_align(const uint32 size) { return ((size+0x7)&~0x7); }
#endif

struct chunk_header {
	uint16 code;			// kind of data in this chunk
							// if the first character is lower case this is
							// a header chunk
	uint16 reserved;		// always set to zero
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
	int32 binder_count;
};

// ---------------------------------------------------------------------------

struct when_chunk {
	struct chunk_header header;
	bigtime_t when;
};

// ---------------------------------------------------------------------------

// This is the size for the message header and all possible header
// chunks.
enum {
	CORE_HEADER_SIZE	= sizeof(struct value_map_header)	// signature
						+ sizeof(struct offsets_chunk)		// offsets
						+ sizeof(struct when_chunk)			// timestamp
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

struct map_item_info {
	type_code type;			// type-code of data
	uint32 length;			// number of bytes of data
	// actual data follows here...
};

struct data_chunk {
	struct chunk_header header;
	struct map_item_info key;
	struct map_item_info value;
	// Key data and then value data follow, each chunk aligned.
#if defined(__cplusplus)
	inline void* key_data()							{ return ((uint8*)this) + sizeof(header) + sizeof(key) + sizeof(value); }
	inline const void* key_data() const				{ return ((uint8*)this) + sizeof(header) + sizeof(key) + sizeof(value); }
	inline void* value_data()						{ return ((uint8*)this) + sizeof(header) + sizeof(key) + sizeof(value) + chunk_align(key.length); }
	inline const void* value_data() const			{ return ((uint8*)this) + sizeof(header) + sizeof(key) + sizeof(value) + chunk_align(key.length); }
#endif
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

#if defined(__cplusplus)
} }	// namespace B::Support2
#endif

#endif /* _SUPPORT2_VALUEMAPFORMAT_H */
