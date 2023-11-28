
#ifndef _G_LOOPER_H
#define _G_LOOPER_H

#include <BeBuild.h>
#include <Message.h>
#include <GHandler.h>
#include <GMessageQueue.h>

/*----------------------------------------------------------------*/
/*----- GLooper class --------------------------------------------*/

class GLooper {
	
	public:
								GLooper(GDispatcher *dispatcher, const char *name=NULL);
								~GLooper();

				status_t		Register();
				status_t		Unregister();

		static	GLooper *		This();
		static	status_t		RegisterThis();
		static	status_t		UnregisterThis();

	private:

		friend	class			GHandler;
		static	bool			ResumingScheduling();
		static	void			ClearSchedulingResumed();

		static	status_t 		send_msg_proc(port_id port, void *buf, int32 size, uint32 flags, int64 timeout, bool syncReply);
		static	int32			LaunchThread(void *instance);
				int32			Thread();

				thread_id		m_thid;
				GDispatcher *	m_dispatcher;
				int32			m_flags;
};

#endif
