// ===========================================================================
//	jseopt.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __JSEOPT_H
#define __JSEOPT_H

#define __JSE_BEOS__
#define __JSE_UNIX__
#define __JSE_LIB__
#define JSETOOLKIT_APP

#define JSE_C_EXTENSIONS 0
#define JSE_INCLUDE 0
#define JSE_CONDITIONAL_COMPILE 0
#define JSE_CREATEFUNCTIONTEXTVARIABLE 0
#define JSE_FAST_MEMPOOL 0
#define JSE_BROWSEROBJECTS 1
#define JSE_FULL_CYCLIC_CHECK 1
#define NDEBUG 1
#define JSETOOLKIT_APP_INCL_ECMABROWSEROBJECTS
#define JSETOOLKIT_APP_INCL_ECMABUILTINOBJECTS
#define JSETOOLKIT_APP_INCL_ECMABUFFER
#define JSETOOLKIT_APP_INCL_ECMAMATH
#define JSETOOLKIT_APP_INCL_ECMADATE

/*
#define JSE_DEBUGGABLE 1
#define JSE_DEBUG_TCPIP
#define JSE_DEBUG_MASTER
#define JSE_DEBUG_RUN
#define JSE_DEBUG_FILES
#define JSE_DEBUG_REMOTE
#define JSE_DEBUG_PASSWORD

#if defined(JSE_DEBUGGABLE)
  #if defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__)
    #include <sys\socket.h>
    #include <netinet\in.h>
    #include <netdb.h>
    #include <utils.h>
    #include <nerrno.h>
    #include <sys\ioctl.h>
  #endif
  #include "dbgshare.h"
  #include "proxy.h"
  #include "debugme.h"
#endif
*/


//#  include "jsetypes.h"
//#  include "jselib.h"
//#  include "seseclib.h"
//#  include "seuni.h"
//#  include "ecmalib.h"

#endif