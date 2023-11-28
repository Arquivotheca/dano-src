#define ERR_OPENED -10
#define ERR_IO_COMM -11
#define ERR_NOT_OPENED -12
#define ERR_INVALID_PARAM -13
#define STATUS_SUCCESS 0
#define RESPONSECODE long
#define DWORD long
RESPONSECODE OpenUSB( DWORD lun );
RESPONSECODE WriteUSB( DWORD lun, DWORD length, unsigned char *buffer );
RESPONSECODE ReadUSB( DWORD lun, DWORD *length, unsigned char *buffer );
RESPONSECODE CloseUSB( DWORD lun );
