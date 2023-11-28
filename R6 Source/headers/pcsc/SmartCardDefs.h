/******************************************************************************
/
/	File:			SmartCardDefs.h
/
/	Description:	PC/SC Defined constants.
/
/	Copyright 1993-2000, Be Incorporated
/
******************************************************************************/

#ifndef _PCSMARTCARDDEFS_H
#define _PCSMARTCARDDEFS_H

#include <AppDefs.h>
#include <Errors.h>
#include <String.h>
#include <SmartCardTypes.h>

#define B_PCSC_MAX_ATR_SIZE		33
#define B_PCSC_MAX_BUFFER_SIZE	256
#define INFINITE				(0xFFFFFFFFLU)

// Types
typedef uint8			atr_t[B_PCSC_MAX_ATR_SIZE];
typedef BString			STR;

typedef struct
{
	STR			Reader;			// Reader Name
	void		*UserData;		// User defined data
	DWORD		CurrentState;	// Current state of the reader at time of call
	DWORD		EventState;		// State of reader after state change
} SCARD_READERSTATE;

struct GUID_t
{
	uint8	guid[16];
};


template <class T> class pcsc_list;
typedef pcsc_list<STR>		STR_LIST;
typedef pcsc_list<GUID_t>	GUID_LIST;


// Defined Constants
enum
{
	SCARD_SHARE_SHARED		= 0x00000000,
	SCARD_SHARE_EXCLUSIVE	= 0x00000001,
	SCARD_SHARE_DIRECT		= 0x00010000
};

enum
{
	SCARD_PROTOCOL_T0			= 0x00000001,
	SCARD_PROTOCOL_T1			= 0x00000002,
	SCARD_PROTOCOL_RAW			= 0x00000004,
	SCARD_PROTOCOL_OPTIMAL		= 0x00000008,
	SCARD_PROTOCOL_DEFAULT		= 0x80000000,
	SCARD_PROTOCOL_UNDEFINED	= 0x00000000
};

enum
{
	SCARD_LEAVE_CARD,
	SCARD_RESET_CARD,
	SCARD_UNPOWER_CARD,
	SCARD_EJECT_CARD,
	SCARD_CONFISCATE_CARD,
	SCARD_POWER_DOWN_CARD = SCARD_UNPOWER_CARD
};

enum
{
	SCARD_UNKNOWN,
	SCARD_ABSENT,
	SCARD_PRESENT,
	SCARD_SWALLOWED,
	SCARD_POWERED,
	SCARD_NEGOTIABLE,
	SCARD_SPECIFIC
};

enum
{
	SCARD_SCOPE_USER,
	SCARD_SCOPE_TERMINAL,
	SCARD_SCOPE_SYSTEM
};

// Error codes
enum
{
	SCARD_S_SUCCESS				= B_OK,
	SCARD_E_ERROR				= B_ERROR,
	SCARD_E_INVALID_HANDLE		= B_BAD_INDEX,
	SCARD_E_INVALID_PARAMETER	= B_BAD_TYPE,
	SCARD_E_INVALID_VALUE		= B_BAD_VALUE,
	SCARD_E_CANCELLED			= B_CANCELED,
	SCARD_E_NO_MEMORY			= B_NO_MEMORY,
	SCARD_E_INSUFFICIENT_BUFFER = B_NO_MEMORY,
	SCARD_E_UNKNOWN_READER		= B_DEV_BAD_DRIVE_NUM,
	SCARD_E_TIMEOUT				= B_TIMED_OUT,
	SCARD_E_SHARING_VIOLATION	= B_PERMISSION_DENIED,
	SCARD_E_NO_SMARTCARD		= B_DEV_NO_MEDIA,
	SCARD_E_UNKNOWN_CARD		= B_DEV_FORMAT_ERROR,
	SCARD_E_NOT_READY			= B_DEV_NOT_READY,
	SCARD_E_READER_UNAVAILLABLE	= B_IO_ERROR,
	SCARD_E_SYSTEM_CANCELLED	= B_NO_REPLY,
	SCARD_E_PROTO_MISMATCH		= B_ERRORS_END+1,
	SCARD_E_NOT_TRANSACTED,

	SCARD_W_UNUPPORTED_CARD,
	SCARD_W_UNRESPONSIVE_CARD,
	SCARD_W_UNPOWERED_CARD,
	SCARD_W_RESET_CARD,
	SCARD_W_REMOVED_CARD
};

// Event State
enum
{
	SCARD_STATE_UNAWARE		= 0x00000000,
	SCARD_STATE_IGNORE		= 0x00000001,

	SCARD_STATE_UNAVAILABLE	= 0x00000100,
	SCARD_STATE_EMPTY		= 0x00000200,
	SCARD_STATE_PRESENT		= 0x00000400,
	SCARD_STATE_ATRMATCH	= 0x00000800 | SCARD_STATE_PRESENT,
	SCARD_STATE_EXCLUSIVE	= 0x00001000 | SCARD_STATE_PRESENT,
	SCARD_STATE_INUSE		= 0x00002000 | SCARD_STATE_PRESENT,

	SCARD_STATE_CHANGED		= 0x00010000,
	SCARD_STATE_UNKNOWN		= 0x00020000 | SCARD_STATE_CHANGED
};

#define SCARD_STATE_ALL		(SCARD_STATE_UNAVAILABLE | SCARD_STATE_EMPTY | SCARD_STATE_PRESENT | SCARD_STATE_ATRMATCH | SCARD_STATE_EXCLUSIVE | SCARD_STATE_INUSE)



template <class T>
class pcsc_list
{
public:
	pcsc_list();
	pcsc_list(uint32 n);
	pcsc_list(const pcsc_list<T>&);
	pcsc_list<T>& operator=(const pcsc_list<T>&);
	~pcsc_list();
	
	void AddItem(const T& item);
	void AddItem(const T& item, uint32 index);

	uint32 CountItems(void) const;
	bool IsEmpty(void) const;

	T& FirstItem(uint32 index) const;
	T& LastItem(uint32 index) const;
	T& ItemAt(uint32 index) const;
	T& operator [] (int index) const;
	
	void MakeEmpty(void);
	void RemoveItem(uint32 index);
	void RemoveItems(uint32 index, uint32 count);

private:
	void *fPrivateData;
};



#endif
