/******************************************************************************
/
/	File:			MetaPositionIO.h
/
/	Description:	BPositionIO subclass that provides meta-data storage.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef	_META_POSITION_IO_H
#define	_META_POSITION_IO_H

#include <BeBuild.h>
#include <DataIO.h>

class BMessage;

/*-------------------------------------------------------------------*/
/*------- BMetaPositionIO Class -------------------------------------*/

class BMetaPositionIO : public BPositionIO {
public:
						BMetaPositionIO(BPositionIO* target, bool ownsTarget);
						BMetaPositionIO(const BMetaPositionIO &);

virtual					~BMetaPositionIO();

		BMetaPositionIO	&operator=(const BMetaPositionIO &);
		
virtual	ssize_t			Read(void *buffer, size_t size);
virtual	ssize_t			Write(const void *buffer, size_t size);

virtual	ssize_t			ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t			WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t			Seek(off_t pos, uint32 seek_mode);
virtual off_t			Position() const;
virtual status_t		SetSize(off_t size);

virtual	ssize_t			MetaWrite(	const char *in_name, type_code in_type,
									int32 in_index, off_t in_offset,
									const void *in_buf, size_t in_size);
virtual	ssize_t			MetaRead(	const char *in_name, type_code in_type,
									int32 in_index, off_t in_offset,
									void *out_buf, size_t in_size) const;
virtual	status_t		MetaRemove(	const char *in_name, int32 in_index);
virtual	status_t		MetaGetInfo(const char *in_name, int32 in_index,
									meta_info *out_info, BString *out_name,
									void **inout_cookie) const;

		BMessage*		MetaData();
		
/*----- Private or reserved ---------------*/
private:

virtual	void			_ReservedMetaPositionIO1();
virtual	void			_ReservedMetaPositionIO2();

		BPositionIO*	fTarget;
		bool			fOwnsTarget;
		bool			_reserved_bool1;
		bool			_reserved_bool2;
		bool			_reserved_bool3;
		BMessage*		fMetaData;
		uint32			_reserved[3];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _META_POSITION_IO_H */
