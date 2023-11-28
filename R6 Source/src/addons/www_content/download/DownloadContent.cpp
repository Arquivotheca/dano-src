//******************************************************************************
//
//	File:			DownloadContent.cpp
//
//	Copyright 2000, Be Incorporated, All Rights Reserved.
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

#include <CursorManager.h>

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
	DOWNLOAD_NO_ERROR				=  0,
	DOWNLOAD_NAME_CONFLICT_ERROR	= -1,
	DOWNLOAD_NO_SPACE_ERROR			= -2,
	DOWNLOAD_OTHER_CREATE_ERROR		= -3,
	DOWNLOAD_READ_IO_ERROR			= -4,
	DOWNLOAD_WRITE_IO_ERROR			= -5,
	DOWNLOAD_CANCELLED_ERROR		= -6,
	DOWNLOAD_GENERAL_ERROR			= -7,
	DOWNLOAD_URL_INVALID_ERROR		= -8,
	DOWNLOAD_PROTOCOL_CREATE_ERROR	= -9,
	DOWNLOAD_PROTOCOL_OPEN_ERROR	= -10
} download_error_codes;

const ssize_t BUFSIZE = 16384;	// It is optimal to write to CFS in 16K blocks

const size_t UNKNOWN_LENGTH = 0x7fffffffL;	// this is a special value indicating unknown length in libwww

const int32 DOWNLOAD_MESSAGE = 'dnld';
const int32 UPDATE_BANDWIDTH = 'updt';

const char * DOWNLOAD_BUSY_CURSOR_NAME = "busy_web_";

// NOTE: these properties need to stay in alphabetical order.  If a new property
// is inserted in this list, code to handle it should be added to ReadProperty().
static const char*	properties[] = {
						"bytesCompleted",
						"bytesPerSecond",
						"downloadLocation",
						"errCode",
						"errInfo",
						"fractionCompleted",
						"sourceURL",
						"totalBytes"
};

static const int32	numProperties = (sizeof(properties) / sizeof(properties[0]));

class DownloadContentInstance : public ContentInstance, public BinderNode {
public:
							DownloadContentInstance(Content *content, GHandler *handler);
	virtual 				~DownloadContentInstance();
	
			void			ChangedBytecount();
			void			ErrorOccurred();

	virtual	void			Cleanup();
	
protected:

	friend	class			DownloadContent;

	struct store_cookie {
		int32 index;
	};

	virtual	status_t		OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t		NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t		CloseProperties(void *cookie);
	virtual	put_status_t	WriteProperty(const char *name, const property &prop);
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	virtual status_t        HandleMessage(BMessage *msg);


};

class DownloadContent : public Content {
	
	char					m_writeBuf[BUFSIZE];
	BString					m_filename;
	BString					m_errInfo;
	BLocker					m_lock;
	BLocker					m_protocolReadLock;
	DownloadContentInstance *m_instance;
	int						m_fd;
	ssize_t					m_writeOffset;
	ssize_t					m_byteCount;
	ssize_t					m_totalBytes;
	ssize_t					m_errCode;
	bigtime_t				m_startedAt;
	bigtime_t				m_finishedAt;
	Protocol				*m_protocol;
	bool					m_resume;
	bool					m_downloadComplete;

	bool							m_useBusyCursor;
	BCursorManager::cursor_token 	m_busyCursorToken;
	BCursorManager::queue_token		m_busyQueueToken;
	BLocker							m_cursorLock;

public:
							DownloadContent(void *handle);
	virtual 				~DownloadContent();

	virtual ssize_t			Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	virtual size_t			GetMemoryUsage();
	virtual	bool			IsInitialized();
	
private:
			status_t		Open(const char *filePath, bool overwrite, bool resume);
			void			Close();
			ssize_t			BufferedWrite(const void *buffer, ssize_t bufferLen);
			void			Flush();
			void			CancelDownload();	// called when the user cancels the download
			void			Abort();			// cleans up
			void			SetError(ssize_t errCode, const char *errInfo);
			
			void			InitCursor();
			void			SetBusyCursor(bool busy);
			
	virtual status_t		CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);
			status_t		DownloadFromProtocol();
	
	friend class DownloadContentInstance;
};


DownloadContentInstance::DownloadContentInstance(Content *content, GHandler *handler)
	: ContentInstance(content,handler)
{
}

DownloadContentInstance::~DownloadContentInstance()
{
}

void 
DownloadContentInstance::Cleanup()
{
	BinderNode::Cleanup();
	ContentInstance::Cleanup();
}

put_status_t 
DownloadContentInstance::WriteProperty(const char */*name*/, const property &/*prop*/)
{
	return B_ERROR;
}

void
DownloadContentInstance::ChangedBytecount()
{
	NotifyListeners(B_PROPERTY_CHANGED,"bytesCompleted");
//	NotifyListeners(B_PROPERTY_CHANGED,"bytesPerSecond");
	NotifyListeners(B_PROPERTY_CHANGED,"fractionCompleted");
}

void
DownloadContentInstance::ErrorOccurred()
{
	NotifyListeners(B_PROPERTY_CHANGED,"errCode");
	NotifyListeners(B_PROPERTY_CHANGED,"errInfo");
}


status_t DownloadContentInstance::OpenProperties(void **cookie, void *copyCookie)
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

status_t DownloadContentInstance::NextProperty(void *cookie, char *nameBuf, int32 *len)
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

status_t DownloadContentInstance::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

get_status_t 
DownloadContentInstance::ReadProperty(const char *name, property &prop, const property_list &args)
{
	DownloadContent *c = dynamic_cast<DownloadContent*>(GetContent());
	if (c == NULL)
		return B_ERROR;
	
	BAutolock(c->m_lock);

	if (!strcmp(name,"bytesCompleted")) {
		prop = (double)c->m_byteCount;
	} else if (!strcmp(name,"bytesPerSecond")) {
		bigtime_t elapsed = c->m_finishedAt;
		if (elapsed == -1) elapsed = system_time();
		elapsed -= c->m_startedAt;
		prop = ((double)c->m_byteCount) / (((double)elapsed) / 1000000.0);
	} else if (!strcmp(name,"cancelDownload")) {
		if (args.CountItems() == 0) {
			c->CancelDownload();
		}
	} else if (!strcmp(name,"downloadLocation")) {
		prop = c->m_filename;
	} else if (!strcmp(name,"errCode")) {
		prop = (double)c->m_errCode;
	} else if (!strcmp(name,"errInfo")) {
		prop = c->m_errInfo.String();
	} else if (!strcmp(name,"fractionCompleted")) {
		size_t len = c->m_totalBytes;
		if (len > 0)	prop = ((double)c->m_byteCount) / len;
		else			prop = property::undefined;
	} else if (!strcmp(name,"sourceURL")) {
		char buf[4096];
		c->GetResource()->GetURL().GetString(buf, sizeof(buf));
		prop = buf;
	} else if (!strcmp(name,"totalBytes")) {
		size_t len = c->m_totalBytes;
		if (len > 0)	prop = (double)len;
		else			prop = property::undefined;
	} else {
		return ENOENT;
	}
	
	return B_OK;
}

status_t
DownloadContentInstance::HandleMessage(BMessage *msg)
{
	DownloadContent *c = dynamic_cast<DownloadContent*>(GetContent());
	if (c == NULL)
		return B_ERROR;

	switch(msg->what) {
		case DOWNLOAD_MESSAGE: {
			ResumeScheduling();
			c->DownloadFromProtocol();
			break;
		}
		case UPDATE_BANDWIDTH: {
			if (c->m_finishedAt==-1) {
				NotifyListeners(B_PROPERTY_CHANGED,"bytesPerSecond");
				PostDelayedMessage(*msg,1000000);
			}
		} break;
		default:
			break;
	}
	return B_OK;
}

DownloadContent::DownloadContent(void *handle)
	: Content(handle),
	  m_filename(B_EMPTY_STRING),
	  m_errInfo(B_EMPTY_STRING),
	  m_lock("DownloadContent lock"),
	  m_protocolReadLock("DownloadContent read loop lock"),
	  m_instance(NULL),
	  m_fd(-1),
	  m_writeOffset(0),
	  m_byteCount(0),
	  m_totalBytes(0),
	  m_errCode(DOWNLOAD_NO_ERROR),
	  m_startedAt(-1),
	  m_finishedAt(-1),
	  m_protocol(NULL),
	  m_resume(false),
	  m_downloadComplete(false),
	  m_useBusyCursor(false),
	  m_busyCursorToken(0),
	  m_busyQueueToken(0)
{
	InitCursor();
}

DownloadContent::~DownloadContent()
{
	if(m_protocol)
		delete m_protocol;
	Close();
}

ssize_t 
DownloadContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	BAutolock al(m_lock);
	
//	PRINT(( "DownloadContent::Feed(buffer, bufferlen=%ld, done=%s)\n",
//		bufferLen, (done?"true":"false") ));

	if (m_errCode != DOWNLOAD_NO_ERROR) {
		// this download has been cancelled, so tell libwww to stop the Feed()ing
		return B_FINISH_STREAM;
	}

	if (done) {
//		m_finishedAt = system_time();
//		if (!bufferLen && !m_byteCount) SetError(DOWNLOAD_PROTOCOL_OPEN_ERROR,"No data available");
		if (m_downloadComplete) {
			m_finishedAt = system_time();
			if (!bufferLen && !m_byteCount) SetError(DOWNLOAD_PROTOCOL_OPEN_ERROR,"No data available");
		} else {
			// no-op.  The Feed() call has been sent from the
			// "source" of the ContentInstance invocation, which
			// is the beos protocol, as in
			//     SRC="beos:///?signature=application/x-download"
			// done==true here, but we know we're not really done.
			PRINT(("Feed(): called with done=true, assuming this was the beos protocol and ignoring\n"));
		}
	}

	ssize_t written = -1;
	if (bufferLen > 0) {
		written = BufferedWrite(buffer, bufferLen);
		if (written > 0) {
			m_byteCount += written;
			if (m_instance) {
				m_instance->ChangedBytecount();
			}
		}
	}

	if (done && m_downloadComplete) {
		if (m_instance) m_instance->NotifyListeners(B_PROPERTY_CHANGED,"bytesPerSecond");

		Close();	// will Flush() if needed

		bool updateByteCount = false;
		if (((size_t)m_totalBytes) == UNKNOWN_LENGTH) {
			// length was unknown, but now we're done and we know
			// how much data we received, so the length is known
			m_totalBytes = m_byteCount;
			updateByteCount = true;
		} else if (m_byteCount < m_totalBytes) {
			// length was known, but we received less than expected
			SetError(DOWNLOAD_READ_IO_ERROR, "Less data was downloaded than was expected");
			updateByteCount = true;
		}
		
		if (updateByteCount && m_instance) {
			m_instance->ChangedBytecount();
		}			
	}
	
	return bufferLen;
}

size_t 
DownloadContent::GetMemoryUsage()
{
	return sizeof(*this);
}

bool 
DownloadContent::IsInitialized()
{
	return true;
}

status_t
DownloadContent::Open(const char *filePath, bool overwrite, bool resume)
{
	PRINT(("DownloadContent::Open(filePath=\"%s\", overwrite=%s, resume=%s)\n", filePath, (overwrite?"true":"false"), (resume?"true":"false")));
	BAutolock al(m_lock);
	status_t err = B_ERROR;
	m_filename = filePath;
	BEntry entry(filePath);
	bool exists = entry.Exists();
	BDirectory parent;
	BVolume vol;
	off_t bytesFree;
	off_t replaceSize;
	int openflags;
	
	if (!overwrite && !resume && exists) {
		// the pathname already exists
		SetError(DOWNLOAD_NAME_CONFLICT_ERROR, "Filename already exists");
		goto done;
	}
			
	if (exists && !entry.IsFile()) {
		// cannot overwrite anything except for files (not directories, symlinks, etc)
		SetError(DOWNLOAD_NAME_CONFLICT_ERROR, "Cannot overwrite entry at the specified path"); 
		goto done;
	}

	if ((entry.GetParent(&parent) != B_OK) || (parent.GetVolume(&vol) != B_OK)) {
		// is this the correct error message?
		SetError(DOWNLOAD_OTHER_CREATE_ERROR, "Destination directory does not exist");
		goto done;
	}
	
	// check for read-only
	if (vol.IsReadOnly()) {
		SetError(DOWNLOAD_NO_SPACE_ERROR, "Cannot write to a read-only storage device");
		goto done;
	}
		
	// check for space
	bytesFree = vol.FreeBytes();
	replaceSize = 0;
	if (exists) {
		entry.GetSize(&replaceSize);
	}
				
	if ((((size_t)m_totalBytes) != UNKNOWN_LENGTH) && (bytesFree + replaceSize < m_totalBytes)) {
		// not enough space on volume to download this file
		SetError(DOWNLOAD_NO_SPACE_ERROR, "Insufficient space available on storage device"); 
		goto done;
	}
				
	// either the source length is not known or there is enough
	// space to write this file to this volume.  In either case,
	// we should open the file and start the download
	fprintf(stderr, "DownloadContent::Open: opening local file '%s' for writing\n", m_filename.String());
	errno = 0;
	// we can finally open/create the file now
	openflags = O_CREAT|O_RDWR;
	if(overwrite)
		openflags |= O_TRUNC;

	m_fd = open(m_filename.String(), openflags, 0666);
	if (m_fd < 0) {
		SetError(DOWNLOAD_OTHER_CREATE_ERROR, strerror(errno)); // error creating file
	} else {
		err = B_OK;	// Success!
	}

done:	
	return err;		
}

void
DownloadContent::Close()
{
	PRINT(("DownloadContent::Close()\n"));
	BAutolock al(m_lock);
	if (m_fd >= 0) {
		if (m_errCode == DOWNLOAD_NO_ERROR) {
			Flush();
		}
		close(m_fd);
		m_fd = -1;
	}
}

ssize_t
DownloadContent::BufferedWrite(const void *buffer, ssize_t bufferLen)
{
	// PRINT(("DownloadContent::BufferedWrite(buffer, bufferLen=%ld)\n", bufferLen));
	BAutolock al(m_lock);
	ssize_t err,startOffset=0,bytesAvailable,count,written = bufferLen;
	
	while (bufferLen) {
		count = bufferLen;
		bytesAvailable = BUFSIZE - m_writeOffset;
		if (count > bytesAvailable) count = bytesAvailable;
		memcpy((void *)(m_writeBuf + m_writeOffset), ((char*)buffer)+startOffset, count);
		m_writeOffset += count;
		startOffset += count;
		bufferLen -= count;
		if (m_writeOffset == BUFSIZE) {
			while (((err = write(m_fd, (void *)m_writeBuf, m_writeOffset)) < 0) && (errno == EINTR));
			m_writeOffset = 0;
			if (err < 0) {
				int write_errno = errno;
				
				BString error_string(strerror(write_errno));
				DEBUG_ONLY( error_string << " (" << write_errno << ") in BufferedWrite"; )
				
				if (m_fd < 0) {
					SetError(DOWNLOAD_WRITE_IO_ERROR, error_string.String());
				} else {
					switch (errno) {
					  case EIO:
						SetError(DOWNLOAD_WRITE_IO_ERROR, error_string.String());
						break;
									  	
					  case ENOSPC:
						SetError(DOWNLOAD_NO_SPACE_ERROR, error_string.String());
						break;
					  
					  default:
						SetError(DOWNLOAD_GENERAL_ERROR, error_string.String());
						break;
					}
				}
	
				Abort();
				written = -1;
				break;
			}
		}
	}

	return written;
}

void
DownloadContent::Flush()
{
	BAutolock al(m_lock);
	if ((m_writeOffset > 0) && (m_fd >= 0)) {
		write(m_fd, (void *)m_writeBuf, m_writeOffset);
		m_writeOffset = 0;
	}
}

void
DownloadContent::CancelDownload()
{
	BAutolock al(m_lock);
	if (m_errCode == DOWNLOAD_NO_ERROR) {
		SetError(DOWNLOAD_CANCELLED_ERROR, "Download cancelled");
	}
	Abort();
}

void
DownloadContent::Abort()
{
	BAutolock al(m_lock);
	m_writeOffset = 0;

	m_downloadComplete = true; // nobody's saying that the download was *successful*.
	
	if (m_errCode == DOWNLOAD_NO_ERROR) {
		SetError(DOWNLOAD_GENERAL_ERROR, "Unknown error");
	}
	
	Close();
	if (m_instance) {
		m_instance->ChangedBytecount();
	}
}

void
DownloadContent::SetError(ssize_t errCode, const char *errInfo)
{
	fprintf(stderr, "DownloadContent::SetError(%ld, '%s')\n", errCode, errInfo);
	BAutolock al(m_lock);
	
	m_errCode = errCode;
	m_errInfo = errInfo;
	if (m_instance) {
		m_instance->ErrorOccurred();
	}
}

void
DownloadContent::InitCursor()
{
	if (m_busyCursorToken == 0) {
		BCursorManager::cursor_data data(DOWNLOAD_BUSY_CURSOR_NAME /*name*/, 1 /*hotspotX*/, 6 /*hotspotY*/);
			// busy_web_XX => base name of Wagner busy cursor loop
		cursorManager.GetCursorToken(&data, &m_busyCursorToken);
	}
}

void
DownloadContent::SetBusyCursor(bool busy)
{
	if (!m_useBusyCursor) return;
	
	BAutolock cl(m_cursorLock);
	
	PRINT(( "DownloadContent::SetBusyCursor: <%s>\n", (busy?"BUSY":"IDLE") ));
	if (busy) {
		if (m_busyQueueToken && m_busyCursorToken) {
			cursorManager.SetCursor(m_busyCursorToken, 1, &m_busyQueueToken);
		}
	} else {
		if (m_busyQueueToken != 0) {
			cursorManager.RemoveCursor(m_busyQueueToken);
			m_busyQueueToken = 0;
		}
	}
}


status_t
DownloadContent::DownloadFromProtocol()
{
	//PRINT(( "DownloadContent::DownloadFromProtocol\n" ));
	char buf[32 * 1024];
	BAutolock al(m_protocolReadLock);
	
	SetBusyCursor(true);

	if(m_fd < 0 || m_protocol == NULL)
		return B_ERROR;

	off_t filePos = 0;
	if(m_resume) {
		struct stat stat;	
		if(fstat(m_fd, &stat) < 0)
			return B_ERROR;

		filePos = stat.st_size;
	
		if(m_protocol->Seek(filePos, SEEK_SET) != filePos) {
			// could not seek this protocol, probably doesn't support it
			// start over from the start
			filePos = 0;
			ftruncate(m_fd, 0);
		}

		lseek(m_fd, filePos, SEEK_SET);
	}

	bool done = false;
	while(!done) {
		ssize_t amountRead = m_protocol->Read(buf, sizeof(buf));
		// PRINT(( "DownloadContent::DownloadFromProtocol: amountRead=%ld\n", amountRead ));
		if(amountRead <= 0) {
			done = true;
			break;
		}

		ssize_t amountWritten = Feed(buf, amountRead, false);
		// PRINT(( "DownloadContent::DownloadFromProtocol: amountWritten=%ld\n", amountWritten ));
		if(amountWritten == B_FINISH_STREAM) {
			m_protocol->Abort();
			done = true;
			break;
		}

		if(amountWritten <= 0 || amountWritten != amountRead) {
			// Feed should take whatever we feed it
			done = true;
			break;
		}

		filePos += amountWritten;
	}

	// We're done
	m_downloadComplete = true;
	PRINT(( "DownloadContent::DownloadFromProtocol: calling Feed() with done=true to complete\n" ));
	Feed(buf, 0, true);
	
	SetBusyCursor(false);

	return B_OK;
}

status_t 
DownloadContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage &msg)
{
	PRINT(("DownloadContent::CreateInstance()\n"));
	BAutolock al(m_lock);
	const char *fn = msg.FindString("filename");
	const char *ow = msg.FindString("overwrite");
	bool overwrite = false;
	if (ow && (!strcasecmp(ow, "true") || !strcasecmp(ow, "1"))) {
		overwrite = true;
	}

	// we need to always create an instance, even if no downloading could ever
	// happen because the URL is invalid, etc, because the instance is a BinderNode,
	// and is used to report any errors that occur to the embedding page/script.
	m_instance = new DownloadContentInstance(this, handler);
	if(!m_instance) {
		return B_ERROR;
	}

	const char *useCursor = msg.FindString("busycursor");
	if (useCursor)
		m_useBusyCursor = (strcasecmp(useCursor, "true") == 0);

	const char *sourceurl = msg.FindString("sourceurl");
	if(sourceurl) {
		const char *res = msg.FindString("resume");
		m_resume = false;
		if (res && (!strcasecmp(res, "true") || !strcasecmp(res, "1"))) {
			m_resume = true;
		}

		URL url(sourceurl, false);
		if(!url.IsValid()) {
			SetError(DOWNLOAD_URL_INVALID_ERROR, "Download URL is not valid");
			goto finished;
		}

		SetBusyCursor(true);
			// animate cursor while attempting to connect.
		m_protocol = Protocol::InstantiateProtocol(url.GetScheme());
		SetBusyCursor(false);
			// we now know whether we've got a connection -- restore the
			// cursor until it's time to read data
			
		if(!m_protocol) {
			BString errStr("Unable to create protocol of type '");
			errStr << url.GetScheme() << "'";
			SetError(DOWNLOAD_PROTOCOL_CREATE_ERROR, errStr.String());
			goto finished;
		}
		
		ssize_t err;
		BMessage errmessage;
		if((err = m_protocol->Open(url, url, &errmessage,
									(m_resume ? RANDOM_ACCESS : 0))) < 0)
		{
			// XXX: should handle err == B_AUTHENTICATION_ERROR here
			m_protocol->Abort();
			delete m_protocol;
			m_protocol = NULL;

			BString errStr("Error opening URL");
			const char *protocolErr = errmessage.FindString(S_ERROR_MESSAGE);
			if (protocolErr) {
				errStr << ": " << protocolErr;
			}
			SetError(DOWNLOAD_PROTOCOL_OPEN_ERROR, errStr.String());
			goto finished;
		}
		
		m_startedAt = system_time();
		m_totalBytes = m_protocol->GetContentLength();
		
		if (fn && (Open(fn, overwrite, m_resume) != B_OK)) {
			// we don't need to call SetError here because Open() will set it appropriately.
			delete m_protocol;
			m_protocol = NULL;
			goto finished;
		}

		BMessage message(DOWNLOAD_MESSAGE);
		m_instance->PostMessage(message);
	} else {
		printf("DownloadContent: warning: using deprecated API \n"
		       "    Deprecated:  SRC=<url> TYPE=\"application/x-download\"\n"
		       "    Recommended: SRC=\"beos:///?signature=application/x-download\" \n"
		       "                 SOURCEURL=<url>\n");
		m_startedAt = system_time();
		m_totalBytes = GetResource()->GetContentLength();
		if (fn) {
			Open(fn, overwrite, false);
		}
	}

	m_instance->PostDelayedMessage(BMessage(UPDATE_BANDWIDTH),1000000);

finished:
	*outInstance = m_instance;
		// nota bene: the above needs to be after the finished: label
		// to ensure that the contentinstance gets created even in
		// case of an error.
		
	return B_OK;
}

// ----------------------- DownloadContentFactory -----------------------

class DownloadContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "application/x-download");
	}
	
	virtual Content* CreateContent(void* handle, const char*, const char*)
	{
		return new DownloadContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id /*you*/, uint32 /*flags*/, ...)
{
	if (n == 0) return new DownloadContentFactory;
	return 0;
}
