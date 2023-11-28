/* API settings */

#ifndef __ASET8210_H
#define __ASET8210_H

#include "baseconfig.h"

#ifdef __SYS_WINDOWS
#  ifndef VTOOLSD
#    ifndef __HRM8210_NO_DLL
#      define EMUAPIEXPORT  __declspec( dllexport )
#    else
#      define EMUAPIEXPORT
#    endif
#  else
#    define EMUAPIEXPORT
#  endif
#else
#  define EMUAPIEXPORT
#endif




#endif  /*__ASET8210_H */
