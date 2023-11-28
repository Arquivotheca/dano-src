//////////////////////////////////////////////////////////////////////////////
// Initialization Code
//
// FILE : r128_accel_init.c
// DESC : 
//    This file contains all initialization functions required for the
// supported hardware.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Includes //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include <common_includes.h>
#include <accel_includes.h>
#include <registersR128.h>


//////////////////////////////////////////////////////////////////////////////
// Local Prototypes //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int32 Scratch_test(void);
int32 Clear_registers(void);



//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Initialize Card
//    This initializes the graphics device belonging to the active instance
// of the accelerant.

status_t Init_Card(void)
{
	int32 retval = B_OK;
  // Tattletale.
  	ddprintf(("[R128 GFX]  Init_Card() called.\n"));
  	  	
  	// test a register to verify card installed
	retval = Scratch_test();
//#define ATI_ROM 0xc8000
//	if(!retval)	// scan bios rom
//	{	retval = Bios_scan((unsigned char *)ATI_ROM);
//	}
	



  ddprintf(("[R128 GFX]  Init_Card() returning:%08x\n",retval));
  return retval;
}


/////////////////////////////////////////////////////////
// NAME : Scratch_test()
// DESC : read & write port test on register to confirm
//		that a rage 128 board is on the PCI bus
// todo - confirm register i/o, returned values
/////////////////////////////////////////////////////////
int32 Scratch_test(void)
{
// 3. Scratch register test
//		1. read regitster BIOS_0_SCRATCH
//		2. test all bits
//		3. restore value of scratch
#define LONG_TEST_1 0xAAAAAAAA
#define LONG_TEST_2 0x55555555
	int32 Ltemp;
	int32 retval = B_ERROR;
	int32 Lcompare;

  	ddprintf(("[R128 GFX]  Scratch_test() called.\n"));

	READ_REG(BIOS_0_SCRATCH, Ltemp);		// save scratch,
  	ddprintf(("[R128 GFX]  Scratch_test() read.\n"));

	WRITE_REG(BIOS_0_SCRATCH, LONG_TEST_1);	// write
  	ddprintf(("[R128 GFX]  Scratch_test() write.\n"));
	READ_REG(BIOS_0_SCRATCH, Lcompare);		
	if (Lcompare == LONG_TEST_1)			// test, if okay
	{	WRITE_REG(BIOS_0_SCRATCH, LONG_TEST_2);
		READ_REG(BIOS_0_SCRATCH, Lcompare);	// test compilment
		if (Lcompare == LONG_TEST_2)
		{	retval = B_OK;
		}
	}
	WRITE_REG(BIOS_0_SCRATCH,Ltemp);
    ddprintf(("[R128 GFX]  Scratch test() = %08x\n",retval));

	return retval;							// return success
}	


//////////////////////////////////////////////////////////////////////////////
// Get Card Name, Chipset, and Serial Number.
//    This function returns the name, chipset name, and serial number for the
// card controlled by the current instance of the accelerant. These must
// include a terminating null character and must be no longer than the length
// specified (including the terminating null character).

void GetCardNameData(char *name, int name_size, char *chipset,
  int chipset_size, char *serial_number, int serial_size)
{
  strncpy(name, "ATI Rage 128", name_size - 1);
  name[name_size - 1] = 0;

  // HACK - We should be reporting the exact chip variant here, based on the
  // device ID number.
  strncpy(chipset, "ATI Rage 128", chipset_size - 1);
  chipset[chipset_size - 1] = 0;

  // HACK - We should extract the actual serial number here at some point.
  strncpy(serial_number, "unknown", serial_size - 1);
  serial_number[serial_size - 1] = 0;
}


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
