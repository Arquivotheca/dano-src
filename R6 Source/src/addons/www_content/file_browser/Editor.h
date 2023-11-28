/*-----------------------------------------------------------------*/
//
//	File:		Editor.h
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef EDITOR_H
#define EDITOR_H

#include <MessageFilter.h>
#include <String.h>
#include <TextControl.h>

#include "FileListView.h"

class KeyFilter;


/*============================ Editor ============================*/

class Editor : public BTextControl
{
	public:
							Editor				(FileListItem*,
												 BRect,
												 BRect,
												 BString*,
												 drawing_parameters*,
												 bool,
												 uint32 what = eEditAccepted);
		void				AttachedToWindow	();
		void				MessageReceived		(BMessage*);
		filter_result		Filter				(BMessage *message,
												 BHandler **target);

	private:
		void				Accept				();

		bool				fIsFile;
		uint32				fWhat;
		FileListItem*		fCaller;
		drawing_parameters*	fParameters;
};


//====================================================================

class EditFilter : public BMessageFilter
{
	public:
							EditFilter			(FileListItem*,
												 BRect);
		filter_result		Filter				(BMessage*,
												 BHandler**);

	private:
		BRect				fItem;
		FileListItem*		fTarget;
};


/*============================ Editor ============================*/

enum tracking
{
	eTrackingNone = 0,
	eTrackingAccept,
	eTrackingReject,
	eTrackingUnknown
};


class EditControls : public BView
{
	public:
							EditControls		(FileListItem*,
												 BRect,
												 BRect,
												 drawing_parameters*);
		virtual void		Draw				(BRect);
		virtual void		MouseDown			(BPoint);
		virtual void		MouseMoved			(BPoint,
												 uint32,
												 const BMessage*);
		virtual void		MouseUp				(BPoint);

	private:
		int32				fAcceptState;
		int32				fRejectState;
		int32				fTracking;
		BRect				fItem;
		FileListItem*		fCaller;
		drawing_parameters*	fParameters;
};
#endif
