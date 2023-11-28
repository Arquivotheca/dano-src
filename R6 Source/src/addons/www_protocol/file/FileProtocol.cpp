#include <fs_attr.h>
#include <image.h>
#include <Mime.h>
#include <stdio.h>
#include <String.h>
#include <StringBuffer.h>
#include <unistd.h>
#include "Protocol.h"
#include "ResourceCache.h"
#include "URL.h"

using namespace Wagner;

const char *kCGIDirName = "cgi-bin";
const char *kLBXContainerFileName = "images.lbx";

class FileProtocol : public Protocol {
public:
	FileProtocol(void* handle);
	virtual ~FileProtocol();
	virtual status_t Open(const URL &url, const URL&, BMessage *errorParams,
		uint32 flags);
	virtual ssize_t GetContentLength();
	virtual void GetContentType(char *type, int size);
	virtual CachePolicy GetCachePolicy();
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	virtual off_t Seek(off_t position, uint32 seek_mode);
	virtual	off_t Position() const;
private:
	void ExpandPathVariables(StringBuffer &out, StringBuffer &in);
	status_t SpawnCGIScript(const char *path, const char *query);
	status_t ParseResponseHeader();

	int fFD;
	int fFD_is_pipe;
	CachePolicy fCachePolicy;
	char fContentType[B_MIME_TYPE_LENGTH];
};

FileProtocol::FileProtocol(void* handle)
	:	Protocol(handle),
		fFD(-1),
		fFD_is_pipe(false),
		fCachePolicy(CC_NO_STORE)
{
	fContentType[0] = '\0';
}

FileProtocol::~FileProtocol()
{
	if (fFD > 0)
		close(fFD);
}

status_t FileProtocol::Open(const URL &url, const URL&, BMessage* outErrorParams,
	uint32 flags)
{
	// get the URL string
	StringBuffer urlString;
	url.AppendTo(urlString);

	// expand the URL string
	StringBuffer expanded;
	ExpandPathVariables(expanded, urlString);

	URL newURL(expanded.String());

	char path[B_PATH_NAME_LENGTH];
	newURL.GetUnescapedPath(path, B_PATH_NAME_LENGTH);

	// Determine if the is a local CGI file.
	int pathLen = strlen(path);
	const char *lastDelimiter = path + pathLen;
	while (lastDelimiter > path && *lastDelimiter != '/')
		lastDelimiter--;

	lastDelimiter--;
	uint length = 0;
	while (lastDelimiter > path && *lastDelimiter != '/') {
		lastDelimiter--;
		length++;
	}

	lastDelimiter++;
	if (length == strlen(kCGIDirName) && strncmp(kCGIDirName, lastDelimiter, length) == 0) {
		if (flags & RANDOM_ACCESS) {
			PRINT(("Can't request range from CGI script\n"));
			return B_NO_RANDOM_ACCESS;
		}
		
		char unescaped_query[2048];
		url.GetUnescapedQuery(unescaped_query, sizeof(unescaped_query) - 1);
		status_t err = SpawnCGIScript(path, unescaped_query);
		if (err < 0)
			return err;
	
		err = ParseResponseHeader();
		if (err < 0)
			return err;

		fCachePolicy = CC_NO_CACHE;
		return B_OK;
	}

	//	This is a normal file, just open it.
	fFD = open(path, O_RDONLY | O_CLOEXEC);

	// www.hack.com: cfs seems to return B_ERROR if the file cannot be found!
	if (fFD < 0)
		fFD = B_FILE_NOT_FOUND;
	/////////////////////////////////////////////////////////////////////////


	if (fFD == B_FILE_NOT_FOUND) {
		// This file may actually be in a "virtual" directory that is stored
		// in a compressed file.  Check to see if the LBX container is present,
		// and if so, pass the URL on to a content handler that can load content
		// from the LBX.  Note that it won't get fed any data because Read()
		// will return < 0 bytes.
		// FIXME: this does not handle the case where the requested name is not
		// in the directory and does not exist in the LBX file.
		char dirPath[B_PATH_NAME_LENGTH];
		strncpy(dirPath, path, pathLen);
		char *filenameStart = dirPath + pathLen;
		while (filenameStart > dirPath && *filenameStart != '/')
			filenameStart--;

		filenameStart++;
		// take the path name and append the LBX container name
		strcpy(filenameStart, kLBXContainerFileName);
		// stat the path to see if the file exists
		struct stat sb;
		int err = stat(dirPath, &sb);
		if (S_ISREG(sb.st_mode)) {
			strcpy(fContentType, "application/x-vnd.Be.Lbx");
			return B_OK;
		}
	}

	if (fFD < 0) {
		outErrorParams->AddString(S_ERROR_TEMPLATE, "Errors/file.html");
		outErrorParams->AddString("filename", path);
		outErrorParams->AddString(S_ERROR_MESSAGE, strerror(fFD));
		return fFD;
	}

	if (fs_read_attr(fFD, "BEOS:TYPE", B_MIME_STRING_TYPE, 0, fContentType, B_MIME_TYPE_LENGTH) < 0)
		strcpy(fContentType, "application/octet-stream");

	return B_OK;		
}

ssize_t FileProtocol::GetContentLength()
{
	struct stat sb;
	
	if(fFD_is_pipe) {
		return 0x7fffffff;
	} else {
		fstat(fFD, &sb);
		if(sb.st_size <= 0) {
			return 0x7fffffff;
		}
	}
	
	return sb.st_size;
}

void FileProtocol::GetContentType(char *type, int size)
{
	strncpy(type, fContentType, size);
}

CachePolicy FileProtocol::GetCachePolicy()
{
	return fCachePolicy;
}

ssize_t FileProtocol::Read(void *buffer, size_t size)
{
	return read(fFD, buffer, size);		
}

ssize_t FileProtocol::ReadAt(off_t pos, void *buffer, size_t size)
{
	return read_pos(fFD, pos, buffer, size);
}

off_t FileProtocol::Seek(off_t position, uint32 seekMode)
{
	return lseek(fFD, position, seekMode);
}

off_t FileProtocol::Position() const
{
	return lseek(fFD, 0, SEEK_CUR);
}

void FileProtocol::ExpandPathVariables(StringBuffer &out, StringBuffer &inBuf)
{
	const char *in = inBuf.String();
	while (*in) {
		if (*in == '$') {
			in++;
			char variableName[B_PATH_NAME_LENGTH];
			int i = 0;
			int len = 0;
			while (*in && *in != '/') {
				if (len++ < B_PATH_NAME_LENGTH)
					variableName[i++] = *in;
			
				in++;
			}

			variableName[i] = '\0';
			
			out.Append(configSettings[variableName].String().String());
		} else
			out << *in++;
	}
}

status_t FileProtocol::SpawnCGIScript(const char *path, const char *query)
{
	int pipeDescriptors[2];
	status_t err = B_OK;
	const char *argv[4];
	thread_id thid;
	int oldStdOut = -1;
	StringBuffer environment;

	// Lock stdout.  This has three purposes.  First, it protects against
	// random debugging garbage being injected into the pipe before it's
	// properly duped it and cloned it into the child process.  Second,
	// it protects against multiple threads mucking with fd 1, which could
	// end up losing it.  Third, it protects the environment area, which gets
	// messed with here.
	flockfile(stdout);

	err = pipe(pipeDescriptors);
	if (err < 0)
		goto error1;

	oldStdOut = dup(1);
	if (oldStdOut < 0) {
		err = oldStdOut;
		goto error2;
	}

	err = close(1);
	if (err < 0)
		goto error3;

	err = dup2(pipeDescriptors[1], 1);
	if (err < 0)
		goto error3;

	fcntl(pipeDescriptors[0], F_SETFD, FD_CLOEXEC);
	fcntl(oldStdOut, F_SETFD, FD_CLOEXEC);

	argv[0] = path;
	argv[1] = NULL;

	char resString[B_PATH_NAME_LENGTH];
	strncpy(resString,configSettings["RESOURCES"].String().String(),B_PATH_NAME_LENGTH);
	environment << "RESOURCES=" << resString;
	putenv(environment.String());
	
	environment.Clear();
	strncpy(resString,configSettings["SCRIPTS"].String().String(),B_PATH_NAME_LENGTH);
	environment << "SCRIPTS=" << resString;
	putenv(environment.String());

	environment.Clear();
	environment << "QUERY_STRING=" << query;
	putenv(environment.String());
	putenv("REQUEST_METHOD=GET");

	thid = load_image(1, argv, (const char**) environ);
	if (thid < 0) {
		err = thid;
		goto error3;
	}

	resume_thread(thid);

	close(pipeDescriptors[1]);
	close(1);
	dup2(oldStdOut, 1);
	close(oldStdOut);
	fFD = pipeDescriptors[0];	// Read end
	fFD_is_pipe= true;
	funlockfile(stdout);

	return B_OK;

error3:
	close(1);
	dup2(oldStdOut, 1);
	close(oldStdOut);
error2:
	close(pipeDescriptors[0]);
	close(pipeDescriptors[1]);
error1:
	funlockfile(stdout);
	return err;
}

status_t FileProtocol::ParseResponseHeader()
{
	StringBuffer buf;
	for (;;) {
		char c;
		if (read(fFD, &c, 1) < 0)
			return B_ERROR;

		if (c == '\n') {
			if (buf.Length() == 0)
				break;		// end of header (CRLF CRLF)
			
			const char *tag = buf.String();
			char *value = strchr(tag, ':') + 1;
			if (value != 0 && strncasecmp(tag, "content-type", value - tag) == 0) {
				while (*value == ' ')
					value++;

				strcpy(fContentType, value);					
			}

			buf.Clear();
		} else if (c != '\r')
			buf << c;
	}

	return B_OK;
}

class FileProtocolFactory : public ProtocolFactory {
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 // BE AWARE: Any changes you make to these identifiers should
		 // also be made in the 'addattr' command in the makefile.
		into->AddString(S_PROTOCOL_SCHEMES, "file");
	}
	
	virtual Protocol* CreateProtocol(void* handle, const char*)
	{
		return new FileProtocol(handle);
	}
	
	virtual bool KeepLoaded() const
	{
		return true;
	}
};

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id /* you */, uint32 /* flags */, ...)
{
	if (n == 0)
		return new FileProtocolFactory;

	return 0;
}
