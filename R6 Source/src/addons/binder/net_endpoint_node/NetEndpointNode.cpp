#include "NetEndpointNode.h"
#include <GLooper.h>
#include <Debug.h>

#ifndef B_DONT_DO_THAT
	#define B_DONT_DO_THAT EPERM
#endif

#define XPRINT	PRINT

#define RECV_BUFFER_SIZE 2048
#define LOCAL_BUFFER_SIZE (RECV_BUFFER_SIZE * 5)

#define SELECT_TIMEOUT (1000000)

#define NETWORK_END_OF_LINE "\015\012"

#define streq(s1,s2) (strcmp(s1,s2) == 0)
#define strcaseeq(s1,s2) (strcasecmp(s1,s2) == 0)
#define min(a,b) (((a)<(b))?(a):(b))

NetEndpointNode::NetEndpointNode()
	: BinderNode(), // superclass
	  m_readBuf(BNetBuffer(LOCAL_BUFFER_SIZE)),
	  m_glock(Gehnaphore())
{
	m_endpoint = new BNetEndpoint();
	PRINT(("(+) NetEndpointNode ctor: new endpoint = %lp\n", m_endpoint));
}

NetEndpointNode::NetEndpointNode(BNetEndpoint *providedEndpoint, bool is_open)
	: BinderNode(), // superclass
	  m_open(is_open),
	  m_readBuf(BNetBuffer(LOCAL_BUFFER_SIZE)),
	  m_glock(Gehnaphore())
{
	m_endpoint = providedEndpoint;
	PRINT(("(+) NetEndpointNode ctor: provided endpoint = %lp\n", m_endpoint));
	
	if (m_open) {
		PostMessage(new BMessage(k_msgStartReading));
	}

}


NetEndpointNode::~NetEndpointNode()
{
	PRINT(("(-) NetEndpointNode dtor\n"));
	if (m_open) b_Close();
	if (m_endpoint != NULL) { delete m_endpoint; m_endpoint = NULL; }
}

put_status_t 
NetEndpointNode::WriteProperty(const char */*name*/, const property &/*prop*/)
{
	return B_DONT_DO_THAT;
}

get_status_t 
NetEndpointNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	get_status_t read_error = B_OK;
	
	if (streq(name, "Open")) { 				/* Open(string host, int port) */
		prop = b_Open(args[0].String(), args[1].Number());
	} else if (streq(name, "IsOpen")) {
		prop = property(m_open?"true":"false");
	} else if (streq(name, "Send")) { 		/* Send(string data) */
		if (args.Count() == 1)
			prop = b_Send(args[0].String());
	} else if (streq(name, "SendLine")) { 	/* SendLine(string data) */
		BString data("");
		if (args.Count() == 1)
			data = args[0].String();
		data << NETWORK_END_OF_LINE;
		b_Send(data);
	} else if (streq(name, "Receive")) { 	/* Receive(int length) */
		if (args.Count() > 0)
			prop = b_Receive(args[0].Number());
		else
			prop = b_Receive();
	} else if (streq(name, "Close")) { 		/* Close() */
		prop = b_Close();
	} else if (streq(name, "CRLF") || streq(name, "GetNewline")) { 		/* CRLF() */
		prop = b_CRLF();
	} else if (streq(name, "toString")) {
		BString str("[object NetEndpointNode");
		if (m_open) {
			char host[256];
			short unsigned int port;
			m_address.GetAddr(&(*host), &port); // ugghhh -- compiler pedantry
			str << " connected to " << host << ":" << (int)port;
		}
		str << "]";
		prop = property(str.String());
	} else if (streq(name, "DataAvailable")) {
		ssize_t bufferedBytes = (ssize_t)(m_readBuf.BytesRemaining());
		if (bufferedBytes<0) bufferedBytes = 0;
		prop = property((long)(bufferedBytes));
	} else {
		read_error = ENOENT;
	}

	return read_error;
}

status_t 
NetEndpointNode::HandleMessage(BMessage *message)
{
	status_t retval = B_OK;
	switch(message->what) {
		case k_msgStartReading: {
			ResumeScheduling(); // allow other BMessages to come in
			GLooper::UnregisterThis(); // get a worker thread
			
			PRINT(("NetEndpointNode::HandleMessage => (+) StartReading started\n"));
	
			m_shouldRead = true;

//			while (m_open && m_shouldRead) {
//				if (m_endpoint->IsDataPending(/*SELECT_TIMEOUT*/)) {
//					m_shouldRead = (InternalReceive() == B_OK);
//				}
//			}
			while (m_open && m_shouldRead) { // will block
				m_shouldRead = (InternalReceive() == B_OK);
			}

			PRINT(("NetEndpointNode::HandleMessage => (-) StartReading finished\n"));

			break;
		}
		case k_msgStopReading: {
			ResumeScheduling(); // allow other BMessages to come in
			m_shouldRead = false;
			break;
		}
		default:
			retval = BinderNode::HandleMessage(message);
			break;
	}
	return retval;
}



BinderNode::property 
NetEndpointNode::b_Open(const BString &host, const int32 port)
{
	PRINT(("NetEndpointNode::b_Open(\"%s\", %d)\n", host.String(), port));

//	if (m_readBuf.InitCheck() != B_OK)
//		PRINT(("NetEndpointNode::b_Open: warning: read buffer did not initialize properly\n"));
	
	status_t result;
	property return_value;
	if ((result = m_address.SetTo(host.String(), port)) == B_OK) {
		if ((result = m_endpoint->InitCheck()) == B_OK) {
			result = m_endpoint->Connect(m_address);
			// m_endpoint->SetNonBlocking(); // argh!!!
		}
	}
		
	if (result == B_OK) {
		return_value = property("ok");
		m_open = true;
	} else {
		BString err = "error: ";
		err << strerror(result);
		return_value = property(err);
	}

	if (m_open) {
		PostMessage(new BMessage(k_msgStartReading));
	}
	
	PRINT(( "NetEndpointNode::b_Open -> return_value='%s'\n",
		return_value.String().String() ));
	return return_value;
}

BinderNode::property 
NetEndpointNode::b_Close()
{
	PostMessage(new BMessage(k_msgStopReading));
	
	GehnaphoreAutoLock autoglock(m_glock);

	PRINT(("NetEndpointNode::Close\n"));
	if (m_endpoint != NULL && (m_endpoint->InitCheck() == B_OK)) {
		m_endpoint->Close();
	} else {
		PRINT(("NetEndpointNode::Close -> endpoint is null or did not initialize\n"));
	}
	m_open = false;
	return property("ok");
}

BinderNode::property 
NetEndpointNode::b_Send(const BString &data)
{
	GehnaphoreAutoLock autoglock(m_glock);

	PRINT(( "NetEndpointNode::Send -> data = '%s'\n", data.String() ));

	if (m_open) {
		if (data.Length() > 0) {
			BNetBuffer writeBuf(data.Length());
			writeBuf.AppendString(data.String());
			m_endpoint->Send(writeBuf);
		}
		return property("ok");
	} else {
		return property("error: connection closed");
	}
}

BinderNode::property
NetEndpointNode::b_Receive(size_t _size = 0)
{
	XPRINT(("NetEndpointNode::b_Receive(%d)\n", _size));
	
	GehnaphoreAutoLock autoglock(m_glock);
	// property return_value;
	BinderContainer *return_value = new BinderContainer();
	return_value->SetPermissions(permsRead|permsWrite|permsCreate|permsDelete);
	BString result_string("ok");
	
	ASSERT(return_value->PutProperty("size", property(0)) == B_OK);
	return_value->PutProperty("data", BinderNode::property::undefined);
	
	ssize_t sizeToRead = min(LOCAL_BUFFER_SIZE, _size); // TODO: dsandler: clean this shit up!

	XPRINT(("NetEndpointNode::b_Receive -> m_open=%s\n",
		(m_open?"true":"false") ));
		
	ssize_t bufferedBytes = (ssize_t)(m_readBuf.BytesRemaining());
	if (bufferedBytes<0) bufferedBytes = 0;

	XPRINT(( "NetEndpointNode::b_Receive -> bufferedBytes=%ld\n",
		bufferedBytes ));

//	if (m_open || (bufferedBytes > 0)) { // is there more data?
	if (!m_open && bufferedBytes <= 0) {
		result_string = "eof"; // property("error: connection closed");
	} else if (bufferedBytes <= 0) {
		XPRINT(("NetEndpointNode::b_Receive -> no data available\n", sizeToRead));
		// open, but no data ready
	} else {
		sizeToRead = min(sizeToRead, bufferedBytes);

		XPRINT(("NetEndpointNode::b_Receive -> sizeToRead=%lu\n", sizeToRead));
		
		if (sizeToRead > 0) { // there's actually pending data

			BString data;
			char * str = data.LockBuffer(sizeToRead+2);
			
			str[sizeToRead+1] = '\0'; // the string may terminate earlier, but we guarantee that it's long enough

			status_t remove_result = m_readBuf.RemoveString(str, sizeToRead+1);
			
			data.UnlockBuffer(-1);
			
			if (remove_result != B_OK) {
				result_string = "error";
			} else {
				XPRINT(("NetEndpointNode::b_Receive -> data='%s', length=%d\n", data.String(), data.Length()));
				return_value->PutProperty("size", property(data.Length()));
				return_value->PutProperty("data", property(data));
			}
		}
	} 

	XPRINT(( "NetEndpointNode::b_Receive -> return result = '%s'\n",
		result_string.String() ));
		
	return_value->PutProperty("result", property(result_string));
	return binder_node(return_value);
}

BinderNode::property 
NetEndpointNode::b_Flush()
{
	return property("ok");
}

BinderNode::property
NetEndpointNode::b_CRLF()
{
	return property(NETWORK_END_OF_LINE); // 13, 10 decimal
}


status_t
NetEndpointNode::InternalReceive()
{
	status_t return_value = B_OK;
	
	size_t size = RECV_BUFFER_SIZE;

	PRINT(("NetEndpointNode::InternalReceive(%ld)\n", size));
	PRINT(("NetEndpointNode::InternalReceive -> m_endpoint = %lp\n",
		m_endpoint));
	
	if (m_open) {
		char data[size+1];
		int32 sizeRead = m_endpoint->Receive(data, size);
		status_t net_error = m_endpoint->Error();
		
		PRINT(("NetEndpointNode::InternalReceive -> sizeRead=%ld\n", sizeRead));
		
		if (sizeRead > 0) { // && (m_endpoint->Error() == B_OK)
			PRINT(("NetEndpointNode::InternalReceive -> B_OK (new data ready)\n"));

			GehnaphoreAutoLock autoglock(m_glock);
			
			data[sizeRead] = '\0';
			m_readBuf.AppendString(data);
			NotifyListeners(B_PROPERTY_CHANGED, "DataAvailable");
			
		} else if ((m_endpoint->Error() == B_WOULD_BLOCK)) {
			PRINT(("NetEndpointNode::InternalReceive -> B_WOULD_BLOCK (no data, cx open)\n"));
			// no data.  come back later!
		} else { // sizeRead is probably 0
			// connection is closed or some other error
			PRINT(("NetEndpointNode::InternalReceive -> error: %ld(%s)\n",
				net_error, strerror(net_error) ));
			PRINT(("NetEndpointNode::InternalReceive -> sizeRead = %ld\n", sizeRead));
			b_Close();
			return_value = EOF;
		}
	} else {
		return_value = EOF; // property("error: connection closed");
	}

	return return_value;
}
