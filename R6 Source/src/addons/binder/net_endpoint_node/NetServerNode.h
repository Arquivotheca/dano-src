#ifndef NETSERVERNODE_H
#define NETSERVERNODE_H

#include <NetEndpoint.h>
#include <Binder.h>

enum {
	k_msgStartListening = '+ls!',
	k_msgStopListening = '-ls!'
};

class NetServerNode : public BinderNode {
public:
							NetServerNode();
							~NetServerNode();

	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

	virtual	status_t		HandleMessage(BMessage *message);

protected:
	virtual property		b_Listen(const int32 port);
	virtual property		b_StopListening();
	
	atom<BinderContainer>	m_connections;
	bool					m_shouldListen;
	
	int32					m_port;

	BNetEndpoint * 			m_endpoint;
	Gehnaphore				m_glock; // 9mm
};

#endif

