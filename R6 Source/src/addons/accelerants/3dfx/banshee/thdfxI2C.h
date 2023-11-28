/*
	
	3DFXI2C.h - this module derived from Bt848I2C.cpp
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef THDFX_I2C_H
#define THDFX_I2C_H

#include <unistd.h>
#include <SupportDefs.h>

#include "../common/I2CBus.h"

/* defines for I2C status */
#define THDFX_I2C_SUCCESS		 0
#define THDFX_I2C_DONE_ERROR	-1
#define THDFX_I2C_RACK_ERROR	-2

class thdfxI2C : public BI2CBus
{
public:
							thdfxI2C(const char * name, int32 thdfx);
virtual						~thdfxI2C();
							
/* low level access to i2c bits */
		void				I2CReset();
		void				I2CStart();						
		void				I2CStop();
		void				I2COne();
		void				I2CZero();
		uchar				I2CAck();
		uchar				I2CSendByte(uchar data, bigtime_t wait_for_ack);
		uchar				I2CReadByte(bool last);

/* high level transaction access to i2c */						
		void				I2CWrite1(uchar address, uchar data1);
		void				I2CWrite2(uchar address, uchar data1, uchar data2);
		uchar				I2CRead(uchar address);
		char				I2CStatus();
		bool				I2CDevicePresent(uchar address);

private:
		uchar				I2CBits();
		void				I2CSetBits(uchar clock, uchar data);

		uint32				fthdfx;
		uint32			i2c_register;
		uint32			i2c_status;

};

#endif


