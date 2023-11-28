
#ifndef TCPIPSocket_h
#define TCPIPSocket_h

enum {
	RMT_APP_SOCKET = 100,
	RMT_APP_EVENT_SOCKET,
	RMT_WINDOW_SOCKET,
	RMT_OFFSCREEN_WINDOW_SOCKET
};

class TCPIPSocket {
		int 			m_socketHandle;
		long			m_remoteIP;
		unsigned short	m_remotePort;
		unsigned short	m_localPort;

		void *			m_userData;
		bool			m_isBlocking;
public:

						TCPIPSocket();
						~TCPIPSocket();
			
		uint32			RemoteIP() { return m_remoteIP; };
		void			SetBlocking(bool blocking);
		bool			Open(char *hostName, unsigned short port);
		bool			Close();
		int32			Send(const void *buffer, long buflen);
		int32			Receive(void *buffer, long *buflen);
		int32			ReceiveWait(void *buffer, long buflen);
		bool			Listen(unsigned short port);
		TCPIPSocket	*	Accept();
		void			Unblock();
		
		void			SetUserData(void *userData);
		void *			GetUserData();
};

#endif
