/*	$Id: DInternalProxy.h,v 1.3 1999/03/05 14:25:14 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 11/30/98 16:04:31
*/

#ifndef DINTERNALPROXY_H
#define DINTERNALPROXY_H

#include "DProxy.h"
#include "DType.h"

class DNub;

class DInternalProxy : public DProxy
{
	DNub& fNub;
	const DType *fType;
	ptr_t fAddress;
	
  public:
	DInternalProxy(DNub& nub, const DType *type, ptr_t addr)
		: fNub(nub), fType(type), fAddress(addr) {}
	virtual ~DInternalProxy() {}
	
	virtual status_t ReadData(ptr_t addr, void *buffer, size_t size);
	virtual status_t WriteData(ptr_t addr, void *buffer, size_t size);
	
	virtual ptr_t VariableAddress();
	virtual size_t VariableSize();
	
	virtual bool IsPointer();
	virtual status_t Dereference();
};

#endif
