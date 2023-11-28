/*
	IMTransport.h
*/
#ifndef IM_TRANSPORT_H
#define IM_TRANSPORT_H

#include <Binder.h>
#include <DataIO.h>
#include <image.h>
#include <OS.h>

class IMTransport : public BinderNode {
	public:
								IMTransport();
		virtual					~IMTransport();
	
		virtual status_t		Login(const char *username) = 0;
		virtual status_t		Logout() = 0;
	
	protected:
		virtual status_t		OpenSocketConnection(const char *server, int port);
		virtual status_t		PacketReceived(const void *packet, size_t size);
		
		int fSocket;	
	private:
								IMTransport(const IMTransport &rhs);
		IMTransport &			operator=(const IMTransport &rhs);
		
		
};

// interface
typedef IMTransport* (*make_nth_transport_type)(int32 n, image_id you, uint32 flags, ...);
extern "C" _EXPORT IMTransport* make_nth_transport(int32 n, image_id you, uint32 flags, ...);

#endif

// End of IMTransport.h
