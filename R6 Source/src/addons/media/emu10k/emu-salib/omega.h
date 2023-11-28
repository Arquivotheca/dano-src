//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: omega.h
//
// Author: Michael Preston (current version)
//
// Description:
//    The error class to be included in other classes to keep track of
// their instance error-states.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Jul 19, 1996  Modified for new error code format.
//                                   Added error severity, propogation,
//                                   module identification
// MW                  Mar  3, 1995  Mod for new v2.0 datatype names
// RSCrawford          Jul  7, 1994  Initial creation.
//
//*****************************************************************************

#ifndef __OMEGA_H
#define __OMEGA_H

#include "datatype.h"
//#include "stringcl.h"
#include "win_mem.h"
#include "emuerrs.h"

#define NoModule (CHAR *)NULL

typedef enum {
   oscInfoStatus, oscWarningStatus, oscErrorStatus, oscFatalStatus
} OmegaStatusCode;

class OmegaErrorCode
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   CHAR *achModuleWhichCausedError;
   OmegaStatusCode oscStatus;
   BYTE byErrorCode;
};

class EMUEXPORT OmegaClass
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   OmegaClass(CHAR *achModuleNamearg=NoModule)
      {achModuleName = achModuleNamearg;
       ClearError();}

   BYTE GetError() const {return omega.byErrorCode;}
   void GetError(OmegaErrorCode& oec) const {oec = omega;}

   BOOL IsBad() const {return (omega.oscStatus > oscWarningStatus);}
   BOOL IsOK() const {return (omega.oscStatus < oscErrorStatus);}
   CHAR *GetModuleName() const {return achModuleName;}

   protected:

   void PropogateError(OmegaClass *child) {omega = child->omega;}
   void SetError(BYTE byErrorCode)
      {omega.achModuleWhichCausedError = NoModule;
       omega.oscStatus = ((byErrorCode == SUCCESS) ? oscInfoStatus :
						     oscErrorStatus);
       omega.byErrorCode = byErrorCode;}
   void SetError(OmegaStatusCode oscStatus,
		 BYTE byErrorCode)
      {omega.achModuleWhichCausedError = achModuleName;
       omega.oscStatus = oscStatus;
       omega.byErrorCode = byErrorCode;}
   void SetError(const OmegaErrorCode& oec) {omega = oec;}
   void ClearError()
      {omega.achModuleWhichCausedError = NoModule;
       omega.oscStatus = oscInfoStatus;
       omega.byErrorCode = SUCCESS;}

   private:

   OmegaErrorCode omega;
   CHAR *achModuleName;
};

#endif // __OMEGA_H
////////////////////////// End of OMEGA.H ///////////////////////////
