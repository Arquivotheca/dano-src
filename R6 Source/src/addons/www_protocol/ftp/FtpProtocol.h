#ifndef _FTP_PROTOCOL_H
#define _FTP_PROTOCOL_H

/*
 * FtpProtocol - a Wagner Protocol object for handling FTP connections
 *
 * This class borrows liberally from Howard Berkey's FtpClient sample code.
 */

#include <File.h>
#include <Protocol.h>
#include <String.h>

class BNetEndpoint;
class FtpConnection;

using namespace Wagner;

class FtpProtocol : public Protocol {
public:
						FtpProtocol(void* handle);
	virtual				~FtpProtocol();
	
	virtual status_t	Open(const URL &url, const URL&, BMessage* outErrorParams,
							uint32 flags);
	virtual status_t	OpenRW(const URL &url, const URL&, BMessage* outErrorParams,
							uint32 flags, bool no_list = false);
	virtual ssize_t 	GetContentLength();
	virtual void 		GetContentType(char *type, int size);
	virtual CachePolicy GetCachePolicy();
	virtual ssize_t		Read(void *buffer, size_t size);
	virtual ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
	virtual off_t		Seek(off_t position, uint32 seek_mode);
	virtual	off_t		Position() const;
	virtual bool		GetRedirectURL(URL&, bigtime_t *out);
	virtual void		Abort();

	virtual status_t	Rename(const char* old_name, const char* new_name);
	virtual status_t	Delete(const char* file);
	virtual status_t	MakeDirectory(const char* name);
	virtual status_t	SaveFileToFtp(BFile*, const char* name);
	virtual status_t	SaveFileToLocal(BFile*, const char* name);


protected:
	enum ftp_mode {
		binary_mode,
		ascii_mode
	};

	enum {
		ftp_complete = 1UL,
		ftp_connected = 2,
		ftp_passive = 4
	};

	status_t			OpenCommon(const URL &url, const URL&, BMessage* outErrorParams,
							uint32 flags, bool no_html = false, bool no_list = false);
	status_t			Connect(const URL &url, int &outcode, bool autodial);

	bool				CurrentDir(BString &dir);
	bool				SetCurrentDir(const BString &dir);
	bool				DoDirListing(BString &outListing);
	bool				RequestFile(const BString &filename);
	bool				GetFileSize(const BString &filename, ssize_t &size);
	
	void				SetPassiveMode(bool on);
	bool				TestFlag(unsigned long state) const;
	void				SetFlag(unsigned long state);
	void				ClearFlag(unsigned long state);

private:

	bool				OpenDataConnection();
	bool				OpenPassiveDataConnection();
	bool				OpenNonPassiveDataConnection();
	bool				AcceptDataConnection();
	int					CloseDataConnection();
	void				DeleteDataConnection();
	void				ReturnControlConnectionToPool();
	status_t			GetReply(BString *outstr, int *outcode, int *codetype);
		
						// generates HTML from an FTP LIST result and sticks it
						// into the fMallocIO member.
	void				GenerateHTMLDirListing(BString &listing, const URL &base);

						// generate a non-HTML from an FTP LIST result and sticks it
						// into the fMallocIO member.
	void				GenerateDirListing(BString &listing, const URL &base);

						// convert a ftp status code to an OS status code.
	status_t			StatusCodeToOS(int);
	
	ssize_t			fContentLength;
	ssize_t			fAmountRead;
	uint32			fFlags;
	FtpConnection*	fControlConn;
	BNetEndpoint*	fDataConn;
	bigtime_t		fKeepAliveTimeout;
	int				fMaxKeepAliveRequests;
	BMallocIO*		fMallocIO;
	bool			fIsDirectory;
	
};

#endif // _FTP_PROTOCOL_H
