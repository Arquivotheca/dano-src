/////// gserial.c 
//// Made by Atul Pandit


#define G_NAME     "Serial"
#define G_RELEASE  "4.31.002"//changed from "4.31.001"-madhu

#define G_UNIX
// -------------------------------------------------
// Pragma section
//  - comment is called if _MSC_VER is defined.
// -------------------------------------------------*/
#ifdef _MSC_VER
#pragma comment(exestr,"Gemplus(c) "G_NAME" Ver "G_RELEASE" "__DATE__)
#endif
/// -------------------------------------------------------
// Compatibility section
// - __MSC is define for compatibily with MSC under Borland 
// environment.
// -----------------------------------------------------------
#define __MSC

#include <OS.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <limits.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>

#include "gemplus.h"
#include "gemcom.h"

#include "IFD_Handler_private.h"


#define DTR_CONTROL_ENABLE                1
#define DTR_CONTROL_DISABLE               0
#define RTS_CONTROL_ENABLE                1
#define RTS_CONTROL_DISABLE               0

#define COM_C_system_error		-1

#ifdef _SUN_SOURCE
#define _CBAUD          CBAUD
#endif


//----------------------------------------------
//   Module public interface.
//    - gtser.h
// ----------------------------------------------*/
#include "gtser.h"

/*-------------------------------------------------------------------
Constant section:
   - DEFAULT_EXCHANGE_TIMEOUT defines the default time-out for an
     exchange beetween the DLL and the W32GTSER server process (1200ms).
   - START_TIME_OUT defines the time out for the W32GTSER.EXE start (10 sec).
   - END_TIME_OUT defines the time out for the W32GTSER.EXE end (2 sec).
------------------------------------------------------------------------------*/
#define DEFAULT_EXCHANGE_TIMEOUT          1200
#define START_TIME_OUT                   10000
#define END_TIME_OUT                      2000
/*---------------------------------------------------------------------
Macro section:
 - CTRL_BAUD_RATE control the baud rate parameter. Today, under Windows 3.1, it
   is not possible to use a value greater than 57600 bauds.
 - WORD_LEN, PARITY and STOP retreive the configuration values to pass to
   Windows to configure the port.
----------------------------------------------------------------------*/

#define CTRL_BAUD_RATE(x) (((x) <= 57600u) ? (x) : 57600u)
#define WORD_LEN(x)       (BYTE)(((x) & 3) + 5)
#define PARITY(x)         (BYTE)(parity[((BYTE)(x) >> 3) & 3])
#define STOP(x)           (BYTE)(stop[((x) >> 2) & 1])


static WORD16
   parity[4]  = {NOPARITY,ODDPARITY,NOPARITY,EVENPARITY};
static WORD16
   stop[2]    = {ONESTOPBIT,TWOSTOPBITS};


static int port_fd = -1;

static INT16 g_Counter = 0;
static INT16 g_Error = 0;
static INT32 g_TimeOut = 0;
static INT32 g_TxSize = 0, g_RxSize = 0;
static WORD16 g_InitRts = 0;
static WORD16 g_InitDtr = 0;

struct termios save_termios; 

static INT32 g_iNbByteRead = 0;

FILE *fw = NULL;

// MAX_INPUT is defined as 255
static BYTE g_szReadBuffer[500];

#ifndef put_msg
void put_msg(char *fmt, ...)
{
	char buf[200];
	va_list argptr;
	fw = fopen("gemgcr2.log", "a");	
	memset(buf, 0, sizeof(buf));
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr);
	va_end(argptr);
	if (fw != NULL)
	{
		fprintf(fw, "%s", buf);	
		fflush(fw);
	}	
	fclose(fw);
}
#endif



INT16 G_DECL G_SerPortOpen(const TGTSER_PORT G_FAR *Param)
{
	D(unsigned long t;)
	int channelNumber;
	int flags;
	char com_name[64];
	struct termios current_termios; 
	int x;

	if (port_fd >= 0)
		return GE_HOST_PORT_OPEN;

	channelNumber = Param->Port & 0x00FF;
	flags = (Param->Port >> 8) & 0xFF;

	// Build the device name
	sprintf(com_name, "/dev/ports/serial%u", channelNumber);

	// flag
	if (flags & 0x1)
		flags = O_EXCL;

	D(bug("Opening port 0x%08lX on device \"%s\"\n", Param->Port, com_name));
	D(t = system_time();)
	port_fd = open(com_name, flags | O_RDWR | O_NONBLOCK | O_NOCTTY);
	D(printf("open() takes %lu us to complete\n", system_time()-t);)

	if (port_fd >= 0)
	{
		struct termios t;
		fcntl( port_fd, F_SETFL, fcntl(port_fd, F_GETFL) & ~O_NONBLOCK);
		ioctl( port_fd, TCGETA, &t);
		t.c_cflag |= CLOCAL;
		ioctl( port_fd, TCSETA, &t);
	}
	D(bug("handle = %d\n", port_fd));


	if (port_fd == COM_C_system_error)
	{
		D(bug("G_SerPortOpen() error (%s)\n", errno));
		return GE_HOST_PORT_INIT;
	}

	if (tcgetattr(port_fd, &current_termios) == COM_C_system_error)
	{
		D(bug("tcgetattr() error (%s)\n", strerror(errno)));
		close(port_fd);
		port_fd = -1;
		return GE_HOST_PORT_INIT;
	}

	save_termios = current_termios;

	current_termios.c_iflag = 0; 
	current_termios.c_oflag = 0; // Raw output modes
	current_termios.c_cflag = 0; // Raw output modes

	// Do not echo characters because if you connect 
	// to a host it or your
	// modem will echo characters for you. 
	// Don't generate signals.
	current_termios.c_lflag = 0;

	// control flow - enable receiver - ignore modem 
	// control lines
	current_termios.c_cflag = CREAD | CLOCAL;

 	x = CTRL_BAUD_RATE(Param->BaudRate);
	D(bug("Baud Rate = %d\n", x));

	switch((int)CTRL_BAUD_RATE(Param->BaudRate))
	{
		case 50    : current_termios.c_cflag  |= B50; break;
		case 75    : current_termios.c_cflag  |= B75; break;
		case 110   : current_termios.c_cflag  |= B110; break;
		case 134   : current_termios.c_cflag  |= B134; break;
		case 150   : current_termios.c_cflag  |= B150; break;
		case 200   : current_termios.c_cflag  |= B200; break;
		case 300   : current_termios.c_cflag  |= B300; break;
		case 600   : current_termios.c_cflag  |= B600; break;
		case 1200  : current_termios.c_cflag  |= B1200; break;
		case 1800  : current_termios.c_cflag  |= B1800; break;
		case 2400  : current_termios.c_cflag  |= B2400; break;
		case 4800  : current_termios.c_cflag  |= B4800; break;
		case 9600  : current_termios.c_cflag  |= B9600; break;
		case 19200 : current_termios.c_cflag  |= B19200; break;
		case 38400 : current_termios.c_cflag  |= B38400; break;
		default    : current_termios.c_cflag  |= B9600; break;
	}

	x = WORD_LEN(Param->Mode);
	D(bug("Word Len = %d\n", x));

	switch((int)WORD_LEN(Param->Mode))// number of databits
	{
		case 7 : current_termios.c_cflag |= CS7; break;
		case 8 : current_termios.c_cflag |= CS8; break;
		default : current_termios.c_cflag |= CS8; break;
	}

	x = PARITY(Param->Mode);
	D(bug("Parity = %d\n", x));

	switch ((int)PARITY(Param->Mode))
	{
		case ODDPARITY : current_termios.c_cflag |= PARENB + PARODD;	break;
		case EVENPARITY : current_termios.c_cflag |= PARENB;			break;
	}

	x = STOP(Param->Mode);
	D(bug("Stop = %d\n", x));

	if (STOP(Param->Mode) == TWOSTOPBITS)  // stop bits 
	{
		current_termios.c_cflag |= CSTOPB;
	}
	
	// control caracteres 
	// This is Non Canonical mode set.
	// by setting c_cc[VMIN] = 0 and c_cc[VTIME] = 10
	// each read operation will wait for 10/10 = 1sec
	// If single byte is received, read function returns.
	// if timer expires, read() returns 0.

	current_termios.c_cc[VMIN]  = 0;	// Minimum bytes to read 
	current_termios.c_cc[VTIME] = 10;	// Time between two bytes read (VTIME x 0.10 s) 

	if (tcsetattr(port_fd, TCSANOW, &current_termios) == COM_C_system_error)
    {
		D(bug("tcgetattr() error (%s)\n", strerror(errno)));
		close(port_fd);
		port_fd = -1;
		return GE_HOST_PORT_INIT;
	}

	g_Counter = 1;
	g_Error   = 0;
	g_TimeOut = Param->TimeOut;
	g_TxSize  = Param->TxSize;
	g_RxSize  = Param->RxSize;

	return G_OK;
}


INT16 G_DECL G_SerPortClose(const INT16 dum_Handle)
{
	if (port_fd < 0)
		return GE_HOST_PORT_CLOSE;

	tcflush(port_fd, TCIOFLUSH);

	tcsetattr(port_fd, TCSANOW, &save_termios);

	close(port_fd);
	port_fd = -1;
	return G_OK;
}


INT16 G_DECL G_SerPortWrite(const INT16 dum_Handle,
							const WORD16       Length,
							const BYTE   G_FAR Buffer[])
{
	INT32 iWriteStat; // set to TRUE if the WriteFile function succeded
	WORD16 length = Length;
	struct tms tm;

	if (port_fd < 0)
		return GE_HOST_PORT_CLOSE;

	iWriteStat = write(port_fd, (BYTE G_FAR *)Buffer, length);

	if ((WORD16) iWriteStat != length)
		return GE_HOST_PORT_BREAK;

	tcdrain(port_fd); // function here..

	return G_OK;
}



INT16 G_DECL G_SerPortRead( const INT16 dum_Handle,
						    WORD16 G_FAR *Length,
						    BYTE   G_FAR  Buffer[])
{
	INT32 iRetour;
	INT32 timeout;
	WORD16 length = 0;
	WORD16 rlength = 0;

	if (port_fd < 0)
		return GE_HOST_PORT_CLOSE;

	rlength = *Length;
	if (g_iNbByteRead > 0)
	{
		if (g_iNbByteRead > rlength)	length = rlength;
		else							length = g_iNbByteRead;

		memcpy(Buffer, g_szReadBuffer, length);
		g_iNbByteRead -= length;

		if (g_iNbByteRead > 0)
			memcpy(g_szReadBuffer, g_szReadBuffer+length, g_iNbByteRead);
		rlength -= length;
	}

	timeout = g_TimeOut;
	while ((rlength > 0) && (timeout > 0))
	{
		iRetour = read(port_fd, (BYTE G_FAR *)Buffer+length, rlength);
		if (iRetour == COM_C_system_error)
		{
			D(bug("G_SerPortRead() error (%s)\n", strerror(errno)));
			return GE_HOST_PARAMETERS;
		}
		else if (iRetour > 0)
		{
			rlength -= (WORD16)iRetour;
			length  += (WORD16)iRetour;
		}
		else
		{
			iRetour = 0;
			wait_ms(100);
			timeout -= 100;
		}
	}
	*Length = length;
	return G_OK;	
}



INT16 G_DECL G_SerPortFlush
(
    const INT16 dum_Handle,
   const WORD16 Select
)
{
INT16
   response;
WORD16 select = Select;


	if ( port_fd < 0 )
	{
		return GE_HOST_PORT_CLOSE;
	}


	if (select & HGTSER_TX_QUEUE)
	{
		tcflush(port_fd, TCOFLUSH);
	}
//-------------------------------------------------------
//   If HGTSER_RX_QUEUE is selected
//   Then
//      Flushes the Rx queue by calling FlushComm.
//      Reset port_config.error field.
//-------------------------------------------------------
	if (select & HGTSER_RX_QUEUE)
	{
		tcflush(port_fd, TCIFLUSH);

		g_Error = 0;
		g_iNbByteRead = 0;
	}
	return G_OK;
}

INT16 G_DECL G_SerPortStatus
(
   const INT16  dum_Handle,
         WORD16 G_FAR *TxLength,
         WORD16  G_FAR *RxLength,
         TGTSER_STATUS G_FAR *Status
)
{
INT32         iReadStat;

	if ( port_fd < 0 )
	{
		return GE_HOST_PORT_CLOSE;
	}

	iReadStat = read(port_fd, &g_szReadBuffer[g_iNbByteRead], MAX_INPUT);

	if (iReadStat == COM_C_system_error)
	{
		return GE_HOST_PARAMETERS;
	}
	else
	{
		g_iNbByteRead += iReadStat;
		*RxLength = (WORD16) iReadStat;
		*TxLength = 0;
	}

	g_Error = 0;
	return G_OK;
}

INT16 G_DECL G_SerPortGetState
(
   TGTSER_PORT G_FAR *Param,
   WORD16      G_FAR *UserNb
)
{
struct termios current_termios;
TGTSER_PORT port_param;

	memset(&port_param, 0, sizeof(TGTSER_PORT));

	if ( port_fd < 0 )
	{
		return GE_HOST_PORT_CLOSE;
	}

	if ( tcgetattr(port_fd, &current_termios) == COM_C_system_error)
	{
		return GE_HOST_PORT_OS;
	}

#if (defined _DEC_SOURCE) || (defined _LINUX_SOURCE)
   port_param.BaudRate = cfgetispeed(&current_termios); /// CHECK THIS
#else
	switch (current_termios.c_cflag & CBAUD)
	{
		case B50    : port_param.BaudRate = 50; break;
		case B75    : port_param.BaudRate = 75; break;
		case B110   : port_param.BaudRate = 110; break;
		case B134   : port_param.BaudRate = 134; break;
		case B150   : port_param.BaudRate = 150; break;
		case B200   : port_param.BaudRate = 200; break;
		case B300   : port_param.BaudRate = 300; break;
		case B600   : port_param.BaudRate = 600; break;
		case B1200  : port_param.BaudRate = 1200; break;
		case B1800  : port_param.BaudRate = 1800; break;
		case B2400  : port_param.BaudRate = 2400; break;
		case B4800  : port_param.BaudRate = 4800; break;
		case B9600  : port_param.BaudRate = 9600; break;
		case B19200 : port_param.BaudRate = 19200; break;
		case B38400 : port_param.BaudRate = 38400; break;
		default :
		  return GE_HOST_PORT_OS;
	}
#endif

	switch (current_termios.c_cflag & CSIZE)
	{
		case CS7: port_param.Mode = HGTSER_WORD_7; break;
		case CS8: port_param.Mode = HGTSER_WORD_8; break;
		default:
			return GE_HOST_PORT_OS;
	}

	switch (current_termios.c_cflag & (PARENB + PARODD))
	{
		case 0: port_param.Mode |= HGTSER_NO_PARITY; break;
		case PARENB: port_param.Mode |= HGTSER_EVEN_PARITY; break;
		case PARENB+PARODD: port_param.Mode |= HGTSER_ODD_PARITY; break;
		default:
			return GE_HOST_PORT_OS;
	}

	switch (current_termios.c_cflag & CSTOPB)
	{
		case 0 : port_param.Mode |= HGTSER_STOP_BIT_1; break;
		case CSTOPB: port_param.Mode |= HGTSER_STOP_BIT_2; break;
		default:
			return GE_HOST_PORT_OS;
	}

	*UserNb = 1;

	memcpy(Param, &port_param, sizeof(TGTSER_PORT));

	return G_OK;
}



INT16 G_DECL G_SerPortSetState(TGTSER_PORT G_FAR *Param)
{
	struct termios current_termios;
	TGTSER_PORT port_param;

	// copying to local variable..
	port_param = *Param;	
	port_param = *Param;

	if (port_fd < 0)
		return GE_HOST_PORT_CLOSE;

	if (tcgetattr(port_fd, &current_termios) == COM_C_system_error)
		return GE_HOST_PORT_INIT;

	current_termios.c_iflag = 0;
	current_termios.c_oflag = 0;
	current_termios.c_cflag = 0;
	current_termios.c_lflag = 0;

	switch((int)CTRL_BAUD_RATE(port_param.BaudRate))
	{
		case 50 : current_termios.c_cflag  |= B50; break;
		case 75 : current_termios.c_cflag  |= B75; break;
		case 110 : current_termios.c_cflag  |= B110; break;
		case 134 : current_termios.c_cflag  |= B134; break;
		case 150 : current_termios.c_cflag  |= B150; break;
		case 200 : current_termios.c_cflag  |= B200; break;
		case 300 : current_termios.c_cflag  |= B300; break;
		case 600 : current_termios.c_cflag |= B600; break;
		case 1200 : current_termios.c_cflag  |= B1200; break;
		case 1800 : current_termios.c_cflag  |= B1800; break;
		case 2400 : current_termios.c_cflag  |= B2400; break;
		case 4800 : current_termios.c_cflag  |= B4800; break;
		case 9600 : current_termios.c_cflag  |= B9600; break;
		case 19200 : current_termios.c_cflag  |= B19200; break;
		case 38400 : current_termios.c_cflag  |= B38400; break;
		default : current_termios.c_cflag  |= B9600;
		break;
	}

	// no. of data bits..
	switch((int)WORD_LEN(port_param.Mode))
	{
		case 7 : current_termios.c_cflag |= CS7; break;
		case 8 : current_termios.c_cflag |= CS8; break;
		default : current_termios.c_cflag |= CS8; break;
	}

	// parity..
	switch((int)PARITY(port_param.Mode))
	{
		case ODDPARITY : current_termios.c_cflag |= PARENB + PARODD;	break;
		case EVENPARITY : current_termios.c_cflag |= PARENB;			break;
	}

	if (STOP(port_param.Mode) == TWOSTOPBITS)
		current_termios.c_cflag |= CSTOPB;

	//control modes 
	current_termios.c_cflag |= CREAD; // Enable receiver
	current_termios.c_cflag |= CLOCAL; // pas de gestion ligne modem

	// control caracteres 
	// Non Canonical mode
	current_termios.c_cc[VMIN]  = 0; // Minimum bytes to read
    current_termios.c_cc[VTIME] = 10; // Time between two bytes read (VTIME x 0.10 s) 

	if (tcsetattr(port_fd, TCSANOW, &current_termios) == COM_C_system_error)
		return GE_HOST_PARAMETERS;

	return G_OK;
}


INT16 G_DECL G_SerPortSetLineState
(
    const INT16 dum_Handle,
   const BYTE   Line,
   const INT32  Enable,
   const WORD32 Time
)
{
int iRetour;
int sStatLine;
WORD32 time = Time;
// struct pollfd filedes[1];			// MA
INT32 enable = Enable;
BYTE line = Line;

	if ( port_fd < 0 )
	{
		return GE_HOST_PORT_CLOSE;
	}

	
#ifdef _HPUX_SOURCE
   iRetour =  ioctl(port_fd, MCGETA, &sStatLine);
#else // _HPUX_SOURCE
   iRetour =  ioctl(port_fd, TIOCMGET, &sStatLine);
#endif // _HPUX_SOURCE

	if ( iRetour != COM_C_system_error )
	{
		return GE_HOST_PARAMETERS;
	}

      switch (line)
      {
         case HGTSER_DTR_LINE :
#ifdef _HPUX_SOURCE
            g_InitDtr = 
               ((sStatLine & MDTR) == 0 ? DTR_CONTROL_DISABLE : DTR_CONTROL_ENABLE);
            if (enable)
               sStatLine |= MDTR;
            else
               sStatLine &= ~MDTR;
#else // _HPUX_SOURCE
            g_InitDtr = 
               ((sStatLine & TIOCM_DTR) == 0 ? DTR_CONTROL_DISABLE : DTR_CONTROL_ENABLE);
            if (enable)
               sStatLine |= TIOCM_DTR;
            else
               sStatLine &= ~TIOCM_DTR;
#endif // _HPUX_SOURCE
            break;
         
         case HGTSER_RTS_LINE :
#ifdef _HPUX_SOURCE
            g_InitRts = 
               ((sStatLine & MRTS) == 0 ? RTS_CONTROL_DISABLE : RTS_CONTROL_ENABLE);
            if (enable)
               sStatLine |= MRTS;
            else
               sStatLine &= ~MRTS;
#else // _HPUX_SOURCE
            g_InitRts = 
               ((sStatLine & TIOCM_RTS) == 0 ? RTS_CONTROL_DISABLE : RTS_CONTROL_ENABLE);
            if (enable)
               sStatLine |= TIOCM_RTS;
            else
               sStatLine &= ~TIOCM_RTS;
#endif // _HPUX_SOURCE 
            break;

         default :
            return GE_HOST_PARAMETERS;
      }

#ifdef _HPUX_SOURCE
      iRetour = ioctl(port_fd, MCSETA, &sStatLine);
#else // _HPUX_SOURCE
//      iRetour = ioctl(port_fd, TIOCMSET, &sStatLine);

	{ // MA
	int rts = ((sStatLine & TIOCM_RTS) == 0) ? 0 : 1;
	int dtr = ((sStatLine & TIOCM_DTR) == 0) ? 0 : 1;
	iRetour = ioctl(port_fd, TCSETRTS, rts);
	iRetour |= ioctl(port_fd, TCSETDTR, dtr);
	}

#endif // _HPUX_SOURCE


	if ( (time > 0) && (iRetour != COM_C_system_error) )
	{
//		filedes[0].fd = port_fd; // previously it was 0.
//		filedes[0].events = POLLNVAL;
//		poll(filedes,1,time);
		
		wait_ms(time);		// MA

		switch (line)
		{
		case HGTSER_DTR_LINE :
#ifdef _HPUX_SOURCE
		   if (g_InitDtr != DTR_CONTROL_DISABLE)
			  sStatLine |= MDTR;
		   else
			  sStatLine &= ~MDTR;
#else // _HPUX_SOURCE 
		   if (g_InitDtr != DTR_CONTROL_DISABLE)
			  sStatLine |= TIOCM_DTR;
		   else
			  sStatLine &= ~TIOCM_DTR;
#endif // _HPUX_SOURCE 
		   break;
   
		case HGTSER_RTS_LINE :
#ifdef _HPUX_SOURCE
		   if (g_InitDtr != RTS_CONTROL_DISABLE)
			  sStatLine |= MRTS;
		   else
			  sStatLine &= ~MRTS;
#else // _HPUX_SOURCE 
		   if (g_InitDtr != RTS_CONTROL_DISABLE)
			  sStatLine |= TIOCM_RTS;
		   else
			  sStatLine &= ~TIOCM_RTS;
#endif // _HPUX_SOURCE 
		   break;
		}
#ifdef _HPUX_SOURCE
		iRetour = ioctl(port_fd,MCSETA,&sStatLine);
#else // _HPUX_SOURCE 
//		iRetour = ioctl(port_fd,TIOCMSET,&sStatLine);
	{ // MA
	int rts = ((sStatLine & TIOCM_RTS) == 0) ? 0 : 1;
	int dtr = ((sStatLine & TIOCM_DTR) == 0) ? 0 : 1;
	iRetour = ioctl(port_fd, TCSETRTS, rts);
	iRetour |= ioctl(port_fd, TCSETDTR, dtr);
	}
#endif // _HPUX_SOURCE
	}


	if (iRetour != COM_C_system_error)
	{
		return G_OK;
	}
	else
	{
		return GE_UNKNOWN_PB;
	}
}

INT16 G_DECL G_SerPortGetLineState
(
    const INT16 dum_Handle,
   const BYTE         Line,
	INT32 G_FAR *Enable
)
{
int
   iRetour;

#ifdef _HPUX_SOURCE
mflag
#else // _HPUX_SOURCE
int
#endif // _HPUX_SOURCE
   sStatLine;
INT32 enable=0;
BYTE line=Line;


	if ( port_fd < 0 )
	{
		return GE_HOST_PORT_CLOSE;
	}

	#ifdef _HPUX_SOURCE
		iRetour = ioctl(port_fd, MCGETA, &sStatLine);
	#else // _HPUX_SOURCE 
		iRetour = ioctl(port_fd, TIOCMGET, &sStatLine);
	#endif // _HPUX_SOURCE

	if ( iRetour == COM_C_system_error )
	{
		fprintf(stderr, "GetPortLineState: ioctl fails\n");
		return GE_HOST_PARAMETERS;
	}

	switch (line)
	{
		case HGTSER_DTR_LINE :
#ifdef _HPUX_SOURCE
			enable = ((sStatLine &= MDTR) == MDTR);
#else // _HPUX_SOURCE
		enable = ((sStatLine &= TIOCM_DTR) == TIOCM_DTR);
#endif // _HPUX_SOURCE
		break;
         
		case HGTSER_RTS_LINE :
#ifdef _HPUX_SOURCE
			enable = ((sStatLine &= MRTS) == MRTS);
#else // _HPUX_SOURCE
            enable = ((sStatLine &= TIOCM_RTS) == TIOCM_RTS);
#endif // _HPUX_SOURCE
            break;

		default :
            return GE_HOST_PARAMETERS;
	}

	*Enable = enable;
	return G_OK;
}

#ifdef _HPUX_SOURCE
static  mflag
#else
static  int
#endif
        g_com_status=0; // holds the status line of serial port



INT16 G_DECL G_SerPortSetEvent
(
    const INT16 dum_Handle,
   const WORD16 Event
)
{
int
	iRetour;
WORD16 event = Event;

	if ( port_fd < 0 )
	{
		return GE_HOST_PORT_CLOSE;
	}

	#ifdef _HPUX_SOURCE
		iRetour = ioctl(port_fd, MCGETA, &g_com_status);
	#else // _HPUX_SOURCE
		iRetour = ioctl(port_fd, TIOCMGET, &g_com_status);
	#endif // _HPUX_SOURCE

	if ( iRetour == COM_C_system_error )
	{
		fprintf(stderr, "SerPortSetEvent error in ioctl\n");
		return GE_HOST_PARAMETERS;
	}

	switch (event)
	{
		case HGTSER_RI_LINE :
#ifdef _HPUX_SOURCE
		g_com_status |= MRI;
#elif _LINUX_SOURCE
		g_com_status |= TIOCM_RI;
#endif // _HPUX_SOURCE 
		break;

	case HGTSER_DCD_LINE :
	case HGTSER_CTS_LINE :
	case HGTSER_DSR_LINE :
	default :
		break;
	}

	return G_OK;
}




INT16 G_DECL G_SerPortGetEvent
(
    const INT16 dum_Handle,
   const WORD16        Event,
	INT32  G_FAR *Found
)
{
int
	iRetour;
#ifdef _HPUX_SOURCE
mflag
#else // _HPUX_SOURCE
int
#endif // _HPUX_SOURCE
	sStatLine;

WORD16 event = Event;
INT32 found = *Found;


#ifdef _HPUX_SOURCE
	iRetour = ioctl(port_fd, MCGETA, &sStatLine);
#else // _HPUX_SOURCE
	iRetour = ioctl(port_fd, TIOCMGET, &sStatLine);
#endif // _HPUX_SOURCE

	if ( iRetour == COM_C_system_error )
	{
		fprintf(stderr, "G_SerPortGetEvent() ioctl failed\n");
		return GE_HOST_PARAMETERS;
	}
	switch (event)
	{
		case HGTSER_RI_LINE :
#ifdef _HPUX_SOURCE
			found = ((g_com_status & MRI) != (sStatLine & MRI));
#else // _HPUX_SOURCE
			found = ((g_com_status & TIOCM_RI) != (sStatLine & TIOCM_RI));
#endif // _HPUX_SOURCE
			break;

		case HGTSER_DCD_LINE :
#ifdef _HPUX_SOURCE
			found = ((g_com_status & MDCD) != (sStatLine & MDCD));
#else // _HPUX_SOURCE
			found = ((g_com_status & TIOCM_CD) != (sStatLine & TIOCM_CD));
#endif // _HPUX_SOURCE
			break;

		case HGTSER_CTS_LINE :
#ifdef _HPUX_SOURCE
			found = ((g_com_status & MCTS) != (sStatLine & MCTS));
#else // _HPUX_SOURCE
			found = ((g_com_status & TIOCM_CTS) != (sStatLine & TIOCM_CTS));
#endif // _HPUX_SOURCE 
			break;

		case HGTSER_DSR_LINE :
#ifdef _HPUX_SOURCE
			found = ((g_com_status & MDSR) != (sStatLine & MDSR));
#else // _HPUX_SOURCE
			found = ((g_com_status & TIOCM_DSR) != (sStatLine & TIOCM_DSR));
#endif // _HPUX_SOURCE
			break;

		default :
			return GE_HOST_PARAMETERS;
	}
	
	*Found = found;
	return G_OK;
}


INT16 G_DECL G_SerPortSetTimeouts
(
   const INT16         Handle,
   DWORD         		  BWT
)
{
	return G_OK;
}








	
