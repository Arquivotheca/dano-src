//	Coercion.cpp

#include "Coercion.h"
#include <Message.h>
#include <stdio.h>
#include <memory.h>
#include <alloca.h>
#include <string.h>
#include <stdlib.h>

CoercionHandler::CoercionHandler()
{
}


CoercionHandler::~CoercionHandler()
{
}


//	The default handler only coerces "as is"
bool
CoercionHandler::CanCoerce(
	unsigned long		fromType,
	unsigned long		toType)
{
	return fromType == toType;
}


bool
CoercionHandler::DoCoerce(
	void * &			outData,	//	return block allocated with malloc()
	ssize_t &			outSize,
	unsigned long		outType,
	BMessage *			message,
	const char *		name,
	long				index)
{
	const void *data;
	status_t	err = message->FindData(name, outType, index, &data, &outSize);
	if (err != B_NO_ERROR || !data)
		return false;
	outData = malloc(outSize);
	memcpy(outData, data, outSize);
	return true;
}


bool
StandardNumberCoercions::CanCoerce(
	unsigned long		fromType,
	unsigned long		toType)
{
	bool f = false;
	bool t = false;

	switch (fromType) {
		case B_INT32_TYPE:
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_INT16_TYPE:
		case B_UINT8_TYPE:
		case B_UINT16_TYPE:
		case B_UINT32_TYPE:
			f = true;
			break;
		}
	
		switch (toType) {
		case B_INT32_TYPE:
		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_INT16_TYPE:
		case B_UINT8_TYPE:
		case B_UINT16_TYPE:
		case B_UINT32_TYPE:
			t = true;
			break;
	}

	return f && t;
}


bool
StandardNumberCoercions::DoCoerce(
	void * &			outData,	//	return block allocated with malloc()
	ssize_t &			outSize,
	unsigned long		outType,
	BMessage *			message,
	const char *		name,
	long				index)
{
	double floatVal = 0;
	float f;
	long intVal = 0;
	short s;
	char c;
	type_code type;
	long size;

	if (message->GetInfo(name, &type, &size))
		return false;
	const void *data;
	status_t	err = message->FindData(name, type, index, &data, &size);
	if (err != B_NO_ERROR || !data)
		return false;

	switch (type) {
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			memcpy(&intVal, data, sizeof(intVal));
			floatVal = intVal;
			break;
		case B_FLOAT_TYPE:
			memcpy(&f, data, sizeof(f));
			floatVal = f;
			intVal = (long)f;
			break;
		case B_DOUBLE_TYPE:
			memcpy(&floatVal, data, sizeof(floatVal));
			intVal = (long)floatVal;
			break;
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_UINT8_TYPE:
			memcpy(&c, data, sizeof(c));
			floatVal = c;
			intVal = c;
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			memcpy(&s, data, sizeof(s));
			floatVal = s;
			intVal = s;
			break;
		default:
			return false;
	}

	outData = malloc(8);		//	OK, so we know sizeof(double) is the longest we'll see
	switch (outType) {
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			outSize = sizeof(long);
			memcpy(outData, &intVal, outSize);
			break;
		case B_FLOAT_TYPE:
			f = floatVal;
			outSize = sizeof(float);
			memcpy(outData, &f, outSize);
			break;
		case B_DOUBLE_TYPE:
			outSize = sizeof(double);
			memcpy(outData, &floatVal, outSize);
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			s = intVal;
			outSize = sizeof(short);
			memcpy(outData, &s, outSize);
			break;
		case B_CHAR_TYPE:
		case B_UINT8_TYPE:
		case B_BOOL_TYPE:
			c = intVal;
			outSize = sizeof(char);
			memcpy(outData, &c, outSize);
			break;
		default:
			free(outData);
			outData = NULL;
			return false;
	}

	return true;
}


bool
StandardGraphicsCoercions::CanCoerce(
	unsigned long		fromType,
	unsigned long		toType)
{
	int f = 0;

	switch (fromType) {
	case B_INT32_TYPE:
	case B_FLOAT_TYPE:
	case B_DOUBLE_TYPE:
		f = 1;
		break;
	case B_POINT_TYPE:
		f = 2;
		break;
	}

	/*	points can be made of numbers
	 *	rects can be made of numbers or points
	 *	colors can be made of numbers
	 */
	return	((toType == B_POINT_TYPE) && (f == 1)) ||
			((toType == B_RECT_TYPE) && ((f == 1) || (f == 2))) ||
			((toType == B_RGB_COLOR_TYPE) && (f == 1));
}


	static float
	GetFloat(
		BMessage *		message,
		const char *	name,
		int &			pos)
	{
		float f = 0.0;
		type_code type;
		long count;
		message->GetInfo(name, &type, &count);
		if (count < pos)
			return f;		//	can happen for rgb.alpha
		switch (type) {
			case B_INT32_TYPE:
				f = message->FindInt32(name, pos);
				break;
			case B_FLOAT_TYPE:
				f = message->FindFloat(name, pos);
				break;
			case B_DOUBLE_TYPE:
				f = message->FindDouble(name, pos);
				break;
		}
		pos++;
		return f;
	}

	static BPoint
	GetPoint(
		BMessage *		message,
		const char *	name,
		int &			pos)
	{
		BPoint ret(B_ORIGIN);
		type_code type;
		long count;
		message->GetInfo(name, &type, &count);
		if (count < pos)
			return ret;		//	shouldn't happen
		switch (type) {
			case B_INT32_TYPE:
			case B_FLOAT_TYPE:
			case B_DOUBLE_TYPE:
				ret.x = GetFloat(message, name, pos);
				ret.y = GetFloat(message, name, pos);
				break;
			case B_POINT_TYPE:
				ret = message->FindPoint(name, pos);
			default:
				pos++;
				break;
		}
		return ret;
	}


bool
StandardGraphicsCoercions::DoCoerce(
	void * &			outData,	//	return block allocated with malloc()
	long &				outSize,
	unsigned long		outType,
	BMessage *			message,
	const char *		name,
	long				index)
{
	type_code type;
	long count;
	if (message->GetInfo(name, &type, &count))
		return false;

	/*	Check that we know input types
	 */
	switch (type) {
	case B_INT32_TYPE:
	case B_FLOAT_TYPE:
	case B_DOUBLE_TYPE:
	case B_POINT_TYPE:
		break;
	default:
		return false;
	}

	/*	Check that there's enough to go around
	 */
	switch (outType) {
	case B_POINT_TYPE:
		if ((count-index < 2) || (type == B_POINT_TYPE))
			return false;
		break;
	case B_RECT_TYPE:
		if ((count-index < 2) || ((count-index < 4) && (type != B_POINT_TYPE)))
			return false;
		break;
	case B_RGB_COLOR_TYPE:
		if ((count-index < 3) || (type == B_POINT_TYPE))
			return false;
		break;
	default:
		return false;
	}

	/*	Do the conversion
	 */
	int pos = index;
	if (outType == B_POINT_TYPE) {
		BPoint ret = GetPoint(message, name, pos);
		outSize = sizeof(BPoint);
		outData = malloc(outSize);
		memcpy(outData, &ret, outSize);
		return true;
	}
	if (outType == B_RECT_TYPE) {
		BPoint leftTop = GetPoint(message, name, pos);
		BPoint rightBottom = GetPoint(message, name, pos);
		BRect ret(leftTop, rightBottom);
		outSize = sizeof(BRect);
		outData = malloc(outSize);
		memcpy(outData, &ret, outSize);
		return true;
	}
	if (outType == B_RGB_COLOR_TYPE) {
		rgb_color ret;
		ret.red = (uint8) GetFloat(message, name, pos);
		ret.green = (uint8) GetFloat(message, name, pos);
		ret.blue = (uint8) GetFloat(message, name, pos);
		ret.alpha = (uint8) GetFloat(message, name, pos);
		outSize = sizeof(rgb_color);
		outData = malloc(outSize);
		memcpy(outData, &ret, outSize);
		return true;
	}
	/*	Should not reach here
	 */
	return false;
}


bool
StandardTextCoercions::CanCoerce(
	unsigned long		fromType,
	unsigned long		toType)
{
	bool f = false;
	bool t = false;

	switch (fromType) {
	case B_INT32_TYPE:
	case B_FLOAT_TYPE:
	case B_DOUBLE_TYPE:
	case B_BOOL_TYPE:
	case B_CHAR_TYPE:
	case B_INT16_TYPE:
	case B_UINT8_TYPE:
	case B_UINT16_TYPE:
	case B_UINT32_TYPE:
	case B_ASCII_TYPE:
	case B_STRING_TYPE:
		f = true;
		break;
	}

	switch (toType) {
	case B_ASCII_TYPE:
	case B_STRING_TYPE:
	case B_REF_TYPE:	/*	Can do entry_ref as path */
		t = true;
		break;
	}

	return f && t;
}


bool
StandardTextCoercions::DoCoerce(
	void * &			outData,	//	return block allocated with malloc()
	long &				outSize,
	unsigned long		outType,
	BMessage *			message,
	const char *		name,
	long				index)
{
	type_code type;
	ssize_t size;
	long err = 0;
	long count;

	if (message->GetInfo(name, &type, &count))
		return false;
	const void *data;
	status_t	err1 = message->FindData(name, type, index, &data, &size);
	if (err1 != B_NO_ERROR || !data)
		return false;

	if (outType == B_REF_TYPE) {
		char *temp;
		entry_ref	ref;
		switch (type) {
			case B_ASCII_TYPE:
				temp = (char *)alloca(size+1);
				memcpy(temp, data, size);
				temp[size] = 0;
				data = temp;
			case B_STRING_TYPE:
				err = get_ref_for_path((char *)data, &ref);
				if (err) {
					outData = NULL;
				} else {
					// flatten the entry_ref
					int len = strlen(ref.name) + 1;
					outSize = sizeof(dev_t) + sizeof(ino_t) + len;
					outData = malloc(outSize);
					entry_ref*		outRef = (entry_ref*) outData;
					outRef->device = ref.device;
					outRef->directory = ref.directory;
					memcpy(&outRef->name, ref.name, len);
				}
				return !err;
	
			default:
				return false;
		}
		/*	not reached */
	}

	/*	For text to text, it's easy */
	if ((type == B_STRING_TYPE) || (type == B_ASCII_TYPE)) {
		outData = malloc(size+1);
		memcpy(outData, data, size);
		outSize = strlen((char *)outData);
		if (type == B_STRING_TYPE)
			outSize += 1;

	/*	For numerics to text, it's slightly harder */
	} else {
		/*	Lots of working variables */
		char c;
		unsigned char uc;
		short s;
		unsigned short us;
		long l;
		unsigned long ul;
		float f;
		double d;
		/*	Allocate space */
		outData = malloc(100);	/*	no format will exceed this size */
#define X(m, v)	memcpy(&v, data, sizeof(v)); \
				sprintf((char *)outData, m, v)
		/*	Get and format the actual values */
		switch (type) {
		case B_DOUBLE_TYPE:
			X("%.f", d);
			break;
		case B_FLOAT_TYPE:
			X("%.f", f);
			break;
		case B_INT32_TYPE:
			X("%ld", l);
			break;
		case B_UINT32_TYPE:
			X("%lu", ul);
			break;
		case B_INT16_TYPE:
			X("%d", s);
			break;
		case B_UINT16_TYPE:
			X("%d", us);
			break;
		case B_CHAR_TYPE:
			X("%d", c);
			break;
		case B_UINT8_TYPE:
			X("%d", uc);
			break;
		case B_BOOL_TYPE:
			if (*(char *)data) {
				strcpy((char *)outData, "true");
			} else {
				strcpy((char *)outData, "false");
			}
			break;
		default:
			free(outData);
			return false;
		}
		/*	Fix up size for destination type */
		outSize = strlen((char *)outData);
		if (outType == B_STRING_TYPE)
			outSize += 1;
	}

	return true;
}



Coercions::Coercions()
{
	AddCoercionHandler(new CoercionHandler);
}


Coercions::~Coercions()
{
	while (handlers.CountItems()) {
		delete (CoercionHandler *)handlers.RemoveItem(handlers.CountItems()-1);
	}
}


void
Coercions::AddCoercionHandler(
	CoercionHandler *	handler)
{
	if (!handlers.HasItem(handler))
		handlers.AddItem(handler);
}


bool
Coercions::GetRef(
	entry_ref &			outRef,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_REF_TYPE, message, name, item);
	if (result) {
		// unflatten the entry_ref
		entry_ref*		ref = (entry_ref*) data;
		
		outRef.device = ref->device;
		outRef.directory = ref->directory;
		free(outRef.name);
		outRef.name = strdup((const char *)&ref->name);
		free(data);
	}
	return result;
}



bool
Coercions::GetDouble(
	double &			outDouble,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_DOUBLE_TYPE, message, name, item);
	if (result) {
		memcpy(&outDouble, data, sizeof(double));
		free(data);
	}
	return result;
}


bool
Coercions::GetFloat(
	float &				outFloat,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_FLOAT_TYPE, message, name, item);
	if (result) {
		memcpy(&outFloat, data, sizeof(float));
		free(data);
	}
	return result;
}


bool
Coercions::GetLong(
	long &				outLong,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_INT32_TYPE, message, name, item);
	if (result) {
		memcpy(&outLong, data, sizeof(long));
		free(data);
	}
	return result;
}


bool
Coercions::GetString(
	char * &			outString,	//	call free()
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_ASCII_TYPE, message, name, item);
	if (result) {
		outString = (char *)malloc(size+1);
		memcpy(outString, data, size);
		outString[size] = 0;
		free(data);
	}
	return result;
}


bool
Coercions::GetRect(
	BRect &				outRect,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_RECT_TYPE, message, name, item);
	if (result) {
		memcpy(&outRect, data, sizeof(BRect));
		free(data);
	}
	return result;
}


bool
Coercions::GetPoint(
	BPoint &			outPoint,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_POINT_TYPE, message, name, item);
	if (result) {
		memcpy(&outPoint, data, sizeof(BPoint));
		free(data);
	}
	return result;
}


bool
Coercions::GetColor(
	rgb_color &			outColor,
	BMessage *			message,
	const char *		name,
	long				item)
{
	void *data;
	long size;
	bool result = DoCoerce(data, size, B_RGB_COLOR_TYPE, message, name, item);
	if (result) {
		memcpy(&outColor, data, sizeof(rgb_color));
		free(data);
	}
	return result;
}


bool
Coercions::DoCoerce(
	void * &			outData,	//	call free()
	long &				outDataSize,
	unsigned long		toType,
	BMessage *			message,
	const char *		name,
	long				item)
{
	type_code type;
	long count;
	if (message->GetInfo(name, &type, &count))
		return false;
	int iterator = 0;
again:
	CoercionHandler *handler = FindHandler(type, toType, iterator);
	if (!handler)
		return false;
	if (!handler->DoCoerce(outData, outDataSize, toType, message, name, item)) {
		iterator++;
		goto again;	/*	try again with the next supposed match	*/
	}
	return true;
}


CoercionHandler *
Coercions::FindHandler(
	unsigned long		inType,
	unsigned long		outType,
	int &				iterator)
{
	for (; iterator<handlers.CountItems(); iterator++) {
		CoercionHandler *handler = (CoercionHandler *)handlers.ItemAt(iterator);
		if (handler->CanCoerce(inType, outType))
			return handler;
	}
	return NULL;
}

