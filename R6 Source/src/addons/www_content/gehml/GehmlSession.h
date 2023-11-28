
#ifndef	_GEHMLSESSION_H
#define	_GEHMLSESSION_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#include <stdio.h>
#include <InterfaceDefs.h>
#include <messages.h>
#include <Locker.h>
#include <Region.h>
#include <app_p/session.h>

struct message;
class TCPIPSocket;
class BString;

namespace BPrivate {
class session_buffer;
class AppSession;
}

#define BASE_SESSION BPrivate::AppSession

using namespace BPrivate;

class _BSession;

//------------------------------------------------------------------------------

class GehmlSession : public BASE_SESSION {

public:
		GehmlSession(long s_port, long r_port)
			: BASE_SESSION (s_port,r_port) {};
		GehmlSession(long s_port, const char *name=NULL)
			: BASE_SESSION (s_port,name) {};
};

//------------------------------------------------------------------------------

#endif
