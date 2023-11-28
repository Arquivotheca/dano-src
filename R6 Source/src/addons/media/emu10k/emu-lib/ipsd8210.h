/* ipsd8010.h 
 *
 * This is the preliminary version of the system dependent Interrupt Pending
 * manager.  This interface will definitely change.  This is only here so
 * the system independent Interrupt Pending manager (ip8210) will compile.
*/

/* Include files */
#include "datatype.h"
#include "emuerrs.h"

/* Typedefs */
typedef DWORD IPCBHANDLE;
typedef DWORD IPSVCHANDLE;
typedef BOOL  (*fIPService)(IPSVCHANDLE serviceID, IPCBHANDLE *);
typedef void (*fIPCallback)(IPCBHANDLE callbackID);

BEGINEMUCTYPE
BOOL ipsdSetupInterrupt(DWORD interruptID, IPSVCHANDLE serviceID,
                        fIPService srvc, fIPCallback cb);

//////////////////////////////////////////////////////
//
// not used?
EMUSTAT ipsdResetInterrupt(DWORD interruptID);
// not used?
EMUSTAT ipsdScheduleCallback(IPCBHANDLE callbackID);
// not used?
IPCBHANDLE ipsdServiceInterrupts();
// not used?
void ipsdServiceCallback(IPCBHANDLE callbackID);

ENDEMUCTYPE
