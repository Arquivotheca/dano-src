#ifndef ARRAY_SCANNER_H
#define ARRAY_SCANNER_H

#include <String.h>

namespace BPrivate {

	enum value_type {
		VALUE_NUMBER = 0,
		VALUE_STRING = 1
	};
	
	class ArrayParserValue
	{
	public:
		ArrayParserValue()
			: fType(VALUE_NUMBER), fNumber(0)
		{
		}
		
		ArrayParserValue(const ArrayParserValue& o)
			: fType(o.fType), fString(o.fString), fNumber(o.fNumber)
		{
		}
		
		virtual ~ArrayParserValue()
		{
		}
	
		void SetType(value_type type)			{ fType = type; }
		value_type Type() const					{ return fType; }
		
		void SetNumber(int64 value)				{ fType = VALUE_NUMBER; fNumber = value; }
		int64 Number() const					{ return fNumber; }
		
		void SetString(const char* value)		{ fType = VALUE_STRING; fString = value; }
		const char* String() const				{ return fString.String(); }
		
		const BString& StringObject() const		{ return fString; }
		BString& StringObject()					{ fType = VALUE_STRING; return fString; }
		
	private:
		value_type fType;
		BString fString;
		int64 fNumber;
	};
}	// namespace BPrivate

using namespace BPrivate;

typedef ArrayParserValue YYSTYPE;

enum {
	// reached end of scanned file
	EOS = 500,
	
	// standard C type keywords
	CHAR,
	INT,
	SHORT,
	LONG,
	SIGNED,
	UNSIGNED,
	DEFINE,
	
	// common typedefs as keywords
	UCHAR,
	USHORT,
	ULONG,
	INT8,
	UINT8,
	INT16,
	UINT16,
	INT32,
	UINT32,
	INT64,
	UINT64,
	
	// symbolic types
	IDENTIFIER,
	NUMBER,
	STRING
};

status_t StartScanningString(const char* data, size_t size);
status_t StopScanningString();

#endif
