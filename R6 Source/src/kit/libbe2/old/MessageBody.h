/******************************************************************************
/
/	File:			MessageBody.h
/
/	Description:	Data contained in a BMessage object.
/
/	Copyright 2000, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _OLD_MESSAGEBODY_H
#define _OLD_MESSAGEBODY_H

#include "MessageFormat.h"
#include <support2/TypeConstants.h>	/* For convenience */
#include <string.h>

namespace B {

namespace Storage2 { struct entry_ref; }

namespace Support2 {
class BAtom;
class BDataIO;
}

namespace Old {
using namespace Storage2;

namespace BPrivate {

extern const char *B_SPECIFIER_ENTRY;
extern const char *B_PROPERTY_ENTRY;
extern const char *B_PROPERTY_NAME_ENTRY;

// optimization for gcc
#if __GNUC__
#define STANDARD_CALL __attribute__((stdcall))
#define ARITHMETIC_CALL __attribute__((stdcall,const))
#else
#define STANDARD_CALL
#define ARITHMETIC_CALL
#endif

class BMessageBody;

// This structure is used for temporary using a BMessageBody as
// flattened data.  Use with the StartWriting() and FinishWriting()
// methods.
struct message_write_context {
	const uint8* data;
	size_t size;

private:
	friend class BMessageBody;
	
	uint8 stackBuffer[MAX_HEADER_SIZE + sizeof(chunk_header)*2];
	uint8* allocBuffer;
	int32 alreadySending;
};

// This structure contains the message data that is included with
// each BMessage instance, and stored in the flattened form in
// the header area.  (Not included here is the message_target.)
struct header_args {
	uint32 what;
	int32 cur_specifier;
	bigtime_t when;
	bool has_when;
	
	inline void init()
	{
		what = 0;
		cur_specifier = -1;
		when = 0;
		has_when = false;
	}
	
	inline void init(uint32 in_what, int32 in_cur_specifier,
					 bigtime_t in_when, bool in_has_when)
	{
		what = in_what;
		cur_specifier = in_cur_specifier;
		when = in_when;
		has_when = in_has_when;
	}
};
	
// Code for flattening and unflattening entry_ref objects.
#define MAX_ENTRY_REF_SIZE (sizeof(dev_t) + sizeof(ino_t) + B_FILE_NAME_LENGTH)
status_t entry_ref_flatten(char* buffer, size_t* outSize, const entry_ref* ref);
status_t entry_ref_unflatten(entry_ref* outRef, const char* buffer, size_t size);
status_t entry_ref_swap(char* buffer, size_t size);

// Backwards compatibility -- these are implemented in MessageOld.cpp.

/*
// return a new BMessageBody from an old flattened message.
BMessageBody* unflatten_old_message(BDataIO* stream,
									header_args* outArgs,
									message_target* outTarget,
									status_t* outResult);
// construct on old flattened message from a BMessageBody.
ssize_t flatten_old_message(uint8* data,
							const BMessageBody* body,
							const header_args* args);
*/

/*
class BMemoryStreamIO : public BDataIO
{
public:
	BMemoryStreamIO(const void *p, size_t len, BDataIO* remaining=NULL);
	virtual ~BMemoryStreamIO();

	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

private:
	const void* fPosition;
	size_t fLength;
	BDataIO* fRemaining;
};
*/
// these are for the app_server to insert its own memory management
// functions; by default these are just malloc(), realloc(), and free()
extern void* (*message_malloc) (size_t size);
extern void* (*message_realloc) (void* ptr, size_t size);
extern void (*message_free) (void* ptr);

class BMessageBody {
public:
	// return a new, empty message body; reference count starts at 0
	static BMessageBody* Create() STANDARD_CALL;
	static BMessageBody* Create(size_t bodySize) STANDARD_CALL;
	
	// return a copy of this message body; reference count starts at 0
	// acquire()/increfs() is called for all atoms in the new message
	BMessageBody* Clone() const STANDARD_CALL;
	
	// increment and decrement reference count on this body
	// after last reference, release()/decrefs() is called for all atoms
	// in the message
	void Acquire() const STANDARD_CALL;
	void Release() const STANDARD_CALL;
	
	// set up to edit this body; returns either this object or a copy
	// of it, adjusting reference counts appropriately in the second
	// case (i.e., the new object is returned with a reference count of one,
	// the old has its reference count decremented)
	BMessageBody* Edit() const STANDARD_CALL;
	
	// clear body; returns either this object if it is not shared, or
	// NULL if there is another reference on it (and decrementing this
	// reference)
	// release()/decrefs() is called for all atoms if the same object
	// is returned
	BMessageBody* Reset() const STANDARD_CALL;
	
	// return true if there are no fields in this message
	bool IsEmpty() const ARITHMETIC_CALL;
	
	// return number of atom fields in this message
	int32 CountAtoms() const ARITHMETIC_CALL;
	
	enum {
		AM_INCREMENT	= (1<<1),
		AM_DECREMENT	= 0,
		AM_NO_RECURSION	= (1<<2)
	};
	
	// increment or decrement the atom count by the number of atoms in
	// the given flattened message
	void BumpAtoms(int32 delta) STANDARD_CALL;
	int32 BumpAtoms(const uint8* data, size_t size, uint32 mode) STANDARD_CALL;
	int32 BumpAtoms(const data_chunk* pos, uint32 mode,
					int32 index=0, int32 count=1<<30) STANDARD_CALL;
	
	// increment or decrement the reference count of atoms in message
	void HandleAtoms(void *tag, uint32 mode) const STANDARD_CALL;
	
	// versions of above that work on raw message data
	static void HandleAtoms(void *tag, const uint8* data, size_t size, uint32 mode,
							int32 hintNumAtoms = 0) STANDARD_CALL;
	static void HandleAtoms(void *tag, const data_chunk* pos, uint32 mode,
							int32 index=0, int32 count=1<<30) STANDARD_CALL;
					 
	// return true if this body contains scripting specifiers
	bool HasSpecifiers() const ARITHMETIC_CALL;
	void SetHasSpecifiers(bool state) STANDARD_CALL;
	
	// return the number of bytes in this message's body
	size_t BodySize() const ARITHMETIC_CALL;
	
	// return the total number of bytes for this structure and the body
	// data
	size_t FullSize() const ARITHMETIC_CALL;
	
	// return the number of bytes needed for this message's header
	size_t CalcHeaderSize(const header_args* args, bool inclTarget = false) const ARITHMETIC_CALL;
	
	// return the total number of bytes when this message is flattened;
	// if willSend is true, the header will also include target/reply info
	size_t CalcFlatSize(const header_args* args, bool inclTarget = false) const ARITHMETIC_CALL;
	
	// like above, but for fixed-sized headers
	size_t FixedHeaderSize() const ARITHMETIC_CALL;
	size_t FixedFlatSize() const ARITHMETIC_CALL;
	
	// flatten message into a stream or block of memory;
	// acquire()/increfs() is called for all atoms in the resulting
	// flattened data (including those inside sub-messages)
	/*
	status_t Flatten(const header_args* args, BDataIO* stream,
					 ssize_t* outSize,
					 const message_target* target=0,
					 bool fixedSize=false) const STANDARD_CALL;
	*/
	status_t Flatten(const header_args* args, char* buffer, ssize_t availSize,
					 const message_target* target=0,
					 bool fixedSize=false) const STANDARD_CALL;
	
	// unflatten stream or block of memory into message; can be called
	// on a NULL BMessageBody, which will create and return a new object
	// with a reference count starting at 1;
	// acquire()/increfs() is called for all atoms in the new bmessage
	// data (including those inside sub-messages)
	/*
	BMessageBody* Unflatten(BDataIO* stream, uint32 sig, header_args* outArgs,
							message_target* outTarget, status_t* outResult) STANDARD_CALL;
	*/
	BMessageBody* Unflatten(const char* data, size_t size, header_args* outArgs,
							message_target* outTarget, status_t* outResult) STANDARD_CALL;
	
	// flatten message directly into a port
	status_t WritePort(port_id port, int32 code,
					   const header_args* args, const message_target* target=0,
					   uint32 flags=0, bigtime_t timeout=B_INFINITE_TIMEOUT) const STANDARD_CALL;
	
	// unflatten message directly from a port
	BMessageBody* ReadPort(port_id port, ssize_t size,
						   int32* outCode, header_args* outArgs,
						   message_target* outTarget,
						   status_t* outResult,
						   uint32 flags=0, bigtime_t timeout=B_INFINITE_TIMEOUT) STANDARD_CALL;
	
	// flatten a message in-place, for temporary use in writing it somewhere
	status_t StartWriting(message_write_context* context, const header_args* args,
						  const message_target* target=0, bool fixedSize=true) const STANDARD_CALL;
	void FinishWriting(message_write_context* context) const STANDARD_CALL;
	
	// return number of field names in this message
	int32 CountNames(type_code type) const ARITHMETIC_CALL;
	
	// this is the information passed in and out of functions when
	// adding and removing data
	struct data_info {
		data_off pos;
		const char* name;
		type_code type;
		size_t count;
		size_t size;
		const void* data;
	};
	
	// find a particular field in the message; returns B_OK if found
	// the first form does a quick binary search, the second form is
	// slower but allows you to iterate over selected items
	status_t FindData(const char* name, data_info* outInfo, int32 index=-1) const STANDARD_CALL;
	status_t FindData(const char* name, type_code type, int32 which,
					  data_info* outInfo, int32 index=-1,
					  data_off* inoutCookie=0) const STANDARD_CALL;
	
	// find a particular field in the message with a known size and
	// copy its data
	status_t CopyData(const char* name, type_code type, int32 index,
					  void* destination, size_t size) const STANDARD_CALL;
	
	// allocate space in message; if a field with "name" does not
	// exist, a new one is created; otherwise, the existing field
	// is extended to include space for a new array element.
	// returns with inoutInfo->pos containing the location of the
	// targetted field, and inoutInfo->data pointing to the data
	// that was reserved
	BMessageBody* AllocData(const char* name, data_info* inoutInfo,
							bool hintFixedSize, status_t* outResult) STANDARD_CALL;
	
	// reallocate space in message; a field of "name" with data
	// at "index" must exist; returns with inoutInfo->pos containing
	// the location of the field, and inoutInfo->data pointing to
	// the data that was reallocated;
	// release()/decrefs() is called if this field is an atom or
	// a bmessage containing atoms; you must call acquire()/increfs()
	// for any new data you place into it
	BMessageBody* ReAllocData(const char* name, data_info* inoutInfo,
							  int32 index, status_t* outResult) STANDARD_CALL;
	
	// free space in a message; if "index" is < 0, the entire field
	// "name" is removed; otherwise, only the selected array element
	// is removed;
	// release()/decrefs() is called if this field is an atom or
	// a bmessage containing atoms
	BMessageBody* FreeData(const char* name, int32 index,
							status_t* outResult) STANDARD_CALL;
	
	// add a field to the message, either creating a new one or appending
	// to an existing one; returns with inoutInfo->pos set to the offset
	// of the changed field
	BMessageBody* FastAddData(const char* name, data_info* inoutInfo,
							  bool hintFixedSize, status_t* outResult) STANDARD_CALL;
	
	// like above, but acquire()/increfs() is called for a atom or
	// message containing atoms
	BMessageBody* AddData(const char* name, data_info* inoutInfo,
						  bool hintFixedSize, status_t* outResult) STANDARD_CALL;
	
	// replace data in an existing field; release()/decrefs() is called
	// if the old data is an atom or message containing atoms
	BMessageBody* FastReplaceData(const char* name, data_info* inoutInfo,
								  int32 index, status_t* outResult) STANDARD_CALL;
	
	// like above, but acquire()/increfs() is called for a atom or
	// message containing atoms
	BMessageBody* ReplaceData(const char* name, data_info* inoutInfo,
								int32 index, status_t* outResult) STANDARD_CALL;
	
	BMessageBody* Rename(const char* name, const char* newName,
						 status_t* outResult) STANDARD_CALL;
	
	const void* Body() const				{ return fBody; }

	// really private, but must be public for PPC
	enum {
		MBF_SENDING			= (1<<0),
		MBF_HAS_SPECIFIERS	= (1<<1)
	};
	
private:
	// this is the data that can be included in a flattened message
	// header, so that we can unflatten it without first instantiating
	// a BMessageBody
	struct header_data {
		mutable int32		fFlags;
		
		// position in body where next data block should be added; this is
		// also where the index chunk starts
		data_off			fDataEnd;
		
		// position in body where final end chunk is located
		data_off			fBodyEnd;
		
		// number of data entries that are atoms (B_ATOM_TYPE or B_ATOMREF_TYPE)
		int32				fAtomCount;
		
		int32				_reserved;
		
		inline void init_metadata() {
			atomic_and(&fFlags, ~MBF_HAS_SPECIFIERS);
		}
		inline void init_all() {
			fFlags = 0;
			fDataEnd = 0;
			fBodyEnd = 0;
			fAtomCount = 0;
			_reserved = 0;
		}
	};
	
	BMessageBody(size_t initialBodySize);
	~BMessageBody();
	
	/*
	friend BMessageBody* unflatten_old_message(BDataIO* stream, header_args* outArgs,
											message_target* outTarget, status_t* outResult);
	*/
	
	void InitData();
	void InitMetaData();
	
	friend inline BMessageBody* reserve_array_data(BMessageBody* This,
												   data_chunk* pos,
												   data_info* inoutInfo,
												   bool hintFixedSize,
												   status_t* outResult);
	
	// search for data quickly, only by name and optionally returning the item's
	// location in the index
	const data_chunk* FindDataItem(const char* name, int32* outIndex=NULL) const STANDARD_CALL;
	
	// search for data slowly, NULL to match every name, B_ANY_TYPE for every
	// type, 'which' is the number of matches to skip through, starting at the
	// item at 'pos'
	const data_chunk* FindDataItem(const char* name, type_code type, int32 which,
								   status_t* err, const data_chunk* pos=0) const STANDARD_CALL;
	
	// copy information about data in the chunk into 'outInfo'
	static status_t FillInData(data_info* outInfo,
								const data_chunk* chunk, int32 index) STANDARD_CALL;
	
	void OffsetIndicies(data_off insertion, int32 amount) STANDARD_CALL;
	status_t MoveIndex(int32 oldPos, int32 newPos) STANDARD_CALL;
	
	// Managing size of body.  Note that ContractData is guaranteed to never
	// fail -- if it can't do a realloc(), then it will keep the body the same
	// size and copy in-place.  (Not that it currently does a realloc() anyway,
	// but we must think about the future...)
	BMessageBody* Resize(size_t bodySize) STANDARD_CALL;
	BMessageBody* ExpandData(data_off where, size_t amount, int32 index=-1) STANDARD_CALL;
	BMessageBody* ContractData(data_off where, size_t amount, int32 index=-1) STANDARD_CALL;
	
	// write message header into buffer; buffer must be at least
	// MAX_HEADER_SIZE bytes large and the used size is returned in 'size'
	status_t CreateHeader(uint8* buffer, size_t* outSize,
						  const header_args* args,
						  const message_target* target=0, bool fixedSize=false) const STANDARD_CALL;
	
	// read a header chunk into this message body
	static status_t ParseHeaderChunk(const chunk_header* chunk, const uint8* data,
									 header_args* outArgs,
									 header_data* outHeader, message_target* outTarget,
									 bool swapping) STANDARD_CALL;
	
	// swap data in message body for reading a message that was
	// written with a different endian-ness
	status_t SwapBodyChunks(size_t* inoutSize, bool skipFirstHeader) STANDARD_CALL;
	
	// iterate through all body chunks to do a minimal sanity check
	// and note important offsets
	status_t ParseBodyChunks(size_t bodySize) STANDARD_CALL;
	
	// how many BMessage instances are using this body
	mutable int32		fRefCount;
	
	header_data			fHeader;
	
	// the amount of space available in fBody, and the amount that
	// has been used so far
	size_t				fBodyAvail;
	size_t				fBodySize;
	
	// reserved space for creating message header when sending
	mutable uint64		fBuffer[(MAX_HEADER_SIZE+sizeof(uint64)-1)/sizeof(uint64)];
	
	// the rest of this structure is the actual message data; by
	// default, reserve room for 2 8-byte size fields of not more
	// than 14 character length names.
	uint8				fBody[(	(kDataChunkSize+8+14)*2		// data
								+ sizeof(chunk_header)+8	// index
								+ sizeof(chunk_header)		// end
									+ 7) & ~7				// align
								];
};

// These are publically inlined for performance.

inline
void BMessageBody::Acquire() const
{
	atomic_add(&fRefCount, 1);
}

inline
BMessageBody* BMessageBody::Edit() const
{
	// The most common case is a body with only one holder, so
	// do that first.
	if (this) {
		if (fRefCount <= 1) return const_cast<BMessageBody*>(this);
	
	} else {
		// If called on non-existent object, create one.
		BMessageBody* o = Create();
		if (o) o->Acquire();
		return o;
	}
	
	// If more than one holder, we must copy the existing body and
	// return a new one for editing.
	BMessageBody* o = Clone();
	if (!o) return 0;
	
	o->Acquire();
	Release();
	return o;
}

inline
bool BMessageBody::IsEmpty() const
{
	return (this ? (fHeader.fDataEnd == 0) : true);
}

inline
int32 BMessageBody::CountAtoms() const
{
	return (this ? fHeader.fAtomCount : 0);
}

inline
bool BMessageBody::HasSpecifiers() const
{
	return (this ? (fHeader.fFlags&MBF_HAS_SPECIFIERS) : false);
}

inline
size_t BMessageBody::FixedHeaderSize() const
{
	return sizeof(message_header)
			+ sizeof(offsets_chunk)
			+ sizeof(script_chunk)
			+ sizeof(when_chunk)
			+ sizeof(target_chunk);
}

inline
size_t BMessageBody::CalcHeaderSize(const header_args* args, bool inclTarget) const
{
	return sizeof(message_header)
			+ (this ? sizeof(offsets_chunk) : 0)
			+ (HasSpecifiers() ? sizeof(script_chunk) : 0)
			+ (args->has_when ? sizeof(when_chunk) : 0)
			+ (inclTarget ? sizeof(target_chunk) : 0);
}

inline
size_t BMessageBody::FixedFlatSize() const
{
	return FixedHeaderSize() + BodySize();
}

inline
size_t BMessageBody::CalcFlatSize(const header_args* args, bool inclTarget) const
{
	return CalcHeaderSize(args, inclTarget) + BodySize();
}

inline
void BMessageBody::SetHasSpecifiers(bool state)
{
	if (state) atomic_or(&(fHeader.fFlags), MBF_HAS_SPECIFIERS);
	else atomic_and(&(fHeader.fFlags), ~MBF_HAS_SPECIFIERS);
}

inline
status_t BMessageBody::FindData(const char* name, data_info* outInfo, int32 index) const
{
	if (!this) return B_NAME_NOT_FOUND;
	
	const data_chunk* pos = FindDataItem(name);
	if (!pos) return B_NAME_NOT_FOUND;
	
	outInfo->pos = (data_off)((const uint8*)pos - fBody);
	return FillInData(outInfo, pos, index);
}

inline
status_t BMessageBody::CopyData(const char* name, type_code type, int32 index,
								void* destination, size_t size) const
{
	if (index < 0) return B_BAD_INDEX;
	
	data_info info;
	status_t err = FindData(name, &info, index);
	if (err != B_OK) return err;
	if (info.type != type) return B_BAD_TYPE;
	if (info.size != size) return B_MISMATCHED_VALUES;
	memcpy(destination, info.data, size);
	return B_OK;
}

} }	} // namespace B::Old::BPrivate

#undef STANDARD_CALL
#undef ARITHMETIC_CALL

#endif /* _MESSAGE_BODY_H */

