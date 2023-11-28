/*****************************************************************************

     File: StringIO.cpp

	 Written By: Dianne Hackborn

     Copyright (c) 2000 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <support2/StringIO.h>

#include <support2/Debug.h>
#include <support2/String.h>

#include <string.h>
#include <unistd.h>

namespace B {
namespace Support2 {

// ----------------------------------------------------------------- //

BStringIO::BStringIO() : BPositionIO(this), BTextOutput(this)
{
}

// ----------------------------------------------------------------- //

BStringIO::~BStringIO()
{
}

// ----------------------------------------------------------------- //

const char * BStringIO::String()
{
	if (!Buffer()) return "";
	if (!AssertSpace(Size()+1)) {
		((char*)Buffer())[Size()] = 0;
	}
	return (const char*)Buffer();
}

// ----------------------------------------------------------------- //

size_t BStringIO::StringLength() const
{
	return Size();
}

// ----------------------------------------------------------------- //

void BStringIO::Reset()
{
	Seek(0,SEEK_SET);
	End();
}

// ----------------------------------------------------------------- //

void BStringIO::PrintAndReset(ITextOutput::arg io)
{
	io << String();
	Reset();
}

// ----------------------------------------------------------------- //

} }	// namespace B::Support2
