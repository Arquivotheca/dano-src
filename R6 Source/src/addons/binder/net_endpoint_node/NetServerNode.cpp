#include <Debug.h>

#include "NetServerNode.h"

#include "NetEndpointNode.h"

#define ACCEPT_TIMEOUT_SEC 5

#define streq(s1,s2) (strcmp(s1,s2) == 0)
#define strcaseeq(s1,s2) (strcasecmp(s1,s2) == 0)
#define min(a,b) (((a)<(b))?(a):(b))


NetServerNode::NetServerNode()
	: BinderNode(),
	  m_connections(NULL),
	  m_shouldListen(false),
	  m_endpoint(new BNetEndpoint()),
	  m_glock(Gehnaphore())
{
	m_connections = new BinderContainer();
	PRINT(("(+) NetServerNode ctor\n"));
}


NetServerNode::~NetServerNode()
{
	PRINT(("(-) NetServerNode dtor\n"));
	if (m_endpoint) {
		m_endpoint->Close();
		delete m_endpoint;
	}
	if (m_connections) delete m_connections;
}

get_status_t 
NetServerNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (streq(name, "Listen")) { 	/* Listen(int port) */
		prop = b_Listen(args[0].Number());
		return B_OK;
	} else if (streq(name, "StopListening")) { 	/* Listen() */
		prop = b_StopListening();
		return B_OK;
	} else if (streq(name, "connections")) {
		prop = property(m_connections);
		return B_OK;
	} else if (streq(name, "PrintToStream")) {
		BString str("[object NetServerNode");
		if (m_shouldListen) {
			str << " listening on port ";
			str << m_port;
		}
		str << "]";
		prop = property(str.String());
		return B_OK;
	} else {
		return ENOENT;
	}
}

status_t 
NetServerNode::HandleMessage(BMessage *message)
{
	status_t retval = B_OK;
	switch(message->what) {
		case k_msgStartListening:
			ResumeScheduling(); // huh?

			if (m_shouldListen) break;
			m_shouldListen = true;
			m_endpoint->Listen();

			while (m_shouldListen) {
				BNetEndpoint *connect = NULL; 
				PRINT(("NetServerNode::HandleMessage : calling Accept()\n"));
				connect = m_endpoint->Accept(1000 * ACCEPT_TIMEOUT_SEC);
				if (connect) {
					PRINT(("NetServerNode::HandleMessage : got new connection\n"));
//					binder_node newnode(new NetEndpointNode(connect));
					NetEndpointNode * newnode = new NetEndpointNode(connect, true); // connection is already open
					
					BString str("connection_");
					str << (long)(real_time_clock_usecs() % 10000000);
					
					PRINT(("NetServerNode::HandleMessage : adding as '%s'\n", str.String()));
					m_connections->AddProperty(str.String(), newnode, (permsRead|permsWrite|permsDelete));
				} 
				PRINT(("NetServerNode::HandleMessage : ResumeScheduling (end of loop)\n"));
			} 
			PRINT(("NetServerNode::HandleMessage : Close()ing endpoint\n"));
			m_endpoint->Close();
			break;
		case k_msgStopListening:
			PRINT(("NetServerNode::HandleMessage : server will shut down in %d seconds\n", ACCEPT_TIMEOUT_SEC));
			m_shouldListen = false;
			break;
		default:
			retval = BinderNode::HandleMessage(message);
			break;
	}
	return retval;
}


BinderNode::property 
NetServerNode::b_Listen(const int32 port)
{
	property return_value;
	int32 result;
	
	m_port = port;
	
	if ((result = m_endpoint->InitCheck()) != B_OK) {
		return_value = property("error");
	} else {
		if (m_endpoint->Bind(m_port) == B_OK) {
			return_value = property("ok");
			PostMessage(new BMessage(k_msgStartListening));
		} else {
			return_value = property("error: port in use");
		}
	}
	return return_value;
}

BinderNode::property 
NetServerNode::b_StopListening()
{
	PostMessage(new BMessage(k_msgStopListening));
	return property("ok");
}

