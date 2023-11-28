#include "ConnectedIO.h"
#include <ResourceCache.h>
#include <PasswordManager.h>


#include <string.h>
#include <stdio.h>

ConnectedIO::ConnectedIO(const URL &url, const URL &refer) :
	fURL(url), fRefer(refer), fProtocol(NULL), fPos(0LL)
{
//	printf("ConnectedIO created\n");
}


ConnectedIO::~ConnectedIO()
{
//	printf("ConnectedIO::~ConnectedIO()\n");
	Cleanup();
}

ssize_t 
ConnectedIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (!fProtocol) {
		ssize_t ret = 0L;
		ret = Connect();
		if (ret < B_OK) {
			return ret;
		}
	}
	return fProtocol->ReadAt(pos, buffer, size);
}

ssize_t 
ConnectedIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (!fProtocol) {
		ssize_t ret = 0L;
		ret = Connect();
		if (ret < B_OK)
			return ret;
	}

	return fProtocol->WriteAt(pos, buffer, size);
}

off_t 
ConnectedIO::Seek(off_t position, uint32 seek_mode)
{
	if (seek_mode == SEEK_SET)
		fPos = position;
	else if (seek_mode == SEEK_END)
		fPos = fSize + position;
	else if (seek_mode == SEEK_CUR)
		fPos += position;
	return fPos;
}

off_t 
ConnectedIO::Position() const
{
	return fPos;
}

status_t 
ConnectedIO::SetSize(off_t size)
{
	return B_ERROR;
}

ssize_t 
ConnectedIO::Connect()
{
//	printf("ConnectedIO::Connect()\n");
	status_t status = B_OK;
	const char *scheme = fURL.GetScheme();
	
	while (fProtocol == NULL) {
		fProtocol = Protocol::InstantiateProtocol(scheme);
		if (fProtocol == NULL)
			break;
		
		BMessage msg;
		status = fProtocol->Open(fURL, fRefer, &msg, PRIVATE_COPY | RANDOM_ACCESS);
		if (status == B_AUTHENTICATION_ERROR) {
			const char *challenge = "";
			msg.FindString(S_CHALLENGE_STRING, &challenge);
			BString user, password;
			
			if (passwordManager.GetPassword(fURL.GetHostName(), challenge, &user, &password)) {
				fURL.SetTo(fURL.GetScheme(), fURL.GetHostName(), fURL.GetPath(),
					fURL.GetPort(), fURL.GetFragment(), user.String(), password.String(),
					fURL.GetQuery(), fURL.GetQueryMethod());
					
				delete fProtocol; fProtocol = 0;
				msg.MakeEmpty();
				continue;
			}
		}
		
		if (status < B_OK) {
			delete fProtocol; fProtocol = 0;
			break;
		}	

		bigtime_t delay;
		if (fProtocol->GetRedirectURL(fURL, &delay)) {
			delete fProtocol; fProtocol = 0;
			continue;
		}
	}
	
	if (fProtocol) {
		ssize_t contentLen = contentLen = fProtocol->GetContentLength();
		if (contentLen == 0x7FFFFFFFL)
			contentLen = (size_t)(fProtocol->Seek(0, SEEK_END));
		
		fSize = contentLen;
		status = contentLen;
	}
	return status;
}

void 
ConnectedIO::Acquired()
{
//	printf("ConnectedIO Acquired!\n");
	if (!fProtocol) {
		Connect();
	}
}

void 
ConnectedIO::Cleanup()
{
//	printf("ConnectedIO::Cleanup()\n");
	delete fProtocol;
	fProtocol = NULL;
}

const char *
ConnectedIO::Scheme() const
{
	return fURL.GetScheme();
}

void 
ConnectedIO::_delete()
{
}


