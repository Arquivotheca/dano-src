/*******************************************************************************
/
/	File:			ivaluecontrol.h
/
/   Description:    This is a protocol implementation class.  That is,
		a abstract base class intended for controls to use to implement
		setting and getting a value in a nice data type independent way.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_IVALUE_CONTROL_H
#define	_IVALUE_CONTROL_H

#include "NamedData.h"

class IValueControl 
{
public:
	virtual	status_t	SetValue(const BNamedData &value)=0;
	virtual status_t	GetValue(BNamedData &value)=0;

protected:

};

#endif 
