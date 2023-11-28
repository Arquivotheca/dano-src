/******************************************************************************
/
/	File:			RandomAccessIO.h
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef	_SUPPORT2_STORAGE_H
#define	_SUPPORT2_STORAGE_H

#include <support2/IStorage.h>
#include <support2/IBinder.h>

namespace B {
namespace Support2 {

/*-------------------------------------------------------------------*/

class BMemoryStore : public IStorage
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BMemoryStore);

							BMemoryStore();
							BMemoryStore(const BMemoryStore &);
							BMemoryStore(void *data, size_t size);
		virtual				~BMemoryStore();
		
				BMemoryStore&operator=(const BMemoryStore &);
				
				bool		operator<(const BMemoryStore &) const;
				bool		operator<=(const BMemoryStore &) const;
				bool		operator==(const BMemoryStore &) const;
				bool		operator!=(const BMemoryStore &) const;
				bool		operator>=(const BMemoryStore &) const;
				bool		operator>(const BMemoryStore &) const;
				
				const void *Buffer() const;
				status_t	AssertSpace(size_t newSize);
				status_t	Copy(const BMemoryStore &);
				int32		Compare(const BMemoryStore &) const;

		virtual	off_t		Size() const;
		virtual	status_t	SetSize(off_t position);

		virtual	ssize_t		ReadAtV(off_t position, const struct iovec *vector, ssize_t count);
		virtual	ssize_t		WriteAtV(off_t position, const struct iovec *vector, ssize_t count);
		virtual	status_t	Sync();

				void		SetBlockSize(size_t blocksize);
		
	protected:

		// TO DO: Implement LStorage and RStorage.
		virtual	atom_ptr<IBinder>		AsBinderImpl()			{ return NULL; }
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const	{ return NULL; }
		
	private:

		virtual	void *		MoreCore(void *oldBuf, size_t newSize);
		virtual	void		FreeCore(void *oldBuf);
				
				size_t		fLength;
				char		*fData;
};

/*-------------------------------------------------------------------*/

class BMallocStore : public BMemoryStore
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BMallocStore);
		
							BMallocStore();
							BMallocStore(const BMemoryStore &);
		
				void		SetBlockSize(size_t blocksize);

	private:

		virtual	void *		MoreCore(void *oldBuf, size_t newSize);
		virtual	void		FreeCore(void *oldBuf);
		
				size_t		fBlockSize;
				size_t		fMallocSize;
};

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline bool BMemoryStore::operator<(const BMemoryStore &o) const
{
	return Compare(o) < 0;
}

inline bool BMemoryStore::operator<=(const BMemoryStore &o) const
{
	return Compare(o) <= 0;
}

inline bool BMemoryStore::operator==(const BMemoryStore &o) const
{
	return Compare(o) == 0;
}

inline bool BMemoryStore::operator!=(const BMemoryStore &o) const
{
	return Compare(o) != 0;
}

inline bool BMemoryStore::operator>=(const BMemoryStore &o) const
{
	return Compare(o) >= 0;
}

inline bool BMemoryStore::operator>(const BMemoryStore &o) const
{
	return Compare(o) > 0;
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_RANDOMACCESSIO_H */
