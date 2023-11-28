/*-----------------------------------------------------------------*/
//
//	File:		ImageTextFilesView.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef _IMAGE_TEXT_FILES_VIEW_H_
#define _IMAGE_TEXT_FILES_VIEW_H_

#include "FileListView.h"
#include "Editor.h"

#include <Bitmap.h>
#include <Path.h>
#include <String.h>
#include <View.h>

struct file_field_offsets
{
	float	ornament;
	float	name;
	float	modified;
	float	size;
	float	controls;
};

enum eFileFields
{
	eFileNone = 0,
	eFileOrnament,
	eFileName,
	eFileModification,
	eFileSize,
	eFileControls
};

enum eFileSortFields
{
	eFileOrnamentAscending = 0,
	eFileOrnamentDescending,
	eFileNameAscending,
	eFileNameDescending,
	eFileModificationAscending,
	eFileModificationDescending,
	eFileSizeAscending,
	eFileSizeDescending,
	eFileControlsAscending,
	eFileControlsDescending
};

class TFileListItem;


/*=================================================================*/

class ImageTextFilesView : public FilesView
{
	public:
							ImageTextFilesView	(BRect frame,
												 drawing_parameters*,
												 VIEW_TYPE);
		void				AddFile				(BString* path,
												 BString* name,
												 const char* type,
												 time_t modified,
												 off_t size,
												 PROTOCOL);
		void				Draw				(BRect);
		void				ItemChanged			(FileListItem*);
		void				MouseDown			(BPoint);
		void				MouseMoved			(BPoint,
												 uint32,
												 const BMessage*);
		void				MouseUp				(BPoint);

	private:
		void				DrawTabs			();
		int32				InsertItem			(FileListView*,
												 TFileListItem*);
		void				Resize				(eFileFields,
												 BPoint,
												 float);
		void				SetPosition			(float);
		void				Sort				(eFileSortFields);

		float				fOriginalPosition;
		BPoint				fStart;
		eFileFields			fResizing;
		eFileSortFields		fSort;
		file_field_offsets	fOffsets;
		VIEW_TYPE			fViewType;
};


//--------------------------------------------------------------------

class TFileListItem : public FileListItem
{
	public:

							TFileListItem		(BView*,
												 BString*,
												 BString*,
												 const char* type,
												 PROTOCOL,
												 const time_t modified,
												 const off_t size,
												 file_field_offsets*,
												 drawing_parameters*,
												 BBitmap*,
												 BView*,
												 FilesView*,
												 VIEW_TYPE);
							~TFileListItem		();
		void				DrawItem			(BView*,
												 BRect,
												 bool);
		void				EditorCallBack		(BMessage*);
		VIEW_TYPE			ViewType			()
													{ return fViewType; };
		void				Edit				(BView*,
												 BRect);
		bool				Editing				()
													{ return fEditor; };
		time_t				Modified			()
													{ return fModified; };
		BString*			Name				()
													{ return &fName; };
		off_t				Size				()
													{ return fSize; };
	private:
		void				RemoveEditor		();

		off_t				fSize;
		time_t				fModified;
		BBitmap*			fOffscreen;
		BString				fName;
		BView*				fEditControls;
		BView*				fOffscreenView;
		BView*				fView;
		file_field_offsets*	fOffsets;
		Editor*				fEditor;
		VIEW_TYPE			fViewType;
};
#endif
