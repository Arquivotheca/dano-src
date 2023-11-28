//*****************************************************************************
//
//	File:		MessageBody.cpp
//
//	
//	Written by:	Dianne Hackborn (though I'll never admit to it)
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#include "MessageBody.h"

#include <atomic.h>
#include <Atom.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <Entry.h>
#include <OS.h>
#include <StreamIO.h>
#include <StringIO.h>

#include <CallStack.h>
#include <message_util.h>
#include <token.h>

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <new>

#if DEBUG
#define DB_STINLINE 
#define DB_INLINE 
#else
#define DB_STINLINE static inline
#define DB_INLINE inline
#endif

// Turn this on to print brief messages when port errors
// occur.  This can be useful to see when message queues get
// filled up and other problems, but for production should
// probably be turned off.
#define PORT_ERRORS DEBUG

// Turn this on to print more extensive messages when errors
// occur, usually in the form of stack crawls or data dumps.
#define VERBOSE_ERRORS DEBUG

// Use this to print error messages when parsing messages.  This
// is normally turned on, because these kinds of errors are rare
// and could be useful for a third party developer to see.
#if SUPPORTS_STREAM_IO
#define DPARSE(x) x
#else
#define DPARSE(x)
#endif

// This is where messages are written.
#define DOUT BErr
//#define DOUT BSer

// Turn this on to leak check message body objects.
//#define CHECK_LEAKS 1

// Turn this on for general code tracing.
//#define D DEBUG_ONLY
#define D(x)

// Turn this on for port operation tracing.
//#define DPORT(x) x
//#define DPORT DEBUG_ONLY
#define DPORT(x)

// Turn this on for data stream tracing.
//#define DSTREAM DEBUG_ONLY
#define DSTREAM(x)

// Turn this on for atom tracing.
//#define DATOM(x) x
//#define DATOM DEBUG_ONLY
#define DATOM(x)

// Use these to track atom reference counting.
//#define REF_ATOM(a, op) { int32 c = a->op(); DOUT << #op ": " << a << " from count " << c << endl; }
#define REF_ATOM(a, op, tag) a->op(tag)
//#define BUMP_ATOM(count, delta) { count += delta; DOUT << "Bumped atom count by " << delta << " to " << count << endl; }
#define BUMP_ATOM(count, delta) count += delta

static inline bool cmp_types(type_code t1, type_code t2)
{
	return (t1==t2) || (t1==B_ANY_TYPE) || (t2==B_ANY_TYPE);
}

static const char* PREFIX_TEXT = "\033[35mBMessageBody\033[0m";

// optimization for gcc
#if __GNUC__
#define DECLARE_RETURN(x) return x
#else
#define DECLARE_RETURN(x)
#endif

namespace BPrivate {

// ---------------------------------------------------------------------------

DB_STINLINE void chunk_pad(void* const pos)
{
	static const uint8 masks[][8] = {
		{ 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00 },
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 },
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 }
	};
	const size_t used = ((size_t)pos) & 0x7;
	if (used != 0) *((uint64*)((size_t)pos&~0x7)) &= *((uint64*)(masks[used-1]));
}

DB_STINLINE bool is_header_chunk(const uint8* const chunk)
{
	return (((const chunk_header*)chunk)->code) & 0x2000;
}

DB_STINLINE uint8* next_chunk(uint8* const cur)
{
	return cur + chunk_align((((const chunk_header*)cur)->size));
}

DB_STINLINE const uint8* next_chunk(const uint8* const cur)
{
	return cur + chunk_align((((const chunk_header*)cur)->size));
}

DB_STINLINE const data_off next_chunk_off(const uint8* const base, const data_off pos)
{
	return pos + chunk_align((((const chunk_header*)(base+pos))->size));
}

// ---------------------------------------------------------------------------

static const chunk_header gOffsetsChunkHeader = {
	OFFSETS_CODE, OFFSETS_SUBCODE, sizeof(offsets_chunk)
};

static const offsets_chunk gOffsetsPadChunk = {
	{ PAD_CODE, PAD_SUBCODE, sizeof(offsets_chunk) },
	0, 0, 0, 0
};

// ---------------------------------------------------------------------------

static const chunk_header gScriptChunkHeader = {
	SCRIPT_CODE, SCRIPT_SUBCODE, sizeof(script_chunk)
};

static const script_chunk gScriptPadChunk = {
	{ PAD_CODE, PAD_SUBCODE, sizeof(script_chunk) },
	0, 0
};

// ---------------------------------------------------------------------------

static const chunk_header gWhenChunkHeader = {
	WHEN_CODE, WHEN_SUBCODE, sizeof(when_chunk)
};

static const when_chunk gWhenPadChunk = {
	{ PAD_CODE, PAD_SUBCODE, sizeof(when_chunk) },
	0
};

// ---------------------------------------------------------------------------

static const chunk_header gTargetChunkHeader = {
	TARGET_CODE, TARGET_SUBCODE, sizeof(target_chunk)
};

// This is gross, and all because we want a constructor for message_target.
static const struct target_pad_chunk {
	chunk_header header;
	int32 target[sizeof(message_target)/sizeof(int32)];
} gTargetPadChunk = {
	{ PAD_CODE, PAD_SUBCODE, sizeof(target_chunk) },
	{ NO_TOKEN, -1, NO_TOKEN, -1, 0, 0 }
};

// ---------------------------------------------------------------------------

// Common data for the end chunk.
static const chunk_header gEndChunk = {
	END_CODE, END_SUBCODE, sizeof(chunk_header)
};

struct initial_message_chunks {
	struct chunk_header index;
	struct chunk_header end;
};

static const initial_message_chunks gEmptyMessage = {
	{ INDEX_CODE, INDEX_SUBCODE, sizeof(chunk_header) },
	{ END_CODE, END_SUBCODE, sizeof(chunk_header) }
};


// ---------------------------------------------------------------------------

DB_STINLINE data_off chunk_data_off(const uint8* base, data_off chunk)
{
	return chunk + chunk_align(kDataChunkSize
								+ ((const data_chunk*)(base+chunk))->name_length);
}

DB_STINLINE data_off fixed_array_off(const uint8* /*base*/, data_off data, size_t size, uint32 i)
{
	return data + sizeof(fixed_array) + i*size;
}

DB_STINLINE data_off fixed_array_end(const uint8* base, data_off data, size_t size)
{
	return data + sizeof(fixed_array) + size*((fixed_array*)(base+data))->count;
}

DB_STINLINE data_off variable_array_off(size_t* outSize, const uint8* base, data_off data, uint32 i)
{
	const size_t total_size = ((const variable_array*)(base+data))->total_size;
	const data_off index = data+sizeof(variable_array)+total_size;
	data_off pos = (i==0 ? 0 : chunk_align(((data_off*)(base+index))[i-1]));
	data_off end = ((data_off*)(base+index))[i];
	*outSize = end-pos;
	return data + sizeof(variable_array) + pos;
}

DB_STINLINE data_off variable_array_index(const uint8* base, data_off data)
{
	return data + sizeof(variable_array) + ((const variable_array*)(base+data))->total_size;
}

DB_STINLINE data_off variable_array_end(const uint8* base, data_off data)
{
	return data + sizeof(variable_array)
				+ ((const variable_array*)(base+data))->total_size
				+ (((const variable_array*)(base+data))->count * sizeof(data_off));
}

}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

// These are for catching memory corruption problems with purify debug.

#if DEBUG

void   *db_memcpy (void *dst, const void *src, size_t len)
{
	ASSERT(len > 0);
	while (len > 0) {
		*(char*)dst = *(const char*)src;
		dst = ((char*)dst) + 1;
		src = ((const char*)src) + 1;
		len--;
	}
	return dst;
}

void   *db_memmove(void *dst, const void *src, size_t len)
{
	ASSERT(len > 0);
	dst = ((char*)dst) + len-1;
	src = ((const char*)src) + len-1;
	while (len > 0) {
		*(char*)dst = *(const char*)src;
		dst = ((char*)dst) - 1;
		src = ((const char*)src) - 1;
		len--;
	}
	return dst;
}

char   *db_strcpy(char *dst, const char *src)
{
	while (*src) *dst++ = *src++;
	return dst;
}

int     db_strcmp(const char *str1, const char *str2)
{
	while (*str1 && *str2) {
		str1++;
		str2++;
	}
	return ((int)*str1) - ((int)*str2);
}

#else

#define db_memcpy(x, y, z) memcpy(x, y, z)
#define db_memmove(x, y, z) memmove(x, y, z)
#define db_strcpy(x, y) strcpy(x, y)
#define db_strcmp(x, y) strcmp(x, y)

#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace BPrivate {

status_t entry_ref_flatten(char* buffer, size_t* outSize, const entry_ref* ref)
{
	db_memcpy(buffer, &ref->device, sizeof(dev_t));
	db_memcpy(buffer + sizeof(dev_t), &ref->directory, sizeof(ino_t));
	
	size_t l;
	if (ref->name) {
		l = strlen(ref->name) + 1;
		db_memcpy(buffer + sizeof(dev_t) + sizeof(ino_t), ref->name, l);
	} else {
		l = 0;
	}
	
	*outSize = sizeof(dev_t)+sizeof(ino_t)+l;
	return B_OK;
}

status_t entry_ref_unflatten(entry_ref* outRef, const char* buffer, size_t size)
{
	if (size < (sizeof(dev_t) + sizeof(ino_t))) {
		*outRef = entry_ref();
		return B_BAD_VALUE;
	}
	
	db_memcpy(&outRef->device, buffer, sizeof(dev_t));
	buffer += sizeof(dev_t);
	db_memcpy(&outRef->directory, buffer, sizeof(ino_t));
	buffer += sizeof(ino_t);
	
	if (outRef->device != -1 && size > (sizeof(dev_t) + sizeof(ino_t))) {
		outRef->set_name(buffer);
		if (outRef->name == NULL) {
			*outRef = entry_ref();
			return B_NO_MEMORY;
		}
	} else {
		outRef->set_name(NULL);
	}
	
	return B_OK;
}

status_t entry_ref_swap(char* buffer, size_t size)
{
	STATIC_ASSERT(sizeof(dev_t) == sizeof(int32));
	STATIC_ASSERT(sizeof(ino_t) == sizeof(int64));
	
	if (size < (sizeof(dev_t) + sizeof(ino_t))) return B_BAD_DATA;
	
	dev_t	*dev = (dev_t *) buffer;
	*dev = B_BYTE_SWAP_INT32(*dev);

	ino_t	*ino = (ino_t *) (buffer + sizeof(dev_t));
	*ino = B_BYTE_SWAP_INT64(*ino);
	
	return B_OK;
}

}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace BPrivate {

#if CHECK_LEAKS
static int32 gMallocCount = 0;
static int32 gBodyCount = 0;

static void* my_malloc(size_t size)
{
	atomic_add(&gMallocCount, 1);
	void* ptr = malloc(size);
	return ptr;
}

static void my_free(void* ptr)
{
	atomic_add(&gMallocCount, -1);
	free(ptr);
}

static void* my_realloc(void* ptr, size_t size)
{
	void* next = realloc(ptr, size);
	return next;
}

void* (*message_malloc) (size_t size) = my_malloc;
void (*message_free) (void* ptr) = my_free;
void* (*message_realloc) (void* ptr, size_t size) = my_realloc;
#else
void* (*message_malloc) (size_t size) = malloc;
void (*message_free) (void* ptr) = free;
void* (*message_realloc) (void* ptr, size_t size) = realloc;
#endif

class BMessageBodyCache
{
public:
	BMessageBodyCache()
		: fCount(0)
	{
		fCache.first = NULL;
		fCache.sequence = 0;
	}
	~BMessageBodyCache()
	{
		#if CHECK_LEAKS
		int32 remain = 0;
		#endif
		void* next;
		while ((next=atomic_pop(&fCache)) != NULL) {
			message_free(next);
			#if CHECK_LEAKS
			remain++;
			#endif
		}
		
		#if CHECK_LEAKS
		if (gMallocCount || gBodyCount || fCount != remain) {
			BErr << "Message cache: count=" << fCount << ", actual=" << remain << endl;
			BErr << "Leaked mallocs: " << gMallocCount << ", Leaked bodies: "
				<< gBodyCount << endl;
		}
		if (fMessages.CountItems() > 0) {
			BErr << "Leaked " << fMessages.CountItems() << " message bodies:" << endl;
			for (int32 i=0; i<fMessages.CountItems(); i++) {
				BMessageBody* body = (BMessageBody*)fMessages.ItemAt(i);
				BCallStack* stack = (BCallStack*)fStacks.ItemAt(i);
				BErr << "  Body " << body << ":" << endl;
				if (stack) stack->LongPrint(BErr, NULL, "    ");
			}
		}
		#endif
	}
	
	DB_INLINE void* Get(size_t block_size)
	{
		if (block_size != sizeof(BMessageBody)) {
			return message_malloc(block_size);
		}
		
		void* top = atomic_pop(&fCache);
		if (top) {
			atomic_add(&fCount, -1);
			return top;
		}
		
		return message_malloc(sizeof(BMessageBody));
	}
	
	DB_INLINE void Save(void *pointer, size_t block_size)
	{
		if (!pointer) return;
		
		if (block_size != sizeof(BMessageBody) || fCount >= 50) {
			message_free(pointer);
			return;
		}
		
		if (fCount < 10) {
			atomic_add(&fCount, 1);
			atomic_push(&fCache, pointer);
		} else {
			message_free(pointer);
		}
	}

	// This is a cover for calling message_realloc().  It is needed so that
	// when performing leak checking, we can track changes in the address of
	// the message body.
	#if CHECK_LEAKS
	DB_INLINE BMessageBody* Resize(BMessageBody* body, size_t size)
	{
		BAutolock _l(fAccess);
		BMessageBody* next = (BMessageBody*)message_realloc(body, size);
		if (body == next)
			return next;
		const int32 i = fMessages.IndexOf(body);
		if (i < 0) {
			debugger("Message body doesn't exist");
			return next;
		}
		fMessages.ReplaceItem(i, next);
		return next;
	}
	#else
	inline BMessageBody* Resize(BMessageBody* body, size_t size)
	{
		return (BMessageBody*)message_realloc(body, size);
	}
	#endif
	
	// Tell the leak checker that a message body has been constructed.
	#if CHECK_LEAKS
	DB_INLINE void NoteConstruct(BMessageBody* body)
	{
		BAutolock _l(fAccess);
		const int32 i = fMessages.IndexOf(body);
		if (i >= 0) {
			BCallStack* stack = (BCallStack*)fStacks.ItemAt(i);
			BStringIO str;
			str << "Message body " << body << " already exists";
			if (stack) {
				str << endl << "Location of original creation:" << endl;
				stack->LongPrint(str);
			}
			debugger(str.String());
			return;
		}
		atomic_add(&gBodyCount, 1);
		fMessages.AddItem(body);
		BCallStack* stack = new BCallStack;
		stack->Update(0);
		fStacks.AddItem(stack);
	}
	#else
	inline void NoteConstruct(BMessageBody*) { }
	#endif
	
	// Tell the leak checker that a message body has been destroyed.
	#if CHECK_LEAKS
	DB_INLINE void NoteDestroy(BMessageBody* body)
	{
		BAutolock _l(fAccess);
		const int32 i = fMessages.IndexOf(body);
		if (i < 0) {
			debugger("Message body doesn't exist");
			return;
		}
		atomic_add(&gBodyCount, -1);
		fMessages.RemoveItem(i);
		BCallStack* stack = (BCallStack*)fStacks.ItemAt(i);
		fStacks.RemoveItem(i);
		delete stack;
	}
	#else
	inline void NoteDestroy(BMessageBody*) { }
	#endif
	
private:
	int32 fCount;
	atomic_list_head fCache;
	
#if CHECK_LEAKS
	BLocker fAccess;
	BList fMessages;
	BList fStacks;
#endif
};

static BMessageBodyCache cache;

}	// namespace BPrivate

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

// This is a little faster than the BMemoryIO class, and has
// a magical "buffer concatenation" mechanism.

BMemoryStreamIO::BMemoryStreamIO(const void *p, size_t len, BDataIO* remaining)
	: fPosition(p), fLength(len), fRemaining(remaining)
{
}

BMemoryStreamIO::~BMemoryStreamIO()
{
}

ssize_t BMemoryStreamIO::Read(void *buffer, size_t size)
{
	DSTREAM(DOUT << "BMemoryStreamIO: Reading " << size
				 << " bytes into " << buffer << endl);
	if (size > fLength) {
		size_t baseSize = fLength;
		if (fLength > 0) {
			DSTREAM(DOUT << "BMemoryStreamIO: copying " << fLength << " existing bytes: ");
			DSTREAM(DOUT << BHexDump(fPosition, fLength, 16, "\t") << endl);
			memcpy(buffer, fPosition, fLength);
			size -= fLength;
			fLength = 0;
		}
		if (size > 0 && fRemaining) {
			ssize_t res = fRemaining->Read(((char*)buffer)+baseSize, size);
			if (res < B_OK) return res;
			DSTREAM(DOUT << "BMemoryStreamIO: read " << size << " bytes: ");
			DSTREAM(DOUT << BHexDump(((char*)buffer)+baseSize, size, 16, "\t") << endl);
			baseSize += res;
		}
		return baseSize;
	}
	
	DSTREAM(DOUT << "BMemoryStreamIO: copying " << size << " of " << fLength
				 << " existing bytes: ");
	DSTREAM(DOUT << BHexDump(fPosition, size, 16, "\t") << endl);
	
	if (size > 0) memcpy(buffer, fPosition, size);
	fPosition = ((const char*)fPosition) + size;
	fLength -= size;
	return size;
}

ssize_t BMemoryStreamIO::Write(const void */*buffer*/, size_t /*size*/)
{
	return EPERM;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

DB_INLINE
void BMessageBody::InitMetaData()
{
	fHeader.init_metadata();
}

DB_INLINE
void BMessageBody::InitData()
{
	fHeader.init_all();
	
	// Starts with the index chunk followed by the terminator.
	*(initial_message_chunks*)fBody = gEmptyMessage;
	fHeader.fBodyEnd = sizeof(gEmptyMessage.index);
	fBodySize = sizeof(gEmptyMessage);
}

DB_INLINE
status_t BMessageBody::CreateHeader(uint8* buffer, size_t* outSize,
									const header_args* args,
									const message_target* target, bool fixedSize) const
{
	uint8* p = buffer;
	((message_header*)p)->signature = MESSAGE_SIGNATURE;
	if (fixedSize) ((message_header*)p)->size = FixedFlatSize();
	else ((message_header*)p)->size = CalcFlatSize(args, target ? true : false);
	((message_header*)p)->what = args->what;
	((message_header*)p)->reserved0 = 0;
	p += sizeof(message_header);
	
	if (this) {
		*(chunk_header*)p = gOffsetsChunkHeader;
		((offsets_chunk*)p)->data_end = fHeader.fDataEnd;
		((offsets_chunk*)p)->body_end = fHeader.fBodyEnd;
		((offsets_chunk*)p)->atom_count = fHeader.fAtomCount;
		((offsets_chunk*)p)->reserved0 = 0;
		p += sizeof(offsets_chunk);
	} else if (fixedSize) {
		*(offsets_chunk*)p = gOffsetsPadChunk;
		p += sizeof(offsets_chunk);
	}
	
	if (HasSpecifiers()) {
		*(chunk_header*)p = gScriptChunkHeader;
		((script_chunk*)p)->cur_specifier = args->cur_specifier;
		((script_chunk*)p)->reserved0 = 0;
		p += sizeof(script_chunk);
	} else if(fixedSize) {
		*(script_chunk*)p = gScriptPadChunk;
		p += sizeof(script_chunk);
	}

	if (args->has_when) {
		*(chunk_header*)p = gWhenChunkHeader;
		((when_chunk*)p)->when = args->when;
		p += sizeof(when_chunk);
	} else if(fixedSize) {
		*(when_chunk*)p = gWhenPadChunk;
		p += sizeof(when_chunk);
	}

	if (target) {
		*(chunk_header*)p = gTargetChunkHeader;
		((target_chunk*)p)->target = *target;
		((target_chunk*)p)->target.reserved0 = 0;
		p += sizeof(target_chunk);
	} else if (fixedSize) {
		*(target_chunk*)p = *(target_chunk*)&gTargetPadChunk;
		p += sizeof(target_chunk);
	}
	
	*outSize = (size_t)(p-buffer);
	return B_OK;
}

DB_INLINE
status_t BMessageBody::ParseHeaderChunk(const chunk_header* chunk,
										const uint8* data,
										header_args* outArgs,
										header_data* outHeader,
										message_target* outTarget, bool swapping)
{
	if (chunk->size < sizeof(chunk_header))
		return B_NOT_A_MESSAGE;
	
	switch (chunk->code) {
		case OFFSETS_CODE: {
			if (chunk->size < (sizeof(offsets_chunk)-sizeof(uint32)*2)) return B_NOT_A_MESSAGE;
			outHeader->fDataEnd = ((offsets_chunk*)data)->data_end;
			outHeader->fBodyEnd = ((offsets_chunk*)data)->body_end;
			if (chunk->size >= sizeof(offsets_chunk)) {
				outHeader->fAtomCount = ((offsets_chunk*)data)->atom_count;
			} else {
				outHeader->fAtomCount = 0;
			}
			if (swapping) {
				outHeader->fDataEnd = B_BYTE_SWAP_INT32(outHeader->fDataEnd);
				outHeader->fBodyEnd = B_BYTE_SWAP_INT32(outHeader->fBodyEnd);
				outHeader->fAtomCount = B_BYTE_SWAP_INT32(outHeader->fAtomCount);
			}
		} break;
		case SCRIPT_CODE: {
			if (chunk->size < sizeof(script_chunk)) return B_NOT_A_MESSAGE;
			atomic_or(&(outHeader->fFlags), MBF_HAS_SPECIFIERS);
			outArgs->cur_specifier = ((script_chunk*)data)->cur_specifier;
			if (swapping) {
				outArgs->cur_specifier = B_BYTE_SWAP_INT32(outArgs->cur_specifier);
			}
		} break;
		case WHEN_CODE: {
			if (chunk->size < sizeof(when_chunk)) return B_NOT_A_MESSAGE;
			outArgs->has_when = true;
			outArgs->when = ((when_chunk*)data)->when;
			if (swapping) {
				outArgs->when = B_BYTE_SWAP_INT64(outArgs->when);
			}
		} break;
		case TARGET_CODE: {
			if (chunk->size < sizeof(target_chunk)) return B_NOT_A_MESSAGE;
			if (outTarget) {
				*outTarget = ((target_chunk*)data)->target;
				if (swapping) {
					outTarget->target = B_BYTE_SWAP_INT32(outTarget->target);
					outTarget->reply_port = B_BYTE_SWAP_INT32(outTarget->reply_port);
					outTarget->reply_target = B_BYTE_SWAP_INT32(outTarget->reply_target);
					outTarget->reply_team = B_BYTE_SWAP_INT32(outTarget->reply_team);
					outTarget->flags = B_BYTE_SWAP_INT32(outTarget->flags);
				}
				// temporary PREFERRED_TOKEN compatibility.
				if (outTarget->target == -2) {
					outTarget->target = NO_TOKEN;
					outTarget->flags |= MTF_PREFERRED_TARGET;
				}
			}
		} break;
		default:
			return 1;
	}
	return B_OK;
}

DB_INLINE
void BMessageBody::OffsetIndicies(data_off insertion, int32 amount)
{
	const int32 N = (((chunk_header*)(fBody+fHeader.fDataEnd))->size
					-sizeof(chunk_header)) / sizeof(data_off);
	data_off* base = (data_off*)(fBody+fHeader.fDataEnd+sizeof(chunk_header));
	for (int32 i=0; i<N; i++) {
		if (base[i] >= insertion) base[i] += amount;
	}
}

DB_INLINE
status_t BMessageBody::MoveIndex(int32 oldPos, int32 newPos)
{
	// This moves 'oldPos' so it appears in the index at 'newPos',
	// AS IF 'oldPos' IS NO LONGER THERE.  This second condition is
	// the reason for all of these strange +/- 1 things everywhere.
	// (And it is here so that we can rename an item by finding its
	// current index, finding the theoretical index of the new name,
	// and call this function with those two values.)
	if (oldPos == newPos || oldPos+1 == newPos) return B_OK;
	
	const int32 N = (((chunk_header*)(fBody+fHeader.fDataEnd))->size
					-sizeof(chunk_header)) / sizeof(data_off);
	data_off* base = (data_off*)(fBody+fHeader.fDataEnd+sizeof(chunk_header));
	if (oldPos < 0 || oldPos >= N) return B_BAD_INDEX;
	
	if ((oldPos+1) < newPos) {
		if (newPos > N) return B_BAD_INDEX;
		data_off tmp = base[oldPos];
		db_memcpy(base+oldPos, base+oldPos+1, (newPos-oldPos-1)*4);
		base[newPos-1] = tmp;
	} else if (oldPos > newPos) {
		if (newPos < 0) return B_BAD_INDEX;
		data_off tmp = base[oldPos];
		db_memmove(base+newPos+1, base+newPos, (oldPos-newPos)*4);
		base[newPos] = tmp;
	}
	
	return B_OK;
}

// ---------------------------------------------------------------------------

DB_INLINE
BMessageBody::BMessageBody(const size_t initialBodySize)
	: fRefCount(0),
	  fBodyAvail(initialBodySize)
{
	cache.NoteConstruct(this);
	D(DOUT << "BMessageBody: constructor called on " << this << endl);
	InitData();
}

DB_INLINE
BMessageBody::~BMessageBody()
{
	D(DOUT << "BMessageBody: destructor called on " << this << endl);
	if (fHeader.fAtomCount > 0) {
		// Need to release references on any atoms.
		DATOM(DOUT << "Deleting message body " << this << " with "
					<< CountAtoms() << " atoms" << endl);
		HandleAtoms(this, AM_DECREMENT);
	}
	cache.NoteDestroy(this);
}

// ---------------------------------------------------------------------------

BMessageBody* BMessageBody::Create()
	DECLARE_RETURN(o)
{
	BMessageBody* o = (BMessageBody*)cache.Get(sizeof(BMessageBody));
	if (!o) return o;
	D(DOUT << "BMessageBody: creating " << o << endl);
	ASSERT(chunk_align((size_t)o) == (size_t)o);
	new (o) BMessageBody(sizeof(((BMessageBody*)NULL)->fBody));
	return o;
}

BMessageBody* BMessageBody::Create(size_t bodySize)
	DECLARE_RETURN(o)
{
	if (bodySize <= sizeof(((BMessageBody*)NULL)->fBody))
		bodySize = sizeof(((BMessageBody*)NULL)->fBody);
	else
		bodySize = chunk_align(bodySize);
	BMessageBody* o = (BMessageBody*)cache.Get(bodySize+sizeof(BMessageBody)-sizeof(((BMessageBody*)NULL)->fBody));
	if (!o) return o;
	D(DOUT << "BMessageBody: creating " << o << endl);
	new (o) BMessageBody(bodySize);
	return o;
}

BMessageBody* BMessageBody::Clone() const
	DECLARE_RETURN(o)
{
	size_t extraSize = fBodySize;
	if (extraSize < sizeof(fBody)) extraSize = 0;
	else extraSize -= sizeof(fBody);
	
	BMessageBody* o = (BMessageBody*)cache.Get(sizeof(BMessageBody) + extraSize);
	if (!o) return o;
	
	D(DOUT << "BMessageBody: cloning " << o << " (from " << this << ")" << endl);
	
	db_memcpy(o, this, sizeof(BMessageBody) + extraSize);
	o->fRefCount = 0;
	atomic_and(&(o->fHeader.fFlags), ~MBF_SENDING);
	o->fBodyAvail = sizeof(fBody) + extraSize;
	o->fBodySize = fBodySize;
	
	if (o->fHeader.fAtomCount > 0) {
		// Need to acquire a new reference on all atoms.
		DATOM(DOUT << "Cloning message " << o << " (from " << this << ") with "
					<< o->CountAtoms() << " atoms" << endl);
		o->HandleAtoms(o, AM_INCREMENT);
	}
	
	cache.NoteConstruct(o);

	return o;
}

void BMessageBody::Release() const
{
	int32 c = atomic_add(&fRefCount, -1);
	if (c == 1) {
		D(DOUT << "BMessageBody: deleting " << this << endl);
		BMessageBody* This = const_cast<BMessageBody*>(this);
		This->BMessageBody::~BMessageBody();
		cache.Save(This, sizeof(*This)-sizeof(This->fBody)+This->fBodyAvail);
	}
	ASSERT( c >= 1 );
}

BMessageBody* BMessageBody::Reset() const
{
	if (!this) return NULL;
	
	if (fRefCount <= 1) {
		// If only one object holds us, it must be the caller, so it
		// is safe to re-use this object.
		BMessageBody* This = const_cast<BMessageBody*>(this);
		if (This->fHeader.fAtomCount > 0) {
			// Need to release references on any atoms.
			DATOM(DOUT << "Resetting message " << this << " with "
						<< This->CountAtoms() << " atoms" << endl);
			This->HandleAtoms(This, AM_DECREMENT);
		}
		This->InitData();
		return This;
	}
	
	Release();
	return NULL;
}

void BMessageBody::BumpAtoms(int32 delta)
{
	BUMP_ATOM(fHeader.fAtomCount, delta);
}

int32 BMessageBody::BumpAtoms(const uint8* data, size_t size, uint32 mode)
{
	const chunk_header* pos = (const chunk_header*)fBody;
	while ((const uint8*)pos >= data && (const uint8*)pos < (data+size) &&
			pos->code != END_CODE) {
		if (pos->code == OFFSETS_CODE) {
			if (pos->size >= sizeof(offsets_chunk)) {
				const int32 count = ((offsets_chunk*)pos)->atom_count;
				if (mode&AM_INCREMENT) { BUMP_ATOM(fHeader.fAtomCount, count); }
				else { BUMP_ATOM(fHeader.fAtomCount, -count); }
				return count;
			}
		} else if (!is_header_chunk((const uint8*)pos)) {
			break;
		}
		pos = (const chunk_header*)next_chunk((const uint8*)pos);
	}
	
	return 0;
}

int32 BMessageBody::BumpAtoms(const data_chunk* pos, uint32 mode,
							   int32 index, int32 count)
{
	if (pos->type != B_MESSAGE_TYPE) return 0;
	
	int32 amount = 0;
	
	switch (pos->header.subcode) {
		case SINGLE_DATA_SUBCODE: {
			amount += BumpAtoms((const uint8*)(pos->data_struct()), pos->entry_size, mode);
		} break;
		case FIXED_ARRAY_SUBCODE: {
			const fixed_array* data = (const fixed_array*)pos->data_struct();
			for (int32 i=index; i<(int32)data->count && count > 0; i++, count--) {
				amount += BumpAtoms(data->data(pos->entry_size, i), pos->entry_size, mode);
			}
		} break;
		case VARIABLE_ARRAY_SUBCODE: {
			const variable_array* data = (const variable_array*)pos->data_struct();
			for (int32 i=index; i<(int32)data->count && count > 0; i++, count--) {
				size_t size;
				const uint8* bytes = data->data(&size, i);
				amount += BumpAtoms(bytes, size, mode);
			}
		} break;
	}
	
	return amount;
}

void BMessageBody::HandleAtoms(void *tag, uint32 mode) const
{
	HandleAtoms(tag, fBody, fBodySize, mode, fHeader.fAtomCount);
}

void BMessageBody::HandleAtoms(void *tag, const uint8* data, size_t size,
							   uint32 mode, int32 hintNumAtoms)
{
	const data_chunk* pos = (const data_chunk*)data;
	while ((const uint8*)pos >= data && (const uint8*)pos < (data+size) &&
			pos->header.code != END_CODE) {
		if (pos->header.code == OFFSETS_CODE) {
			if (pos->header.size >= sizeof(offsets_chunk)) {
				hintNumAtoms = ((offsets_chunk*)pos)->atom_count;
				if (hintNumAtoms == 0) return;
			}
		} else if (pos->header.code == DATA_CODE) {
			if (pos->type == B_ATOM_TYPE || pos->type == B_ATOMREF_TYPE
					|| pos->type == B_MESSAGE_TYPE) {
				HandleAtoms(tag, pos, mode);
			}
		}
		pos = (const data_chunk*)next_chunk((const uint8*)pos);
	}
}

static inline void exec_handle_atom(void *tag, BAtom* a, type_code type, uint32 mode)
{
	if (a) {
		if (type == B_ATOM_TYPE) {
			if (mode&BMessageBody::AM_INCREMENT) {
				REF_ATOM(a, Acquire, tag);
			} else {
				REF_ATOM(a, Release, tag);
			}
		} else {
			if (mode&BMessageBody::AM_INCREMENT) {
				REF_ATOM(a, IncRefs, tag);
			} else {
				REF_ATOM(a, DecRefs, tag);
			}
		}
	}
}

void BMessageBody::HandleAtoms(void *tag, const data_chunk* pos, uint32 mode,
							   int32 index, int32 count)
{
	if (pos->type != B_ATOM_TYPE && pos->type != B_ATOMREF_TYPE &&
			pos->type != B_MESSAGE_TYPE) {
		return;
	}
	
	switch (pos->header.subcode) {
		case SINGLE_DATA_SUBCODE: {
			const size_t size = pos->entry_size;
			if (pos->type == B_MESSAGE_TYPE && size > sizeof(message_header)) {
				if (!(mode&AM_NO_RECURSION)) {
					HandleAtoms(tag,
								((const uint8*)(pos->data_struct()))+sizeof(message_header),
								size-sizeof(message_header), mode);
				}
			} else if (size == 4 && index == 0) {
				exec_handle_atom(tag,*(BAtom**)(pos->data_struct()), pos->type, mode);
			}
		} break;
		case FIXED_ARRAY_SUBCODE: {
			const fixed_array* data = (const fixed_array*)pos->data_struct();
			const size_t size = pos->entry_size;
			for (int32 i=index; i<(int32)data->count && count > 0; i++, count--) {
				if (pos->type == B_MESSAGE_TYPE && size > sizeof(message_header)) {
					if (!(mode&AM_NO_RECURSION)) {
						HandleAtoms(tag,
									data->data(pos->entry_size, i)+sizeof(message_header),
									size-sizeof(message_header), mode);
					}
				} else if (size == 4) {
					exec_handle_atom(tag,*(BAtom**)(data->data(4, i)), pos->type, mode);
				}
			}
		} break;
		case VARIABLE_ARRAY_SUBCODE: {
			const variable_array* data = (const variable_array*)pos->data_struct();
			for (int32 i=index; i<(int32)data->count && count > 0; i++, count--) {
				size_t size;
				const uint8* bytes = data->data(&size, i);
				if (pos->type == B_MESSAGE_TYPE && size > sizeof(message_header)) {
					if (!(mode&AM_NO_RECURSION)) {
						HandleAtoms(tag, bytes+sizeof(message_header),
									size-sizeof(message_header), mode);
					}
				} else if (size == 4) {
					exec_handle_atom(tag,*(BAtom**)bytes, pos->type, mode);
				}
			}
		} break;
	}
}

size_t BMessageBody::BodySize() const
{
	return (this ? fBodySize : sizeof(gEmptyMessage));
}

size_t BMessageBody::FullSize() const
{
	return (this ? fBodySize : sizeof(gEmptyMessage))
			+ sizeof(BMessageBody) - sizeof(((BMessageBody*)NULL)->fBody);
}

status_t BMessageBody::Flatten(const header_args* args,
							   BDataIO* stream, ssize_t* outSize,
							   const message_target* target, bool fixedSize) const
{
	D(DOUT << "BMessageBody::Flatten(" << this << ") stream" << endl);
	
	uint8 header[MAX_HEADER_SIZE];
	size_t size;
	status_t err = CreateHeader(header, &size, args, target, fixedSize);
	if (err != B_OK) return err;
	err = stream->Write(header, size);
	if (err < B_OK) return err;
	if (this) {
		const size_t bodySize = BodySize();
		err = stream->Write(fBody, bodySize);
		if (outSize) *outSize = size + bodySize;
	} else {
		#if DEBUG
		if (BodySize() != sizeof(gEmptyMessage)) {
			DOUT << PREFIX_TEXT << ": BodySize() does not match empty message\n" << endl;
		}
		#endif
		err = stream->Write(&gEmptyMessage, sizeof(gEmptyMessage));
		if (outSize) *outSize = size + sizeof(gEmptyMessage);
	}
	#if DEBUG
	if (((message_header*)header)->size != err+size) {
		DOUT << PREFIX_TEXT << ": flattened message size " << (err+size)
			 << " does not match written " << ((message_header*)header)->size << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
	}
	#endif
	
	if (err >= B_OK) {
		if (CountAtoms()) {
			DATOM(DOUT << "Handling atoms in " << this << " for flatten" << endl);
			HandleAtoms(const_cast<BMessageBody*>(this), AM_INCREMENT);
		}
		return B_OK;
	}
	
	return err;
}

status_t BMessageBody::Flatten(const header_args* args,
							   char* buffer, ssize_t /*availSize*/,
							   const message_target* target, bool fixedSize) const
{
	D(DOUT << "BMessageBody::Flatten(" << this << ") buffer " << buffer << endl);
	
	size_t size;
	status_t err = CreateHeader((uint8*)buffer, &size, args, target, fixedSize);
	if (err != B_OK) return err;
	buffer += size;
	if (this) {
		db_memcpy(buffer, fBody, BodySize());
	} else {
		db_memcpy(buffer, &gEmptyMessage, sizeof(gEmptyMessage));
	}
	
	if (CountAtoms()) {
		DATOM(DOUT << "Handling atoms in " << this << " for flatten" << endl);
		HandleAtoms(const_cast<BMessageBody*>(this), AM_INCREMENT);
	}
	return B_OK;
}

static const ssize_t kMinFlatSize = sizeof(message_header)+sizeof(chunk_header);

BMessageBody* BMessageBody::Unflatten(BDataIO* stream, uint32 sig, header_args* outArgs,
									  message_target* target, status_t* err)
{
	D(DOUT << "BMessageBody::Unflatten(" << this << ") stream" << endl);
	
	uint8 buffer[MAX_HEADER_SIZE];
	bool swapping = false;
	
	// Place signature back at front of message header.
	*(uint32*)buffer = sig;
	
	// First read the message header plus first chunk header,
	// to determine how big the stream is and the first chunk type.
	ssize_t size = stream->Read(buffer+sizeof(sig), kMinFlatSize-sizeof(sig));
	if (size < B_OK) {
		#if VERBOSE_ERRORS
		DOUT << "BMessageBody::Unflatten() error reading header: " << strerror(size) << endl;
		#endif
		*err = size;
		return this;
	}
	size += sizeof(sig);
	
	// Parse message prefix.
	// NOTE: The signature is also checked in BMessage::Unflatten()!
	size_t headerSize = sizeof(message_header);
	if (((const message_header*)buffer)->signature != MESSAGE_SIGNATURE || size < kMinFlatSize) {
		if (((const message_header*)buffer)->signature != SWAPPED_MESSAGE_SIGNATURE || size < kMinFlatSize) {
			// This is does not look like a regular flattened message.
			// Try parsing in old format.  This is simple and brain-dead,
			// so it just always creates a new message.
			BMemoryStreamIO join(buffer, size, stream);
			BMessageBody* msg = unflatten_old_message(&join, outArgs, target, err);
			if (*err != B_OK) {
				#if VERBOSE_ERRORS
				DOUT << PREFIX_TEXT << "::Unflatten() error: " << strerror(*err) << endl;
				#endif
				if (msg) msg->Release();
				return this;
			}
			if (this) this->Release();
			return msg;
		}
		swapping = true;
	}
	
	outArgs->what = ((const message_header*)buffer)->what;
	size_t bodySize = ((const message_header*)buffer)->size;
	
	if (swapping) {
		outArgs->what = B_BYTE_SWAP_INT32(outArgs->what);
		bodySize = B_BYTE_SWAP_INT32(bodySize);
	}
	
	header_data header;
	header.init_all();
	
	// Parse through header chunks until a body chunk is found.
	
	chunk_header* next_chunk = (chunk_header*)(buffer+sizeof(message_header));
	
	if (swapping) {
		next_chunk->code = B_BYTE_SWAP_INT16(next_chunk->code);
		next_chunk->subcode = B_BYTE_SWAP_INT16(next_chunk->subcode);
		next_chunk->size = B_BYTE_SWAP_INT32(next_chunk->size);
	}
	
	while (is_header_chunk((const uint8*)next_chunk)) {
	
		// Read data for this chunk and next chunk's header.
		chunk_header chunk = *next_chunk;
		size_t remain = chunk.size;
		if (remain < sizeof(chunk_header)) {
			DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() header size too small: "
						<< remain << endl);
			*err = B_NOT_A_MESSAGE;
			return this;
		}
		while (remain > 0) {
			size_t amount = remain;
			if (amount > sizeof(buffer)) {
				// Complicated case -- this is some chunk we don't understand
				// that is larger than the buffer.  Need to be sure that as
				// we read it in sections, the last read gets the full next
				// chunk header.
				amount = sizeof(buffer);
				if ((remain-amount) < sizeof(chunk_header)) {
					amount = sizeof(buffer)-sizeof(chunk_header);
				}
			}
			size = stream->Read(buffer, amount);
			if (size != (ssize_t)amount) {
				if (size < B_OK) {
					DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() error reading header chunk: "
								<< strerror(size) << endl);
					*err = size;
					return this;
				}
				DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() not enough read for header chunk: "
							<< size << ", but expected" << amount);
				#if VERBOSE_ERRORS
				DOUT << (BHexDump(buffer, amount).SetSingleLineCutoff(-1)) << endl;
				#endif
				*err = B_NOT_A_MESSAGE;
				return this;
			}
			next_chunk = (chunk_header*)(buffer + amount - sizeof(chunk_header));
			remain -= amount;
		}
		
		headerSize += chunk.size;
		
		ParseHeaderChunk(&chunk, buffer-sizeof(chunk_header),
						 outArgs, &header, target, swapping);
		
		if (swapping) {
			next_chunk->code = B_BYTE_SWAP_INT16(next_chunk->code);
			next_chunk->subcode = B_BYTE_SWAP_INT16(next_chunk->subcode);
			next_chunk->size = B_BYTE_SWAP_INT32(next_chunk->size);
		}
	}
	
	// At this point, headerSize is the number of bytes in the
	// flattened message's header, bodySize is the number of bytes
	// in the total header, and next_chunk points to the first
	// chunk_header size bytes of the body data.  Copy this into
	// the message body, and read the rest of the body after it in
	// one fell swoop.
	
	bodySize -= headerSize;
	
	// If the body is not large enough for any data, don't bother
	// allocating memory for it, and just return immediately.
	
	if (bodySize <= sizeof(gEmptyMessage)) {
		// But do read remaining data.
		char buffer[sizeof(gEmptyMessage)];
		size = stream->Read(buffer, bodySize);
		if (this) Release();
		*err = size < B_OK ? size : B_OK;
		return NULL;
	}
	
	// Make sure this object is large enough to contain all of
	// the body data.
	BMessageBody* This = Resize(bodySize);
	if (!This) {
		DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() out of memory" << endl);
		*err = B_NO_MEMORY;
		return this;
	}
	
	This->fHeader = header;
	
	// Now read that data into the message.
	
	*(chunk_header*)(This->fBody) = *next_chunk;
	size = stream->Read(This->fBody+sizeof(chunk_header),
						bodySize-sizeof(chunk_header));
	if (size != (ssize_t)(bodySize-sizeof(chunk_header))) {
		if (size < B_OK) {
			DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() error reading body: "
						<< strerror(size) << endl);
			*err = size;
			return This;
		}
		DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() not enough read for body: "
					<< size << ", but expected " << (bodySize-sizeof(chunk_header)));
		#if VERBOSE_ERRORS
		DOUT << (BHexDump(This->fBody, bodySize, 16, "\t").SetSingleLineCutoff(-1))
			 << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
		#endif
		*err = B_NOT_A_MESSAGE;
		return This;
	}
	
	if (swapping) {
		*err = This->SwapBodyChunks(&bodySize, false);
		if (*err != B_OK) {
			DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() error swapping body: "
						<< strerror(*err) << endl);
			#if VERBOSE_ERRORS
			DOUT << (BHexDump(This->fBody, bodySize, 16, "\t").SetSingleLineCutoff(-1))
				 << endl;
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(DOUT);
			#endif
			return This;
		}
	}
	
	// Finally, parse and check that big gloop of data we just
	// just sucked in.
	
	*err = This->ParseBodyChunks(bodySize);
	if (*err != B_OK) {
		DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() error parsing body: "
					<< strerror(*err) << endl);
		#if VERBOSE_ERRORS
		DOUT << (BHexDump(This->fBody, bodySize, 16, "\t").SetSingleLineCutoff(-1))
			 << endl;
		#endif
	}
	
	if (*err == B_OK) {
		if (This->CountAtoms()) {
			DATOM(DOUT << "Handling atoms in " << this << " for unflatten" << endl);
			This->HandleAtoms(This, AM_INCREMENT);
		}
	}
	
	return This;
}

BMessageBody* BMessageBody::Unflatten(const char* data, size_t size,
									  header_args* outArgs, message_target* target,
									  status_t* err)
{
	D(DOUT << "BMessageBody::Unflatten(" << this << ") buffer" << endl);
	
	bool swapping = false;
	
	// First check message header.
	if (size < sizeof(uint32)) {
		*err = B_NOT_A_MESSAGE;
		return this;
	}
	
	// Parse message prefix.
	size_t headerSize = sizeof(message_header);
	if (((const message_header*)data)->signature != MESSAGE_SIGNATURE || size < sizeof(message_header)) {
		if (((const message_header*)data)->signature != SWAPPED_MESSAGE_SIGNATURE || size < sizeof(message_header)) {
			// This is does not look like a regular flattened message.
			// Try parsing in old format.  This is simple and brain-dead,
			// so it just always creates a new message.
			BMemoryStreamIO stream(data, size);
			BMessageBody* msg = unflatten_old_message(&stream, outArgs, target, err);
			if (*err != B_OK) {
				#if VERBOSE_ERRORS
				DOUT << PREFIX_TEXT << "::Unflatten() error: " << strerror(*err) << endl;
				#endif
				if (msg) msg->Release();
				return this;
			}
			if (this) this->Release();
			return msg;
		}
		swapping = true;
	}
	
	outArgs->what = ((const message_header*)data)->what;
	size_t bodySize = ((const message_header*)data)->size;
	
	if (swapping) {
		outArgs->what = B_BYTE_SWAP_INT32(outArgs->what);
		bodySize = B_BYTE_SWAP_INT32(bodySize);
	}
	
	data += sizeof(message_header);
	size -= sizeof(message_header);
	
	header_data header;
	header.init_all();
	
	// Parse through header chunks until a body chunk is found.
	while (size >= sizeof(chunk_header)) {
		chunk_header chunk = *(chunk_header*)data;
	
		if (swapping) {
			chunk.code = B_BYTE_SWAP_INT16(chunk.code);
			chunk.subcode = B_BYTE_SWAP_INT16(chunk.subcode);
			chunk.size = B_BYTE_SWAP_INT32(chunk.size);
		}
		
		if (!is_header_chunk((const uint8*)&chunk)) break;
		
		if (size < chunk.size) {
			DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() header size too small: "
					<< size << endl);
			*err = B_NOT_A_MESSAGE;
			return this;
		}
		
		headerSize += chunk.size;
		
		ParseHeaderChunk(&chunk, (const uint8*)data,
						 outArgs, &header, target, swapping);
		
		data += chunk.size;
		size -= chunk.size;
	}
	
	// At this point, headerSize is the number of bytes in the
	// flattened message's header and bodySize is the number of bytes
	// in the total header.  Compute the size of the actual body and
	// copy it all in one fell swoop.
	
	bodySize -= headerSize;
	
	// If the body is not large enough for any data, don't bother
	// allocating memory for it, and just return immediately.
	
	if (bodySize <= sizeof(gEmptyMessage)) {
		if (this) Release();
		*err = B_OK;
		return NULL;
	}
	
	if (bodySize > size) {
		DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() not enough data for body: "
					<< size << ", but expected " << bodySize);
		#if VERBOSE_ERRORS
		DOUT << (BHexDump(data, size, 16, "\t").SetSingleLineCutoff(-1))
			 << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
		#endif
		*err = B_NOT_A_MESSAGE;
		return this;
	}
	
	// Make sure this object is large enough to contain all of
	// the body data.
	BMessageBody* This = Resize(bodySize);
	if (!This) {
		DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() out of memory" << endl);
		*err = B_NO_MEMORY;
		return this;
	}
	
	This->fHeader = header;
	
	// Now copy that data into the message.
	memcpy(This->fBody, data, bodySize);
	
	if (swapping) {
		*err = This->SwapBodyChunks(&bodySize, false);
		if (*err != B_OK) {
			DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() error swapping body: "
						<< strerror(*err) << endl);
			#if VERBOSE_ERRORS
			DOUT << (BHexDump(This->fBody, bodySize, 16, "\t").SetSingleLineCutoff(-1))
				 << endl;
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(DOUT);
			#endif
			return This;
		}
	}
	
	// Finally, parse and check that big gloop of data we just
	// just sucked in.
	
	*err = This->ParseBodyChunks(bodySize);
	if (*err != B_OK) {
		DPARSE(DOUT << PREFIX_TEXT << "::Unflatten() error parsing body: "
					<< strerror(*err) << endl);
		#if VERBOSE_ERRORS
		DOUT << (BHexDump(This->fBody, bodySize, 16, "\t").SetSingleLineCutoff(-1))
			 << endl;
		#endif
	}
	
	if (*err == B_OK) {
		if (This->CountAtoms()) {
			DATOM(DOUT << "Handling atoms in " << this << " for flatten" << endl);
			This->HandleAtoms(This, AM_INCREMENT);
		}
	}
	
	return This;
}

status_t BMessageBody::WritePort(port_id port, int32 code,
								 const header_args* args, const message_target* target,
								 uint32 flags, bigtime_t timeout) const
	DECLARE_RETURN(result)
{
	message_write_context context;
	
	if (CountAtoms()) {
		debugger("Can't send a message containing atoms");
	}
	
	DPORT(port_info pi);
	DPORT(get_port_info(port, &pi));
	DPORT(DOUT << "BMessageBody: Writing to port " << port
				<< " (owned by team " << pi.team << "), what " << args->what << endl);
	
	// If this message is "really big", then flatten it into a
	// new area and only send the area id through the port.
	if (BodySize() > 40*1024) {
		const size_t size = CalcHeaderSize(args, target ? true : false)
						  + BodySize();
		void* addr = NULL;
		area_id area = create_area("BMessage Send", &addr,
								   B_ANY_ADDRESS,
								   ((size+B_PAGE_SIZE-1)/B_PAGE_SIZE)*B_PAGE_SIZE,
								   B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
#if SUPPORTS_STREAM_IO
			DOUT << PREFIX_TEXT << ": Error creating area: "
				<< strerror(area);
#endif
			return area;
		}
		
		status_t result = Flatten(args, (char*)addr, size, target, false);
		if (result != B_OK) {
			delete_area(area);
			return result;
		}
		
		int32 data[2];
		data[0] = code;
		data[1] = area;
		
		do {
			result = write_port_etc(port, 'AREA',
									&data, sizeof(data), flags, timeout);
		} while (result == B_INTERRUPTED);
		
		if (result < B_OK) {
			delete_area(area);
		}
		
		return result;
	}
	
	status_t result = StartWriting(&context, args, target);
	if (result == B_OK) {
	
		DPORT(DOUT << (BHexDump(context.data, context.size).SetSingleLineCutoff(-1)) << endl);
		
		do {
			result = write_port_etc(port, code,
									context.data, context.size, flags, timeout);
		} while (result == B_INTERRUPTED);
	
		if (result < B_OK) {
			#if PORT_ERRORS
			port_info pi;
			pi.name[0] = 0;
			get_port_info(port, &pi);
			DOUT << PREFIX_TEXT << ": Error writing port " << port
				 << " (" << pi.name << "): "
				 << strerror(result) << endl;
			#endif
			#if VERBOSE_ERRORS
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(DOUT);
			#endif
		}
	#if PORT_ERRORS
	} else {
		DOUT << PREFIX_TEXT << ": Error starting write: " << strerror(result) << endl;
	#endif
	}
	
	FinishWriting(&context);
	
	return result;
}

BMessageBody* BMessageBody::ReadPort(port_id port, ssize_t size,
								int32* outCode, header_args* outArgs,
								message_target* outTarget, status_t* outResult,
								uint32 flags, bigtime_t timeout)
{
	DPORT(port_info pi);
	DPORT(get_port_info(port, &pi));
	DPORT(DOUT << "BMessageBody: Reading from port " << port
				<< " (owned by team " << pi.team << ")" << endl);
	
	if (size < B_OK) {
		while ((size = port_buffer_size_etc(port, flags, timeout)) == B_INTERRUPTED)
			;
		if (size < 0) {
			#if PORT_ERRORS
			if (size != B_TIMED_OUT) {
				port_info pi;
				pi.name[0] = 0;
				get_port_info(port, &pi);
				DOUT << PREFIX_TEXT << ": Error in team " << _find_cur_team_id_()
					 << " getting size of port " << port
					 << " (" << pi.name << "): "
					 << strerror(size) << endl;
			}
			#endif
			*outResult = (status_t)size;
			return this;
		}
	}
	
	if (size == 0) {
		// If there is no data, just get the message code.
		do {
			*outResult = read_port_etc(port, outCode, NULL, 0, flags, timeout);
		} while (*outResult == B_INTERRUPTED);
		
		// Return error indicating a message was not read, but the read
		// from the port was successful.
		if (*outResult == B_OK) *outResult = B_NOT_A_MESSAGE;
		#if PORT_ERRORS
		else if (*outResult != B_TIMED_OUT) {
			port_info pi;
			pi.name[0] = 0;
			get_port_info(port, &pi);
			DOUT << PREFIX_TEXT << ": Error in team " << _find_cur_team_id_()
				 << " reading from port " << port
				 << " (" << pi.name << "): "
				 << strerror(*outResult) << endl;
		}
		#endif
		
		if (this) Release();
		return NULL;
	}
	
	if (size == sizeof(int32)*2) {
		// This probably is an area_id of the actual message data.
		int32 buffer[2];
		int32 code;
		do {
			*outResult = read_port_etc(port, &code, buffer, sizeof(buffer), flags, timeout);
		} while (*outResult == B_INTERRUPTED);
		
		if (*outResult < B_OK) {
			return this;
		}
		
		if (code == 'AREA') {
			// Yep!
			if (outCode) *outCode = buffer[0];
			void* addr = NULL;
			area_id area = clone_area("BMessage Receive", &addr,
									  B_ANY_ADDRESS, B_READ_AREA, buffer[1]);
			delete_area(buffer[1]);
			if (area < B_OK) {
				*outResult = area;
				return this;
			}
			
			BMessageBody* This = Unflatten((char*)addr, 0x7ffffff,
										   outArgs, outTarget, outResult);
			delete_area(area);
			return This;
		}
		
		// Not an area -- just return raw data.
		BMessageBody* This = this;
		if (!This) {
			This = Create();
			if (This) This->Acquire();
		}
		if (This) {
			data_info data;
			data.type = B_RAW_TYPE;
			data.size = size;
			data.data = buffer;
			This = This->FastAddData("be:port_data", &data, false, outResult);
			if (*outResult == B_OK) *outResult = B_NOT_A_MESSAGE;
		} else {
			*outResult = B_NO_MEMORY;
		}
		outArgs->init();
		outArgs->what = B_RAW_PORT_DATA;
		*outTarget = message_target();
		return This;
	}
	
	// First we need an object big enough to contain the incoming
	// message.  We'll ignore the space used by the header, because
	// the memory waste is minimal and that may allow us to avoid
	// a realloc().
	BMessageBody* This = Resize(size);
	if (!This) {
		DPARSE(DOUT << PREFIX_TEXT << "::ReadPort() out of memory" << endl);
		*outResult = B_NO_MEMORY;
		return this;
	}
	
	const size_t expectedHeaderSize = This->FixedHeaderSize();
	uint8* buffer = This->fBody-expectedHeaderSize;
	uint8* base = buffer;
	
	DPORT(DOUT << "BMessageBody: Reading " << size << " bytes from port at offset "
				<< expectedHeaderSize << endl);
				
	do {
		*outResult = read_port_etc(port, outCode, (char*)buffer, size, flags, timeout);
	} while (*outResult == B_INTERRUPTED);

	if (*outResult < B_OK) {
		#if PORT_ERRORS
		port_info pi;
		pi.name[0] = 0;
		get_port_info(port, &pi);
		DOUT << PREFIX_TEXT << ": Error in team " << _find_cur_team_id_()
			 << " reading from port " << port
			 << " (" << pi.name << "): "
			 << strerror(*outResult) << endl;
		#endif
		return This;
	}
	
	DPORT(DOUT << (BHexDump(buffer, size).SetSingleLineCutoff(-1)) << endl);
	
	// See if all looks good.  This is very fragile -- if the received message
	// isn't exactly what we were expecting, then go through the slower but
	// robust Unflatten() interface.
	bool bad = false;
	if (((const message_header*)buffer)->signature != MESSAGE_SIGNATURE
		|| (((const message_header*)buffer)->size != (size_t)size)) {
		DPORT(DOUT << "BMessageBody: Mismatch parsing message header!" << endl
					<< "BMessageBody: Signature="
					<< ((const message_header*)buffer)->signature
					<< ", port size=" << size
					<< ", header size="
					<< ((const message_header*)buffer)->size << endl);
		bad = true;
	}
	
	outArgs->what = ((const message_header*)buffer)->what;
	buffer += sizeof(message_header);
	
	This->fHeader.init_all();
	
	// Parse message header.
	while (!bad && buffer >= base && buffer < (This->fBody+size) &&
			is_header_chunk(buffer) &&  ((chunk_header*)buffer)->code != END_CODE) {
		status_t state = ParseHeaderChunk((chunk_header*)buffer, buffer,
										  outArgs, &(This->fHeader), outTarget, false);
		if (state < B_OK) {
			DPARSE(DOUT << PREFIX_TEXT << ": Mismatch parsing header chunk from port" << endl);
			bad = true;
		} else {
			buffer = next_chunk(buffer);
		}
	}
	
	if (buffer < base || buffer >= (This->fBody+size)) {
		DPARSE(DOUT << PREFIX_TEXT << ": Header extended beyond data" << endl);
		bad = true;
	}
	
	size_t bodySize = size - expectedHeaderSize;
	
	// We are now at the message body, which should magically be
	// located at fBody.
	if (!bad && buffer != This->fBody) {
		// Whoops, didn't match up.  We can try moving the data in
		// a desperate attempt to make believe that it's still okay.
		bodySize = size - size_t(buffer-base);
		DPARSE(DOUT << PREFIX_TEXT << ": Need to move body data from "
					<< This->fBody << " to " << buffer << endl);
		DPORT(DOUT << "Expected header size=" << expectedHeaderSize
			 << ", Found=" << size_t(buffer-base)
			 << ", Body=" << bodySize);
		#if VERBOSE_ERRORS
		DOUT << (BHexDump(base, size).SetSingleLineCutoff(-1)) << endl;
		#endif
		if (buffer < This->fBody) {
			// We know this won't go out of bounds because we made
			// the object big enough to contain -all- of the data.
			db_memmove(This->fBody, buffer, bodySize);
		} else {
			db_memcpy(This->fBody, buffer, bodySize);
		}
		buffer = fBody;
	}
	
	// Parse message body.
	if (!bad && This->ParseBodyChunks(bodySize) != B_OK) {
		DPARSE(DOUT << PREFIX_TEXT
					<< "BMessageBody: Mismatch parsing data chunks from port" << endl);
		bad = true;
	}
	
	if (bad) {
		// Well if I can't be fast, then I'll be really slow...  hmph.
		DPORT(DOUT << "BMessageBody: Damn, forced to Unflatten()" << endl);
		BMessageBody* other = ((BMessageBody*)NULL)->Unflatten((const char*)base, size,
															   outArgs, outTarget, outResult);
		if (*outResult == B_NOT_A_MESSAGE) {
			// If the data isn't a message, we will return this error
			// code.  However, also construct and return a valid message
			// which contains the raw data we found.
			if (other) other->Release();
			if (This) {
				other = Create();
				if (other) {
					other->Acquire();
					data_info data;
					data.type = B_RAW_TYPE;
					data.size = size;
					data.data = base;
					other = other->FastAddData("be:port_data", &data, false, outResult);
					if (*outResult == B_OK) *outResult = B_NOT_A_MESSAGE;
				} else {
					*outResult = B_NO_MEMORY;
				}
				outArgs->init();
				outArgs->what = B_RAW_PORT_DATA;
				*outTarget = message_target();
			} else {
				other = NULL;
			}
		}
		
		if (This) This->Release();
		return other;
	}
	
	DPORT(DOUT << "BMessageBody: Done reading from port!" << endl);
	
	// Amazing, it looks okay!
	*outResult = B_OK;
	return This;
}

// ---------------------------------------------------------------------------

status_t BMessageBody::StartWriting(message_write_context* context, const header_args* args,
									const message_target* target, bool fixedSize) const
{
	context->data = NULL;
	context->size = 0;
	context->allocBuffer = NULL;
	context->alreadySending = 0;
	
	D(DOUT << "BMessageBody: Starting write of body " << this << endl);
	
	if (CountAtoms()) {
		debugger("Can't write a message containing atoms");
	}
	
	const size_t headerSize = fixedSize
							? FixedHeaderSize()
							: CalcHeaderSize(args, target ? true : false);
	
	if (!this) {
		// If there is no body, we will construct the message on the stack.
		D(DOUT << "BMessageBody: No message body, writing from stack." << endl);
		context->size = headerSize + BodySize();
		context->data = context->stackBuffer;
		context->alreadySending = 1;
		ASSERT (context->size <= sizeof(context->stackBuffer));
		const status_t result = Flatten(args, (char*)context->data,
										context->size, target, fixedSize);
		if (result != B_OK) return result;
		
	} else if ((context->alreadySending=(atomic_or(&(fHeader.fFlags), MBF_SENDING)
										 & MBF_SENDING)) != 0) {
		// Someone else is already using this body to send; have to do it
		// the slow way.  Make no attempt to optimize the memory allocation,
		// because this should be a very rare path.
		D(DOUT << "BMessageBody: This body is already in use, flattening for write." << endl);
		context->size = headerSize + BodySize();
		context->data = context->allocBuffer = (uint8*)message_malloc(context->size);
		if (!context->allocBuffer) return B_NO_MEMORY;
		const status_t result = Flatten(args, (char*)context->data,
										context->size, target, fixedSize);
		if (result != B_OK) {
			message_free(context->allocBuffer);
			return result;
		}
		
	} else {
		// We can send directly from the message block, woo hoo!
		D(DOUT << "BMessageBody: Writing directly from message body." << endl);
		context->data = ((uint8*)fBuffer) + sizeof(fBuffer) - headerSize;
		ASSERT(context->data >= (uint8*)fBuffer);
		size_t writtenSize = 0;
		const status_t result = CreateHeader(const_cast<uint8*>(context->data),
											 &writtenSize, args, target, fixedSize);
		if (result != B_OK) return result;
		ASSERT(writtenSize == headerSize);
		context->size = headerSize + BodySize();
		
	}
	
	return B_OK;
}

void BMessageBody::FinishWriting(message_write_context* context) const
{
	if (context->allocBuffer) message_free(context->allocBuffer);
	if (!context->alreadySending) atomic_and(&(fHeader.fFlags), ~MBF_SENDING);
}

// ---------------------------------------------------------------------------

int32 BMessageBody::CountNames(type_code type) const
{
	D(DOUT << "BMessageBody::CountNames(" << this << ", type=" << type << ")" << endl);
	
	if (!this) return 0;
	
	int32 num = 0;
	
	if (type == B_ANY_TYPE) {
		const data_chunk* pos = (const data_chunk*)fBody;
		while (pos->header.code != END_CODE) {
			if (pos->header.code == DATA_CODE) {
				num++;
			}
			pos = (const data_chunk*)next_chunk((const uint8*)pos);
		}
	} else {
		const data_chunk* pos = (const data_chunk*)fBody;
		while (pos->header.code != END_CODE) {
			if (pos->header.code == DATA_CODE && cmp_types(pos->type,type)) {
				num++;
			}
			pos = (const data_chunk*)next_chunk((const uint8*)pos);
		}
	}
	
	return num;
}

status_t BMessageBody::FillInData(data_info* outInfo,
									const data_chunk* chunk, int32 index)
{
	outInfo->name = chunk->name;
	outInfo->type = chunk->type;
	
	switch (chunk->header.subcode) {
		case SINGLE_DATA_SUBCODE: {
			outInfo->count = 1;
			if (index > 0) return B_BAD_INDEX;
			outInfo->size = chunk->entry_size;
			outInfo->data = chunk->data_struct();
		} break;
		case FIXED_ARRAY_SUBCODE: {
			const fixed_array* data = (const fixed_array*)chunk->data_struct();
			outInfo->count = data->count;
			if (index >= (int32)data->count) return B_BAD_INDEX;
			outInfo->size = chunk->entry_size;
			outInfo->data = data->data(outInfo->size, index >= 0 ? index : 0);
		} break;
		case VARIABLE_ARRAY_SUBCODE: {
			const variable_array* data = (const variable_array*)chunk->data_struct();
			outInfo->count = data->count;
			if (index >= (int32)data->count) return B_BAD_INDEX;
			if (index >= 0) {
				outInfo->data = data->data(&outInfo->size, index);
			} else {
				outInfo->size = NULL_SIZE;
				size_t dummy;
				outInfo->data = data->data(&dummy, 0);
			}
		} break;
		default:
			return B_BAD_DATA;
	}
	return B_OK;
}

status_t BMessageBody::FindData(const char* name, type_code type, int32 which,
								data_info* outInfo, int32 index, data_off* inoutCookie) const
{
	if (!this) return B_NAME_NOT_FOUND;
	
	const data_chunk* pos = NULL;
	if (inoutCookie) {
		if (*inoutCookie >= fBodySize) {
			return B_BAD_VALUE;
		}
		pos = (const data_chunk*)(fBody + *inoutCookie);
	}
	status_t err;
	pos = FindDataItem(name, type, which, &err, pos);
	if (!pos) return err;
	
	outInfo->pos = (data_off)((const uint8*)pos - fBody);
	if (inoutCookie) {
		*inoutCookie = next_chunk_off(fBody, outInfo->pos);
		if (*inoutCookie > fBodySize) {
			*inoutCookie = fBodySize;
			return B_BAD_DATA;
		}
	}
	return FillInData(outInfo, pos, index);
}

// ---------------------------------------------------------------------------

namespace BPrivate {

// Not static because this is a friend of BMessageBody.
BMessageBody* reserve_array_data(BMessageBody* This,
								 data_chunk* pos,
								 BMessageBody::data_info* inoutInfo,
								 bool hintFixedSize,
								 status_t* outResult)
{
	BMessageBody* origThis = This;
	
	if (!cmp_types(pos->type, inoutInfo->type)) {
		*outResult = B_NAME_IN_USE;
		return This;
	}
	
	const data_off chunkOff = (data_off)((uint8*)pos - This->fBody);
	const data_off dataOff = (data_off)((uint8*)pos->data_struct() - This->fBody);
	
	inoutInfo->pos = chunkOff;
	const size_t size = ((data_chunk*)(This->fBody+chunkOff))->entry_size;
	
	switch (pos->header.subcode) {
		case SINGLE_DATA_SUBCODE: {
			// Turn existing single entry into an array.
			if (hintFixedSize && size == inoutInfo->size) {
				// Now it is a fixed_array.
				// Grow entry by the new 'fixed_array' prefix and the size of an entry.
				const data_off endOff = dataOff+size;
				const data_off newEndOff = endOff + size + sizeof(fixed_array);
				This = This->ExpandData(endOff, chunk_align(newEndOff)-chunk_align(endOff));
				if (!This) {
					*outResult = B_NO_MEMORY;
					return origThis;
				}
				pos = (data_chunk*)(This->fBody + chunkOff);
				D(DOUT << "BMessageBody: single data chunk=" << pos << endl);
				// Make room for fixed_array structure at head of data.
				db_memmove(This->fBody+dataOff+sizeof(fixed_array),
						This->fBody+dataOff, chunk_align(size));
				// Copy new data in and adjust sizes.
				fixed_array* data = (fixed_array*)(This->fBody+dataOff);
				data->count = 2;
				data->reserved0 = 0;
				inoutInfo->data = data->data(size, 1);
				pos->header.subcode = FIXED_ARRAY_SUBCODE;
				pos->header.size = uint32(((const uint8*)inoutInfo->data)+inoutInfo->size
										- (const uint8*)pos);
			} else {
				// Now it is a variable_array.
				// Grow entry by the new 'variable_array' prefix and the size of
				// this second entry, plus two entries in the index.
				const data_off grow = sizeof(variable_array)
									+ chunk_align(inoutInfo->size)
									+ chunk_align(sizeof(data_off)*2);
				const data_off endOff = chunk_align(dataOff+size);
				This = This->ExpandData(endOff, grow);
				if (!This) {
					*outResult = B_NO_MEMORY;
					return origThis;
				}
				pos = (data_chunk*)(This->fBody + chunkOff);
				D(DOUT << "BMessageBody: single data pos=" << pos << endl);
				// Make room for variable_array structure at head of data.
				db_memmove(This->fBody+dataOff+sizeof(variable_array),
						This->fBody+dataOff, chunk_align(size));
				// Set up header structure and trailing entry index.
				variable_array* data = (variable_array*)(This->fBody+dataOff);
				data->count = 2;
				data->total_size = chunk_align(size) + chunk_align(inoutInfo->size);
				data_off* index = (data_off*)( ((uint8*)(data+1)) + data->total_size );
				*index++ = size;
				*index++ = chunk_align(size) + inoutInfo->size;
				pos->entry_size = 0;
				inoutInfo->data = ((uint8*)(data+1)) + chunk_align(size);
				pos->header.subcode = VARIABLE_ARRAY_SUBCODE;
				pos->header.size = uint32((const uint8*)index - (const uint8*)pos);
			}
		} break;
		
		case FIXED_ARRAY_SUBCODE: {
			// Add a new entry to an existing fixed-size array.
			fixed_array* data = (fixed_array*)(This->fBody+dataOff);
			if (size != inoutInfo->size) {
				// TO DO: Promote to variable_array structure.
				*outResult = B_MISMATCHED_VALUES;
				return This;
			}
			const data_off endOff = fixed_array_end(This->fBody, dataOff, size);
			const data_off newEndOff = endOff + size;
			if (chunk_align(endOff) != chunk_align(newEndOff)) {
				This = This->ExpandData(endOff, chunk_align(newEndOff)-chunk_align(endOff));
				if (!This) {
					*outResult = B_NO_MEMORY;
					return origThis;
				}
				pos = (data_chunk*)(This->fBody + chunkOff);
				data = (fixed_array*)(This->fBody + dataOff);
				D(DOUT << "BMessageBody: fixed_array pos=" << pos
						<< ", data=" << data << endl);
			}
			
			data->count++;
			inoutInfo->data = This->fBody+endOff;
			pos->header.size = uint32(dataOff-chunkOff + sizeof(fixed_array) + data->count*size);
		} break;
		
		case VARIABLE_ARRAY_SUBCODE: {
			// Add a new entry to an existing variable-size array.  The additional
			// space we need is a chunk-aligned section for the new data, plus
			// a possible additional chunk-aligned size of data if the index grows
			// into a new alignment.
			variable_array* data = (variable_array*)(This->fBody+dataOff);
			const data_off endOff = next_chunk_off(This->fBody, chunkOff);
			if (endOff > This->fBodySize) {
				*outResult = B_BAD_DATA;
				return origThis;
			}
			const data_off origIndexSize = data->count*sizeof(data_off);
			const data_off indexChange = chunk_align(origIndexSize+sizeof(data_off))
									   - chunk_align(origIndexSize);
			const data_off newEndOff = endOff
									 + chunk_align(inoutInfo->size)
									 + indexChange;
			
			This = This->ExpandData(endOff, newEndOff-endOff);
			if (!This) {
				*outResult = B_NO_MEMORY;
				return origThis;
			}
			pos = (data_chunk*)(This->fBody + chunkOff);
			D(DOUT << "BMessageBody: variable_array pos=" << pos << endl);
			data = (variable_array*)(This->fBody + dataOff);
			
			// Move index to new location.
			const size_t newTotalSize = data->total_size + chunk_align(inoutInfo->size);
			data_off* index = (data_off*)(This->fBody + dataOff
										  + sizeof(variable_array)
										  + newTotalSize);
			db_memmove(index,
					   This->fBody+dataOff+sizeof(variable_array) + data->total_size,
					   chunk_align(origIndexSize));
			index[data->count] = data->total_size + inoutInfo->size;
			if (indexChange) chunk_pad((uint8*)(index+data->count+1));
			inoutInfo->data = ((uint8*)(data+1))+data->total_size;
			data->total_size = newTotalSize;
			data->count++;
			pos->header.size = uint32(newEndOff - chunkOff);
		} break;
		
		default:
			DPARSE(DOUT << PREFIX_TEXT << ": Unknown data chunk type "
						<< (int32)pos->header.subcode << endl);
			*outResult = B_ERROR;
			return This;
	}
	
	chunk_pad(((char*)inoutInfo->data)+inoutInfo->size);
	
	D(DOUT << "Finished with: " << BHexDump(This->fBody, This->fBodySize) << endl);
	
	*outResult = B_OK;
	return This;
}

}	/* namespace BPrivate */

BMessageBody* BMessageBody::AllocData(const char* name, data_info* inoutInfo,
									  bool hintFixedSize, status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::AllocData(" << this << ", name=" << name << ")" << endl);
	
	BMessageBody* This = this;
	
	int32 indexPos;
	data_chunk* pos = const_cast<data_chunk*>(FindDataItem(name, &indexPos));
	
	if (pos) {
		// Data with this name already exists -- reserve in array.
		return reserve_array_data(this, pos, inoutInfo, hintFixedSize, outResult);
	}
	
	if (indexPos < 0) {
		*outResult = B_BAD_DATA;
		return This;
	}
	
	// Reserve data at end of body.
	
	data_off chunkOff = fHeader.fDataEnd;
	size_t nameSize = strlen(name);
	if (nameSize > B_FIELD_NAME_LENGTH) {
		*outResult = B_NAME_TOO_LONG;
		return This;
	}
	
	size_t size = chunk_align(kDataChunkSize + nameSize) + inoutInfo->size;
	This = ExpandData(chunkOff, chunk_align(size), indexPos);
	if (!This) {
		*outResult = B_NO_MEMORY;
		return this;
	}
	
	pos = (data_chunk*)(This->fBody + chunkOff);
	pos->header.code = DATA_CODE;
	pos->header.subcode = SINGLE_DATA_SUBCODE;
	pos->header.size = size;
	pos->type = inoutInfo->type;
	pos->entry_size = inoutInfo->size;
	pos->name_length = nameSize;
	strcpy(pos->name, name);
	chunk_pad(pos->name+nameSize+1);
	
	// Add this new chunk to the index.
	
	*((data_off*)(This->fBody + This->fHeader.fDataEnd
				+ sizeof(chunk_header) + sizeof(data_off)*indexPos)) = chunkOff;
			
	// Return position of data.
	
	inoutInfo->data = pos->data_struct();
	inoutInfo->pos = chunkOff;
	
	chunk_pad(((char*)inoutInfo->data) + inoutInfo->size);
	
	if (inoutInfo->type == B_ATOM_TYPE || inoutInfo->type == B_ATOMREF_TYPE) {
		BUMP_ATOM(This->fHeader.fAtomCount, 1);
	}
	
	*outResult = B_OK;
	return This;
}

BMessageBody* BMessageBody::ReAllocData(const char* name, data_info* inoutInfo,
										int32 index, status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::ReAllocData(" << this << ", name=" << name
			<< ", index=" << index << ")" << endl);
	
	BMessageBody* This = this;
	
	if (index < 0) {
		*outResult = B_BAD_INDEX;
		return This;
	}
	
	int32 indexPos;
	data_chunk* pos = const_cast<data_chunk*>(FindDataItem(name, &indexPos));
	
	if (!pos) {
		*outResult = B_NAME_NOT_FOUND;
		return This;
	}
	
	if (indexPos < 0) {
		*outResult = B_BAD_DATA;
		return This;
	}
	
	if (!cmp_types(pos->type, inoutInfo->type)) {
		*outResult = B_BAD_TYPE;
		return This;
	}
	
	data_off chunkOff = (data_off)((uint8*)pos - fBody);
	data_off dataOff = (data_off)((uint8*)pos->data_struct() - fBody);
	data_off dataEnd = 0;
	
	const size_t newSize = chunk_align(inoutInfo->size);
	
	if (fHeader.fAtomCount) {
		DATOM(DOUT << "Handling atoms in " << this << " for ReAlloc" << endl);
		if (pos->type == B_ATOM_TYPE || pos->type == B_ATOMREF_TYPE) {
			HandleAtoms(this, pos, AM_DECREMENT, index, 1);
		} else if (pos->type == B_MESSAGE_TYPE) {
			if (BumpAtoms(pos, AM_DECREMENT, index, 1)) {
				HandleAtoms(this, pos, AM_DECREMENT, index, 1);
			}
		}
	}
		
	switch (pos->header.subcode) {
		case SINGLE_DATA_SUBCODE: {
			if (index != 0) {
				*outResult = B_BAD_INDEX;
				return This;
			}
			const size_t oldSize = chunk_align(pos->entry_size);
			if (newSize > oldSize) {
				This = This->ExpandData(dataOff + oldSize, newSize - oldSize);
			} else if (newSize < oldSize) {
				This = This->ContractData(dataOff, oldSize - newSize);
			}
			if (!This) {
				*outResult = B_NO_MEMORY;
				return this;
			}
			pos = (data_chunk*)(This->fBody + chunkOff);
			pos->entry_size = inoutInfo->size;
			inoutInfo->data = pos->data_struct();
			dataEnd = dataOff + inoutInfo->size;
			chunk_pad(((char*)inoutInfo->data)+inoutInfo->size);
		} break;
		case FIXED_ARRAY_SUBCODE: {
			fixed_array* data = (fixed_array*)(This->fBody + dataOff);
			if (index >= (int32)data->count) {
				*outResult = B_BAD_INDEX;
				return This;
			}
			if (inoutInfo->size != pos->entry_size) {
				*outResult = B_MISMATCHED_VALUES;
				return This;
			}
			inoutInfo->data = data->data(pos->entry_size, index);
			dataEnd = fixed_array_end(This->fBody, dataOff, pos->entry_size);
		} break;
		case VARIABLE_ARRAY_SUBCODE: {
			variable_array* data = (variable_array*)(This->fBody + dataOff);
			if (index >= (int32)data->count) {
				*outResult = B_BAD_INDEX;
				return This;
			}
			size_t oldSize;
			data_off itemOff = variable_array_off(&oldSize, This->fBody, dataOff, index);
			const ssize_t delta = (newSize - chunk_align(oldSize));
			if (delta) {
				if (delta > 0) {
					This = This->ExpandData(itemOff + chunk_align(oldSize), delta);
				} else {
					This = This->ContractData(itemOff, -delta);
				}
				if (!This) {
					*outResult = B_NO_MEMORY;
					return this;
				}
				data = (variable_array*)(This->fBody + dataOff);
				data->total_size += delta;
				data_off* indicies = (data_off*)
					(This->fBody+variable_array_index(This->fBody, dataOff));
				uint32 i = index;
				indicies[i++] += (ssize_t)inoutInfo->size - (ssize_t)oldSize;
				while (i < data->count) indicies[i++] += delta;
			}
			
			inoutInfo->data = This->fBody + itemOff;
			dataEnd = next_chunk_off(This->fBody, chunkOff) + delta;
			chunk_pad(((char*)inoutInfo->data)+inoutInfo->size);
		} break;
		default: {
			DPARSE(DOUT << PREFIX_TEXT << ": Unknown data chunk type "
						<< (void*)(pos->header.subcode) << endl);
			*outResult = B_BAD_DATA;
			return This;
		}
	}
	
	inoutInfo->pos = chunkOff;
	
	pos = (data_chunk*)(This->fBody + chunkOff);
	pos->header.size = dataEnd - chunkOff;
	
	*outResult = B_OK;
	return This;
}

BMessageBody* BMessageBody::FreeData(const char* name, int32 index,
										status_t* outResult)
{
	D(DOUT << "BMessageBody::FreeData(" << this << ", name=" << name
			<< ", index=" << index << ")" << endl);
	
	int32 indexPos;
	data_chunk* pos = const_cast<data_chunk*>(FindDataItem(name, &indexPos));
	
	if (!pos) {
		*outResult = B_NAME_NOT_FOUND;
		return this;
	}
	
	if (indexPos < 0) {
		*outResult = B_BAD_DATA;
		return this;
	}
	
	data_off chunkOff = (data_off)((uint8*)pos - fBody);
	
	if (index == 0) {
		// Figure out if this is the one-and-only item in the field.
		if (pos->header.subcode == SINGLE_DATA_SUBCODE) index = -1;
		else if (pos->header.subcode == FIXED_ARRAY_SUBCODE) {
			const fixed_array* fa = (const fixed_array*)(pos->data_struct());
			if (fa->count == 1) index = -1;
		} else if (pos->header.subcode == VARIABLE_ARRAY_SUBCODE) {
			const variable_array* va = (const variable_array*)(pos->data_struct());
			if (va->count == 1) index = -1;
		}
	}
	
	if (index < 0) {
		// Delete the entire field.
		*outResult = B_OK;
		if (fHeader.fAtomCount) {
			DATOM(DOUT << "Handling atoms in " << this << " for FreeData" << endl);
			if (pos->type == B_ATOM_TYPE || pos->type == B_ATOMREF_TYPE) {
				BUMP_ATOM(fHeader.fAtomCount, -1);
				HandleAtoms(this, pos, AM_DECREMENT);
			} else if (pos->type == B_MESSAGE_TYPE) {
				if (BumpAtoms(pos, AM_DECREMENT)) {
					HandleAtoms(this, pos, AM_DECREMENT);
				}
			}
		}
		return ContractData(chunkOff, chunk_align(pos->header.size), indexPos);
		
	} else {
		// Delete only a single item in the field.
		if (pos->header.subcode == SINGLE_DATA_SUBCODE) {
			*outResult = B_BAD_INDEX;
			return this;
		}
		
		if (fHeader.fAtomCount) {
			DATOM(DOUT << "Handling atoms in " << this << " for FreeData (field)" << endl);
			if (pos->type == B_ATOM_TYPE || pos->type == B_ATOMREF_TYPE) {
				HandleAtoms(this, pos, AM_DECREMENT, index, 1);
			} else if (pos->type == B_MESSAGE_TYPE) {
				if (BumpAtoms(pos, AM_DECREMENT, index, 1)) {
					HandleAtoms(this, pos, AM_DECREMENT, index, 1);
				}
			}
		}
		
		if (pos->header.subcode == FIXED_ARRAY_SUBCODE) {
			fixed_array* fa = (fixed_array*)(pos->data_struct());
			if (index >= (int32)fa->count) {
				*outResult = B_BAD_INDEX;
				return this;
			}
			fa->count--;
			const size_t size = pos->entry_size;
			uint8* place = fa->data(size, index);
			
			const size_t reduction = chunk_align(pos->header.size)
								   - chunk_align(pos->header.size-size);
			if (reduction != 0) {
				// If this change causes the chunk size to reduce over a
				// chunk-aligned boundary, then we need to do a full contraction.
				
				pos->header.size -= size;
				const size_t chunkSize = pos->header.size;
				const data_off indexOff = chunkOff + chunk_align(pos->header.size);
				const data_off placeOff = (size_t)(place-fBody);
				
				// Move the array data inside the chunk.
				if (placeOff < chunkOff+chunkSize) {
					db_memcpy(place, place+size, chunkOff+chunkSize-placeOff);
				}
				
				// And reduce the size of the chunk.
				*outResult = B_OK;
				return ContractData(indexOff, reduction);
				
			} else {
				// The change in size does not go over a chunk-aligned
				// boundary, so just remove in-place.
				if ((size_t)(place-(uint8*)pos) < pos->header.size-size) {
					db_memcpy(place, place+size, pos->header.size-size - (place-(uint8*)pos));
				} else {
					chunk_pad(place+size);
				}
				pos->header.size -= size;
			}
			
		} else if (pos->header.subcode == VARIABLE_ARRAY_SUBCODE) {
			variable_array* va = (variable_array*)(pos->data_struct());
			if (index >= (int32)va->count) {
				*outResult = B_BAD_INDEX;
				return this;
			}
			
			// This is more complicated, because we need to contract both
			// the actual data and possibly its entry in the index.  First
			// move things around within the chunk, and then let the rest
			// of the code do the contraction at the end.
			size_t reduction;
			uint8* data = va->data(&reduction, index);
			reduction = chunk_align(reduction);
			
			// Offset the indicies.
			data_off* indicies = (data_off*)( ((uint8*)(va+1)) + va->total_size );
			va->total_size -= reduction;
			uint32 i = index;
			while (i < va->count) indicies[i++] -= reduction;
			
			// Move the data.
			db_memcpy(data, data+reduction,
					  (size_t)(indicies+index) - (size_t)(data+reduction));
			if (index < ((int32)va->count)-1) {
				db_memcpy(((uint8*)(indicies+index)) - reduction,
						  indicies+index+1, sizeof(data_off)*(va->count-index-1));
			}
			
			// Add in any size change from the index, and find the new end
			// of this chunk.
			const data_off origIndexSize = va->count*sizeof(data_off);
			reduction += chunk_align(origIndexSize)
					   - chunk_align(origIndexSize-sizeof(data_off));
			va->count--;
			uint8* place = ((uint8*)(va+1))
						 + va->total_size
						 + chunk_align(va->count*sizeof(data_off));
			pos->header.size = (size_t)(place-((uint8*)pos));
			
			// And now contract the data.
			*outResult = B_OK;
			return ContractData((size_t)(place-fBody), reduction);
		}
	}
	
	*outResult = B_OK;
	return this;
}

BMessageBody* BMessageBody::FastAddData(const char* name, data_info* inoutInfo,
										bool hintFixedSize, status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::FastAddData(" << this << ", name=" << name << ")" << endl);
	
	const void* data = inoutInfo->data;
	
	BMessageBody* This = AllocData(name, inoutInfo, hintFixedSize, outResult);
	if (*outResult != B_OK) return This;
	
	db_memcpy((void*)inoutInfo->data, data, inoutInfo->size);
	
	D(DOUT << "Done adding!" << endl);
	
	return This;
}

BMessageBody* BMessageBody::AddData(const char* name, data_info* inoutInfo,
									bool hintFixedSize, status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::AddData(" << this << ", name=" << name << ")" << endl);
	
	const void* data = inoutInfo->data;
	
	BMessageBody* This = AllocData(name, inoutInfo, hintFixedSize, outResult);
	if (*outResult != B_OK) return This;
	
	if (inoutInfo->type == B_ATOM_TYPE && inoutInfo->size == 4) {
		BAtom* a = *(BAtom**)data;
		DATOM(DOUT << "Adding atom data in " << this << " (acquire)" << endl);
		if (a) REF_ATOM(a, Acquire, This);
	} else if (inoutInfo->type == B_ATOMREF_TYPE && inoutInfo->size == 4) {
		BAtom* a = *(BAtom**)data;
		DATOM(DOUT << "Adding atom data in " << this << " (increfs)" << endl);
		if (a) REF_ATOM(a, IncRefs, This);
	} else if (inoutInfo->type == B_MESSAGE_TYPE) {
		if (This->BumpAtoms((const uint8*)data, inoutInfo->size, AM_INCREMENT)) {
			DATOM(DOUT << "Adding atom data in " << this << " (message)" << endl);
			This->HandleAtoms(This, (const uint8*)data, inoutInfo->size, AM_INCREMENT);
		}
	}
	
	db_memcpy((void*)inoutInfo->data, data, inoutInfo->size);
	
	D(DOUT << "Done adding!" << endl);
	
	return This;
}

BMessageBody* BMessageBody::FastReplaceData(const char* name, data_info* inoutInfo,
											int32 index, status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::FastReplaceData(" << this << ", name=" << name
			<< ", index=" << index << ")" << endl);
	
	const void* data = inoutInfo->data;
	
	BMessageBody* This = ReAllocData(name, inoutInfo, index, outResult);
	if (*outResult != B_OK) return This;
	
	db_memcpy((void*)inoutInfo->data, data, inoutInfo->size);
	
	D(DOUT << "Done replacing!" << endl);
	
	return This;
}

BMessageBody* BMessageBody::ReplaceData(const char* name, data_info* inoutInfo,
										int32 index, status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::ReplaceData(" << this << ", name=" << name
			<< ", index=" << index << ")" << endl);
	
	const void* data = inoutInfo->data;
	
	BMessageBody* This = ReAllocData(name, inoutInfo, index, outResult);
	if (*outResult != B_OK) return This;
	
	if (inoutInfo->type == B_ATOM_TYPE && inoutInfo->size == 4) {
		BAtom* a = *(BAtom**)data;
		DATOM(DOUT << "Adding atom data in " << this << " (acquire)" << endl);
		if (a) REF_ATOM(a, Acquire, This);
	} else if (inoutInfo->type == B_ATOMREF_TYPE && inoutInfo->size == 4) {
		BAtom* a = *(BAtom**)data;
		DATOM(DOUT << "Adding atom data in " << this << " (increfs)" << endl);
		if (a) REF_ATOM(a, IncRefs, This);
	} else if (inoutInfo->type == B_MESSAGE_TYPE) {
		if (This->BumpAtoms((const uint8*)data, inoutInfo->size, AM_INCREMENT)) {
			DATOM(DOUT << "Adding atom data in " << this << " (message)" << endl);
			This->HandleAtoms(This, (const uint8*)data, inoutInfo->size, AM_INCREMENT);
		}
	}
	
	db_memcpy((void*)inoutInfo->data, data, inoutInfo->size);
	
	D(DOUT << "Done replacing!" << endl);
	
	return This;
}

BMessageBody* BMessageBody::Rename(const char* name, const char* newName,
									status_t* outResult)
	DECLARE_RETURN(This)
{
	D(DOUT << "BMessageBody::Rename(" << this << ", name=" << name
			<< ", newName=" << newName << ")" << endl);
	
	BMessageBody* This = this;
	
	int32 oldIndex=-1;
	data_chunk* oldPos = const_cast<data_chunk*>(FindDataItem(name, &oldIndex));
	
	if (!oldPos) {
		*outResult = B_NAME_NOT_FOUND;
		return This;
	}
	
	if (oldIndex < 0) {
		*outResult = B_BAD_DATA;
		return This;
	}
	
	int32 newIndex=-1;
	data_chunk* newPos = const_cast<data_chunk*>(FindDataItem(newName, &newIndex));
	
	if (newPos) {
		*outResult = B_NAME_IN_USE;
		return This;
	}
	
	if (newIndex < 0) {
		*outResult = B_BAD_DATA;
		return This;
	}
	
	data_off chunkOff = (data_off)((uint8*)oldPos - fBody);
	
	const size_t newLen = strlen(newName);
	
	if (newLen >= 255) {
		*outResult = B_NAME_TOO_LONG;
		return This;
	}
	
	*outResult = This->MoveIndex(oldIndex, newIndex);
	if (*outResult != B_OK) {
		return This;
	}
	
	const ssize_t oldSize = chunk_align(kDataChunkSize + strlen(name));
	const ssize_t newSize = chunk_align(kDataChunkSize + newLen);
	
	if (newSize > oldSize) {
		This = This->ExpandData(chunkOff + kDataChunkSize, newSize - oldSize);
	} else if (newSize < oldSize) {
		This = This->ContractData(chunkOff + kDataChunkSize, oldSize - newSize);
	}
	if (!This) {
		// Need to return indicies to original locations.
		MoveIndex(newIndex, oldIndex);
		*outResult = B_NO_MEMORY;
		This = this;
		return This;
	}
	
	oldPos = (data_chunk*)(This->fBody + chunkOff);
	
	oldPos->header.size += (newSize-oldSize);
	oldPos->name_length = newLen;
	strcpy(oldPos->name, newName);
	chunk_pad(oldPos->name+newLen+1);
	
	*outResult = B_OK;
	return This;
}

// ---------------------------------------------------------------------------

const data_chunk* BMessageBody::FindDataItem(const char* name, int32* outIndex) const
{
	const size_t indexSize = ((const data_chunk*)(fBody+fHeader.fDataEnd))->header.size;
	const int32 N = (indexSize-sizeof(chunk_header)) / sizeof(data_off);
	
	// This condition is to determine whether to use a binary search to find
	// the requested item.  The rules are:
	// * If there are at least three items in the index, a binary
	//   search should always be used.
	// * Otherwise, if the location index is requested and there is at least
	//   one item in the index then a binary search must be used.
	if (N >= 3 || (outIndex != NULL && N > 0)) {
		
		const data_off* base = (const data_off*)(fBody+fHeader.fDataEnd+sizeof(chunk_header));
		int32 lower = 0;
		int32 upper = N - 1;
		ASSERT((data_off)(fHeader.fDataEnd+chunk_align((upper+1)*sizeof(data_off))+sizeof(chunk_header))
					== fHeader.fBodyEnd);

		// Look for data chunk with binary search.
		while( lower <= upper ) {
			int32 middle = lower + (upper-lower+1)/2;
			const data_off middleIndex = base[middle];
			if (middleIndex >= fHeader.fDataEnd) {
				DPARSE(DOUT << PREFIX_TEXT << ": Invalid index " << middleIndex
							<< "(#" << middle << ") in message body (end="
							<< fHeader.fDataEnd << ")" << endl);
				DPARSE(DOUT << PREFIX_TEXT << BHexDump(fBody, fBodySize) << endl);
				return NULL;
			}
			const int32 cmp = strcmp(name, ((const data_chunk*)(fBody+middleIndex))->name);
			if( cmp < 0 ) upper = middle-1;
			else if( cmp > 0 ) lower = middle+1;
			else {
				// This is the one we are looking for.
				if (outIndex) *outIndex = middle;
				return (const data_chunk*)(fBody+middleIndex);
			}
		}
		
		if (outIndex) {
			// At this point, 'upper' and 'lower' are around the last checked item.
			// Arbitrarily use 'upper' and determine the position where the
			// requested name would theoretically appear in the index.
			if( upper < 0 ) upper = 0;
			else if( upper < N ) {
				data_off upperIndex = base[upper];
				if (upperIndex >= fHeader.fDataEnd) {
					DPARSE(DOUT << PREFIX_TEXT << ": Invalid index " << upperIndex
								<< "(#" << upper << ") in message body (end="
								<< fHeader.fDataEnd << ")" << endl);
					DPARSE(DOUT << PREFIX_TEXT << BHexDump(fBody, fBodySize) << endl);
					return NULL;
				}
				if( strcmp(name, ((const data_chunk*)(fBody+upperIndex))->name) > 0 ) {
					upper++;
				}
			}
			
			if (upper >= N)
				*outIndex = N;					// Place at end.
			else
				*outIndex = upper;				// Place right here.
		}
		
		return NULL;
	}
	
	if (N > 0) {
		const data_chunk* pos = (const data_chunk*)fBody;
		while (pos->header.code != END_CODE) {
			if (pos->header.code == DATA_CODE) {
				if (strcmp(pos->name, name) == 0) return pos;
			}
			pos = (const data_chunk*)next_chunk((const uint8*)pos);
			if ((const uint8*)pos >= (fBody+fHeader.fDataEnd)) break;
		}
	}
	
	// Not found.  Presumably was because there was no data (i.e., the
	// data index is empty), so set the index for this chunk.
	if (outIndex) *outIndex = 0;
	
	return NULL;
}

const data_chunk* BMessageBody::FindDataItem(const char* name, type_code type,
											 int32 which, status_t* err,
											 const data_chunk* pos) const
{
	bool foundType = false, foundName = false;
	
	if (!pos) pos = (const data_chunk*)fBody;
	while ((const uint8*)pos >= fBody &&
			(const uint8*)pos < (fBody+fHeader.fDataEnd) &&
			pos->header.code != END_CODE) {
		if (pos->header.code == DATA_CODE) {
			if (cmp_types(type, pos->type)) {
				foundType = true;
				if (!name || strcmp(pos->name, name) == 0) {
					foundName = true;
					if (which <= 0) {
						if (err) *err = B_OK;
						return pos;
					}
					which--;
				}
			}
		}
		pos = (const data_chunk*)next_chunk((const uint8*)pos);
	}
	
	if (err) {
		if (!foundType) *err = B_BAD_TYPE;
		else if (!foundName) *err = B_NAME_NOT_FOUND;
		else *err = B_BAD_INDEX;
	}
	
	return 0;
}

BMessageBody* BMessageBody::Resize(size_t bodySize)
{
	if (!this) {
		BMessageBody* This = Create(bodySize);
		if (This) This->Acquire();
		return This;
	} else if (bodySize > fBodyAvail) {
		size_t newSize = chunk_align(bodySize) + sizeof(*this) - sizeof(fBody);
		BMessageBody* This = cache.Resize(this, newSize);
		D(DOUT << "BMessageBody: " << This << " resized to " << newSize
				<< " bytes from " << this << endl);
		if (!This) return NULL;
		This->fBodyAvail = chunk_align(bodySize);
		return This;
	}
	
	return this;
}

BMessageBody* BMessageBody::ExpandData(data_off where, size_t amount, int32 index)
	DECLARE_RETURN(This)
{
#if DEBUG
	// sanity checking
	if (where > fBodySize) {
		DOUT << "BMessageBody::ExpandData(): where " << (void*)where
			 << " is after data " << (void*)fBodySize
			 << (BHexDump(fBody, fBodySize).SetSingleLineCutoff(-1)) << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
		debugger("BMessageBody::ExpandData(): 'where' is after data");
	}
	if ((amount&0x3) != 0) {
		DOUT << "BMessageBody::ExpandData(): amount " << (void*)amount
			 << " is not chunk aligned" << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
		debugger("BMessageBody::ExpandData(): 'amount' is not chunk aligned");
	}
#endif

	const size_t indexSize = ((chunk_header*)(fBody+fHeader.fDataEnd))->size;
	const size_t dataEnd = fHeader.fDataEnd;
	
	size_t fullAmount = amount;
	if (index >= 0) {
		fullAmount += chunk_align(indexSize+sizeof(data_off)) - chunk_align(indexSize);
	}
	
	BMessageBody* This = this;
	if (fullAmount+fBodySize > fBodyAvail) {
		size_t newSize = chunk_align((fBodyAvail*3)/2);
		if (fullAmount+fBodySize > newSize) newSize = fullAmount+fBodySize;
		This = cache.Resize(This, newSize + sizeof(*this)-sizeof(fBody));
		D(DOUT << "BMessageBody: " << This << " resized to "
				<< (newSize + sizeof(*this)-sizeof(fBody))
				<< " bytes from " << this << endl);
		if (!This) return 0;
		This->fBodyAvail = newSize;
	}
	
	if (index >= 0) {
		data_off indexWhere = dataEnd
							+ sizeof(chunk_header)
							+ index*sizeof(data_off);
		const data_off indexEnd = dataEnd + indexSize;
		ASSERT(indexEnd <= This->fHeader.fBodyEnd);
		ASSERT(indexWhere >= This->fHeader.fDataEnd+sizeof(chunk_header));
		ASSERT(indexWhere <= indexEnd);
		((chunk_header*)(This->fBody+dataEnd))->size += sizeof(data_off);
		db_memmove(This->fBody+chunk_align(indexEnd)+fullAmount,
				This->fBody+chunk_align(indexEnd),
				This->fBodySize-chunk_align(indexEnd));
		if (indexEnd > indexWhere) {
			db_memmove(This->fBody+indexWhere+amount+sizeof(data_off),
					This->fBody+indexWhere,
					indexEnd-indexWhere);
		}
		chunk_pad(This->fBody + indexEnd + amount + sizeof(data_off));
		if (where < indexWhere) {
			db_memmove(This->fBody+where+amount, This->fBody+where,
					indexWhere-where);
		}
		This->fHeader.fBodyEnd += fullAmount;
	} else if (where < This->fBodySize) {
		db_memmove(This->fBody+where+amount, This->fBody+where,
				This->fBodySize-where);
		if (This->fHeader.fBodyEnd >= where) This->fHeader.fBodyEnd += amount;
	}
	
	if (where <= dataEnd) This->fHeader.fDataEnd += amount;
	if (where < dataEnd) This->OffsetIndicies(where, amount);
	This->fBodySize += fullAmount;
	
	#if DEBUG
	// Make sure offsets into body are correct.
	const int32 N = (((chunk_header*)(This->fBody+This->fHeader.fDataEnd))->size
					-sizeof(chunk_header)) / sizeof(data_off);
	if((data_off)(This->fHeader.fDataEnd+chunk_align(N*sizeof(data_off))+sizeof(chunk_header))
				!= This->fHeader.fBodyEnd) {
		DOUT << "BMessageBody::ExpandData(): End of offset table "
			 << (void*)(This->fHeader.fDataEnd+chunk_align(N*sizeof(data_off))+sizeof(chunk_header))
			 << " does not match body end " << (void*)(This->fHeader.fBodyEnd) << endl;
		debugger("BMessageBody::ExpandData(): End of offset table does not match body end");
	}
	#endif
	
	return This;
}

BMessageBody* BMessageBody::ContractData(data_off where, size_t amount, int32 index)
	DECLARE_RETURN(This)
{
#if DEBUG
	// sanity checking
	if (where > fBodySize) {
		DOUT << "BMessageBody::ContractData(): where " << (void*)where
			 << " is after data " << (void*)fBodySize
			 << (BHexDump(fBody, fBodySize).SetSingleLineCutoff(-1)) << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
		debugger("BMessageBody::ContractData(): 'where' is after data");
	}
	if ((amount&0x3) != 0) {
		DOUT << "BMessageBody::ContractData(): amount " << (void*)amount
			 << " is not chunk aligned" << endl;
		BCallStack stack;
		stack.Update(1);
		stack.LongPrint(DOUT);
		debugger("BMessageBody::ContractData(): 'amount' is not chunk aligned");
	}
#endif

	const size_t indexSize = ((chunk_header*)(fBody+fHeader.fDataEnd))->size;
	const size_t dataEnd = fHeader.fDataEnd;
		
	size_t fullAmount = amount;
	if (index >= 0) {
		fullAmount += chunk_align(indexSize) - chunk_align(indexSize-sizeof(data_off));
	}
	
	BMessageBody* This = this;
	
	// for now, never shrink message object
	//if (amount+fBodySize > fBodyAvail) {
	//	fBodyAvail *= 2;
	//	if (amont+fBodySize > fBodyAvail) fBodyAvail = amount+fBodySize;
	//	This = cache.Resize(This, fBodyAvail);
	//}
	
	if (index >= 0) {
		data_off indexWhere = dataEnd
							+ sizeof(chunk_header)
							+ index*sizeof(data_off);
		const data_off indexEnd = dataEnd + indexSize;
		ASSERT(indexEnd <= This->fHeader.fBodyEnd);
		ASSERT(indexWhere >= This->fHeader.fDataEnd+sizeof(chunk_header));
		ASSERT(indexWhere < indexEnd);
		((chunk_header*)(This->fBody+dataEnd))->size -= sizeof(data_off);
		if (where < indexWhere) {
			db_memcpy(This->fBody+where, This->fBody+where+amount,
					indexWhere-where-amount);
		}
		if (indexWhere < (indexEnd-sizeof(data_off))) {
			db_memcpy(This->fBody+indexWhere-amount,
					This->fBody+indexWhere+sizeof(data_off),
					indexEnd-indexWhere-sizeof(data_off));
		}
		chunk_pad(This->fBody + indexEnd - amount - sizeof(data_off));
		db_memcpy(This->fBody+chunk_align(indexEnd)-fullAmount,
				This->fBody+chunk_align(indexEnd),
				This->fBodySize-chunk_align(indexEnd));
		This->fHeader.fBodyEnd -= fullAmount;
	} else if (where < This->fBodySize) {
		db_memcpy(This->fBody+where, This->fBody+where+amount,
				This->fBodySize-where-amount);
		if (This->fHeader.fBodyEnd >= where) {
			if (This->fHeader.fBodyEnd >= where+amount) This->fHeader.fBodyEnd -= amount;
			else This->fHeader.fBodyEnd = where;
		}
	}
	
	if (where <= dataEnd) {
		if ((where+amount) <= dataEnd) This->fHeader.fDataEnd -= amount;
		else This->fHeader.fDataEnd = where;
		This->OffsetIndicies(where, -amount);
	}
	This->fBodySize -= fullAmount;
	
	#if DEBUG
	// Make sure offsets into body are correct.
	const int32 N = (((chunk_header*)(This->fBody+This->fHeader.fDataEnd))->size
					-sizeof(chunk_header)) / sizeof(data_off);
	if((data_off)(This->fHeader.fDataEnd+chunk_align(N*sizeof(data_off))+sizeof(chunk_header))
				!= This->fHeader.fBodyEnd) {
		DOUT << "BMessageBody::ContractData(): End of offset table "
			 << (void*)(This->fHeader.fDataEnd+chunk_align(N*sizeof(data_off))+sizeof(chunk_header))
			 << " does not match body end " << (void*)(This->fHeader.fBodyEnd) << endl;
		debugger("BMessageBody::ContractData(): End of offset table does not match body end");
	}
	#endif
	
	return This;
}

status_t BMessageBody::SwapBodyChunks(size_t* inoutSize, bool skipFirstHeader)
{
	uint8* pos = fBody;
	uint8* end = fBody+*inoutSize;
	
	while (pos < end) {
		if (pos+sizeof(chunk_header) > end) return B_NOT_A_MESSAGE;
		
		chunk_header* chunk = (chunk_header*)pos;
		if (!skipFirstHeader) {
			chunk->code = B_BYTE_SWAP_INT16(chunk->code);
			chunk->subcode = B_BYTE_SWAP_INT16(chunk->subcode);
			chunk->size = B_BYTE_SWAP_INT32(chunk->size);
		}
		
		pos += chunk->size;
		if (pos > end) return B_NOT_A_MESSAGE;
		
		bool skip = false;
		
		switch (chunk->code) {
			case DATA_CODE: {
				data_chunk* data = (data_chunk*)chunk;
				data->type = B_BYTE_SWAP_INT32(data->type);
				data->entry_size = B_BYTE_SWAP_INT32(data->entry_size);
				switch (chunk->subcode) {
					case SINGLE_DATA_SUBCODE: {
						if (((uint8*)data->data_struct())+data->entry_size > pos)
							return B_NOT_A_MESSAGE;
						swap_data(data->type, data->data_struct(),
								  data->entry_size, B_SWAP_ALWAYS);
					} break;
					case FIXED_ARRAY_SUBCODE: {
						fixed_array* item = (fixed_array*)data->data_struct();
						item->count = B_BYTE_SWAP_INT32(item->count);
						item->reserved0 = B_BYTE_SWAP_INT32(item->reserved0);
						if (item->data(data->entry_size, item->count) > pos)
							return B_NOT_A_MESSAGE;
						swap_data(data->type, item+1,
									data->entry_size*item->count, B_SWAP_ALWAYS);
					} break;
					case VARIABLE_ARRAY_SUBCODE: {
						variable_array* item = (variable_array*)data->data_struct();
						item->count = B_BYTE_SWAP_INT32(item->count);
						item->total_size = B_BYTE_SWAP_INT32(item->total_size);
						const int32 N = item->count;
						data_off* index = (data_off*)(((uint8*)(item+1))+item->total_size);
						if ((size_t)index < (size_t)(item+1)
								|| (size_t)index >= (size_t)pos)
							return B_NOT_A_MESSAGE;
						int32 i;
						for (i=0; i<N; i++) index[i] = B_BYTE_SWAP_INT32(index[i]);
						for (i=0; i<N; i++) {
							size_t size;
							uint8* buf = item->data(&size, i);
							if (buf < (uint8*)(item+1) || (buf+size) >= (uint8*)index)
								return B_NOT_A_MESSAGE;
							if (data->type == B_REF_TYPE) {
								entry_ref_swap((char*)buf, size);
							} else if (data->type != B_STRING_TYPE) {
								swap_data(data->type, buf, size, B_SWAP_ALWAYS);
							}
						}
					} break;
					default:
						skip = true;
						break;
				}
			} break;
			
			case INDEX_CODE: {
				const int32 N = (chunk->size-sizeof(chunk_header))/sizeof(data_off);
				data_off* off = (data_off*)(chunk+1);
				for (int32 i=0; i<N; i++) {
					*off = B_BYTE_SWAP_INT32(*off);
					off++;
				}
			} break;
			
			case END_CODE: {
				// End of the message -- stop now.
				*inoutSize -= (size_t)(end-pos);
				return B_OK;
			} break;
			
			default:
				skip = true;
				break;
		}
		
		if (skip) {
			// This is not a chunk we understand...  just remove
			// it from the message.
			TRESPASS();
			db_memcpy(pos-chunk->size, pos, end-pos);
			pos -= chunk->size;
			end -= chunk->size;
			*inoutSize -= chunk->size;
			continue;
		}
		
		skipFirstHeader = false;
	}
	
	return B_OK;
}

status_t BMessageBody::ParseBodyChunks(size_t bodySize)
{
	fBodySize = bodySize;
	
	// Quick exit -- if fDataEnd and fBodyEnd appear to be pointing
	// at the correct things, just bail.
	if (((chunk_header*)(fBody+fHeader.fDataEnd))->code == INDEX_CODE &&
		((chunk_header*)(fBody+fHeader.fBodyEnd))->code == END_CODE) {
		return B_OK;
	}
	
	// Hunt through body chunks for interesting places.
	
	fHeader.fDataEnd = 0;
	fHeader.fBodyEnd = 0;
	
	uint8* pos = fBody;
	uint8* end = fBody+bodySize;
	
	uint16 code = 0;
	while (pos >= fBody && pos < end &&
			(code=((chunk_header*)pos)->code) != END_CODE) {
		if (code == INDEX_CODE) fHeader.fDataEnd = (size_t)(pos-fBody);
		pos = next_chunk(pos);
	}
	
	if (code != END_CODE) return B_NOT_A_MESSAGE;
	
	fHeader.fBodyEnd = (size_t)(pos-fBody);
	
	// Make sure the chunk at fDataEnd is an index chunk (in the future,
	// we should generate an index if it isn't one).
	if (fHeader.fDataEnd > 0 && ((chunk_header*)(fBody+fHeader.fDataEnd))->code != INDEX_CODE) {
		return B_NOT_A_MESSAGE;
	}
	
	return B_OK;
}
