//******************************************************************************
//
//	File:		session.h
//
//	Description:	connection session.
//
//	Copyright 1992, Be Incorporated
//
//******************************************************************************

#ifndef	_SESSION_H
#define	_SESSION_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif

#include <Locker.h>
#include <Rect.h>

struct message;
class TCPIPSocket;
class BRegion;
class BString;

namespace BPrivate {

class session_buffer;
class IRegion;

//------------------------------------------------------------------------------

class	AppSession {

public:
					AppSession(TCPIPSocket *sock, bool validate_locking=false);
					AppSession(long s_port, long r_port, bool validate_locking=false);
					AppSession(long s_port, const char *name=NULL, bool validate_locking=false);
virtual				~AppSession();

		bool		lock() const;
		bool		lock(int32 token, int32 cmd=GR_PICK_VIEW);
		void		unlock() const;
		
		void		select(int32 token, int32 cmd=GR_PICK_VIEW);
		void		forget_last();
		
		void		swrite(long size, const void *buffer);
		void		swrite_l(long value);
		void		swrite_s(short value);
		void		swrite(const char *);
		void		swrite(const session_buffer& buffer);
		void		sread(long size, void *buffer);
		void		sreadd(long size, void *buffer);
		char		*sread();
		void		sread(BString* into);
		void		sread(long size, session_buffer* buffer);
		int32		drain(int32 size);
		long		sread2(long size, void *buffer);
		void		add_to_buffer(message *);
		long		get_other(message *a_message);
		void		flush();
		void		flush_debug();
		void		full_sync();
		void		sclose();
		void		xclose();
		void		swrite_point(const BPoint *p);
		void		swrite_point_a(const BPoint *p);
		void		sread_point(BPoint *p);
		void		sread_point_a(BPoint *p);
		void		sread_rect(clipping_rect *r);
		void		sread_rect(BRect *r);
		void		sread_rect_a(BRect *r);
		void		swrite_rect(const BRect *r);
		void		swrite_rect(const clipping_rect *r);
		void		swrite_rect_a(const BRect *r);
		void		swrite_coo(const float *value);
		void		swrite_coo_a(const float *value);
		void		sread_coo(float *value);
		void		sread_coo_a(float *value);

		long		sread_region(BRegion *a_region);
		long		sread_region(BPrivate::IRegion *a_region);
		void		swrite_region(const BRegion *a_region);
		void		swrite_region(const BPrivate::IRegion *a_region);
		
		// a special write, to place data directly in the buffer.
		// note very well: be prepared to deal with a NULL return by
		// allocating your own buffer.
		void*		inplace_write(long size);
		
		int32		send_buffer();
		int32		recv_buffer();

#ifdef	DEBUG_PROTO
		void		pdebug(long port_id, long session_id, message *a_message);
#endif

private:
mutable	BLocker		session_lock;

		message		*s_message;
		message		*r_message;
		char		*message_buffer;
		long		cur_pos_send;
		long		cur_pos_receive;
		long		session_id;
		bool		delete_send_port;
		bool		validate_locking;
		
		long		send_port;
		long		receive_port;
		long		receive_size;
		TCPIPSocket *socket;
		int32		m_msgCode;
		int32		state;
		int32		cur_cmd;
		int32		cur_token;
};

//------------------------------------------------------------------------------

}	// namespace BPrivate

using namespace BPrivate;

/*-------------------------------------------------------------*/

inline void AppSession::swrite_s(short value)
{
	swrite(2, &value);
}

inline void AppSession::swrite_l(long value)
{
	swrite(4, &value);
}

inline void AppSession::swrite_point_a(const BPoint *p)
{
	swrite(2*sizeof(float), (void *)p);
}

inline void AppSession::swrite_rect_a(const BRect *r)
{
	swrite(sizeof(BRect), (void *)r);
}

inline void AppSession::swrite_coo_a(const float *value)
{
	swrite(sizeof(float), value);
}

/*-------------------------------------------------------------*/

inline void AppSession::sread_rect_a(BRect *r)
{
	sread(4*sizeof(float), r);
}

inline void AppSession::sread_rect(clipping_rect *r)
{
	sread(sizeof(clipping_rect), r);
}

inline void AppSession::sread_point_a(BPoint *p)
{
	sread(2*sizeof(float), p);
}

inline void AppSession::sread_coo_a(float *value)
{
	sread(sizeof(float), value);
}

/*-------------------------------------------------------------*/

#endif
