



#ifndef _DEFINES
#define _DEFINES



typedef unsigned char BYTE;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef short BOOL;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef long LONG;
typedef long RESPONSECODE;
typedef const char* LPCSTR;
typedef unsigned long SCARDCONTEXT;
typedef char* STR;
typedef unsigned long* PSCARDCONTEXT;
typedef unsigned long* LPSCARDCONTEXT;
typedef unsigned long SCARDHANDLE;
typedef unsigned long* PSCARDHANDLE;
typedef unsigned long* LPSCARDHANDLE;
typedef const void* LPCVOID;
typedef void* LPVOID;
typedef const unsigned char* LPCBYTE;
typedef unsigned long*  LPDWORD;
typedef char* LPSTR;
typedef char* LPCWSTR;
typedef unsigned char  WORD8;
typedef unsigned int WORD32;
typedef unsigned short WORD16;
typedef signed short INT16;

#define  G_OK  0 
#define BUFFER_SIZE 261
#define MAX_DATA 255
#define COMMAND_LEN 4 







 #define IFD_DEFAULT_MODE					0
#define IFD_WITHOUT_PTS_REQUEST					1
#define IFD_NEGOTIATE_PTS_OPTIMAL   				2
#define IFD_NEGOTIATE_PTS_MANUALLY				3

#define IFD_NEGOTIATE_PTS1					0x10
#define IFD_NEGOTIATE_PTS2					0x20
#define IFD_NEGOTIATE_PTS3					0x40

#define IFD_NEGOTIATE_T0					0x00
#define IFD_NEGOTIATE_T1					0x01

#define ICC_VCC_5V							0
#define ICC_VCC_3V							1
#define ICC_VCC_DEFAULT						ICC_VCC_5V


#define IFD_POWER_UP                    500
#define IFD_POWER_DOWN                  501
#define IFD_RESET                       502

#define GE_HI_LEN 						(-311)
#define GE_HI_CMD_LEN 						(-313)
#define GW_APDU_LE               				3
#define GE_APDU_LE               				(-512)
#define MAX_IFD_STRING 						100
#define ISOCARD 						0x02
#define FASTISOCARD 						0x12

#ifndef LOBYTE
#define LOBYTE(w)   ((BYTE)(w))
#endif
#ifndef HIBYTE
#define HIBYTE(w)  ((BYTE)(((WORD16)(w))>>8))
#endif




typedef struct apdu_comm
{
WORD8 Command[COMMAND_LEN];
WORD32 LengthIn;
WORD8  *DataIn;
WORD32	LengthExpected;
} APDU_COMMAND;

typedef struct apdu_resp
{
WORD32 LengthOut;
WORD8  *DataOut;
WORD32	Status;
} APDU_RESPONSE;

#endif


