//******************************************************************************
//
//	File:			UploadContent.cpp
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <Autolock.h>
#include <Binder.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <Locker.h>
#include <Message.h>
#include <Screen.h>
#include <View.h>
#include <Volume.h>

#include <errno.h>
#include <stdio.h>

#include <ResourceCache.h>

#include "Gehnaphore.h"
#include "Content.h"
#include "Resource.h"
#include "Timer.h"
#include "Protocol.h"
#include "URL.h"

using namespace Wagner;

typedef enum {
	UPLOAD_NO_ERROR					=  0,
	UPLOAD_GENERAL_ERROR			= -1,
	UPLOAD_CONNECTION_ERROR			= -2,
	UPLOAD_IO_ERROR					= -3,
} upload_error_codes;

const size_t UNKNOWN_LENGTH = 0x7fffffffL;	// this is a special value indicating unknown length in libwww

const int32 UPLOAD_MESSAGE = 'upld';

// NOTE: these properties need to stay in alphabetical order.  If a new property
// is inserted in this list, code to handle it should be added to ReadProperty().
static const char*	properties[] = {
						"bytesCompleted",
						"cancelUpload",
						"errCode",
						"errInfo",
						"fractionCompleted",
						"totalBytes"
};

static const int32	numProperties = (sizeof(properties) / sizeof(properties[0]));

class UploadContentInstance : public ContentInstance, public BinderNode {
public:
							UploadContentInstance(Content *content, GHandler *handler);
	virtual 				~UploadContentInstance();
	
			void			ChangedBytecount();
			void			ErrorOccurred();

	virtual	void			Cleanup();
	
protected:

	friend	class			UploadContent;

	struct store_cookie {
		int32 index;
	};

	virtual	status_t		OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t		NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t		CloseProperties(void *cookie);
	virtual	put_status_t	WriteProperty(const char *name, const property &prop);
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	virtual status_t        HandleMessage(BMessage *msg);

	static	void			HttpCallback(void *cookie, const char *name, const char *value);
};

class UploadContent : public Content {
	BLocker					m_lock;
	BLocker					m_uploadLock;
	UploadContentInstance   *m_instance;
	Protocol				*m_protocol;
	ssize_t					m_totalBytes;
	ssize_t					m_completedBytes;
	BString					m_errInfo;
	ssize_t					m_errCode;
	URL						m_targetURL;

public:
							UploadContent(void *handle);
	virtual 				~UploadContent();

	virtual ssize_t			Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t			GetMemoryUsage();
	virtual	bool			IsInitialized();
	
private:
			void			SetError(ssize_t errCode, const char *errInfo);
			void			CancelUpload();	// called when the user cancels the download
			
	virtual status_t		CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);
			status_t		UploadToProtocol();

	friend class UploadContentInstance;
};


UploadContentInstance::UploadContentInstance(Content *content, GHandler *handler)
	: ContentInstance(content,handler)
{
}

UploadContentInstance::~UploadContentInstance()
{
}

void 
UploadContentInstance::Cleanup()
{
	BinderNode::Cleanup();
	ContentInstance::Cleanup();
}

put_status_t 
UploadContentInstance::WriteProperty(const char *name, const property &prop)
{
	return B_ERROR;
}

void
UploadContentInstance::ChangedBytecount()
{
	NotifyListeners(B_PROPERTY_CHANGED,"bytesCompleted");
	NotifyListeners(B_PROPERTY_CHANGED,"fractionCompleted");
}

void
UploadContentInstance::ErrorOccurred()
{
	NotifyListeners(B_PROPERTY_CHANGED,"errCode");
	NotifyListeners(B_PROPERTY_CHANGED,"errInfo");
}

status_t UploadContentInstance::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) {
		*c = *((store_cookie*)copyCookie);
	} else {
		c->index = 0;
	}
	*cookie = c;
	return B_OK;
}

status_t UploadContentInstance::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;

	if (c->index < numProperties) {	
		const char *name = properties[c->index];
		strncpy(nameBuf, name, *len);
		*len = strlen(name);
		c->index++;
		err = B_OK;
	}
	
	return err;
}

status_t UploadContentInstance::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

get_status_t 
UploadContentInstance::ReadProperty(const char *name, property &prop, const property_list &args)
{
	UploadContent *c = dynamic_cast<UploadContent*>(GetContent());
	if (c == NULL)
		return B_ERROR;
	
	BAutolock(c->m_lock);
	
	if (!strcmp(name,"bytesCompleted")) {
		prop = (double)c->m_completedBytes;
	} else if (!strcmp(name,"totalBytes")) {
		size_t len = c->m_totalBytes;
		if (len > 0)	prop = (double)len;
		else			prop = property::undefined;
	} else if (!strcmp(name,"fractionCompleted")) {
		size_t len = c->m_totalBytes;
		if (len > 0)	prop = ((double)c->m_completedBytes) / len;
		else			prop = property::undefined;
	} else if (!strcmp(name,"errCode")) {
		prop = (double)c->m_errCode;
	} else if (!strcmp(name,"errInfo")) {
		prop = c->m_errInfo.String();
	} else if (!strcmp(name,"cancelUpload")) {
		c->CancelUpload();
	} else {
		return ENOENT;
	}

	return B_OK;
}

status_t
UploadContentInstance::HandleMessage(BMessage *msg)
{
	UploadContent *c = dynamic_cast<UploadContent*>(GetContent());
	if (c == NULL)
		return B_ERROR;

	switch(msg->what) {
		case UPLOAD_MESSAGE: {
			ResumeScheduling();
			if(c->UploadToProtocol() < 0)
				ErrorOccurred();
			break;
		}
		default:
			break;
	}
	return B_OK;
}

void
UploadContentInstance::HttpCallback(void *cookie, const char *name, const char *value)
{
	UploadContentInstance *uci = static_cast<UploadContentInstance*>(cookie);
	if (uci == NULL)
		return;
	UploadContent *c = dynamic_cast<UploadContent*>(uci->GetContent());
	if (c == NULL)
		return;
	
	if(strcmp("totalBytes", name) == 0) {
		c->m_totalBytes = *(ssize_t *)value;
//		printf("UploadContentInstance::HttpCallback: totalBytes now %d\n", c->m_totalBytes);
		uci->ChangedBytecount();
	} else if(strcmp("sentBytes", name) == 0) {
		c->m_completedBytes = *(ssize_t *)value;
//		printf("UploadContentInstance::HttpCallback: completedBytes now %d\n", c->m_completedBytes);
		uci->ChangedBytecount();
	}	
	
}

UploadContent::UploadContent(void *handle)
	: Content(handle),
	  m_lock("UploadContent lock"),
	  m_uploadLock("UploadContent upload lock"),
	  m_instance(NULL),
	  m_protocol(NULL),
	  m_totalBytes(UNKNOWN_LENGTH),
	  m_completedBytes(0),
	  m_errInfo(B_EMPTY_STRING),
	  m_errCode(0)
{

}

UploadContent::~UploadContent()
{
	if(m_protocol)
		delete m_protocol;
}

ssize_t 
UploadContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	return bufferLen;
}

size_t 
UploadContent::GetMemoryUsage()
{
	return sizeof(*this);
}

bool 
UploadContent::IsInitialized()
{
	return true;
}

void
UploadContent::SetError(ssize_t errCode, const char *errInfo)
{
	fprintf(stderr, "UploadContent::SetError(%ld, '%s')\n", errCode, errInfo);
	BAutolock al(m_lock);
	m_errCode = errCode;
	m_errInfo = errInfo;
	if (m_instance) {
		m_instance->ErrorOccurred();
	}
}

status_t 
UploadContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg)
{
	BAutolock al(m_lock);

	const char *sourceurl = msg.FindString("sourceurl");
	const char *sourceurlfield = msg.FindString("sourceurlfield");
	const char *targeturl = msg.FindString("targeturl");
	if(sourceurl == 0 || sourceurlfield == 0 || targeturl == 0)
		return B_ERROR;
		
	m_targetURL.SetTo(targeturl);
	if(!m_targetURL.IsValid()) {
		printf("UploadContent::CreateInstance: target url is not valid\n");
		return B_ERROR;
	}

	if(strcmp(m_targetURL.GetScheme(), "http") != 0) {
		printf("UploadContent::CreateInstance: http scheme only supported on target url\n");
		return B_ERROR;
	}

	// add the file to the upload URL.
	m_targetURL.AddUploadQueryParameter(sourceurlfield, sourceurl);
	m_targetURL.SetQueryMethod(POST_QUERY);

	m_protocol = Protocol::InstantiateProtocol(m_targetURL.GetScheme());
	if(!m_protocol) {
		printf("UploadContent::CreateInstance: error creating protocol\n");
		return B_ERROR;
	}

	m_instance = new UploadContentInstance(this, handler);
	if(!m_instance) {
		delete m_protocol;
		return B_ERROR;
	}

	// we will have the http procotol call back with progress messages
	m_protocol->SetMetaCallback(m_instance, &UploadContentInstance::HttpCallback);

	BMessage message(UPLOAD_MESSAGE);
	m_instance->PostMessage(message);

	*outInstance = m_instance;

	return B_OK;	
}

status_t
UploadContent::UploadToProtocol()
{
	BAutolock al(m_uploadLock);

	BMessage errmessage;
	// this actually sends the data
	if(m_protocol->Open(m_targetURL, m_targetURL, &errmessage, 0) < 0) {
		if (m_totalBytes > 0 && m_totalBytes != UNKNOWN_LENGTH) {
			// it had connected at least and started transferring
			// XXX clean this up a bit, it will return this error if it
			// gets cancelled by the user, which clearly isn't the case.
			SetError(UPLOAD_IO_ERROR, "Error transferring data to server\n");
		} else {
			// XXX make more error cases. This one isn't technically
			// correct for many cases.
			SetError(UPLOAD_CONNECTION_ERROR, "Error connecting to server\n");
		}
		return B_ERROR;
	}
	return B_OK;
}

void
UploadContent::CancelUpload()
{
	BAutolock al(m_lock);
	// This will kill the http post in progress by closing the socket
	// it had open, which will cause the rest of the transfer to fail
	if(m_protocol)
		m_protocol->Abort();
}

// ----------------------- UploadContentFactory -----------------------

class UploadContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-upload");
	}
	
	virtual Content* CreateContent(void* handle, const char*, const char*)
	{
		return new UploadContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0) return new UploadContentFactory;
	return 0;
}
