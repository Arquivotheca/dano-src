/*	ShowWindow.cpp	*/

#include <TranslatorRoster.h>
#include <BitmapStream.h>

#include "ShowWindow.h"
#include "BitmapView.h"
#include "prefs.h"

#include <stdio.h>
#include <Application.h>
#include <FilePanel.h>
#include <Message.h>
#include <Alert.h>
#include <string.h>
#include <ResourceStrings.h>
#include <NodeInfo.h>
#include <MessageFilter.h>
#include <InterfaceDefs.h>
#include <InterfaceKit.h>
#include <Path.h>

#include <interface/PrintJob.h>
#include <print/PrintPanel.h>

#include "RecentItems.h"

extern BResourceStrings g_strings;


#define OPEN_ITEM	'open'
#define QUIT_ITEM	'quit'
#define ABOUT_ITEM 'abot'

#define SAVE_AS_ITEM 'sava'

#define PRINT_CONFIG_ITEM	'prtc'
#define PRINT_ITEM			'prtj'
#define PRINT_PREVIEW_TIEM	'prtp'
#define PRINT_PANEL_MESSAGE	'PANL'

#define DITHER_ITEM 'ditr'

#define UNDO_ITEM 'undo'
#define CUT_ITEM 'cut '
#define COPY_ITEM 'copy'
#define PASTE_ITEM 'past'
#define CLEAR_ITEM 'clea'
#define SELECT_ALL_ITEM 'sela'
#define SLICE_ITEM 'slic'
#define ZOOM_ITEM 'zoom'

extern float MENU_BAR_HEIGHT;


static filter_result filter_commands
	(BMessage *message, BHandler **target, BMessageFilter *filter)
{
	const char * bytes;
	if (!message->FindString("bytes", &bytes)) {
		uint32 what = 0;
		int32 modifiers = 0;
		message->FindInt32("modifiers", &modifiers);
		if (*bytes == B_FUNCTION_KEY) {
			int32 key;
			message->FindInt32("key", &key);
			switch (key) {
			case B_F1_KEY:
				what = UNDO_ITEM;
				break;
			case B_F2_KEY:
				what = CUT_ITEM;
				break;
			case B_F3_KEY:
				what = COPY_ITEM;
				break;
			case B_F4_KEY:
				what = PASTE_ITEM;
				break;
			case B_F5_KEY:
				what = CLEAR_ITEM;
				break;
			case B_F6_KEY:
				what = SELECT_ALL_ITEM;
				break;
			case B_F7_KEY:
				what = DITHER_ITEM;
				break;
			case B_F8_KEY:
				what = OPEN_ITEM;
				break;
			case B_F12_KEY:
				what = QUIT_ITEM;
				break;
			}
		}
		else switch (*bytes) {
			case B_DELETE:
			case B_BACKSPACE:
				if (dynamic_cast<BTextView*>(*target) == NULL) {
					if (!(modifiers & B_CONTROL_KEY)) {
						what = CLEAR_ITEM;
						break;
					}
					what = CUT_ITEM;
				}
				break;
			case B_INSERT:
				what = PASTE_ITEM;
				break;
		}
		if (what) {
			filter->Looper()->PostMessage(what);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}


static BMenu *
MakeExportMenu(
	const char * title,
	int32 * o_format, 
	int32 * o_translator,
	BMessage * o_dnd_types)
{
	bool first = true;
	/* create the menu that we'll stick items into */
	BMenu * exports = new BMenu(title);

	/* for convenience of coding, cache the default list */
	BTranslatorRoster *r = BTranslatorRoster::Default();

	/* we're going to ask each and every translator */
	translator_id * trans;
	int32 count;
	if (r->GetAllTranslators(&trans, &count))
	{
		delete exports;
		return NULL;
	}

	/* find out which of them can export for us */
	int32 actuals = 0;
	for (int ix=0; ix<count; ix++)
	{
		/* they have to have B_TRANSLATOR_BITMAP among inputs */
		const translation_format * formats;
		int32 nInput;
		if (r->GetInputFormats(trans[ix], &formats, &nInput))
		{
			continue;
		}
		for (int iy=0; iy<nInput; iy++)
		{
			if (formats[iy].type == B_TRANSLATOR_BITMAP)
			{	/* using goto avoids pesky flag variables! */
				goto do_this_translator;
			}
		}
		continue; /* didn't have translator bitmap */

	do_this_translator:
		/* figure out what the Translator can write */
		int32 nOutput;
		if (r->GetOutputFormats(trans[ix], &formats, &nOutput))
		{
			continue;
		}
		/* add everything besides B_TRANSLATOR_BITMAP to outputs */
		for (int iy=0; iy<nOutput; iy++)
		{
			if (formats[iy].type != B_TRANSLATOR_BITMAP)
			{
				if (first) {
					first = false;
					*o_format = formats[iy].type;
					*o_translator = trans[ix];
				}
				o_dnd_types->AddString("be:types", formats[iy].MIME);
				o_dnd_types->AddString("be:type_descriptions", formats[iy].name);
				o_dnd_types->AddInt32("be:_format", formats[iy].type);
				o_dnd_types->AddInt32("be:_translator", trans[ix]);
				BMessage * msg = new BMessage(SAVE_AS_ITEM);
				msg->AddInt32("format", formats[iy].type);
				msg->AddInt32("translator", trans[ix]);
				msg->AddBool("force", true);
				BMenuItem * item = new BMenuItem(formats[iy].name, msg);
				exports->AddItem(item);
				actuals++;
			}
		}
	}

	/* done with list of installed translators */
	delete[] trans;

	/* if there is nothing, we return nothing */
	if (!actuals)
	{
		delete exports;
		exports = NULL;
	}
	return exports;
}


static status_t
SlicesRefFromRef(
	entry_ref * outSlices,
	const entry_ref * ref)
{
	*outSlices = entry_ref();
	
	if (*ref == entry_ref()) return B_BAD_VALUE;
	
	BString name = ref->name;
	int32 end = name.FindLast('.');
	if (end > 0)
		name.Truncate(end);
	name += ".slice";
	
	*outSlices = *ref;
	outSlices->set_name(name.String());
	
	return B_OK;
}

class SliceDetails : public BControl
{
public:
	SliceDetails(BRect frame, const char* name, BMessage* msg, uint32 resizeMask)
		: BControl(frame, name, "", msg, resizeMask,
					B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP)
	{
		fName = new BTextControl(frame, "Name", "Region: ", "",
								 new BMessage('name'), B_FOLLOW_NONE);
		AddChild(fName);
		fName->ResizeToPreferred();
		
		static const struct {
			const char* name;
			const char* label;
			uint32 code;
		} dimen_controls[_NUM_CONTROL] = {
			{ "Left", " (", 'left' },
			{ "Top", ",", 'top ' },
			{ "Right", ") - (", 'rght' },
			{ "Bottom", ",", 'bttm' },
			{ "Width", ")  Width: ", 'wdth' },
			{ "Height", "  Height: ", 'hght' }
		};
		for (int32 i=0; i < _NUM_CONTROL; i++) {
			fDimens[i] = new BTextControl(frame, dimen_controls[i].name,
										  dimen_controls[i].label,
										  "",
										  new BMessage(dimen_controls[i].code),
										  B_FOLLOW_NONE);
			AddChild(fDimens[i]);
			fDimens[i]->ResizeToPreferred();
		}
		fHaveSize = false;
	}

	virtual void AttachedToWindow()
	{
		BControl::AttachedToWindow();
		
		SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
		SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
		SetColorsFromParent();
		
		BMessenger me(this);
		fName->SetTarget(me);
		for (int32 i=0; i<_NUM_CONTROL; i++)
			fDimens[i]->SetTarget(me);
		LayoutViews(true);
	}
	
	virtual void GetPreferredSize(float* w, float* h)
	{
		if (!fHaveSize) LayoutViews(false);
		*w = fPrefWidth;
		*h = fPrefHeight;
	}
	
	virtual void FrameResized(float w, float h)
	{
		BControl::FrameResized(w, h);
		LayoutViews(true);
	}
	
	virtual void MessageReceived(BMessage* msg)
	{
		bool used = false;
		bool changed = false;
		
		switch (msg->what) {
			case 'name':
				changed = used = true;
				break;
			case 'left':
			case 'rght':
				used = true;
				changed = Difference(WIDTH_CONTROL, RIGHT_CONTROL, LEFT_CONTROL,
									 msg->what == 'left');
				break;
			case 'wdth':
				used = true;
				changed = Sum(RIGHT_CONTROL, LEFT_CONTROL, WIDTH_CONTROL);
				break;
			case 'top ':
			case 'bttm':
				used = true;
				changed = Difference(HEIGHT_CONTROL, BOTTOM_CONTROL, TOP_CONTROL,
									 msg->what == 'top ');
				break;
			case 'hght':
				used = true;
				changed = Sum(BOTTOM_CONTROL, TOP_CONTROL, HEIGHT_CONTROL);
				break;
		}
		
		if (used) {
			if (changed && Message()) {
				BMessage msg(*Message());
				BString name;
				BRect rect;
				GetData(&name, &rect);
				msg.AddRect("slice", rect);
				msg.AddString("name", name);
				if (name != fOrigName) {
					msg.AddString("orig_name", fOrigName.String());
					fOrigName = name;
				}
				Invoke(&msg);
			}
		} else {
			BControl::MessageReceived(msg);
		}
	}
	
	void SetData(const char* name, BRect slice)
	{
		if (name) {
			fName->SetText(name);
			fOrigName = name;
			fName->SetEnabled(true);
			SetDimenText(LEFT_CONTROL, slice.left);
			SetDimenText(TOP_CONTROL, slice.top);
			SetDimenText(RIGHT_CONTROL, slice.right);
			SetDimenText(BOTTOM_CONTROL, slice.bottom);
			SetDimenText(WIDTH_CONTROL, slice.Width()+1);
			SetDimenText(HEIGHT_CONTROL, slice.Height()+1);
			for (int32 i=0; i<_NUM_CONTROL; i++) {
				fDimens[i]->SetEnabled(true);
			}
		} else {
			fName->SetText("");
			fName->SetEnabled(false);
			for (int32 i=0; i<_NUM_CONTROL; i++) {
				fDimens[i]->SetText("");
				fDimens[i]->SetEnabled(false);
			}
		}
	}
	
	void GetData(BString* outName, BRect* outRect)
	{
		*outName = fName->Text();
		outRect->left = GetDimenText(LEFT_CONTROL);
		outRect->top = GetDimenText(TOP_CONTROL);
		outRect->right = GetDimenText(RIGHT_CONTROL);
		outRect->bottom = GetDimenText(BOTTOM_CONTROL);
	}
	
private:
	void LayoutViews(bool position)
	{
		int32 i;
		const BRect bounds(Bounds());
		
		float nw, nh;
		fName->GetPreferredSize(&nw, &nh);
		nw = fName->StringWidth(fName->Label());
		const float numWidth = fName->StringWidth("888") + 15;
		const float numMax = fName->StringWidth("888888") + 15;
		float totWidth = nw;
		for (i=0; i<_NUM_CONTROL; i++)
			totWidth += fDimens[i]->StringWidth(fDimens[i]->Label());
		
		if (position) {
			float availWidth = bounds.Width();
			float extraWidth = availWidth-totWidth;
			if (extraWidth < 0) extraWidth = 0;
			float numExtra = (extraWidth/(_NUM_CONTROL+1));
			if (numExtra > numMax) {
				numExtra = numMax;
				extraWidth = availWidth-totWidth-(numExtra*_NUM_CONTROL);
			} else {
				extraWidth = numExtra;
			}
			
			Window()->BeginViewTransaction();
			fName->MoveTo(FRAME, FRAME);
			fName->ResizeTo(nw+extraWidth-2, nh);
			
			float pos = FRAME+nw+extraWidth;
			for (i=0; i<_NUM_CONTROL; i++) {
				float next = pos + fDimens[i]->StringWidth(fDimens[i]->Label())
							+ numExtra;
				float w = floor(next+.5) - floor(pos+.5);
				fDimens[i]->MoveTo(pos, FRAME);
				fDimens[i]->ResizeTo(w-2, nh);
				pos = next;
			}
			Window()->EndViewTransaction();
		}
		
		fPrefWidth = totWidth + numWidth*(_NUM_CONTROL+1) + FRAME*2;
		fPrefHeight = nh + FRAME*2;
		fHaveSize = true;
	}
	
	bool SetDimenText(int32 control, float val)
	{
		const int32 i = (int32)(val+.5);
		const char* text = fDimens[control]->Text();
		bool numeric = false;
		bool valid = true;
		while (*text) {
			if (*text >= '0' && *text <= '9') numeric = true;
			else if (*text != ' ') valid = false;
			text++;
		}
		if (!numeric || !valid || i != GetDimenText(control)) {
			BString text;
			text << (int32)(val+.5);
			fDimens[control]->SetText(text.String());
			return true;
		}
		return false;
	}
	float GetDimenText(int32 control)
	{
		return atoi(fDimens[control]->Text());
	}
	
	bool Difference(int32 target, int32 v1, int32 v2, bool conformLeft)
	{
		bool c = false;
		float n1 = GetDimenText(v1);
		float n2 = GetDimenText(v2);
		if (n2 > n1) {
			if (conformLeft) {
				if (SetDimenText(v1, n2)) c = true;
				n1 = n2;
			} else {
				if (SetDimenText(v2, n1)) c = true;
				n2 = n1;
			}
		}
		if (SetDimenText(target, n1 - n2 + 1)) c = true;
		return c;
	}
	
	bool Sum(int32 target, int32 v1, int32 v2)
	{
		return SetDimenText(target, GetDimenText(v2) + GetDimenText(v1) - 1);
	}
	
	enum {
		FRAME = 3
	};
	
	enum {
		LEFT_CONTROL = 0,
		TOP_CONTROL,
		RIGHT_CONTROL,
		BOTTOM_CONTROL,
		WIDTH_CONTROL,
		HEIGHT_CONTROL,
		_NUM_CONTROL
	};
	
	BTextControl* fName;
	BTextControl* fDimens[_NUM_CONTROL];
	
	BString fOrigName;
	
	bool fHaveSize;
	float fPrefWidth, fPrefHeight;
};

ShowWindow::ShowWindow(
	BRect area,
	const char * title,
	const entry_ref & ref,
	translator_id translator,
	uint32 format,
	window_type type,
	uint32 flags) :
	BWindow(area, title, type, flags | B_PULSE_NEEDED),
	fRef(ref), fTranslator(translator), fFormat(format)
{
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, filter_commands));
	SetPulseRate(200000);
	fDirty = fSlicesDirty = false;

	BMenuBar * bar = new BMenuBar(BRect(0,0,area.Width(), MENU_BAR_HEIGHT-1), "MainMenu");
	BMenu * file = new BMenu(g_strings.FindString(18));

	BMenuItem * open = new BMenuItem(BRecentFilesList::NewFileListMenu(g_strings.FindString(19),
		NULL, NULL, be_app, 10, false, "image", "application/x-vnd.Be-ShowImage"),
		new BMessage(OPEN_ITEM));
	open->SetShortcut('O', B_COMMAND_KEY);

	file->AddItem(open);

	file->AddSeparatorItem();

	/* Create the Save menu item if a file was given. */
	if (fRef != entry_ref() && fTranslator != -1) {
		BPath path(&fRef);
		path.GetParent(&path);
		BEntry entry(path.Path());
		entry_ref dir;
		if (entry.GetRef(&dir) == B_OK) {
			BMessage * msg = new BMessage(SAVE_AS_ITEM);
			msg->AddInt32("format", fFormat);
			msg->AddInt32("translator", fTranslator);
			msg->AddRef("directory", &dir);
			msg->AddString("name", fRef.name);
			BMenuItem * save = new BMenuItem(g_strings.FindString(39), msg, 'S');
			file->AddItem(save);
		}
	}
	
	/* Create the Save As menu item, if there are available translators */
	fDragFormat = -1;
	fDragTranslator = -1;
	BMenu * exports = MakeExportMenu(g_strings.FindString(21), &fDragFormat, &fDragTranslator, &fDNDTypes);
	if (exports != NULL) file->AddItem(exports);

	BMenuItem * close = new BMenuItem(g_strings.FindString(20), new BMessage(B_QUIT_REQUESTED), 'W');
	file->AddItem(close);

	/* print menu items */
	file->AddSeparatorItem();
	BMenuItem * printconfig = new BMenuItem(g_strings.FindString(100), new BMessage(PRINT_CONFIG_ITEM), 'P', B_SHIFT_KEY);
	file->AddItem(printconfig);
	BMenuItem * print = new BMenuItem(g_strings.FindString(101), new BMessage(PRINT_ITEM), 'P');
	file->AddItem(print);
	BMenuItem * printpreview = new BMenuItem(g_strings.FindString(102), new BMessage(PRINT_PREVIEW_TIEM), 'P', B_SHIFT_KEY|B_OPTION_KEY);
	file->AddItem(printpreview);

	file->AddSeparatorItem();
	BMenuItem * about = new BMenuItem(g_strings.FindString(22), new BMessage(ABOUT_ITEM));
	file->AddItem(about);
	file->AddSeparatorItem();
	BMenuItem * quit = new BMenuItem(g_strings.FindString(6), new BMessage(QUIT_ITEM), 'Q');
	file->AddItem(quit);
	bar->AddItem(file);

	BMenu * edit = new BMenu(g_strings.FindString(29));
	BMenuItem * undo = new BMenuItem(g_strings.FindString(30), new BMessage(UNDO_ITEM), 'Z');
	edit->AddItem(undo);
	edit->AddSeparatorItem();
	BMenuItem * cut = new BMenuItem(g_strings.FindString(31), new BMessage(CUT_ITEM), 'X');
	edit->AddItem(cut);
	BMenuItem * copy = new BMenuItem(g_strings.FindString(32), new BMessage(COPY_ITEM), 'C');
	edit->AddItem(copy);
	BMenuItem * paste = new BMenuItem(g_strings.FindString(33), new BMessage(PASTE_ITEM), 'V');
	edit->AddItem(paste);
	BMenuItem * clear = new BMenuItem(g_strings.FindString(34), new BMessage(CLEAR_ITEM));
	edit->AddItem(clear);
	edit->AddSeparatorItem();
	BMenuItem * select_all = new BMenuItem(g_strings.FindString(35), new BMessage(SELECT_ALL_ITEM), 'A');
	edit->AddItem(select_all);
	edit->AddSeparatorItem();
	BMenuItem * slice = new BMenuItem(g_strings.FindString(40), new BMessage(SLICE_ITEM));
	edit->AddItem(slice);
	bar->AddItem(edit);

	BMenu * view = new BMenu(g_strings.FindString(23));
	BMenuItem * dither = new BMenuItem(g_strings.FindString(24), new BMessage(DITHER_ITEM));
	view->AddItem(dither);
	view->AddSeparatorItem();
	static const struct { int32 id; float factor; } zoom_levels[] = {
		{ 42, .25 }, { 43, .50 }, { 44, .75 }, { 45, 1.0 },
		{ 46, 2.0 }, { 47, 4.0 }, { 48, 8.0 }, { 49, 12.0 },
		{ 50, 16.0 },
		{ -1, 0 }
	};
	for (int32 i=0; zoom_levels[i].id >= 0; i++) {
		BMessage * msg = new BMessage(ZOOM_ITEM);
		msg->AddFloat("factor", zoom_levels[i].factor);
		BMenuItem * zoom = new BMenuItem(g_strings.FindString(zoom_levels[i].id), msg);
		view->AddItem(zoom);
	}
	bar->AddItem(view);

	AddChild(bar);

	float min_h = 0, min_v = 0, max_h = 1600, max_v = 1200;
	GetSizeLimits(&min_h, &max_h, &min_v, &max_v);
	min_h = 300;
	min_v = 100+MENU_BAR_HEIGHT;
	if (Bounds().Width() < min_h) {
		ResizeTo(min_h, Bounds().Height());
	}
	if (Bounds().Height() < min_v) {
		ResizeTo(Bounds().Width(), min_v);
	}
	SetSizeLimits(min_h, max_h, min_v, max_v);

	SetPulseRate(100000);

	thePanel = NULL;
	saveAsPanel = NULL;
	fSliceDetails = NULL;

	fPrintPanel = NULL;
	fPrintSettings = NULL;
}


ShowWindow::~ShowWindow()
{
	delete fPrintPanel;
	delete fPrintSettings;
	delete thePanel;
	delete saveAsPanel;
	be_app->PostMessage(msg_WindowDeleted);
}

void
ShowWindow::MessageReceived(
	BMessage * message)
{
	BitmapView * bmv = dynamic_cast<BitmapView *>(ChildAt(1));
	switch (message->what) {
	case PRINT_PANEL_MESSAGE:
		if (!fPrintSettings)	fPrintSettings = new BMessage(*message);
		else					*fPrintSettings = *message;
		break;
	case OPEN_ITEM:
		DoOpen(message);
		break;
	case QUIT_ITEM:
		be_app->PostMessage(B_QUIT_REQUESTED);
		break;
	case PRINT_CONFIG_ITEM:
		DoPrintSetup(bmv);
		break;
	case PRINT_ITEM:
		DoPrint(bmv);
		break;
	case PRINT_PREVIEW_TIEM:
		DoPrint(bmv, true);
		break;
	case ABOUT_ITEM:
		be_app->PostMessage(B_ABOUT_REQUESTED);
		break;
	case SAVE_AS_ITEM:
		DoSaveAs(message);
		break;
	case UNDO_ITEM:
		if (bmv) bmv->Undo();
		break;
	case CUT_ITEM:
		if (bmv) bmv->Cut();
		break;
	case COPY_ITEM:
		if (bmv) bmv->Copy();
		break;
	case PASTE_ITEM:
		if (bmv) bmv->Paste();
		break;
	case CLEAR_ITEM:
		if (bmv) bmv->Clear();
		break;
	case SELECT_ALL_ITEM:
		if (bmv) bmv->SelectAll();
		break;
	case SLICE_ITEM:
		if (bmv) EnableSlicing(!bmv->IsSlicing());
		break;
	case DITHER_ITEM:
		be_app->PostMessage(message);
		break;
	case ZOOM_ITEM:
		{
			float factor;
			if (message->FindFloat("factor", &factor) == B_OK && bmv)
				bmv->SetZoomFactor(factor);
		} break;
	case 'slcg':
		{
			if (bmv) {
				const char* name;
				BRect slice;
				if (message->FindString("name", &name) == B_OK &&
						message->FindRect("slice", &slice) == B_OK) {
					const char* orig;
					if (message->FindString("orig_name", &orig) == B_OK)
						bmv->RemoveSlice(orig);
					else
						orig = NULL;
					bmv->AddSlice(name, slice, orig ? true : false);
					name = bmv->ActiveSlice(&slice);
					fSliceDetails->SetData(name, slice);
				}
			}
		} break;
	case msg_SetDirty: {
			bool b = true;
			if (message->FindBool("dirty", &b) == B_OK)
				fDirty = b;
			if (message->FindBool("slices", &b) == B_OK) {
				BRect slice;
				const char* name = bmv->ActiveSlice(&slice);
				fSliceDetails->SetData(name, slice);
				fSlicesDirty = b;
			}
		} break;
	case msg_SelectSlice: {
			BRect slice;
			const char* name = bmv->ActiveSlice(&slice);
			fSliceDetails->SetData(name, slice);
		} break;
	default:
		BWindow::MessageReceived(message);
		break;
	}
}


bool
ShowWindow::QuitRequested()
{
	if (!fDirty && !fSlicesDirty) return true;
	Activate(true);
	char str[300];
	sprintf(str, g_strings.FindString(36), Title());
	if (1 == (new BAlert("", str, g_strings.FindString(37), g_strings.FindString(20)))->Go()) {
		return true;
	}
	return false;
}


void
ShowWindow::DoOpen(
	BMessage * message)
{
	if (!message->HasRef("refs")) {
		be_app->PostMessage(msg_ShowOpenPanel);
	}
	else {
		message->what = B_REFS_RECEIVED;
		be_app->PostMessage(message);
	}
}

void
ShowWindow::DoSaveAs(
	BMessage * message)
{
	/* Retrieve the specific format from the Save As menu item */
	int32 format;
	int32 translator;
	bool ok = true;
	if (message->FindInt32("format", &format) ||
		message->FindInt32("translator", &translator)) {
		ok = false;	/* this shouldn't happen */
	}
	else {
		fDragFormat = format;
		fDragTranslator = translator;
	}

	/* First, is this a Save As from menu, or from file panel? */
	if (!saveAsPanel)
	{
		saveAsPanel = new BFilePanel(B_SAVE_PANEL, 
			new BMessenger(this), (const entry_ref *)NULL, 0, false, NULL, 
			NULL, false, true);
	}
	if (!message->HasRef("directory"))
	{
		saveAsPanel->SetMessage(message);
		saveAsPanel->Show();
		return;
	}

	/* Here, it seems we're saving from a panel */
	entry_ref dir_ref;
	const char * name = NULL;
	if (message->FindRef("directory", &dir_ref) ||
		message->FindString("name", &name))
	{
		return; /* Error happened, don't crash! */
	}
	BDirectory dir(&dir_ref);

	/* Find the bitmap we want to export (app-specific) */
	BitmapView * bmv = dynamic_cast<BitmapView*>(ChildAt(1));
	if (bmv == NULL) {
		(new BAlert("", g_strings.FindString(28), g_strings.FindString(10)))->Go();
		return;
	}
	BBitmap * bitmap = bmv->GetBitmap();

	if (!bitmap || !ok) {
		return;
	}

	const bool force = message->FindBool("force");
	
	if (force || fDirty) {
		/* Set up the output file */
		BFile output;
		status_t err = output.SetTo(&dir, name, O_RDWR | O_CREAT | O_TRUNC);
	
		/* Use the handy utility stream class to read/write a bitmap in-site */
		BBitmapStream input(bitmap);
	
		/* Set the MIME type of the output file */
		int32 count_fmt = 0;
		const translation_format * out_fmt = NULL;
		if (!BTranslatorRoster::Default()->GetOutputFormats(translator, &out_fmt, &count_fmt)) {
			for (int ix=0; ix<count_fmt; ix++) {
				if (out_fmt[ix].type == format) {
					BNodeInfo info(&output);
					info.SetType(out_fmt[ix].MIME);
					break;
				}
			}
		}
		/* Do the actual saving -- note that this is just a translation */
		if (err >= B_OK)
			err = BTranslatorRoster::Default()->Translate(
				translator, &input, NULL, &output, format);
		
		/* Make sure BitmapStream won't delete our bitmap */
		input.DetachBitmap(&bitmap);
		
		if (err >= B_OK) err = output.Sync();
		if (err < B_OK)
		{
			char str[350];
			sprintf(str, g_strings.FindString(25), name, strerror(err), err);
			BAlert * alrt = new BAlert("", str, g_strings.FindString(26));
			alrt->Go();
			BEntry ent(&dir, name);
			ent.Remove();
		}
		else {
			fDirty = false;
			SetTitle(name);
			BEntry ent(&dir, name);
			ent.GetRef(&fRef);
			SlicesRefFromRef(&fSliceRef, &fRef);
			if (fPrintPanel) {
				BString title(g_strings.FindString(103));
				title << " (" << name << ")";
				fPrintPanel->SetTitle(title);
			}
		}
	}
	
	if ((force || fSlicesDirty) && fSliceRef != entry_ref()) {
		BMessage slices;
		bmv->GetSlices(&slices);
		if (slices.IsEmpty()) {
			// Delete the slice file if the slices have changed.
			if (fSlicesDirty) {
				BEntry ent(&fSliceRef);
				if (ent.InitCheck() == B_OK) {
					ent.Remove();
					fSlicesDirty = false;
				}
			}
		} else {
			BFile output;
			status_t err = output.SetTo(&fSliceRef, O_RDWR | O_CREAT | O_TRUNC);
			if (err >= B_OK) err = slices.Flatten(&output);
			if (err >= B_OK) err = output.Sync();
			if (err < B_OK) {
				char str[350];
				sprintf(str, g_strings.FindString(25), fSliceRef.name, strerror(err), err);
				BAlert * alrt = new BAlert("", str, g_strings.FindString(26));
				alrt->Go();
				BEntry ent(&fSliceRef);
				ent.Remove();
			} else {
				fSlicesDirty = false;
			}
		}
	}
}


status_t
ShowWindow::LoadSlices()
{
	BitmapView * bmv = dynamic_cast<BitmapView*>(ChildAt(1));
	if (bmv == NULL)
		return B_UNSUPPORTED;
	
	status_t result = SlicesRefFromRef(&fSliceRef, &fRef);
	
	BFile file;
	if (result >= B_OK) result = file.SetTo(&fSliceRef, B_READ_ONLY);
	
	BMessage slices;
	if (result >= B_OK) result = slices.Unflatten(&file);
	
	if (result >= B_OK) {
		bmv->SetSlices(&slices);
		EnableSlicing(true);
	} else {
		EnableSlicing(false);
	}
	
	return B_OK;
}

void
ShowWindow::EnableSlicing(
	bool	enabled)
{
	const bool current = fSliceDetails ? true : false;
	if (current == enabled) return;
	
	BitmapView * bmv = dynamic_cast<BitmapView*>(ChildAt(1));
	if (bmv == NULL)
		return;
	const BRect frame(bmv->Frame());
	
	BeginViewTransaction();
	if (enabled) {
		fSliceDetails = new SliceDetails(BRect(-100, -100, -10, -10), "SliceDetails",
										 new BMessage('slcg'),
										 B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
		AddChild(fSliceDetails);
		float pw, ph;
		fSliceDetails->GetPreferredSize(&pw, &ph);
		bmv->MoveTo(frame.left, frame.top+ph);
		bmv->ResizeTo(frame.Width(), frame.Height()-ph);
		bmv->EnableSlicing(true);
		BRect slice;
		const char* name = bmv->ActiveSlice(&slice);
		fSliceDetails->SetData(name, slice);
		fSliceDetails->ResizeTo(frame.Width(), ph);
		fSliceDetails->MoveTo(frame.left, frame.top);
	} else {
		BRect pFrame = fSliceDetails->Frame();
		RemoveChild(fSliceDetails);
		delete fSliceDetails;
		fSliceDetails = NULL;
		bmv->EnableSlicing(false);
		bmv->ResizeTo(frame.Width(), frame.Height()+pFrame.Height());
		bmv->MoveTo(frame.left, pFrame.top);
	}
	EndViewTransaction();
}

bool
ShowWindow::GetDragInfo(
	int32 * o_format,
	int32 * o_translator)
{
	if (fDragFormat == -1) return false;
	*o_format = fDragFormat;
	*o_translator = fDragTranslator;
	return true;
}


void
ShowWindow::AddDNDTypes(
	BMessage * intoMessage)
{
	const char * type;
	bool first = true;
	for (int ix=0; !fDNDTypes.FindString("be:types", ix, &type); ix++) {
		const char * name = "";
		fDNDTypes.FindString("be:type_descriptions", ix, &name);
		if (first)
			intoMessage->AddString("be:types", B_FILE_MIME_TYPE);

		intoMessage->AddString("be:types", type);
		intoMessage->AddString("be:filetypes", type);
		intoMessage->AddString("be:type_descriptions", name);
	}
}


bool
ShowWindow::MatchType(
	const char * mime_type,
	int32 * o_format,
	int32 * o_translator,
	const char *type_name)
{
	const char * str = NULL;
	for (int ix=0; !fDNDTypes.FindString("be:types", ix, &str); ix++) {
		const char *name = 0;
		fDNDTypes.FindString("be:type_descriptions", ix, &name);
		if (!strcasecmp(str, mime_type) && (!type_name || !strcmp(name, type_name))) {
			// match by mime type and, if specified, by specific type name
			// (this provides a way to distinguish between say two image/gif translators
			if (!fDNDTypes.FindInt32("be:_format", ix, o_format) && 
				!fDNDTypes.FindInt32("be:_translator", ix, o_translator)) {
				return true;
			}
		}
	}
	return false;
}

void
ShowWindow::MenusBeginning()
{
	BWindow::MenusBeginning();
	KeyMenuBar()->FindItem(DITHER_ITEM)->SetMarked(g_prefs.dither);
	BitmapView * bmv = dynamic_cast<BitmapView *>(ChildAt(1));
	bool has_undo = (bmv ? bmv->CanUndo() : false);
	KeyMenuBar()->FindItem(UNDO_ITEM)->SetEnabled(has_undo);
	bool has_selection = (bmv ? bmv->HasSelection() : false);
	KeyMenuBar()->FindItem(CUT_ITEM)->SetEnabled(has_selection);
	KeyMenuBar()->FindItem(COPY_ITEM)->SetEnabled(has_selection);
	KeyMenuBar()->FindItem(PASTE_ITEM)->SetEnabled(bmv ? bmv->CanPaste() : false);
	KeyMenuBar()->FindItem(CLEAR_ITEM)->SetEnabled(has_selection);
	if (bmv && fSliceRef != entry_ref()) {
		KeyMenuBar()->FindItem(SLICE_ITEM)->SetEnabled(true);
		KeyMenuBar()->FindItem(SLICE_ITEM)->SetMarked(bmv->IsSlicing());
	} else {
		KeyMenuBar()->FindItem(SLICE_ITEM)->SetEnabled(false);
		KeyMenuBar()->FindItem(SLICE_ITEM)->SetMarked(false);
	}
	BMenuItem * zoom = KeyMenuBar()->FindItem(ZOOM_ITEM);
	if (zoom) {
		BMenu * menu = zoom->Menu();
		for (int32 i=0; i<menu->CountItems(); i++) {
			zoom = menu->ItemAt(i);
			if (zoom && zoom->Message() && zoom->Message()->what == ZOOM_ITEM) {
				if (bmv) {
					float factor;
					if (zoom->Message()->FindFloat("factor", &factor) == B_OK &&
							factor == bmv->ZoomFactor())
						zoom->SetMarked(true);
					else
						zoom->SetMarked(false);
					zoom->SetEnabled(true);
				} else {
					zoom->SetEnabled(false);
					zoom->SetMarked(false);
				}
			}
		}
	}
}

void ShowWindow::DoPrintSetup(BitmapView *bmv)
{
	if (fPrintPanel == NULL) {
		fPrintPanel = new BPrintPanel(BPrintPanel::B_CONFIG_JOB, fPrintSettings, NULL, NULL, BPrintPanel::B_HIDE_ON_CANCEL);
		fPrintPanel->SetTarget(BMessenger(this));
		fPrintPanel->SetTemplateMessage(BMessage(PRINT_PANEL_MESSAGE));
		BString title(g_strings.FindString(103));
		title << " (" << Title() << ")";
		fPrintPanel->SetTitle(title);
		fPrintPanel->SetPageCount(1);
		fPrintPanel->SetCurrentPage(1);
	}
	fPrintPanel->Show();
}

void ShowWindow::DoPrint(BitmapView *bmv, bool preview)
{
	if (!bmv)
		return;

	if (fPrintSettings == NULL)
		fPrintSettings = new BMessage();
	
	BPrintJob printjob(*fPrintSettings, Title(), preview);
	if (printjob.InitCheck() != B_OK) {
		printjob.HandleError(printjob.InitCheck());
		return;
	}

	status_t err;
	if ((err = printjob.BeginJob()) != B_OK) {
		printjob.HandleError(err);
		return;
	}

	// scale and center the bitmap
	BRect b = bmv->RealBitmapBounds();
	const float ratio = (b.Height()+1)/(b.Width()+1);
	BRect p = printjob.PaperRect().InsetBySelf((2.0/2.54)*72.0, (2.0/2.54)*72.0); // 2 cm margins
	p = (p & printjob.PrintableRect()) - printjob.PrintableRect().LeftTop();
	BRect d = p.OffsetToCopy(B_ORIGIN);
	if ((d.Width() * ratio) <= p.Height()) {
		d.bottom = d.Width() * ratio;
	} else {
		d.right = d.Height() / ratio;
	}
	d.OffsetTo(p.left + 0.5*(p.Width()-d.Width()), p.top + 0.5*(p.Height()-d.Height()));
	const float scale = (d.Width()+1) / (b.Width()+1);
	printjob.SetScale(scale);

	if ((err = printjob.DrawView(bmv, b, d.LeftTop())) != B_OK) {
		printjob.HandleError(err);
		return;
	}

	if ((err = printjob.SpoolPage()) != B_OK) {
		printjob.HandleError(err);
		return;
	}

	if ((err = printjob.CommitJob()) != B_OK) {
		printjob.HandleError(err);
		return;
	}
}
