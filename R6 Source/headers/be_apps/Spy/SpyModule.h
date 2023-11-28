// --------------------------------------------------------------------------- 
/* 
	SpyModule.cpp 
	 
	Copyright (c) 2000 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			December 1, 2000 
 
	Base Module class for the CommandModule class to use. 
*/ 
// --------------------------------------------------------------------------- 
#ifndef SPYMODULE_H
#define SPYMODULE_H

#include <Handler.h>

#include <be_apps/Spy/SpyComm.h>

/*!
	Base class for the system to use, is used directly by the Monitor (spyd)
	modules, and indirectly (BSpyDisplayModule derivative) by the Display part (Spy).
	
	This is a BHandler with a SpyComm object attaced to it.
	
	\sa BSpyComm
	\sa BHandler
 */
								
class BSpyModule : public BHandler
{
	public:
						//! \param Name Names lend character.
						BSpyModule(const char* Name);
		virtual			~BSpyModule();
	
						//! Subscribe to your message constants here.
		virtual void	AttachedToLooper() = 0;
		virtual void	MessageReceived(BMessage* Message);
		
	protected:
						//! \param Message The subscribed to message.
						//! Where you get your subscritons.
		virtual void	SubscriptionReceived(BMessage* Message) = 0;	

						//! See the BSpyComm docs about these functions.
						//! \sa BSpyComm
				void	SendRemoteMessage(BMessage* Message, const char* TargetID = NULL);
				void	ReplyToMessage(BMessage* ReplyTo, BMessage* WithThis);		

				void	SubscribeTo(uint32 MessageConstant);
				void	UnsubscribeFrom(uint32 MessageConstant);
	
	private:
		BSpyComm	fSpyCommUplink;

};

#endif

