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

#define B_SPOOL_VERSION		0x00002000

namespace BPrivate
{
	struct spool_header_t
	{
		uint32	version;		// 0x00001000 for old driver, 0x00002000 for new
		int32	page_count;
		off_t	first_page;
		uint8	type;			// valid only if version >= 0x00002000
		uint8	flags;			// valid only if version >= 0x00002000
		uint16	_reserved_3_;
		int32	_reserved_4_;
		int32	_reserved_5_;
		enum {
			PICTURE = 0,
			RAW = 1,
			DIRECT = 2
		};
		uint32 Version() { return version; }
		off_t FirstPage() { return first_page; }
		uint8 Flags() { return flags; }
		uint8 Type() { return type; }
	};

	struct _page_header_
	{
		_page_header_ () : pictureCount(0), nextPage(0) {
			memset(reserved, 0, sizeof(reserved));
		}
		int32 pictureCount;
		off_t nextPage;
		int32 reserved[10];
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

	uint32		NbPictures(void) const;
	status_t 	GotoPicture(uint32 picture);

	status_t	ReadPoints(BPoint *p) const;
	status_t	ReadClips(BRect *r) const;
	BPicture *	ReadPicture(void) const;
	status_t	GetRidOfPicture(BPicture *picture);

	BNode 		*SpoolFile() const { return fSpoolFile; };

private:
	BPrintStream();
	BPrintStream(const BPrintStream &);
	BPrintStream& operator = (const BPrintStream&);
	status_t init_data(void);

	BFile *fSpoolFile;
	BList *fPageHeaderList;
	BList *fPictureList;

	bool fFileMode;
	uint32 fFirstPage;
	uint32 fLastPage;
	uint32 fCurrentPage;
	uint32 fCurrentPicture;
	BPrintJobSettings fSettings;
	spool_header_t fHeader;
	_page_header_ *fPageHeaderArray;
	off_t fFirstPageOffset;
};

#endif
