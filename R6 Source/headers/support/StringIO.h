/******************************************************************************
/
/	File:			StringIO.h
/
/	Description:	BDataIO implementation for creating BStrings.
/
/	Copyright 2000, Be Incorporated
/
******************************************************************************/

#ifndef	_STRING_IO_H
#define	_STRING_IO_H

#include <DataIO.h>

/*-------------------------------------------------------------------*/
/*------- BStringIO Class -------------------------------------------*/

class BStringIO : public BPositionIO {
public:
					BStringIO();
					BStringIO(BString* target);
virtual				~BStringIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;
virtual status_t	SetSize(off_t size);

		void		SetBlockSize(size_t blocksize);

const	char*		String() const;
		size_t		StringLength() const;

		void		Attach(BString* target);
		BString*	Detach();

/*----- Private or reserved ---------------*/
private:

virtual	void		_ReservedStringIO1();
virtual	void		_ReservedStringIO2();
virtual	void		_ReservedStringIO3();
virtual	void		_ReservedStringIO4();
virtual	void		_ReservedStringIO5();
virtual	void		_ReservedStringIO6();

					BStringIO(const BStringIO &);
		BStringIO	&operator=(const BStringIO &);

		void		Reset();
		
		size_t		fBlockSize;
		size_t		fMallocSize;
		size_t		fLength;
		off_t		fPosition;
		BString*	fString;
mutable	char*		fData;
		bool		fOwnsString;
		bool		_more_bool[3];
		int32		_reserved[2];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _STRING_IO_H */
