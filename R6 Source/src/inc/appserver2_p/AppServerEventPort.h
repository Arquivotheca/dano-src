
#ifndef	_APPSERVER2_EVENTPORT_H_
#define	_APPSERVER2_EVENTPORT_H_

#include <support2/SupportDefs.h>
#include <kernel/OS.h>

namespace B {
namespace AppServer2 {

enum {
	B_UPDATE_INVALIDATED	= 0x00000001,
	B_UPDATE_SCROLLED		= 0x00000002,
	B_UPDATE_RESIZED		= 0x00000004,
	B_UPDATE_EXPOSED		= 0x00000008
};

class BAppServerEventPort {

	public:

							BAppServerEventPort(
								int32 *atomic,
								uint32 *nextBunch,
								uint32 eventBase,
								sem_id clientBlock);
							~BAppServerEventPort();

		void				ProcessPending();
		BMessage *			GenerateMessage();
		int32				Messages();

	private:
	
		int32 *				m_atomic;
		sem_id				m_clientBlock;
		uint32 *			m_nextBunch;
		uint32				m_eventBase;
		int32				m_eventsQueued;
		bool				m_pending;
};

} } // namespace B::AppServer2

using namespace B::AppServer2;

#endif /* _APPSERVER2_EVENTPORT_H_ */
