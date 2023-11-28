
#ifndef _G_HANDLER_H
#define _G_HANDLER_H

#include <BeBuild.h>
#include <Message.h>
#include <GMessageQueue.h>
#include <Atom.h>
#include <GDispatcher.h>

class BLooper;
class BMessageFilter;
class BMessage;
class BList;

enum scheduling {
	CANCEL_SCHEDULE = 0,
	DO_SCHEDULE,
	DO_RESCHEDULE
};

/*----------------------------------------------------------------*/
/*----- GHandler class --------------------------------------------*/

class GHandler : virtual public BAtom {

	public:
									GHandler(const char *name = NULL);
									GHandler(GDispatcher *dispatcher, const char *name = NULL);
		virtual						~GHandler();

		const	char *				Name();
		virtual	void				Cleanup();
		virtual	void				PostMessageAtTime(BMessage *message, bigtime_t absoluteTime);
				void				PostMessageAtTime(const BMessage &message, bigtime_t absoluteTime) { PostMessageAtTime(new BMessage(message), absoluteTime); };
		virtual	void				PostMessage(BMessage *message);
				void				PostMessage(uint32 message_constant);
				void				PostMessage(const BMessage &message) { PostMessage(new BMessage(message)); };
				void				PostDelayedMessage(BMessage *message, bigtime_t delay);
				void				PostDelayedMessage(const BMessage &message, bigtime_t delay) { PostDelayedMessage(new BMessage(message), delay); };
		virtual	status_t			HandleMessage(BMessage *message);
		virtual	status_t			DispatchMessage();
				void				ResumeScheduling();
		virtual	bigtime_t			NextMessage();
				BMessage *			DequeueMessage(uint32 what);
				int32				CountMessages(uint32 what=ANY_WHAT);
				void 				DumpMessages();
				void				ClearMessages();

				port_id				Port();
				int32				Token();
		static	atomref<GHandler>	LookupByToken(int32 token);
		
	private:

				friend class			GDispatcher;
				friend class			BMessenger;
				
		static	int32				g_defaultDispatcherCreated;
		static	dispatcher			g_defaultDispatcher;

				void				Init(GDispatcher *dispatcher, const char *name);
				void				Unschedule();
				void				DeferScheduling();
				scheduling			StartSchedule();
				void				DoneSchedule();
				
				Gehnaphore			m_lock;
				GMessageQueue		m_msgQueue;
				char *				m_name;
				GHandler *			m_next;
				GHandler *			m_prev;
				GDispatcher *		m_dispatcher;
				int32				m_state;
				int32				m_token;
				uint32				m_reserved[7];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _HANDLER_H */
