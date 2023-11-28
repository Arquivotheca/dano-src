#if ! defined MULTIPOSITIONIO_INCLUDED
#define MULTIPOSITIONIO_INCLUDED 1

#include <DataIO.h>
#include <Locker.h>

class MultiPositionIOClient;

class MultiPositionIOServer : public BLocker
{
	BPositionIO				*fSource;
#if !defined(NDEBUG)
	int32					fClients;
	friend class MultiPositionIOClient;
#endif

public:
							MultiPositionIOServer(BPositionIO *source);
							~MultiPositionIOServer();
	MultiPositionIOClient	*GetClient();
};

class MultiPositionIOClient : public BPositionIO
{
	MultiPositionIOServer	*fServer;
	BPositionIO				*fSource;
	off_t					fPos;

						MultiPositionIOClient(MultiPositionIOServer *server, BPositionIO *source);
#if !defined(NDEBUG)
virtual					~MultiPositionIOClient();
#endif
public:

	virtual	ssize_t		Read(void *buffer, size_t size);
	virtual	ssize_t		Write(const void *buffer, size_t size);
	
	virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
	virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);
	
	virtual off_t		Seek(off_t position, uint32 seek_mode);
	virtual	off_t		Position() const;
	
	virtual status_t	SetSize(off_t size);

	friend class MultiPositionIOServer;
};

#endif
