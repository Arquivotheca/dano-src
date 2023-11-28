/*
	
	I2CBus.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#include  "I2CBus.h"

//--------------------------------------------------------------------------

BI2CBus::BI2CBus(const char *name)
{
	strncpy(fName,name,32);
}

//--------------------------------------------------------------------------

BI2CBus::~BI2CBus()
{

}

//--------------------------------------------------------------------------

char *
BI2CBus::Name()
{
	return fName;
}

//--------------------------------------------------------------------------

void
BI2CBus::I2CReset()
{
	
}

//--------------------------------------------------------------------------

void
BI2CBus::I2CStart()
{
	
}

//--------------------------------------------------------------------------

void
BI2CBus::I2CStop()
{
	
}

//--------------------------------------------------------------------------

void
BI2CBus::I2COne()
{
	
}

//--------------------------------------------------------------------------

void
BI2CBus::I2CZero()
{
	
}

//--------------------------------------------------------------------------

uchar
BI2CBus::I2CAck()
{
	return 0;	
}

//--------------------------------------------------------------------------

uchar
BI2CBus::I2CSendByte(uchar , bigtime_t )
{
	return 0;	
}

//--------------------------------------------------------------------------

uchar
BI2CBus::I2CReadByte(bool )
{
	return 0;	
}

//--------------------------------------------------------------------------

void
BI2CBus::I2CWrite1(uchar , uchar )
{

}

//--------------------------------------------------------------------------

void
BI2CBus::I2CWrite2(uchar , uchar , uchar )
{

}

//--------------------------------------------------------------------------

uchar	
BI2CBus::I2CRead(uchar )
{
	return 0;
}

//--------------------------------------------------------------------------

char	
BI2CBus::I2CStatus()
{
	return 0;
}

//--------------------------------------------------------------------------

bool
BI2CBus::I2CDevicePresent(uchar )
{
	return false;
}

//--------------------------------------------------------------------------

void
BI2CBus::_ReservedI2CBus1()
{

}

//--------------------------------------------------------------------------

void
BI2CBus::_ReservedI2CBus2()
{

}

//--------------------------------------------------------------------------

void
BI2CBus::_ReservedI2CBus3()
{

}

