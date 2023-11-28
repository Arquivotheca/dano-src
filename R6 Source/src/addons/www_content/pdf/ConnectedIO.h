
#ifndef _CONNECTED_IO_H_
#define _CONNECTED_IO_H_

#include <DataIO.h>
#include <URL.h>
#include <Protocol.h>

using namespace Wagner;

class ConnectedIO : public BPositionIO, public BAtom
{
	public:
							ConnectedIO(const URL &url, const URL &refer);
							~ConnectedIO();
		virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
		virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);
	
		virtual off_t		Seek(off_t position, uint32 seek_mode);
		virtual	off_t		Position() const;
	
		virtual status_t	SetSize(off_t size);

	
		virtual	void		Acquired();
		virtual	void		Cleanup();
		ssize_t				Connect();
		const char *		Scheme() const;
	private:
		virtual	void		_delete();
		URL					fURL;
		URL					fRefer;
		Protocol *			fProtocol;
		off_t				fSize;
		off_t				fPos;
};

#endif
