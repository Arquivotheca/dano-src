#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Debug.h>

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <DataIO.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>
#include <TranslationUtils.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <DataIO.h>

#include <ResourceEditor.h>

#include <experimental/BitmapTools.h>

#include "IconEditor.h"
#include "IconPicker.h"
#include "BitmapEditor.h"

#include "utils.h"

enum {
	msg_set_zoom = 'zoom'
};

static rgb_color
DesktopColor(void)
{
	BScreen b(B_MAIN_SCREEN_ID);
	
	return b.DesktopColor();
}

// *********************** TCustomFatBitsEditor ***********************

// This class intercepts change reports from the base fat bits editor,
// and forwards them up to TIconEditor.

class TCustomBitmapEditor : public TBitmapEditor {
public:
	TCustomBitmapEditor(TIconEditor* editor, float width, float height,
						color_space cspace, const char* name);
	~TCustomBitmapEditor();
	
	void BitmapChanged(const char* what);
	void NewColorSelection(rgb_color color, bool primary);
	void NewHotSpot(int32 x, int32 y);
	
private:
	TIconEditor* fEditor;
};

TCustomBitmapEditor::TCustomBitmapEditor(TIconEditor* editor,
										 float width, float height,
										 color_space cspace,
										 const char* name)
	: TBitmapEditor(width, height, cspace, name),
	  fEditor(editor)
{
}

TCustomBitmapEditor::~TCustomBitmapEditor()
{
}

void
TCustomBitmapEditor::BitmapChanged(const char* what)
{
	if( fEditor ) fEditor->ReportBitsChange(this, what);
}

void
TCustomBitmapEditor::NewColorSelection(rgb_color color, bool primary)
{
	if( fEditor ) fEditor->DropperColorSelection(color, primary);
}

void
TCustomBitmapEditor::NewHotSpot(int32 x, int32 y)
{
	if( fEditor ) fEditor->ReportHotSpotChange(this, x, y);
}

// ****************************** TIconEditor ******************************

TIconEditor::TIconEditor(BRect r, const BMessage* configuration)
	: BView(r, "icon editor", B_FOLLOW_ALL,
		B_FRAME_EVENTS | B_WILL_DRAW | B_PULSE_NEEDED ),
	  fLayout(UNKNOWN_LAYOUT),
	  fPriWidth(32), fPriHeight(32), fPriColorSpace(B_CMAP8),
	  fSecWidth(16), fSecHeight(16), fSecColorSpace(B_CMAP8),
	  fControls(0), fIconPicker(0),
	  fPriEditor(0), fPriViewer(0), fPriScroller(0), fPriZoomer(0),
	  fSecEditor(0), fSecViewer(0), fSecScroller(0), fSecZoomer(0),
	  fColorPicker(0), fToolPicker(0)
{
	GetPrefs();
	if( configuration ) SetConfiguration(configuration);
}

TIconEditor::~TIconEditor()
{
	SetPrefs();
	DeleteIcon(&fPriEditor, &fPriViewer, &fPriScroller, &fPriZoomer);
	DeleteIcon(&fSecEditor, &fSecViewer, &fSecScroller, &fSecZoomer);
}

void
TIconEditor::BitmapChanged(TBitmapEditor* editor, const char* what, bool mini)
{
	(void)editor;
	(void)what;
	(void)mini;
	// not interested in this.
}

void
TIconEditor::HotSpotChanged(TBitmapEditor* editor, int32 x, int32 y, bool mini)
{
	(void)editor;
	(void)x;
	(void)y;
	(void)mini;
	// not interested in this.
}

void
TIconEditor::ReportBitsChange(TCustomBitmapEditor* editor, const char* what)
{
	if( editor == fPriEditor ) {
		BitmapChanged(editor, what, false);
	} else if( editor == fSecEditor ) {
		BitmapChanged(editor, what, true);
	}
}

void
TIconEditor::ReportHotSpotChange(TCustomBitmapEditor* editor,
								 int32 x, int32 y)
{
	if( x != editor->HotSpotX() || y != editor->HotSpotY() ) {
		editor->SetHotSpot(x, y);
		if( editor == fPriEditor ) {
			HotSpotChanged(editor, x, y, false);
		} else if( editor == fSecEditor ) {
			HotSpotChanged(editor, x, y, true);
		}
	}
}

void
TIconEditor::SetControls(BView* controls)
{
	if( fControls ) {
		fControls->RemoveSelf();
		delete fControls;
	}
	fControls = controls;
	if( fControls ) AddChild(fControls);
}

void
TIconEditor::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	if( Parent() ) SetViewColor(Parent()->ViewColor());
	else SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if( fPriEditor && !fPriEditor->Looper() ) {
		Window()->AddHandler(fPriEditor);
	}
	if( fSecEditor && !fSecEditor->Looper() ) {
		Window()->AddHandler(fSecEditor);
	}
	if( fIconPicker ) fIconPicker->StartShowing(fPriEditor, fSecEditor);
}

void
TIconEditor::AllAttached()
{
	AddParts();
	
	if( fPriZoomer ) fPriZoomer->Menu()->SetTargetForItems(BMessenger(this));
	if( fSecZoomer ) fSecZoomer->Menu()->SetTargetForItems(BMessenger(this));
	
	fColorPicker->SetTarget(this, NULL);
	fToolPicker->SetTarget(this, NULL);
	fIconPicker->SetTarget(this, NULL);
	
	if( !Window()->CurrentFocus() && fPriViewer ) {
		fPriViewer->MakeFocus();
	}
}

void
TIconEditor::DetachedFromWindow()
{
	if( fColorWindow.IsValid() ) {
		BMessage msg(B_QUIT_REQUESTED);
		fColorWindow.SendMessage(&msg);
	}
	
	if( fPriEditor ) {
		fPriEditor->DeSelect();
		Looper()->RemoveHandler(fPriEditor);
	}
	if( fSecEditor ) {
		fSecEditor->DeSelect();
		Looper()->RemoveHandler(fSecEditor);
	}
	
	BView::DetachedFromWindow();
}

void
TIconEditor::GetPreferredSize(float *width, float *height)
{
	if( width ) *width = fPrefWidth;
	if( height ) *height = fPrefHeight;
}

void TIconEditor::DeleteIcon(TCustomBitmapEditor** editor,
							 TFatBitsEditor** viewer,
							 BScrollView** scroller,
							 BMenuField** zoomer)
{
	if( fIconPicker ) fIconPicker->StartShowing(0, 0);
	
	if( *zoomer ) (*zoomer)->RemoveSelf();
	if( *viewer ) (*viewer)->RemoveSelf();
	if( *scroller ) (*scroller)->RemoveSelf();
	delete *zoomer;
	*zoomer = 0;
	delete *viewer;
	*viewer = 0;
	delete *scroller;
	*scroller = 0;
	delete *editor;
	*editor = 0;
}

struct zoom_menu_item { const char* name; float factor; };
static const zoom_menu_item zoom_menu_items[] = {
	{ "100%", 1.0 },
	{ "200%", 2.0 },
	{ "400%", 4.0 },
	{ "600%", 6.0 },
	{ "800%", 8.0 },
	{ "1200%", 12.0 },
	{ "1600%", 16.0 },
	{ NULL, 0 }
};

static BMenuField* BuildZoomMenu(int32 which, float init_factor)
{
	BMenu* menu = new BPopUpMenu("", true, false);
	const zoom_menu_item* z = zoom_menu_items;
	while( z && z->name ) {
		BMessage* msg = new BMessage(msg_set_zoom);
		msg->AddInt32("which", which);
		msg->AddFloat("factor", z->factor);
		BMenuItem* item = new BMenuItem(z->name, msg);
		if( z->factor == init_factor ) item->SetMarked(true);
		else item->SetMarked(false);
		menu->AddItem(item);
		z++;
	}
	
	BMenuField* f = new BMenuField(BRect(0, 0,
										 B_V_SCROLL_BAR_WIDTH,
										 B_H_SCROLL_BAR_HEIGHT),
								   "zoom", "", menu,
								   B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	f->SetDivider(0);
	return f;
}

void
TIconEditor::LayoutViews(float w, float h)
{
	(void)w;
	(void)h;
	
	BRect bounds(Bounds());
	
	if( fLayout == UNKNOWN_LAYOUT ) fLayout = BOTH_LAYOUT;
	
	float pw=0,ph=0;
	
	if( Window() ) Window()->BeginViewTransaction();
	
	if( !fToolPicker ) {
		fToolPicker = new TToolPicker(
			BPoint(0,0), msg_new_tool, B_HORIZONTAL, 2, fCurrentTool);
		fToolPicker->SetResizingMode(B_FOLLOW_NONE);
		AddChild(fToolPicker);
	}
	
	fPrefWidth = fPrefHeight = 0;
	fToolPicker->GetPreferredSize(&fPrefWidth, &fPrefHeight);
	const float toolRight = fPrefWidth;
	
	if( fLayout == PRIMARY_LAYOUT || fLayout == BOTH_LAYOUT ) {
		if( fPriEditor || fPriViewer || fPriScroller ) {
			if( !fPriEditor ) {
				DeleteIcon(&fPriEditor, &fPriViewer, &fPriScroller, &fPriZoomer);
			}
		}
		if( !fPriEditor ) {
			fPriEditor = new TCustomBitmapEditor(this, fPriWidth, fPriHeight,
												 fPriColorSpace, "large");
			fPriEditor->SetCurrentTool(fCurrentTool);
			fPriEditor->SetSelectionMode(fSelectionMode);
			fPriEditor->SetDitherColorConversions(fDitherColorConversions);
			fPriEditor->NewPenColor(fForeColor);
			fPriEditor->NewSecondaryColor(fBackColor);
			fPriEditor->SetBackgroundColor(fBackgroundColor);
		}
		fPriEditor->SetAttributes(fPriWidth, fPriHeight, fPriColorSpace);
		
		if( fPriEditor && !fPriEditor->Looper() && Window() ) {
			Window()->AddHandler(fPriEditor);
		}
		if( fPriEditor && !fPriViewer ) {
			fPriViewer = new TFatBitsEditor(*fPriEditor, fGridColor, 8);
			fPriViewer->SetResizingMode(B_FOLLOW_ALL);
		}
		if( fPriViewer && !fPriScroller ) {
			fPriScroller = new BScrollView("PrimaryScroll", fPriViewer,
										   B_FOLLOW_NONE, B_WILL_DRAW,
										   true, true, B_FANCY_BORDER);
			AddChild(fPriScroller);
		}
		if( fPriScroller && !fPriZoomer ) {
			fPriZoomer = BuildZoomMenu(0, fPriViewer->Zoom());
			fPriScroller->AddChild(fPriZoomer);
			fPriZoomer->MoveTo(fPriScroller->Bounds().right-B_V_SCROLL_BAR_WIDTH-2,
							   fPriScroller->Bounds().bottom-B_H_SCROLL_BAR_HEIGHT-1);
			fPriZoomer->ResizeTo(B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT-1);
			if( Window() ) {
				fPriZoomer->Menu()->SetTargetForItems(BMessenger(this));
			}
		}
		
		fPriScroller->MoveTo(fPrefWidth+10,0);
		fPriViewer->GetPreferredSize(&pw, &ph);
		pw += B_V_SCROLL_BAR_WIDTH+4;
		ph += B_H_SCROLL_BAR_HEIGHT+4;
		fPrefWidth += 10 + pw;
		if( ph > fPrefHeight ) fPrefHeight = ph;
	} else {
		DeleteIcon(&fPriEditor, &fPriViewer, &fPriScroller, &fPriZoomer);
		pw = ph = 0;
	}

	if( fLayout == SECONDARY_LAYOUT || fLayout == BOTH_LAYOUT ) {
		if( fSecEditor || fSecViewer || fSecScroller ) {
			if( !fSecEditor ) {
				DeleteIcon(&fSecEditor, &fSecViewer, &fSecScroller, &fSecZoomer);
			}
		}
		if( !fSecEditor ) {
			fSecEditor = new TCustomBitmapEditor(this, fSecWidth, fSecHeight,
												 fSecColorSpace, "mini");
			fSecEditor->SetCurrentTool(fCurrentTool);
			fSecEditor->SetSelectionMode(fSelectionMode);
			fSecEditor->SetDitherColorConversions(fDitherColorConversions);
			fSecEditor->NewPenColor(fForeColor);
			fSecEditor->NewSecondaryColor(fBackColor);
			fSecEditor->SetBackgroundColor(fBackgroundColor);
		}
		fSecEditor->SetAttributes(fSecWidth, fSecHeight, fSecColorSpace);
		if( fSecEditor && !fSecEditor->Looper() && Window() ) {
			Window()->AddHandler(fSecEditor);
		}
		if( fSecEditor && !fSecViewer ) {
			fSecViewer = new TFatBitsEditor(*fSecEditor, fGridColor, 8);
			fSecViewer->SetResizingMode(B_FOLLOW_ALL);
		}
		if( fSecViewer && !fSecScroller ) {
			fSecScroller = new BScrollView("SecondaryScroll", fSecViewer,
										   B_FOLLOW_NONE, B_WILL_DRAW,
										   true, true, B_FANCY_BORDER);
			AddChild(fSecScroller);
		}
		if( fSecScroller && !fSecZoomer ) {
			fSecZoomer = BuildZoomMenu(1, fSecViewer->Zoom());
			fSecScroller->AddChild(fSecZoomer);
			fSecZoomer->MoveTo(fSecScroller->Bounds().right-B_V_SCROLL_BAR_WIDTH-2,
							   fSecScroller->Bounds().bottom-B_H_SCROLL_BAR_HEIGHT-1);
			fSecZoomer->ResizeTo(B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT-1);
			if( Window() ) {
				fSecZoomer->Menu()->SetTargetForItems(BMessenger(this));
			}
		}
	} else {
		DeleteIcon(&fSecEditor, &fSecViewer, &fSecScroller, &fSecZoomer);
		pw = ph = 0;
	}
	
	if( fSecScroller ) {
		fSecViewer->GetPreferredSize(&pw, &ph);
		pw += B_V_SCROLL_BAR_WIDTH+4;
		ph += B_H_SCROLL_BAR_HEIGHT+4;
		if( ph > fPrefHeight ) fPrefHeight = ph;
	} else {
		pw = ph = 0;
	}
	
	if( !fIconPicker ) {
		fIconPicker = new TIconPicker(BRect(0,0,100,100));
		fIconPicker->SetResizingMode(B_FOLLOW_NONE);
		AddChild(fIconPicker);
	}
	fIconPicker->StartShowing(fPriEditor, fSecEditor);
	float iw=0,ih=0;
	fIconPicker->GetPreferredSize(&iw, &ih);
	
	if( !fColorPicker ) {
		fColorPicker = new TColorPicker(BRect(0, 0, 10, 10),
										fForeColor, fBackColor);
		fColorPicker->SetResizingMode(B_FOLLOW_NONE);
		AddChild(fColorPicker);
	}
	float cw=0, ch=0;
	fColorPicker->GetPreferredSize(&cw, &ch);
	// Add space for window resizing control.
	cw += B_V_SCROLL_BAR_WIDTH;
	
	float ctw=0, cth=0;
	if( fControls ) fControls->GetPreferredSize(&ctw, &cth);
	
	float right_size = pw > iw ? pw : iw;
	right_size = right_size > ctw ? right_size : ctw;
	
	if( Window() ) {
		if( fSecScroller && fIconPicker ) {
			fIconPicker->MoveTo(bounds.right-right_size-1,
								fSecScroller->Frame().bottom+10);
		} else if( fIconPicker ) {
			fIconPicker->MoveTo(bounds.right-right_size-1, 0);
		}
		if( fIconPicker ) {
			fIconPicker->ResizeTo(right_size,
								  bounds.bottom-ch-10-(cth>0?(cth+10):0)
								  - fIconPicker->Frame().top);
		}
		if( fControls ) {
			fControls->MoveTo(bounds.right-right_size+floor((right_size-ctw)/2)-1,
							  fIconPicker->Frame().bottom+10);
			fControls->ResizeTo(ctw, cth);
		}
	}
	
	if( Window() ) {
		if( fPriScroller ) {
			fPriScroller->ResizeTo(bounds.right-right_size-10-fPriScroller->Frame().left,
								   bounds.bottom-ch-10-fPriScroller->Frame().top);
		}
		if( fSecScroller ) {
			fSecScroller->MoveTo(bounds.right-right_size-1, 0);
		}
	}
	
	fPrefWidth += right_size + 10;
	if( fControls ) cth += 10;
	if( (ph+10+ih+cth) > fPrefHeight ) fPrefHeight = ph+10+ih+cth;
	
	float prefix = fColorPicker->PrefixWidth();
	if( Window() ) fColorPicker->MoveTo(toolRight-prefix+10, bounds.bottom-ch-1);
	
	if( (cw-prefix+toolRight+10) > fPrefWidth ) fPrefWidth = cw-prefix+toolRight+10;
	fPrefHeight += ch+10;
	
	if( Window() ) Window()->EndViewTransaction();
	
	// If this view has focus, pass it down to one of the bitmap views.
	if( IsFocus() ) MakeFocus(true);
}

void
TIconEditor::FrameResized(float w, float h)
{
	LayoutViews(w, h);
}

void 
TIconEditor::MakeFocus(bool on)
{
	if( on && fPriViewer ) fPriViewer->MakeFocus(on);
	else if( on && fSecViewer ) fSecViewer->MakeFocus(on);
	else if( on != IsFocus() ) BView::MakeFocus(on);
}


void
TIconEditor::AddParts()
{
	LayoutViews(Bounds().Width(), Bounds().Height());
}

status_t
TIconEditor::GetConfiguration(BMessage* into) const
{
	status_t err = into->AddInt32("be:tool", fCurrentTool);
	if( err == B_OK ) err = into->AddInt32("be:selmode", (int32)fSelectionMode);
	if( err == B_OK ) err = into->AddBool("be:dither", (int32)fDitherColorConversions);
	if( err == B_OK ) err = into->AddData("be:pricolor", B_RGB_COLOR_TYPE,
											&fForeColor, sizeof(rgb_color));
	if( err == B_OK ) err = into->AddData("be:seccolor", B_RGB_COLOR_TYPE,
											&fBackColor, sizeof(rgb_color));
	if( err == B_OK ) err = into->AddData("be:gridcolor", B_RGB_COLOR_TYPE,
											&fGridColor, sizeof(rgb_color));
	if( err == B_OK ) err = into->AddData("be:bgcolor", B_RGB_COLOR_TYPE,
											&fBackgroundColor, sizeof(rgb_color));
	if( err == B_OK ) err = into->AddPoint("be:color_window_offsets",
										   fColorWindowOffsets);
	return err;
}

status_t
TIconEditor::SetConfiguration(const BMessage* from)
{
	int32 num;
	const rgb_color* col;
	ssize_t size;
	
	status_t err = from->FindInt32("be:tool", &num);
	if( err == B_OK ) {
		fCurrentTool = num;
		if( fSecEditor ) fSecEditor->SetCurrentTool(fCurrentTool);
		if( fPriEditor ) fPriEditor->SetCurrentTool(fCurrentTool);
		if( fToolPicker ) fToolPicker->ChangeSelection(fCurrentTool);
	}
	
	err = from->FindInt32("be:selmode", &num);
	if( err == B_OK ) {
		SetSelectionMode((paste_selection_mode)num);
	}
	
	err = from->FindData("be:pricolor", B_RGB_COLOR_TYPE, (const void**)&col, &size);
	if( err == B_OK && col && size == sizeof(rgb_color) ) {
		fForeColor = *col;
		if( fSecEditor ) fSecEditor->NewPenColor(fForeColor);
		if( fPriEditor ) fPriEditor->NewPenColor(fForeColor);
	}
	
	err = from->FindData("be:seccolor", B_RGB_COLOR_TYPE, (const void**)&col, &size);
	if( err == B_OK && col && size == sizeof(rgb_color) ) {
		fBackColor = *col;
		if( fSecEditor ) fSecEditor->NewSecondaryColor(fBackColor);
		if( fPriEditor ) fPriEditor->NewSecondaryColor(fBackColor);
	}
	
	err = from->FindData("be:gridcolor", B_RGB_COLOR_TYPE, (const void**)&col, &size);
	if( err == B_OK && col && size == sizeof(rgb_color) ) {
		fGridColor = *col;
		if( fSecViewer ) fSecViewer->SetGridColor(fGridColor);
		if( fPriViewer ) fPriViewer->SetGridColor(fGridColor);
	}
						
	err = from->FindData("be:bgcolor", B_RGB_COLOR_TYPE, (const void**)&col, &size);
	if( err == B_OK && col && size == sizeof(rgb_color) ) {
		fBackgroundColor = *col;
		if( fSecEditor ) fSecEditor->SetBackgroundColor(fBackgroundColor);
		if( fPriEditor ) fPriEditor->SetBackgroundColor(fBackgroundColor);
	}
	
	BPoint point;
	err = from->FindPoint("be:color_window_off", &point);
	if( err == B_OK ) {
		fColorWindowOffsets = point;
	}
	
	return B_OK;
}

void
TIconEditor::GetPrefs()
{
	fCurrentTool = kPencilTool;
	fSelectionMode = kPasteCopy;
	fForeColor = kBlack;
	fBackColor = B_TRANSPARENT_32_BIT;
	fGridColor = kGridGray;
	fBackgroundColor = DesktopColor();
	fColorWindowOffsets.x = 10; fColorWindowOffsets.y = -5;
	fDitherColorConversions = true;
	
	BPath path;
	int32 mode = 0;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
				
		path.Append (kIconEditorPrefsfileName);
		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
		
			// skip the window loc
			lseek (ref, sizeof(BPoint), 0);
			
			bool blah;
			// Used to be currently selected icon.
			if ( read(ref, &blah, sizeof(bool)) != sizeof(bool)) {
				goto BAIL;
			}
			
			if ( read(ref, &fCurrentTool, sizeof(int32)) != sizeof(int32)) {
				fCurrentTool = kPencilTool;
				goto BAIL;
			}
			
			if ( read(ref, &fForeColor, sizeof(rgb_color)) != sizeof(rgb_color)) {
				fForeColor = kBlack;
				goto BAIL;
			}
			
			if ( read(ref, &fBackColor, sizeof(rgb_color)) != sizeof(rgb_color)) {
				fBackColor = B_TRANSPARENT_32_BIT;
				goto BAIL;
			}
			
			if ( read(ref, &fGridColor, sizeof(rgb_color)) != sizeof(rgb_color)) {
				fGridColor = kGridGray;
				goto BAIL;
			}
			
			if ( read(ref, &fBackgroundColor, sizeof(rgb_color)) != sizeof(rgb_color)) {
				fBackgroundColor = DesktopColor();
				goto BAIL;
			}

			if ( read(ref, &fColorWindowOffsets, sizeof(BPoint)) != sizeof(BPoint)) {
				fColorWindowOffsets.x = 10;
				fColorWindowOffsets.y = -5;
				goto BAIL;
			}
			
			if ( read(ref, &mode, sizeof(int32)) != sizeof(int32)) {
				goto BAIL;
			}
			SetSelectionMode((paste_selection_mode)mode);
			
			if ( read(ref, &mode, sizeof(int32)) != sizeof(int32)) {
				goto BAIL;
			}
			SetDitherColorConversions(mode&1);
			
BAIL:			
			close(ref);
		
			return;	
		}
	}	
}

void
TIconEditor::SetPrefs()
{
	BPath path;

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		
		path.Append (kIconEditorPrefsfileName);
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {

			// skip the window loc
			lseek (ref, sizeof(BPoint), 0);
			
			bool blah = true;		// used to be currently selected icon.
			write (ref, &blah, sizeof(bool));
			write (ref, &fCurrentTool, sizeof(int32));
			write (ref, &fForeColor, sizeof(rgb_color));
			write (ref, &fBackColor, sizeof(rgb_color));
			write (ref, &fGridColor, sizeof(rgb_color));
			write (ref, &fBackgroundColor, sizeof(rgb_color));
			write (ref, &fColorWindowOffsets, sizeof(BPoint));
			int32 mode = fSelectionMode;
			write (ref, &mode, sizeof(int32));
			mode = fDitherColorConversions ? 1 : 0;
			write (ref, &mode, sizeof(int32));
			
			close(ref);
			
		}
	}
}

void 
TIconEditor::Draw(BRect updateRect)
{
	inherited::Draw(updateRect);
}

void
TIconEditor::KeyDown(const char *key, int32 numBytes)
{
	inherited::KeyDown(key,numBytes);
}

void
TIconEditor::NewSecondaryImage(const BBitmap* icon, BRect srcRect, bool report)
{
	if( fSecEditor ) {
		fSecEditor->SetBitmap(icon, srcRect, report);
	}
}

void
TIconEditor::NewPrimaryImage(const BBitmap* icon, BRect srcRect, bool report)
{
	if( fPriEditor ) {
		fPriEditor->SetBitmap(icon, srcRect, report);
	}
}

void
TIconEditor::SetBitmap(const BBitmap* primary, const BBitmap* secondary)
{
	if( primary && secondary ) fLayout = BOTH_LAYOUT;
	else if( primary ) fLayout = PRIMARY_LAYOUT;
	else if( secondary ) fLayout = SECONDARY_LAYOUT;
	
	if( primary ) {
		fPriWidth = floor(primary->Bounds().Width()+1.5);
		fPriHeight = floor(primary->Bounds().Height()+1.5);
		fPriColorSpace = primary->ColorSpace();
	}
	if( secondary ) {
		fSecWidth = floor(secondary->Bounds().Width()+1.5);
		fSecHeight = floor(secondary->Bounds().Height()+1.5);
		fSecColorSpace = secondary->ColorSpace();
	}
	
	AddParts();
	
	if( primary ) NewPrimaryImage(primary, primary->Bounds(), false);
	if( secondary ) NewSecondaryImage(secondary, secondary->Bounds(), false);
}

void
TIconEditor::SetPrimaryAttributes(float width, float height,
								  color_space cspace)
{
	if( width > 0 ) fPriWidth = floor(width+.5);
	if( height > 0 ) fPriHeight = floor(height+.5);
	if( cspace != B_NO_COLOR_SPACE ) fPriColorSpace = cspace;
	AddParts();
}

void
TIconEditor::SetSecondaryAttributes(float width, float height,
									color_space cspace)
{
	if( width > 0 ) fSecWidth = floor(width+.5);
	if( height > 0 ) fSecHeight = floor(height+.5);
	if( cspace != B_NO_COLOR_SPACE ) fSecColorSpace = cspace;
	AddParts();
}

void
TIconEditor::SetPrimaryHotSpot(int32 x, int32 y)
{
	if( PrimaryBitmapEditor() ) PrimaryBitmapEditor()->SetHotSpot(x, y);
}

int32
TIconEditor::PrimaryHotSpotX() const
{
	if( PrimaryBitmapEditor() ) return PrimaryBitmapEditor()->HotSpotX();
	return -1;
}

int32
TIconEditor::PrimaryHotSpotY() const
{
	if( PrimaryBitmapEditor() ) return PrimaryBitmapEditor()->HotSpotY();
	return -1;
}

void
TIconEditor::SetSecondaryHotSpot(int32 x, int32 y)
{
	if( SecondaryBitmapEditor() ) SecondaryBitmapEditor()->SetHotSpot(x, y);
}

int32
TIconEditor::SecondaryHotSpotX() const
{
	if( SecondaryBitmapEditor() ) return SecondaryBitmapEditor()->HotSpotX();
	return -1;
}

int32
TIconEditor::SecondaryHotSpotY() const
{
	if( SecondaryBitmapEditor() ) return SecondaryBitmapEditor()->HotSpotY();
	return -1;
}

status_t
TIconEditor::FindUpdateIcon(BMessage* msg, bool large)
{
	// first figure out which editor this bitmap is being
	// placed in.
	TBitmapEditor* target = large ? fPriEditor : fSecEditor;
	
	// if no target, just skip it.
	if (!target) return B_OK;
	
	BBitmap* icon = 0;
	status_t err = B_OK;

	// now figure out the source icon; first check if it was a
	// drag from an icon well.
	const char *whichIcon;
	if (msg->FindString("icon", &whichIcon) == B_OK) {
		BMessage currMsg;
		
		if (strcmp(whichIcon, "large") == 0) {
			if (large) {
				// if the source is also the large icon, just skip it
				return B_OK;
			}
			err = msg->FindMessage(kLargeIconMimeType, &currMsg);
		} else if (strcmp(whichIcon, "mini") == 0 && large) {
			if (!large) {
				// if the source is also the mini icon, just skip it
				return B_OK;
			}
			err = msg->FindMessage(kMiniIconMimeType, &currMsg);
		} else err = B_ERROR;
		
		if (err == B_OK) icon = (BBitmap*) BBitmap::Instantiate(&currMsg);
	}
	
	if (icon == 0) {
		// not a generic icon; try to interpret as a file.
		
		entry_ref ent;
		err = msg->FindRef("refs", &ent);
		if (err != B_OK && msg->what == B_REFS_RECEIVED) {
			(new BAlert("", "No file found in message.", "Stop"))->Go(0);
			return err;
		}
		BFile input;
		if (err == B_OK) {
			if ((err = input.SetTo(&ent, O_RDONLY)) < B_OK) {
				char str[350];
				sprintf(str, "Cannot open '%s': %s.", ent.name, strerror(err));
				(new BAlert("", str, "Stop"))->Go(0);
				return err;
			}
		}
		if (err == B_OK) icon = BTranslationUtils::GetBitmap(&input);
		
		// If no translator was able to handle it, check to see if this is
		// a flattened BBitmap archive.
		if (!icon) {
			printf("Trying to interpret as a BBitmap archive.\n");
			BMessage archive;
			input.Seek(0, SEEK_SET);
			err = archive.Unflatten(&input);
			if (err == B_OK) {
				printf("Unflattened; now instantiating.\n");
				BArchivable *a = instantiate_object(&archive);
				if((icon = dynamic_cast<BBitmap *>(a)) == 0)
					delete a;
			} else {
				printf("Error unflattening: %s\n", strerror(err));
			}
		}
		
		if (input.InitCheck() == B_OK && !icon) {
			char str[350];
			sprintf(str, "The file '%s' is not a recognized bitmap.", ent.name);
			(new BAlert("", str, "Stop"))->Go(0);
			return B_ERROR;
		}
	}
	
	if (icon == 0) {
		(new BAlert("", "The data is not a recognized bitmap.", "Stop"))->Go(0);
		return B_ERROR;
	}
	
	if (icon) {
		/* scale bitmap if it's too big */
		if (icon->Bounds().Width() >= target->Width() ||
				icon->Bounds().Height() >= target->Height()) {
			float scaling_x = (target->Width()-1)/icon->Bounds().Width();
			float scaling_y = (target->Height()-1)/icon->Bounds().Height();
			float scaling = (scaling_x < scaling_y) ? scaling_x : scaling_y;
			BRect r(icon->Bounds());
			r.right = floor(r.right*scaling+.5);
			r.bottom = floor(r.bottom*scaling+.5);
			BBitmap * scaled = new BBitmap(r, B_BITMAP_CLEAR_TO_WHITE,
										   icon->ColorSpace());
			err = scale_bitmap(scaled, icon);
			if (err != B_OK) {
				delete scaled;
				delete icon;
				(new BAlert("", "Unable to scale bitmap.", "Stop"))->Go(0);
				return err;
			}
			
			delete icon;
			icon = scaled;
		}
		
		target->BeginPasteSelection(icon, icon->Bounds(), BPoint(0, 0));
	}
	
	delete icon;
	
	return err;
}

void
TIconEditor::HandleIconDrop(BMessage *msg)
{
	// figure out which editor this message is being dropped in.
	const char *whichIcon;
	if (msg->FindString("drop", &whichIcon) == B_OK) {
		if (strcmp("large", whichIcon) == 0) {
			FindUpdateIcon(msg, true);
			return;
		}
		if (strcmp("mini", whichIcon) == 0 ) {
			FindUpdateIcon(msg, false);
			return;
		}
	}
	
	// otherwise, try placing it in both icons.
	if( fPriEditor ) if (FindUpdateIcon(msg, true) != B_OK) return;
	if( fSecEditor ) FindUpdateIcon(msg, false);
}

void
TIconEditor::DumpIcons() const
{
	BMallocIO io;
	if( fPriEditor && fSecEditor ) {
		if( fPriEditor ) fPriEditor->DumpEntireMap(io, "PrimaryImage");
		if( fSecEditor ) fSecEditor->DumpEntireMap(io, "SecondaryImage");
	} else {
		if( fPriEditor ) fPriEditor->DumpEntireMap(io, "Image");
		if( fSecEditor ) fSecEditor->DumpEntireMap(io, "Image");
	}
	
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	BMessage *message = be_clipboard->Data();
	if (!message) {
		printf("no clip msg\n");
		return;
	}
	
	message->AddData("text/plain", B_MIME_TYPE,
					 io.Buffer(), io.BufferLength());

	be_clipboard->Commit();
	be_clipboard->Unlock();
}

void
TIconEditor::DumpCursor() const
{
	BMallocIO io;
	if( fPriEditor ) fPriEditor->DumpCursor(io, "Cursor");
	
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	BMessage *message = be_clipboard->Data();
	if (!message) {
		printf("no clip msg\n");
		return;
	}
	
	message->AddData("text/plain", B_MIME_TYPE,
					 io.Buffer(), io.BufferLength());

	be_clipboard->Commit();
	be_clipboard->Unlock();
}

void 
TIconEditor::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		HandleIconDrop(msg);
		return;
	}
	
	switch (msg->what) {
		case kDumpSelection:
			#if 0
			if (fIconSelection) 
				fPriEditor->DumpSelection();
			else
				fSecEditor->DumpSelection();
			#endif
			break;
		case kDumpIcons:
			DumpIcons();
#if 0
			BMessage* msg = new BMessage('test');
			fSavePanel = new BFilePanel(B_SAVE_PANEL,new BMessenger(this),NULL,B_FILE_NODE,msg);
			fSavePanel->Show();
#endif
			break;
		
		case kDumpCursor:
			DumpCursor();
			break;
		
		case msg_set_zoom: {
			float factor=0;
			int32 which;
			if( msg->FindFloat("factor", &factor) == B_OK &&
					msg->FindInt32("which", &which) == B_OK ) {
				if( which == 0 && fPriViewer ) fPriViewer->SetZoom(factor);
				if( which == 1 && fSecViewer ) fSecViewer->SetZoom(factor);
			}
		} break;
		
		case msg_set_bg_color:
			// show a color picker window
			ChooseColorWindow();
			break;
		case msg_save_colorwind_loc: {
			BPoint point;
			if( msg->FindPoint("loc", &point) == B_OK ) {
				BRect frame;
				if( Parent() ) frame = Parent()->ConvertToScreen(Frame());
				else frame = Window()->ConvertToScreen(Frame());
				fColorWindowOffsets.x = point.x - frame.left;
				fColorWindowOffsets.y = point.y - frame.top;
			}
		} break;
		case msg_new_bg_color:
			{
				int8 red, green, blue;
				fBackgroundColor.alpha = 255;
				
				msg->FindInt8("red", &red);
				msg->FindInt8("green", &green);
				msg->FindInt8("blue", &blue);
				
				fBackgroundColor.red = red;
				fBackgroundColor.green = green;
				fBackgroundColor.blue = blue;

				if( fPriEditor ) fPriEditor->SetBackgroundColor(fBackgroundColor);
				if( fSecEditor ) fSecEditor->SetBackgroundColor(fBackgroundColor);
			}
			break;
			
		case msg_selection_tool:
			fToolPicker->ChangeSelection(kSelectionTool);
			break;
		case msg_eraser_tool:
			fToolPicker->ChangeSelection(kEraserTool);
			break;
		case msg_pencil_tool:
			fToolPicker->ChangeSelection(kPencilTool);
			break;
		case msg_eye_tool:
			fToolPicker->ChangeSelection(kEyeDropperTool);
			break;
		case msg_fill_tool:
			fToolPicker->ChangeSelection(kBucketTool);
			break;
		case msg_line_tool:
			fToolPicker->ChangeSelection(kLineTool);
			break;
		case msg_rect_tool:
			fToolPicker->ChangeSelection(kRectTool);
			break;
		case msg_frect_tool:
			fToolPicker->ChangeSelection(kFilledRectTool);
			break;
		case msg_rrect_tool:
			fToolPicker->ChangeSelection(kRoundRectTool);
			break;
		case msg_frrect_tool:
			fToolPicker->ChangeSelection(kFilledRoundRectTool);
			break;
		case msg_oval_tool:
			fToolPicker->ChangeSelection(kOvalTool);
			break;
		case msg_foval_tool:
			fToolPicker->ChangeSelection(kFilledOvalTool);
			break;
		case msg_hotspot_tool:
			fToolPicker->ChangeSelection(kHotSpotTool);
			break;
		case msg_next_tool:
			fToolPicker->NextTool();
			break;
		case msg_prev_tool:
			fToolPicker->PrevTool();
			break;
		
		case kSetSelectionMode: {
			int32 n;
			if (msg->FindInt32("mode", &n) == B_OK) {
				SetSelectionMode((paste_selection_mode)n);
			}
		} break;
		
		case kToggleDithering:
			SetDitherColorConversions(!DitherColorConversions());
			break;
			
		case msg_fore_color: {
			rgb_color color;
			if (msg->FindInt32("colorindex", (int32*)&color) == B_NO_ERROR) {
				if( fSecEditor ) fSecEditor->NewPenColor(color);
				if( fPriEditor ) fPriEditor->NewPenColor(color);
				fForeColor = color;
			}
		} break;
		case msg_back_color: {
			rgb_color color;
			if (msg->FindInt32("colorindex", (int32*)&color) == B_NO_ERROR) {
				if( fSecEditor ) fSecEditor->NewSecondaryColor(color);
				if( fPriEditor ) fPriEditor->NewSecondaryColor(color);
				fBackColor = color;
			}
		} break;
		
		case msg_new_tool:
			if (msg->FindInt32("tool",&fCurrentTool) == B_OK) {
				if( fPriEditor ) fPriEditor->SetCurrentTool(fCurrentTool);
				if( fSecEditor ) fSecEditor->SetCurrentTool(fCurrentTool);
			}
			break;
			
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void 
TIconEditor::MouseDown(BPoint where)
{
	inherited::MouseDown(where);
}

void
TIconEditor::MouseMoved(BPoint where, uint32 transit, const BMessage* drop)
{
	inherited::MouseMoved(where, transit, drop);
}

void
TIconEditor::Pulse()
{
}

void
TIconEditor::SetSelectionMode(paste_selection_mode mode)
{
	fSelectionMode = mode;
	if( fSecEditor ) fSecEditor->SetSelectionMode(mode);
	if( fPriEditor ) fPriEditor->SetSelectionMode(mode);
}

paste_selection_mode
TIconEditor::SelectionMode() const
{
	return fSelectionMode;
}

void
TIconEditor::SetDitherColorConversions(bool state)
{
	fDitherColorConversions = state;
	if( fSecEditor ) fSecEditor->SetDitherColorConversions(state);
	if( fPriEditor ) fPriEditor->SetDitherColorConversions(state);
}

bool
TIconEditor::DitherColorConversions() const
{
	return fDitherColorConversions;
}

// sent from eye dropper with actual index for bitmap
void
TIconEditor::DropperColorSelection(rgb_color color, bool primary)
{
	if (primary) {
	
		fForeColor = color;
		if( fSecEditor ) fSecEditor->NewPenColor(color);
		if( fPriEditor ) fPriEditor->NewPenColor(color);
		if( fColorPicker ) fColorPicker->SetForeColor(color);
			
	} else {
	
		fBackColor = color;
		if( fSecEditor ) fSecEditor->NewSecondaryColor(color);
		if( fPriEditor ) fPriEditor->NewSecondaryColor(color);
		if( fColorPicker ) fColorPicker->SetBackColor(color);
			
	}
}

const BBitmap*
TIconEditor::PrimaryImage()
{
	//	ask for a composited image of the icon
	return fPriEditor ? fPriEditor->RealBitmap() : 0;
}

const BBitmap*
TIconEditor::SecondaryImage()
{
	//	ask for a composited image of the icon
	return fSecEditor ? fSecEditor->RealBitmap() : 0;
}

const TBitmapEditor*
TIconEditor::PrimaryBitmapEditor() const
{
	return fPriEditor;
}

TBitmapEditor*
TIconEditor::PrimaryBitmapEditor()
{
	return fPriEditor;
}

const TBitmapEditor*
TIconEditor::SecondaryBitmapEditor() const
{
	return fSecEditor;
}

TBitmapEditor*
TIconEditor::SecondaryBitmapEditor()
{
	return fSecEditor;
}

const int32 msg_activate = 'actv';

void
TIconEditor::ChooseColorWindow()
{
	BRect frame;
	if( Parent() ) frame = Parent()->ConvertToScreen(Frame());
	else frame = Window()->ConvertToScreen(Frame());
	
	BPoint loc;
	loc.x = frame.left + fColorWindowOffsets.x;
	loc.y = frame.top + fColorWindowOffsets.y;
	
	rgb_color bgColor;
	bgColor = fPriEditor
			? fPriEditor->BackgroundColor()
			: fSecEditor->BackgroundColor();
	
	if( !fColorWindow.IsValid() ) {
		BWindow* win = new TColorWindow(loc, this, bgColor);
		fColorWindow = BMessenger(win);
	} else {
		BMessage msg(msg_activate);
		fColorWindow.SendMessage(&msg);
	}
}

// **************************************************************************

const int32 msg_okay = 'okay';
const int32 msg_cancel = 'canc';
const int32 msg_desktop_color = 'desk';

TColorWindow::TColorWindow(BPoint loc, BView* target,
	rgb_color initialColor)
	:	BWindow(BRect(loc.x, loc.y, loc.x+1, loc.y+1), "color",
		B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE,
		B_CURRENT_WORKSPACE)
{
	AddToSubset(target->Window());
	fTarget = BMessenger(target);
	fColor = fOriginalColor = initialColor;
	
	fCC = new TColorControl(	BPoint(10, 10), B_CELLS_32x8, 4,
		"" , new BMessage(msg_new_bg_color));
	fCC->SetModificationMessage(new BMessage(msg_new_bg_color));
	fCC->SetTarget(this);
	
	float h, w;
	fCC->GetPreferredSize(&w, &h);
	
	BScreen s(this);
	BRect wframe(loc, BPoint(loc.x+20+w, loc.y+20+h+40));
	BRect sframe(s.Frame());
	if( wframe.left < sframe.left ) {
		wframe.right += sframe.left-wframe.left;
		wframe.left = sframe.left;
	}
	if( wframe.top < sframe.top ) {
		wframe.bottom += sframe.top-wframe.top;
		wframe.top = sframe.top;
	}
	if( wframe.right > sframe.right ) {
		wframe.left -= wframe.right-sframe.right;
		wframe.right = sframe.right;
	}
	if( wframe.bottom > sframe.bottom ) {
		wframe.top -= wframe.bottom-sframe.bottom;
		wframe.bottom = sframe.bottom;
	}
	
	MoveTo(wframe.left, wframe.top);
	ResizeTo(wframe.Width(), wframe.Height());
	
	BRect brect(Bounds());
	brect.right++; brect.bottom++;
	BBox* box = new BBox(brect, "", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	AddChild(box);
	box->AddChild(fCC);
	fCC->SetValue((rgb_color) initialColor);
	
	BRect r(Bounds().Width() - 10 - (box->StringWidth("Cancel") + 20),
			Bounds().Height() - 10 - 26,
			Bounds().Width() - 10,
			Bounds().Height() - 10);
	BButton* okay = new BButton(r, "okay", "OK", new BMessage(msg_okay),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	box->AddChild(okay);

	r.right = r.left - 10;
	r.left = r.right - (box->StringWidth("Cancel") + 20);
	
	BButton* cancel = new BButton(r, "cancel", "Cancel",
		new BMessage(msg_cancel), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	box->AddChild(cancel);
	
	r.left = 10;
	r.right = r.left + box->StringWidth("Desktop Color") + 20;
	BButton* desktopBtn = new BButton(r, "desktop", "Desktop Color",
		new BMessage(msg_desktop_color), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	box->AddChild(desktopBtn);
	
	SetDefaultButton(okay);
	
	Show();
}


TColorWindow::~TColorWindow()
{
}

void 
TColorWindow::MessageReceived(BMessage *m)
{
	switch (m->what) {
		case msg_activate:
			Activate();
			break;
		
		case msg_new_bg_color:
			{
				fColor = fCC->ValueAsColor();
				m->AddInt8("red", fColor.red);
				m->AddInt8("green", fColor.green);
				m->AddInt8("blue", fColor.blue);
				fTarget.SendMessage(m);
			}
			break;
			
		case msg_okay:
			{
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		case msg_cancel:
			// cancel the change, set to original color
			{
				BMessage m(msg_new_bg_color);
				m.AddInt8("red", fOriginalColor.red);
				m.AddInt8("green", fOriginalColor.green);
				m.AddInt8("blue", fOriginalColor.blue);
				fTarget.SendMessage(&m);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		case msg_desktop_color:
			{
				fColor = DesktopColor();
				fCC->SetValue((rgb_color)fColor);
				BMessage m(msg_new_bg_color);
				m.AddInt8("red", fColor.red);
				m.AddInt8("green", fColor.green);
				m.AddInt8("blue", fColor.blue);
				fTarget.SendMessage(&m);
			}
			break;
			
		default:
			BWindow::MessageReceived(m);
	}
}

bool 
TColorWindow::QuitRequested()
{
	ReportPosition();
	return true;
}

void
TColorWindow::ReportPosition()
{
	BPoint loc(Frame().LeftTop());
	BMessage msg(msg_save_colorwind_loc);
	msg.AddPoint("loc", loc);
	fTarget.SendMessage(&msg);
}
