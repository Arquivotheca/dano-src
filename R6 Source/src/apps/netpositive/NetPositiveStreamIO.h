/*******************************************************************************
/
/	File:			NetPositiveStreamIO.h
/
/   Description:    
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_NETPOSITIVESTREAMIO_H
#define	_NETPOSITIVESTREAMIO_H

#include <DataIO.h>
#include <String.h>
#include <OS.h>

// BNetPositiveStreamIO is a class that allows plug-ins and other applications
// to receive streamed network resources from NetPositive.  It works with plug-ins
// that operate in NetPositve's address space, as well as applications in other
// address spaces.  It does this by operating in one of two modes -- as a
// BMallocIO clone for shared address spaces, or through ports for separate address
// spaces.  This modality happens transparently to the users of this class.

// This class has an additional feature -- if you try to read beyond the current
// end of the stream, instead of returning an error, it will block until the
// data appears.  There are ReadWithTimeout and ReadAtWithTimeout calls to let
// you determine how long you wish to block.

// Due to this unique functionality, this class is not instantiated or destroyed in
// the same manner as other BDataIO subclasses.  It cannot be created directly.  It
// can only be instantiated from a BMessage archive.  This means that you must always
// receive dehydrated archives from NetPositive and cannot arbitrarily create them on
// your own.  You must make a separate request of NetPositive for each BNetPositiveStreamIO
// that is created; Please resist the temptation to reverse-engineer the archive format
// and create instances on your own.

// Also unique is the fact that BNetPositiveStreamIO is reference-counted; you cannot
// delete instances directly.  This is because the instance could be shared with NetPositive
// if you are running in its address space.  Please resist the temptation to cast it to
// something else and delete it directly; bad things will happen.

// If you are writing an application that accepts NetPositive plug-ins, please contact
// Be to get help with creating and handling BNetPositiveStreamIO instances from the
// other side of the pipe.

// NetPositive will always use ReadAt() or WriteAt() calls when reading from or writing to the
// other end of the stream, so you are free to use Seek(), Position(), Read(), and Write() calls
// without having to worry about your stream position getting messed up.

// NEEDED - a mechanism for the stream to let you know when there is no data left to read.
// NEEDED - If it's a cross-address-space stream, know which side of the fence this instance
// is on and disallow reads/writes/other operations as allowable.


class BNetPositiveStreamIO : public BPositionIO {
public:
						BNetPositiveStreamIO();

		void			Reference();
		void			Dereference();
		
virtual ssize_t			ReadWithTimeout(void *buffer, size_t size, bigtime_t timeout = B_INFINITE_TIMEOUT);
virtual ssize_t			ReadAtWithTimeout(off_t pos, void *buffer, size_t size, bigtime_t timeout = B_INFINITE_TIMEOUT);

virtual ssize_t			ReadAt(off_t pos, void *buffer, size_t size);
virtual ssize_t			WriteAt(off_t pos, const void *buffer, size_t size);

virtual off_t			Seek(off_t position, uint32 seek_mode);
virtual off_t			Position() const;

virtual status_t		SetSize(off_t size);

// BPositionIO's Size() call will return the number of bytes that have been written to the stream so far.
// ContentLength() will return the total number of bytes in the stream.
virtual ssize_t			ContentLength() const;
virtual ssize_t			AmountWritten() const;
virtual const char*		ContentType() const;
virtual void			SetContentLength(ssize_t length);
virtual void			SetContentType(const char *type);
virtual void			SetError(uint32 errorCode);
virtual uint32			GetError() const;

/*----- Private or reserved ---------------*/
private:

// Constructors and destructors are private.  You can only create this object
// from an archive, and since it is reference-counted, you cannot delete it
// directly.
						BNetPositiveStreamIO(const BNetPositiveStreamIO& copy);
virtual					~BNetPositiveStreamIO();
		BNetPositiveStreamIO& operator=(const BNetPositiveStreamIO &);

virtual	void			_ReservedNetPositiveStreamIO1();
virtual	void			_ReservedNetPositiveStreamIO2();
virtual void			_ReservedNetPositiveStreamIO3();
virtual void			_ReservedNetPositiveStreamIO4();

		uint32			fRefCount;
		
		size_t			fBlockSize;
		size_t			fMallocSize;
		size_t			fLength;
		char			*fData;
		off_t			fPosition;
		uint32			fError;
		
		size_t			fContentLength;
		BString			fContentType;

		int32			_reserved[2];
};

#endif /* _NETPOSITIVESTREAMIO_H */
