
/*

XXX	Things that remain XXX

	Get and Set for XPath and for rgb_color 

*/

#include <render2/Rect.h>
#include <xml2/BContent.h>

#include <string.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

namespace B {
namespace XML {



// =====================================================================
// ==         GET FUNCTIONS BEGIN HERE !!!                            ==
// =====================================================================

// =====================================================================
status_t
BValued::GetValue(const char ** value) const
{
	*value = _value.String();
	return B_OK;
}


// =====================================================================
status_t
BValued::GetValue(BString * value) const
{
	*value = _value;
	return B_OK;
}


#if !_SMALL_XML_FOOTPRINT_

// =====================================================================
// S is space, # is a floating point number, ',' is a comma
// The format must be this:  S+ # S+ ','  S+ # S+ ','  S+ # S+ ','  S+ # S+
// The order is left, top, right, bottom
status_t
BValued::GetValue(BRect * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
//LEFT:
	// Preceeding whitespace and left
	double left = strtod(str, &end);
	str = end;
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	// Soak up whitespace between the end of the number and the comma
	while (*str && (isspace(*str) || *str == ','))
	{
		if (*str == ',')
			goto TOP;
		str++;
	}
	// Either not space or end of string
	return B_XML_CANT_CONVERT_STRING;
	
TOP:
	++str;
	double top = strtod(str, &end);
	str = end;
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*str && (isspace(*str) || *str == ','))
	{
		if (*str == ',')
			goto RIGHT;
		str++;
	}
	return B_XML_CANT_CONVERT_STRING;
	
RIGHT:
	++str;
	double right = strtod(str, &end);
	str = end;
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*str && (isspace(*str) || *str == ','))
	{
		if (*str == ',')
			goto BOTTOM;
		str++;
	}
	return B_XML_CANT_CONVERT_STRING;
	
BOTTOM:
	++str;
	double bottom = strtod(str, &end);
	str = end;
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*str)
		if (!isspace(*str++))
			return B_XML_CANT_CONVERT_STRING;

//BUILD_B_RECT:
	// We won! return!
	value->left = left;
	value->top = top;
	value->right = right;
	value->bottom = bottom;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(BPoint * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
//X:
	// Preceeding whitespace and left
	double x = strtod(str, &end);
	str = end;
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	// Soak up whitespace between the end of the number and the comma
	while (*str && (isspace(*str) || *str == ','))
	{
		if (*str == ',')
			goto Y;
		str++;
	}
	// Either not space or end of string
	return B_XML_CANT_CONVERT_STRING;
	
Y:
	++str;
	double y = strtod(str, &end);
	str = end;
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*str)
		if (!isspace(*str++))
			return B_XML_CANT_CONVERT_STRING;
	
//BUILD_B_POINT:
	value->x = x;
	value->y = y;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(int8 * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	long i = strtol(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	if (i <= SCHAR_MAX && i >= SCHAR_MIN)
	{
		*value = (int8) i;
		return B_NO_ERROR;
	}
	
	return B_XML_CANT_CONVERT_STRING;
}


// =====================================================================
status_t
BValued::GetValue(int16 * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	long i = strtol(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	if (i <= SHRT_MAX && i >= SHRT_MIN)
	{
		*value = (int16) i;
		return B_NO_ERROR;
	}
	
	return B_XML_CANT_CONVERT_STRING;
}


// =====================================================================
status_t
BValued::GetValue(int32 * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	long i = strtol(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	*value = (int32) i;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(int64 * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	long long i = strtoll(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	*value = (int64) i;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(uint8 * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	long i = strtol(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	if (i <= UCHAR_MAX && i >= 0)
	{
		*value = (uint8) i;
		return B_NO_ERROR;
	}
	
	return B_XML_CANT_CONVERT_STRING;

}


// =====================================================================
status_t
BValued::GetValue(uint16 * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	long i = strtol(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	if (i <= USHRT_MAX && i >= 0)
	{
		*value = (uint16) i;
		return B_NO_ERROR;
	}
	
	return B_XML_CANT_CONVERT_STRING;
}


// =====================================================================
status_t
BValued::GetValue(uint32 * value) const
{
	const char * str = _value.String();
	const char * tmp = str;
	char * end;
	
	errno = 0;
	
	// Special check because the stroull doesn't properly
	// return errors for negative numbers
	while (*tmp && isspace(*tmp))
		tmp++;
	if (*tmp == '-')
		return B_XML_CANT_CONVERT_STRING;
	
	long i = strtoul(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	*value = (uint32) i;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(uint64 * value) const
{
	const char * str = _value.String();
	const char * tmp = str;
	char * end;
	
	errno = 0;
	
	// Special check because the stroull doesn't properly
	// return errors for negative numbers
	while (*tmp && isspace(*tmp))
		tmp++;
	if (*tmp == '-')
		return B_XML_CANT_CONVERT_STRING;
	
	unsigned long long i = strtoull(str, &end, 0);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	*value = (uint64) i;
	return B_NO_ERROR;
}


// =====================================================================
// Ways of representing TRUE:
// 		- The string "true", case insensitive
// Ways of representing FALSE:
// 		- The string "false", case insensitive
// Everything else is an error
status_t
BValued::GetValue(bool * value) const
{
	const char * str = _value.String();
	
	if (0 == strcasecmp("true", str))
	{
		*value = true;
		return B_NO_ERROR;
	}
	
	if (0 == strcasecmp("false", str))
	{
		*value = false;
		return B_NO_ERROR;
	}
	
	return B_XML_CANT_CONVERT_STRING;
}


// =====================================================================
// FLT_MIN is set to 0 for some reason.  So we don't do range checking.
status_t
BValued::GetValue(float * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	double d = strtod(str, &end);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	*value = (float) d;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(double * value) const
{
	const char * str = _value.String();
	char * end;
	
	errno = 0;
	
	double d = strtod(str, &end);
	
	if (errno)
		return B_XML_CANT_CONVERT_STRING;
	while (*end)
		if (!isspace(*end++))
			return B_XML_CANT_CONVERT_STRING;
	
	*value = (double) d;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BValued::GetValue(rgb_color * value) const
{
	// XXX Waiting for libwww
	return B_ERROR;
}


// =====================================================================
status_t
BValued::GetValueXPath(BContent ** pathTo)
{
	// XXX waiting for XPath
	return B_ERROR;
}

#endif

// =====================================================================
void
BValued::GetValueRaw(BString & value) const
{
	value = "";
	
	const char * str = _value.String();
	while (*str)
	{
		switch (*str)
		{
			// This is not UTF-8 safe, is it?
			case '<':
				value.Append("&lt;");
				break;
			case '>':
				value.Append("&gt;");
				break;
			case '&':
				value.Append("&amp;");
				break;
			case '\'':
				value.Append("&apos;");
				break;
			case '\"':
				value.Append("&quot;");
				break;
			default:
				value += *str;
		}
		str++;
	}
}

// =====================================================================
// ==         SET FUNCTIONS BEGIN HERE !!!                            ==
// =====================================================================


// =====================================================================
status_t
BValued::SetValue(const char * value)
{
	status_t err;
	BString str(value);
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(const char * value, int32 length)
{
	status_t err;
	BString str(value, length);
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(BString & value, bool adopt)
{
	status_t err;
	err = ValidateValueChange(value);
	if (err != B_OK)
		return err;
	
	if (adopt)
		_value.Adopt(value);
	else
		_value = value;
	return B_OK;
}


#if !_SMALL_XML_FOOTPRINT_

// =====================================================================
status_t
BValued::SetValue(BRect value)
{
	status_t err;
	BString str("");
	
	str << value.left;
	str << ',';
	str << value.top;
	str << ',';
	str << value.right;
	str << ',';
	str << value.bottom;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(BPoint value)
{
	status_t err;
	BString str("");
	
	str << value.x;
	str << ',';
	str << value.y;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(int8 value)
{
	status_t err;
	BString str("");
	
	str << (int32) value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(int16 value)
{
	status_t err;
	BString str("");
	
	str << value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(int32 value)
{
	status_t err;
	BString str("");
	
	str << value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(int64 value)
{
	status_t err;
	BString str("");
	
	str << value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(uint8 value)
{
	status_t err;
	BString str("");
	
	str << (int32) value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(uint16 value)
{
	status_t err;
	BString str("");
	
	str << (int) value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(uint32 value)
{
	status_t err;
	BString str("");
	
	str << value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(uint64 value)
{
	status_t err;
	BString str("");
	
	str << value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(bool value)
{
	status_t err;
	BString str = value ? "true" : "false";
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(float value)
{
	status_t err;
	BString str("");
	
	str << value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::SetValue(double value)
{
	status_t err;
	BString str("");
	
	str << (float) value;
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
// val < 16
static char
nibble2char(char val)
{
	if (val > 15)
		return ' ';
	else if (val < 10)
		return '0' + val;
	else
		return 'A' + val - 10;
}
status_t
BValued::SetValueRGBColor(rgb_color value)
{
	BString str("#");
	str += nibble2char((value.red & 0xF0) >> 4);
	str += nibble2char(value.red & 0x0F);
	str += nibble2char((value.green & 0xF0) >> 4);
	str += nibble2char(value.green & 0x0F);
	str += nibble2char((value.blue & 0xF0) >> 4);
	str += nibble2char(value.blue & 0x0F);
	return B_ERROR;
}


// =====================================================================
static bool
color_compare(rgb_color col, uint8 red, uint8 green, uint8 blue)
{
	return col.red == red && col.green == green && col.blue == blue;
}
status_t
BValued::SetValueHTMLColor(rgb_color value)
{
	if (color_compare(value, 0x00, 0x00, 0x00))
		return SetValue("black");
	else if (color_compare(value, 0xC0, 0xC0, 0xC0))
		return SetValue("silver");
	else if (color_compare(value, 0x80, 0x80, 0x80))
		return SetValue("gray");
	else if (color_compare(value, 0xFF, 0xFF, 0xFF))
		return SetValue("white");
	else if (color_compare(value, 0x80, 0x00, 0x00))
		return SetValue("maroon");
	else if (color_compare(value, 0xFF, 0x00, 0x00))
		return SetValue("red");
	else if (color_compare(value, 0x80, 0x00, 0x80))
		return SetValue("purple");
	else if (color_compare(value, 0xFF, 0x00, 0xFF))
		return SetValue("fuchsia");
	else if (color_compare(value, 0x00, 0x80, 0x00))
		return SetValue("green");
	else if (color_compare(value, 0x00, 0xFF, 0x00))
		return SetValue("lime");
	else if (color_compare(value, 0x80, 0x80, 0x00))
		return SetValue("olive");
	else if (color_compare(value, 0xFF, 0xFF, 0x00))
		return SetValue("yellow");
	else if (color_compare(value, 0x00, 0x00, 0x80))
		return SetValue("navy");
	else if (color_compare(value, 0x00, 0x00, 0xFF))
		return SetValue("blue");
	else if (color_compare(value, 0x00, 0x80, 0x80))
		return SetValue("teal");
	else if (color_compare(value, 0x00, 0xFF, 0xFF))
		return SetValue("aqua");
	else
		return SetValueRGBColor(value);
}


// =====================================================================
status_t
BValued::SetValueXPath(BContent * value)
{
	// XXX Waiting for XPath
	return B_ERROR;
}


#endif


// =====================================================================
// ==         STRING MANIPULATION FUNCTIONS BEGIN HERE !!!            ==
// =====================================================================

// =====================================================================
status_t
BValued::Append(const char * value)
{
	status_t err;
	BString str = _value;
	
	str.Append(value);
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::Insert(const char * value, int32 start)
{
	status_t err;
	BString str = _value;
	
	str.Insert(value, start);
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::Remove(int32 start, int32 len)
{
	status_t err;
	BString str = _value;
	
	str.Remove(start, len);
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}


// =====================================================================
status_t
BValued::Replace(const char * value, int32 start, int32 len)
{
	// XXX Somebody test me!!!
	
	status_t err;
	BString str;
	
	str.SetTo(_value, start);
	str.Append(value);
	if (start + len < _value.Length())
		str.Append(_value.String() + start + len + 1);
	
	err = ValidateValueChange(str);
	if (err != B_OK)
		return err;
	
	_value = str;
	return B_OK;
}



// =====================================================================
// ==         VALIDATION FUNCTIONS BEGIN HERE !!!                     ==
// =====================================================================

// =====================================================================
status_t
BValued::ValidateValueChange(BString & newVal)
{
	(void) newVal;
	return B_OK;
}



// =====================================================================
// ==         CONSTRUCTOR AND DESTRUCTOR BEGIN HERE !!!               ==
// =====================================================================

// =====================================================================
BValued::BValued(const char * value)
	:_value("")
{
	status_t err;
	
	// XXX this is yucky.  Any better suggestions?
	BString str(value);
	err = ValidateValueChange(str);
	if (err != B_OK)
		return ;
	_value = str;
}


// =====================================================================
BValued::BValued(const BString & value)
	:_value("")
{
	status_t err;
	
	// XXX this is yucky.  Any better suggestions?
	BString str(value);
	err = ValidateValueChange(str);
	if (err != B_OK)
		return ;
	_value = str;
}


// =====================================================================
BValued::BValued(const BValued & copy)
	:_value(copy._value)
{
	
}


// =====================================================================
BValued::BValued()
	:_value("")
{
	
}


// =====================================================================
BValued::~BValued()
{
	
}


}; // namespace XML
}; // namespace B



