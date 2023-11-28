/* ++++++++++
	FILE:	Flattenable.cpp
	REVS:	$Revision$
	NAME:	peter
	DATE:	Fri May 23 10:21:58 PDT 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <Flattenable.h>

/* ----------------------------------------------------------------- */

bool BFlattenable::AllowsTypeCode(type_code c) const
{
	return (c == TypeCode());
}

/* ----------------------------------------------------------------- */

BFlattenable::~BFlattenable()
{
	//	do nothing particular here
}

void BFlattenable::_ReservedFlattenable1()
{
	//	this used to be the slot now used for the virtual destructor
}

void BFlattenable::_ReservedFlattenable2() {}
void BFlattenable::_ReservedFlattenable3() {}

/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
/* ----------------------------------------------------------------- */
