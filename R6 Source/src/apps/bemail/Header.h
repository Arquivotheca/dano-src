//--------------------------------------------------------------------
//	
//	Header.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef HEADER_H
#define HEADER_H

#include <fs_attr.h>
#include <NodeInfo.h>
#include <Point.h>
#include <Rect.h>
#include <View.h>
#include <Window.h>
#include <TextControl.h>
#include "ComboBox.h"

#define HEADER_HEIGHT		 82
#define MIN_HEADER_HEIGHT	(HEADER_HEIGHT - 8)//26)

#define TO_TEXT				"To:"
#define FROM_TEXT			"From:"
#define TO_FIELD_H			 39
#define FROM_FIELD_H		 31
#define TO_FIELD_V			  8
#define TO_FIELD_WIDTH		270
#define FROM_FIELD_WIDTH	280
#define TO_FIELD_HEIGHT		 16

#define SUBJECT_TEXT		"Subject:"
#define SUBJECT_FIELD_H		 18
#define SUBJECT_FIELD_V		 33
#define SUBJECT_FIELD_WIDTH	270
#define SUBJECT_FIELD_HEIGHT 16

#define CC_TEXT				"CC:"
#define CC_FIELD_H			 40
#define CC_FIELD_V			 58
#define CC_FIELD_WIDTH		192
#define CC_FIELD_HEIGHT		 16

#define BCC_TEXT			"BCC:"
#define BCC_FIELD_H			268
#define BCC_FIELD_V			 58
#define BCC_FIELD_WIDTH		197
#define BCC_FIELD_HEIGHT	 16

class	TTextControl;
class	BFile;
class	BMenuField;
class	BMenuItem;
class	BPopupMenu;
class	QPopupMenu;


//====================================================================

class THeaderView : public BBox {
public:

					THeaderView(BRect, BRect, bool, BFile*, bool); 
	virtual void	MessageReceived(BMessage*);
	virtual void	AttachedToWindow(void);
	void			BuildMenus();
	void			SetAddress(BMessage*);
	status_t		LoadMessage(BFile*);

	TTextControl	*fBcc;
	TTextControl	*fCc;
	TTextControl	*fSubject;
	TTextControl	*fTo;

private:		
	void			InitEmailCompletion();
	void			InitGroupCompletion();
	
	bool			fIncoming;
	bool			fResending;
	QPopupMenu		*fBccMenu;
	QPopupMenu		*fCcMenu;
	QPopupMenu		*fToMenu;
	BFile			*fFile;
	BStringView		*fDate;
	BDefaultChoiceList emailList;
};


//====================================================================

class TTextControl : public BComboBox {
public:

					TTextControl(BRect, char*, BMessage*, bool, bool, 
					  int32 resizingMode = B_FOLLOW_NONE);
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage*);

private:

	bool			fIncoming;
	bool			fResending;
	char			fLabel[100];
	int32			fCommand;
};

#endif
