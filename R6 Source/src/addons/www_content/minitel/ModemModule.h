/*************************************************************************
/
/	ModemModule.h
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#ifndef _MODEM_MODULE_H_
#define _MODEM_MODULE_H_

#include <SerialPort.h>
#include <stdio.h>

class	MinitelView;
class	Protocole;


#define kSTATUS_BUFFERING	 -10
#define kSTATUS_SUSPEND		  -1
#define kSTATUS_DISCONNECT	   0
#define kSTATUS_OPEN		   1
#define kSTATUS_DIALING		   2
#define kSTATUS_HANGING		   3
#define kSTATUS_CONNECT		  10
#define kSEARCH_NO			  -1
#define kSEARCH_FOUND		-999

#define kPPP_MAX_CFG_LEN	256


struct	modem_info
{
	void		PrintToStream()
					{
						printf("modem_info: 0x%x\n", (int)this);
						printf("      port: %s\n", port);
						printf("      init: %s\n", init);
						printf("    number: %s\n", number);
						printf("     speed: %d\n", (int)speed);
						printf("\n");
					};
	char		port[kPPP_MAX_CFG_LEN + 1];
	char		init[kPPP_MAX_CFG_LEN + 1];
	char		number[64];
	data_rate	speed;
};


//========================================================================

class ModemModule
{
	public:
							ModemModule			(Protocole*,
												 MinitelView*);
							~ModemModule		();
		void				CharFromModem		(unsigned char c);
		void				CharToModem			(unsigned char c);
		void				Connect				(const char* number = NULL);
		void				Disconnect			();
		BSerialPort*		TTY					();

		int32				fStatus;

private:
		void				FindModem			();
		void				ModemConfig			();
		void				OpenTty				();
		void				SendChar			(unsigned char c);
		int32				SendCmdAndWaitForAnswer	(char* cmd,
													 char* ans,
													 int32 timeout);
		void				SendString			(char* string);

		char*				fCmdDial;
		char*				fCmdEsc;
		char*				fCmdHup;
		char*				fMsgConnect;
		char*				fMsgNoCarrier;
		char*				fMsgOk;
		char*				fSearchString;
		int32				fConnectTimeout;
		int32				fSearchInd;
		modem_info			fModemInfo;
		BSerialPort*		fTTY;
		thread_id			fReadThread;
		MinitelView*		fAffiChageEcran;
		Protocole*			fProtocole;
};

status_t read_thread_loop(void* data);

#endif	/* _MODEM_MODULE_H_ */
