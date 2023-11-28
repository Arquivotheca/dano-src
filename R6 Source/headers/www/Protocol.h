#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#include <OS.h>
#include <image.h>
#include <Errors.h>
#include <DataIO.h>
#include "Resource.h"

namespace Wagner {

const uint kMaxSchemeLength = 32;
typedef void (*MetaCallback)(void *cookie, const char *name, const char *value);

#define S_PROTOCOL_SCHEMES "schemes"

// These are standard error fields for protocols to place in
// 'outErrorParams'.
#define S_ERROR_TEMPLATE "template"
#define S_ERROR_SCHEME "scheme"
#define S_ERROR_MIME_TYPE "mime_type"
#define S_ERROR_MESSAGE "message"
#define S_CHALLENGE_STRING "challenge"

class URL;
class ContentHandle;

class Protocol : public BPositionIO
{
public:
						Protocol(void* handle);
virtual					~Protocol();

virtual	status_t		Open(	const URL &url, const URL &requestor,
								BMessage *errorParams, uint32 flags);
virtual	ssize_t			GetContentLength();
virtual	void			GetContentType(char *type, int size);
virtual	CachePolicy		GetCachePolicy();
// Returns true if this redirects to another URL, and fills in the URL field.
virtual	bool			GetRedirectURL(URL&, bigtime_t *outDelay);

virtual	void			Abort();
virtual	ssize_t			Read(void *buffer, size_t size);
virtual	ssize_t			ReadAt(off_t pos, void *buffer, size_t size);
virtual	off_t			Seek(off_t position, uint32 seek_mode);
virtual	off_t			Position() const;

static	Protocol*		InstantiateProtocol(const char *scheme);

// Note to Protocol users: When doing network write()s or send()s, you might
// receive a SIGPIPE if there's an error during the send().  The default
// action for SIGPIPE is to kill the thread, which is usually not what you
// want.  You can set your thread to ignore SIGPIPEs by calling
// "signal(SIGPIPE, SIG_IGN)" (#include <signal.h>).  This will also carry
// over to threads that that thread spawns, since they will inherit the
// spawning thread's signal mask.  Wagner does this for its main thread,
// so if your code will be called only by Wagner, you don't have to worry
// about this.

virtual	ssize_t			Write(const void *buffer, size_t size);
virtual	ssize_t			WriteAt(off_t pos, const void *buffer, size_t size);
virtual status_t		SetMetaCallback(void *cookie, MetaCallback);

private:
		/* FBC */
virtual	void			_ReservedProtocol2();
virtual	void			_ReservedProtocol3();
virtual	void			_ReservedProtocol4();
virtual	void			_ReservedProtocol5();
virtual	void			_ReservedProtocol6();
virtual	void			_ReservedProtocol7();
virtual	void			_ReservedProtocol8();
virtual	void			_ReservedProtocol9();
virtual	void			_ReservedProtocol10();
virtual	void			_ReservedProtocol11();
virtual	void			_ReservedProtocol12();
		uint32			_reserved[8];


virtual	status_t		SetSize(off_t size);
		ContentHandle*	m_handle;
};

class ProtocolFactory {
public:
						ProtocolFactory();
virtual					~ProtocolFactory();
virtual	void			GetIdentifiers(BMessage* into)						= 0;
virtual	Protocol*		CreateProtocol(void* handle, const char* scheme)	= 0;
	
		// Return true to try to keep your add-on loaded even when it
		// is not being used.
virtual	bool			KeepLoaded() const;
	
		// Return the amount of memory used statically by this protocol
		// add-on -- that is, the amount of heap space it uses just to
		// be loaded but -not- any memory allocated privately by Protocol
		// objects.
virtual	size_t			GetMemoryUsage() const;

private:
		/* FBC */
virtual	void						_ReservedProtocolFactory1();
virtual	void						_ReservedProtocolFactory2();
virtual	void						_ReservedProtocolFactory3();
virtual	void						_ReservedProtocolFactory4();
virtual	void						_ReservedProtocolFactory5();
virtual	void						_ReservedProtocolFactory6();
virtual	void						_ReservedProtocolFactory7();
virtual	void						_ReservedProtocolFactory8();
virtual	void						_ReservedProtocolFactory9();
virtual	void						_ReservedProtocolFactory10();
virtual	void						_ReservedProtocolFactory11();
virtual	void						_ReservedProtocolFactory12();
		uint32						_reserved[8];
};

}

using namespace Wagner;

// interface
typedef ProtocolFactory* (*make_nth_protocol_type)(int32 n, image_id you, uint32 flags, ...);
extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id you, uint32 flags, ...);

#endif
