//--------------------------------------------------------------------
//
//	Status.cpp
//
//	Written by: Robert Polic
//
//	Copyright 1996 Be, Inc. All Rights Reserved.
//
//  Change history:
//
//	01/28/98	MAV001	Changed PPD directory from B_BEOS_SYSTEM_DIRECTORY
//						to B_BEOS_ETC_DIRECTORY.
//
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <printclient.h>
#include <String.h>

#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <ListView.h>
#include <Screen.h>
#include <StringView.h>

#include "Status.h"



//------------------------------------------------------------------
//		parser_interface.cpp
//extern "C" {
//	extern FILE *yyin;
//	extern int yylineno;
//	extern int yyrestart();
//	extern int yyparse();
//	extern int yywrap();
//#include "parser_interface.h"
//};
//
//#undef _PARSER_INTERFACE_H_
//#define PARSETEST
//#include "parser_interface.h"
//
//int test_ppd(const char *ppd_file)
//{
//	FILE *file = fopen(ppd_file, "r");
//	if (file == NULL)
//		return -1;
//	
//	yylineno = 1;
//	yyin = file;
//	yyrestart(file);
//	int rv = yyparse();
//	yywrap();
//
//	fclose(file);
//	return rv;
//}
//------------------------------------------------------------------

const int32 msg_okay = 'okay';
const int32 msg_cancel = 'canc';
const int32 msg_model_single_click = 'sin2';
const int32 msg_model_double_click = 'dbl2';

// ************************************************************************

TStatusWindow::TStatusWindow(char* driverName)
	: BWindow(BRect(0, 0, 400, 315), "Select a printer model", B_MODAL_WINDOW,
		B_NOT_H_RESIZABLE)
{
	BRect r(Bounds());
	r.InsetBy(-1, -1);

	fView = new TStatusView(r, driverName);
	AddChild(fView);

	fPrinterName[0] = fPPD[0];
	fNotDone = true;
	fResult = false;
	
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
}

void
TStatusWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case msg_model_single_click:
			fView->UpdateControls();
			break;
		
		case msg_model_double_click:
			fView->UpdateControls();
			
			// see if everything is filled in
			// if it is close the window and 
			// pass the printer info to the server
			if (fView->PPD())
			{
				fResult = true;
				fNotDone = false;
			}	
			break;

		case msg_okay:
			fResult = true;
			fNotDone = false;
			break;
		case msg_cancel:
			fResult = false;
			fNotDone = false;
			break;
	}
}

// some convenience functions for convenience
// so that we dont have to do obscure view->view->dataMember
//	accessing
const char*
TStatusWindow::PrinterName()
{
	return fView->PrinterName();
}

const char*
TStatusWindow::PPD()
{
	return fView->PPD();
}

bool
TStatusWindow::NotDone()
{
	return fNotDone;
}

bool
TStatusWindow::Result()
{
	return fResult;
}

// ************************************************************************

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

TStatusView::TStatusView(BRect rect, char *driverName)
	: BBox(rect, "TStatusView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	BRect r;
	int32 h = (int32)FontHeight(this, true);
	int32 y = 10;
	
	fModelPathList = new BList;
	
	// printer model
	r.Set(	14, y,
			Bounds().Width()-B_V_SCROLL_BAR_WIDTH-14,
			y + h + 10);

	fModelLabel = new BStringView( r, B_EMPTY_STRING, "Please choose a printer model:");
	
	y = (int32)fModelLabel->Frame().bottom+5;
	r.Set(	14, y,
			Bounds().Width()-B_V_SCROLL_BAR_WIDTH-14,
			y + (15 * h) + 10);

	// the list
	fModelList = new BOutlineListView(r, "list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
	fModelList->SetSelectionMessage(new BMessage(msg_model_single_click));
	fModelList->SetInvocationMessage(new BMessage(msg_model_double_click));
	fModelScroller = new BScrollView("scroll", fModelList, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);

	// 	buttons
	r.top = fModelScroller->Frame().bottom + 13;
	r.bottom = r.top + 20;
	r.right = Bounds().Width() - 15;
	r.left = r.right - 75;
	fOkayBtn = new BButton( r, "bt:ok", "OK", new BMessage(msg_okay), B_FOLLOW_BOTTOM);
	fOkayBtn->MakeDefault(true);
	
	r.right = r.left - 10;
	r.left = r.right - 75;
	fCancelBtn = new BButton( r, "bt:cancel", "Cancel", new BMessage(msg_cancel), B_FOLLOW_BOTTOM);

	// All these objects will be Added in AttachedToWindow(), because
	// We don't know the height of the window yet.
}

TStatusView::~TStatusView()
{
	int32 count;
	count = fModelList->FullListCountItems();
	for (int32 index=count ; index >= 0 ; index--)
		delete ((BStringItem*)fModelList->RemoveItem(index));

	count = fModelPathList->CountItems();
	for (int32 index=count ; index >= 0 ; index--)
		delete ((MPrinterModelItem*)fModelPathList->RemoveItem(index));
}

void
TStatusView::AttachedToWindow()
{
	BBox::AttachedToWindow();
	
	//	make this window variable height based on the font
	int32 y = Window()->Frame().IntegerHeight();
	y -= (int32)fOkayBtn->Frame().bottom;
	y -= 10;	
	Window()->ResizeBy(0, -y);

	// Set the window limits
	float minW, maxW, minH, maxH;
	Window()->GetSizeLimits(&minW, &maxW, &minH, &maxH);
	Window()->SetSizeLimits(Window()->Bounds().Width(), maxW, Window()->Bounds().Height(), maxH);

	// Now we can add the children
	AddChild(fModelLabel);
	AddChild(fModelScroller);
	AddChild(fCancelBtn);
	AddChild(fOkayBtn);
	
	PopulateModelList();
	UpdateControls();
}

void
TStatusView::PopulateModelList()
{
	if (LockLooper() == false)
		return;

	// Load the list of available ppd files
	BPath ppdir;
	find_directory(B_BEOS_ETC_DIRECTORY, &ppdir);
	ppdir.Append("ppd");
	BDirectory ppdDir(ppdir.Path());
	BEntry entry;

//int nberror = 0;

	ppdDir.Rewind();
	while(ppdDir.GetNextEntry(&entry) == B_OK)
	{
		if (entry.IsDirectory())
		{
			char manufacturer[256];
			entry.GetName(manufacturer);
			BPath manu_path = ppdir;
			manu_path.Append(manufacturer);

			BStringItem *item = new BStringItem(manufacturer, 0, false);
			fModelList->AddItem(item);
			fModelPathList->AddItem(NULL);	// add a dummy item to keep the same indexes
			
			// Find all printers for this manufacturer
			BDirectory model(&entry);
			entry_ref printers;
			while (model.GetNextRef(&printers) == B_OK)
			{
				// Get the real printer name from the PPD file
				BNode node(&printers);
				if (node.InitCheck() == B_OK)
				{
					char printerName[256];
					*printerName = 0;

					BPath path = manu_path;
					path.Append(printers.name);

					if (node.ReadAttr("be:pname", B_STRING_TYPE, 0, printerName, 256) <= 0)
					{
						FILE *handle = fopen(path.Path(), "rt");
						if (handle)
						{
							char input[256];
							while (fgets(input, 256, handle))
							{
								BString kPrinterNameKeyword("*NickName:");
								const ssize_t kLength = kPrinterNameKeyword.Length();
								char temp[256];
								memcpy(temp, input, kLength);
								temp[kLength] = 0;
								BString tmp_str(temp);
								if (kPrinterNameKeyword == tmp_str)
								{
									const char *p = input + kLength;
									while ((*p) && (*p != '\"')) { p++; }
									if (*p++)
									{
										int c = 0;
										while ((*p) && (*p != '\"'))
											printerName[c++] = *p++;
										printerName[c++] = 0;
										status_t err = node.WriteAttr("be:pname", B_STRING_TYPE, 0, printerName, c);
										break;
									}
								}									
							}
							fclose(handle);
						}
					}
												
//if (test_ppd(path.Path()) != 0)
//	nberror++, printf("%s (%s)\n\n", path.Path(), printerName);
						
					if (*printerName)
					{
						MPrinterModelItem *model_item = new MPrinterModelItem;
						model_item->path = path;
						fModelPathList->AddItem(model_item);
						fModelList->AddItem(new BStringItem(printerName, 1));
					}
				}


			}
		}
	}
	
//printf("\n%d error\n", nberror);

	UnlockLooper();
}


void
TStatusView::UpdateControls()
{
	if ((fModelList->FullListCurrentSelection() >= 0) &&
		(fModelList->Superitem(fModelList->FullListItemAt(fModelList->FullListCurrentSelection()))))
	{
		fOkayBtn->SetEnabled(true);
	}
	else
	{
		fOkayBtn->SetEnabled(false);
	}
}

void
TStatusView::Draw(BRect u)
{
	BBox::Draw(u);
}

void
TStatusView::MessageReceived(BMessage *msg)
{
	BBox::MessageReceived(msg);
}


const char*
TStatusView::PrinterName()
{
	return fPrinterName->Text();
}

const char*
TStatusView::PPD()
{
	int32 index = fModelList->FullListCurrentSelection();
	if (index >= 0)
	{
		MPrinterModelItem* item = (MPrinterModelItem*)fModelPathList->ItemAt(index);
		if (item)
			return item->path.Path();
	}
	
	return NULL;
}
