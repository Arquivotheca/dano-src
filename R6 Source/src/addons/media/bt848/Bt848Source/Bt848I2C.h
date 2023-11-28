/*
	
	Bt848I2C.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef BT848_I2C_H
#define BT848_I2C_H

#include <unistd.h>
#include <SupportDefs.h>
#include <bt848_driver.h>

#include "I2CBus.h"

class Bt848I2C : public BI2CBus
{
public:
							Bt848I2C(const char * name, int32 bt848, bt848_config *config);
virtual						~Bt848I2C();
							
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

		uint32				fBt848;
		bt848_config		*fConfig;

};

#endif


