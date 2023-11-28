/*
 * FtpProtocol - a Wagner Protocol object for handling FTP connections
 *
 * This class borrows liberally from Howard Berkey's FtpClient sample code.
 *
 *	To do:
 *		- Once persistent connections are in place, it would be nice to show server
 *		  message text on the first directory listing page of the connection.
 *
 *		- Clean up the error messages returned so that there is no English text
 *		  explicitly placed into the error messages, and that there is a ftp-specific
 *		  error template in place.  The current implementation uses the http.html error
 *		  template in a hackish way.
 *
 *		- Change the directory listing mechanism to allow customization of the listing
 *		  page.  One way to allow this customization would be to output an array of Javascript
 *		  data containing the listing and include a script that would output the data for display
 *		  to the user.
 *
 *		- Add folder/file icons to the directory listings.  This could be accomplished using the
 *		  Javascript listing technique described above if the data about the listing contained
 *		  the type of each list item (file or folder)
 *
 *
 *
 *	RFC: http://www.nic.mil/ftp/rfc/rfc959.txt
 */

#include "FtpProtocol.h"
#include "ftpparse.h"
#include "FtpConnectionPool.h"
#include "FtpConnection.h"
#include <NetworkKit.h>
#include <DataIO.h>
#include <DNSCache.h>
#include <ContentView.h>	//for USER_ACTION

#include <time.h>

enum {
	URL_LENGTH = 2048
};

static FtpConnectionPool connectionPool;

// ----------------------- FtpProtocolFactory -----------------------

FtpProtocol::FtpProtocol(void* handle)
	: Protocol(handle)
{
	fContentLength = 0;
	fAmountRead = 0;
	fFlags = ftp_passive; // passive mode is enabled by default
	fControlConn = NULL;
	fDataConn = NULL;
	fKeepAliveTimeout = B_INFINITE_TIMEOUT;
	fMaxKeepAliveRequests = 0x7fffffff;
	fMallocIO = NULL;
	fIsDirectory = false;
	FTP_TRACE(("New FtpProtocol %08X created by thread %ld.\n", reinterpret_cast<unsigned int>(this), find_thread(NULL)));
}

FtpProtocol::~FtpProtocol()
{
	FTP_TRACE(("FtpProtocol %08X, thread %ld: Beginning of ~FtpProtocol().\n", reinterpret_cast<unsigned int>(this), find_thread(NULL)));
	CloseDataConnection();
	ReturnControlConnectionToPool();
	delete fMallocIO;
	FTP_TRACE(("FtpProtocol %08X deleted in thread %ld.\n", reinterpret_cast<unsigned int>(this), find_thread(NULL)));
}

int FtpProtocol::CloseDataConnection()
{
	int code = 0;

	if (fControlConn && fDataConn) {
		BString command;

		command.SetTo("ABOR");
		if (fControlConn->SendRequest(command) >= B_OK) {
			BString replyStr;
			int codeType = 0;
			status_t rc = B_OK;
			
			DeleteDataConnection();
			
			// Get replies until we get a 2 (Aborted OK) or 5
			// (permanent error).
			do {
				rc = GetReply(&replyStr, &code, &codeType);
			} while (codeType != 2 && codeType != 5 && rc >= B_OK);
		}
	}
	
	if (fDataConn) {
		DeleteDataConnection();
	}
	
	return code;
}

void FtpProtocol::DeleteDataConnection()
{
	delete fDataConn;
	fDataConn = NULL;
}

status_t FtpProtocol::OpenRW(const URL &url, const URL &ignored, BMessage* outErrorParams,
	uint32 flags, bool no_list)
{
	return OpenCommon(url, ignored, outErrorParams, flags, true, no_list);
}

status_t FtpProtocol::Open(const URL &url, const URL &ignored, BMessage* outErrorParams,
	uint32 flags)
{
	return OpenCommon(url, ignored, outErrorParams, flags);
}

status_t FtpProtocol::OpenCommon(const URL &url, const URL&, BMessage* outErrorParams,
	uint32 flags, bool no_html, bool no_list)
{
	int code = 0;
	status_t err;
	
	//fprintf(stderr, "FtpProtocol::OpenURL():\n");
	if ((err = Connect(url, code, (flags & USER_ACTION))) == B_OK) {
		//fprintf(stderr, "Connect() succeeded\n");
		// now we're connected and logged in, so let's find out what is referenced
		// by the path in the URL
		BString path(B_EMPTY_STRING, URL_LENGTH);
		char *pathPtr = path.LockBuffer(URL_LENGTH);
		url.GetUnescapedPath(pathPtr, URL_LENGTH);
		path.UnlockBuffer();

		if (SetCurrentDir(path)) {
			// it's a directory, so we should list it
			fIsDirectory = true;
			//fprintf(stderr, "\tlisting directory %s\n", path.String());
			BString listing;
			if (no_list)
			{ }
			else if (DoDirListing(listing)) {
				if (no_html)
					GenerateDirListing(listing, url);
				else
					GenerateHTMLDirListing(listing, url);
				if (fMallocIO != NULL) {
					fContentLength = fMallocIO->BufferLength();
					fAmountRead = 0;
				}
			} else {
				err = B_ERROR;
				outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/http.html");	
				BString errMsg("Directory listing for ");
				errMsg.Append(path);
				errMsg.Append(" failed.");
				outErrorParams->AddString("http_message", errMsg.String());
				outErrorParams->AddString("result_code", "404");
			}
		} else {
			// it's not a directory, try to fetch it
			fIsDirectory = false;
			//strncpy(outContentType, "text/plain", B_MIME_TYPE_LENGTH);
			//fprintf(stderr, "\trequesting file %s\n", path.String());
			if (!RequestFile(path)) {
				err = B_ERROR;
				outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/http.html");			
				outErrorParams->AddString("result_code", "404");			
				BString errMsg("File '");
				errMsg.Append(path);
				errMsg.Append("' not found.");
				outErrorParams->AddString("http_message", errMsg.String());
			}
		}
	} else {
		if (err == B_NAME_NOT_FOUND) {
			outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/hostunknown.html");
			outErrorParams->AddString(S_ERROR_MESSAGE, strerror(err));
			outErrorParams->AddString("host", url.GetHostName());
		} else if (code == 530) {
			//fprintf(stderr, "FtpProtocol: authentication error!\n");
			// Authentication error
			outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/login.html");
			outErrorParams->AddString(S_CHALLENGE_STRING, "ftp");
			err = B_AUTHENTICATION_ERROR;
		} else {
			outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/connect.html");
			outErrorParams->AddString(S_ERROR_MESSAGE, strerror(err));
			outErrorParams->AddString("host", url.GetHostName());		
		}

		fContentLength = 0;
	}

	return err;
}

ssize_t FtpProtocol::GetContentLength()
{
	return fContentLength;
}

void FtpProtocol::GetContentType(char *type, int size)
{
	if (fIsDirectory)
		strncpy(type, "text/html", size);
	else
		strncpy(type, "application/octet-stream", size);
}

CachePolicy FtpProtocol::GetCachePolicy()
{
	return CC_CACHE;
}

ssize_t FtpProtocol::Read(void *buffer, size_t size)
{
	ssize_t s = B_ERROR;

//	FTP_TRACE((FTP_STATUS_COLOR "Read() begin: %Ld\n", system_time()));

	if (fMallocIO != NULL) {
	
		FTP_TRACE((FTP_STATUS_COLOR "Attempting to Read() %ld bytes from fMallocIO.  "
			"fMallocIO->BufferLength() = %ld.\n" FTP_NORMAL_COLOR,
			size, fMallocIO->BufferLength()));
		
		s = fMallocIO->Read(buffer, size);

#if ENABLE_FTP_TRACE
		if (static_cast<ssize_t>(size) >= fContentLength - fAmountRead) {
			FTP_TRACE((FTP_STATUS_COLOR
				"FtpProtocol %08X: %ld Bytes left.  "
				"Tried to read %ld.  "
				"Read %ld.  "
				"Total read: %ld.\n"
				FTP_NORMAL_COLOR,
				reinterpret_cast<unsigned int>(this),
				fContentLength - fAmountRead,
				size,
				s,
				fAmountRead + s));
		}
#endif // ENABLE_FTP_TRACE

	} else if (fDataConn != NULL) {
		s = fDataConn->Receive(buffer, size);

#if ENABLE_FTP_TRACE
		if (static_cast<ssize_t>(size) >= fContentLength - fAmountRead) {
			FTP_TRACE((FTP_STATUS_COLOR
				"FtpProtocol %08X: %ld Bytes left.  "
				"Tried to read %ld.  "
				"Read %ld.  "
				"Total read: %ld.\n"
				FTP_NORMAL_COLOR,
				reinterpret_cast<unsigned int>(this),
				fContentLength - fAmountRead,
				size,
				s,
				fAmountRead + s));
		}
#endif // ENABLE_FTP_TRACE

		if (s > 0)
			fAmountRead += s;
		
		if (fAmountRead >= fContentLength || s <= 0) {
			DeleteDataConnection();
			// Read the "226 Transfer completed" reply:
			BString replyStr;
			int code, codeType;
			status_t rc;
			do {
				rc = GetReply(&replyStr, &code, &codeType);
			} while (codeType != 2 && codeType != 5 && rc >= B_OK);
		}
		
		if (s <= 0) {
			CloseDataConnection();
		}
	}

//	FTP_TRACE((FTP_STATUS_COLOR "Read() end:   %Ld\n\n", system_time()));

	//fprintf(stderr, "FtpProtocol::Read() called, returning %ld\n", s);	
	return s;
}

ssize_t FtpProtocol::ReadAt(off_t pos, void *buffer, size_t size)
{
	//fprintf(stderr, "FtpProtocol::ReadAt() called!\n");
	if (fMallocIO != NULL) {
		return fMallocIO->ReadAt(pos, buffer, size);
	} else {
		return B_NO_RANDOM_ACCESS;
	}
}

off_t FtpProtocol::Seek(off_t position, uint32 seek_mode)
{
	//fprintf(stderr, "FtpProtocol::Seek() called!\n");
	if (fMallocIO != NULL) {
		return fMallocIO->Seek(position, seek_mode);
	} else {
		return 0;
	}
}

off_t FtpProtocol::Position() const
{
	//fprintf(stderr, "FtpProtocol::Position() called!\n");
	if (fMallocIO != NULL) {
		return fMallocIO->Position();
	} else {
		return 0;
	}
}

bool FtpProtocol::GetRedirectURL(URL&, bigtime_t * /* out_delay */)
{
	//fprintf(stderr, "FtpProtocol::GetRedirectURL() called!\n");
	return false;
}

void FtpProtocol::Abort()
{
}

status_t FtpProtocol::Connect(const URL &url, int &outcode, bool autodial)
{
	status_t rc = B_ERROR;
	int code, port;
	BString cmd, replyStr;
	BNetAddress addr;

	CloseDataConnection();
	ReturnControlConnectionToPool();
	outcode = 0;

	port = url.GetPort();
	if (port == 0) {
		port = 21; // ftp control port is 21
	}
	
	//fprintf(stderr, "FtpProtocol::Connect(): connecting to %s:%d\n", url.GetHostName(), port);
	
	if (autodial) {
		//lift any current network blackout, so we can connect successfully
		dnsCache.Blackout(false);
	}

	// FtpConnectionPool::GetFtpConnection() gets (or creates) an FtpConnection
	// and, if necessary, logs us in under the given username & password.
	rc = connectionPool.GetFtpConnection(url.GetHostName(), port,
		url.GetUserName(), url.GetPassword(), true, &code, &fControlConn);
	outcode = code;
	
	if (rc >= B_OK) {
		SetFlag(ftp_connected);
	} else {
		ClearFlag(ftp_connected);
		ReturnControlConnectionToPool();
	}

	return rc;
}


bool FtpProtocol::CurrentDir(BString &dir)
{
	bool rc = false;
	int code, codeType;
	BString cmd("PWD"), replyStr;
	int32 i;
	
	dir.SetTo(B_EMPTY_STRING);
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		if (GetReply(&replyStr, &code, &codeType) >= B_OK) {
			if (codeType == 2) {
				i = replyStr.FindFirst('"');
				if (i >= 0) {
					i++;
					int32 len = replyStr.FindFirst('"') - i;
					if (len > 0) {
						dir.Insert(replyStr, i, replyStr.FindFirst('"') - i, 0);
						rc = true;
					}
				}
			}
		}
	}

	return rc;
}

bool FtpProtocol::SetCurrentDir(const BString &dir)
{
	bool rc = false;
	int code, codeType;
	BString cmd("CWD "), replyStr;

	cmd += dir;

	if (dir.Length() == 0) {
		cmd += '/';
	}
	
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		if (GetReply(&replyStr, &code, &codeType) >= B_OK) {
			if (codeType == 2) {
				rc = true;
			}
		}
	}
	return rc;
}

bool FtpProtocol::RequestFile(const BString &filename)
{
	bool rc = false;
	BString cmd, replyStr;
	int code, codeType;

	if (!GetFileSize(filename, fContentLength)) {
		fContentLength = 0x7fffffff; // we don't know the size
	}
	fAmountRead = 0;

	// set connection to binary mode
	cmd.SetTo("TYPE I");
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		GetReply(&replyStr, &code, &codeType);
	}
	
	if (OpenDataConnection()) {
		// start file transfer
		cmd.SetTo("RETR ");
		cmd += filename;

		if (fControlConn->SendRequest(cmd) >= B_OK) {
			if (GetReply(&replyStr, &code, &codeType) >= B_OK) {
				if (codeType <= 2) {
					if (AcceptDataConnection()) {
						// the connection is now open.  To get the file contents, call
						// fDataConn->Receive() like the Read() function does.
						rc = true;
					}
				}
			}			
		}
	}
	
	return rc;
}

bool FtpProtocol::GetFileSize(const BString &filename, ssize_t &size)
{
	bool rc = false;
	int code, codeType;
	BString cmd, replyStr;

	// set connection to binary mode
	cmd.SetTo("TYPE I");
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		GetReply(&replyStr, &code, &codeType);
	}

	cmd.SetTo("SIZE ");
	cmd += filename;
	
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		if (GetReply(&replyStr, &code, &codeType) >= B_OK) {
			if (code == 213) {
				// Reply should be of the format "213 SSSSS" where SSSSS is the size in bytes.
				size = atoi(replyStr.String() + 4);
				if (size != 0) rc = true;
			} else {
				size = -1;
			}
		}
	}
	return rc;
}

status_t FtpProtocol::Rename(const char* old_name, const char* new_name)
{
	int			code;
	int			codeType;
	status_t	result = B_ERROR;
	BString		cmd;
	BString		replyStr;
	
	// send rename from command
	cmd.SetTo("RNFR ");
	cmd.Append(old_name);
	if (fControlConn->SendRequest(cmd) >= B_OK)
	{
		if (GetReply(&replyStr, &code, &codeType) >= B_OK)
		{
			if (code == 350)
			{
				// send rename to command
				cmd.SetTo("RNTO ");
				cmd.Append(new_name);
				if (fControlConn->SendRequest(cmd) >= B_OK)
					GetReply(&replyStr, &code, &codeType);
			}
		}
		result = StatusCodeToOS(code);
	}
	return result;
}


status_t FtpProtocol::Delete(const char* name)
{
	int			code;
	int			codeType;
	BString		cmd;
	BString		replyStr;
	status_t	result = B_ERROR;
	
	// send delete command
	cmd.SetTo("DELE ");
	cmd.Append(name);
	if (fControlConn->SendRequest(cmd) >= B_OK)
	{
		if (GetReply(&replyStr, &code, &codeType) >= B_OK)
			result = StatusCodeToOS(code);
	}
	return result;
}


status_t FtpProtocol::MakeDirectory(const char* name)
{
	int			code;
	int			codeType;
	BString		cmd;
	BString		replyStr;
	status_t	result = B_ERROR;

	cmd.SetTo("MKD ");
	cmd.Append(name);
	if (fControlConn->SendRequest(cmd) >= B_OK)
	{
		if (GetReply(&replyStr, &code, &codeType) >= B_OK)
		{
			if (code == 550)
			{
				if (strstr(replyStr.String(), "Permission denied"))
					result = B_PERMISSION_DENIED;
				else
					result = B_FILE_EXISTS;
			}
			else
				result = StatusCodeToOS(code);
		}
	}
	return result;
}

status_t FtpProtocol::SaveFileToFtp(BFile* src, const char* name)
{
	status_t result = B_ERROR;
	BString cmd, replyStr;
	int code, codeType;

	// set connection to binary mode
	cmd.SetTo("TYPE I");
	if (fControlConn->SendRequest(cmd) >= B_OK)
		GetReply(&replyStr, &code, &codeType);
	
	if (OpenDataConnection()) {
		// start file transfer
		cmd.SetTo("STOR ");
		cmd += name;

		if (fControlConn->SendRequest(cmd) >= B_OK)
		{
			if (GetReply(&replyStr, &code, &codeType) >= B_OK)
			{
				if (codeType <= 2)
				{
					if (AcceptDataConnection())
					{
						void*	buffer;
						size_t	size = 2 * 1024;
						ssize_t	read;
						ssize_t	s;

						while ((buffer = malloc(size)) == 0);
							size /= 2;

						while ((read = src->Read(buffer, size)) > 0)
						{
							s = fDataConn->Send(buffer, read);
							if (s < 0)
							{
								result = s;
								break;
							}
						}
						code = CloseDataConnection();
						if (code != 0)
							result = StatusCodeToOS(code);
						free(buffer);
					}
				}
			}				
		}	
	}
	
	return result;
}

status_t FtpProtocol::SaveFileToLocal(BFile* target, const char* name)
{
	status_t	result = B_ERROR;
	BString		file(name);

	if (RequestFile(name))
	{
		size_t	size = 2048;
		void*	buffer = malloc(size);

		if (buffer)
		{
			ssize_t	size_read, size_written;

			if (fMallocIO != NULL)
			{
				result = B_NO_ERROR;
				while ((size_read = fMallocIO->Read(buffer, size)) > 0)
				{
					if ((size_written = target->Write(buffer, size_read)) < 0)
					{
						result = size_written;
						break;
					}
				}
			}
			else if (fDataConn != NULL)
			{
				result = B_NO_ERROR;

				while ((size_read = fDataConn->Receive(buffer, size)) > 0)
				{
					if ((size_written = target->Write(buffer, size_read)) < 0)
					{
						result = size_written;
						break;
					}
					else {
						fAmountRead += size_read;
					}
					FTP_TRACE((FTP_STATUS_COLOR "Trying to read %ld bytes of file \"%s\"...\n"
						FTP_NORMAL_COLOR, size, name));
				}
				
				if (size_read <= 0) {
					DeleteDataConnection();
					// Read the "226 Transfer completed" reply:
					BString replyStr;
					int code, codeType;
					status_t rc;
					do {
						rc = GetReply(&replyStr, &code, &codeType);
					} while (codeType != 2 && codeType != 5 && rc >= B_OK);
				}
		
				FTP_TRACE((FTP_STATUS_COLOR "FtpProtocol received %ld bytes of file \"%s\".\n"
					FTP_NORMAL_COLOR, fAmountRead, name));

				CloseDataConnection();
			}
			free(buffer);
		}
	}
	return result;
}

bool FtpProtocol::DoDirListing(BString &outListing)
{
	//fprintf(stderr, "FtpProtocol::DoDirListing()\n");
	bool rc = false;
	int code, codeType;
	BString cmd, replyStr;
	
	// set connection to ASCII mode
	cmd.SetTo("TYPE A");
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		GetReply(&replyStr, &code, &codeType);
	}
	
	if (OpenDataConnection()) {
		// send LIST request for the current working directory
		cmd.SetTo("LIST");

		if ((fControlConn->SendRequest(cmd) >= B_OK) &&
			(GetReply(&replyStr, &code, &codeType) >= B_OK) &&
			codeType <= 2 && AcceptDataConnection())
		{
			// read LIST results from server into outListing
			char buf[513];
			int32 numread = 1;
			while (numread > 0) {
				memset(buf, 0, sizeof(buf));

//				FTP_TRACE((FTP_STATUS_COLOR "Attempting to read %ld bytes of LISTing.\n"
//					FTP_NORMAL_COLOR, sizeof(buf) - 1));

				numread = fDataConn->Receive(buf, sizeof(buf) - 1);

//				FTP_TRACE((FTP_STATUS_COLOR "Read %ld bytes of LISTing.\n"
//					FTP_NORMAL_COLOR, numread));

				buf[numread] = '\0';
				
				outListing += buf;
			}
			// make sure we have just LFs, not solitary CRs or CR-LFs
			outListing.ReplaceAll("\r\n", "\n");
			outListing.ReplaceAll("\r", "\n");
			
//			FTP_TRACE(("Looking for \"226 Transfer complete.\"...\n"));

			// Some ftp servers (notably ftp.be.com) seem to not send
			// a "226 Transfer Complete" until you close the data
			// connection.
			DeleteDataConnection();

			// read command reply
			GetReply(&replyStr, &code, &codeType);
			rc = (codeType == 2);
		}
	}
	CloseDataConnection();

	return rc;
}

// generates HTML from an FTP LIST result and sticks it
// into the fMallocIO member.
void FtpProtocol::GenerateHTMLDirListing(BString &listing, const URL &base)
{
	delete fMallocIO;
	fMallocIO = new BMallocIO();
	fMallocIO->SetBlockSize(1024);

	BString basePath(B_EMPTY_STRING, URL_LENGTH);
	char *pathPtr = basePath.LockBuffer(URL_LENGTH);
	base.GetUnescapedPath(pathPtr, URL_LENGTH);
	basePath.UnlockBuffer();
		
	if (basePath[basePath.Length() - 1] != '/') { basePath << '/'; }
	BString baseStr(B_EMPTY_STRING, 160);
	baseStr << base.GetScheme() << "://" << base.GetHostName();
	if (base.GetPort()) { baseStr << ":" << (int32)base.GetPort(); }
	baseStr << basePath;
	
	BString html(B_EMPTY_STRING, 2048);
	html << "<html><head><title>" << baseStr << "</title></head>\n<body>\n";
	html << "<h1>" << baseStr << "</h1>\n";
	html << "<tt><table border=0 cols=3 width=90%>\n";
	html << "<tr><td colspan=3><hr></td></tr>\n";
	html << "<tr><td width=\"50%\">Filename</td><td align=right>Size</td><td align=right>Date</td></tr>\n";
	html << "<tr><td colspan=3><hr></td></tr>\n";
	
	fMallocIO->Write((void *)html.String(), html.Length());
	int32 nl_idx = 0, begin_idx = 0;
	while ((nl_idx = listing.FindFirst('\n', begin_idx)) >= 0) {
		ftpparse fp;
		BString name;
		char datebuf[80];
		int32 len = nl_idx - begin_idx;
		if (ftp_parse(&fp, (listing.String() + begin_idx), len)) {
			while (fp.name[0] == ' ') fp.name++; // skip leading spaces that ftp_parse sometimes causes
			name.SetTo(fp.name, fp.namelen);
			html.SetTo("<tr><td><tt><a href=\"");
			BString unescaped_url_string(baseStr);
			unescaped_url_string += name;
			URL href_url(unescaped_url_string.String(), true);
			// XXX: I think the basePath and first name in this line should be URL
			// escaped, so that a filename with "+" in it can be found (otherwise
			// the "+" gets translated into a " "). 
			html << href_url.GetPath() << "\">" << name << "</a>";
			html << "</tt></td><td align=right><tt>" << (int32)fp.size << "</tt></td><td align=right>";
			FTP_TRACE(("Generating HTML: \"%s\"\n", html.String()));
			struct tm tm_struct;
			switch (fp.mtimetype) {
				case FTPPARSE_MTIME_LOCAL:
				case FTPPARSE_MTIME_REMOTEMINUTE:
				case FTPPARSE_MTIME_REMOTEDAY:
					localtime_r((const time_t *)&fp.mtime, &tm_struct);
					// print the date in the format "01 Jan 2000 00:00:00"
					strftime(datebuf, sizeof(datebuf), "%d %b %Y %T", &tm_struct);
					html << "<tt>" << datebuf << "</tt>";
					break;
				case FTPPARSE_MTIME_UNKNOWN:
				default:
					// don't print anything for the date cell
					break;
			}
			html << "</td></tr>\n";
			fMallocIO->Write((void *)html.String(), html.Length());
		}
		begin_idx = nl_idx + 1;
	}
	html.SetTo("<tr><td colspan=3><hr></td></tr>\n");
	html << "</table>\n</body>\n</html>\n";
	fMallocIO->Write((void *)html.String(), html.Length());
	fMallocIO->Seek(0, SEEK_SET);
}

// generates non-HTML from an FTP LIST result and sticks it
// into the fMallocIO member.
void FtpProtocol::GenerateDirListing(BString &listing, const URL &base)
{
	delete fMallocIO;
	fMallocIO = new BMallocIO();
	fMallocIO->SetBlockSize(1024);

	BString basePath(B_EMPTY_STRING, URL_LENGTH);
	char *pathPtr = basePath.LockBuffer(URL_LENGTH);
	base.GetUnescapedPath(pathPtr, URL_LENGTH);
	basePath.UnlockBuffer();
		
	if (basePath[basePath.Length() - 1] != '/') { basePath << '/'; }
	BString baseStr(B_EMPTY_STRING, 160);
	baseStr << base.GetScheme() << "://" << base.GetHostName();
	if (base.GetPort()) { baseStr << ":" << (int32)base.GetPort(); }
	baseStr << basePath;
	
	BString list(B_EMPTY_STRING, 2048);
	
	int32 nl_idx = 0, begin_idx = 0;
	int32 null = 0;
	while ((nl_idx = listing.FindFirst('\n', begin_idx)) >= 0) {
		ftpparse fp;
		int32 len = nl_idx - begin_idx;
		if (ftp_parse(&fp, (listing.String() + begin_idx), len)) {
			while (fp.name[0] == ' ') fp.name++; // skip leading spaces that ftp_parse sometimes causes
			fMallocIO->Write(basePath.String(), basePath.Length());
			fMallocIO->Write(fp.name, fp.namelen);
			fMallocIO->Write(&null, 1);
			fMallocIO->Write(fp.name, fp.namelen);
			fMallocIO->Write(&null, 1);
			fMallocIO->Write(&fp.size, sizeof(fp.size));
			struct tm tm_struct;
			switch (fp.mtimetype) {
				case FTPPARSE_MTIME_LOCAL:
				case FTPPARSE_MTIME_REMOTEMINUTE:
				case FTPPARSE_MTIME_REMOTEDAY:
					localtime_r((const time_t *)&fp.mtime, &tm_struct);
					fMallocIO->Write(&tm_struct, sizeof(tm));
					break;
				case FTPPARSE_MTIME_UNKNOWN:
				default:
					// don't print anything for the date cell
					break;
			}
			if ((fp.flagtrycwd) && (fp.flagtryretr)) // probably a link
				fMallocIO->Write("l", 1);
			else if (fp.flagtrycwd)
				fMallocIO->Write("d", 1);			// directory
			else
				fMallocIO->Write("f", 1);			// file
		}
		begin_idx = nl_idx + 1;
	}
	fMallocIO->Write(&null, 1);
	fMallocIO->Seek(0, SEEK_SET);
}

void FtpProtocol::SetPassiveMode(bool on)
{
	if (on) {
		SetFlag(ftp_passive);
	} else {
		ClearFlag(ftp_passive);
	}
}

bool FtpProtocol::TestFlag(unsigned long state) const
{
	return (bool) ((fFlags & state) != 0);
}

void FtpProtocol::SetFlag(unsigned long state)
{
	fFlags |= state;
}

void FtpProtocol::ClearFlag(unsigned long state)
{
	fFlags &= ~state;
}

bool FtpProtocol::OpenDataConnection()
{
	bool rc = false;
		
	if (TestFlag(ftp_passive)) {
		rc = OpenPassiveDataConnection();
		if (!rc) {
			ClearFlag(ftp_passive);
			rc = OpenNonPassiveDataConnection();
		}
	} else {
		rc = OpenNonPassiveDataConnection();
	}

	return rc;
}

bool FtpProtocol::OpenPassiveDataConnection()
{
	BString cmd, replyStr;
	BNetAddress addr;
	int code, codeType;
	int32 lparenp1_idx, rparen_idx;
	struct sockaddr_in sa;
	bool rc = false;

	CloseDataConnection();
	fDataConn = new BNetEndpoint();

	//
	// Here we send a "pasv" command and connect to the remote server
	// on the port it sends back to us
	//
	cmd.SetTo("PASV");
	if (fControlConn->SendRequest(cmd) >= B_OK) {
		if (GetReply(&replyStr, &code, &codeType) >= B_OK) {
			if (codeType == 2) {
				 //  server should give us something like:
		 		 // "227 Entering Passive Mode (192,168,1,1,10,187)"
				int paddr[6];
				unsigned char ucaddr[6];
				lparenp1_idx = replyStr.FindFirst('(');
				rparen_idx = replyStr.FindFirst(')', lparenp1_idx);
				lparenp1_idx++;

				BString addrStr;
				replyStr.CopyInto(addrStr, lparenp1_idx, rparen_idx - lparenp1_idx); 

				if (sscanf(addrStr.String(), "%d,%d,%d,%d,%d,%d",
					&paddr[0], &paddr[1], &paddr[2], &paddr[3],
					&paddr[4], &paddr[5]) == 6)
				{
					for (int i = 0; i < 6; i++) {
						ucaddr[i] = (unsigned char) (paddr[i] & 0xff);
					}
					memset(&sa, 0, sizeof(sa));
					sa.sin_family = AF_INET;
					sa.sin_len = sizeof(sa);
					memcpy(&sa.sin_addr.s_addr, &ucaddr[0], (size_t) 4);
					memcpy(&sa.sin_port, &ucaddr[4], (size_t) 2);
					addr.SetTo(sa);
					if (fDataConn->Connect(addr) == B_NO_ERROR) {
						rc = true;
						FTP_TRACE((FTP_STATUS_COLOR "FtpProtocol established PASV data connection.\n"
							FTP_NORMAL_COLOR));
					}
				}
			}
		}
	}
	
	return rc;
}
	
bool FtpProtocol::OpenNonPassiveDataConnection()
{
	bool rc = false;

	CloseDataConnection();
	fDataConn = new BNetEndpoint();

	//
	// Here we bind to a local port and send a PORT command
	//
	if (fDataConn->Bind() == B_NO_ERROR) {
		BString cmd, replyStr;
		int code, codeType;
		BNetAddress addr;
		char buf[255];
		unsigned short port;

		fDataConn->Listen();
	
		addr = fDataConn->LocalAddr();
		addr.GetAddr(buf, &port);
		BString host(buf);
	
//		FTP_TRACE(("fDataConn->LocalAddr() = %s.\n", buf));
		
		// A ftp HOST-PORT specification is the 4 octets of the address,
		// followed by the 2 octets of the port number, comma separated.
		// It looks like this: h,h,h,h,p,p 
		int32 dot_idx = 0;
		while ((dot_idx = host.FindFirst('.', ++dot_idx)) > B_ERROR) {
			host[dot_idx] = ',';
		}
	
		sprintf(buf, ",%d,%d", (port & 0xff00) >> 8, port & 0x00ff);
		cmd.SetTo("PORT ");
		cmd += host;
		cmd += buf;
		if (fControlConn->SendRequest(cmd) >= B_OK) {
			GetReply(&replyStr, &code, &codeType);
		}
		//
		// PORT failure is in the 500-range
		//
		if (codeType == 2) {
			rc = true;
			FTP_TRACE((FTP_STATUS_COLOR "FtpProtocol established non-PASV data connection.\n"
				FTP_NORMAL_COLOR));
		}
	}
	
	return rc;
}

bool FtpProtocol::AcceptDataConnection()
{
	BNetEndpoint *ep;
	bool rc = false;

	if (TestFlag(ftp_passive) == false) {
		if (fDataConn != NULL) {
			ep = fDataConn->Accept();
			if (ep != NULL) {
				DeleteDataConnection();
				fDataConn = ep;
				rc = true;
			}
		}
	} else {
		rc = true;
	}

	return rc;
}


// convert an ftp status code to an BeOS status code
// <RMP: this is not complete>
status_t FtpProtocol::StatusCodeToOS(int code)
{
	switch (code)
	{
		case 226:	return B_NO_ERROR;			// Closing data connection. Requested file action successful.
		case 250:	return B_NO_ERROR;			// Requested file action okay, completed.
		case 257:	return B_NO_ERROR;			// Pathname created
		case 350:	return B_NO_ERROR;			// Requested file action pending further information.
		case 421:	return B_UNSUPPORTED;		// Service not available, closing control connection.
		case 425:	return B_BUSTED_PIPE;		// Can't open data connection
		case 426:	return B_BUSTED_PIPE;		// Connection closed; transfer aborted.
		case 450:	return B_BUSY;				// File unavailable (e.g., file busy).
		case 451:	return B_ERROR;				// Requested action aborted. Local error in processing.
		case 452:	return B_DEVICE_FULL;		// Insufficient storage space in system.
		case 500:	return B_BAD_VALUE;			// Syntax error, command unrecognized.
		case 501:	return B_BAD_VALUE;			// Syntax error in parameters or arguments.
		case 502:	return B_NOT_ALLOWED;		// Command not implemented.
		case 503:	return B_MISMATCHED_VALUES;	// Bad sequence of commands.
		case 530:	return B_PERMISSION_DENIED;	// Not logged in.
		case 532:	return B_PERMISSION_DENIED;	// Need account for storing files.
		case 550:	return B_FILE_NOT_FOUND;	// File unavailable (e.g., file not found, no access).
		case 551:	return B_ERROR;				// Requested action aborted. Page type unknown.
		case 552:	return B_DEVICE_FULL;		// Exceeded storage allocation
		case 553:	return B_FILE_ERROR;		// File name not allowed.
	}
	return B_ERROR;
}


void FtpProtocol::ReturnControlConnectionToPool()
{
	connectionPool.ReturnConnection(fControlConn, fKeepAliveTimeout,
		fMaxKeepAliveRequests);
	fControlConn = NULL;
}


status_t FtpProtocol::GetReply(BString *outstr, int *outcode, int *codetype)
{
	status_t rc = fControlConn->GetReply(outstr, outcode, codetype);
	if (rc <= B_ERROR) {
		// This is usually pretty bad, and fControlConn is probably now
		// out of sync in its SendRequest()/GetReply() sequence, so when
		// we return it to the pool, tell the pool not to reuse it.
		fKeepAliveTimeout = 0;
		fMaxKeepAliveRequests = 0;
	}
	
	return rc;
}



// ----------------------- FtpProtocolFactory -----------------------

class FtpProtocolFactory : public ProtocolFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_PROTOCOL_SCHEMES, "ftp");
	}
	
	virtual Protocol* CreateProtocol(void* handle, const char* /* scheme */)
	{
		return new FtpProtocol(handle);
	}
	
	virtual bool KeepLoaded() const
	{
		return true;
	}
};

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n,
	image_id /* you */, uint32 /* flags */, ...)
{
	if (n == 0) {
		return new FtpProtocolFactory();
	}
	return NULL;
}
