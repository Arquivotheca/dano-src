//--------------------------------------------------------------------
//
//   FILE:  PS_Setup.cpp
//   REVS:  $Revsion: 1.8 $
//  NAME:  Robert Polic
//
//   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
//
//
//  Change history:
//
//	01/28/98	MAV001	Changed PPD directory from B_BEOS_SYSTEM_DIRECTORY
//						to B_BEOS_ETC_DIRECTORY.
//
//--------------------------------------------------------------------

#define P // printf
#define PP //printf

#include <File.h>  
#include <Path.h>
#include <FindDirectory.h>
#include <MenuBar.h>

#include <Box.h>  

#include <fcntl.h>  
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <PrintJob.h>
#include "PS_Setup.h"

#include "Element.h"
#include "ppd_control.h"


#include <OutlineListView.h>
#include <ListItem.h>
#include <Box.h>
#include <ScrollView.h>
#include <Screen.h>

//====================================================================

BSetup::BSetup(BMessage *msg, char *name, TPrintDrv *drv)
	   :BWindow(BRect(100, 100, 100 + SETUP_WIDTH, 100 + SETUP_HEIGHT),
				name, B_TITLED_WINDOW, B_NOT_RESIZABLE | 
									   B_NOT_MINIMIZABLE |
									   B_NOT_ZOOMABLE)
{
	char	print_name[512];
	long	length;

	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint  pt((screenFrame.Width() - Bounds().Width())*0.5f, (screenFrame.Height() - Bounds().Height())*0.5f);
	if (screenFrame.Contains(pt))
		MoveTo(pt);

	parent = drv;	
	fSetupMessage = msg;
	fResult = B_NO_ERROR;

	sprintf(print_name, "%s Setup", name);
	SetTitle(print_name);

	fView = new TSetupView(Bounds(), msg);

	// Try to find the ppd file
	BPath	path;

	find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
	path.Append("printers");
	path.Append(name);

	BNode setting_node(path.Path());

	char ppd_name[B_FILE_NAME_LENGTH];
	length = setting_node.ReadAttr("ppd_name",
								 B_STRING_TYPE,
					  			 0,
								 ppd_name,
								 B_FILE_NAME_LENGTH);
	
	if (length > 0) 
		ppd_name[length] = 0;

	if ((strlen(ppd_name) > 0))
	{
		strcpy(ppd_fullname, ppd_name);
		fView->ppd_file = new BFile(ppd_fullname, O_RDONLY);
		if (!fView->ppd_file->IsReadable())
		{
			fResult = B_ERROR;
			delete fView;
		}
	}

	Lock();
	AddChild(fView);

	fSetupSem = create_sem(0, "SetupSem");

	// DR9 feature
	fFilter = new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN,
								 &SetupKeyFilter);
	AddCommonFilter(fFilter);

	Unlock();
}

//--------------------------------------------------------------------

BSetup::~BSetup()
{
	// nothing
	fflush(stdout);
}

//--------------------------------------------------------------------

void BSetup::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_OK:
			fView->UpdateMessage(fSetupMessage);
			fResult = B_NO_ERROR;
			// by releasing this semaphore, the Go() method will unblock.
			release_sem(fSetupSem);
			break;

		case M_CANCEL:
			fResult = B_ERROR;
			// by releasing this semaphore, the Go() method will unblock.
			release_sem(fSetupSem);
			break;
		case M_ADV:
			fView->UpdateMessage(fSetupMessage);
			OpenOptionWindow();
			break;
		case ADV_OPT_CLOSE:
			fView->UpdateFromPPD(fSetupMessage);
			fOptionWindow->Lock();
			fOptionWindow->Quit();			
			break;
		case PAPER_SIZE_CHANGED:
		case UNITS_LABEL_INCH_MESSAGE:
		case UNITS_LABEL_CM_MESSAGE:
		case UNITS_LABEL_POINTS_MESSAGE:
		case ORIENT_PORTRAIT:
		case ORIENT_LANDSCAPE:
		case LAYOUT_1_UP:
		case LAYOUT_LAST_UP:
			PostMessage(msg, fView);
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

//--------------------------------------------------------------------

void
BSetup::OpenOptionWindow()
{
	fOptionWindow = new OptionWindow(BRect(100,100,400,600), fSetupMessage, this);
	fOptionWindow->Go();	
}

bool BSetup::QuitRequested()
{
	// we get here from the user clicking the close box
	// or hitting Alt-w, so treat it like a cancel: set
	// fResult to error, and release fSetupSem, which
	// frees the Go() function.  Return false so we don't
	// cal Quit() twice.

	fResult = B_ERROR;
	release_sem(fSetupSem);
	return false;
}
//--------------------------------------------------------------------

long BSetup::Go(void)
{
	long value;

	if (fView->ppd_file == NULL) return B_ERROR;

	Show();

	acquire_sem(fSetupSem);
	delete_sem(fSetupSem);

	// synchronous call to close the alert window. Remember that this will
	// 'delete' the object that we're in. That's why the return value is
	// saved on the stack.
	value = fResult;
	Lock();
	Quit();
	return(value);
}

//--------------------------------------------------------------------

filter_result SetupKeyFilter(BMessage *msg, BHandler **target, BMessageFilter *filter)
{
	BLooper *looper = filter->Looper();
	TSetupView	*view = ((BSetup *)looper)->fView;
	if ((*target != view->margin_left->ChildAt(0)) &&
		(*target != view->margin_right->ChildAt(0)) &&
		(*target != view->margin_top->ChildAt(0)) &&
		(*target != view->margin_bottom->ChildAt(0))
		/*&& (*target != view->fScaling->ChildAt(0)) */		// scaling not in Genki
		)
		return B_DISPATCH_MESSAGE;

	ulong mods = 0;
	mods = msg->FindInt32("modifiers");
	if (mods & B_COMMAND_KEY)
		return B_DISPATCH_MESSAGE;

	const uchar *bytes = NULL;
	if (msg->FindString("bytes", (const char **)&bytes) != B_NO_ERROR)
		return B_DISPATCH_MESSAGE;

	long key = bytes[0];

	if (isdigit(key) || (key == '.') || iscntrl(key))
		return B_DISPATCH_MESSAGE;

	if (isascii(key))
		return B_SKIP_MESSAGE;

	return B_DISPATCH_MESSAGE;
}


//====================================================================

TSetupView::TSetupView(BRect frame, BMessage *msg)
		   :BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW),
		   ppd_file(NULL)
{
	BRect		r;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if (msg->HasInt32("orientation")) {
		fOrient = ORIENT_PORTRAIT + msg->FindInt32("orientation");
	}
	else {
		fOrient = ORIENT_PORTRAIT;
	}

	if (msg->HasFloat("scaling")) {
		fScale = msg->FindFloat("scaling");
	}
	else {
		fScale = 100.0;
	}
	
	if (msg->HasInt32("units")) {
		long val;
		msg->FindInt32("units",&val);
		unit = (UNIT_TYPE)val;
	}
	else {
		unit = INCHES;
	}
	
	if (msg->HasBool("first up")) {
		fFirstUp = msg->FindBool("first up");
	}
	else {
		fFirstUp = true;
	}
	
	if (msg->HasString("page size")) {
		const char *name;
		msg->FindString("page size",(const char**)&name);
		page_size_name = name;
	}
	
	if (msg->HasRect("paper_rect")) {
		r = msg->FindRect("paper_rect");
		fPageWidth = r.Width() / 72.0;
		fPageHeight = r.Height()/ 72.0;
	}
	else {
		fPageWidth = 8.5;
		fPageHeight = 11.0;
	}

	if (msg->HasRect("printable_rect")) {
		fImageable = msg->FindRect("printable_rect");
	}
	else {
		fImageable.Set(0,0,-1,-1);
	}

	if (msg->HasBool("bitmap")) {
		msg->FindBool("bitmap",&use_bitmap);
	}
	else use_bitmap = false;

	if (msg->HasBool("HighQuality")) {
		msg->FindBool("HighQuality", &raster_quality);
	}
	else raster_quality = false;

	if (msg->HasBool("in_color")) {
		msg->FindBool("in_color", &use_color);
	} else {
		use_color = false;
	}
}

//--------------------------------------------------------------------
TSetupView::~TSetupView()
{
	PAGESIZE *page;
	int32 count = page_size_list.CountItems();
	for(int32 i=0; i < count; i++){
		page = static_cast<PAGESIZE*>(page_size_list.RemoveItem(0L));
		delete page;		
	}
	delete ppd_file;
}

//--------------------------------------------------------------------
void TSetupView::ReadPPD(void)
{
	BSetup *window = dynamic_cast<BSetup*>(Window());
	PPD *ppd = PPD::Instance();
		
//	BPath path(window->ppd_fullname);
//	window->parent->ReadPPD(path.Leaf());
	window->parent->ReadPPD(window->ppd_fullname);

	ppd->AssignConstraints();

	PAGESIZE *page;

	Invocation *inv;
	
	// first get page sizes
	int32 invCount = ppd->HasInvocation("PaperDimension");
	for(int i=0; i < invCount ; i++){
		inv = ppd->FindInvocation("PaperDimension", NULL, i);
		page = new PAGESIZE;
		strcpy(page->name, inv->Translation());

		int width, height;
		const char *dimString = inv->GetFirstString();
		if(dimString == NULL) {
			delete page;
			continue;
		}
		sscanf(dimString, "%d %d", &width, &height);
		delete dimString;
		page->width = width / 72.;
		page->height = height / 72.;
		
		strcpy(page->realname, inv->Option());
		page_size_list.AddItem(page);
		page->message = page_size_list.CountItems();		
	}
	
	// now get imageable areas...
	invCount = page_size_list.CountItems();
	for(int i=0; i < invCount; i++){
		page = (PAGESIZE*)page_size_list.ItemAt(i);
		inv = ppd->FindInvocation("ImageableArea", page->realname, 0);
		if(!inv) continue;
		
		float llx, lly, urx, ury;
		const char *dimString = inv->GetFirstString();
		if(dimString == NULL) {
			continue;
		}
		sscanf(dimString, "%f %f %f %f", &llx, &lly, &urx, &ury);
		delete dimString;
				
		page->imageable.Set(	llx,
								page->height*72-ury,
								urx,
								page->height*72-lly);
	}
}

//--------------------------------------------------------------------

void TSetupView::AttachedToWindow(void)
{
	bool		marked = false;
	long		loop;
	BMenuItem	*item;
	BPopUpMenu	*menu;
	BRect		r;
	char		label[128];


	// Default font (usefull for the string width calculation)
	SetFont(be_plain_font);

	// Read ppd file
	ReadPPD();

	// Start off clean
	isMarginDirty = false;

	// Initialize a nice menu
	menu = new BPopUpMenu("m:format");

	
	BMenuItem *default_item = NULL;
	PAGESIZE *default_page = NULL;
	for (loop = 0; loop < page_size_list.CountItems(); loop++) {
		PAGESIZE *page = (PAGESIZE *) page_size_list.ItemAt(loop);
		BMessage *message = new BMessage(PAPER_SIZE_CHANGED);
		message->AddInt32("format",page->message);
		menu->AddItem(item = new BMenuItem(page->name,message));
		if (loop == 0) {
			default_item = item;
			default_page = page;
		}
		if (!marked && (page_size_name == page->realname)) {
			item->SetMarked(TRUE);
			marked = TRUE;
			if (!fImageable.IsValid()) fImageable = page->imageable;
			default_item = item;
			default_page = page;
		}
	}

	// If no menu is marked, then select the first one, that is the last one in the list
	if (!marked && (default_item != NULL)) {
		default_item->SetMarked(TRUE);
		fPageWidth = default_page->width;
		fPageHeight = default_page->height;
		fImageable = default_page->imageable;
		page_size_name = default_page->realname;
		if (!fImageable.IsValid()) fImageable = default_page->imageable;
	}

	// Paper size menu field
	r.Set(PAPER_SIZE_HOR, PAPER_SIZE_VERT, PAPER_SIZE_HOR + PAPER_SIZE_W, PAPER_SIZE_VERT + PAPER_SIZE_H);
	fPaper = new BMenuField(r, "mf:format", PAPER_SIZE_LABEL, menu);
	fPaper->SetDivider(StringWidth(LAYOUT_LABEL) + 7);	// size of LAYOUT_LABEL, to be alligned
	fPaper->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fPaper);

	// Layout menu field
	menu = new BPopUpMenu("m:layout");
	menu->SetRadioMode(true);
	BMessage *message = new BMessage(LAYOUT_1_UP);
	menu->AddItem(item = new BMenuItem(LAYOUT_FIRST_TOP_LABEL,message));
	message = new BMessage(LAYOUT_LAST_UP);
	menu->AddItem(item = new BMenuItem(LAYOUT_LAST_TOP_LABEL,message));
	if (fFirstUp) menu->ItemAt(0)->SetMarked(true);
	else menu->ItemAt(1)->SetMarked(true);
	
	r.OffsetBy(0.,LAYOUT_VERT_SHIFT);
	fLayout = new BMenuField(r, "mf:layout", LAYOUT_LABEL, menu);
	fLayout->SetDivider(StringWidth(LAYOUT_LABEL) + 7);
	fLayout->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fLayout);

#if 0  		// scaling not implemented in Genki...
	// Scale text control
	r.OffsetBy(0.,LAYOUT_VERT_SHIFT);
	r.Set(SCALE_LEFT,r.top , SCALE_RIGHT, r.bottom);
	fScaling = new BTextControl(r,SCALE_TEXT,SCALE_TEXT,"100",new BMessage(SIZE_SCALING));
	fScaling->SetAlignment(B_ALIGN_LEFT, B_ALIGN_LEFT);
	fScaling->SetDivider(SCALE_DIVIDER);
	AddChild(fScaling);
	sprintf(label,"%d",(long)(fScale));
	fScaling->SetText(label);

	r.left = r.right + 2;
	r.right = r.left + 20;
	BStringView *percent = new BStringView(r,"%","%");
	percent->SetViewColor(ViewColor());
	percent->SetLowColor(LowColor());
	AddChild(percent);
#endif

	// Orientation text
	r.Set(	ORIENTATION_TEXT_LEFT,	ORIENTATION_TEXT_TOP,
			PORTRAIT_LEFT - 5,		ORIENTATION_TEXT_TOP + 14);
	BStringView *fOrientationText = new BStringView(r,"sv:orientation", ORIENTATION_TEXT);
	fOrientationText->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fOrientationText);

	// Orientation Portrait button	
	r.Set(PORTRAIT_LEFT,ORIENTATION_TOP,PORTRAIT_LEFT+ORIENTATION_WIDTH,ORIENTATION_TOP+ORIENTATION_HEIGHT);
	portrait = new OrientationButton(r, new BPicture, new BPicture);
	AddChild(portrait);
	portrait->Init(ORIENT_PORTRAIT);

	// Orientation Landscape button	
	r.Set(LANDSCAPE_LEFT,ORIENTATION_TOP,LANDSCAPE_LEFT+ORIENTATION_WIDTH,ORIENTATION_TOP+ORIENTATION_HEIGHT);
	landscape = new OrientationButton (r, new BPicture, new BPicture);
	AddChild(landscape);
	landscape->Init(ORIENT_LANDSCAPE);

	if (fOrient == ORIENT_PORTRAIT) {
		portrait->SetValue(true);
		landscape->SetValue(false);
	}
	else {
		portrait->SetValue(false);
		landscape->SetValue(true);
	}

	// Color option	
	r.Set(COLOR_H,COLOR_V,COLOR_H+COLOR_WIDTH,COLOR_V+COLOR_HEIGHT);
	color_box = new BCheckBox (r,"cb:color",COLOR_TEXT,NULL);
	color_box->SetValue(use_color);
	if((PPD::Instance())->LanguageLevel() == 1) {
		color_box->SetEnabled(false);
	}
	AddChild(color_box);

	// Rasterize options
	ps_generation_menu = new BPopUpMenu("m:psmode");
	ps_generation_menu->SetLabelFromMarked(true);
	ps_generation_menu->AddItem(new BMenuItem(RATERIZE_PS_TEXT, new BMessage)); // NULL ?
	ps_generation_menu->AddItem(new BMenuItem(RATERIZE_8_TEXT, new BMessage));
	ps_generation_menu->AddItem(new BMenuItem(RATERIZE_24_TEXT, new BMessage));
	if (use_bitmap == false) {
		ps_generation_menu->ItemAt(0)->SetMarked(true);
	} else {
		if (raster_quality == false)
			ps_generation_menu->ItemAt(1)->SetMarked(true);
		else	
			ps_generation_menu->ItemAt(2)->SetMarked(true);
	}

	r.Set(RASTERIZE_H,RASTERIZE_V,RASTERIZE_H+RASTERIZE_WIDTH,RASTERIZE_V+RASTERIZE_HEIGHT);
	ps_generation_field = new BMenuField(r, "mf:psmode", RATERIZE_TEXT, ps_generation_menu);
	ps_generation_field->SetDivider(StringWidth(LAYOUT_LABEL) + 7 + 20);
	ps_generation_field->SetAlignment(B_ALIGN_RIGHT);

	AddChild(ps_generation_field);

	// Margin box
	r.Set(BOX_OFFSET_HOR,BOX_OFFSET_VERT,BOX_OFFSET_HOR+BOX_OFFSET_W,BOX_OFFSET_VERT+BOX_OFFSET_H);
	BBox *box = new BBox(r);
	box->SetLabel(BOX_LABEL);
	AddChild(box);

	// Margin view
	r.Set(VIEW_MARGIN_H,VIEW_MARGIN_V,VIEW_MARGIN_H + VIEW_MARGIN_MAX_W,VIEW_MARGIN_V + VIEW_MARGIN_MAX_H);
	fView = new TMarginView(r,0,0);
	box->AddChild(fView);
	fView->SetPageSize(fPageWidth,fPageHeight, fImageable, fOrient);
	
	// Units button
	menu = new BPopUpMenu("m:units");
	menu->SetRadioMode(true);
	menu->AddItem(item = new BMenuItem(UNITS_LABEL_INCH,	new BMessage(UNITS_LABEL_INCH_MESSAGE)));
	menu->AddItem(item = new BMenuItem(UNITS_LABEL_CM,		new BMessage(UNITS_LABEL_CM_MESSAGE)));
	menu->AddItem(item = new BMenuItem(UNITS_LABEL_POINTS,	new BMessage(UNITS_LABEL_POINTS_MESSAGE)));
	menu->ItemAt((long) unit)->SetMarked(true);

	r.Set(UNITS_HOR,UNITS_VERT,UNITS_HOR + UNITS_W+3,UNITS_VERT + UNITS_H);
	fUnits = new BMenuField(r, "mf:units", B_EMPTY_STRING, menu);
	fUnits->SetDivider(0);
	box->AddChild(fUnits);

	// Margin text controls
	NiceString(label,0.);
	r.Set(TV_LEFT,TC_TOP_V,TC_RIGHT,TC_TOP_V + TC_VERT_SHIFT);
	margin_top = new BTextControl(r,"tc:tmargin",TV_LABEL_FOR_TOP,label, new BMessage(MARGIN_CHG));
	margin_top->SetModificationMessage(new BMessage(MARGIN_DIRTY));
	margin_top->SetDivider(TV_DIVIDER);
	margin_top->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	margin_top->SetTarget(this);
	box->AddChild(margin_top);

	r.Set(TV_LEFT,TC_TOP_V + TC_VERT_SHIFT,TC_RIGHT,TC_TOP_V + 2*TC_VERT_SHIFT);
	margin_bottom = new BTextControl(r,"tc:bmargin",TV_LABEL_FOR_BOTTOM,label, new BMessage(MARGIN_CHG));
	margin_bottom->SetModificationMessage(new BMessage(MARGIN_DIRTY));
	margin_bottom->SetDivider(TV_DIVIDER);
	margin_bottom->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	margin_bottom->SetTarget(this);
	box->AddChild(margin_bottom);

	r.Set(TV_LEFT,TC_TOP_V + 2.*TC_VERT_SHIFT,TC_RIGHT,TC_TOP_V + 3*TC_VERT_SHIFT);
	margin_left = new BTextControl(r,"tc:lmargin",TV_LABEL_FOR_LEFT,label, new BMessage(MARGIN_CHG));
	margin_left->SetModificationMessage(new BMessage(MARGIN_DIRTY));
	margin_left->SetDivider(TV_DIVIDER);
	margin_left->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	margin_left->SetTarget(this);
	box->AddChild(margin_left);

	r.Set(TV_LEFT,TC_TOP_V + 3.*TC_VERT_SHIFT,TC_RIGHT,TC_TOP_V + 4*TC_VERT_SHIFT);
	margin_right = new BTextControl(r,"tc:rmargin",TV_LABEL_FOR_RIGHT,label, new BMessage(MARGIN_CHG));
	margin_right->SetModificationMessage(new BMessage(MARGIN_DIRTY));
	margin_right->SetDivider(TV_DIVIDER);
	margin_right->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	margin_right->SetTarget(this);
	box->AddChild(margin_right);

	// String showing size
	r.Set(SIZE_STRING_HOR,SIZE_STRING_VERT,SIZE_STRING_HOR+SIZE_STRING_W,SIZE_STRING_VERT+SIZE_STRING_H);
	char sizeLabel[128];
	sprintf(sizeLabel,"%.2f x %.2f",fPageWidth,fPageHeight);
	sizeString = new BStringView(r, B_EMPTY_STRING, sizeLabel);
	sizeString->SetViewColor(ViewColor());
	sizeString->SetLowColor(LowColor());
	box->AddChild(sizeString);
	UpdateSize();

	/* advanced options */	
	r.Set(SET_ADV_BUTTON_H, SET_ADV_BUTTON_V,
		  SET_ADV_BUTTON_H + BUTTON_WIDTH,
		  SET_ADV_BUTTON_V + BUTTON_HEIGHT);
	advanced_button = new BButton(r, "bt:advanced", SET_ADV_BUTTON_TEXT, new BMessage(M_ADV));
	AddChild(advanced_button);

	r.Set(SET_CANCEL_BUTTON_H, SET_CANCEL_BUTTON_V,
		  SET_CANCEL_BUTTON_H + BUTTON_WIDTH,
		  SET_CANCEL_BUTTON_V + BUTTON_HEIGHT);
	cancel_button = new BButton(r, "bt:cancel", SET_CANCEL_BUTTON_TEXT, new BMessage(M_CANCEL));
	AddChild(cancel_button);

	r.Set(SET_OK_BUTTON_H, SET_OK_BUTTON_V,
		  SET_OK_BUTTON_H + BUTTON_WIDTH,
		  SET_OK_BUTTON_V + BUTTON_HEIGHT);
	valid_button = new BButton(r, "bt:ok", SET_OK_BUTTON_TEXT, new BMessage(M_OK));
	AddChild(valid_button);
	valid_button->MakeDefault(true);
}

//--------------------------------------------------------------------

void TSetupView::Draw(BRect rect)
{
	BRect	r;

	r = Bounds();

	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(r.left, r.top), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.left, r.top + 1), BPoint(r.left, r.bottom - 1));
	StrokeLine(BPoint(r.left + 3, LINE_V + 4),
			   BPoint(r.right - 3, LINE_V + 4));

	SetHighColor(120, 120, 120);
	StrokeLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom));
	StrokeLine(BPoint(r.right - 1, r.bottom), BPoint(r.left, r.bottom));
	StrokeLine(BPoint(r.left + 3, LINE_V + 3),
			   BPoint(r.right - 3, LINE_V + 3));
}

//--------------------------------------------------------------------

void TSetupView::MessageReceived(BMessage *msg)
{
	long	orient;
	PAGESIZE *page;
	
	switch (msg->what) {
		case SIZE_SCALING:
			break;

		case PAPER_SIZE_CHANGED:
			// Get page description
			page = (PAGESIZE *) page_size_list.ItemAt(msg->FindInt32("format")-1);
			UpdatePaperSizeMenu(page);
			break;

		case ORIENT_PORTRAIT:
			landscape->SetValue(false);
			portrait->SetValue(true);
			orient = msg->what;			
			if (orient != fOrient) {
				fOrient = orient;
				UpdateSize();
				fView->SetPageSize(fPageWidth, fPageHeight, fImageable, fOrient);
			}
			break;
		case ORIENT_LANDSCAPE:
			portrait->SetValue(false);
			landscape->SetValue(true);
			orient = msg->what;			
			if (orient != fOrient) {
				fOrient = orient;
				UpdateSize();
				fView->SetPageSize(fPageWidth, fPageHeight, fImageable, fOrient);
			}
			break;
		case UNITS_LABEL_INCH_MESSAGE:
			unit = INCHES;
			UpdateSize();
			break;
		case UNITS_LABEL_CM_MESSAGE:
			unit = CM;
			UpdateSize();
			break;
		case UNITS_LABEL_POINTS_MESSAGE:
			unit = POINTS;
			UpdateSize();
			break;
		case LAYOUT_1_UP:
			fFirstUp = true;
			break;
		case LAYOUT_LAST_UP:
			fFirstUp = false;
			break;
		case MARGIN_DIRTY:
			isMarginDirty = true;
			break;
		case MARGIN_CHG:
			UpdateMargins();
			UpdateSize(false);
			fView->SetPageSize(fPageWidth, fPageHeight, fImageable, fOrient);
			break;			
	}
}

void TSetupView::UpdateMargins(void)
{
	BRect imageable;	

	if(fOrient == ORIENT_PORTRAIT)
	{	
		imageable.left = atof(margin_left->Text());
		imageable.top = atof(margin_top->Text());
		imageable.right = atof(margin_right->Text());
		imageable.bottom = atof(margin_bottom->Text());
	}
	else
	{
		imageable.top = atof(margin_left->Text());
		imageable.right = atof(margin_top->Text());
		imageable.bottom = atof(margin_right->Text());
		imageable.left = atof(margin_bottom->Text());	
	}
	
	// Convert to points
	if (unit == INCHES)
	{
		imageable.left = imageable.left * 72.;
		imageable.top = imageable.top * 72.;
		imageable.right = imageable.right * 72.;
		imageable.bottom = imageable.bottom * 72.;
	}
	else if (unit == CM)
	{
		imageable.left = imageable.left /2.54 * 72.;
		imageable.top = imageable.top /2.54 * 72.;
		imageable.right = imageable.right /2.54 * 72.;
		imageable.bottom = imageable.bottom /2.54 * 72.;
	}
	
	imageable.right = fPageWidth * 72. - imageable.right;
	imageable.bottom = fPageHeight * 72. - imageable.bottom;

	if (imageable.IsValid())
	{
		fImageable = imageable;
	}

	isMarginDirty = false;
}

void TSetupView::UpdateSize(bool update_margin_fields)
{
	char dimension[64];
	char top[32];
	char bottom[32];
	char left[32];
	char right[32];

	/* dimension string */
	float logicalWidth, logicalHeight;
	if(fOrient == ORIENT_LANDSCAPE)
	{
		logicalWidth = fPageHeight;
		logicalHeight = fPageWidth;
	}
	else
	{
		logicalWidth = fPageWidth;
		logicalHeight = fPageHeight;
	}
	
	switch(unit)
	{
		case INCHES:
			sprintf(dimension, "%.1f x %.1f", logicalWidth, logicalHeight);
			break;
		case CM:
			sprintf(dimension, "%.1f x %.1f", (2.54*logicalWidth), (2.54*logicalHeight));
			break;
		case POINTS:
			sprintf(dimension, "%ld x %ld", (long)(72.*logicalWidth), (long)(72.*logicalHeight));
			break;
	}

	sizeString->SetText(dimension);

	
	if (update_margin_fields == true)
	{
		/* calculate the margins... */
		switch(unit)
		{
			case INCHES:
			{
				sprintf(top,    "%.1f", (fImageable.top/72.));
				sprintf(bottom, "%.1f", (fPageHeight - fImageable.bottom/72.));
				sprintf(left,   "%.1f", (fImageable.left/72.));
				sprintf(right,  "%.1f", (fPageWidth - fImageable.right/72.));
				break;	
			}
	
			case CM:
			{
				sprintf(top,    "%.1f", (2.54 * (fImageable.top / 72.)));
				sprintf(bottom, "%.1f", (2.54 * (fPageHeight - fImageable.bottom / 72.)));
				sprintf(left,   "%.1f", (2.54 * (fImageable.left / 72.)));
				sprintf(right,  "%.1f", (2.54 * (fPageWidth - fImageable.right / 72.)));
				break;
			}
	
			case POINTS:
			{
				sprintf(top,    "%ld", (long)fImageable.top);
				sprintf(bottom, "%ld", (long)(72.*fPageHeight - fImageable.bottom));
				sprintf(left,   "%ld", (long)fImageable.left);
				sprintf(right,  "%ld", (long)(72.*fPageWidth - fImageable.right));
				break;
			}
				
		}
	
		if (fOrient == ORIENT_LANDSCAPE)
		{
			margin_top->SetText(right);
			margin_bottom->SetText(left);
			margin_left->SetText(top);
			margin_right->SetText(bottom);
		}
		else
		{
			margin_top->SetText(top);
			margin_bottom->SetText(bottom);
			margin_left->SetText(left);
			margin_right->SetText(right);
		}
	}
}
//--------------------------------------------------------------------

void TSetupView::NiceString(char *str, float n)
{
	long	size;

	sprintf(str, "%f", n);
	size = strlen(str);
	while (size > 1) {
		if ((str[size - 1] == '0') && (str[size - 2] != '.')) {
			size--;
			str[size] = 0;
		}
		else
			break;
	}
}

//--------------------------------------------------------------------

void TSetupView::UpdateMessage(BMessage *msg)
{
	BRect	r;
	
	if (isMarginDirty) {
		UpdateMargins();
	}


	bool raster, quality;
	int index = 0;
	BMenuItem *item;	
	if (item = ps_generation_menu->FindMarked()) {
		if ((index = ps_generation_menu->IndexOf(item)) < 0)
			index = 0;
	}
	
	if (index == 0)
	{
		raster = quality = false;
	}
	else if (index == 1)
	{
		raster = true; quality = false;
	}
	else if (index == 2)
	{
		raster = quality = true;	
	}
		
	if (msg->HasBool("bitmap"))		msg->ReplaceBool("bitmap", raster);
	else							msg->AddBool("bitmap", raster);

	if (msg->HasBool("HighQuality"))	msg->ReplaceBool("HighQuality", quality);
	else								msg->AddBool("HighQuality", quality);



	if (msg->HasBool("in_color")) {
		msg->ReplaceBool("in_color",color_box->Value());
	}
	else {
		msg->AddBool("in_color",color_box->Value());
	}

	if (msg->HasInt32("orientation")) {
		msg->ReplaceInt32("orientation", fOrient - ORIENT_PORTRAIT);
	}
	else {
		msg->AddInt32("orientation", fOrient - ORIENT_PORTRAIT);
	}

	if (msg->HasString("page size")) {
		msg->ReplaceString("page size",page_size_name.String());
	}
	else {
		msg->AddString("page size",page_size_name.String());
	}


	// make sure that Page Size and Region are in sync with adv opts
	const char *name;
	uint32 type;
	int32 count;
	char *keyword;
	char *option;

	BString foundPageSize;
	BString foundPageRegion;

	if(msg->HasMessage("ppd_item")){
		BMessage ppd;
		msg->FindMessage("ppd_item", &ppd);

		// first get the options for PageSize and PageRegion, if any
		for(int32 i=0;
			ppd.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
			i++){
	
			char *dup_name = strdup(name);		
			char *save_ptr;
			keyword = strtok_r(dup_name, "|", &save_ptr);
			option = strtok_r(NULL, "|", &save_ptr);
		
			if(!strcmp(keyword, "PageSize")) {
				foundPageSize = option;
			} else if(!strcmp(keyword, "PageRegion")) {
				foundPageRegion = option;
			}
			
			free(dup_name);
		}

		// if they're out of sync, use page size from the PageSetup panel
		if(foundPageSize != page_size_name){
			BString tmp = "PageSize|";
			tmp << foundPageSize;
			ppd.RemoveName(tmp.String());

			tmp = "PageSize|";
			tmp << page_size_name;
			ppd.AddBool(tmp.String(), true);

			tmp = "PageRegion|";
			tmp << foundPageRegion;
			ppd.RemoveName(tmp.String());
			
			tmp = "PageRegion|";
			tmp << page_size_name;
			ppd.AddBool(tmp.String(), true);

		}
		
		msg->ReplaceMessage("ppd_item", &ppd);
			
	} else {
		// Setup message didn't contain ppd message...
		BMessage ppd;
		BString page = "PageSize|";
		page << page_size_name;
		ppd.AddBool(page.String(), true);

		page = "PageRegion|";
		page << page_size_name;
		ppd.AddBool(page.String(), true);
		
		msg->AddMessage("ppd_item", &ppd);
	}


#if 0 // scaling not implemented in Genki
	n = atof(fScaling->Text());
	if (msg->HasFloat("scaling"))
		msg->ReplaceFloat("scaling", n);
	else
		msg->AddFloat("scaling", n);
#endif

	msg->RemoveName("xres");
	msg->RemoveName("yres");
	msg->AddInt64("xres", 300);
	msg->AddInt64("yres", 300);


	msg->RemoveName("paper_rect");
	msg->RemoveName("printable_rect");

	r.left = 0.0;
	r.top = 0.0;
	
	if(fOrient == ORIENT_LANDSCAPE)
	{
		r.bottom = fPageWidth * 72.;
		r.right = fPageHeight * 72.;
	}
	else
	{
		r.right = fPageWidth * 72.;
		r.bottom = fPageHeight * 72.;
	}
	
	msg->AddRect("paper_rect", r);

	if (msg->HasInt32("units")) msg->ReplaceInt32("units",unit);
	else msg->AddInt32("units",unit);

	if(fOrient == ORIENT_LANDSCAPE)
	{
		r.top = fImageable.left;
		r.bottom = fImageable.right;
		r.left = fImageable.top;
		r.right = fImageable.bottom;
	}
	else
	{
		r = fImageable;
	}

	msg->AddRect("printable_rect", r);


	msg->RemoveName("first up");
	msg->AddBool("first up",fFirstUp);
}

void TSetupView::UpdateFromPPD(BMessage *setup)
{
	if(setup == NULL || !setup->HasMessage("ppd_item")) { return; }
	
	const char *name;
	char *dup_name;
	uint32 type;
	int32 count;
	char *keyword;
	char *option;

	BMessage ppdMsg;
	setup->FindMessage("ppd_item", &ppdMsg);
	
	// handle options set in the "Advanced Option" window
	for(int32 i=0;
		ppdMsg.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
		i++){

		dup_name = strdup(name);		
		char *save_ptr;
		keyword = strtok_r(dup_name, "|", &save_ptr);
		option = strtok_r(NULL, "|", &save_ptr);
	
		// update page size
		if(strcmp(keyword, "PageSize")) { continue; }
		PAGESIZE *page = LookupPageSize(option);
		if(page) {
			P("Calling UpdatePaperSizeMenu(0x%x)\n", page);
			UpdatePaperSizeMenu(page);
		}		

		free (dup_name);	
	}	
}

//--------------------------------------------------------------------
void
TSetupView::UpdatePaperSizeMenu(PAGESIZE* page)
{
	float width;
	float height;
	float bar_size;
	char label[128];
	
	page_size_name = page->realname;

	// Compute menu bar size
	bar_size = PAPER_SIZE_W - (StringWidth(LAYOUT_LABEL) + 7);
	
	strcpy(label,page->name);
	// Check if name is too long
	if (bar_size < StringWidth(page->name)) {
		// Approximation of an average width per char
		long len = strlen(page->name);
		float average = StringWidth(page->name) / len;
		long lastIndex = (long)(bar_size / average);
		label[lastIndex - 1 ] = '\0';
		label[lastIndex - 2] = '.';
		label[lastIndex - 3] = '.';
		label[lastIndex - 4] = '.';
	}
	fPaper->MenuBar()->ItemAt(0)->SetLabel(label);

	width = page->width;
	height = page->height;
	if (width != fPageWidth) {
		fPageWidth = width;
	}
	if (height != fPageHeight) {
		fPageHeight = height;
	}

	fImageable = page->imageable;
	UpdateSize();
	fView->SetPageSize(fPageWidth, fPageHeight, fImageable, fOrient);
}

//--------------------------------------------------------------------
PAGESIZE*
TSetupView::LookupPageSize(const char *realname)
{
	PAGESIZE *page;
	for(int32 i=0; (page=static_cast<PAGESIZE*>(page_size_list.ItemAt(i))); i++){
		if(!strcmp(page->realname, realname)){
			P("LookupPageSize: Found matching page.\n");
			return page;
		}
	}
	P("LookupPageSize: Couldn't find a matching page for %s!\n", realname);
	return NULL;
}

//====================================================================

TMarginView::TMarginView(BRect frame, float width, float height)
			:BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	penSize = 3.0;

	fWidth = width;
	fHeight = height;
	
	maxWidth = frame.Width();
	maxHeight = frame.Height();
	center.x = (frame.left + frame.right) / 2.;
	center.y = (frame.top + frame.bottom) / 2.;
}

//--------------------------------------------------------------------

TMarginView::~TMarginView(void)
{
}

//--------------------------------------------------------------------

void TMarginView::Draw(BRect rect)
{
	BRect	r;

	r = Bounds();

	SetHighColor(255,255,255);
	FillRect(r);
	SetPenSize(penSize);
	SetHighColor(0,0,0);
	StrokeRect(r);

	pattern aPattern = { {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa } };	

	
	r.top += (marginBox.top > MIN_MARGIN ? marginBox.top : MIN_MARGIN);
	r.bottom -= (marginBox.bottom > MIN_MARGIN ? marginBox.bottom : MIN_MARGIN);
	r.left += (marginBox.left > MIN_MARGIN ? marginBox.left : MIN_MARGIN);
	r.right -= (marginBox.right > MIN_MARGIN ? marginBox.right : MIN_MARGIN);

	SetPenSize(0.);
	StrokeRect(r,aPattern);
}

//--------------------------------------------------------------------

void TMarginView::SetPageSize(float width, float height, BRect imageable, long orient)
{
	BRect	r;

/*
	if ((fWidth == width) || (fHeight == height))
	{
		return;
	}
*/
	float logicalHeight, logicalWidth;
	
	fWidth = width;
	fHeight = height;
	Window()->Lock();

	if (orient == ORIENT_LANDSCAPE)
	{	
		logicalWidth = height;
		logicalHeight = width;

		r.left = center.x - maxWidth/2.;
		r.right = center.x + maxWidth/2.;
		r.top = center.y - (logicalHeight * maxWidth / logicalWidth) / 2.;
		r.bottom = center.y + (logicalHeight * maxWidth / logicalWidth) / 2.;
	
		pointsPerPixelH = (r.right - r.left) / (logicalWidth*72);
		pointsPerPixelV = (r.bottom - r.top) / (logicalHeight*72);
		
		marginBox.top = (width*72 - imageable.right)* pointsPerPixelV;
		marginBox.bottom = imageable.left * pointsPerPixelV;
		marginBox.left = imageable.top * pointsPerPixelH;
		marginBox.right =  (height*72 - imageable.bottom) * pointsPerPixelH;	
	}	
	else	/* portrait */
	{
		logicalWidth = width;
		logicalHeight = height;
		
		r.top = center.y - maxHeight / 2.;
		r.bottom = center.y + maxHeight / 2.;
		r.left = center.x - (logicalWidth * maxHeight / logicalHeight) / 2.;
		r.right = center.x + (logicalWidth * maxHeight / logicalHeight) / 2.;

		pointsPerPixelH = (r.right - r.left) / (logicalWidth*72);
		pointsPerPixelV = (r.bottom - r.top) / (logicalHeight*72);

		marginBox.top = (imageable.top * pointsPerPixelV);
		marginBox.bottom = (height*72 - imageable.bottom) * pointsPerPixelV;
		marginBox.left = (imageable.left * pointsPerPixelH);
		marginBox.right = (width*72 - imageable.right) * pointsPerPixelH;
	}

	ResizeTo(r.Width(),r.Height());
	MoveTo(r.left,r.top);

	Window()->Unlock();

	Invalidate();
}

//--------------------------------------------------------------------

OrientationButton::OrientationButton(BRect rect,BPicture *pict1,BPicture *pict2)
	: BPictureButton(rect,"",pict1,pict2,NULL,B_TWO_STATE_BUTTON)
{
	// BPictureButton owns the two BPictures passed in
}

//--------------------------------------------------------------------

void OrientationButton::Init(ORIENT orient)
{
	BFont font;
	if (Parent() == NULL) return;
	
	if (orient == ORIENT_LANDSCAPE) {
		SetMessage(new BMessage(ORIENT_LANDSCAPE));
	}
	else {
		SetMessage(new BMessage(ORIENT_PORTRAIT));
	}

	Parent()->Window()->Lock();
	
	BView *view = new BView(BRect(0,0,10,10),"",0,0);
	Parent()->Window()->AddChild(view);	

	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	view->SetFont(be_plain_font);
	view->SetFontSize(32);
	view->GetFont(&font);
	font.SetRotation(0.);
	view->SetFont(&font);
	view->SetHighColor(255,255,255);
	view->FillRect(Bounds());	
	view->SetHighColor(200,200,200);
	if (orient == ORIENT_LANDSCAPE) {
		view->MovePenTo(Bounds().Width() - 2.,Bounds().Height() - 5.);
		view->GetFont(&font);
		font.SetRotation(90.);
		view->SetFont(&font);
	}
	else {
		view->MovePenTo(2.,Bounds().Height() - 5.);
	}
	view->DrawString("A");
	view->SetPenSize(2);
	view->SetHighColor(0,0,0);
	view->StrokeRect(Bounds());	
	picture = view->EndPicture();

	SetEnabledOff(picture);

	picture = new BPicture();
	view->BeginPicture(picture);
	view->SetFont(be_plain_font);
	view->SetFontSize(32);
	view->GetFont(&font);
	font.SetRotation(0.);
	view->SetFont(&font);

	view->SetHighColor(200,200,200);
	view->FillRect(Bounds());	
	view->SetHighColor(0,0,0);
	if (orient == ORIENT_LANDSCAPE) {
		view->MovePenTo(Bounds().Width() - 2.,Bounds().Height() - 5.);
		view->GetFont(&font);
		font.SetRotation(90.);
		view->SetFont(&font);
	}
	else {
		view->MovePenTo(2.,Bounds().Height() - 5.);
	}
	view->DrawString("A");
	view->SetPenSize(2);
	view->StrokeRect(Bounds());	
	picture = view->EndPicture();

	SetEnabledOn(picture);

	view->RemoveSelf();
	delete view;

	Parent()->Window()->Unlock();

}

//--------------------------------------------------------------------
// MGM -- new stuff...testing

PPDMenuItem::PPDMenuItem(const char *label, BMessage *msg)
	: BMenuItem(label, msg)
{
	fIsConflicted = false;
}

PPDMenuItem::~PPDMenuItem()
{
	// nothing;
}

void
PPDMenuItem::Draw()
{
	P("In PPDMenuItem::Draw() for %s.\n", Label());
	if(fIsConflicted){
		rgb_color orig_color = Menu()->HighColor();
		Menu()->SetHighColor(255,0,0);
		BMenuItem::Draw();
		Menu()->SetHighColor(orig_color);		
	} else {
		BMenuItem::Draw();
	}
}

void
PPDMenuItem::SetConflicted(bool val)
{
	fIsConflicted = val;
}

bool
PPDMenuItem::IsConflicted() const
{
	return fIsConflicted;
}



InvocationRadioButton::InvocationRadioButton(BRect rect, Invocation *target)
	: BRadioButton(rect, "", target->Translation(), new BMessage(CBOX))
{
	if(strlen(target->Translation()) == 0){
		SetLabel(target->Option());
	}
	
	fTarget = target;
	if(target->IsEnabled()){
		SetValue(1);
	}
}

void
InvocationRadioButton::SetValue(int32 value)
{
	if(value){
		fTarget->IsEnabled(true);
	} else {
		fTarget->IsEnabled(false);
	}
	BRadioButton::SetValue(value);
}

TOutlineListView::TOutlineListView(BRect frame, const char *name)
	: BOutlineListView(frame, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL)
{
	// nothing
}

TOutlineListView::~TOutlineListView()
{
	BListItem *item;
	int32 count = FullListCountItems();
	for(int32 i=0; i < count; i++){
		item = FullListItemAt(i);
		delete item;
	}
	MakeEmpty();
}



UIStringItem::UIStringItem(const char *name, int32 level, UI *target)
	: BStringItem(name, level),
	fTarget(target)
{
}

UI*
UIStringItem::Target() const
{
	return fTarget;
}

void
UIStringItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	rgb_color orig_color = owner->HighColor();
	if(fTarget && fTarget->IsConflicted()){
		owner->SetHighColor(255,0,0);
	}
	
	BStringItem::DrawItem(owner, itemRect, complete);

	owner->SetHighColor(orig_color);
}

OptionWindow::OptionWindow(BRect rect, BMessage *msg, BSetup *parent)
	: BWindow(rect, "Advanced Options", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
				B_NOT_H_RESIZABLE | B_NOT_CLOSABLE)
{
	// Set window size limits
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(Frame().Width(), maxWidth, Frame().Height()*0.5f, maxHeight);

	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint  pt((screenFrame.Width() - Bounds().Width())*0.5f, (screenFrame.Height() - Bounds().Height())*0.5f);
	if (screenFrame.Contains(pt))
		MoveTo(pt);

	fParent = parent;
	fSetupMsg = msg;
	fResult = B_OK;
	fOptionView = new OptionView(Bounds(), parent);
	AddChild(fOptionView);
}

OptionWindow::~OptionWindow()
{
	// nothing yet
}

long
OptionWindow::Go()
{
	Show();
	return fResult;
}

void
OptionWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case OPTION_OK:
			fOptionView->UpdateMessage(fSetupMsg);
			/* fall through */
		case OPTION_CANCEL:
		{
			BMessage close(ADV_OPT_CLOSE);
			fParent->PostMessage(&close);
			break;
		}
		default:
			PostMessage(msg, fOptionView);
			break;
	}
}

const BMessage*
OptionWindow::GetSetupMessage() const
{
	return fSetupMsg;
}

OptionView::OptionView(BRect rect, BSetup *setup_win)
	: BBox(rect, "optionview", B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW)
{
	ResizeBy(1,1);
	fSetupWindow = setup_win;
	fCurrentUI = NULL;
}

OptionView::~OptionView()
{
	// nothing yet
}

void
OptionView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fOptionList = new TOutlineListView(BRect(10,10,290-B_V_SCROLL_BAR_WIDTH,330), "Options");
	fOptionList->SetSelectionMessage(new BMessage(UI_SELECTED));

	BScrollView *sv = new BScrollView("scroll", fOptionList, B_FOLLOW_ALL,
							B_FRAME_EVENTS | B_WILL_DRAW, false, true, B_FANCY_BORDER);

	const BMessage *setup = dynamic_cast<OptionWindow*>(Window())->GetSetupMessage();

	PPD *ppd = PPD::Instance(setup);
	const char *name;
	bool sameGroup;
	Group *lastGroup = NULL;
	UIStringItem *lastItem = NULL;

	for(UI *ui=ppd->GetFirstUI(); ui; ui=ppd->GetNextUI()){

		// do we need to start a new group?
		if(ui->Parent() && ui->Parent() != lastGroup){
			sameGroup = false;
			lastGroup = ui->Parent();
			const char *parentName;
			parentName = lastGroup->Translation();
			lastItem = new UIStringItem(parentName);
			P("lastItem = [%s]\n", lastItem->Text());
			fOptionList->AddItem(lastItem);
			P("fOptionList->AddItem(%s)\n", lastItem->Text());
		} else {
			sameGroup = true;
			lastGroup = ui->Parent();
		}

		// add the new item in the appropriate place
		name = ui->Translation();
		if(sameGroup){
			UIStringItem *tmpItem = new UIStringItem(name, lastItem->OutlineLevel(), ui);
			P("tmpItem = newUIStringItem(%s, %d, 0x%x)\n", name, lastItem->OutlineLevel(), ui);
			fOptionList->AddItem(tmpItem);
			P("fOptionList->AddItem(%s)\n", lastItem->Text());
			lastItem = tmpItem;
		} else {
			UIStringItem *tmpItem = new UIStringItem(name, 0, ui);
			P("tmpItem = new UIStringItem(%s, %d, 0x%x)\n",name, 0, ui);
			fOptionList->AddUnder(tmpItem, lastItem);
			P("fOptionList->AddUnder(%s, %s)\n", tmpItem->Text(), lastItem->Text());
			lastItem = tmpItem;
		}
	}

	AddChild(sv);

	// add ok button
	const int32 Width = 300;
	const int32 Height = 500;
	BRect r(Width - BUTTON_WIDTH - 10, Height - BUTTON_HEIGHT - 10,
			Width - 10, Height - 11);
	BButton *button;
	button = new BButton(r, "be:ok", "Save", new BMessage(OPTION_OK), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(button);
	button->MakeDefault(true);
	
	r.Set((Width - BUTTON_WIDTH - 10) - BUTTON_WIDTH - 12,
			Height - BUTTON_HEIGHT - 10,
			(Width - BUTTON_WIDTH - 10) - 12,
			Height - 11);
	button = new BButton(r, "be:cancel", "Cancel", new BMessage(OPTION_CANCEL), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(button);


}

void
OptionView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case UI_SELECTED:
		{
			int32 item = fOptionList->FullListCurrentSelection();
			UIStringItem *strItem = (UIStringItem*)fOptionList->FullListItemAt(item);		
			if(!strItem) { return; }

			BView *child = FindView("dyview");
			if(child) {
				RemoveChild(child);
				delete child;
			}
			
			UI *ui = strItem->Target();
			if(ui){ 	// avoids Group markers
				BuildInterface(ui);
			}
			break;
		}
		
		case UI_ITEM_SELECTED:
		{
			void *v;
			Invocation *inv;
			if(msg->HasPointer("invocation")){
				msg->FindPointer("invocation", &v);
				inv = static_cast<Invocation*>(v);
				inv->IsEnabled(true);
			}
			UpdateStringItems();
			break;
		}

		case CBOX:
		{
			UpdateStringItems();
			break;			
		}

	}
}

void
OptionView::UpdateStringItems()
{
	fOptionList->Draw(fOptionList->Bounds());
}

void
OptionView::BuildInterface(UI *ui)
{
	P("ui: 0x%x\n", ui);
	P("ui->name: [%s]\n", ui->Name());
	switch(ui->InterfaceType())
	{
	case BOOLEAN:
		BuildBooleanInterface(ui);
		break;
	case PICKONE:
		BuildListInterface(ui, PICKONE);
		break;
	case PICKMANY:
		BuildListInterface(ui, PICKMANY);
		break;
	}

	if(fCurrentUI) fCurrentUI->IsEnabled(false);
	fCurrentUI = ui;
	fCurrentUI->IsEnabled(true);
}

void
OptionView::BuildBooleanInterface(UI *ui)
{
	BRect r(10, 0, 290, 130);
	r.top = (fOptionList->Frame().bottom + 10);
	r.bottom += r.top;
	BView *view = new BView(r, "dyview", B_FOLLOW_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	const char *title = ui->Translation();
	if(title == NULL || strlen(title) == 0)
		title = ui->Name();

	BBox *bb;
	bb = new BBox(BRect(10,10,270,120));
	bb->SetLabel(ui->Translation());

	InvocationRadioButton *rb;

	Invocation *inv = ui->GetFirstInvoke();
	rb = new InvocationRadioButton(BRect(10,30,220,50), inv);
	if(inv->IsEnabled()) rb->SetValue(1);
	if(!ui->IsAvailable() || !inv->IsAvailable()){
		rb->SetEnabled(false);
	}
	bb->AddChild(rb);
	
	inv = ui->GetNextInvoke();
	rb = new InvocationRadioButton(BRect(10,60,220,80), inv);
	if(inv->IsEnabled()) rb->SetValue(1);
	if(!ui->IsAvailable() || !inv->IsAvailable()){
		rb->SetEnabled(false);
	}
	bb->AddChild(rb);
	
	view->AddChild(bb);
	AddChild(view);

}

void
OptionView::BuildListInterface(UI* ui, UIType)
{
	BRect r(10, 0, 290, 130);
	r.top = (fOptionList->Frame().bottom + 10);
	r.bottom += r.top;
	BView *view = new BView(r, "dyview", B_FOLLOW_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	const char *label = ui->Translation();
	if(label == NULL || strlen(label) == 0)
		label = ui->Name();

	const char *item_name;
	BMessage *msg;
	int32 counter = 0;
	PPDMenuItem *item;
	BPopUpMenu *menu = new BPopUpMenu("menu");
	for(Invocation *inv=ui->GetFirstInvoke(); inv; inv=ui->GetNextInvoke()){
		item_name = inv->Translation();
		msg = new BMessage(UI_ITEM_SELECTED);
		msg->AddInt32("item", counter++);
		msg->AddPointer("invocation", inv);
		menu->AddItem(item = new PPDMenuItem(item_name, msg));	
		if(inv->IsEnabled()){
			item->SetMarked(true);
		}
		if(!inv->IsAvailable())
			item->SetConflicted(true);
	}

	BMenuField *mf = new BMenuField(BRect(20, 40, 240, 60), "menufield", label,
									menu);
	if(!ui->IsAvailable()){
		P("Huh?  This ui is not available??\n");
		mf->SetEnabled(false);
	}

	BBox *bb;
	bb = new BBox(BRect(10,10,270,120));
	bb->SetLabel(ui->Translation());

	bb->AddChild(mf);
	view->AddChild(bb);
	
	AddChild(view);	
}

void
OptionView::UpdateMessage(BMessage *msg)
{
	UIStringItem *strItem;
	UI *ui;
	Invocation *inv;

	/* some printers require these after tray-handling */
	char *deferPageSize = NULL;
	char *deferPageRegion = NULL;

	BMessage *ppdMessage = new BMessage;
	
	AttachJclInvocations(ppdMessage);

	for(int count = 0;
		(strItem = (UIStringItem*)fOptionList->FullListItemAt(count));
		count++){
		
		ui = strItem->Target();
		if(!ui) continue;
		
		for(inv=ui->GetFirstInvoke(); inv; inv=ui->GetNextInvoke()){
			if(inv->IsEnabled() && !inv->IsDefault()){
				char *key = inv->GetEncodedName();
				if(strcmp(inv->Name(), "PageSize") == 0){
					deferPageSize = key;
					continue;
				} else if(strcmp(inv->Name(), "PageRegion") == 0) {
					deferPageRegion = key;
					continue;
				}
				PP("Name: [%s] Option: [%s] EncodedName: [%s]\n",
					inv->Name(), inv->Option(), key);
				ppdMessage->AddBool(key, true);
				free (key);
			}
		}
	}

	// handle page size and region
	if(deferPageSize){
		ppdMessage->AddBool(deferPageSize, true);
		free(deferPageSize);
	}
	
	if(deferPageRegion){
		ppdMessage->AddBool(deferPageRegion, true);
		free(deferPageRegion);
	}

	if(msg->HasMessage("ppd_item")){
		msg->ReplaceMessage("ppd_item", ppdMessage);
	} else {
		msg->AddMessage("ppd_item", ppdMessage);	
	}

	P("After Updating message, setup message looks like this:\n");
	//ppdMessage->PrintToStream();

	delete ppdMessage;
}

void
OptionView::AttachJclInvocations(BMessage *msg)
{
	PPD *ppd = PPD::Instance();
	UI *ui;
	Invocation *inv;
	for(ui=ppd->GetFirstJCL(); ui; ui=ppd->GetNextJCL()){
		for(inv=ui->GetFirstInvoke(); inv; inv=ui->GetNextInvoke()){
			if(inv->IsEnabled() && !inv->IsDefault()){
				char *key = inv->GetEncodedName();
				P("Name: [%s] Option: [%s] EncodedName: [%s]\n",
					inv->Name(), inv->Option(), key);
				msg->AddBool(key, true);
				free(key);
			}
		}
	}
}

void
OptionView::UpdateSetupWindow(Invocation *inv)
{
#if 0
	if(strcmp(inv->Parent()->Name(), "PageSize")){
		// send a PAPER_SIZE_CHANGED message to the Setup window.
	}
#endif
}
