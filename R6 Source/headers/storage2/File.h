/***************************************************************************
//
//	File:			File.h
//
//	Description:	BFile class
//
//	Copyright 1992-98, Be Incorporated, All Rights Reserved.
//
***************************************************************************/


#ifndef _STORAGE2_FILE_H
#define _STORAGE2_FILE_H

#include <fcntl.h>
#include <sys/stat.h>
#include <support2/SupportDefs.h>
#include <support2/IBinder.h>
#include <support2/IStorage.h>
#include <storage2/Node.h>

namespace B {
namespace Storage2 {

class BFile : public BNode, public IStorage
{
	public:
		B_STANDARD_ATOM_TYPEDEFS(BFile)

							BFile();
							BFile(const entry_ref *ref, uint32 open_mode);
							BFile(const BEntry *entry, uint32 open_mode);
							BFile(const char *path, uint32 open_mode);
							BFile(const BDirectory *dir, const char *path,
								  uint32 open_mode);
							BFile(const BFile &file);
		
		virtual				~BFile();
		
				status_t	SetTo(const entry_ref *ref, uint32 open_mode);
				status_t	SetTo(const BEntry *entry, uint32 open_mode);
				status_t	SetTo(const char *path, uint32 open_mode);
				status_t	SetTo(const BDirectory *dir, const char *path,
								uint32 open_mode);
								  
				bool		IsReadable() const;
				bool		IsWritable() const;
		
		virtual	off_t		Size() const;
		virtual	status_t	SetSize(off_t size);

		virtual	ssize_t		ReadAtV(off_t position, const struct iovec *vector, ssize_t count);
		virtual	ssize_t		WriteAtV(off_t position, const struct iovec *vector, ssize_t count);
		virtual	status_t	Sync();
		
				BFile &		operator=(const BFile &file);
		
	protected:
		// TO DO: Implement LStorage and RStorage.
		virtual	atom_ptr<IBinder>		AsBinderImpl()			{ return NULL; }
		virtual	atom_ptr<const IBinder>	AsBinderImpl() const	{ return NULL; }
		
	private:
		
				/* FBC */
		virtual	void		_PhiloFile1();
		virtual	void		_PhiloFile2();
		virtual	void		_PhiloFile3();
		virtual	void		_PhiloFile4();
		virtual	void		_PhiloFile5();
		virtual	void		_PhiloFile6();
		virtual	void		close_fd();
		
				uint32		_philoData[8];
				uint32		fMode;
};

} } // namespace B::Storage2

#endif	// _STORAGE2_FILE_H
