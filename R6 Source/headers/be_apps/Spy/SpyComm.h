// --------------------------------------------------------------------------- 
/* 
	SpyComm.h 
	 
	Copyright (c) 2000 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			December 1, 2000 
 
	Remote Communications class for Spy-O-Matic  
*/ 
// --------------------------------------------------------------------------- 
#ifndef SPYCOMM_H
#define SPYCOMM_H

class BHandler;
class BMessage;

/*!
	This class is used to communicate with the rest of the \e Spy-O-Matic system.
	
	It is meant as an aggregate class that a BHandler derivative will create.
	
	Normally you won't be using this class directly, as it is included in many of the
	Monitor/Display classes that would need it for you.

	\sa BHandler
	\sa BSpyDisplayView
	\sa BSpyDisplayModule
	\sa BSpyModule	
 */

class BSpyComm
{
	public:
							/*!
								\param Handler The handler class that will receive
								               messages when subscribed messages are received.
								               
								When this class goes away, you will stop receiving messages.
							 */
							BSpyComm(BHandler& Handler);
		virtual				~BSpyComm();
		
		                    /*! \param MessageConstant The constant you want to be notified about when
		                                               it comes into the messaging system.
		                                               
		                        Use this to 'sign up' for messages that other parts of the system send.
		                     */
				void		SubscribeTo(uint32 MessageConstant);

							/*!
								\param MessageConstant The constant to stop receiving notices about.
								
								\note This function currently does nothing.							
							 */
				void		UnsubscribeFrom(uint32 MessageConstant);

							/*!
								\param Message The BMessage you want to send.
								
								\param TargetID [optional] a url string address (either 3 dot or friendly style)
								                to send he message to. If this is \c NULL the currently targeted
								                target will be used (as set by the user of the application).
							
								Send a Message to a remote host. This message will be the subject of a notification
								as used by \c SubscribeTo().
  							 */				
				void		SendRemoteMessage(BMessage* Message,
							                  const char* TargetID = NULL);
							                  
							/*!
								\param ReplyTo The Message to reply to.
								\param WithThis The Message to send.
								
								When you get a message you want to reply to wihout having to worry who the sender
								was, use this function. Pass the received message as \a ReplyTo and the message you
								want to send as \a WithThis.	
							 */                  
				void		ReplyToMessage(BMessage* ReplyTo, BMessage* WithThis);
				
							/*!
								/param Message The message you want to broadcast.
								/param The port you want it broadcast on.
								
								This static function only exists because I needed it for the SpyD Sense plugin and
								I thought it might also be useful for other people. The Message will not come in to
								the Subscription mechanisim, you will need to set up a UDP listener to receive the
								message on the target machine(s).
							 */
		static	status_t	UDPBroadcastMessage(BMessage& Message, uint16 Port);		
				
	protected:
	
	private:
		BHandler&	fHandler;
};

#endif

