#ifndef NETENDPOINTNODE_H
#define NETENDPOINTNODE_H

#include <String.h>
#include <NetEndpoint.h>
#include <NetBuffer.h>
#include <SmartArray.h>

#include <Binder.h>
#include <Gehnaphore.h>


enum {
	k_msgStartReading = '+rd!',
	k_msgStopReading = '-rd!'
};

class NetEndpointNode : public BinderNode {
public:
							NetEndpointNode();
							NetEndpointNode(BNetEndpoint*, bool);
							~NetEndpointNode();

	virtual	put_status_t	WriteProperty(const char *name, const property &prop);
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

	virtual	status_t		HandleMessage(BMessage *message);

protected:
	virtual property		b_Open(const BString &host, const int32 port);
	virtual property		b_Close();
	virtual property		b_Send(const BString &data);
	virtual property		b_Receive(size_t size = 0);
	virtual property		b_Flush();
	virtual property 		b_CRLF();
	
	virtual status_t 		InternalReceive();

	bool					m_open;
	bool					m_shouldRead;
		
	BNetBuffer				m_readBuf;
	BNetAddress				m_address;
	BNetEndpoint * 			m_endpoint;
	
	Gehnaphore				m_glock; // 9mm
};


#endif

