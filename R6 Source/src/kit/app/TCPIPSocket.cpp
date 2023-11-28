
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <OS.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "TCPIPSocket.h"

void TCPIPSocket::Unblock()
{
	sockaddr sa;
	int i=sizeof(sa);
	getsockname(m_socketHandle,&sa,&i);
};

TCPIPSocket::TCPIPSocket()
{
	m_socketHandle = -1;
	m_userData = NULL;
	m_isBlocking = true;
};

TCPIPSocket::~TCPIPSocket()
{
	if (m_socketHandle >= 0)
		Close();
};

void TCPIPSocket::SetBlocking(bool blocking)
{
	int32 on = 0xFFFFFFFF;
	int32 off = 0;

	if (blocking == m_isBlocking) return;
	setsockopt(m_socketHandle,SOL_SOCKET,SO_NONBLOCK,blocking?&off:&on,4);
	m_isBlocking = blocking;
};

void TCPIPSocket::SetUserData(void *ud)
{
	m_userData = ud;
};

void * TCPIPSocket::GetUserData()
{
	return m_userData;
};

bool TCPIPSocket::Open(char *hostName, unsigned short port)
{
	hostent * pHostEnt;
	sockaddr_in connectTo;
	
	if ((pHostEnt=gethostbyname(hostName))==NULL) {
		// errval set!
		printf("socket1\n");
		return FALSE;
	};
	if (pHostEnt->h_addr_list == NULL) {
		// errval set!
		printf("socket2\n");
		return FALSE;
	};
	m_remoteIP =  *((long *)pHostEnt->h_addr);
	m_remotePort = port;
	if ((m_socketHandle=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
		// errval set!
		printf("socket3\n");
		return FALSE;
	};
	
	memset(&connectTo, 0, sizeof(connectTo));
	connectTo.sin_len = sizeof(connectTo);
	connectTo.sin_family = AF_INET;
	connectTo.sin_port = htons(m_remotePort);
	connectTo.sin_addr.s_addr = m_remoteIP;

/*	connectFrom.sin_family = AF_INET;
	connectFrom.sin_port = 0;
	connectFrom.sin_addr.s_addr = 0;

	if (bind(m_socketHandle,(sockaddr*)&connectFrom,sizeof(connectFrom))!=0) {
		// errval set!
		return FALSE;
	};
*/	
	if (connect(m_socketHandle,(sockaddr*)&connectTo,sizeof(connectTo))!=0) {
		// errval set!
		printf("socket4\n");
		return FALSE;
	};
	
	return TRUE;
};

bool TCPIPSocket::Close()
{
	if (m_socketHandle>=0)
		close(m_socketHandle);
	m_socketHandle = -1;
	return TRUE;
};

int32 TCPIPSocket::Send(const void *buffer, long buflen)
{
	int 	totalSent=0;
	int 	lastSent=1;
	char *	buf=(char*)buffer;
	
	while ((totalSent!=buflen)&&(lastSent>0)) {
		lastSent = write(m_socketHandle,buf,buflen-totalSent);
		totalSent += lastSent;
		buf += lastSent;
	};
	
	if (totalSent != buflen) {
		// errval set!
		return lastSent;
	};
	return 0;
};

int32 TCPIPSocket::Receive(void *buffer, long *buflen)
{
	int32 r;
	char *	buf=(char*)buffer;
	
	*buflen = read(m_socketHandle,buf,*buflen);
	
	if (*buflen > 0) return 0;

	r = *buflen;
	*buflen = 0;
	return r;
};

int32 TCPIPSocket::ReceiveWait(void *buffer, long buflen)
{
	int 	totalRecv=0;
	int 	lastRecv=1;
	char *	buf=(char*)buffer;
	
	while ((totalRecv!=buflen)&&(lastRecv>0)) {
		lastRecv = read(m_socketHandle,buf,buflen-totalRecv);
		totalRecv += lastRecv;
		buf += lastRecv;
	};
	
	if (totalRecv != buflen) {
		// errval set!
		return lastRecv;
	};
	return 0;
};

bool TCPIPSocket::Listen(unsigned short port)
{
	struct sockaddr_in mine;

	m_localPort = port;

	int s;
	int i = 5;
	do {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if ((s < 0) && (i!=1)) {
			snooze(5000000);
		}
		else {
			break;
		}
	} while (--i);

	if (s < 0) {
		printf("ack can't create socket\n");
		return false;
	}

	memset(&mine, 0, sizeof(mine));
	mine.sin_len = sizeof(mine);
	mine.sin_family = AF_INET;
	mine.sin_port = htons(port);
	mine.sin_addr.s_addr = 0;
	if (bind(s, (struct sockaddr *)&mine, sizeof(mine)) < 0) {
		printf("ack can't bind socket\n");
		close(s);
		return false;
	}
	if (listen(s, 32) < 0) {
		printf("ack can't listen socket\n");
		close(s);
		return false;
	}
	m_socketHandle = s;
	return true;
};

TCPIPSocket * TCPIPSocket::Accept()
{
	struct sockaddr_in from;
	int fromlen = sizeof(from);

	int s = accept(m_socketHandle, (struct sockaddr *) &from, &fromlen);
	if (s > 0) {
		TCPIPSocket *sock = new TCPIPSocket();
		sock->m_socketHandle = s;
		sock->m_localPort = m_localPort;
		sock->m_remotePort = ntohs(from.sin_port);
		sock->m_remoteIP = from.sin_addr.s_addr;
		return sock;
	};

	return NULL;
};
