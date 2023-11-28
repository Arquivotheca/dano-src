//******************************************************************************
//
//	File:		Color.cpp
//
//	Description:	BColor and BColor32 class implementation.
//	
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <render2/Color.h>

#include <support2/ITextStream.h>

namespace B {
namespace Render2 {

void BColor::Construct(const value_ref& ref, status_t *result)
{
	if (ref.type == B_COLOR_TYPE) {
		if (ref.length == sizeof(BColor)) {
			*this = *static_cast<const BColor*>(ref.data);
			if (result) *result = B_OK;
		} else {
			if (result) *result = B_BAD_VALUE;
			*this = BColor();
		}
	} else {
		if (result) *result = B_BAD_TYPE;
		*this = BColor();
	}
}

BColor::BColor(const BValue& o, status_t *result)
{
	Construct(value_ref(o), result);
}

BColor::BColor(const value_ref& o, status_t *result)
{
	Construct(o, result);
}

BValue
BColor::AsValue() const
{
	return BValue(B_COLOR_TYPE, this, sizeof(BColor));
}

//------------------------------------------------------------------------------

void BColor::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BColor(";
	else io << "(";
	io << "r=" << red << ", g=" << green << ", b=" << blue << ", a=" << alpha << ")";
}

status_t BColor::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BColor obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

//------------------------------------------------------------------------------
ITextOutput::arg operator<<(ITextOutput::arg io, const BColor& BColor)
{
	BColor.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

//------------------------------------------------------------------------------

void BColor32::Construct(const value_ref& ref, status_t *result)
{
	if (ref.type == B_COLOR_32_TYPE) {
		if (ref.length == sizeof(BColor32)) {
			*this = *static_cast<const BColor32*>(ref.data);
			if (result) *result = B_OK;
		} else {
			if (result) *result = B_BAD_VALUE;
			*this = BColor32();
		}
	} else {
		if (result) *result = B_BAD_TYPE;
		*this = BColor32();
	}
}

BColor32::BColor32(const BValue& o, status_t *result)
{
	Construct(value_ref(o), result);
}

BColor32::BColor32(const value_ref& o, status_t *result)
{
	Construct(o, result);
}

BValue
BColor32::AsValue() const
{
	return BValue(B_COLOR_32_TYPE, this, sizeof(BColor32));
}

//------------------------------------------------------------------------------

void BColor32::PrintToStream(ITextOutput::arg io, uint32 flags) const
{
	if (flags&B_PRINT_STREAM_HEADER) io << "BColor32(";
	else io << "(";
	io	<< "r=" << int32(red) << ", g=" << int32(green)
		<< ", b=" << int32(blue) << ", a=" << int32(alpha) << ")";
}

status_t BColor32::printer(ITextOutput::arg io, const value_ref& val, uint32 flags)
{
	status_t result;
	BColor32 obj(val, &result);
	if (result == B_OK) obj.PrintToStream(io, flags);
	return result;
}

ITextOutput::arg operator<<(ITextOutput::arg io, const BColor32& BColor)
{
	BColor.PrintToStream(io, B_PRINT_STREAM_HEADER);
	return io;
}

} }	// namespace B::Render2
