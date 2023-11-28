// ----------------------------------------------------------------------------------------------- 
/* 
	SpyDisplayModule 
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			December 10, 2000 
 
	Abstract base class for SpyModules, provides a little functionality. 
*/ 
// ----------------------------------------------------------------------------------------------- 
#ifndef SPYDISPLAYMODULE_H
#define SPYDISPLAYMODULE_H

#include <vector> 
class BView;

#define B_SPY_WINDOW_SET_MESSAGE_TARGET	'__ST'

#include <be_apps/Spy/SpyModule.h>

/*!
	The module the Display portion of Spy-O-Matic uses to organize modules. Implement it's CreateView()
	method and return your BSpyDisplayView derivative.
 
 */

class BSpyDisplayModule : public BSpyModule
{
	public:
								/*!
									\param Name A name you give your module."
									
									Call this and pass the name from your derivative.
								*/
								BSpyDisplayModule(const char* Name);
		virtual					~BSpyDisplayModule();
		
								/*!
									\returns A BSpyDisplayView derivative that know how to
									         represent the data that your MonitorModule
									         spits out.								

									\sa BSpyDisplayView()
								 */
		virtual BView*			CreateView() = 0;
		
								/*!
									You have been attaced to the main looper which does all the message
									processing. This is the place to subscribe to any messages that you might
									want.
									
									\note You won't normally subscribe to the data messages that your Views
									      want data for, do that in the BSpyDisplayView() derivative.
									      
									\sa BSpyDisplayView()
								 */
		virtual	void			AttachedToLooper();
		virtual void			MessageReceived(BMessage* Message);
		
	protected:
								/*!
									\param Message A message you have subscribed to.
									
									Here is where you react to any messages you subscribed to.
								 */
		virtual void			SubscriptionReceived(BMessage* Message);
	
	private:
				void			HandleViewRequest(BMessage* Message);
				
};


#endif

