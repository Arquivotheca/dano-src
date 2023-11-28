
#ifndef _SUPPORT2_HANDLER_H
#define _SUPPORT2_HANDLER_H

#include <support2/Binder.h>
#include <support2/Locker.h>
#include <support2/MessageList.h>

namespace B {
namespace Support2 {

class BTeam;

/**************************************************************************************/

class IHandler : public IInterface
{
public:

	B_DECLARE_META_INTERFACE(Handler)

			status_t		PostMessageAtTime(	const BMessage &message,
												bigtime_t absoluteTime,
												IHandler::arg replyTo = atom_ptr<IHandler>(),
												uint32 flags = 0);
			status_t		PostDelayedMessage(	const BMessage &message,
												bigtime_t delay,
												IHandler::arg replyTo = atom_ptr<IHandler>(),
												uint32 flags = 0);
			status_t		PostMessage(		const BMessage &message,
												IHandler::arg replyTo = atom_ptr<IHandler>(),
												uint32 flags = 0);

	virtual	status_t		EnqueueMessage(BMessage* message) = 0;

	virtual	void			RedirectMessagesTo(	IHandler::arg new_dest) = 0;

protected:

	virtual					~IHandler() = 0;
};

/**************************************************************************************/

class LHandler : public LInterface<IHandler>
{
public:
	virtual	status_t		Told(value &in);
	virtual	status_t		Asked(const value &outBindings, value &out);
};

/**************************************************************************************/

class BHandler : public LHandler
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BHandler);
		
							BHandler();

	virtual	status_t		EnqueueMessage(BMessage* message);
	virtual	void			RedirectMessagesTo(	IHandler::arg new_dest);

	virtual	status_t		HandleMessage(const BMessage &msg);

			bigtime_t		NextMessageTime();
			status_t		DispatchMessage();

			void			ResumeScheduling();

protected:
			
	virtual					~BHandler();
	
	virtual	status_t		Acquired(const void* id);
	virtual	status_t		Released(const void* id);
		
			BMessage *		DequeueMessage(uint32 what);
			int32			CountMessages(uint32 what=B_ANY_WHAT);
			void 			DumpMessages();
			void			ClearMessages();

private:
	friend	class			BTeam;
	
			enum scheduling {
				CANCEL_SCHEDULE = 0,
				DO_SCHEDULE,
				DO_RESCHEDULE
			};

			void			DeferScheduling();
			void			Unschedule();
			scheduling		StartSchedule();
			void			DoneSchedule();

			atom_ptr<BTeam>	m_team;
			BLocker			m_lock;
			BMessageList	m_msgQueue;
			int32			m_state;
			
			BHandler *		m_next;
			BHandler *		m_prev;
			IHandler::ptr	m_redirectedTo;

};

/**************************************************************************************/

} } // namespace B::Support2

#endif /* _SUPPORT2_HANDLER_H */
