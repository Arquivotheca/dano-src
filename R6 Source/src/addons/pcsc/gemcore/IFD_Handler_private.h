/*****************************************************************
/
/ File   :   IFD_Handler.h
/ Author :   Atul Pandit
/
******************************************************************/

#ifndef _IFD_Handler_h_
#define _IFD_Handler_h_

#include <SmartCardAddOn.h>
#include "pcscdefines.h"
#include "defines.h"

#define M_NB_MAX_DEVICES 1

typedef struct
{
	char name[64];
	int flags;
} BEOS_DEVICE;

struct DEVICE_CAPABILITIES {
  
  char *Vendor_Name;          	// Tag 0x0100
  char *IFD_Type;			// Tag 0x0101
  DWORD IFD_Version;		// Tag 0x0102
  char *IFD_Serial;		// Tag 0x0103
  DWORD IFD_Channel_ID;  	// Tag 0x0110
  
				// Something missing from the Specification ??????
  
  DWORD Asynch_Supported;	// Tag 0x0120
  DWORD Default_Clock;		// Tag 0x0121
  DWORD Max_Clock;		// Tag 0x0122
  DWORD Default_Data_Rate;	// Tag 0x0123
  DWORD Max_Data_Rate;		// Tag 0x0124
  DWORD Max_IFSD;		// Tag 0x0125
  DWORD Synch_Supported;	// Tag 0x0126
  DWORD Power_Mgmt;		// Tag 0x0131
  DWORD Card_Auth_Devices;	// Tag 0x0140
  DWORD User_Auth_Device;	// Tag 0x0142
  DWORD Mechanics_Supported;	// Tag 0x0150
  DWORD Vendor_Features;	// Tag 0x0180 - 0x01F0   User Defined.
  
} Device;

struct ICC_STATE {

  BYTE ICC_Presence;		// Tag 0x0300
  BYTE ICC_Interface_Status;	// Tag 0x0301
  BYTE ATR[MAX_ATR_SIZE];	// Tag 0x0303
  BYTE ICC_Type;		// Tag 0x0304

  /* The following is an extension.  It is a temporary storage space
     for storing the Friendly Readername given by the application if
     it requires a temporary exclusive transaction                   */

  DWORD ICC_LOCK_ID;               // Tag 0x0350
  DWORD ICC_LOCK;                  // Tag 0x0351

  DWORD ProtocolType;
  BYTE SelectionFlags;
  BYTE PTS1;
  BYTE PTS2;
  BYTE PTS3;
  
} ICC;

struct PROTOCOL_OPTIONS {
  
  DWORD Protocol_Type;		// Tag 0x0201
  DWORD Current_Clock;		// Tag 0x0202
  DWORD Current_F;		// Tag 0x0203
  DWORD Current_D;		// Tag 0x0204
  DWORD Current_N;		// Tag 0x0205 //TC1(Guard Time)*
  DWORD Current_W;		// Tag 0x0206 //TC2(Work Waiting time)
  DWORD Current_IFSC;		// Tag 0x0207 //TA3(only T=1)
  DWORD Current_IFSD;		// Tag 0x0208 //0xFE
  DWORD Current_BWT;		// Tag 0x0209 //TB3(b5-b8)
  DWORD Current_CWT;		// Tag 0x020A //TB3(b1-b4)
  DWORD Current_EBC;		// Tag 0x020B //TC3
} Protocol;


// Reader Specific defines used in IFD_Handler.c file
// added by atul
#define IFD_OS_VERSION                  "GemCore-R1."


#endif

