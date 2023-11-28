
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1996. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* Filename: emufilio.h
*
* Authors: Mike Guzewicz
*
* Description:  Generic File I/O routines
*
* History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  ---------------------------------------
*  0.001       MG        Mar 13, 96   File Created
*
******************************************************************************
*/

#ifndef __EMUFILIO_H
#define __EMUFILIO_H

#include "datatype.h"
#include "omega.h"
#include "win_mem.h"

enum enType
{
  enFReadBinary,
  enFReadASCII,
  enFWriteBinary,
  enFWriteASCII,
  enFAppendBinary,
  enFAppendASCII,

  enFTypeEnd
};

enum enWhence
{
  enFSeekSet,
  enFSeekCur,
  enFSeekEnd
};

class EmuAbstractFileIO : public OmegaClass
{
  public:

  // Real default constructor to pass module name into Omega
  EmuAbstractFileIO(CHAR *moduleName) : OmegaClass(moduleName) {}

  // Virtual destructor causes derived class destructor to be
  // fired upon 'delete'-ing a pointer to the virtual base class,
  // rather than requiring one "knows" what type of derived class
  // they have before 'delete'-ing it.
  virtual ~EmuAbstractFileIO(void) {}

  void *operator new(size_t size) {return NewAlloc(size);}
  void operator delete(void *ptr) {DeleteAlloc(ptr);}

  virtual EMUSTAT Open(void * parameter, enType type) = 0;
  virtual void    Close(void) = 0;
  virtual LONG    Read(void * buffer, LONG size, LONG num) = 0;
  virtual LONG    Write(const void * buffer, LONG size, LONG num) = 0;
  virtual LONG    Seek(LONG offset, enWhence whence) = 0;
  virtual LONG    Tell(BOOL absolute) = 0;
  virtual CHAR*   GetPathName()=0;
};

#endif
