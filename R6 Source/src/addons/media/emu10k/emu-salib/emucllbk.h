#ifndef __EMUCLLBK_H
#define __EMUCLLBK_H
     
#include "datatype.h"

typedef enum CallBackFunctionType
{
  emuCallBackDownloading = 0,
  emuCallBackCompacting,
  emuCallBackReading
} CallBackFunction;

// Problem: Code Warrior does not like pascal keyword in the parenthesis
//          Visual C++ only likes pascal keyword in the parenthesis
//          Issue is the usage of the 'pascal' keyword. Recommend we
//          kill it.
typedef BOOL (*downLoadCallBack)(CallBackFunction theFunc, BYTE percentDone);

#endif
