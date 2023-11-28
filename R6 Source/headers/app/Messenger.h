/******************************************************************************
/
/	File:			Messenger.h
/
/	Description:	BMessenger class provides the mechanism for delivering
/					BMessages to BLooper/BHandler targets.
/					Eminently stack-allocable.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MESSENGER_H
#define _MESSENGER_H

#include <BeBuild.h>
#include <OS.h>
#include <ByteOrder.h>
#include <Message.h>		/* For convenience */

class GHandler;
class BDataIO;
class BHandler;
class BLooper;
class BMessenger;
namespace B {
namespace App2 {
class BHandler;
}
}


/*---------------------------------------------------------------*/
/* --------- BMessenger class----------------------------------- */

bool operator<(const BMessenger & a, const BMessenger & b);
bool operator!=(const BMessenger & a, const BMessenger & b);

class BMessenger {
public:	
					BMessenger();

					BMessenger(GHandler *handler);
					BMessenger(const char *mime_sig, 
								team_id team = -1,
								status_t *perr = NULL);

					BMessenger(const BHandler *handler, 
								const BLooper *looper = NULL,
								status_t *perr = NULL);
					BMessenger(const BMessenger &from);
					~BMessenger();

/* Target */
		bool		IsTargetLocal() const;
		BHandler	*Target(BLooper **looper) const;
		bool		LockTarget() const;
		status_t	LockTargetWithTimeout(bigtime_t timeout) const;

/* Asynchronous message sending */
		status_t	SendMessageAtTime(const BMessage& a_message,
										bigtime_t absoluteTime,
										const BMessenger& reply_to = BMessenger(),
										uint32 flags = B_TIMEOUT,
										bigtime_t timeout = B_INFINITE_TIMEOUT) const;
		
		status_t	SendMessage(uint32 command,
								const BMessenger& reply_to = BMessenger(),
								uint32 flags = B_TIMEOUT,
								bigtime_t timeout = B_INFINITE_TIMEOUT) const;
		status_t	SendMessage(const BMessage& a_message,
								const BMessenger& reply_to = BMessenger(),
								uint32 flags = B_TIMEOUT,
								bigtime_t timeout = B_INFINITE_TIMEOUT);
		status_t	SendDelayedMessage(const BMessage& a_message,
										bigtime_t delay,
										const BMessenger& reply_to = BMessenger(),
										uint32 flags = B_TIMEOUT,
										bigtime_t timeout = B_INFINITE_TIMEOUT) const;
		
/* Synchronous message sending */
		status_t	SendMessage(const BMessage& a_message,
								BMessage* reply,
								uint32 flags = B_TIMEOUT,
								bigtime_t send_timeout = B_INFINITE_TIMEOUT,
								bigtime_t reply_timeout = B_INFINITE_TIMEOUT) const;
								
		status_t	SendMessage(uint32 command, BMessage* reply) const;

/* Old message sending interface */
		status_t	SendMessage(uint32 command, BHandler *reply_to) const;
		status_t	SendMessage(BMessage *a_message,
								BHandler *reply_to = NULL,
								bigtime_t timeout = B_INFINITE_TIMEOUT) const;
		status_t	SendMessage(BMessage *a_message,
								BMessenger reply_to,
								bigtime_t timeout = B_INFINITE_TIMEOUT) const;
		status_t	SendMessage(BMessage *a_message,
								BMessage *reply,
								bigtime_t send_timeout = B_INFINITE_TIMEOUT,
								bigtime_t reply_timeout = B_INFINITE_TIMEOUT) const;
	
/* Operators and misc */
		BMessenger	&operator=(const BMessenger &from);
		bool		operator==(const BMessenger &other) const;

		bool		IsValid() const;
		team_id		Team() const;

/*----- Private or reserved ------------------------------*/
private:
friend class BRoster;
friend class _TRoster_;
friend class BMessage;
friend class B::App2::BHandler;
friend inline void		_set_message_reply_(BMessage *, BMessenger);
friend inline port_id	_get_messenger_port_(const BMessenger&);
friend inline int32		_get_messenger_token_(const BMessenger&);
friend inline bool		_get_messenger_preferred_(const BMessenger&);
friend status_t			swap_data(type_code, void *, size_t, swap_action);
friend bool		operator<(const BMessenger & a, const BMessenger & b);
friend bool		operator!=(const BMessenger & a, const BMessenger & b);
friend BDataIO& operator<<(BDataIO& io, const BMessenger& messenger);
				
					BMessenger(team_id team,
								port_id port,
								int32 token,
								bool preferred);

		void		InitData(const char *mime_sig,
							team_id team,
							status_t *perr);

		port_id		fPort;
		int32		fHandlerToken;
		team_id		fTeam;
		int32		extra0;
		int32		extra1;
		bool		fPreferredTarget;
		bool		extra2;
		bool		extra3;
		bool		extra4;
};

BDataIO& operator<<(BDataIO& io, const BMessenger& messenger);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

inline status_t BMessenger::SendMessage(uint32 command,
									const BMessenger& reply_to,
									uint32 flags,
									bigtime_t timeout) const
{
	return SendMessageAtTime(BMessage(command), 0, reply_to, flags, timeout);
}

inline status_t BMessenger::SendMessage(const BMessage& a_message,
									const BMessenger& reply_to,
									uint32 flags,
									bigtime_t timeout)
{
	return SendMessageAtTime(a_message, 0, reply_to, flags, timeout);
}

inline status_t BMessenger::SendDelayedMessage(const BMessage& a_message,
											bigtime_t delay,
											const BMessenger& reply_to,
											uint32 flags,
											bigtime_t timeout) const
{
	return SendMessageAtTime(a_message, system_time()+delay, reply_to, flags, timeout);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _MESSENGER_H */
