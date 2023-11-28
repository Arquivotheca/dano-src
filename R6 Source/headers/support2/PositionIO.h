/***************************************************************************
//
//	File:			support2/PositionIO.h
//
//	Description:	A BPositionIO converts byte stream operations in to
//					IStorage operations.  In other words, it provides
//					byte input, output, and seeking interfaces to an
//					underlying random-access storage.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef	_SUPPORT2_POSITIONIO_H
#define	_SUPPORT2_POSITIONIO_H

#include <sys/uio.h>
#include <support2/SupportDefs.h>
#include <support2/ByteStream.h>
#include <support2/IStorage.h>

namespace B {
namespace Support2 {

/*---------------------------------------------------------------------*/
/*------- BPositionIO Class -------------------------------------------*/

class BPositionIO : public LByteInput, public LByteOutput, public LByteSeekable
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BPositionIO)

								BPositionIO(IStorage::arg store);
		virtual					~BPositionIO();
		
		virtual	BValue			Inspect(const BValue &which, uint32 flags = 0);
		
		virtual	ssize_t			ReadV(const struct iovec *vector, ssize_t count);
		virtual	ssize_t			WriteV(const struct iovec *vector, ssize_t count);
		virtual	status_t		End();
		virtual	status_t		Sync();
		
		virtual off_t			Seek(off_t position, uint32 seek_mode);
		virtual	off_t			Position() const;

	protected:

								BPositionIO(IStorage *This);

	private:

				IStorage *		m_store;
				off_t			m_pos;
};

/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_POSITIONIO_H */
