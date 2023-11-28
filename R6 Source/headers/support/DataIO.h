/******************************************************************************
/
/	File:			DataIO.h
/
/	Description:	Pure virtual BDataIO and BPositioIO classes provide
/					the protocol for Read()/Write()/Seek().
/
/					BMallocIO and BMemoryIO classes implement the protocol,
/					as does BFile in the Storage Kit.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef	_DATA_IO_H
#define	_DATA_IO_H

#include <BeBuild.h>
#include <SupportDefs.h>

class BMessage;
class BString;

struct meta_info
{
	uint32		type;
	size_t		count;
	off_t		size;
};

/*-----------------------------------------------------------------*/
/*------- BDataIO Class -------------------------------------------*/

class BDataIO {
public:
					BDataIO();
virtual				~BDataIO();

virtual	ssize_t		Read(void *buffer, size_t size) = 0;
virtual	ssize_t		Write(const void *buffer, size_t size) =0;

/*----- Public meta data API ---------------*/

		ssize_t		WriteMetaData(	const char *in_name, type_code in_type,
									const void *in_buf, size_t in_size);
		ssize_t		WriteMetaData(	const char *in_name, type_code in_type, int32 in_index,
									const void *in_buf, size_t in_size);
		ssize_t		WriteMetaData(	const char *in_name, type_code in_type, int32 in_index,
									off_t in_offset, const void *in_buf, size_t in_size);
		
		ssize_t		ReadMetaData(	const char *in_name, type_code in_type,
									void *out_buf, size_t in_size) const;
		ssize_t		ReadMetaData(	const char *in_name, type_code in_type, int32 in_index,
									void *out_buf, size_t in_size) const;
		ssize_t		ReadMetaData(	const char *in_name, type_code in_type, int32 in_index,
									off_t in_offset, void *out_buf, size_t in_size) const;
		
		status_t	RemoveMetaData(	const char *in_name);
		status_t	RemoveMetaData(	const char *in_name, int32 in_index);
		status_t	RemoveAllMetaData();
		
		status_t	GetMetaDataInfo(	const char *in_name,
										meta_info *out_info, BString *out_name = NULL) const;
		status_t	GetMetaDataInfo(	const char *in_name, int32 in_index,
										meta_info *out_info, BString *out_name = NULL) const;
		status_t	GetNextMetaData(	void** cookie,
										meta_info *out_info, BString *out_name = NULL) const;
		
/*----- Implementor's meta data API ---------------*/

virtual	ssize_t		MetaWrite(	const char *in_name, type_code in_type,
								int32 in_index, off_t in_offset,
								const void *in_buf, size_t in_size);
virtual	ssize_t		MetaRead(	const char *in_name, type_code in_type,
								int32 in_index, off_t in_offset,
								void *out_buf, size_t in_size) const;
virtual	status_t	MetaRemove(	const char *in_name, int32 in_index);
virtual	status_t	MetaGetInfo(const char *in_name, int32 in_index,
								meta_info *out_info, BString *out_name,
								void **inout_cookie) const;

/*----- Private or reserved ---------------*/
private:

#if !_PR3_COMPATIBLE_
virtual	void		_ReservedDataIO5();
virtual	void		_ReservedDataIO6();
virtual	void		_ReservedDataIO7();
virtual	void		_ReservedDataIO8();
virtual	void		_ReservedDataIO9();
virtual	void		_ReservedDataIO10();
virtual	void		_ReservedDataIO11();
virtual	void		_ReservedDataIO12();
#endif

					BDataIO(const BDataIO &);
		BDataIO		&operator=(const BDataIO &);

		int32		_reserved[2];
};

/*---------------------------------------------------------------------*/
/*------- BPositionIO Class -------------------------------------------*/

class BPositionIO : public BDataIO {
public:
					BPositionIO();
virtual				~BPositionIO();

virtual	ssize_t		Read(void *buffer, size_t size);
virtual	ssize_t		Write(const void *buffer, size_t size);

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size) = 0;
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size) = 0;

virtual off_t		Seek(off_t position, uint32 seek_mode) = 0;
virtual	off_t		Position() const = 0;

virtual status_t	SetSize(off_t size);

/*----- Private or reserved ---------------*/
private:
virtual	void		_ReservedPositionIO1();
virtual	void		_ReservedPositionIO2();
virtual void		_ReservedPositionIO3();
virtual void		_ReservedPositionIO4();

#if !_PR3_COMPATIBLE_
virtual	void		_ReservedPositionIO5();
virtual	void		_ReservedPositionIO6();
virtual	void		_ReservedPositionIO7();
virtual	void		_ReservedPositionIO8();
virtual	void		_ReservedPositionIO9();
virtual	void		_ReservedPositionIO10();
virtual	void		_ReservedPositionIO11();
virtual	void		_ReservedPositionIO12();
#endif

		int32		_reserved[2];
};

/*-------------------------------------------------------------------*/
/*------- BMallocIO Class -------------------------------------------*/

class BMallocIO : public BPositionIO {
public:
					BMallocIO();
					BMallocIO(const BMallocIO &);

virtual				~BMallocIO();

		BMallocIO	&operator=(const BMallocIO &);
		
		bool		operator<(const BMallocIO &) const;
		bool		operator<=(const BMallocIO &) const;
		bool		operator==(const BMallocIO &) const;
		bool		operator!=(const BMallocIO &) const;
		bool		operator>=(const BMallocIO &) const;
		bool		operator>(const BMallocIO &) const;
		
virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;
virtual status_t	SetSize(off_t size);

		void		SetBlockSize(size_t blocksize);

const	void		*Buffer() const;
		size_t		BufferLength() const;

		// Return Buffer(), ensuring it is \0-terminated.
		// (Termination not included in the string/buffer length.)
const	char		*AsString() const;

		int			Compare(const BMallocIO &) const;

/*----- Private or reserved ---------------*/
private:

virtual	void		_ReservedMallocIO1();
virtual	void		_ReservedMallocIO2();

		void		Copy(const BMallocIO &);
		
		size_t		fBlockSize;
		size_t		fMallocSize;
		size_t		fLength;
		char		*fData;
		off_t		fPosition;
		uint32		_reserved[1];
};

/*-------------------------------------------------------------------*/
/*------- BMemoryIO Class -------------------------------------------*/

class BMemoryIO : public BPositionIO {
public:
					BMemoryIO(void *p, size_t len);
					BMemoryIO(const void *p, size_t len);
virtual				~BMemoryIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

virtual	off_t		Seek(off_t pos, uint32 seek_mode);
virtual off_t		Position() const;

virtual	status_t	SetSize(off_t size);

/*----- Private or reserved ---------------*/
private:
virtual	void		_ReservedMemoryIO1();
virtual	void		_ReservedMemoryIO2();

					BMemoryIO(const BMemoryIO &);
		BMemoryIO	&operator=(const BMemoryIO &);

		bool		fReadOnly;
		char		*fBuf;
		size_t		fLen;
		size_t		fPhys;
		size_t		fPos;

		int32		_reserved[1];
};

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline ssize_t BDataIO::WriteMetaData(	const char *in_name, type_code in_type,
										const void *in_buf, size_t in_size)
{
	return MetaWrite(in_name, in_type, 0, 0, in_buf, in_size);
}

inline ssize_t BDataIO::WriteMetaData(	const char *in_name, type_code in_type, int32 in_index,
										const void *in_buf, size_t in_size)
{
	return MetaWrite(in_name, in_type, in_index, 0, in_buf, in_size);
}

inline ssize_t BDataIO::WriteMetaData(	const char *in_name, type_code in_type, int32 in_index,
										off_t in_offset, const void *in_buf, size_t in_size)
{
	return MetaWrite(in_name, in_type, in_index, in_offset, in_buf, in_size);
}
		
inline ssize_t BDataIO::ReadMetaData(	const char *in_name, type_code in_type,
										void *out_buf, size_t in_size) const
{
	return MetaRead(in_name, in_type, 0, 0, out_buf, in_size);
}

inline ssize_t BDataIO::ReadMetaData(	const char *in_name, type_code in_type, int32 in_index,
										void *out_buf, size_t in_size) const
{
	return MetaRead(in_name, in_type, in_index, 0, out_buf, in_size);
}

inline ssize_t BDataIO::ReadMetaData(	const char *in_name, type_code in_type, int32 in_index,
										off_t in_offset, void *out_buf, size_t in_size) const
{
	return MetaRead(in_name, in_type, in_index, in_offset, out_buf, in_size);
}
		
inline status_t BDataIO::RemoveMetaData(const char *in_name)
{
	return MetaRemove(in_name, -1);
}

inline status_t BDataIO::RemoveMetaData(const char *in_name, int32 in_index)
{
	return MetaRemove(in_name, in_index);
}

inline status_t BDataIO::RemoveAllMetaData()
{
	return MetaRemove(NULL, -1);
}

inline status_t BDataIO::GetMetaDataInfo(	const char *in_name,
											meta_info *out_info, BString *out_name) const
{
	return MetaGetInfo(in_name, 0, out_info, out_name, NULL);
}

inline status_t BDataIO::GetMetaDataInfo(	const char *in_name, int32 in_index,
											meta_info *out_info, BString *out_name) const
{
	return MetaGetInfo(in_name, in_index, out_info, out_name, NULL);
}

inline status_t BDataIO::GetNextMetaData(	void** cookie,
											meta_info *out_info, BString *out_name) const
{
	return MetaGetInfo(NULL, -1, out_info, out_name, cookie);
}

inline bool BMallocIO::operator<(const BMallocIO &o) const
{
	return Compare(o) < 0;
}

inline bool BMallocIO::operator<=(const BMallocIO &o) const
{
	return Compare(o) <= 0;
}

inline bool BMallocIO::operator==(const BMallocIO &o) const
{
	return Compare(o) == 0;
}

inline bool BMallocIO::operator!=(const BMallocIO &o) const
{
	return Compare(o) != 0;
}

inline bool BMallocIO::operator>=(const BMallocIO &o) const
{
	return Compare(o) >= 0;
}

inline bool BMallocIO::operator>(const BMallocIO &o) const
{
	return Compare(o) > 0;
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _DATA_IO_H */
