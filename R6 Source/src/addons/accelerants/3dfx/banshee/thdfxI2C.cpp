/*
	
	thdfxI2C.cpp - this module derived from Bt848I2C.cpp
	
	Copyright 1999 Be Incorporated, All Rights Reserved.
	
*/

#include <stdio.h>

#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/common/debug.h>

#include  "thdfxI2C.h"


//------------------------------------------------------------------------------

thdfxI2C::thdfxI2C(const char *name, int32 thdfx):BI2CBus(name)
{
	fthdfx = thdfx;
}

//------------------------------------------------------------------------------

thdfxI2C::~thdfxI2C()
{

}

//--------------------------------------------------------------------------

uchar
thdfxI2C::I2CBits()
{
	ioctl(fthdfx,THDFX_IOCTL_READ_I2C_REG,&i2c_register);
	return ((uchar)(i2c_register & 0x03));	
}

//--------------------------------------------------------------------------

void
thdfxI2C::I2CSetBits(uchar clock, uchar data)
{
	i2c_register = ((data & 0x01) << 1) | (clock & 0x01);
	ioctl(fthdfx,THDFX_IOCTL_WRITE_I2C_REG,&i2c_register, sizeof(i2c_register));
	//snooze(10);	
}

//--------------------------------------------------------------------------

void
thdfxI2C::I2CReset()
{
	I2CSetBits(1,1);	
}

//--------------------------------------------------------------------------

void
thdfxI2C::I2CStart()
{
	I2CSetBits(0,1);
	I2CSetBits(1,1);
	I2CSetBits(1,0);
	I2CSetBits(0,0);	
}

//--------------------------------------------------------------------------

void
thdfxI2C::I2CStop()
{
	I2CSetBits(0,0);
	I2CSetBits(1,0);
	I2CSetBits(1,1);	
}

//--------------------------------------------------------------------------

void
thdfxI2C::I2COne()
{
	I2CSetBits(0,1);
	I2CSetBits(1,1);
	I2CSetBits(0,1);	
}

//--------------------------------------------------------------------------

void
thdfxI2C::I2CZero()
{
	I2CSetBits(0,0);
	I2CSetBits(1,0);
	I2CSetBits(0,0);	
}

//--------------------------------------------------------------------------

uchar
thdfxI2C::I2CAck()
{
    int ack;
    
    I2CSetBits(0,1);
    I2CSetBits(1,1);
    ack = I2CBits();
    I2CSetBits(0,1);

    if (ack == 3)
		{
    	i2c_status |= THDFX_I2C_RACK_ERROR;
//			dprintf(("3dfx_accel: I2CAck - ACK ERROR ack=0x%x\n", ack));
		}
    else
    	i2c_status &= !THDFX_I2C_RACK_ERROR;
    
    return ack;
}

//--------------------------------------------------------------------------

uchar
thdfxI2C::I2CSendByte(uchar data, bigtime_t wait_for_ack)
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
thdfxI2C::I2CReadByte(bool last)
{
    unsigned char data=0;
    
    I2CSetBits(0,1);
    for (int32 i=7; i>=0; i--)
    {
		I2CSetBits(1,1);
		if (I2CBits() & 0x02)
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
thdfxI2C::I2CWrite1(uchar address, uchar data1)
{
#ifdef HW
	fConfig->i2c_address = address;
	fConfig->i2c_data1 = data1;
	ioctl(fthdfx,BT848_I2CWRITE1,fConfig);
#else
    I2CStart();
    I2CSendByte(address << 1,0);
    I2CSendByte(data1,0);
    I2CStop();
#endif
}

//------------------------------------------------------------------------------

void
thdfxI2C::I2CWrite2(uchar address, uchar data1, uchar data2)
{
#ifdef HW
	fConfig->i2c_address = address;
	fConfig->i2c_data1 = data1;
	fConfig->i2c_data2 = data2;
	ioctl(fthdfx,BT848_I2CWRITE2,fConfig);
#else
//dprintf(("3dfx_accel: I2CWrite2, address = 0x%x, data1 = 0x%x, data2 = 0x%x\n", address, data1, data2));
    I2CStart();
    I2CSendByte(address,0);
    I2CSendByte(data1,0);
		I2CSendByte(data2,0);
    I2CStop();
#endif
}

//------------------------------------------------------------------------------

uchar	
thdfxI2C::I2CRead(uchar address)
{
#ifdef HW
	fConfig->i2c_address = address;
	ioctl(fthdfx,BT848_I2CREAD,fConfig);
	return(fConfig->i2c_data1);
#else
    int ret;
    I2CStart();
    I2CSendByte(address, 0);
    ret = I2CReadByte(true);
    I2CStop();
//dprintf(("3dfx_accel: I2CRead, address = 0x%x, ret = 0x%x\n", address, ret));
    return ret;
#endif
}

//------------------------------------------------------------------------------

char	
thdfxI2C::I2CStatus()
{
	return(i2c_status);
}

//------------------------------------------------------------------------------

bool
thdfxI2C::I2CDevicePresent(uchar address)
{
	I2CRead(address);
	if (I2CStatus() != THDFX_I2C_RACK_ERROR)
		return true;
	else
		return false;
}

//------------------------------------------------------------------------------

