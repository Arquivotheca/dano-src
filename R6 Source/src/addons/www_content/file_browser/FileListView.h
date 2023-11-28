/*-----------------------------------------------------------------*/
//
//	File:		FileListView.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef _FILE_LIST_VIEW_H_
#define _FILE_LIST_VIEW_H_

#include <Font.h>
#include <ListView.h>
#include <Path.h>
#include <String.h>

#include "Parameters.h"


using namespace Wagner;

enum EDITOR_MESSAGES
{
	eDetachedFromWindow	= 'DTCH',
	eEditAccepted 		= 'EXCP',
	eEditCanceled		= 'CNCL'
};

enum PROTOCOL
{
	eLocal				= 0,
	eFTP,
	eHTTP
};

enum VIEW_TYPE
{
	eMediaView			= 0,
	eImageView,
	eTextView,
	eOtherView
};

class FileListView;
class FileListItem;


/*=================================================================*/

class FilesView : public BView
{
	public:
							FilesView			(BRect frame,
												 const char* name,
												 drawing_parameters*);
							~FilesView			();
		virtual void		AddFile				(BString* path,
												 BString* name,
												 const char* mime,
												 time_t time,
												 off_t size,
												 PROTOCOL) = 0;
		virtual void		Draw				(BRect);
		virtual void		ItemChanged			(FileListItem*) = 0;
		virtual void		MakeEmpty			();

		void				CopyFiles			(const char* target);
		void				DeleteFiles			();
		void				EditFiles			();
		bool				Editing				();
		void				GetFiles			(BMessage*);
		void				OpenFiles			();
		void				DrawTab				(float left,
												 float right,
												 BString label,
												 int32 sorted,
												 bool draw_right_edge = true);
		FileListView*		List				()
													{ return fList; };

		drawing_parameters*	fParameters;
		BBitmap*			fOffscreen;
		BView*				fOffscreenView;

	private:
		FileListView*		fList;
};


/*=================================================================*/

class FileListView : public BListView
{
	public:
							FileListView		(BRect frame,
												 drawing_parameters*);
							~FileListView		();
		virtual void		AllAttached			();
		virtual void		Draw				(BRect);
		virtual void		DetachedFromWindow	();
		virtual void		MessageReceived		(BMessage*);
		virtual void		MouseDown			(BPoint);
		virtual void		MouseMoved			(BPoint,
												 uint32,
												 const BMessage*);
		virtual void		MouseUp				(BPoint);

		drawing_parameters*	fParameters;
		BBitmap*			fOffscreen;
		BView*				fOffscreenView;

	private:
		void				SetButtonState		();

		bool				fDeselect;
		bool				fDragging;
		int32				fSelectedItem;
		BPoint				fStart;
		FileListItem*		fRolledOverItem;
};


/*=================================================================*/

class FileListItem : public BListItem
{
	public:
							FileListItem		(BString*,
												 BString*,
												 const char* type,
												 PROTOCOL,
												 drawing_parameters*,
												 FilesView*);
		virtual void		EditorCallBack		(BMessage*);
		virtual VIEW_TYPE	ViewType			() = 0;
		virtual void		RollOver			(bool);
		virtual	void		Edit				(BView*,
												 BRect) = 0;
		virtual bool		Editing				() = 0;
		const char*			Path				()
													{ return fPath.String(); };
		const char*			Leaf				()
													{ return fName.String(); };
		PROTOCOL			Protocol			()
													{ return fProtocol; };
		const char*			Scheme				();
		status_t			Rename				(const char*);
		const char*			Type				()
													{ return fFileType.String(); };
		void				Update				(BView*,
												 const BFont*);

		drawing_parameters*	fParameters;
		FilesView*			fFilesView;

	protected:
		bool				fRollOver;

	private:
		BString				fName;
		BString				fPath;
		BString				fFileType;
		PROTOCOL			fProtocol;
};
#endif
