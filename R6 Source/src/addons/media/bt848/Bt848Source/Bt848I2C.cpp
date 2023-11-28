/*
	
	Bt848I2C.cpp
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#include <stdio.h>
#include  "Bt848I2C.h"

//------------------------------------------------------------------------------

Bt848I2C::Bt848I2C(const char *name, int32 bt848, bt848_config *config):BI2CBus(name)
{
	fBt848 = bt848;
	fConfig = config;
}

//------------------------------------------------------------------------------

Bt848I2C::~Bt848I2C()
{

}

//--------------------------------------------------------------------------

uchar
Bt848I2C::I2CBits()
{
	ioctl(fBt848,BT848_READ_I2C_REG,fConfig);
	return ((uchar)(fConfig->i2c_register & 0x03));	
}

//--------------------------------------------------------------------------

void
Bt848I2C::I2CSetBits(uchar clock, uchar data)
{
	fConfig->i2c_register = ((clock & 0x01) << 1) | (data & 0x01);
	ioctl(fBt848,BT848_WRITE_I2C_REG,fConfig);
	//snooze(10);	
}

//--------------------------------------------------------------------------

void
Bt848I2C::I2CReset()
{
	I2CSetBits(1,1);	
}

//--------------------------------------------------------------------------

void
Bt848I2C::I2CStart()
{
	I2CSetBits(0,1);
	I2CSetBits(1,1);
	I2CSetBits(1,0);
	I2CSetBits(0,0);	
}

//--------------------------------------------------------------------------

void
Bt848I2C::I2CStop()
{
	I2CSetBits(0,0);
	I2CSetBits(1,0);
	I2CSetBits(1,1);	
}

//--------------------------------------------------------------------------

void
Bt848I2C::I2COne()
{
	I2CSetBits(0,1);
	I2CSetBits(1,1);
	I2CSetBits(0,1);	
}

//--------------------------------------------------------------------------

void
Bt848I2C::I2CZero()
{
	I2CSetBits(0,0);
	I2CSetBits(1,0);
	I2CSetBits(0,0);	
}

//--------------------------------------------------------------------------

uchar
Bt848I2C::I2CAck()
{
    int ack;
    
    I2CSetBits(0,1);
    I2CSetBits(1,1);
    ack = I2CBits();
    I2CSetBits(0,1);

    if (ack == 3)
    	fConfig->i2c_status |= BT848_I2C_RACK_ERROR;
    else
    	fConfig->i2c_status &= !BT848_I2C_RACK_ERROR;
    
    return ack;
}

//--------------------------------------------------------------------------

uchar
Bt848I2C::I2CSendByte(uchar data, bigtime_t wait_for_ack)
{
	I2CSetBits(0,0);
	
	for (int32 i=7; i>=0; i--)
	{
		(data&(1<<i)) ? I2COne() : I2CZero();
	}

	if (wait_for_ack)
		snooze(wait_for_ack);

    return I2CAck();

}

//--------------------------------------------------------------------------

uchar
Bt848I2C::I2CReadByte(bool last)
{
    unsigned char data=0;
    
    I2CSetBits(0,1);
    for (int32 i=7; i>=0; i--)
    {
		I2CSetBits(1,1);
		if (I2CBits() & 0x01)
			data |= (1<<i);
		I2CSetBits(0,1);
    }
    last ? I2COne() : I2CZero();
    return data;
}

//------------------------------------------------------------------------------

// comment out the line below to bit bang I2C transactions
//#define HW 1

void
Bt848I2C::I2CWrite1(uchar address, uchar data1)
{
#ifdef HW
	fConfig->i2c_address = address;
	fConfig->i2c_data1 = data1;
	ioctl(fBt848,BT848_I2CWRITE1,fConfig);
#else
    I2CStart();
    I2CSendByte(address << 1,0);
    I2CSendByte(data1,0);
    I2CStop();
#endif
}

//------------------------------------------------------------------------------

void
Bt848I2C::I2CWrite2(uchar address, uchar data1, uchar data2)
{
#ifdef HW
	fConfig->i2c_address = address;
	fConfig->i2c_data1 = data1;
	fConfig->i2c_data2 = data2;
	ioctl(fBt848,BT848_I2CWRITE2,fConfig);
#else
    I2CStart();
    I2CSendByte(address << 1,0);
    I2CSendByte(data1,0);
	I2CSendByte(data2,0);
    I2CStop();
#endif
}

//------------------------------------------------------------------------------

uchar	
Bt848I2C::I2CRead(uchar address)
{
#ifdef HW
	fConfig->i2c_address = address;
	ioctl(fBt848,BT848_I2CREAD,fConfig);
	return(fConfig->i2c_data1);
#else
    int ret;
    I2CStart();
    I2CSendByte((address << 1) | 1 , 0);
    ret = I2CReadByte(true);
    I2CStop();
    return ret;
#endif
}

//------------------------------------------------------------------------------

char	
Bt848I2C::I2CStatus()
{
	return(fConfig->i2c_status);
}

//------------------------------------------------------------------------------

bool
Bt848I2C::I2CDevicePresent(uchar address)
{
	I2CRead(address);
	if (I2CStatus() != BT848_I2C_RACK_ERROR)
		return true;
	else
		return false;
}

//------------------------------------------------------------------------------

