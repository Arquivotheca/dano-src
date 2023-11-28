/*************************************************************************
/
/	ModemModule.cpp
/
/	Modified by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <Alert.h>
#include <beia_settings.h>
#include <File.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <SerialPort.h>
#include <Window.h>

#include "MinitelView.h"
#include "ModemModule.h"
#include "Protocole.h"


/*
static data_rate speed_from_tag[] = {B_1200_BPS,
									 B_2400_BPS,
									 B_4800_BPS,
									 B_9600_BPS,
									 B_19200_BPS,
									 B_38400_BPS};
*/

const char*	kCurrVersion = "V1.10";
const int	kMaxStrLen = 512;


//========================================================================

status_t read_thread_loop(void* data)
// main read loop thread
{
	ModemModule*	modem_treatment = (ModemModule*)data;
	BSerialPort*	tty = modem_treatment->TTY();

//	tty->SetTimeout(100);
//	tty->SetNumBytes(0);
//	tty->SetNumBytes(1);
	while (1)
	{
		int32			n;
		unsigned char	c;

		n = tty->Read(&c, 1);
		if (n > 0)
			modem_treatment->CharFromModem(c);
	}
	return B_NO_ERROR;
}

//------------------------------------------------------------------------

static unsigned char evenp(unsigned char ch)
/* return even char matching a given 7 bits ascii character (from kermit) */
{
	unsigned char	b;
	register int32	a;

	b = ch & 0177;
	a = (b & 15) ^ ((b >> 4) & 15);
	a = (a & 3) ^ ((a >> 2) & 3);
	a = (a & 1) ^ ((a >> 1) & 1);
/* odd : a = 1 - a; */
	b = ch | (a << 7);
	return b;
}

//------------------------------------------------------------------------

static char* strsave(const char* string)
{
	char*	ptr;

	if (!string)
		return NULL;
	ptr = (char*)malloc(strlen(string) * sizeof(char) + 1);
	if (ptr != NULL)
		strcpy(ptr, string);
	return (ptr);
}


//========================================================================

ModemModule::ModemModule(Protocole* protocole, MinitelView* view)
	: fStatus			(kSTATUS_DISCONNECT),
	  fCmdDial			(NULL),
	  fCmdEsc			(NULL),
	  fCmdHup			(NULL),
	  fMsgConnect		(NULL),
	  fMsgNoCarrier		(NULL),
	  fMsgOk			(NULL),
	  fSearchString		(NULL),
	  fConnectTimeout	(10),
	  fSearchInd		(kSEARCH_NO),
	  fReadThread		(0),
	  fAffiChageEcran	(view),
	  fProtocole		(protocole)
{
	fTTY = new BSerialPort();
	ModemConfig();
}

//------------------------------------------------------------------------

ModemModule::~ModemModule()
{
	Disconnect();
	delete fTTY;
	free(fCmdDial);
	free(fCmdHup);
	free(fCmdEsc);
	free(fMsgOk);
	free(fMsgConnect);
	free(fMsgNoCarrier);
}

//------------------------------------------------------------------------

void ModemModule::CharFromModem(unsigned char c)
// submitted by read thread - must send it to Protocole for treatment
{
	c &= 0x7f;

/*
	if (fStatus == kSTATUS_DIALING)
		printf("ModemModule::CharFromModem(): 0x%x ('%c')\n", c, c);
*/

	if ((c) && (fSearchString) && (fSearchInd >= 0))
	{
		if (fSearchString[fSearchInd] == c)
		{
			fSearchInd++;
			if (fSearchString[fSearchInd] == '\0')
				fSearchInd = kSEARCH_FOUND;
		}
		else
			fSearchInd = 0;
	}
	if ((fStatus == kSTATUS_CONNECT) || (fStatus == kSTATUS_OPEN))
	{
		if (fSearchInd == kSEARCH_FOUND)
			fAffiChageEcran->Window()->PostMessage(kDISCONNECT, fAffiChageEcran);
		else
			fProtocole->CharFromModem(c);
	}
}

//------------------------------------------------------------------------

void ModemModule::CharToModem(unsigned char c)
// have to send a char (from Protocole) to modem
{
	if ((fStatus == kSTATUS_CONNECT) || (fStatus == kSTATUS_OPEN))
	{
		unsigned char	cc = evenp(c);
		fTTY->Write(&cc, 1);
	}
/*	
	else
		fprintf(stderr, "ModemModule::CharToModem(): error (status=%d)\n", fStatus);
*/
}

//------------------------------------------------------------------------

void ModemModule::Connect(const char* number)
{
	if (number)
	{
		strncpy(fModemInfo.number, number, sizeof(fModemInfo.number));
		fModemInfo.number[sizeof(fModemInfo.number) - 1] = 0;
	}
//	fModemInfo.PrintToStream();

	if (fStatus == kSTATUS_DISCONNECT)
	{
		OpenTty();
		if (fStatus == kSTATUS_OPEN)
		{
			fStatus = kSTATUS_DIALING;
			if (SendCmdAndWaitForAnswer(fModemInfo.init, fMsgOk, 2))
			{
				//printf("ModemModule::Compose(): init failed\n");
				//Modem is not responding or bad initialization string
				(new BAlert("", "Vérifiez la connexion de votre ligne téléphonique et essayez de vous connecter de nouveau.",
						"Continuez", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
				Disconnect();
			}
			else
			{
				char	buffer[1024];

				sprintf(buffer, "%s%s", fCmdDial, fModemInfo.number);
				if (SendCmdAndWaitForAnswer(buffer, fMsgConnect, fConnectTimeout))
				{
					//printf("ModemModule::Compose(): dial failed\n");
					Disconnect();
				}
				else
				{
					fSearchString = fMsgNoCarrier;
					fSearchInd = 0;
					fStatus = kSTATUS_CONNECT;
				}
			}
		}
	}
/*
	else
		printf("ModemModule::Compose(): error (status=%d)\n", (int)fStatus);
*/
}

//------------------------------------------------------------------------

void ModemModule::Disconnect()
// must kill read thread and close serial port
{
	if ((fStatus != kSTATUS_DISCONNECT) || (fReadThread))
	{
		if (fStatus == kSTATUS_CONNECT)
		{
			fStatus = kSTATUS_HANGING;
			snooze(1500000);
			SendString(fCmdEsc);
			snooze(1500000);
			SendString(fCmdHup);
			SendChar('\r');
		}
		fSearchInd = kSEARCH_NO;
		fSearchString = NULL;
		if (fReadThread > 0)
		{
			kill_thread(fReadThread);
			fReadThread = 0;
		}
		fTTY->Close();
		fStatus = kSTATUS_DISCONNECT;
		fProtocole->StatusConnect(0);
	}
/*
	else
		fprintf(stderr, "ModemModule::Disconnect(): error (status=%d)\n", fStatus);
*/
}

//------------------------------------------------------------------------

BSerialPort* ModemModule::TTY()
// return opened serial port - NULL if not connected
{
	if (fStatus > kSTATUS_DISCONNECT)
		return fTTY;
	else
		return NULL;
}


//========================================================================

void ModemModule::FindModem()
{
	get_setting("ppp", "serialport", fModemInfo.port, kPPP_MAX_CFG_LEN);
}

//------------------------------------------------------------------------

void ModemModule::ModemConfig()
{
	/* RMP - default settings */
	strcpy(fModemInfo.port, "serial1");
	strcpy(fModemInfo.init, "atz");
	strcpy(fModemInfo.number, "03611");
	fModemInfo.speed = B_1200_BPS;

	fConnectTimeout = 30;

	free(fCmdDial);
	fCmdDial = strsave("ATDT");
	free(fCmdHup);
	fCmdHup = strsave("ATH");
	free(fCmdEsc);
	fCmdEsc = strsave("+++");
	free(fMsgOk);
	fMsgOk = strsave("OK");
	free(fMsgConnect);
	fMsgConnect = strsave("CONNECT");
	free(fMsgNoCarrier);
	fMsgNoCarrier = strsave("NO CARRIER");

	FindModem();
}

//------------------------------------------------------------------------

void ModemModule::OpenTty()
// open serial port - create read thread
{
	status_t	result;

	if (fStatus == kSTATUS_DISCONNECT)
	{
		if ((result = fTTY->Open(fModemInfo.port)) > 0)
		{
			fTTY->ClearInput();
			fTTY->ClearOutput();
			fStatus = kSTATUS_OPEN;
			fReadThread = spawn_thread(read_thread_loop, "TTY$read", B_NORMAL_PRIORITY, (void *)this);
			if (fReadThread > 0)
			{
				resume_thread(fReadThread);
				fProtocole->StatusConnect(1);
				fTTY->SetDataBits(B_DATA_BITS_8);
				fTTY->SetStopBits(B_STOP_BITS_1);
				fTTY->SetParityMode(B_NO_PARITY);
//				fTTY->SetDataRate(speed_from_tag[fModemInfo.speed]);
//				fTTY->SetDTR(TRUE);
				fTTY->SetTimeout(100000);
				fTTY->SetFlowControl(B_HARDWARE_CONTROL);
			}
			else
			{
				fStatus = kSTATUS_DISCONNECT;
				fTTY->ClearInput();
				fTTY->ClearOutput();
				fTTY->Close();
/*
				fprintf(stderr, "ModemModule::OpenTty(): thread spawn failed\n");
*/
			}
		}
		else
		{
			char*	err;

			err = (char*)malloc(strlen(strerror(result)) + 256);
			sprintf(err, "Erreur: Modem %s: %s", fModemInfo.port, strerror(result));
			(new BAlert("", err, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			free(err);
//			fprintf(stderr, "ModemModule::OpenTty(): open failed\n");
		}
	}
/*
	else
		printf("ModemModule::OpenTty(): error (status=%d)\n", (int)fStatus);
*/
}

//------------------------------------------------------------------------

void ModemModule::SendChar(unsigned char c)
// send a char to the tty translating it to even parity
{
	if (fStatus != kSTATUS_DISCONNECT)
	{
		unsigned char	cc = evenp(c);
		fTTY->Write(&cc, 1);
	}
}

//------------------------------------------------------------------------

int32 ModemModule::SendCmdAndWaitForAnswer(char* cmd, char* ans, int32 timeout)
// 
{
	if ((cmd) && (*cmd != '\0'))
	{
		//SendString(cmd);
		//SendChar('\r');
		fTTY->Write(cmd, strlen(cmd));
		fTTY->Write("\r", 1);
	}
	
	if ((ans) && (*ans != '\0'))
	{
		bool		found = FALSE;
		bigtime_t	timemax = real_time_clock() + timeout;

		fSearchString = ans;
		fSearchInd = 0;
		while (timemax >= real_time_clock())
		{
			snooze(1000);
			if (fSearchInd == kSEARCH_FOUND)
			{
				found = TRUE;
				break;
			}
		}
		fSearchInd = kSEARCH_NO;
		if (found)
			return 0;
		else
			return 1;
	}
	else
		return 0;
}

//------------------------------------------------------------------------

void ModemModule::SendString(char* string)
// send string to the tty translating it to even parity
{
	if ((fStatus != kSTATUS_DISCONNECT) && (string))
	{
		unsigned char	buffer[1024];
		int32			len = strlen(string);

		if (len)
		{
			if (len > 1024)
				len = 1024;
			for (int32 i = 0; i < len; i++)
			{
				buffer[i] = evenp(string[i]);
			}
			fTTY->Write(buffer, len);
		}
	}
}
