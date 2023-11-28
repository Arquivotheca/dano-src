/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "BasicValue.h"

#include <Rect.h>
#include <Point.h>

#include <Debug.h>
#include <String.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef VALIDATE
#if DEBUG
#define VALIDATE(x, ret) { ASSERT(x); if( !(x) ) { ret; } }
#else
#define VALIDATE(x, ret) if( !(x) ) { ret; }
#endif
#endif

BasicValue::BasicValue()
	: mType(0), mDataAlloc(false), mDataBuf(0), mDataSize(0)
{
}

BasicValue::BasicValue(type_code type, const void* value, ssize_t size)
	: mType(0), mDataAlloc(false), mDataBuf(0), mDataSize(0)
{
	SetValue(type, value, size);
}

BasicValue::~BasicValue()
{
	FreeMemory();
}

BasicValue::BasicValue(const BasicValue& o)
	: mType(0), mDataBuf(0), mDataSize(0)
{
	*this = o;
}

BasicValue& BasicValue::operator=(const BasicValue& o)
{
	SetValue(o.Type(), o.Value(), o.Size());
	return *this;
}

bool BasicValue::operator==(const BasicValue& o)
{
	if( Type() != o.Type() ) return false;
	if( Size() != o.Size() ) return false;
	if( memcmp(Value(), o.Value(), Size()) != 0 ) return false;
	return true;
}

bool BasicValue::operator!=(const BasicValue& o)
{
	return ! ( *this == o ) ;
}

status_t BasicValue::SetValue(type_code type, const void* value, ssize_t size)
{
	FreeMemory();
	
	switch( type ) {
		case B_POINTER_TYPE: {
			void* defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mPointer), return B_BAD_VALUE);
			mPointer = *(const void**)value;
			mDataBuf = const_cast<void**>(&mPointer);
			mDataSize = sizeof(mPointer);
		} break;

		case B_INT64_TYPE: {
			int64 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mInt64), return B_BAD_VALUE);
			mInt64 = *(const int64*)value;
			mDataBuf = &mInt64;
			mDataSize = sizeof(mInt64);
		} break;
		
		case B_INT32_TYPE: {
			int32 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mInt32), return B_BAD_VALUE);
			mInt32 = *(const int32*)value;
			mDataBuf = &mInt32;
			mDataSize = sizeof(mInt32);
		} break;
		
		case B_INT16_TYPE: {
			int16 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mInt16), return B_BAD_VALUE);
			mInt16 = *(const int16*)value;
			mDataBuf = &mInt16;
			mDataSize = sizeof(mInt16);
		} break;
		
		case B_CHAR_TYPE:
		case B_INT8_TYPE: {
			int8 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mInt8), return B_BAD_VALUE);
			mInt8 = *(const int8*)value;
			mDataBuf = &mInt8;
			mDataSize = sizeof(mInt8);
		} break;
		
		case B_UINT64_TYPE: {
			uint64 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mUInt64), return B_BAD_VALUE);
			mUInt64 = *(const uint64*)value;
			mDataBuf = &mUInt64;
			mDataSize = sizeof(mUInt64);
		} break;
		
		case B_UINT32_TYPE: {
			uint32 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mUInt32), return B_BAD_VALUE);
			mUInt32 = *(const uint32*)value;
			mDataBuf = &mUInt32;
			mDataSize = sizeof(mUInt32);
		} break;
		
		case B_UINT16_TYPE: {
			uint16 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mUInt16), return B_BAD_VALUE);
			mUInt16 = *(const uint16*)value;
			mDataBuf = &mUInt16;
			mDataSize = sizeof(mUInt16);
		} break;
		
		case B_UINT8_TYPE: {
			uint8 defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mUInt8), return B_BAD_VALUE);
			mUInt8 = *(const uint8*)value;
			mDataBuf = &mUInt8;
			mDataSize = sizeof(mUInt8);
		} break;
		
		case B_SIZE_T_TYPE: {
			size_t defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mSizeT), return B_BAD_VALUE);
			mSizeT = *(const size_t*)value;
			mDataBuf = &mSizeT;
			mDataSize = sizeof(mSizeT);
		} break;
		
		case B_SSIZE_T_TYPE: {
			ssize_t defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mSSizeT), return B_BAD_VALUE);
			mSSizeT = *(const ssize_t*)value;
			mDataBuf = &mSSizeT;
			mDataSize = sizeof(mSSizeT);
		} break;
		
		case B_OFF_T_TYPE: {
			off_t defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mOffT), return B_BAD_VALUE);
			mOffT = *(const off_t*)value;
			mDataBuf = &mOffT;
			mDataSize = sizeof(mOffT);
		} break;
		
		case B_TIME_TYPE: {
			time_t defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mTimeT), return B_BAD_VALUE);
			mTimeT = *(const time_t*)value;
			mDataBuf = &mTimeT;
			mDataSize = sizeof(mTimeT);
		} break;
		
		case B_BOOL_TYPE: {
			bool defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(char), return B_BAD_VALUE);
			mBool = (*(const char*)value) ? true : false;
			mDataBuf = &mBool;
			mDataSize = sizeof(mBool);
		} break;
					
		case B_FLOAT_TYPE: {
			float defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mFloat), return B_BAD_VALUE);
			mFloat = *(const float*)value;
			mDataBuf = &mFloat;
			mDataSize = sizeof(mFloat);
		} break;
		
		case B_DOUBLE_TYPE: {
			double defVal = 0;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mDouble), return B_BAD_VALUE);
			mDouble = *(const double*)value;
			mDataBuf = &mDouble;
			mDataSize = sizeof(mDouble);
		} break;
		
		case B_RECT_TYPE: {
			BRect defVal;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(BRect), return B_BAD_VALUE);
			mRect = new BRect(*(const BRect*)value);
			mDataBuf = mRect;
			mDataSize = sizeof(*mRect);
		} break;
		
		case B_POINT_TYPE: {
			BPoint defVal;
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(BPoint), return B_BAD_VALUE);
			mPoint = new BPoint(*(const BPoint*)value);
			mDataBuf = mPoint;
			mDataSize = sizeof(*mPoint);
		} break;

		case B_RGB_COLOR_TYPE: {
			rgb_color defVal = { 0, 0, 0, 255 };
			if( size == 0 ) {
				value = &defVal;
				size = sizeof(defVal);
			}
			VALIDATE(size == sizeof(mRGBColor), return B_BAD_VALUE);
			mRGBColor = *(const rgb_color*)value;
			mDataBuf = &mRGBColor;
			mDataSize = sizeof(mRGBColor);
		} break;
		
		default:
			return B_BAD_TYPE;
	}
	
	mType = type;
	
	return B_OK;
}

static status_t str_to_int(int64& value, const char*& str,
							bool isSigned=true, bool skipToNum=false, int32 base=10)
{
	value=0;
	int32 sign=1;
	
	if( skipToNum ) {
		while( *str &&
				( (*str < '0' || *str > '9') &&
				  (!isSigned || *str != '-') ) ) {
			str++;
		}
	} else {
		while( *str && isspace(*str) ) str++;
	}
	
	if( isSigned && *str == '-' ) {
		sign=-1;
		str++;
		while( *str && isspace(*str) ) str++;
	}
	
	bool hasNum = false;
	
	if( *str == '0' ) {
		str++;
		if( *str == 'x' ) {
			base = 16;
			str++;
		} else if( *str == 'b' ) {
			base = 2;
			str++;
		} else if( *str >= '0' && *str <= '7' ) {
			base = 8;
		} else {
			hasNum = true;
		}
	}
	
	while( *str && !isspace(*str) ) {
		int32 digit = -1;
		if( *str >= '0' && *str <= '9' ) digit = *str - '0';
		else if( *str >= 'a' && *str <= 'z' ) digit = *str - 'a' + 10;
		else if( *str >= 'A' && *str <= 'Z' ) digit = *str - 'A' + 10;
		if( digit < 0 || digit >= base ) {
			if( skipToNum ) break;
			value *= sign;
			return B_BAD_VALUE;
		}
		hasNum = true;
		value *= base;
		value += digit;
		str++;
	}
	
	value *= sign;
	
	return hasNum ? B_OK : B_BAD_VALUE;
}

static status_t str_to_dbl(double& value, const char*& str)
{
	value = 0;
	
	while( *str &&
			( (*str < '0' || *str > '9') &&
			  *str != '-' && *str != '+' && *str != '.' &&
			  *str != 'e' && *str != 'E' ) ) {
		str++;
	}
	
	if( *str == 0 ) return B_BAD_VALUE;
	
	value = atof(str);
	
	// Now check syntax of number.
	
	if( *str == '-' || *str == '+' ) str++;
	
	if( ( *str < '0' || *str > '9' ) &&
		*str != '.' && *str != 'e' && *str != 'E' ) return B_BAD_VALUE;
	
	bool hasNum = false;
	
	while( *str >= '0' && *str <= '9' ) {
		hasNum = true;
		str++;
	}
	if( *str == '.' ) {
		str++;
		while( *str >= '0' && *str <= '9' ) {
			hasNum = true;
			str++;
		}
	}
	
	if( *str == 'e' || *str == 'E' ) {
		str++;
		if( *str == '-' || *str == '+' ) str++;
		while( *str >= '0' && *str <= '9' ) {
			hasNum = true;
			str++;
		}
	}
	
	if( !hasNum || ( *str >= '0' && *str <= '9' ) ||
		*str == '-' || *str == '+' || *str == '.' ||
		*str == 'e' || *str == 'E' ) {
		return B_BAD_VALUE;
	}
	
	return B_OK;
}

status_t BasicValue::SetValueFromString(const char* str)
{
	int64 i64=0;
	uint64 ui64=0;
	double dbl=0;
	
	status_t err = B_OK;
	
	// First do any common numeric conversion that is possible.
	
	switch( mType ) {
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
			err = str_to_int(i64, str);
			break;
		
		case B_SIZE_T_TYPE:
		case B_OFF_T_TYPE:
		case B_POINTER_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_CHAR_TYPE:
		case B_UINT8_TYPE:
			err = str_to_int(i64, str, false);
			ui64 = i64;
			break;
			
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			err = str_to_dbl(dbl, str);
			break;
	}
	
	if( err != B_OK ) return err;
	
	// Now store the actual value.
	
	switch( mType ) {
		case B_POINTER_TYPE:
			mPointer = (void *)ui64;
			break;

		case B_SSIZE_T_TYPE:
			mSSizeT = (ssize_t)i64;
			break;
			
		case B_SIZE_T_TYPE:
			mSizeT = (size_t)i64;
			break;
			
		case B_OFF_T_TYPE:
			mOffT = (off_t)i64;
			break;
			
		case B_TIME_TYPE:
			mTimeT = (time_t)i64;
			break;
			
		case B_INT64_TYPE:
			mInt64 = i64;
			break;
		
		case B_INT32_TYPE:
			mInt32 = (int32)i64;
			break;
		
		case B_INT16_TYPE:
			mInt16 = (int16)i64;
			break;
		
		case B_CHAR_TYPE:
		case B_INT8_TYPE:
			mInt8 = (int8)i64;
			break;
		
		case B_UINT64_TYPE:
			mUInt64 = ui64;
			break;
		
		case B_UINT32_TYPE:
			mUInt32 = (uint32)ui64;
			break;
		
		case B_UINT16_TYPE:
			mUInt16 = (uint16)ui64;
			break;
		
		case B_UINT8_TYPE:
			mUInt8 = (uint8)ui64;
			break;
		
		case B_BOOL_TYPE:
			if( *str == 't' || *str == 'T' || *str == '1' ) mBool = true;
			else if( *str == 'f' || *str == 'F' || *str == '0' ) mBool = false;
			else return B_BAD_VALUE;
			break;
		
		case B_FLOAT_TYPE:
			mFloat = float(dbl);
			break;
		
		case B_DOUBLE_TYPE:
			mDouble = dbl;
			break;
			
		case B_RECT_TYPE:
			if( mRect ) {
				BRect rect;
				err = str_to_dbl(dbl, str);
				if( err == B_OK ) rect.left = float(dbl);
				if( err == B_OK ) err = str_to_dbl(dbl, str);
				if( err == B_OK ) rect.top = float(dbl);
				if( err == B_OK ) err = str_to_dbl(dbl, str);
				if( err == B_OK ) rect.right = float(dbl);
				if( err == B_OK ) err = str_to_dbl(dbl, str);
				if( err == B_OK ) rect.bottom = float(dbl);
				if( err == B_OK ) *mRect = rect;
			}
			break;
			
		case B_POINT_TYPE:
			if( mPoint ) {
				BPoint point;
				err = str_to_dbl(dbl, str);
				if( err == B_OK ) point.x = float(dbl);
				if( err == B_OK ) err = str_to_dbl(dbl, str);
				if( err == B_OK ) point.y = float(dbl);
				if( err == B_OK ) *mPoint = point;
			}
			break;

		case B_RGB_COLOR_TYPE: {
			rgb_color color;
			err = str_to_int(i64, str, false, true);
			if( err == B_OK ) color.red = (uint8)i64;
			if( err == B_OK ) err = str_to_int(i64, str, false, true);
			if( err == B_OK ) color.green = (uint8)i64;
			if( err == B_OK ) err = str_to_int(i64, str, false, true);
			if( err == B_OK ) color.blue = (uint8)i64;
			if( err == B_OK ) err = str_to_int(i64, str, false, true);
			if( err == B_OK ) color.alpha = (uint8)i64;
			if( err == B_OK ) mRGBColor = color;
		} break;
		
		default:
			return B_BAD_TYPE;
	}
	
	#if 0
	if( mDataBuf ) {
		ASSERT(is_type_swapped(mType));
		swap_data(mType, mDataBuf, mDataSize, B_SWAP_HOST_TO_LENDIAN);
	}
	#endif
	
	return err;
}

status_t BasicValue::SetStringFromValue(BString* to, bool asHex) const
{
	char buffer[256];
	
	switch( mType ) {
		case B_POINTER_TYPE:
			sprintf(buffer, "%p", mPointer);
			break;

		case B_SSIZE_T_TYPE:
			sprintf(buffer, asHex ? "0x%LX" : "%ld", mSSizeT);
			break;
		
		case B_SIZE_T_TYPE:
			sprintf(buffer, asHex ? "0x%08lX" : "%lu", mSizeT);
			break;
		
		case B_OFF_T_TYPE:
			sprintf(buffer, asHex ? "0x%08lX" : "%Lu", mOffT);
			break;
		
		case B_TIME_TYPE:
			sprintf(buffer, asHex ? "0x%08lX" : "%ld", mTimeT);
			break;
		
		case B_INT64_TYPE:
			sprintf(buffer, asHex ? "0x%LX" : "%Ld", mInt64);
			break;
		
		case B_INT32_TYPE:
			sprintf(buffer, asHex ? "0x%08lX" : "%ld", mInt32);
			break;
		
		case B_INT16_TYPE:
			sprintf(buffer, asHex ? "0x%04X" : "%d", mInt16);
			break;
		
		case B_CHAR_TYPE:
		case B_INT8_TYPE:
			sprintf(buffer, asHex ? "0x%02X" : "%d", (int16)mInt8);
			break;
		
		case B_UINT64_TYPE:
			sprintf(buffer, asHex ? "0x%LX" : "%Lu", mUInt64);
			break;
		
		case B_UINT32_TYPE:
			sprintf(buffer, asHex ? "0x%08lX" : "%lu", mUInt32);
			break;
		
		case B_UINT16_TYPE:
			sprintf(buffer, asHex ? "0x%04X" : "%u", mUInt16);
			break;
		
		case B_UINT8_TYPE:
			sprintf(buffer, asHex ? "0x%02X" : "%u", (uint16)mUInt8);
			break;
		
		case B_BOOL_TYPE:
			strcpy(buffer, mBool ? "True" : "False");
			break;
		
		case B_FLOAT_TYPE:
			sprintf(buffer, "%g", mFloat);
			if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
				!strchr(buffer, 'E') ) {
				strncat(buffer, ".0", sizeof(buffer)-1);
			}
			break;
		
		case B_DOUBLE_TYPE:
			sprintf(buffer, "%g", mDouble);
			if( !strchr(buffer, '.') && !strchr(buffer, 'e') &&
				!strchr(buffer, 'E') ) {
				strncat(buffer, ".0", sizeof(buffer)-1);
			}
			break;
			
		case B_RECT_TYPE:
			if( mRect ) {
				sprintf(buffer, "(%g,%g)-(%g,%g)",
						mRect->left, mRect->top, mRect->right, mRect->bottom);
			} else {
				to->SetTo("(?,?)-(?,?)");
				return B_OK;
			}
			break;
			
		case B_POINT_TYPE:
			if( mPoint ) {
				sprintf(buffer, "(%g,%g)", mPoint->x, mPoint->y);
			} else {
				to->SetTo("(?,?)");
				return B_OK;
			}
			break;

		case B_RGB_COLOR_TYPE:
			sprintf(buffer, asHex ? "R=0x%02X,G=0x%02X,B=0x%02X,A=0x%02X"
								  : "R=%u,G=%u,B=%u,A=%u",
					(uint16)mRGBColor.red, (uint16)mRGBColor.green,
					(uint16)mRGBColor.blue, (uint16)mRGBColor.alpha);
			break;
		
		default:
			return B_BAD_TYPE;
	}
	
	to->SetTo(buffer);
	
	return B_OK;
}

type_code BasicValue::Type() const
{
	return mType;
}

const void* BasicValue::Value() const
{
	if( !mDataBuf ) FillDataBuffer();
	return mDataBuf;
}

ssize_t BasicValue::Size() const
{
	if( !mDataBuf ) FillDataBuffer();
	return mDataSize;
}

void BasicValue::FreeMemory()
{
	switch( mType ) {
		case B_RECT_TYPE:
			delete mRect;
			break;
		case B_POINT_TYPE:
			delete mPoint;
			break;
	}
	
	mType = 0;
	mPointer = 0;
	
	if( mDataAlloc ) free(mDataBuf);
	mDataAlloc = false;
	mDataBuf = 0;
	mDataSize = 0;
}

void BasicValue::FillDataBuffer() const
{
	switch( mType ) {
	}
}
