/*-----------------------------------------------------------------*/
//
//	File:		Editor.h
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#ifndef EDITOR_H
#define EDITOR_H

#include <MessageFilter.h>
#include <TextControl.h>

#include "Parameters.h"


enum CONTROL
{
	eRejectControl = 0,
	eAcceptControl
};

#define kEDIT_COMPLETE	'eDIT'


//====================================================================

class FileListItem;

class Editor : public BTextControl
{
	public:
							Editor				(BRect,
												 const char*,
												 BHandler*,
												 drawing_parameters*);
		void				AttachedToWindow	();
		void				WindowActivated		(bool);

	private:
		drawing_parameters*	fParameters;
};


//====================================================================

class EditFilter : public BMessageFilter
{
	public:
							EditFilter			(BHandler*);
		filter_result		Filter				(BMessage*,
												 BHandler**);

	private:
		BHandler*			fTarget;
};


//====================================================================

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
							EditControls		(BRect,
												 BHandler*,
												 drawing_parameters*);
		void				Draw				(BRect);
		void				MouseDown			(BPoint);
		void				MouseMoved			(BPoint,
												 uint32,
												 const BMessage*);
		void				MouseUp				(BPoint);
		void				WindowActivated		(bool);

	private:
		BRect				CalcDrawRect		(BBitmap*,
												 CONTROL);

		bool				fActive;
		int32				fAcceptState;
		int32				fRejectState;
		int32				fTracking;
		BHandler*			fTarget;
		drawing_parameters*	fParameters;
};
#endif

