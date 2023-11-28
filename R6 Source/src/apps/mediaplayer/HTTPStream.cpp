#include <Debug.h>
#include <Autolock.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <Roster.h>
#include "HTTPStream.h"
#include "URL.h"
#include "debug.h"
#include "MemoryCache.h"
#include "HTTPHeader.h"

const off_t kEOF = 0x7fffffffffffffffLL;
const ssize_t kDownloadWriteSize = 0x10000;
const int32 kMaxHangUps = 5;

struct {
	const char *attr_name;
	const char *pretty_name;
} kHeaderDisplayAttrs[] = {
	{"icy-name", "Name"},
	{"icy-genre", "Genre"},
	{"icy-url", "Home"},
	{"server", "Server"},
	{"meta:protocol", "Protocol"},
	{"content-length", "Length"},
	{"last-modified", "Modified"},
	{"x-audiocast-name", "Name"},				// Icecast 1.3 attrs
	{"x-audiocast-admin", "Admin"},
	{"x-audiocast-url", "Home"},
	{"x-audiocast-location", "Location"},
	{"x-audiocast-description", "Description"},
	{"x-audiocast-genre", "Genre"},
	{0, 0}
};


StreamHandler* HTTPStream::InstantiateHTTPStream(const char*)
{
	return new HTTPStream;
}

HTTPStream::HTTPStream()
	:	fFileCache(0),
		fMemoryCache(0),
		fFileLength(-1),
		fError(-1),
		fRequestedPosition(kEOF),
		fNumHangUps(0),
		fDownloadThread(-1),
		fDownloadPos(0),
		fDownloadedSize(0),
		fClosing(false),
		fMemBuffer(0)
{
	fDataAvailable = create_sem(0, "data_available");
}

HTTPStream::~HTTPStream()
{
	// Stop the download thread
	if (fDownloadThread > 0) {
		fClosing = true;

		send_signal(fDownloadThread, SIGUSR1);
			// Unblock this thread if it is waiting on socket I/O.

		int32 status;
		wait_for_thread(fDownloadThread, &status);
	}

	fSocket.Close();

	delete fMemoryCache;
	fMemoryCache = 0;
	
	delete fFileCache;
	fFileCache = 0;
}

status_t HTTPStream::InitCheck() const
{
	return fError;
}

status_t HTTPStream::SetTo(const URL &requestURL, BString &outError)
{
	ASSERT(fDownloadThread == -1);

	URL url = requestURL;
	HTTPHeader header;
	int resultCode;
	status_t error;
	bool redirect;
	bool retry_with_get = false;

	do {
		redirect = false;
		int port = url.GetPort();
		if (port == 0)
			port = 80;

		writelog("Open connection to %s:%d\n", url.GetHostName(), port);
		error = fSocket.Open(url.GetHostName(), port);
		if (error != 0) {
			if (error < 0) {
				// Regular error
				fError = error;
				outError = strerror(error);
			} else {
				// h_error, returned from gethostbyname()
				outError = "No such host";
				fError = -1;
				error = -1;
			}
	
			return error;
		}

		// Do a head request on this file	
		if (retry_with_get)
			error = fSocket.WriteLine("GET %s HTTP/1.0\r\nConnection: Keep-Alive\r\n\r\n",
									  url.GetEscapedPath());
		else
			error = fSocket.WriteLine("HEAD %s HTTP/1.0\r\nConnection: Keep-Alive\r\n\r\n",
									  url.GetEscapedPath());
		if (error < 0) {
			writelog("Head request failed\n");
			outError = strerror(error);
			return error;
		}
	
		error = header.ParseStream(&fSocket, &resultCode);
		if (error < 0) {
			if (fClosing)
				return B_ERROR;
				
			writelog("Server returned error from HEAD request... I'll just try a GET\n");
			error = 0;
			retry_with_get = true;
			redirect = true;
			continue;
		}

		if (resultCode >= 300 && resultCode <= 399) {
			// This is a redirect, find the new location and
			// go there.
			const char *newLocation = header.FindAttr("location");
			if (newLocation == 0) {
				if (resultCode == 306) {
					// Icecast servers send "306 Grow Up" as an error when
					// sent a HEAD request (its bogus)
					// Check for the set of circumstances under
					// which this happens and handle it.
					const char *server = header.FindAttr("server");
					if (server && strstr(server, "icecast") != 0) {
						fSocket.Reopen();					
						fSocket.WriteLine("\r\n\r\n");
						header.ParseStream(&fSocket, &resultCode);
						break;
					}
				}
			
			
				writelog("Malformed redirect response\n");
				outError = "Malformed redirect response";
				return B_ERROR;
			} else
				writelog("Redirect to %s\n", newLocation);
	
			url.SetTo(newLocation);
			redirect = true;
		}
	} while (redirect);

	fNumHangUps = 0;
	fHostName = url.GetHostName();
	fFilePath = url.GetEscapedPath();
	fUnknownFileSize = false;

	// Check to see if this is shoutcast
	const char *protocol = header.FindAttr("meta:protocol");
	if (protocol && strncmp(protocol, "ICY", 3) == 0) {
		writelog("This is a shoutcast server\n");
		fUnknownFileSize = true;
		fFileLength = 0x7fffffffffffffffLL;
	}

	// Set the name of this "station" if one is specified
	const char *name = header.FindAttr("icy-name");
	if (name)
		fConnectionName = name;

	// Munge out some other attrs
	for (int i = 0; kHeaderDisplayAttrs[i].attr_name; i++) {
		const char *val = header.FindAttr(kHeaderDisplayAttrs[i].attr_name);
		if (val)
			fDescription << kHeaderDisplayAttrs[i].pretty_name << ": "
				<< val << "\n";
	}


#if 0
	if (resultCode == 200) {
		const char *content_type = header.FindAttr("content-type");
		if (content_type && strcmp(content_type, "text/html") == 0) {
			// The server fed us html, probably asking for
			// an email address or something.  Open a browser
			// window so the user knows why this is failing.
			BString urlString;
			urlString << "http://" << hostname << ":" << (int32) port << filepath;
			const char *url_list[] = {urlString.String(), };
			be_roster->Launch(B_URL_HTTP, 1, (char**) url_list);
		}
	}
#endif

	if (resultCode == 200 && !fUnknownFileSize) {
		const char *len = header.FindAttr("content-length");
		if (len == 0) {
			writelog("head request didn't have content-length\n");
			fUnknownFileSize = true;
			fUseByteRanges = false;
			fFileLength = 0x7fffffffffffffffLL;
		} else {
			fFileLength = (off_t) atoi(len);
			fFileCache = new FileCache;
			fFileCache->SetMaxSize(fFileLength);
			fUseByteRanges = false;
			const char *acceptRanges = header.FindAttr("accept-ranges");
			if (acceptRanges && strcmp(acceptRanges, "bytes") == 0) {
				writelog("Server handles byte ranges\n");
				fUseByteRanges = true;	// Server accepts byte ranges
			}
	
			// Sometimes the server will hang up after a HEAD, but will
			// keepalive after byte reads.  reopen and try again.
			if (!fSocket.IsOpen())
				fSocket.Reopen();
		}
	} else {
		// This server doesn't support HEAD, or is shoutcast.  Just request
		// the file and get the header information there
		writelog("Byte range reading disabled\n");
		fUseByteRanges = false;
	}
	
	if (!fUseByteRanges && !fUnknownFileSize) {
		//
		// Submit a normal read request
		//
		if (!fSocket.IsOpen())
			fSocket.Reopen();

		while (true) {
			error = SendRequest();
			if (error < 0)
				if (!HandleSocketError()) {
					outError = strerror(error);
					return error;
				}

			HTTPHeader header;
			int resultCode;
			error = header.ParseStream(&fSocket, &resultCode);
			if (error < 0) {
				if (!HandleSocketError()) {
					outError = strerror(error);
					return error;
				}
					
				continue;
			}		

			if (resultCode != 200)  {
				writelog("GET request failed, giving up\n");
				const char *response = header.FindAttr("meta:error_string");
				if (response) {
					outError = response;
					writelog("Server error: %s\n", response);
				}

				fError = -1;
				return -1;
			}

			// Notice that we find the content length again here,
			// because the HEAD command might have failed.
			const char *len = header.FindAttr("content-length");
			if (len == 0) {
				fUnknownFileSize = true;
				fFileLength = 0x7fffffffffffffffLL;
			} else {
				fFileLength = (off_t) atoi(len);
				fFileCache->SetMaxSize(fFileLength);
			}
			
			break;
		}
	}

	fError = B_OK;
	if (fUnknownFileSize)
		fMemoryCache = new MemoryCache;

	fLastRead = system_time();
	fRawDataRate = 0;
	fDownloadRate = 0;
	
	
	fClosing = false;
	fDownloadThread = spawn_thread(StartDownloadThread, "download_stream", 
		B_NORMAL_PRIORITY, this);
	resume_thread(fDownloadThread);
	
	return B_OK;
}

// This mainly exists to "un-stick" a decoder thread that may be waiting for
// data.
status_t HTTPStream::Unset()
{
	if (fMemoryCache)
		fMemoryCache->Unset();

	delete_sem(fDataAvailable);
	fDataAvailable = -1;

	fError = -1;
	return B_OK;
}

ssize_t HTTPStream::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t sizeRead = -1;
	if (fMemoryCache)
		sizeRead = fMemoryCache->ReadAt(pos, buffer, size);
	else if (fFileCache) {
		if (size == 0)
			return 0;
			
		if (pos < 0)
			return -1;
	
		if (fError != B_OK)
			return fError;
	
		if (pos + size > fFileLength)
			size = fFileLength - pos;
			
		if (size == 0)
			return 0;
	
		BAutolock _lock(&fFileCacheLock);
		if (!IsDataAvailable(pos, size)) {
			fRequestedPosition = pos;
			fRequestedSize = size;
			fFileCacheLock.Unlock();
			if (acquire_sem(fDataAvailable) < 0)
				return B_ERROR;
				
			fFileCacheLock.Lock();
		}
	
		if (!IsDataAvailable(pos, size))
			return -1;
	
		sizeRead = fFileCache->ReadAt(pos, buffer, size);
	}

	bigtime_t now = system_time();
	double currentSample = (double) sizeRead / ((double) (now - fLastRead + 1) /
		1000000.0);

	fRawDataRate = (double) fRawDataRate / 2 + currentSample / 2;
	fLastRead = now;

	return sizeRead;
}


off_t HTTPStream::Seek(off_t position, uint32 seekMode)
{
	if (fError != B_OK)
		return fError;
		
	if (seekMode == SEEK_SET)
		fReadOffset = position;
	else if (seekMode == SEEK_CUR)
		fReadOffset += position;
	else if (fUnknownFileSize)
		return B_DEV_SEEK_ERROR;
	else
		fReadOffset = fFileLength - position;

	return fReadOffset;
}

off_t HTTPStream::Position() const
{
	if (fError != B_OK)
		return fError;

	return fReadOffset;
}

bool HTTPStream::IsDataAvailable(off_t pos, size_t size)
{
	BAutolock _lock(&fFileCacheLock);
	if (fMemoryCache)
		return fMemoryCache->AmountBuffered() >= size;
	else if (fFileCache)
		return fFileCache->IsDataAvailable(pos, size);
	else
		return false;
}

int32 HTTPStream::StartDownloadThread(void *stream)
{
	((HTTPStream*)stream)->DownloadThread();
	return 0;
}

void HTTPStream::DownloadThread()
{
	signal(SIGUSR1, InterruptSocketRead);

	if (fUseByteRanges)
		fChunkBytesToRead = 0;
	
	bigtime_t downloadStart = system_time();
	int totalRead = 0;

	fDownloadPos = 0;
	char *buffer = new char[kDownloadWriteSize + 1];
	ssize_t bufferSize = 0;
	while (!fClosing) {
		if (!fUnknownFileSize && fUseByteRanges && fChunkBytesToRead <= 0) {
			//	Set up a byte range read
			fFileCacheLock.Lock();
			if (fRequestedPosition < fFileLength)
				fDownloadPos = fRequestedPosition;
				
			fDownloadPos = fFileCache->FindNextFreeChunk(fDownloadPos);
			ASSERT(fDownloadPos != fFileLength);
			fFileCacheLock.Unlock();

			// No more to read.
			if (fDownloadPos == -1)
				break;

			ssize_t requestSize = fFileLength - fDownloadPos;
	
			// Read small chunks at first because header data may be scattered.
			if (requestSize > kDownloadWriteSize)
				requestSize = kDownloadWriteSize;

			if (SendRequest(fDownloadPos, fDownloadPos + requestSize) < 0) {
				writelog("Error sending request\n");
				if (!HandleSocketError())
					break;
					
				continue;
			}

			HTTPHeader header;
			int resultCode;
			if (header.ParseStream(&fSocket, &resultCode) < 0) {
				writelog("Error reading response\n");
				if (!HandleSocketError())
					break;
					
				continue;
			}		

			if (resultCode != 206 && resultCode != 200)  {
				writelog("Bad server response\n");
				if (!HandleSocketError())
					break;
					
				continue;
			}

			fChunkBytesToRead = requestSize;
		}

		//
		// Read from socket.  This handles byte range and normal cases.
		//
		size_t sizeToRead = kDownloadWriteSize - bufferSize;
		if (fUseByteRanges && fChunkBytesToRead < sizeToRead)
			sizeToRead = fChunkBytesToRead;

		ASSERT(sizeToRead <= kDownloadWriteSize);
		ASSERT(sizeToRead > 0);			

		ssize_t bytesRead = fSocket.Read(buffer + bufferSize, sizeToRead);
		if (bytesRead <= 0) {
			writelog("Error reading data\n");
			
			// Error, Write out current buffer
			if (bufferSize > 0) {
				if (fMemoryCache)
					fMemoryCache->Write(buffer, bufferSize);
				else if (fFileCache) {
					BAutolock _lock(&fFileCacheLock);
					fFileCache->WriteAt(fDownloadPos, buffer, bufferSize);
					fDownloadPos += bufferSize;
				} else
					debugger("no stream cache set!");
			}
			
			if (fDownloadedSize < fDownloadPos)
				fDownloadedSize = fDownloadPos;
				
			bufferSize = 0;

			if (!fUnknownFileSize && fDownloadPos >= fFileLength)
				break;	// EOF

			if (!HandleSocketError())
				return;
				
			continue;
		}

		totalRead += bytesRead;
		fDownloadRate = (double) totalRead / ((double)(system_time() - downloadStart + 1)
			/ 1000000.0);

		if (fUseByteRanges)
			fChunkBytesToRead -= bytesRead;
	
		bufferSize += bytesRead;
		if (fUnknownFileSize) {
			fMemoryCache->Write(buffer, bufferSize);
			bufferSize = 0;
		} else {
			if (bufferSize >= kDownloadWriteSize || fDownloadPos + bufferSize >= fFileLength) {
				BAutolock _lock(&fFileCacheLock);
				fFileCache->WriteAt(fDownloadPos, buffer, bufferSize);
				fDownloadPos += bufferSize;
				fDownloadedSize += bufferSize;
				bufferSize = 0;
				if (fRequestedPosition < fFileLength &&
					IsDataAvailable(fRequestedPosition, fRequestedSize)) {
					fRequestedPosition = kEOF;
					release_sem(fDataAvailable);
				}
			}
		}
	}

	delete [] buffer;
	delete_sem(fDataAvailable);
}

off_t HTTPStream::GetDownloadedSize() const
{
	return fDownloadedSize;
}

size_t HTTPStream::GetLength() const
{
	return fFileLength;
}

bool HTTPStream::HandleSocketError()
{
	if (fClosing)
		return false;	

	writelog("Attempting to recover from socket error #%d\n", fNumHangUps + 1);
	while (true) {
		if (fNumHangUps > kMaxHangUps) {
			writelog("That's it, I can't take any more rejection.  Give up\n");
			return false;
		}
			
		if (fSocket.Reopen() != B_OK) {
			fNumHangUps++;
			continue;
		}

		// Revert to normal reading if this fails, as the server
		// may not support keepalive.  This could probably be
		// made smarter at some point.
		if (fUseByteRanges) {
			writelog("Disabling byte range reading\n");
			fUseByteRanges = false;
		}

		if (fUnknownFileSize)
			fSocket.WriteLine("\r\n\r\n");
		else {
			if (SendRequest() < 0) {
				fNumHangUps++;
				continue;
			}
		}

		HTTPHeader header;
		int resultCode;
		status_t error = header.ParseStream(&fSocket, &resultCode);
		if (error < 0)  {
			writelog("got error trying to get response\n");
			fNumHangUps++;
			continue;
		}
		
		// These indicate internal server errors.  Retry.
		if (resultCode >= 500 && resultCode < 505) {
			writelog("The server is apparently having some problems.  Try again...\n");
			fNumHangUps++;
			continue;
		}

		if (resultCode != 200) {
			header.PrintToStream();
			writelog("server returned error %d\n", resultCode);
			fError = -1;			
			return false;
		}

		fDownloadPos = 0;
		break;			
	}		

	return true;
}

void HTTPStream::InterruptSocketRead(int)
{
	// Yes, this is necessary.
}

void HTTPStream::SetCookies(const char *cookies)
{
	fCookieString = cookies;
}


status_t HTTPStream::SendRequest(off_t start, off_t size)
{
	BString requestString;
	requestString << "GET " << fFilePath << " ";
	if (fUseByteRanges) {
		requestString
			<< "HTTP/1.1\r\n"
			<< "Host: " << fHostName << "\r\n"
			<< "Range: bytes=" << start << "-" << size << "\r\n"
			<< "Connection: Keep-Alive\r\n";
	} else
		requestString << "HTTP/1.0\r\n";
	
	requestString << "User-Agent: Mozilla/3.0 (compatible; BeOS MediaPlayer)\r\n";
	if (fCookieString.Length() > 0)
		requestString << fCookieString;
		
	requestString << "\r\n";
	writelog("C> %s\n", requestString.String());
	return fSocket.Write(requestString.String(), requestString.Length());	
}

bool HTTPStream::IsContinuous() const
{
	return fUnknownFileSize;
}

bool HTTPStream::IsBuffered() const
{
	return (fMemoryCache != 0);
}

const char* HTTPStream::GetConnectionName()
{
	return fConnectionName.String();
}

void HTTPStream::DescribeConnection(BString &outString)
{
	outString << fDescription;
}

float HTTPStream::BufferUtilization() const
{
	if (fMemoryCache)
		return (float) fMemoryCache->AmountBuffered() / (float) fMemoryCache->Size();

	return 1.0;
}

void HTTPStream::GetStats(player_stats *stats)
{
	stats->is_network_stream = true;
	stats->connection_rate = fDownloadRate;
	stats->raw_data_rate = fRawDataRate;
}


