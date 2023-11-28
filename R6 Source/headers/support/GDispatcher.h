
#ifndef _G_DISPATCHER_H
#define _G_DISPATCHER_H

#include <BeBuild.h>
#include <Message.h>
#include <GMessageQueue.h>
#include <Event.h>
#include <Atom.h>

class GLooper;
class GHandler;

struct DispatcherState {
	int32	loopers;
	int32	requestAtom;
};

/*----------------------------------------------------------------*/
/*----- GDispatcher class --------------------------------------------*/

class HandlerTokens;
class GDispatcher : virtual public BAtom {
	
	public:
									GDispatcher(int32 maxThreads=8);
		virtual						~GDispatcher();

				port_id				Port();
				void				MaybeSpawn();

	private:
	
		friend	class	GLooper;
		friend	class	GHandler;
		friend	class	BMessage;

				void				ScheduleHandler(GHandler *handler);

				void				RegisterGLooper(GLooper *looper);
				void				UnregisterGLooper(GLooper *looper);

				int32				ObtainToken(GHandler *handler);
				GHandler*			LookupToken(int32 token);
				void				InvalidateToken(int32 token);

		virtual	void				_delete();

				status_t			ReadMsg(ssize_t size);
				status_t			DispatchMessage();
				bool				InsertHandler(GHandler **handlerP, GHandler *handler);

				void				UnscheduleHandler(GHandler *handler);

				Gehnaphore			m_lock;
				int32				m_maxThreads;
				int32				m_threads;
				port_id				m_pendingPort;
				Event				m_activeEvent;
				GHandler *			m_pendingHandlers;
				GHandler *			m_activeHandlers;
				int32				m_alphaThread;
				uint32				m_state;
				int32				m_rcvbufSize;
				void *				m_rcvbuf;
				uint32				m_reserved[6];
};

typedef atom<GDispatcher> dispatcher;

#endif
