/*	$Id: DInternalProxy.cpp,v 1.2 1999/03/05 14:25:14 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 11/30/98 16:06:48
*/

#include "bdb.h"
#include "DInternalProxy.h"
#include "DNub.h"
#include "DType.h"

status_t DInternalProxy::ReadData(ptr_t addr, void *buffer, size_t size)
{
	status_t result = B_OK;

	try
	{
		BAutolock lock(fNub);
		if (! lock.IsLocked()) THROW(("Error trying to lock nub"));
		
		fNub.ReadData(addr, buffer, size);
	}
	catch (HErr& e)
	{
		e.DoError();
		result = B_ERROR;
	}
	
	return result;
} /* DInternalProxy::ReadData */

status_t DInternalProxy::WriteData(ptr_t /*addr*/, void */*buffer*/, size_t /*size*/)
{
	status_t result = B_OK;

	try
	{
		BAutolock lock(fNub);
		if (! lock.IsLocked()) THROW(("Error trying to lock nub"));
		
		THROW(("Not implemented yet..."));
//		fNub.WriteData(addr, buffer, size);
	}
	catch (HErr& e)
	{
		e.DoError();
		result = B_ERROR;
	}
	
	return result;
} /* DInternalProxy::WriteData */
	
ptr_t DInternalProxy::VariableAddress()
{
	return fAddress;
} /* DInternalProxy::VariableAddress */

size_t DInternalProxy::VariableSize()
{
	return fType->Size();
} /* DInternalProxy::VariableSize */

bool DInternalProxy::IsPointer()
{
	return fType->IsPointer();
} /* DInternalProxy::IsPointer */

status_t DInternalProxy::Dereference()
{
	status_t result = B_OK;

	try
	{
		BAutolock lock(fNub);
		if (! lock.IsLocked()) THROW(("Error trying to lock nub"));
		
		fType = fType->Deref();
		fNub.Read(fAddress, fAddress);
	}
	catch (HErr& e)
	{
		e.DoError();
		result = B_ERROR;
	}
	
	return result;
} /* DInternalProxy::Dereference */

