/*
	
	I2CBus.h
	
	Copyright 1997-8 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _I2CBUS_H
#define _I2CBUS_H

#include <string.h>
#include <SupportDefs.h>

class BI2CBus
{
public:
							BI2CBus(const char *name);
virtual						~BI2CBus();

virtual char *				Name();

/* low level access to i2c bits */
virtual	void				I2CReset();
virtual	void				I2CStart();						
virtual	void				I2CStop();
virtual void				I2COne();
virtual void				I2CZero();
virtual	uchar				I2CAck();
virtual	uchar				I2CSendByte(uchar data, bigtime_t wait_for_ack);
virtual	uchar				I2CReadByte(bool last);

/* high level transaction access to i2c */						
virtual	void				I2CWrite1(uchar address, uchar data1);
virtual	void				I2CWrite2(uchar address, uchar data1, uchar data2);
virtual	uchar				I2CRead(uchar address);
virtual	char				I2CStatus();
virtual	bool				I2CDevicePresent(uchar address);

private:

virtual	void				_ReservedI2CBus1();
virtual	void				_ReservedI2CBus2();
virtual	void				_ReservedI2CBus3();

							BI2CBus(const BI2CBus &);
		BI2CBus				&operator=(const BI2CBus &);

		char				fName[32];
		uint32				_reserved[3];

};

#endif


