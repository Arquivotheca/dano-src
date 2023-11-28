// ---------------------------------------------------------------------------------------- 
/* 
	SpyDisplayView.h 
	 
	Copyright (c) 2000 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			December 22, 2000
 
	Base class for Spy-O-Matic display views. 
*/ 
// ----------------------------------------------------------------------------------------
#ifndef SPYDISPLAYVIEW_H
#define SPYDISPLAYVIEW_H

#include <View.h>

#include <be_apps/Spy/SpyComm.h>

//!	The amount of space to the left and bottom of the Module List View.
const float kEdgeSpace      = 10.0;

/*!
	The class that your BSpyDisplayModule coughs up in the CreateView() class.
	
	This class will know how to display the data that your BSpyModule on the Monitor
	side of the application (in spyd) send out.
	
	Is is a BView derivative with a built in BSpyComm object.
*/

class BSpyDisplayView : public BView
{
	public:
						//! Should be obvious
						BSpyDisplayView(const char* Name,
						                uint32 Flags =  B_WILL_DRAW | B_FRAME_EVENTS,
						                uint32 ResizeMask = B_FOLLOW_ALL);
						BSpyDisplayView(BRect Frame,
										const char *Name,
										uint32 ResizeMask,
										uint32 Flags);

		virtual			~BSpyDisplayView();
		
		virtual void	MessageReceived(BMessage* Message);
		
						/*!
							 This is the place to subscribe to message constants.
							 
							 \note Subscribing more than once is probably not a great idea.
						 */
		virtual void	AttachedToWindow();
		virtual void	DetachedFromWindow();

						/*!
							The function gets called when the remote target becomes invalid
							for any reason (it went away, the user chose something new, etc.)
						 */
		virtual void	DetachedFromRemote();

						/*!
							BSpyComm wrappers.
							
						\sa BSpyComm
						 */
				void	SendRemoteMessage(BMessage* Message, const char* TargetID = NULL);
				void	ReplyToMessage(BMessage* ReplyTo, BMessage* WithThis);		

				void	SubscribeTo(uint32 MessageConstant);
				void	UnsubscribeFrom(uint32 MessageConstant);
		
	protected:
		virtual void	SubscriptionReceived(BMessage* Message);	

	private:
		virtual void	_ReservedSpy0_();
		virtual void	_ReservedSpy1_();
		virtual void	_ReservedSpy2_();
		virtual void	_ReservedSpy3_();
		virtual void	_ReservedSpy4_();
		virtual void	_ReservedSpy5_();
		virtual void	_ReservedSpy6_();
		virtual void	_ReservedSpy7_();
		
	private:
		BSpyComm		fSpyCommUplink;

	private:
		uint32			_ReservedChunk_[8];
};

#endif

