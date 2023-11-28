/*
	Copyright 2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef DPROXY_H
#define DPROXY_H

#ifndef PTR_T_DEFINED
typedef unsigned long ptr_t;
#define PTR_T_DEFINED 1
#endif

class DProxy
{
	DProxy& operator= (const DProxy& proxy);
	DProxy(const DProxy&);
	
  protected:
	DProxy() 			{}
	virtual ~DProxy()	{}
	
  public:
	
	virtual status_t ReadData(ptr_t addr, void *buffer, size_t size) = 0;
	virtual status_t WriteData(ptr_t addr, void *buffer, size_t size) = 0;
	
	virtual ptr_t VariableAddress() = 0;
	virtual size_t VariableSize() = 0;
	
	virtual bool IsPointer() = 0;
	virtual status_t Dereference() = 0;
};

#endif
