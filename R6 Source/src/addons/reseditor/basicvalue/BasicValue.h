/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _BASIC_VALUE_H
#define _BASIC_VALUE_H

#include <GraphicsDefs.h>
#include <Point.h>
#include <Rect.h>

#include <SupportDefs.h>
#include <TypeConstants.h>

class BString;

class BasicValue {
public:
	BasicValue();
	BasicValue(type_code type, const void* value, ssize_t size);
	~BasicValue();
	
	BasicValue(const BasicValue& o);
	BasicValue& operator=(const BasicValue& o);
	
	bool operator==(const BasicValue& o);
	bool operator!=(const BasicValue& o);
	
	status_t SetValue(type_code type, const void* value, ssize_t size);
	
	type_code Type() const;
	const void* Value() const;
	ssize_t Size() const;

	status_t SetValueFromString(const char* str);
	status_t SetStringFromValue(BString* str, bool asHex=false) const;
	
private:
	void FreeMemory();
	void FillDataBuffer() const;
	
	type_code		mType;
	union {
		int64		mInt64;
		int32		mInt32;
		int16		mInt16;
		int8		mInt8;
		uint64		mUInt64;
		uint32		mUInt32;
		uint16		mUInt16;
		uint8		mUInt8;
		size_t		mSizeT;
		ssize_t		mSSizeT;
		off_t		mOffT;
		time_t		mTimeT;
		char		mBool;
		float		mFloat;
		double		mDouble;
		const void*	mPointer;
		BRect*		mRect;
		BPoint*		mPoint;
		rgb_color	mRGBColor;
	};
	
	mutable bool	mDataAlloc;
	mutable void*	mDataBuf;
	mutable ssize_t	mDataSize;
};

#endif
