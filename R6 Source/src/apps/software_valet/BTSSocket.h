// =============================================================================
//    ¥ BTSSocket.h
// =============================================================================
/*	Implementation of a socket object. Socket is not server/client specific,
	only does tcp/ip right now. */
#ifndef _B_BTSSOCKET_
#define _B_BTSSOCKET_

#include <kernel/OS.h>
#include "BTSAddress.h"

// =============================================================================
class BTSSocket
{
	public:
							BTSSocket(const int type, 
									const int protocol,
									const int family = AF_INET );
							BTSSocket(const int socketID);
		virtual				~BTSSocket();
		
				bool		InitCheck() { return (fID >= 0); };
		virtual long		SetOption(const int level, const int option,
									char* data, 
									const unsigned int size) const;
		virtual long		ConnectToAddress(const BTSAddress& address);
		virtual long		BindTo(const BTSAddress& address);
		virtual BTSSocket*	Accept(struct sockaddr *addr, int *size) const;
		virtual	long		Send(const char* buf, const size_t bufSize) const;
		virtual	long		Recv(const char* buf, const size_t bufSize) const;
		virtual long 		Open();	
		virtual long		Listen(const int maxConnections);
		virtual long		Close();
				const int	ID() { return fID; };
				bool		IsBound() { return fBound; };
		virtual bool		IsListening() { return fListening; };
		virtual bool		IsConnected() { return fConnected; };
		
		/*
		virtual long		SendLock() const;
		virtual long		SendUnlock() const;
		virtual long		RecvLock() const;
		virtual long		RecvUnlock() const;
		*/
		
		virtual void		Interrupt();
	private:
		
		int					fID;
		const int			fFamily;
		int					fType;
		int					fProtocol;
		int 				fMaxConnections;
		
		//sem_id				fSendSem;
		//sem_id				fRecvSem;
		
		bool				fConnected;
		bool				fBound;
		bool				fListening;
		bool				fInterrupted;
		int					fTimeout;
		
		virtual void		Init();
		static void			UpdateSendCount(const long numBytes);
		static void			UpdateReceiveCount(const long numBytes);
};

#endif
