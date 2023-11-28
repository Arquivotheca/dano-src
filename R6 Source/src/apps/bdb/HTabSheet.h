/*
	Copyright 1997, Hekkelman Programmatuur

*/

#ifndef HTABSHEET_H
#define HTABSHEET_H

#include <SupportDefs.h>
#include <View.h>
#include <List.h>
#include <ListView.h>

const ulong msg_Flip = 'flip';

class HTabSheet : public BView {
public:
		HTabSheet(BRect frame, const char *name);
		~HTabSheet();
	
virtual void MouseDown(BPoint where);
virtual void Draw(BRect update);
	
BRect ClientArea();
	
BView* AddSheet(const char *name, const char *desc = NULL);
	
		int CurrentTab()	{ return fCurrent; };
	
private:
		
		void FlipTo(int page);
		void MessageReceived(BMessage *msg);
		void AttachedToWindow();

		BRect fClientArea, fListArea;
		BList fPanes;
		BList fDescs;
		BListView *fEntries;	
		int fCurrent;
};

#endif
