//--------------------------------------------------------------------
//	
//	Enclosures.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ENCLOSURES_H
#define ENCLOSURES_H

#include <Bitmap.h>
#include <Box.h>
#include <File.h>
#include <ListView.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <ScrollView.h>
#include <View.h>
#include <Volume.h>

#define ENCLOSURES_HEIGHT	 65

#define ENCLOSE_TEXT		"Enclosures:"
#define ENCLOSE_TEXT_H		 7
#define ENCLOSE_TEXT_V		 3
#define ENCLOSE_FIELD_H		 67
#define ENCLOSE_FIELD_V		 3

class	TListView;
class	TMailWindow;
class	TScrollView;


//====================================================================

class TEnclosuresView : public BView {
public:

					TEnclosuresView(BRect, BRect); 
	virtual	void	Draw(BRect);
	virtual void	MessageReceived(BMessage*);
	void			Focus(bool);

	TListView		*fList;

private:

	bool			fFocus;
	float			fOffset;
	TMailWindow		*fWindow;
};


//====================================================================

class TListView : public BListView {
public:

					TListView(BRect, TEnclosuresView*);
	virtual	void	AttachedToWindow();
	virtual void	MakeFocus(bool);

private:

	TEnclosuresView	*fParent;
};


//====================================================================

class TListItem : public BListItem {
public:

					TListItem(entry_ref*);
	virtual void	DrawItem(BView*, BRect, bool);
	virtual	void	Update(BView*, const BFont*);
	entry_ref*		Ref();

private:

	typedef BListItem	inherited;

	entry_ref*		fRef;
};
#endif
