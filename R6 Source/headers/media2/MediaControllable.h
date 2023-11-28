/***************************************************************************
//
//	File:			media2/MediaControllable.h
//
//	Description:	Exposes controllable parameters for a given
//					media2 object (node, endpoint, or collective.)
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _MEDIA2_MEDIACONTROLLABLE_H_
#define _MEDIA2_MEDIACONTROLLABLE_H_

#include <support2/Binder.h>
#include <support2/Locker.h>

#include <media2/IMediaControllable.h>

namespace B {
namespace Media2 {

using namespace Support2;

class LMediaControllable : public LInterface<IMediaControllable>
{
public:
	virtual	status_t				Called(BValue & in, const BValue &outBindings, BValue &out);
};

class BMediaControllable : public LMediaControllable
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BMediaControllable)

									BMediaControllable();

	// ** IMediaControllable
	virtual	BValue					Control(const BValue & key) const;
	virtual	status_t				SetControl(const BValue & key, const BValue & value);
	virtual	BValue					ControlInfo(const BValue & key) const;

	// ** local goodies
	virtual	status_t				AddControl(const BValue & key, const BValue & def, const BValue & info);
			status_t				AddControl(const BValue & key, const BValue & def);
	virtual	status_t				SetControlInfo(const BValue & key, const BValue & map);
	virtual	status_t				RemoveControl(const BValue & key);
	virtual	status_t				RemoveAllControls();
	
protected:
	virtual							~BMediaControllable();
	virtual	status_t				Acquired(const void* id);
	virtual	status_t				Released(const void* id);

	mutable	BLocker					mLock;
			BValue					mControls;
			BValue					mInfo;
};

} } // B::Media2
#endif //_MEDIA2_MEDIACONTROLLABLE_H_
