//--------------------------------------------------------------------
//
//	Status.h
//
//	Written by: Robert Polic
//  			Small changes by Benoit to get it to work for print setup
//				and many new hacks to make it work correctly
//
//	Copyright 1996 Be, Inc. All Rights Reserved.
//
//--------------------------------------------------------------------

#ifndef STATUS_H
#define STATUS_H

#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <Window.h>

// ************************************************************************

class TStatusView : public BBox {
public:

					TStatusView(BRect frame, char* driverName);
					~TStatusView();
	void			AttachedToWindow();
	void			Draw(BRect);
	void			MessageReceived(BMessage*);

	void			PopulateModelList();
	void			UpdateControls();
	
	const char*		PrinterName();
	const char*		PPD();

private:

	struct MPrinterModelItem
	{
		BPath path;
	};

	BStringView*		fModelLabel;
	BOutlineListView*	fModelList;
	BScrollView*		fModelScroller;
	BList*				fModelPathList;
	
	BStringView*	fNameLabel;
	BTextControl*	fPrinterName;
	BButton*		fOkayBtn;
	BButton*		fCancelBtn;
};

// ************************************************************************

class TStatusWindow : public BWindow {
public:
					TStatusWindow(char* driverName);
	void			MessageReceived(BMessage*);
	
	bool			NotDone();
	bool			Result();
	
	const char*		PrinterName();
	const char*		PPD();

private:
	bool			fNotDone;
	bool			fResult;
	TStatusView*	fView;

	char			fPrinterName[128];
	char			fPPD[128];

};

#endif


