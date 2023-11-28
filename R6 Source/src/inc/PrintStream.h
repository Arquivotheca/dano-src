// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINT_STREAM_H_
#define _PRINT_STREAM_H_

#include <string.h>
#include <List.h>
#include <File.h>
#include <Picture.h>
#include <Rect.h>
#include <Point.h>
#include <Message.h>
#include <SupportDefs.h>
#include <PrintJob.h>

#include <print/PrintJobSettings.h>


namespace BPrivate
{
	struct print_file_header
	{ // Needed by printer drivers
		int32	version;
		int32	page_count;
		off_t	first_page;
		int32	_reserved_3_;
		int32	_reserved_4_;
		int32	_reserved_5_;
	};

	struct _page_header_
	{
		_page_header_ () : pictureCount(0), nextPage(0), scale(1.0f) {
			memset(reserved, 0, sizeof(reserved));
		}
		int32 pictureCount;
		off_t nextPage;
		float scale;
		int32 reserved[9];
	};
	
	struct _page_pics_
	{
		_page_pics_(const BPoint& w, const BRect& r, BPicture *p) : where(w), rect(r), picture(p) { }
		BPoint		where;
		BRect		rect;
		BPicture*	picture;
	};

} using namespace BPrivate;


class BPrintStream
{
public:
			BPrintStream(BFile *file);
			BPrintStream(const BMessage *msg);
	virtual ~BPrintStream();

	const 		BMessage& Settings() const;
	
	int32		NbPages(void) const;
	status_t	GotoPage(uint32 page_number);

	float		Scale(void) const;
	uint32		NbPictures(void) const;
	status_t 	GotoPicture(uint32 picture);

	status_t	ReadPoints(BPoint *p) const;
	status_t	ReadClips(BRect *r) const;
	BPicture *	ReadPicture(void) const;
	status_t	GetRidOfPicture(BPicture *picture);

private:
	BPrintStream();
	BPrintStream(const BPrintStream &);
	BPrintStream& operator = (const BPrintStream&);
	status_t init_data(void);

	BFile *fSpoolFile;
//	BList *fPageOffsetList;
	BList *fPageHeaderList;
	BList *fPictureList;

	bool fFileMode;
	uint32 fFirstPage;
	uint32 fLastPage;
	uint32 fCurrentPage;
	uint32 fCurrentPicture;
	BPrintJobSettings fSettings;
	print_file_header fHeader;
	_page_header_ *fPageHeaderArray;
	off_t fFirstPageOffset;
};

#endif
