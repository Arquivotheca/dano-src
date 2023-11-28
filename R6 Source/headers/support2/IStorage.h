/***************************************************************************
//
//	File:			support2/IStorage.h
//
//	Description:	Abstract interface to a block of raw data.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef	_SUPPORT2_ISTORAGE_H
#define	_SUPPORT2_ISTORAGE_H

#include <sys/uio.h>
#include <support2/SupportDefs.h>
#include <support2/IInterface.h>

namespace B {
namespace Support2 {

/*-------------------------------------------------------------*/
/*------- IStorage Class --------------------------------------*/

class IStorage : public IInterface
{
	public:

		// TODO: implement binder protocol
		B_STANDARD_ATOM_TYPEDEFS(IStorage)

				// Get and set the number of bytes in the store.
		virtual	off_t		Size() const = 0;
		virtual	status_t	SetSize(off_t size) = 0;

				// Read the bytes described by "iovec" from location
				// "position" in the storage.  Returns the number of
				// bytes actually read, or a negative error code.
		virtual	ssize_t		ReadAtV(off_t position, const struct iovec *vector, ssize_t count) = 0;
		
				// Convenience for reading a vector of one buffer.
				ssize_t		ReadAt(off_t position, void* buffer, size_t size);
				
				// Write the bytes described by "iovec" at location
				// "position" in the storage.  Returns the number of
				// bytes actually written, or a negative error code.
		virtual	ssize_t		WriteAtV(off_t position, const struct iovec *vector, ssize_t count) = 0;
		
				// Convenience for reading a vector of one buffer.
				ssize_t		WriteAt(off_t position, const void* buffer, size_t size);
		
				// Make sure all data in the storage is written to its
				// physical device.  Returns B_OK if the data is safely
				// stored away, else an error code.
		virtual	status_t	Sync() = 0;
};

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline ssize_t IStorage::ReadAt(off_t position, void *buffer, size_t size)
{
	iovec v;
	v.iov_base = buffer;
	v.iov_len = size;
	return ReadAtV(position, &v,1);
}

inline ssize_t IStorage::WriteAt(off_t position, const void *buffer, size_t size)
{
	iovec v;
	v.iov_base = const_cast<void*>(buffer);
	v.iov_len = size;
	return WriteAtV(position, &v,1);
}

/*--------------------------------------------------------------*/

} } // namespace B::Support2

#endif	// _SUPPORT2_ISTORAGE_H
