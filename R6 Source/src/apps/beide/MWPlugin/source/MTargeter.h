//==================================================================
//	MTargeter.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MTARGETER_H
#define _MTARGETER_H

#include <SupportDefs.h>

class BHandler;
class BMessage;
class BLooper;

class MTargeter
{
public:
								MTargeter(
									ulong		inMessageType  = 0);
								~MTargeter();

	long						SetTarget(
									BHandler*	inTarget);
	void						SetMessage(
									BMessage*	inMessage);
	void						PostMessageToTarget();

	BHandler*					Target() const ;
	BMessage*					Message() const;

private:

	BMessage*					fMessage;
	BHandler*					fTarget;
	BLooper*					fLooper;
};

#endif
