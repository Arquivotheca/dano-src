/******************************************************************************
/
/	File:			BitmapCollection.h
/
/	Copyright 1992-2000, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef	_BITMAP_COLLECTION_H
#define	_BITMAP_COLLECTION_H

#include <OS.h>
#include <String.h>

class BDataIO;
class BBitmap;


namespace BPrivate
{
	struct bitmap_collection_p;
}


class BBitmapCollection
{
public:
			BBitmapCollection(BDataIO *lbx_data, size_t lbx_size);
	virtual ~BBitmapCollection();

	int32 CountBitmaps() const;
	status_t GetBitmapInfo(const char *name, int32 *width, int32 *height) const;
	status_t GetBitmapInfo(int32 index, BString& name, int32 *width=NULL, int32 *height=NULL) const;
	int32 GetIndexForBitmap(const char *name) const;
	BBitmap *GetBitmap(const char *name);
	BBitmap *GetBitmap(int32 index);

	status_t InitCheck() const;

private:
				BBitmapCollection(const BBitmapCollection &);
	BBitmapCollection&	operator = (const BBitmapCollection &);
	BPrivate::bitmap_collection_p *_m_private;
	BPrivate::bitmap_collection_p& _m_rprivate;
	uint32 _reserved[4];

	virtual void _ReservedBitmapCollection0();
	virtual void _ReservedBitmapCollection1();
	virtual void _ReservedBitmapCollection2();
	virtual void _ReservedBitmapCollection3();
};

#endif
