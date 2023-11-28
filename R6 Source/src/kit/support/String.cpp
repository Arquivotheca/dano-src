//
//	Copyright 1997, 1998, Be Incorporated, All Rights Reserved.
//

//  String class.
//  Has a very low footprint of just 1 pointer on the stack, same
// 	as char *.
//  Supports assigning, appending, truncating, string formating using <<
//  searching, replacing, removing
//
//	No ref counting yet
//	Not thread safe
//	some UTF8 support

#include <String.h>

#include <DataIO.h>
#include <Debug.h>
#include <InterfaceDefs.h>

#include <ctype.h>
#include <stdlib.h>


// inlines for dealing with UTF8
inline char
UTF8SafeTolower(char ch)
{
	return (((uint8)ch) < 0x80) ? tolower(ch) : ch;
}

inline char
UTF8SafeToUpper(char ch)
{
	return (((uint8)ch) < 0x80) ? toupper(ch) : ch;
}

inline bool
UTF8SafeIsAlpha(char ch)
{
	return (((uint8)ch) < 0x80) ? isalpha(ch) : false;
}

inline int32
UTF8CharLen(uchar ch)
{
	return ((0xe5000000 >> ((ch >> 3) & 0x1e)) & 3) + 1;
}

inline uint32
UTF8CharToUint32(const uchar *src, uint32 length)
{	
	uint32 result = 0;

	for (uint32 index = 0; index < length; index++)
		result |= src[index] << (24 - index * 8);

	return result;
}

const int32 kAllocaBufferThreshold = 64;

template <class ElementType>
class _AllocaBuffer {
public:
	// if small, use stack, if large, new
	
	_AllocaBuffer(int32 size)
		:	secondary(size > kAllocaBufferThreshold ? new ElementType [size] : 0)
		{ }
	~_AllocaBuffer()
		{
			delete [] secondary;
		}	
	
	operator ElementType *()
		{
			if (secondary)
				return secondary;

			return primary;
		}
		
	ElementType operator[](int32 index) const
		{
			if (secondary)
				return secondary[index];

			return primary[index];
		}

	ElementType & operator[](int32 index)
		{
			if (secondary)
				return secondary[index];

			return primary[index];
		}
		
private:
	ElementType primary[kAllocaBufferThreshold];
	ElementType *secondary;
};


// Checks only when DEBUG is turned on.  Otherwise, make these inlined
// empty functions.

#if DEBUG

void
BString::_SetUsingAsCString(bool on)
{
	if (!_privateData)
		return;

	if (on)
		*((uint32 *)_privateData - 1) |= 0x80000000;
	else
		*((uint32 *)_privateData - 1) &= ~0x80000000;
}

void 
BString::_AssertNotUsingAsCString() const
{
	if (_privateData)
		ASSERT((*((uint32 *)_privateData - 1) & 0x80000000) == 0);
}

#else

inline void
BString::_SetUsingAsCString(bool /*on*/)
{
}

inline void 
BString::_AssertNotUsingAsCString() const
{
}

#endif

// BString occupies just one word, <_privateData> on the stack.
// When non-empty, _privateData points to the second word of
// a malloced block, which is where the null terminated C-string
// starts. This makes String() trivial and fast.
//
// The length word is stored in the first word of the malloced block.
// To asscess it we go one word before where _privateData points.
// Ref-counting would be implemented by saving the ref count even
// before the length word in the malloced block.
// For this reason it is important that no inlines in the header
// file have any knwledge about these details so that ref-counting
// can be implemented in a compatible way just by rewriting the
// respective low-level calls in this file.
//
// The only thing the header file can rely on is the layout of the
// length word and the fact that _privateData points to the C-string.
//
// The most significant bit is reserved for future use; It doesn't
// matter in this file, but it needs to be properly stripped in
// the Length() inline in the String.h as the inline will be compiled
// into client binaries
//
// For now use the most significant bit for a debugging bit

BString::BString()
	:	_privateData(0)
{
}

BString::BString(const char *str)
{
	uint32 length = str ? strlen(str) : 0;
	_Init(str, length);
}

BString::BString(const char *str, int32 maxLength)
{
	uint32 length = str ? strlen(str) : 0;
	if (length > (uint32)maxLength)
		length = maxLength;

	_Init(str, length);
}

BString::BString(const BString &string)
{
	_Init(string.String(), string.Length());
}


BString::~BString()
{
	if (_privateData)
		free(_privateData - sizeof(int32));
}

void 
BString::_SetLength(int32 length)
{
	ASSERT(length > 0);
	ASSERT(_privateData);

	*((int32 *)_privateData - 1) = length;

}

void 
BString::_Init(const char *str, int32 length)
{
	_privateData = 0;
	ASSERT((!str && !length) || strlen(str) >= (uint32)length);
	
	if (length)
		_privateData = (char *)malloc(length + 1 + sizeof(int32));
	if (_privateData) {
		_privateData += sizeof(int32);
		strncpy(_privateData, str, length);
		_privateData[length] = '\0';
		_SetLength(length);
	}
}

int32 
BString::CountChars() const
{
	int32 numChars = 0;
	int32 len = Length();

	for (int32 i = 0; i < len; i += UTF8CharLen(_privateData[i])) 
		numChars++;

	return numChars;
}


// memory management calls

void 
BString::_DoAssign(const char *str, int32 length)
{
	_AssertNotUsingAsCString();
	ASSERT((!str && !length) || length <= (int32)strlen(str));
	
	if (length) {
		if (length != Length()) {
			_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
				: 0, length + 1 + sizeof(int32));	
			if (!_privateData) 
				return;
			_privateData += sizeof(int32);
		}

		strcpy(_privateData, str);
		_privateData[length] = '\0';
		_SetLength(length);
		return;
	}
	if (_privateData) 
		free(_privateData - sizeof(int32));
	_privateData = 0;
}

void 
BString::_DoAppend(const char *str, int32 length) 
{
	_AssertNotUsingAsCString();
	ASSERT((!str && !length) || length <= (int32)strlen(str));

	if (!str || !length)
		return;

	int32 oldLength = Length();
	int32 newLength = length + oldLength;
	_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
		: 0, newLength + 1 + sizeof(int32));	
	if (_privateData) {
		_privateData += sizeof(int32);
		strncpy(_privateData + oldLength, str, length);
		_SetLength(newLength);
		_privateData[newLength] = '\0';
	}
}

char * 
BString::_GrowBy(int32 length)
{
	// caller needs to fill the resulting space with something
	_AssertNotUsingAsCString();
	ASSERT(length);
	int32 oldLength = Length();
	int32 newLength = length + oldLength;
	_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
		: 0, newLength + 1 + sizeof(int32));	
	if (_privateData) {
		_privateData += sizeof(int32);
		_SetLength(newLength);
		_privateData[newLength] = '\0';
		return _privateData + oldLength;
	}
	return 0;
}

void 
BString::_DoPrepend(const char *str, int32 length) 
{
	_AssertNotUsingAsCString();
	ASSERT((!str && !length) || length <= (int32)strlen(str));

	if (!str || !length)
		return;

	char *newData = (char *)calloc(Length() + length + 1 + sizeof(int32),
		sizeof(char));
	if (newData) {
		newData += sizeof(int32);
		strncpy(newData, str, length);
		int32 oldLength = Length();
		if (_privateData) {
			strncpy(newData + length, _privateData, oldLength);
			free(_privateData - sizeof(int32));
		}
		_privateData = newData;
		_SetLength(oldLength + length);
		_privateData[oldLength + length] = '\0';
	}
}

char *
BString::_OpenAtBy(int32 offset, int32 length)
{
	_AssertNotUsingAsCString();
	ASSERT(length);
	ASSERT(length > 0);
	int32 oldLength = Length();
	int32 newLength = length + oldLength;
	
	// if we knew that realloc will result in a memory move, we could
	// use malloc and memcopy instead to avoid memcopying to the wrong
	// place first and then moving it to the right one
	//
	// could use a heuristic here, doing the above if <length> is "a lot"
	
	_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
		: 0, newLength + 1 + sizeof(int32));	
	if (_privateData) {
		_privateData += sizeof(int32);
		_SetLength(newLength);
		_privateData[newLength] = '\0';
		memmove(_privateData + offset + length, _privateData + offset,
			oldLength - offset);
		return _privateData + offset;
	}
	return 0;
}

char *
BString::_ShrinkAtBy(int32 offset, int32 length)
{
	int32 newLength = Length();
	ASSERT(length && offset + length <= newLength);
	int32 fromOffset = offset + length;
	memcpy(_privateData + offset, _privateData + fromOffset, newLength - 
		fromOffset);

	newLength -= length;
	if (newLength > 0) {
		_privateData = (char *)realloc(_privateData - sizeof(int32),
			newLength + 1 + sizeof(int32));

		if (_privateData) {
			_privateData += sizeof(int32);
			_SetLength(newLength);
			_privateData[newLength] = '\0';
			return _privateData;
		}
	} else {
		_DoAssign(NULL, 0);
	}
	
	return 0;
}

BString &
BString::SetTo(const char *str, int32 newLength)
{
	_AssertNotUsingAsCString();

	if (!str)
		newLength = 0;

	int32 tmpLength = str ? strlen(str) : 0;
	if (newLength < 0 || newLength > tmpLength)
		newLength = tmpLength;

	if (newLength) {
		int32 length = Length();

		if (newLength != length) {
		
			if (str >= _privateData && str <= _privateData + length) {
	
				// handle self assignment case
				char *tmp1 = (char *)malloc(length + 1 + sizeof(int32));
				if (!tmp1)
					return *this;
				
				tmp1 += sizeof(int32);
				strncpy(tmp1, str, newLength);
				tmp1[newLength] = '\0';
				char *tmp2 = _privateData;
				_privateData = tmp1;
				_SetLength(newLength);
				free(tmp2 - sizeof(int32));
				return *this;

			} 
			_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
				: 0, newLength + 1 + sizeof(int32));	
			if (!_privateData) 
				return *this;
			_privateData += sizeof(int32);

		}

		strncpy(_privateData, str, newLength);
		_privateData[newLength] = '\0';
		_SetLength(newLength);
		return *this;
	}
	
	if (_privateData) 
		free(_privateData - sizeof(int32));
	_privateData = 0;
	return *this;
}

BString &
BString::SetTo(char ch, int32 length)
{
	_AssertNotUsingAsCString();
	if (length) {
		if (length != Length()) {
			_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
				: 0, length + 1 + sizeof(int32));	
			if (!_privateData) 
				return *this;
			_privateData += sizeof(int32);
		}

		memset(_privateData, ch, length);
		_privateData[length] = '\0';
		_SetLength(length);
		return *this;
	}
	
	if (_privateData) 
		free(_privateData - sizeof(int32));
	_privateData = 0;
	return *this;
}

void
BString::Swap(BString& with)
{
	char* tmp = _privateData;
	_privateData = with._privateData;
	with._privateData = tmp;
}

static char* gEmptyString = "";

char &
BString::operator[](int32 index)
{
	// this needs to be out of line so that we can do
	// ref-counting in the future
	// if there is no string, we need to return -something-
	// so that the caller doesn't get an invalid reference;
	// they should never try to modify the (zero length)
	// string in this situation
	return _privateData ? _privateData[index] : gEmptyString[index];
}

BString &
BString::operator=(const BString &string)
{
	if (&string != this)
		// guard against self asignment
		_DoAssign(string.String(), string.Length());

	return *this;
}

BString &
BString::operator=(const char *str)
{
	int32 newLength = str ? strlen(str) : 0;
	int32 oldLength = Length();
	if (_privateData && newLength && str >= _privateData
		&& str <= _privateData + oldLength) {
		// self-assigning, special case
		if (str == _privateData)
			// trivial case
			return *this;

		ASSERT(newLength < oldLength);
		char *tmp1 = (char *)malloc(newLength + 1 + sizeof(int32));
		if (!tmp1)
			return *this;
		
		tmp1 += sizeof(int32);
		strcpy(tmp1, str);
		char *tmp2 = _privateData;
		_privateData = tmp1;
		_SetLength(newLength);
		free(tmp2 - sizeof(int32));
	} else
		_DoAssign(str, newLength);
		
	return *this;
}

BString &
BString::SetTo(const BString &string)
{
	if (&string != this)
		_DoAssign(string.String(), string.Length());

	return *this;
}

BString &
BString::Adopt(BString &string)
{
	if (&string == this)
		return *this;

	if (_privateData)
		free(_privateData - sizeof(int32));

	_privateData = string._privateData;
	string._privateData = 0;

	return *this;
}

BString &
BString::SetTo(const BString &string, int32 length)
{
	if (&string == this)
		return *this;

	if (string.Length() < length)
		length = string.Length();

	SetTo(string.String(), length);

	return *this;
}

BString &
BString::Adopt(BString &string, int32 length)
{
	if (&string == this)
		return *this;

	if (_privateData)
		free(_privateData - sizeof(int32));

	_privateData = string._privateData;
	string._privateData = 0;
	
	// don't realloc, just trim the result
	Truncate(length);

	return *this;
}

BString &
BString::CopyInto(BString &string, int32 fromOffset, int32 length) const
{
	if (&string == this)
		return string;

	if (length >= 0) {
		if (fromOffset+length > Length())
			length = Length()-fromOffset;
		if (length < 0)
			return string;
	}
	
	return string.SetTo(String() + fromOffset, length);
}

void 
BString::CopyInto(char *into, int32 fromOffset, int32 length) const
{
	if (!into)
		return;

	if (length >= 0) {
		if (fromOffset+length > Length())
			length = Length()-fromOffset;
		if (length < 0)
			return;
	}
	
	memcpy(into, String() + fromOffset, length);
}

BString &
BString::operator+=(const char *str)
{
	int32 length = str ? strlen(str) : 0;
	_DoAppend(str, length);
	return *this;
}

BString &
BString::operator+=(char ch)
{
	const char tmp[2] = {ch, '\0'};
	_DoAppend(tmp, 1);
	return *this;
}

BString &
BString::Append(const BString &string, int32 length)
{
	if (&string != this) {
		if (string.Length() < length)
			length = string.Length();
	
		_DoAppend(string.String(), length);
	}
	return *this;
}

BString &
BString::Append(const char *str, int32 length)
{
	int32 tmpLength = str ? strlen(str) : 0;

	if (tmpLength < length)
		length = tmpLength;

	_DoAppend(str, length);
	return *this;
}

BString &
BString::Append(char ch, int32 count)
{
	char *fillStart = _GrowBy(count);
	if (fillStart)
		memset(fillStart, ch, count);
	
	return *this;
}

BString &
BString::Prepend(const char *str)
{
	int32 tmpLength = str ? strlen(str) : 0;

	_DoPrepend(str, tmpLength);
	return *this;
}

BString &
BString::Prepend(const BString &string)
{
	if (&string != this) 
		_DoPrepend(string.String(), string.Length());

	return *this;
}

BString &
BString::Prepend(const char *str, int32 length)
{
	int32 tmpLength = str ? strlen(str) : 0;

	if (tmpLength < length)
		length = tmpLength;

	_DoPrepend(str, length);
	return *this;
}

BString &
BString::Prepend(const BString &string, int32 length)
{
	if (&string != this) {
		if (string.Length() < length)
			length = string.Length();
	
		_DoPrepend(string.String(), length);
	}
	return *this;
}

BString &
BString::Prepend(char ch, int32 count)
{
	char *fillStart = _OpenAtBy(0, count);
	if (fillStart)
		memset(fillStart, ch, count);
	
	return *this;
}

BString &
BString::Insert(const char *str, int32 pos)
{
	if (str) {
		int32 length = strlen(str);
		
		char *fillStart = _OpenAtBy(pos, length);
		if (fillStart)
			memcpy(fillStart, str, length);
	}
	return *this;
}

BString &
BString::Insert(const char *str, int32 length, int32 pos)
{
	ASSERT((!str && !length) || length <= (int32)strlen(str));

	if (!str)
		length = 0;

	if (length) {
		int32 tmpLength = strlen(str);
		if (tmpLength < length)
			length = tmpLength;
		
		char *fillStart = _OpenAtBy(pos, length);
		if (fillStart)
			memcpy(fillStart, str, length);
	}
	return *this;
}

BString &
BString::Insert(const char *str, int32 fromOffset, int32 length, int32 pos)
{
	ASSERT((!str && !length) || length <= (int32)strlen(str));

	if (str)
		str += fromOffset;
	else
		length = 0;

	if (length) {
		int32 tmpLength = strlen(str);
		if (tmpLength < length)
			length = tmpLength;
		
		char *fillStart = _OpenAtBy(pos, length);
		if (fillStart)
			memcpy(fillStart, str, length);
	}
	return *this;
}

BString &
BString::Insert(const BString &string, int32 pos)
{
	int32 length = string.Length();
	if (&string != this && length) {
		char *fillStart = _OpenAtBy(pos, length);
		if (fillStart)
			memcpy(fillStart, string._privateData, length);
	}
	return *this;
}

BString &
BString::Insert(const BString &string, int32 length, int32 pos)
{
	int32 stringLength = string.Length();
	if (&string != this) {
		if (stringLength < length)
			length = stringLength;

		if (length) {
			char *fillStart = _OpenAtBy(pos, length);
			if (fillStart)
				memcpy(fillStart, string._privateData, length);
		}
	}
	return *this;
}

BString &
BString::Insert(const BString &string, int32 fromOffset, int32 length, int32 pos)
{
	int32 stringLength = string.Length();
	if (&string != this) {
		if (stringLength - fromOffset < length)
			length = stringLength - fromOffset;

		if (length) {
			char *fillStart = _OpenAtBy(pos, length);
			if (fillStart)
				memcpy(fillStart, string._privateData + fromOffset, length);
		}
	}
	return *this;
}

BString &
BString::Insert(char ch, int32 count, int32 pos)
{
	if (count) {
		char *fillStart = _OpenAtBy(pos, count);
		if (fillStart)
			memset(fillStart, ch, count);
	}
	return *this;
}

BString &
BString::Truncate(int32 newLength, bool lazy)
{
	_AssertNotUsingAsCString();
	if (_privateData && newLength < Length()) {
		if (!newLength) {
			free(_privateData - sizeof(int32));
			_privateData = 0;
			return *this;
		}
		if (!lazy) {
			if (newLength != Length()) {
				_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
					: 0, newLength + 1 + sizeof(int32));	
				if (!_privateData) 
					return *this;
				_privateData += sizeof(int32);
			}
		}
		_SetLength(newLength);
		_privateData[Length()] = '\0';
	}

	return *this;
}

BString &
BString::Remove(int32 from, int32 length)
{
	int32 oldLength = Length();
	if (from + length <= oldLength)
		_ShrinkAtBy(from, length);
	return *this;
}

BString &
BString::RemoveFirst(const BString &string)
{
	int32 length = string.Length();
	if (length) {
		int32 index = FindFirst(string);
		if (index >= 0)
			_ShrinkAtBy(index, length);
	}
	return *this;
}

BString &
BString::RemoveLast(const BString &string)
{
	int32 length = string.Length();
	if (length) {
		int32 index = FindLast(string);
		if (index >= 0)
			_ShrinkAtBy(index, length);
	}
	return *this;
}

BString &
BString::RemoveAll(const BString &pattern)
{
	int32 length = pattern.Length();
	if (length) {
		for (int32 index = 0;;) {
			index = FindFirst(pattern, index);
			if (index < 0)
				break;
			_ShrinkAtBy(index, length);
		}
	}
	return *this;
}

BString &
BString::RemoveFirst(const char *str)
{
	int32 length = str ? strlen(str) : 0;
	if (length) {
		int32 index = FindFirst(str);
		if (index >= 0)
			_ShrinkAtBy(index, length);
	}
	return *this;
}

BString &
BString::RemoveLast(const char *str)
{
	int32 length = str ? strlen(str) : 0;
	if (length) {
		int32 index = FindLast(str);
		if (index >= 0)
			_ShrinkAtBy(index, length);
	}
	return *this;
}

BString &
BString::RemoveAll(const char *str)
{
	int32 length = str ? strlen(str) : 0;
	if (length) {
		for (int32 index = 0;;) {
			index = FindFirst(str, index);
			if (index < 0)
				break;
			_ShrinkAtBy(index, length);
		}
	}
	return *this;
}

BString &
BString::RemoveSet(const char *setOfCharsToRemove)
{
	bool map[128] = {
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false
	};

	bool dealWithUTF8 = false;
	for (const char *tmp = setOfCharsToRemove; *tmp; ) {
		if (*tmp & 0x80) {
			dealWithUTF8 = true;
			tmp += UTF8CharLen(*tmp);
		} else
			map[*tmp++] = true;
	}

	int32 length = Length();
	if (dealWithUTF8) {
		for (int32 index = 0; index < length;) {
			const unsigned char c = (unsigned char)(_privateData[index]);
			if (c <= 127) {
				if (map[c]) {
					int32 fromOffset = index + 1;
					memcpy( _privateData + index, _privateData + fromOffset,
						length - fromOffset);
					length -= 1;
				} else
					 index++;
			} else {
				const int32 utflen = UTF8CharLen(c);
				const uint32 utf = UTF8CharToUint32((uchar*)(_privateData + index), utflen);
				bool found = false;
				for (const char *tmp = setOfCharsToRemove; *tmp && !found; ) {
					if (*tmp & 0x80) {
						const int32 len = UTF8CharLen(*tmp);
						if (utf == UTF8CharToUint32((const uchar*)tmp, len)) {
							const int32 fromOffset = index + len;
							memcpy( _privateData + index, _privateData + fromOffset,
								length - fromOffset);
							length -= len;
							found = true;
						} else {
							tmp += len;
						}
					} else {
						tmp++;
					}
				}
				if (!found) index += utflen;
			}
		}
	} else {
		for (int32 index = 0; index < length;)
			if (map[_privateData[index]]) {
				int32 fromOffset = index + 1;
				memcpy( _privateData + index, _privateData + fromOffset,
					length - fromOffset);
				length -= 1;
			} else
				 index++;
	}
	if (length < Length())
		Truncate(length);

	return *this;
}

BString &
BString::Compact(const char *setOfCharsToCompact)
{
	_DoCompact(setOfCharsToCompact, NULL);
	return *this;
}

BString &
BString::Compact(const char *setOfCharsToCompact, char* replacementChar)
{
	_DoCompact(setOfCharsToCompact, replacementChar);
	return *this;
}

void
BString::_DoCompact(const char* setOfCharsToCompact, const char* replace)
{
	bool map[128] = {
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false
	};

	bool dealWithUTF8 = false;
	for (const char *tmp = setOfCharsToCompact; *tmp; ) {
		if (*tmp & 0x80) {
			dealWithUTF8 = true;
			tmp += UTF8CharLen(*tmp);
		} else
			map[*tmp++] = true;
	}

	char* pos = _privateData;
	char* end = _privateData+Length();
	if (dealWithUTF8) {
		debugger("UTF8 is not yet supported for this operation");
		pos = end;
	} else {
		char* buf = pos;
		bool lastSpace = true;
		while (buf < end) {
			if (map[*buf]) {
				if (!lastSpace) {
					if (replace)
						*pos = *replace;
					pos++;
					lastSpace = true;
				}
			} else {
				if (pos < buf) *pos = *buf;
				lastSpace = false;
				pos++;
			}
			buf++;
		}
		if (pos > _privateData && lastSpace)
			pos--;
	}
	if (pos < end)
		Truncate((int32)(pos-_privateData));
}


BString &
BString::MoveInto(BString &into, int32 from, int32 length)
{
	if (length) {
		CopyInto(into, from, length);
		Remove(from, length);
	}
	return into;
}

void
BString::MoveInto(char *into, int32 from, int32 length)
{
	if (length) {
		CopyInto(into, from, length);
		Remove(from, length);
	}
}

bool 
BString::operator<(const char *str) const
{
	_AssertNotUsingAsCString();
	// can't pass zero pointers to strcmp
	if (!Length())
		return true;
	
	if (!str)
		return false;

	return strcmp(str, _privateData) > 0;
}

bool 
BString::operator<=(const char *str) const
{
	_AssertNotUsingAsCString();
	// can't pass zero pointers to strcmp
	if (!Length())
		return true;
	
	if (!str)
		return false;

	return strcmp(str, _privateData) >= 0;
}

bool 
BString::operator==(const char *str) const
{
	_AssertNotUsingAsCString();
	// can't pass zero pointers to strcmp
	if (!Length() || !str)
		return (!str || *str == 0) == (Length() == 0);

	return strcmp(str, _privateData) == 0;
}

bool 
BString::operator>=(const char *str) const
{
	_AssertNotUsingAsCString();
	// can't pass zero pointers to strcmp
	if (!str)
		return true;
	
	if (!Length())
		return false;

	return strcmp(str, _privateData) <= 0;
}

bool 
BString::operator>(const char *str) const
{
	_AssertNotUsingAsCString();
	// can't pass zero pointers to strcmp
	if (!str)
		return true;
	
	if (!Length())
		return false;

	return strcmp(str, _privateData) < 0;
}

int 
BString::Compare(const BString &string) const
{
	return strcmp(String(), string.String());
}

int 
BString::Compare(const char *str) const
{
	if (!str)
		return Length() ? 1 : 0;
	return strcmp(String(), str);
}

int 
BString::Compare(const BString &string, int32 n) const
{
	return strncmp(String(), string.String(), n);
}

int 
BString::Compare(const char *str, int32 n) const
{
	if (!str)
		return Length() ? 1 : 0;
	return strncmp(String(), str, n);
}

int 
BString::ICompare(const BString &string) const
{
	return strcasecmp(String(), string.String());
}

int 
BString::ICompare(const char *str) const
{
	if (!str)
		return Length() ? 1 : 0;
	return strcasecmp(String(), str);
}

int 
BString::ICompare(const BString &string, int32 n) const
{
	return strncasecmp(String(), string.String(), n);
}

int 
BString::ICompare(const char *str, int32 n) const
{
	if (!str)
		return Length() ? 1 : 0;
	return strncasecmp(String(), str, n);
}

const int32 kShortFindAfterTreshold = 100*1024;

inline int32 
BString::_ShortFindAfter(const char *str, int32 fromOffset) const
{
	const char *s1 = _privateData + fromOffset;
	const char *p1 = str;
	char firstc, c1;
	
	if ((firstc = *p1++) == 0)
		return 0;

	while((c1 = *s1++) != 0)
		if (c1 == firstc)
		{
			const char *s2 = s1;
			const char *p2 = p1;
			char c2;

			while ((c1 = *s2++) == (c2 = *p2++) && c1)
				;
			
			if (!c2)
				return (s1 - 1) - _privateData;
		}
	
	return -1;
}


int32 
BString::_FindAfter(const char *str, int32 length, int32 fromOffset) const
{
	int32 stringLength = Length();
	if (length > stringLength)
		return -1;

	ASSERT(str && length && strlen(str) >= (uint32)length);
	ASSERT(stringLength - fromOffset >= length);
	
	// KMP search
	_AllocaBuffer<int32> next(length + 1);
	
	int32 i = 0;
	int32 j = -1;
	next[0] = -1;
	do {
		if (j == -1 || str[i] == str[j]) {
			i++;
			j++;
			next[i] = (str[j] == str[i]) ? next[j] : j;
		} else
			j = next[j];
	} while (i < length);
	
	int32 result = -1;
	for (int32 i = fromOffset, j = 0; i < stringLength; ) {
		if (j == -1 || str[j] == _privateData[i]) {
			i++;
			j++;
			if (j >= length) {
				result = i - j;
				break;
			}
		} else
			j = next[j];
	}

#if DEBUG
	// double-check our results using a slow but simple approach
	char *debugPattern = strdup(str);
	debugPattern[length] = '\0';
	char *tmpCheck = strstr(_privateData + fromOffset, debugPattern);
	ASSERT((!tmpCheck && result == -1) || (tmpCheck - _privateData == result)); 
	free (debugPattern);
#endif

	return result;
}

int32 
BString::_IFindAfter(const char *str, int32 length, int32 fromOffset) const
{
	_AllocaBuffer<char> pattern(length + 1);
	for (int32 index = 0; index < length; index++)
		pattern[index] = UTF8SafeTolower(str[index]);
	pattern[length] = '\0';

	const char *s1 = _privateData + fromOffset;
	const char *p1 = pattern;
	char firstc, c1;
	
// printf(">>>>>>pattern>%s<\n", p1);
	if ((firstc = *p1++) == 0)
		return 0;

	while((c1 = *s1++) != 0)
		if (UTF8SafeTolower(c1) == firstc)
		{
			const char *s2 = s1;
			const char *p2 = p1;
			char c2;

			while (UTF8SafeTolower(c1 = *s2++) == (c2 = *p2++) && c1)
				;
			
			if (!c2)
				return s1 - 1 - _privateData;
		}
	
	return -1;

#if 0
	int32 stringLength = Length();
	if (length > stringLength)
		return -1;

	ASSERT(str && length && strlen(str) >= length);
	ASSERT(stringLength - fromOffset >= length);
	
	_AllocaBuffer<int32> pattern(length + 1);
	for (int32 index = 0; index < length; index++)
		pattern[index] = UTF8SafeTolower(str[index]);

	// KMP search
	_AllocaBuffer<int32> next(length + 1);
	
	int32 i = 0;
	int32 j = -1;
	next[0] = -1;
	do {
		if (j == -1 || pattern[i] == pattern[j]) {
			i++;
			j++;
			next[i] = (pattern[j] == pattern[i]) ? next[j] : j;
		} else
			j = next[j];
	} while (i < length);
	
	int32 result = -1;
	for (int32 i = fromOffset, j = 0; i < stringLength; ) {
		if (j == -1 || pattern[j] == UTF8SafeTolower(_privateData[i])) {
			i++;
			j++;
			if (j >= length) {
				result = i - j;
				break;
			}
		} else
			j = next[j];
	}

#if DEBUG
	// double-check our results using a slow dumb search
	char *debugPattern = strdup(str);

	for (int32 index = 0; index < length; index++)
		debugPattern[index] = UTF8SafeTolower(str[index]);

	debugPattern[length] = '\0';

	char *tmpCheck = strstr(_privateData + fromOffset, debugPattern);
	ASSERT((!tmpCheck && result == -1) || (tmpCheck - _privateData == result)); 
	free (debugPattern);
#endif

	return result;
#endif
}

int32 
BString::FindFirst(const BString &pattern) const
{
	int32 length = Length();
	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;
	
	if (patternLength > length)
		return -1;
	
	if (length < kShortFindAfterTreshold)
		return _ShortFindAfter(pattern._privateData, 0);

	return _FindAfter(pattern._privateData, patternLength, 0);
}

int32 
BString::FindFirst(const char *str) const
{
	if (!Length())
		return -1;

	return _ShortFindAfter(str, 0);
}

int32 
BString::FindFirst(const char *str, int32 fromOffset) const
{
	if (Length() - fromOffset <= 0)
		return -1;

	return _ShortFindAfter(str, fromOffset);
}


int32 
BString::FindFirst(const BString &pattern, int32 fromOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;
	
	if (patternLength > length - fromOffset)
		return -1;
	
	if (length < kShortFindAfterTreshold)
		return _ShortFindAfter(pattern._privateData, fromOffset);

	return _FindAfter(pattern._privateData, patternLength, fromOffset);
}

int32 
BString::IFindFirst(const BString &pattern) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;
	
	if (patternLength > length)
		return -1;
	
	return _IFindAfter(pattern._privateData, patternLength, 0);
}

int32 
BString::IFindFirst(const char *str) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = str ? strlen(str) : 0;
	if (!patternLength)
		// everything matches null pattern
		return 0;
	
	if (patternLength > length)
		return -1;
	
	return _IFindAfter(str, patternLength, 0);
}

int32 
BString::IFindFirst(const BString &pattern, int32 fromOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;
	
	if (patternLength > length - fromOffset)
		return -1;
	
	return _IFindAfter(pattern._privateData, patternLength, fromOffset);
}

int32 
BString::IFindFirst(const char *str, int32 fromOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = str ? strlen(str) : 0;
	if (!patternLength)
		// everything matches null pattern
		return 0;
	
	if (patternLength > length - fromOffset)
		return -1;
	
	return _IFindAfter(str, patternLength, fromOffset);
}

int32 
BString::_FindBefore(const char *str, int32 length, int32 beforeOffset) const
{
	ASSERT(Length() >= beforeOffset);
	ASSERT(Length() >= length);
	
	const char *s1 = _privateData + beforeOffset - length;
	const char *to = _privateData;
	
	for (;;) {
		const char *s2 = s1--;
		if (s2 < to)
			break;
		
		const char *pat = str;
		char c1, c2;
		while ((c1 = *s2++) == (c2 = *pat++) && c2) 
			;
		
		if (!c2)
			return  s1 + 1 - _privateData;
	}
	return -1;
}

int32 
BString::_IFindBefore(const char *str, int32 length, int32 beforeOffset) const
{
	ASSERT(Length() >= beforeOffset);
	ASSERT(Length() >= length);
	
	_AllocaBuffer<char> pattern(length + 1);
	for (int32 index = 0; index < length; index++)
		pattern[index] = UTF8SafeTolower(str[index]);
	pattern[length] = '\0';
	
	const char *s1 = _privateData + beforeOffset - length;
	const char *to = _privateData;
	
	for (;;) {
		const char *s2 = s1--;
		if (s2 < to)
			break;
		
		const char *pat = pattern;
		char c1, c2;
		while (UTF8SafeTolower(c1 = *s2++) == (c2 = *pat++) && c2)
			;
		
		if (!c2)
			return s1 + 1 - _privateData ;
	}
	return -1;
}

int32 
BString::FindLast(const BString &pattern) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;

	if (patternLength > length)
		return -1;

	return _FindBefore(pattern.String(), patternLength, length);
}

int32 
BString::FindLast(const char *str) const
{
	int32 length = Length();
	if (!length)
		return -1;

	if (!str)
		// everything matches null pattern
		return 0;

	int32 patternLength = strlen(str);
	if (patternLength > length)
		return -1;
	
	return _FindBefore(str, patternLength, length);
}

int32 
BString::FindLast(const BString &pattern, int32 beforeOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;

	if (beforeOffset > length)
		beforeOffset = length;

	if (patternLength > beforeOffset)
		return -1;

	return _FindBefore(pattern.String(), patternLength, beforeOffset);
}

int32 
BString::FindLast(const char *str, int32 beforeOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	if (!str)
		return 0;

	int32 patternLength = strlen(str);
	if (!patternLength)
		// everything matches null pattern
		return 0;

	if (beforeOffset > length)
		beforeOffset = length;

	if (patternLength > beforeOffset)
		return -1;

	return _FindBefore(str, patternLength, beforeOffset);
}

int32 
BString::IFindLast(const BString &pattern) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;

	if (patternLength > length)
		return -1;

	return _IFindBefore(pattern.String(), patternLength, length);
}

int32 
BString::IFindLast(const char *str) const
{
	int32 length = Length();
	if (!length)
		return -1;

	if (!str)
		// everything matches null pattern
		return 0;

	int32 patternLength = strlen(str);
	if (patternLength > length)
		return -1;
	
	return _IFindBefore(str, patternLength, length);
}

int32 
BString::IFindLast(const BString &pattern, int32 beforeOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	int32 patternLength = pattern.Length();
	if (!patternLength)
		// everything matches null pattern
		return 0;

	if (beforeOffset > length)
		beforeOffset = length;

	if (patternLength > beforeOffset)
		return -1;

	return _IFindBefore(pattern.String(), patternLength, beforeOffset);
}

int32 
BString::IFindLast(const char *str, int32 beforeOffset) const
{
	int32 length = Length();
	if (!length)
		return -1;

	if (!str)
		return 0;

	int32 patternLength = strlen(str);
	if (!patternLength)
		// everything matches null pattern
		return 0;

	if (beforeOffset > length)
		beforeOffset = length;

	if (patternLength > beforeOffset)
		return -1;

	return _IFindBefore(str, patternLength, beforeOffset);
}

int32 
BString::FindFirst(char ch) const
{
	int32 length = Length();
	if (!length)
		return -1;

	for (int32 index = 0; index < length; index++)
		if (_privateData[index] == ch)
			return index;

	return -1;
}

int32 
BString::FindFirst(char ch, int32 fromOffset) const
{
	int32 length = Length();
	if (length <= fromOffset)
		return -1;

	if (fromOffset < 0)
		fromOffset = 0;
	for (int32 index = fromOffset; index < length; index++)
		if (_privateData[index] == ch)
			return index;

	return -1;
}

int32 
BString::FindLast(char ch) const
{
	int32 length = Length();
	if (!length)
		return -1;

	for (int32 index = length - 1; index >= 0; index--)
		if (_privateData[index] == ch)
			return index;

	return -1;
}

int32 
BString::FindLast(char ch, int32 beforeOffset) const
{
	int32 length = Length();
	if (beforeOffset < 0 || !length)
		return -1;

	if (beforeOffset >= length)
		beforeOffset = length - 1;

	for (int32 index = beforeOffset; index >= 0; index--)
		if (_privateData[index] == ch)
			return index;

	return -1;
}

BString &
BString::ReplaceFirst(char ch1, char ch2)
{
	int32 length = Length();
	for (int32 index = 0; index < length; index++)
		if (_privateData[index] == ch1) {
			_privateData[index] = ch2;
			break;
		}

	return *this;
}

BString &
BString::ReplaceLast(char ch1, char ch2)
{
	for (int32 index = Length() - 1; index >= 0; index--)
		if (_privateData[index] == ch1) {
			_privateData[index] = ch2;
			break;
		}

	return *this;
}

BString &
BString::ReplaceAll(char ch1, char ch2, int32 fromOffset)
{
	int32 length = Length();
	for (int32 index = fromOffset; index < length; index++)
		if (_privateData[index] == ch1) {
			_privateData[index] = ch2;
		}

	return *this;
}

BString &
BString::Replace(char ch1, char ch2, int32 maxReplaceCount, int32 fromOffset)
{
	int32 length = Length();
	for (int32 count = 0, index = fromOffset;
		index < length && count < maxReplaceCount;
		index++)
		if (_privateData[index] == ch1) {
			_privateData[index] = ch2;
			++count;
		}
	return *this;
}

BString &
BString::ReplaceFirst(const char *replaceThis, const char *withThis)
{
	if (replaceThis) {
		int32 offset = FindFirst(replaceThis);
		if (offset >= 0) {
			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			if (sourceLength < replaceLength) 
				_OpenAtBy(offset, replaceLength - sourceLength);
			else if (sourceLength > replaceLength)
				_ShrinkAtBy(offset, sourceLength - replaceLength);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}
		}
	}
	return *this;
}

BString &
BString::ReplaceLast(const char *replaceThis, const char *withThis)
{
	if (replaceThis) {
		int32 offset = FindLast(replaceThis);
		if (offset >= 0) {
			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			if (sourceLength < replaceLength) 
				_OpenAtBy(offset, replaceLength - sourceLength);
			else if (sourceLength > replaceLength)
				_ShrinkAtBy(offset, sourceLength - replaceLength);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}
		}
	}
	return *this;
}

BString &
BString::ReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	if (replaceThis) {
		int32 length = Length();
		for (int32 offset = fromOffset; offset < length; offset++) {
			offset = FindFirst(replaceThis, offset);
			if (offset < 0)
				break;

			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			int32 delta = replaceLength - sourceLength;
			if (delta > 0)
				_OpenAtBy(offset, delta);
			else if (delta < 0)
				_ShrinkAtBy(offset, -delta);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}

			offset += replaceLength - 1;
			length += delta;
		}
	}
	return *this;
}

BString &
BString::Replace(const char *replaceThis, const char *withThis,
	int32 maxReplaceCount, int32 fromOffset)
{
	if (replaceThis) {
		int32 length = Length();
		for (int32 count = 0, offset = fromOffset;
			offset < length && count < maxReplaceCount;
			offset++, count++) {

			offset = FindFirst(replaceThis, offset);
			if (offset < 0)
				break;

			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			int32 delta = replaceLength - sourceLength;
			if (delta > 0)
				_OpenAtBy(offset, delta);
			else if (delta < 0)
				_ShrinkAtBy(offset, -delta);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}

			offset += replaceLength - 1;
			length += delta;			
		}
	}
	return *this;
}

BString &
BString::IReplaceFirst(char ch1, char ch2)
{
	int32 length = Length();
	ch1 = UTF8SafeTolower(ch1);
	for (int32 index = 0; index < length; index++)
		if (UTF8SafeTolower(_privateData[index]) == ch1) {
			_privateData[index] = ch2;
			break;
		}

	return *this;
}

BString &
BString::IReplaceLast(char ch1, char ch2)
{
	ch1 = UTF8SafeTolower(ch1);
	for (int32 index = Length() - 1; index >= 0; index--)
		if (UTF8SafeTolower(_privateData[index]) == ch1) {
			_privateData[index] = ch2;
			break;
		}

	return *this;
}

BString &
BString::IReplaceAll(char ch1, char ch2 , int32 fromOffset)
{
	ch1 = UTF8SafeTolower(ch1);
	int32 length = Length();
	for (int32 index = fromOffset; index < length; index++)
		if (UTF8SafeTolower(_privateData[index]) == ch1) {
			_privateData[index] = ch2;
		}

	return *this;
}

BString &
BString::IReplace(char ch1, char ch2 , int32 maxReplaceCount,
	int32 fromOffset)
{
	ch1 = UTF8SafeTolower(ch1);
	int32 length = Length();
	for (int32 count = 0, index = fromOffset;
		index < length && count < maxReplaceCount;
		index++)
		if (UTF8SafeTolower(_privateData[index]) == ch1) {
			_privateData[index] = ch2;
			++count;
		}
	return *this;
}

BString &
BString::IReplaceFirst(const char *replaceThis, const char *withThis)
{
	if (replaceThis) {
		int32 offset = IFindFirst(replaceThis);
		if (offset >= 0) {
			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			if (sourceLength < replaceLength) 
				_OpenAtBy(offset, replaceLength - sourceLength);
			else if (sourceLength > replaceLength)
				_ShrinkAtBy(offset, sourceLength - replaceLength);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}
		}
	}
	return *this;
}

BString &
BString::IReplaceLast(const char *replaceThis, const char *withThis)
{
	if (replaceThis) {
		int32 offset = IFindLast(replaceThis);
		if (offset >= 0) {
			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			if (sourceLength < replaceLength) 
				_OpenAtBy(offset, replaceLength - sourceLength);
			else if (sourceLength > replaceLength)
				_ShrinkAtBy(offset, sourceLength - replaceLength);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}
		}
	}
	return *this;
}

BString &
BString::IReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	if (replaceThis) {
		int32 length = Length();
		for (int32 offset = fromOffset; offset < length; offset++) {
			offset = IFindFirst(replaceThis, offset);
			if (offset < 0)
				break;

			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			int32 delta = replaceLength - sourceLength;
			if (delta > 0)
				_OpenAtBy(offset, delta);
			else if (delta < 0)
				_ShrinkAtBy(offset, -delta);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}

			offset += replaceLength - 1;
			length += delta;
		}
	}
	return *this;
}

BString &
BString::IReplace(const char *replaceThis, const char *withThis,
	int32 maxReplaceCount, int32 fromOffset)
{
	if (replaceThis) {
		int32 length = Length();
		for (int32 count = 0, offset = fromOffset;
			offset < length && count < maxReplaceCount;
			offset++, count++) {

			offset = IFindFirst(replaceThis, offset);
			if (offset < 0)
				break;

			int32 sourceLength = strlen(replaceThis);
			int32 replaceLength = 0;
			if (withThis)
				replaceLength = strlen(withThis);
			
			int32 delta = replaceLength - sourceLength;
			if (delta > 0)
				_OpenAtBy(offset, delta);
			else if (delta < 0)
				_ShrinkAtBy(offset, -delta);

			if (withThis) {
				char *dst = _privateData + offset;
				memcpy(dst, withThis, replaceLength);
			}
			
			offset += replaceLength - 1;
			length += delta;
		}
	}
	return *this;
}

BString &
BString::ReplaceSet(const char *setOfChars, char with)
{
	char map[128];
	for (int32 index = 0; index < 128; index++)
		map[index] = index;

	bool dealWithUTF8 = false;
	for (const char *tmp = setOfChars; *tmp; ) {
		if (*tmp & 0x80) {
			dealWithUTF8 = true;
			tmp += UTF8CharLen(*tmp);
		} else
			map[*tmp++] = with;
	}

	int32 length = Length();
	if (dealWithUTF8) {
		debugger("UTF8 is not yet supported for this operation");
	} else 
		for (int32 index = 0; index < length; index ++)
			_privateData[index] = map[_privateData[index]];

	return *this;
}

BString &
BString::ReplaceSet(const char *setOfCharsToRemove, const char *with)
{
	// deal with trivial cases
	if (!with)
		return *this;
	
	int32 patternLength = strlen(with);
	if (patternLength == 1)
		return ReplaceSet(setOfCharsToRemove, *with);
	else if (patternLength == 0)
		return RemoveSet(setOfCharsToRemove);

	bool map[128] = {
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false,
		false, false, false, false, false, false, false, false
	};

	bool dealWithUTF8 = false;
	for (const char *tmp = setOfCharsToRemove; *tmp; ) {
		if (*tmp & 0x80) {
			dealWithUTF8 = true;
			tmp += UTF8CharLen(*tmp);
		} else
			map[*tmp++] = true;
	}

	int32 length = Length();
	if (dealWithUTF8) {
		debugger("UTF8 is not yet supported for this operation");
	} else {
		int32 count = 0;
		for (int32 index = 0; index < length; index ++)
			if (map[_privateData[index]])
				count++;
		if (count) {
			int32 moveBy = count * (patternLength - 1);
			_GrowBy(moveBy);
			int32 chunkEnd = length;
			for (int32 index = length - 1; index >= 0; index--) {
				if (map[_privateData[index]]) {
					char *dstPtr = _privateData + index + 1 + moveBy;
					memcpy(dstPtr, _privateData + index + 1, chunkEnd - (index + 1));
					memcpy(dstPtr - patternLength, with, patternLength);

					// printf("done sofar: %s \n", dstPtr - patternLength);
					chunkEnd = index;
					moveBy -= (patternLength - 1);
					length += (patternLength - 1);
					if (moveBy <= 0)
						break;
				}
			}
			if (length > 0) {
				_SetLength(length);
				_privateData[length] = '\0';
			} else {
				_DoAssign(NULL, 0);
			}
		}
	}		

	return *this;
}

char *
BString::LockBuffer(int32 maxLength)
{
	int32 oldLength = Length();
	if (maxLength > oldLength) {
		_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
			: 0, maxLength + 1 + sizeof(int32));
		if (!_privateData)
			return 0;

		_privateData += sizeof(int32);
	}
	if (!oldLength && maxLength)
		_privateData[0] = 0;

	_SetUsingAsCString(true);
	return _privateData;
}

BString &
BString::UnlockBuffer(int32 length)
{
	ASSERT(!_privateData || (*((uint32 *)_privateData - 1) & 0x80000000) != 0);
	_SetUsingAsCString(false);

	if (length >= 0) {
		if (length) {
			_SetLength(length);
			_privateData[length] = '\0';
			return *this;
		}
	} else 
		length = _privateData ? strlen(_privateData) : 0;
	
	if (length)
		_SetLength(length);
	else if (_privateData) {
		free(_privateData - sizeof(int32));
		_privateData = 0;
	}

	return *this;
}

BString &
BString::ToLower()
{
	int32 length = Length();
	for (int32 index = 0; index < length; index++)
		_privateData[index] = UTF8SafeTolower(_privateData[index]);

	return *this;
}

BString &
BString::ToUpper()
{
	int32 length = Length();
	for (int32 index = 0; index < length; index++)
		_privateData[index] = UTF8SafeToUpper(_privateData[index]);

	return *this;
}

BString &
BString::Capitalize()
{
	if (!Length())
		return *this;

	_privateData[0] = UTF8SafeToUpper(_privateData[0]);
	
	int32 length = Length();
	for (int32 index = 1; index < length; index++) 
		_privateData[index] = UTF8SafeTolower(_privateData[index]);

	return *this;
}

BString &
BString::CapitalizeEachWord()
{
	bool sawAlpha = false;
	int32 length = Length();
	for (int32 index = 0; index < length; index++) 
		if (UTF8SafeIsAlpha(_privateData[index])) {
			_privateData[index] = sawAlpha ?
				UTF8SafeTolower(_privateData[index]) : UTF8SafeToUpper(_privateData[index]);
			sawAlpha = true;
		} else
			sawAlpha = false;

	return *this;
}

inline bool
_CharInSet(char ch, const char *set, int32 setSize)
{
	for (int32 index = 0; index < setSize; index++)
		if (ch == set[index])
			return true;
	return false;
}


BString &
BString::CharacterEscape(const char *original, const char *setOfCharsToEscape, char escapeWith)
{
	*this = "";

	int32 setSize = 0;
	if (setOfCharsToEscape)
		setSize = strlen(setOfCharsToEscape);

	if (!setSize) {
		// trivial case
		*this = original;
		return *this;
	}
	
	const char *from = original;
	for (;;) {
		int32 count = 0;
		while(from[count] && !_CharInSet(from[count], setOfCharsToEscape, setSize))
			count++;
		Append(from, count);
		if (!from[count])
			break;
		char tmp[3];
		tmp[0] = escapeWith;
		tmp[1] = from[count];
		tmp[2] = '\0';
		(*this) += tmp;
		from += count + 1;
	}
	return *this;
}

BString &
BString::CharacterEscape(const char *setOfCharsToEscape, char escapeWith)
{
	int32 setSize = 0;
	if (setOfCharsToEscape)
		setSize = strlen(setOfCharsToEscape);

	if (!setSize) 
		// trivial case
		return *this;

	bool hitEscapable = false;
	
	// find out if we even need to modify the string in the first place
	const char *string = String();
	int32 count = Length();
	for (int32 index = 0; index < count; index++)
		if (_CharInSet(string[index], setOfCharsToEscape, setSize)) {
			hitEscapable = true;
			break;
		}

	if (!hitEscapable)
		// nothing to escape, bail
		return *this;
		
	// ToDo:
	// optimize this to do an in-place escape
	BString tmp;
	tmp.Adopt(*this);
	return CharacterEscape(tmp.String(), setOfCharsToEscape, escapeWith);
}

BString &
BString::CharacterDeescape(const char *original, char escapeChar)
{
	(*this) = "";

	// ToDo:
	// optimize this to not grow string for every char

	for (;*original; original++)
		if (*original != escapeChar)
			(*this) += *original;
		else {
			if (!*++original)
				break;
			(*this) += *original;			
		}
	
	return *this;
}

BString &
BString::CharacterDeescape(char escapeChar)
{
	bool hitEscapeChar = false;
	const char *string = String();
	int32 count = Length();
	for (int32 index = 0; index < count; index++)
		if (string[index] == escapeChar) {
			hitEscapeChar = true;
			break;
		}

	if (!hitEscapeChar)
		return *this;

	BString tmp;
	tmp.Adopt(*this);
	return CharacterDeescape(tmp.String(), escapeChar);
}


BString &
BString::operator<<(const char *str)
{
	*this += str;

	return *this;
}

/*---------- Deprecated --------- */

#if __GNUC__ || __MWERKS__
extern "C" {

#if _R4_5_COMPATIBLE_
	_EXPORT BString&
	#if __GNUC__
	__ls__7BStringR7BString
	#elif __MWERKS__
	__ls__7BStringFR7BString
	#endif
	(BString* This, BString& string)
	{
		return This->operator<<(string);
	}
#endif

#if _R5_COMPATIBLE_
	_EXPORT BString&
	#if __GNUC__
	__as__7BStringc
	#elif __MWERKS__
	__as__7BStringFc
	#endif
	(BString* This, char c)
	{
		const char tmp[2] = {c, '\0'};
		return (*This) = tmp;
	}
#endif

}
#endif

BString &
BString::operator<<(const BString &string)
{
	*this += string;
	return *this;
}

BString &
BString::operator<<(char ch)
{
	*this += ch;

	return *this;
}

BString &
BString::operator<<(int num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%d", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

BString &
BString::operator<<(unsigned int num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%u", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

BString &
BString::operator<<(int32 num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%ld", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

BString &
BString::operator<<(uint32 num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%lu", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

BString &
BString::operator<<(int64 num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%Ld", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

BString &
BString::operator<<(uint64 num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%Lu", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

BString &
BString::operator<<(float num)
{
	int32 length = Length();
	char *buffer = (LockBuffer(Length() + 64) + length);
	sprintf(buffer, "%.2f", num);
	UnlockBuffer(length + strlen(buffer));

	return *this;
}

int 
Compare(const BString &a, const BString &b)
{
	return a.Compare(b);
}

int 
ICompare(const BString &a, const BString &b)
{
	return a.ICompare(b);
}

int 
Compare(const BString *a, const BString *b)
{
	return a->Compare(*b);
}

int 
ICompare(const BString *a, const BString *b)
{
	return a->ICompare(*b);
}

void
BMoveBefore(BString* to, BString* from, const size_t step, size_t count)
{
	memcpy(to, from, step*count);
}

void
BMoveAfter(BString* to, BString* from, const size_t step, size_t count)
{
	memmove(to, from, step*count);
}

BDataIO& operator<<(BDataIO& io, const BString& string)
{
	const int32 N = string.Length();
	if (N > 0) io.Write(string.String(), N);
	return io;
}
