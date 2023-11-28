/******************************************************************************
/
/	File:			Message.h
/
/	Description:	BMessage class creates objects that store data and that
/					can be processed in a message loop.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _SUPPORT2_MESSAGE_H
#define _SUPPORT2_MESSAGE_H

#include <support2/SupportDefs.h>
#include <support2/Handler.h>
#include <support2/ITextStream.h>
#include <support2/Value.h>

namespace B {
namespace Support2 {

class BMessageList;

/*-------------------------------------------------------------*/
/* --------- BMessage class----------------------------------- */

class BMessage
{
public:

							BMessage();
							BMessage(uint32 what);
							BMessage(const BMessage &);
							BMessage(const BValue &);
	virtual					~BMessage();
	
			uint32			What() const;
			BMessage&		SetWhat(uint32 val);
			
			bigtime_t		When() const;
			BMessage&		SetWhen(bigtime_t val);
			
			IHandler::ptr	Sender() const;
			BMessage&		SetSender(IHandler::ref val);
			
			IHandler::ptr	ReplyTo() const;
			BMessage&		SetReplyTo(IHandler::ref val);
			
			const BValue&	Data() const;
			BValue&			Data();
			BMessage&		SetData(const BValue& data);
			
			BValue			AsValue() const;
	inline					operator BValue() const			{ return AsValue(); }
	
			void			PrintToStream(ITextOutput::arg io, uint32 flags = 0) const;
	
private:

	friend	class			BMessageList;
	
			uint32			m_what;
			bigtime_t		m_when;
			IHandler::ref	m_sender;
			IHandler::ref	m_replyTo;
			BValue			m_data;
			
			BMessage *		m_next;
			BMessage *		m_prev;
};

ITextOutput::arg operator<<(ITextOutput::arg io, const BMessage& message);

/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _MESSAGE_H */

