
#include <appserver2_p/AppServerEventPort.h>
#include <support2/Message.h>

namespace B {
namespace AppServer2 {

enum {
	epEvents			= 0x0000FFFF,
	epWindowResized		= 0x00010000,
	epWindowMoved 		= 0x00020000,
	epViewsMoveSized	= 0x00040000,
	epBlockClient		= 0x00080000
};

struct BAppServerEvent {
	uint32			next;
	int16			eventType;
	int16			transit;
	void *			meaningless;
	struct message_header {
		uint32 signature;
		uint32 size;
		uint32 what;
		uint32 reserved0;
	}				header;
	uint8			theRest[1];
};

BAppServerEventPort::BAppServerEventPort(
	int32 *atomic,
	uint32 *nextBunch,
	uint32 eventBase,
	sem_id clientBlock)
{
	m_atomic = atomic;
	m_eventBase = eventBase;
	m_clientBlock = clientBlock;
	m_nextBunch = nextBunch;
	m_eventsQueued = 0;
	m_pending = false;
};

BAppServerEventPort::~BAppServerEventPort()
{
};

int32 BAppServerEventPort::Messages()
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	return m_eventsQueued;
};

void BAppServerEventPort::ProcessPending()
{
	uint32 oldValue;
	status_t err;
	
	if (m_eventsQueued) {
		m_pending = true;
		return;
	};
	
	m_pending = false;
	oldValue = atomic_and(m_atomic,0);
	while (oldValue & epBlockClient) {
		do {
			err = acquire_sem(m_clientBlock);
		} while (err == B_INTERRUPTED);
		oldValue = atomic_and(m_atomic,0);
	};
	m_eventsQueued = oldValue & epEvents;
};

BMessage * BAppServerEventPort::GenerateMessage()
{
	if (!m_eventsQueued && m_pending) ProcessPending();
	if (!m_eventsQueued) return NULL;

	BAppServerEvent *msg = (BAppServerEvent *)(*m_nextBunch + m_eventBase);
	m_nextBunch = &msg->next;
	m_eventsQueued--;

	BMessage *bm = new BMessage();
//	bm->Unflatten((char*)&msg->header);
	
	return bm;
};

} } // namespace B::AppServer2
