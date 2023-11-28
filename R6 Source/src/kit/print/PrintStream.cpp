//******************************************************************************
//
//	File:		PrintStream.cpp
//
//	Written by:	Mathias Agopian
//
//	Copyright 2000-2001, Be Incorporated
//
//******************************************************************************

#include <stdio.h>
#include "PrintStream.h"

BPrintStream::BPrintStream(BFile *file)
	: 	fSpoolFile(file),
		fFileMode(true),
		fFirstPage(0),
		fLastPage(0),
		fCurrentPage(0),
		fCurrentPicture(0),
		fPageHeaderArray(NULL),
		fFirstPageOffset(0)
{
	BMessage settings;
	fSpoolFile->Seek(0, SEEK_SET);

	// Read the spool's header
	fSpoolFile->Read(&fHeader, sizeof(fHeader));

	// Then read the Setting message
	settings.Unflatten(fSpoolFile);
	
	// And go to the first page header
	if (fHeader.FirstPage() != -1) // it's an old spoolfile !?
		fSpoolFile->Seek(fHeader.FirstPage(), SEEK_SET);

	// init internal data
	fSettings.SetSettings(settings);
	init_data();
}

BPrintStream::BPrintStream(const BMessage *msg)
	:	fSpoolFile(NULL),
		fFileMode(false),
		fFirstPage(0),
		fLastPage(0),
		fCurrentPage(0),
		fCurrentPicture(0),
		fPageHeaderArray(NULL),
		fFirstPageOffset(0)
{
	BMessage settings;
	msg->FindMessage("setup_msg", &settings);
	msg->FindPointer("header_list", (void **)&fPageHeaderList);
	msg->FindPointer("picture_list", (void **)&fPictureList);
	fHeader.page_count = msg->FindInt32("page_count");
	fSettings.SetSettings(settings);
	init_data();
}

BPrintStream::~BPrintStream()
{
	if (fFileMode) // Delete the header list only in filemode
		delete fPageHeaderList;
	delete [] fPageHeaderArray;
}

status_t BPrintStream::init_data()
{
	fFirstPage = fSettings.FirstPage();
	fLastPage = fFirstPage + NbPages() - 1;
	fFirstPageOffset = 0;

	if (fFileMode)
	{
		fFirstPageOffset = fSpoolFile->Position();
		fPageHeaderList = new BList;
		fPageHeaderArray = new _page_header_[NbPages()];

		// Read all page headers
		for (int32 i=0 ; i<NbPages() ; i++)
		{
			fSpoolFile->Read(fPageHeaderArray+i, sizeof(_page_header_));
			fPageHeaderList->AddItem((void *)(fPageHeaderArray+i));
			fSpoolFile->Seek(fPageHeaderArray[i].nextPage, SEEK_SET);
		}
	}
	return B_OK;
}

const BMessage& BPrintStream::Settings() const
{
	return fSettings.Message();
}
	
int32 BPrintStream::NbPages(void) const
{
	return fHeader.page_count;
}

status_t BPrintStream::GotoPage(uint32 page_number)
{
	page_number -= fFirstPage;
	if ((page_number < 0) || (page_number >= NbPages()))
		return B_BAD_INDEX;
	if (fFileMode)
	{
		off_t offset = fFirstPageOffset + sizeof(_page_header_);
		if (page_number > 0)
			offset = static_cast<_page_header_ *>(fPageHeaderList->ItemAt(page_number - 1))->nextPage + sizeof(_page_header_);
		if (offset == 0)	// Sanity check
			return B_ERROR;
		fSpoolFile->Seek(offset, SEEK_SET);
	}
	fCurrentPage = page_number;
	fCurrentPicture = 0;
	return B_OK;
}

uint32 BPrintStream::NbPictures(void) const
{
	return static_cast<_page_header_ *>(fPageHeaderList->ItemAt(fCurrentPage))->pictureCount;
}

status_t BPrintStream::GotoPicture(uint32 picture)
{
	if (fFileMode == false)
	{
		fCurrentPicture = picture;
		return B_OK;
	}
	return B_NOT_ALLOWED;
}

status_t BPrintStream::ReadPoints(BPoint *p) const
{
	if (fFileMode)
	{
		fSpoolFile->Read(p, sizeof(BPoint));
	}
	else
	{
		uint32 index = (uint32)fFirstPageOffset;
		if (fCurrentPage > 0)
			index = static_cast<_page_header_ *>(fPageHeaderList->ItemAt(fCurrentPage - 1))->nextPage;
		const _page_pics_ *pp = static_cast<_page_pics_ *>(fPictureList->ItemAt(index+fCurrentPicture));
		if (!pp)
			return B_ERROR;
		*p = pp->where;
	}
	return B_OK;

}

status_t BPrintStream::ReadClips(BRect *r) const
{
	if (fFileMode)
	{
		fSpoolFile->Read(r, sizeof(BRect));
	}
	else
	{
		uint32 index = (uint32)fFirstPageOffset;
		if (fCurrentPage > 0)
			index = static_cast<_page_header_ *>(fPageHeaderList->ItemAt(fCurrentPage - 1))->nextPage;
		const _page_pics_ *pp = static_cast<_page_pics_ *>(fPictureList->ItemAt(index+fCurrentPicture));
		if (!pp)
			return B_ERROR;
		*r = pp->rect;
	}
	return B_OK;
}

BPicture *BPrintStream::ReadPicture(void) const
{
	if (fFileMode)
	{
		BPicture *picture = new BPicture;
		if (picture->Unflatten(fSpoolFile) == B_OK)
			return picture;
	}
	else
	{
		uint32 index = (uint32)fFirstPageOffset;
		if (fCurrentPage > 0)
			index = static_cast<_page_header_ *>(fPageHeaderList->ItemAt(fCurrentPage - 1))->nextPage;
		const _page_pics_ *pp = static_cast<_page_pics_ *>(fPictureList->ItemAt(index+fCurrentPicture));
		if (pp)
			return pp->picture;
	}
	return NULL;
}

status_t BPrintStream::GetRidOfPicture(BPicture *picture)
{
	if (fFileMode)
	{
		delete picture;
	}
	else
	{ // Nothing to do, the BPicture will be deleted by the BPrintJob.
	}
	return B_OK;
}

