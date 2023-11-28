/*****************************************************************************

     $Source: /net/bally/be/rcs/src/inc/app_p/message_util.h,v $

     $Revision: 1.32 $

     $Author: herold $

     $Date: 1997/04/05 02:42:52 $

     Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef _MESSAGE_UTIL_H
#define _MESSAGE_UTIL_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

/* --------------------------------------------------------------------- */
// shared defines with the app_server. 

//
// the 'code' field of write/read port is used to identify the style
// of message that was written to the port. Here are the options:
//

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _LOCKER_H
#include <Locker.h>
#endif
#ifndef _MESSENGER_H
#include <Messenger.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _MESSAGE_BODY_H
#include <MessageBody.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#ifndef _HANDLER_H
#include <Handler.h>
#endif

#include <message_strings.h>

struct _loop_data_ {
	const BLooper	*ptr;
	long			id;
};

BMessage 	*_reconstruct_msg_(uint32 what, uint32 serverBase);

void _set_message_reply_(BMessage *, BMessenger);
inline void _set_message_reply_(BMessage *msg, BMessenger messenger)
	{
	message_target& target = *(message_target*)msg->fTarget;
	target.reply_team = messenger.fTeam;
	target.reply_target = messenger.fHandlerToken;
	target.reply_port = messenger.fPort;
	target.flags = (target.flags&~MTF_PREFERRED_REPLY)
				 | (messenger.fPreferredTarget ? MTF_PREFERRED_REPLY : 0)
				 | MTF_DELIVERED;
	}

void _set_message_target_(BMessage *msg, int32 token, bool preferred = FALSE);
inline void _set_message_target_(BMessage *msg, int32 token, bool preferred)
	{
	message_target& target = *(message_target*)msg->fTarget;
	target.target = token;
	target.flags = (target.flags&~MTF_PREFERRED_TARGET)
				 | (preferred ? MTF_PREFERRED_TARGET : 0);
	}

int32 _get_message_target_(BMessage *msg);
inline int32 _get_message_target_(BMessage *msg)
	{ return ((message_target*)msg->fTarget)->target; }
	
bool _use_preferred_target_(BMessage *msg);
inline bool _use_preferred_target_(BMessage *msg)
	{ return ((message_target*)msg->fTarget)->flags & MTF_PREFERRED_TARGET; }
	
int32 _get_object_token_(const BHandler *r);
inline long _get_object_token_(const BHandler *r)
	{ return r->fToken; }

port_id _get_looper_port_(const BLooper *loop);

port_id _get_messenger_port_(const BMessenger&);
inline port_id _get_messenger_port_(const BMessenger& m)
	{ return m.fPort; }
int32 _get_messenger_token_(const BMessenger&);
inline int32 _get_messenger_token_(const BMessenger& m)
	{ return m.fHandlerToken; }
bool _get_messenger_preferred_(const BMessenger&);
inline bool _get_messenger_preferred_(const BMessenger& m)
	{ return m.fPreferredTarget; }

namespace BPrivate {
class		AppSession;
}

class	_BAppServerLink_ {

public:
					_BAppServerLink_();
					_BAppServerLink_(bool assert_session_exists);
					~_BAppServerLink_();

		BPrivate::AppSession	*session;

private:
		void		Init(bool assert_session_exists);
};

class BClipboard;
extern BClipboard *_system_clipboard_;

extern BLocker *_message_locker_;

#endif
