/*-----------------------------------------------------------------*/
//
//	File:		MediaFilesView.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef _MEDIA_FILES_VIEW_H_
#define _MEDIA_FILES_VIEW_H_

#include "FileListView.h"
#include "Editor.h"
#include "ID3.h"

#include <Bitmap.h>
#include <Path.h>
#include <String.h>
#include <View.h>

struct media_field_offsets
{
	float	ornament;
	float	title;
	float	artist;
	float	album;
	float	track;
	float	time;
	float	controls;
};

enum eMediaFields
{
	eMediaNone = 0,
	eMediaOrnament,
	eMediaTitle,
	eMediaArtist,
	eMediaAlbum,
	eMediaTrack,
	eMediaTime,
	eMediaControls
};

enum eMediaEditors
{
	eMediaEditorTitle = 0,
	eMediaEditorArtist,
	eMediaEditorAlbum,
	eMediaEditorTrack,
	eMediaEditorCount
};

enum eMediaSortFields
{
	eMediaOrnamentAscending = 0,
	eMediaOrnamentDescending,
	eMediaTitleAscending,
	eMediaTitleDescending,
	eMediaArtistAscending,
	eMediaArtistDescending,
	eMediaAlbumAscending,
	eMediaAlbumDescending,
	eMediaTrackAscending,
	eMediaTrackDescending,
	eMediaTimeAscending,
	eMediaTimeDescending,
	eMediaControlsAscending,
	eMediaControlsDescending
};

class TMediaListItem;


/*=================================================================*/

class MediaFilesView : public FilesView
{
	public:
							MediaFilesView		(BRect frame,
												 drawing_parameters*);
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
												 TMediaListItem*);
		void				Resize				(eMediaFields,
												 BPoint,
												 float);
		void				SetPosition			(float);
		void				Sort				(eMediaSortFields);

		float				fOriginalPosition;
		BPoint				fStart;
		eMediaFields		fResizing;
		eMediaSortFields	fSort;
		media_field_offsets	fOffsets;
};


//--------------------------------------------------------------------

class TMediaListItem : public FileListItem
{
	public:

							TMediaListItem		(BView*,
												 BString* path,
												 BString* name,
												 const char* type,
												 PROTOCOL,
												 id3_tags*,
												 bool is_mp3,
												 media_field_offsets*,
												 drawing_parameters*,
												 BBitmap*,
												 BView*,
												 FilesView*);
							~TMediaListItem		();
		void				DrawItem			(BView*,
												 BRect,
												 bool);
		void				EditorCallBack		(BMessage*);
		VIEW_TYPE			ViewType			()
													{ return eMediaView; };
		void				Edit				(BView*,
												 BRect);
		bool				Editing				()
													{ return fEditor[0]; };
		BString*			Album				()
													{ return &fTags->album; };
		BString*			Artist				()
													{ return &fTags->artist; };
		BString*			Title				()
													{ return &fTags->title; };
		int64				Time				()
													{ return fTags->length; };
		int32				Track				()
													{ return fTags->track; };
	private:
		void				RemoveControls		();
		void				RemoveEditor		();

		bool				fIsMP3;
		BBitmap*			fOffscreen;
		BView*				fEditControls;
		BView*				fOffscreenView;
		BView*				fView;
		media_field_offsets*fOffsets;
		id3_tags*			fTags;
		Editor*				fEditor[eMediaEditorCount];
};
#endif
