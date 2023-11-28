
#include <Debug.h>

#include "BookmarkFile.h"

#include "BookmarkList.h"

#include "util.h"

#include <MessageFilter.h>

#include <Button.h>
#include <Bitmap.h>
#include <Font.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PictureButton.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include <Autolock.h>
#include <Debug.h>
#include <Locker.h>
#include <String.h>

#include <TranslatorFormats.h>

#include <experimental/ResourceSet.h>
#include <experimental/BitmapButton.h>

// Yuck, need to include this for get_setting().
#include <ResourceCache.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

float GetMenuFieldSize(BMenuField* field, float* width, float* height,
						bool set_divider = true);

static const BRect bogusRect(0, -400, 50, -350);

enum {
	BOOKMARKLIST_DRAG_MSG		= 'bmdg',
	BOOKMARKLIST_STOP_EDIT_MSG	= 'bmse'
};

// -------------------- Duplicated from BitmapTools.cpp --------------------

static rgb_color mix_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	rgb_color ret;
	ret.red = (uint8)( ( ((uint16)color1.red)*(255-amount)
						+ ((uint16)color2.red)*(amount)
						) / 255 );
	ret.green = (uint8)( ( ((uint16)color1.green)*(255-amount)
						+ ((uint16)color2.green)*(amount)
						) / 255 );
	ret.blue = (uint8)( ( ((uint16)color1.blue)*(255-amount)
						+ ((uint16)color2.blue)*(amount)
						) / 255 );
	ret.alpha = (uint8)( ( ((uint16)color1.alpha)*(255-amount)
						+ ((uint16)color2.alpha)*(amount)
						) / 255 );
	return ret;
}

// -------------------- disable_color --------------------

static inline rgb_color
disable_color(rgb_color color, rgb_color background)
{
	return mix_color(color, background, 128);
}

// ----------------------- bookmark_atts -----------------------

static rgb_color kBCBackground	= { 0x00, 0x00, 0x00, 0xff };
static rgb_color kBCFolder		= { 0x20, 0x40, 0x60, 0xff };
static rgb_color kBCOpenFolder	= { 0x60, 0x40, 0x40, 0xff };
static rgb_color kBCBookmark	= { 0x40, 0x60, 0x80, 0xff };
static rgb_color kBCSelected	= { 0x90, 0x70, 0x90, 0xff };
static rgb_color kBCActive		= { 0xF0, 0x90, 0x20, 0xff };
static rgb_color kBCText		= { 0xff, 0xff, 0xff, 0xff };

bookmark_attrs::bookmark_attrs(const BMessage& attrs)
	: fBackgroundColor(kBCBackground),
	  fFolderColor(kBCFolder),
	  fOpenFolderColor(kBCOpenFolder),
	  fBookmarkColor(kBCBookmark),
	  fSelectedColor(kBCSelected),
	  fActiveColor(kBCActive),
	  fTextColor(kBCText),
	  fUnderlineLinks(false),
	  fUnderlineHeight(1),
	  fUnderlineOffset(0),
	  fListFont(*be_plain_font),
	  fBackground(0),
	  fInfoBitmap(0), fInfoOutsideBitmap(0), fInfoOverBitmap(0),
	  	fInfoOnBitmap(0), fInfoOnOverBitmap(0),
	  fDeleteBitmap(0), fDeleteOutsideBitmap(0), fDeleteOverBitmap(0),
	  fFolderBitmap(0), fFolderClosedBitmap(0), fBookmarkBitmap(0)
{
	rgb_color color;
	const char* str;
	
	PRINT(("Initializing attributes from:")); DEBUG_ONLY(attrs.PrintToStream());
	
	if( attrs.FindInt32("listcolor", (int32*)&color) == B_OK ) {
		fBackgroundColor = color;
	}
	if( attrs.FindInt32("foldercolor", (int32*)&color) == B_OK ) {
		fFolderColor = color;
	}
	if( attrs.FindInt32("openfoldercolor", (int32*)&color) == B_OK ) {
		fOpenFolderColor = color;
	}
	if( attrs.FindInt32("bookmarkcolor", (int32*)&color) == B_OK ) {
		fBookmarkColor = color;
	}
	if( attrs.FindInt32("selectedcolor", (int32*)&color) == B_OK ) {
		fSelectedColor = color;
	}
	if( attrs.FindInt32("activecolor", (int32*)&color) == B_OK ) {
		fActiveColor = color;
	}
	if( attrs.FindInt32("text", (int32*)&color) == B_OK ) {
		fTextColor = color;
	}
	
	if( attrs.FindString("underlinelinks", &str) == B_OK ) {
		if (strcasecmp(str, "true") == 0) {
			fUnderlineLinks = true;
		} else if (strcasecmp(str, "false") == 0) {
			fUnderlineLinks = false;
		} else {
			fUnderlineLinks = atoi(str) != 0;
		}
	}
	
	if( attrs.FindString("underlineheight", &str) == B_OK ) {
		fUnderlineHeight = atoi(str);
		if (fUnderlineHeight < 1) fUnderlineHeight = 1;
	}
	
	if( attrs.FindString("underlineoffset", &str) == B_OK ) {
		fUnderlineOffset = atoi(str);
	}
	
	if( attrs.FindString("listfont", &str) == B_OK ) {
		if( decode_font(str, &fListFont) != B_OK ) {
			fListFont = *be_plain_font;
		}
	}
	
	fBackground = bitmap_from_resource(attrs, "listbackground", "listbackground.png");
	
	fInfoBitmap = bitmap_from_resource(attrs, "infoimg", "info.png");
	fInfoOutsideBitmap = bitmap_from_resource(attrs, "infooutsideimg",
											  "info_outside.png", fInfoBitmap);
	fInfoOverBitmap = bitmap_from_resource(attrs, "infooverimg",
											  "info_over.png", fInfoBitmap);
	fInfoOnBitmap = bitmap_from_resource(attrs, "infoonimg",
											  "info_on.png", fInfoBitmap);
	fInfoOnOverBitmap = bitmap_from_resource(attrs, "infoonoverimg",
											  "info_onover.png", fInfoOnBitmap);
	
	fDeleteBitmap = bitmap_from_resource(attrs, "deleteimg", "delete.png");
	fDeleteOutsideBitmap = bitmap_from_resource(attrs, "deleteoutsideimg",
											  "delete_outside.png", fDeleteBitmap);
	fDeleteOverBitmap = bitmap_from_resource(attrs, "deleteoverimg",
											  "delete_over.png", fDeleteBitmap);
	
	fFolderBitmap = bitmap_from_resource(attrs, "folderimg", "folder.png");
	fFolderClosedBitmap = bitmap_from_resource(attrs, "folderclosedimg",
											   "folder_closed.png", fFolderBitmap);
	fBookmarkBitmap = bitmap_from_resource(attrs, "bookmarkimg", "bookmark.png");
}

bookmark_attrs::~bookmark_attrs()
{
}

const BBitmap* bookmark_attrs::bitmap_from_resource(const BMessage& attrs,
													const char* param,
													const char* def_name,
													const BBitmap* def_bitmap)
{
	const char* str;
	if( attrs.FindString(param, &str) != B_OK ) {
		str = def_name;
	}
	const BBitmap* bm = ResourceBitmap(str);
	PRINT(("Reading picture %s, bitmap is %p\n", str, bm));
	return bm ? bm : def_bitmap;
}

// ----------------------- TExitTextControl -----------------------

class TExitTextControl : public BTextControl, private BMessageFilter
{
public:
	TExitTextControl(BRect frame,
				const char *name,
				const char *label, 
				const char *initial_text, 
				BMessage *exit_message,
				uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
	:	BTextControl(frame, name, label, initial_text, 0, rmask, flags),
		BMessageFilter(B_KEY_DOWN),
		fExitMessage(exit_message)
	{
		if (TextView()) TextView()->AddFilter(this);
	}
	
	virtual	~TExitTextControl()
	{
		if (TextView()) TextView()->RemoveFilter(this);
		delete fExitMessage;
	}

	virtual	filter_result Filter(BMessage *message, BHandler **target)
	{
		uchar ch = 0;
		if (message->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR) {
			if (ch == B_ENTER && fExitMessage) {
				Invoke(fExitMessage);
				return B_SKIP_MESSAGE;
			}
		}
		return B_DISPATCH_MESSAGE;
	}

private:
	BMessage* fExitMessage;
};

// ----------------------- TURLEditor -----------------------

static TBookmarkItem* find_bookmark(TBookmarkItem* folder, TBookmarkItem* item)
{
	if( folder == item ) return item;
	
	for( int32 i=0; i<folder->CountItems(); i++ ) {
		TBookmarkItem* found = find_bookmark(folder->ItemAt(i), item);
		if( found ) return found;
	}
	
	return 0;
}

class TURLEditor : public BView
{
public:
	TURLEditor(BRect frame, const char *name, TBookmarkItem* bm)
		: BView(frame, name, B_FOLLOW_NONE, B_FRAME_EVENTS),
		  fURL(0), fFolder(0)
	{
		fURL = new TExitTextControl(bogusRect, "url_text", 0, bm->URL(),
									new BMessage(BOOKMARKLIST_STOP_EDIT_MSG),
									B_FOLLOW_NONE);
		fURL->SetDivider(-3);
		AddChild(fURL);
		fURL->TextView()->SelectAll();
		
		BMenu* menu = new BPopUpMenu("");
		
		TBookmarkItem* root = bm;
		while( root->Parent() ) root = root->Parent();
		
		BMenuItem* item = new BMenuItem("(None)", 0);
		menu->AddItem(item);
		if( bm->Parent() == root ) item->SetMarked(true);
		menu->AddSeparatorItem();
		
		for( int32 i=0; i<root->CountItems(); i++ ) {
			TBookmarkItem* pos = root->ItemAt(i);
			if( pos->IsFolder() ) {
				BMessage* msg = new BMessage();
				msg->AddPointer("bookmark", pos);
				item = new BMenuItem(pos->Name(), msg);
				menu->AddItem(item);
				if( pos == bm->Parent() ) item->SetMarked(true);
			}
		}
		
		fFolder = new BMenuField(BRect(0, 0, 5, 5),
								 "folder_menu", ResourceString(SI_FOLDER), menu,
								 false, B_FOLLOW_NONE);
		fFolder->SetDivider(0);
		
		AddChild(fFolder);
	}
	
	virtual ~TURLEditor()
	{
	}
	
	const char* URL() const
	{
		return fURL ? fURL->Text() : "";
	}
	
	TBookmarkItem* Folder(TBookmarkItem* root) const
	{
		while( root && root->Parent() ) root = root->Parent();
		if( fFolder->Menu() ) {
			BMenuItem* it = fFolder->Menu()->FindMarked();
			if( it ) {
				TBookmarkItem* folder = 0;
				if( it->Message() ) {
					it->Message()->FindPointer("bookmark", (void**)&folder);
				}
				if( !folder ) return root;
				return find_bookmark(root, folder);
			}
		}
		return 0;
	}
	
	virtual void AllAttached()
	{
		if (Parent()) fURL->SetTarget(BMessenger(Parent()));
	}
	
	virtual void Draw(BRect updateRect)
	{
	}
	
	virtual void FrameResized(float new_width, float new_height)
	{
		BPoint where(0, 0);
		if( fURL ) {
			float pw, ph;
			fURL->GetPreferredSize(&pw, &ph);
			fURL->ResizeToPreferred();
			fURL->ResizeTo(Bounds().Width(), ph);
			fURL->MoveTo(0, 2);
			where.y = ph + 3;
		}
		
		where.y += 2;
		if( fFolder ) {
			float pw, ph;
			GetMenuFieldSize(fFolder, &pw, &ph, true);
			PRINT(("Resizing folder to: %.2fx%.2f\n", pw, ph));
			fFolder->ResizeTo(Bounds().Width(), ph);
			fFolder->MoveTo(0, where.y);
		}
	}
	
	virtual void SetViewColor(rgb_color c)
	{
		rgb_color oldColor = ViewColor();
		inherited::SetViewColor(c);
		if( fURL ) fURL->SetViewColor(c);
		if( fFolder ) fFolder->SetViewColor(c);
		if( Window() && *(int*)&oldColor != *(int*)&c ) {
			Invalidate();
			if( fURL ) fURL->Invalidate();
			if( fFolder ) fFolder->Invalidate();
		}
	}
	
	virtual void SetLowColor(rgb_color c)
	{
		inherited::SetLowColor(c);
		if( fURL ) fURL->SetLowColor(c);
		if( fFolder ) fFolder->SetLowColor(c);
	}
	
	virtual void SetHighColor(rgb_color c)
	{
		inherited::SetLowColor(c);
		if( fFolder ) fFolder->SetHighColor(c);
	}
	
	virtual void SetFont(const BFont *font, uint32 mask = B_FONT_ALL)
	{
		inherited::SetFont(font, mask);
		if( fURL ) {
			fURL->SetFont(font, mask);
			fURL->TextView()->SetFontAndColor(font, mask);
		}
		if( fFolder ) fFolder->SetFont(font, mask);
		FrameResized(Bounds().Width(), Bounds().Height());
	}
	
	virtual void GetPreferredSize(float *width, float *height)
	{
		*width = *height = 0;
		if( fURL ) fURL->GetPreferredSize(width, height);
		if( fFolder ) {
			float pw, ph;
			GetMenuFieldSize(fFolder, &pw, &ph, false);
			*height += ph;
			if( pw > *width ) *width = pw;
		}
		*height += 4;
	}
	
private:
	typedef BView inherited;
	
	BTextControl* fURL;
	BMenuField* fFolder;
};

// ----------------------- TBookmarkListItem -----------------------

enum bookmark_region {
	no_region,
	info_region,
	icon_region,
	label_region,
	delete_region
};

class TBookmarkListItem : public BStringItem
{
public:
	TBookmarkListItem(TBookmarkItem* bookmark, const bookmark_attrs& attrs,
					  uint32 outlineLevel = 0, bool expanded = false);
	
	virtual ~TBookmarkListItem();

	TBookmarkItem* Bookmark() const;
	
	virtual	void DrawItem(BView *owner, BRect bounds, bool complete = false);
	virtual	void Update(BView *owner, const BFont *font);
	
	enum command {
		kNothing,
		kInfo,
		kOpen,
		kDraggable,
		kDelete
	};
	
	// return kNothing if not using this mouse event.
	virtual command MouseDown(BView* owner, BRect bounds, BPoint where);
	virtual	void MouseMoved(BView* owner, BRect bounds,
							BPoint where, uint32 code, const BMessage *msg);
	virtual command MouseUp(BView* owner, BRect bounds, BPoint where);
	
	bool IsEditingInfo() const						{ return fInfoEditing; }
	bool StartEditingInfo(BView* owner, BRect bounds);
	void StopEditingInfo(BView* owner, BRect bounds);
	
	rgb_color BackgroundColor() const;
	rgb_color TextColor() const;
	
	const BBitmap* InfoBitmap() const;
	const BBitmap* IconBitmap() const;
	const BBitmap* DeleteBitmap() const;
	
private:
	typedef BStringItem inherited;
	
	BRect info_frame(BRect bounds) const;
	BRect icon_frame(BRect bounds) const;
	BRect delete_frame(BRect bounds) const;
	float text_left_edge(BRect bounds) const;
	BRect name_editor_frame(BRect bounds) const;
	BRect url_editor_frame(BRect bounds) const;
	void set_info_state(BView* owner, BRect bounds,
						bookmark_region over, bookmark_region pressed,
						bool editing);
	bookmark_region region_at(BRect bounds, BPoint where);
	bool can_delete() const;
	
	const bookmark_attrs& fAttrs;
	TBookmarkItem* fBookmark;
	BTextControl* fNameEditor;
	TURLEditor* fURLEditor;
	BString fTruncated;
	int32 fLineHeight;
	float fTextControlHeight;
	
	float fCountWidth;
	
	bookmark_region fOverRegion;
	bookmark_region fPressRegion;
	
	bool fInfoEditing;
};

TBookmarkListItem::TBookmarkListItem(TBookmarkItem* bookmark,
									 const bookmark_attrs& attrs,
									 uint32 outlineLevel, bool expanded)
	: BStringItem(bookmark ? bookmark->Name() : "", outlineLevel, expanded),
	  fAttrs(attrs), fBookmark(bookmark),
	  fNameEditor(0), fURLEditor(0), fLineHeight(0), fTextControlHeight(0),
	  fCountWidth(0), fOverRegion(no_region),
	  fPressRegion(no_region), fInfoEditing(false)
{
}

TBookmarkListItem::~TBookmarkListItem()
{
	if( fNameEditor ) {
		fNameEditor->RemoveSelf();
		delete fNameEditor;
	}
	if( fURLEditor ) {
		fURLEditor->RemoveSelf();
		delete fURLEditor;
	}
}

TBookmarkItem* TBookmarkListItem::Bookmark() const
{
	return fBookmark;
}

void TBookmarkListItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	rgb_color	viewcol = owner->LowColor();
	rgb_color	bgcol = BackgroundColor();
	rgb_color	txcol = IsEnabled() ? TextColor() : disable_color(TextColor(), BackgroundColor());
	
	if( fNameEditor ) {
		BRect frame = name_editor_frame(bounds);
		rgb_color oldColor = fNameEditor->ViewColor();
		fNameEditor->ResizeTo(frame.Width(), frame.Height());
		fNameEditor->MoveTo(frame.left, frame.top - owner->Bounds().top);
		fNameEditor->SetViewColor(bgcol);
		fNameEditor->SetLowColor(bgcol);
		if( fNameEditor->Window() && *(int*)&oldColor != *(int*)&bgcol ) {
			fNameEditor->Invalidate();
		}
	}
	
	if( fURLEditor ) {
		BRect frame = url_editor_frame(bounds);
		fURLEditor->ResizeTo(frame.Width(), frame.Height());
		fURLEditor->MoveTo(frame.left, frame.top - owner->Bounds().top);
		fURLEditor->SetViewColor(bgcol);
		fURLEditor->SetLowColor(bgcol);
		fURLEditor->SetHighColor(txcol);
	}
	
	owner->PushState();
		
	owner->SetLowColor(bgcol);		// for anti-aliasing text
	owner->SetDrawingMode(B_OP_COPY);
	
	bool countShown = false;
	
	// Figure out where text should be drawn.
	BPoint textpos;
	const char* text = Text();
	float textw = 0;
	BPoint countpos;
	char countstr[16];
	countstr[0] = 0;
	if( text ) {
		owner->SetHighColor(txcol);
		textpos = BPoint(text_left_edge(bounds),
						 bounds.top+fLineHeight-1-BaselineOffset());
		
		float w = fCountWidth;
		
		if( !IsExpanded() && Bookmark() && Bookmark()->CountItems() > 0 ) {
			sprintf(countstr, "%ld", Bookmark()->CountItems());
			countShown = true;
			float cw = owner->StringWidth(countstr);
			if( cw > w ) w = cw;
			countpos = BPoint(bounds.right-2-cw, textpos.y);
		}
		
		float right = bounds.right - w - 4;
		
		if( fTruncated.Length() <= 0 ) {
			textw = owner->StringWidth(text);
			if( textw > (right-textpos.x) ) {
				BFont font;
				owner->GetFont(&font);
				char* out = fTruncated.LockBuffer(strlen(text)+3);
				font.GetTruncatedStrings(&text, 1, B_TRUNCATE_MIDDLE,
										 (right-textpos.x), &out);
				fTruncated.UnlockBuffer();
				textw = owner->StringWidth(fTruncated.String());
			}
		} else {
			textw = owner->StringWidth(fTruncated.String());
		}
	}
	
	// Draw background
	owner->SetHighColor(bgcol);
	owner->FillRect(BRect(bounds.left, bounds.top+1, bounds.right, bounds.bottom-1));
	owner->SetHighColor(viewcol);
	owner->StrokeLine(BPoint(bounds.left, bounds.top),
					  BPoint(bounds.right, bounds.top));
	owner->StrokeLine(BPoint(bounds.left, bounds.bottom),
					  BPoint(bounds.right, bounds.bottom));
	
	// Draw text
	if( Text() ) {
		owner->SetHighColor(txcol);
		
		if( countstr[0] != 0 ) {
			owner->MovePenTo(countpos);
			owner->DrawString(countstr);
		}
		
		owner->MovePenTo(textpos);
		owner->DrawString( fTruncated.Length() > 0 ? fTruncated.String() : text );
		
		if (fAttrs.fUnderlineLinks) {
			if( fOverRegion == icon_region || fOverRegion == label_region ) {
				float y = textpos.y+1+fAttrs.fUnderlineOffset;
				owner->FillRect(BRect(textpos.x, y,
										textpos.x+textw, y+fAttrs.fUnderlineHeight-1));
			}
		}
	}
	
	owner->SetDrawingMode(B_OP_ALPHA);
	owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	
	if( Bookmark() ) {
		// If this item actually has a bookmark, draw its icons for
		// manipulating it.
		
		const BBitmap* infoBM = InfoBitmap();
		const BBitmap* iconBM = IconBitmap();
		const BBitmap* deleteBM = Bookmark()->IsDeletable() ? DeleteBitmap() : NULL;
		
		PRINT(("Bookmark %s drawing info bitmap %p\n", Bookmark()->Name(), infoBM));
		
		if( infoBM ) owner->DrawBitmapAsync(infoBM, info_frame(bounds).LeftTop());
		if( iconBM ) owner->DrawBitmapAsync(iconBM, icon_frame(bounds).LeftTop());
		if( deleteBM && !countShown ) {
			// can only delete if this doesn't have any sub-items.
			if( !Bookmark() || Bookmark()->CountItems() == 0 ) {
				owner->DrawBitmapAsync(deleteBM, delete_frame(bounds).LeftTop());
			}
		}
	}
	
	owner->PopState();
}

void TBookmarkListItem::Update(BView *owner, const BFont *font)
{
	inherited::Update(owner, font);
	
	float extra_w = 0;
	float min_h = 0;
	
	const BBitmap* infoBM = InfoBitmap();
	const BBitmap* iconBM = IconBitmap();
	const BBitmap* deleteBM = DeleteBitmap();
	
	if( infoBM ) {
		extra_w += infoBM->Bounds().Width()+4;
		float h = infoBM->Bounds().Height()+1;
		if( h > min_h ) min_h = h;
	}
	if( iconBM ) {
		extra_w += (iconBM->Bounds().Width()+4)*(OutlineLevel()+1);
		float h = iconBM->Bounds().Height()+1;
		if( h > min_h ) min_h = h;
	}
	
	char count[16];
	sprintf(count, "%ld", Bookmark() ? Bookmark()->CountItems() : 0);
	fCountWidth = owner->StringWidth(count);
	float w = owner->StringWidth("88");
	if( w > fCountWidth ) fCountWidth = w;
	if( deleteBM ) {
		if( deleteBM->Bounds().Width() > fCountWidth ) {
			fCountWidth = deleteBM->Bounds().Width();
		}
	}
	
	fTruncated = "";
	
	SetWidth(Width() + extra_w);
	if( min_h > Height() ) SetHeight(min_h);
	
	fLineHeight = int32(Height()+4.5)+1;
	if( fNameEditor && fTextControlHeight > fLineHeight ) {
		SetHeight(fTextControlHeight-1);
	} else {
		SetHeight(fLineHeight-1);
	}
	font_height fh;
	font->GetHeight(&fh);
	if ((fh.ascent+fh.descent) < fLineHeight) {
		SetBaselineOffset(BaselineOffset()+floor((fLineHeight-fh.ascent-fh.descent)/2));
	}
	SetBaselineOffset(BaselineOffset()+2);
	
	if( fURLEditor ) {
		float pw, ph;
		fURLEditor->GetPreferredSize(&pw, &ph);
		SetHeight(Height() + ph + 2);
	}
}

TBookmarkListItem::command TBookmarkListItem::MouseDown(BView* owner,
														BRect bounds,
														BPoint where)
{
	bookmark_region hit = region_at(bounds, where);
	
	set_info_state(owner, bounds, hit, hit, fInfoEditing);
	
	switch( hit ) {
		case info_region:		return kInfo;
		case icon_region:
		case label_region:		return (Bookmark() && !Bookmark()->IsFolder()) ? kDraggable : kOpen;
		case delete_region:		return can_delete() ? kDelete : kOpen;
		default:				return kNothing;
	}
}

void TBookmarkListItem::MouseMoved(BView* owner, BRect bounds,
								   BPoint where, uint32 code, const BMessage *msg)
{
	bookmark_region new_over = no_region;
	
	if( code != B_EXITED_VIEW ) {
		new_over = region_at(bounds, where);
	}
	
	set_info_state(owner, bounds, new_over, fPressRegion, fInfoEditing);
}

TBookmarkListItem::command TBookmarkListItem::MouseUp(BView* owner,
													  BRect bounds,
													  BPoint where)
{
	bookmark_region hit = region_at(bounds, where);
	command ret = kNothing;
	
	if( hit == fPressRegion && hit != no_region ) {
		switch( fPressRegion ) {
			case info_region:		ret = kInfo;								break;
			case icon_region:
			case label_region:		ret = kOpen;								break;
			case delete_region:		ret = can_delete() ? kDelete : kOpen;		break;
			default:				ret = kNothing;								break;
		}
	}
	
	set_info_state(owner, bounds, hit, no_region, fInfoEditing);
	
	return ret;
}
	
bool TBookmarkListItem::StartEditingInfo(BView* owner, BRect bounds)
{
	if( fInfoEditing ) return false;
	TBookmarkItem* bm = Bookmark();
	if( !bm ) return false;
	
	fInfoEditing = true;
	
	BFont font;
	owner->GetFont(&font);
	
	fNameEditor = new TExitTextControl(bogusRect, "name_editor", 0, bm->Name(),
									   new BMessage(BOOKMARKLIST_STOP_EDIT_MSG),
									   B_FOLLOW_NONE);
	fNameEditor->SetFont(&font);
	fNameEditor->TextView()->SetFontAndColor(&font);
	float w;
	fNameEditor->GetPreferredSize(&w, &fTextControlHeight);
	BRect frame(name_editor_frame(bounds));
	fNameEditor->MoveTo(frame.LeftTop());
	fNameEditor->ResizeTo(frame.Width(), frame.Height());
	fNameEditor->ResizeToPreferred();
	//fNameEditor->SetDivider(-3);
	owner->AddChild(fNameEditor);
	fNameEditor->SetViewColor(BackgroundColor());
	fNameEditor->SetLowColor(BackgroundColor());
	fNameEditor->TextView()->SelectAll();
	fNameEditor->MakeFocus();
	
	fNameEditor->SetTarget(BMessenger(owner));
	
	if( !bm->IsFolder() ) {
		fURLEditor = new TURLEditor(bogusRect, "url_editor", bm);
		fURLEditor->SetViewColor(BackgroundColor());
		fURLEditor->SetLowColor(BackgroundColor());
		fURLEditor->SetFont(&font);
		owner->AddChild(fURLEditor);
	}
	
	set_info_state(owner, bounds, fOverRegion, fPressRegion, true);
	return true;
}

void TBookmarkListItem::StopEditingInfo(BView* owner, BRect bounds)
{
	if( Bookmark() ) {
		if( fURLEditor ) {
			TBookmarkItem* folder = fURLEditor->Folder(Bookmark());
			if( folder && folder != Bookmark()->Parent() ) {
				Bookmark()->Parent()->RemoveItem(Bookmark());
				folder->AddItem(Bookmark());
			}
			Bookmark()->SetURL(fURLEditor->URL());
		}
		if( fNameEditor ) Bookmark()->SetName(fNameEditor->Text());
	}
	
	if( fNameEditor ) {
		fNameEditor->RemoveSelf();
		delete fNameEditor;
		fNameEditor = 0;
	}
	if( fURLEditor ) {
		fURLEditor->RemoveSelf();
		delete fURLEditor;
		fURLEditor = 0;
	}
	set_info_state(owner, bounds, fOverRegion, fPressRegion, false);
}

rgb_color TBookmarkListItem::BackgroundColor() const
{
	if( fPressRegion != no_region ) return fAttrs.fSelectedColor;
	if( fOverRegion != no_region ) return fAttrs.fSelectedColor;
	if( fInfoEditing ) return fAttrs.fSelectedColor;
	
	//if( IsSelected() ) return fAttrs.fSelectedColor;
	
	if( Bookmark() && Bookmark()->IsFolder() ) {
		return IsExpanded() ? fAttrs.fOpenFolderColor : fAttrs.fFolderColor;
	}
	
	return fAttrs.fBookmarkColor;
}

rgb_color TBookmarkListItem::TextColor() const
{
	if( (fPressRegion == icon_region || fPressRegion == label_region)
			&& fPressRegion == fOverRegion ) {
		return fAttrs.fActiveColor;
	}
	return fAttrs.fTextColor;
}

const BBitmap* TBookmarkListItem::InfoBitmap() const
{
	if( fInfoEditing ) {
		switch( fOverRegion ) {
			case info_region:		return fAttrs.fInfoOnOverBitmap;
			default:				return fAttrs.fInfoOnBitmap;
		}
	}
	switch( fOverRegion ) {
		case info_region:		return fAttrs.fInfoOverBitmap;
		case no_region:			return fPressRegion == no_region ? fAttrs.fInfoOutsideBitmap : fAttrs.fInfoBitmap;
		default:				return fAttrs.fInfoBitmap;
	}
}

const BBitmap* TBookmarkListItem::IconBitmap() const
{
	if( Bookmark() && Bookmark()->IsFolder() ) {
		return IsExpanded() ? fAttrs.fFolderBitmap : fAttrs.fFolderClosedBitmap;
	}
	return fAttrs.fBookmarkBitmap;
}

const BBitmap* TBookmarkListItem::DeleteBitmap() const
{
	switch( fOverRegion ) {
		case delete_region:		return fAttrs.fDeleteOverBitmap;
		case no_region:			return fPressRegion == no_region ? fAttrs.fDeleteOutsideBitmap : fAttrs.fDeleteBitmap;
		default:				return fAttrs.fDeleteBitmap;
	}
}

BRect TBookmarkListItem::info_frame(BRect bounds) const
{
	const BBitmap* infoBM = InfoBitmap();
	if( !infoBM ) return BRect();
	
	bounds.left += 4;
	bounds.top += 1 + floor((fLineHeight-3-infoBM->Bounds().Height())/2+.5);
	bounds.right = bounds.left + infoBM->Bounds().Width();
	bounds.bottom = bounds.top + infoBM->Bounds().Height();
	
	return bounds;
}

BRect TBookmarkListItem::delete_frame(BRect bounds) const
{
	const BBitmap* deleteBM = DeleteBitmap();
	if( !deleteBM ) return BRect();
	
	bounds.right -= 4;
	bounds.top += 1 + floor((fLineHeight-3-deleteBM->Bounds().Height())/2+.5);
	bounds.left = bounds.right - deleteBM->Bounds().Width();
	bounds.bottom = bounds.top + deleteBM->Bounds().Height();
	
	return bounds;
}

BRect TBookmarkListItem::icon_frame(BRect bounds) const
{
	const BBitmap* iconBM = IconBitmap();
	if( !iconBM ) return BRect();
	
	BRect frame = info_frame(bounds);
	if( !frame.IsValid() ) {
		frame = bounds;
		frame.left = 4;
	}
	
	frame.left = frame.right + 4 + ( (iconBM->Bounds().Width()+4) * OutlineLevel() );
	frame.top = bounds.top + 1 + floor((fLineHeight-3-iconBM->Bounds().Height())/2+.5);
	frame.right = frame.left + iconBM->Bounds().Width();
	frame.bottom = frame.top + iconBM->Bounds().Height();
	
	return frame;
}

float TBookmarkListItem::text_left_edge(BRect bounds) const
{
	bounds = icon_frame(bounds);
	if( !bounds.IsValid() ) bounds.right = 4;
	
	return bounds.right + 8;
}

BRect TBookmarkListItem::name_editor_frame(BRect bounds) const
{
	bounds.top += floor( (fLineHeight-fTextControlHeight)/2 + .5 );
	bounds.left = text_left_edge(bounds) - 4;
	bounds.bottom = bounds.top + fTextControlHeight-1; //fLineHeight-1;
	return bounds;
}

BRect TBookmarkListItem::url_editor_frame(BRect bounds) const
{
	bounds.left = text_left_edge(bounds) - 4;
	if( fLineHeight > fTextControlHeight ) bounds.top += fLineHeight;
	else bounds.top += fTextControlHeight;
	bounds.bottom -= 2;
	return bounds;
}

void TBookmarkListItem::set_info_state(BView* owner, BRect bounds,
									   bookmark_region over,
									   bookmark_region pressed,
									   bool editing)
{
	if( fOverRegion != over || fPressRegion != pressed || fInfoEditing != editing ) {
		PRINT(("Setting: over=%d, pressed=%d, editing=%d\n",
				over, pressed, editing));
		fOverRegion = over;
		fPressRegion = pressed;
		fInfoEditing = editing;
		if( owner ) {
			PRINT(("Invalidating: ")); DEBUG_ONLY(info_frame(bounds).PrintToStream());
			owner->Invalidate(bounds);
		}
	}
}

bookmark_region TBookmarkListItem::region_at(BRect bounds, BPoint where)
{
	if( !bounds.Contains(where) ) return no_region;
	
	if( info_frame(bounds).Contains(where) ) return info_region;
	if( icon_frame(bounds).Contains(where) ) return icon_region;
	if( delete_frame(bounds).Contains(where) ) return delete_region;
	
	return label_region;
}

bool TBookmarkListItem::can_delete() const
{
	return (Bookmark() && Bookmark()->IsDeletable() && Bookmark()->CountItems() <= 0);
}

// ----------------------- TBookmarkList -----------------------

TBookmarkList::TBookmarkList(BRect frame, const char* name,
							 const BMessage& attrs,
							 list_view_type type,
							 uint32 resizeFlags, uint32 flags)
	: BListView(frame, name, type, resizeFlags, flags),
	  fAttrs(attrs), fDeletionMessage(0),
	  fBookmarks(0), fFolder(0), fEditItem(-1),
	  fHitItem(-1), fPressingItem(false), fCanDrag(false), fOverItem(-1)
{
	fViewColor = fAttrs.fBackgroundColor;
	inherited::SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(fAttrs.fBackgroundColor);
	SetHighColor(fAttrs.fTextColor);
	SetFont(&fAttrs.fListFont);
	UpdateBookmarkList();
}

TBookmarkList::~TBookmarkList()
{
	SetBookmarks(0);
	SetDeletionMessage(0);
}

void TBookmarkList::SetDeletionMessage(BMessage *message)
{
	delete fDeletionMessage;
	fDeletionMessage = message;
}

BMessage* TBookmarkList::DeletionMessage() const
{
	return fDeletionMessage;
}

uint32 TBookmarkList::DeletionCommand() const
{
	return fDeletionMessage ? fDeletionMessage->what : 0;
}

void TBookmarkList::SetBookmarks(TBookmarkFile* bookmarks)
{
	fBookmarks = 0;
	UpdateBookmarkList();
	fBookmarks = bookmarks;
	UpdateBookmarkList();
}

TBookmarkFile* TBookmarkList::Bookmarks() const
{
	return fBookmarks;
}

void TBookmarkList::SetCurrentFolder(TBookmarkItem* folder)
{
	if( folder != CurrentFolder() ) {
		int32 item = -1;
		for( int32 i=0; i<CountItems(); i++ ) {
			TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(i));
			if( bli && bli->Bookmark() == folder ) {
				item = i;
				break;
			}
		}
		if( item >= 0 ) Select(item);
		else DeselectAll();
		
		ScrollToSelection();
	}
}

TBookmarkItem* TBookmarkList::CurrentFolder(uint32* level, int32* index) const
{
	int32 s1 = CurrentSelection();
	TBookmarkListItem* sel = s1 >= 0
						   ? dynamic_cast<TBookmarkListItem*>(ItemAt(s1))
						   : 0;
	if( sel && CurrentSelection(1) >= 0 ) return 0;
	
	TBookmarkItem* it = sel ? sel->Bookmark() : 0;
	if( !it && fBookmarks ) it = fBookmarks->Root();
	if( !it ) return 0;
	
	if( !it->IsFolder() ) it = it->Parent();
	
	uint32 my_level = 0;
	if( level || index ) {
		TBookmarkItem* p = it->Parent();
		while( p ) {
			my_level++;
			p = p->Parent();
		}
		if( level ) *level = my_level;
	}
	
	if( index ) {
		while( s1 >= 0 ) {
			TBookmarkListItem* tbi = dynamic_cast<TBookmarkListItem*>(ItemAt(s1));
			if( tbi && tbi->OutlineLevel() == (my_level-1) &&
					tbi->Bookmark() && tbi->Bookmark()->IsFolder() ) break;
			s1--;
		}
		if( s1 >= 0 ) *index = s1;
		else *index = -1;
	}
	
	return it;
}

TBookmarkItem* TBookmarkList::CurrentBookmark() const
{
	int32 s1 = CurrentSelection();
	TBookmarkListItem* sel = s1 >= 0
						   ? dynamic_cast<TBookmarkListItem*>(ItemAt(s1))
						   : 0;
	if( sel && CurrentSelection(1) >= 0 ) return 0;
	return sel ? sel->Bookmark() : 0;
}

void TBookmarkList::SetCurrentEdit(int32 idx)
{
	PRINT(("Setting up to edit item #%ld\n", idx));
	
	TBookmarkListItem* it = 0;
	if( CurrentEdit() >= 0 &&
			(it=dynamic_cast<TBookmarkListItem*>(ItemAt(CurrentEdit()))) != 0 ) {
		PRINT(("Stoping edit of item %p\n", it));
		it->StopEditingInfo(this, ItemFrame(CurrentEdit()));
		UpdateItem(CurrentEdit());
		fEditItem = -1;
	}
	
	if( idx >= 0 &&
			(it=dynamic_cast<TBookmarkListItem*>(ItemAt(idx))) != 0 ) {
		PRINT(("Starting edit of item %p\n", it));
		if( it->StartEditingInfo(this, ItemFrame(idx)) ) {
			fEditItem = idx;
			Select(idx);
			UpdateItem(idx);
			ScrollToSelection();
		}
	}
}

void TBookmarkList::SetCurrentEdit(TBookmarkItem* item)
{
	for (int32 i=0; i<CountItems(); i++) {
		TBookmarkListItem* it = dynamic_cast<TBookmarkListItem*>(ItemAt(i));
		if (it && it->Bookmark() == item) {
			SetCurrentEdit(i);
			return;
		}
	}
	
	SetCurrentEdit(-1);
}

int32 TBookmarkList::CurrentEdit() const
{
	return fEditItem;
}

int32 TBookmarkList::OffsetAtLevel(uint32 level, int32 offset, int32 starting) const
{
	TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(starting));
	while( bli && offset > 0 && starting < CountItems() ) {
		if( starting < CountItems() ) {
			starting++;
			bli = dynamic_cast<TBookmarkListItem*>(ItemAt(starting));
		}
		while( bli && bli->OutlineLevel() > level && starting < CountItems() ) {
			starting++;
			bli = dynamic_cast<TBookmarkListItem*>(ItemAt(starting));
		}
		if( bli && bli->OutlineLevel() < level ) return starting;
		offset--;
	}
	return starting;
}

void TBookmarkList::AddBookmarks(const BList* bookmarks, TBookmarkItem* folder)
{
	if( Window() ) {
		Window()->DisableUpdates();
		Window()->BeginViewTransaction();
	}
	
	PRINT(("Adding %ld bookmarks to %p...\n", bookmarks->CountItems(), folder));
	
	SetCurrentEdit(-1);
	
	uint32 level = 0;
	int32 index = -1;
	//for now, always add at root level -- the currently selected item
	//of the list view is not shown or managed much.
	//if( !folder ) folder = CurrentFolder(&level, &index);
	if( !folder ) folder = fBookmarks ? fBookmarks->Root() : 0;
	
	DeselectAll();
	
	if( index >= 0 ) ExpandItem(index);
	
	for( int32 i=0; i<bookmarks->CountItems(); i++ ) {
		TBookmarkItem* item = (TBookmarkItem*)bookmarks->ItemAt(i);
		PRINT(("Adding bookmark item: %p (%s)\n", item, item->Name()));
		int32 pos = folder
				  ? folder->AddSortedItem(item, BMessenger(this))
				  : -1;
		if( pos >= 0 ) {
			TBookmarkListItem* bi = new TBookmarkListItem(item, fAttrs, level);
			PRINT(("Adding bookmark %p: level=%ld, pos=%ld, top=%ld\n",
					item, level, pos, index));
			pos = OffsetAtLevel(level, pos, index+1);
			PRINT(("Position in list = %ld\n", pos));
			AddItem(bi, pos);
			Select(pos, true);
		} else {
			delete item;
		}
	}
	
	ScrollToSelection();
	
	if( Window() ) {
		Window()->EndViewTransaction();
		Window()->EnableUpdates();
	}
	
	BMessage msg(T_BOOKMARK_CHANGED);
	Invoke(&msg);
}

bool TBookmarkList::CanRemove(int32 item) const
{
	TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(item));
	if( bli && bli->Bookmark() ) {
		if( !bli->Bookmark()->IsFolder() || bli->Bookmark()->CountItems() == 0 ) {
			return true;
		}
	}
	
	return false;
}

int32 TBookmarkList::RemoveSelected(BList* out_bookmarks)
{
	PRINT(("Removing currently selected bookmarks...\n"));
	
	SetCurrentEdit(-1);
	
	int32 first_index = -1;
	
	int32 sel;
	int32 pos = 0;
	while( (sel=CurrentSelection(pos)) >= 0 ) {
		if( !CanRemove(sel) ) {
			pos++;
			continue;
		}
		TBookmarkListItem* bi = dynamic_cast<TBookmarkListItem*>(ItemAt(sel));
		int32 next_pos = sel < (CountItems()-1) ? sel : (CountItems()-2);
		TBookmarkListItem* next_bi = dynamic_cast<TBookmarkListItem*>(ItemAt(next_pos+1));
		if( next_bi && next_bi->OutlineLevel() < bi->OutlineLevel() && sel > 0 ) {
			next_pos = sel - 1;
		}
		if( first_index < 0 || next_pos < first_index ) first_index = next_pos;
		if( bi ) {
			TBookmarkItem* item = bi->Bookmark();
			if( item && item->Parent() ) {
				PRINT(("Removing bookmark item: %p (%s)\n", item, item->Name()));
				out_bookmarks->AddItem(item);
				if( bi->IsExpanded() ) ContractItem(sel);
				delete RemoveItem(sel);
			}
			if( item->Parent() ) item->Parent()->RemoveItem(item, BMessenger(this));
		}
	}
	
	BMessage msg(T_BOOKMARK_CHANGED);
	Invoke(&msg);
	
	return first_index;
}

static int32 retrieve_bookmarks(BList* new_items, BList* selected_indices,
								int32 cur_index, uint32 level,
								const TBookmarkItem* from,
								const bookmark_attrs& attrs,
								const BList* selected, const BList* expanded)
{
	for( int32 i=0; i<from->CountItems(); i++ ) {
		TBookmarkItem* b = from->ItemAt(i);
		if( b ) {
			bool exp = false;
			if( b->IsFolder() && expanded ) {
				for( int32 j=0; j<expanded->CountItems(); j++ ) {
					if( expanded->ItemAt(j) == b ) {
						PRINT(("Doing previously expanded bookmark at %ld: %p (%s)\n",
								i, b, b->Name()));
						exp = true;
					}
				}
			}
			TBookmarkListItem* item = new TBookmarkListItem(b, attrs, level, exp);
			new_items->AddItem(item);
			if( selected && selected_indices ) {
				for( int32 j=0; j<selected->CountItems(); j++ ) {
					if( selected->ItemAt(j) == b ) {
						PRINT(("Doing previously selected bookmark at %ld: %p (%s)\n",
								i, b, b->Name()));
						selected_indices->AddItem((void*)cur_index);
					}
				}
			}
			
			cur_index++;
			
			if( exp ) {
				item->SetExpanded(true);
				cur_index = retrieve_bookmarks(new_items, selected_indices,
												cur_index, level+1,
												b, attrs, selected, expanded);
			}
		}
	}
	
	return cur_index;
}

void TBookmarkList::UpdateBookmarkList()
{
	if( Window() ) {
		Window()->DisableUpdates();
		Window()->BeginViewTransaction();
	}
	
	PRINT(("Updating list of child bookmarks...\n"));
	
	TBookmarkItem* editing = 0;
	if( fBookmarks && fEditItem >= 0 ) {
		TBookmarkListItem* b = dynamic_cast<TBookmarkListItem*>(ItemAt(fEditItem));
		if( b && b->Bookmark() ) editing = b->Bookmark();
	}
	
	SetCurrentEdit(-1);
	stop_watching();
	
	BList selected;
	BList expanded;
	
	BRect scrollPos = Bounds();
	while( CountItems() > 0 ) {
		BListItem* item = ItemAt(CountItems()-1);
		if( item ) {
			TBookmarkListItem* b = dynamic_cast<TBookmarkListItem*>(item);
			if( fBookmarks && b && b->Bookmark() ) {
				if( item->IsSelected() ) {
					PRINT(("Found selected bookmark: %p (%s)\n",
							b->Bookmark(), b->Bookmark()->Name()));
					selected.AddItem(b->Bookmark());
				}
				if( b->Bookmark()->IsFolder() && item->IsExpanded() ) {
					PRINT(("Found expanded bookmark: %p (%s)\n",
							b->Bookmark(), b->Bookmark()->Name()));
					expanded.AddItem(b->Bookmark());
				}
			}
		}
		Deselect(CountItems()-1);
		delete RemoveItem(CountItems()-1);
	}
	
	if( fBookmarks && fBookmarks->Root() ) {
		BList new_items;
		BList selected_indices;
		retrieve_bookmarks(&new_items, &selected_indices, 0, 0,
							fBookmarks->Root(), fAttrs, &selected, &expanded);
		AddList(&new_items);
		for( int32 i=0; i<selected_indices.CountItems(); i++ ) {
			Select((int32)selected_indices.ItemAt(i), true);
		}
	}
	
	start_watching();
	
	if( editing ) {
		for( int32 i=0; i<CountItems(); i++ ) {
			TBookmarkListItem* b = dynamic_cast<TBookmarkListItem*>(ItemAt(i));
			if( b && b->Bookmark() == editing ) {
				SetCurrentEdit(i);
				break;
			}
		}
	}
	
	ScrollTo(scrollPos.left, scrollPos.top);
	ScrollToSelection();
	
	if( Window() ) {
		Window()->EndViewTransaction();
		Window()->EnableUpdates();
	}
}

void TBookmarkList::UpdateItem(int32 index)
{
	BListItem* it = ItemAt(index);
	if( !it ) return;
	
	float origHeight = it->Height();
	BFont font;
	GetFont(&font);
	it->Update(this, &font);
	
	BRect inval(ItemFrame(index));
	if( origHeight != it->Height() ) {
		// If height has changed, need to re-draw this item and all
		// below it.
		inval.bottom = Bounds().bottom;
		// Kludge: make sure the vertical scroll bar has correct
		// dimensions.  This will cause the list view to call
		// FixupScrollBar().
		FrameResized(Bounds().Width(), Bounds().Height());
	}
	
	Invalidate(inval);
}

void TBookmarkList::ExpandItem(int32 index)
{
	if( index < 0 ) return;
	if( CurrentEdit() >= 0 ) debugger("can't expand while editing");
	
	TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(index));
	if( bli && !bli->IsExpanded() &&
			bli->Bookmark() && bli->Bookmark()->IsFolder() ) {
		InvalidateItem(index);
		BList items;
		retrieve_bookmarks(&items, 0, 0, bli->OutlineLevel()+1,
						   bli->Bookmark(), fAttrs, 0, 0);
		AddList(&items, index+1);
		bli->SetExpanded(true);
		bli->Bookmark()->StartWatching(BMessenger(this));
	}
}

void TBookmarkList::ContractItem(int32 index)
{
	if( index < 0 ) return;
	if( CurrentEdit() >= 0 ) debugger("can't contract while editing");
	
	TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(index));
	if( bli && bli->IsExpanded() &&
			bli->Bookmark() && bli->Bookmark()->IsFolder() ) {
		InvalidateItem(index);
		index++;
		TBookmarkListItem* it;
		while( (it=dynamic_cast<TBookmarkListItem*>(ItemAt(index))) != 0 ) {
			if( it->OutlineLevel() <= bli->OutlineLevel() ) break;
			RemoveItem(index);
			delete it;
		}
		bli->SetExpanded(false);
		bli->Bookmark()->StopWatching(BMessenger(this));
	}
}

void TBookmarkList::SetViewColor(rgb_color color)
{
	fViewColor = color;
}
	
void TBookmarkList::AttachedToWindow()
{
	inherited::AttachedToWindow();
	start_watching();
}

void TBookmarkList::DetachedFromWindow()
{
	// XXX: If something is currently being editing, we won't get
	// a notification about any changes.  Maybe we should completely
	// add/remove the bookmark items when being attached/detached?
	SetCurrentEdit(-1);
	stop_watching();
	inherited::DetachedFromWindow();
}

void TBookmarkList::MessageReceived(BMessage* msg)
{
	if( msg->WasDropped() ) {
		switch( msg->what ) {
			case BOOKMARKLIST_DRAG_MSG: {
				if( !Bookmarks() || !Bookmarks()->Root() ) return;
				TBookmarkItem* orig;
				if( msg->FindPointer("bookmark", (void**)&orig) != B_OK ) return;
				orig = find_bookmark(Bookmarks()->Root(), orig);
				if( !orig ) return;
				BPoint pt = msg->DropPoint();
				ConvertFromScreen(&pt);
				TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(
											ItemAt(IndexOf(pt)));
				TBookmarkItem* folder = bli ? bli->Bookmark() : 0;
				if( !folder ) {
					// if no item over this point, place it in the root folder
					folder = Bookmarks()->Root();
				}
				if( !folder ) return;
				while( folder->Parent() && !folder->IsFolder() ) folder = folder->Parent();
				if( !folder->IsFolder() ) return;
				if( folder == orig->Parent() ) return;
				orig->Parent()->RemoveItem(orig);
				folder->AddItem(orig);
				return;
			} break;
		}
		
		inherited::MessageReceived(msg);
		return;
	}
	
	switch( msg->what ) {
		case BOOKMARKLIST_STOP_EDIT_MSG:
			SetCurrentEdit(-1);
			break;
			
		case T_BOOKMARK_CHANGED: {
			TBookmarkItem* bm = 0;
			uint32 changes = 0;
			if( msg->FindPointer("bookmark", (void**)&bm) == B_OK &&
					msg->FindInt32("changes", (int32*)&changes) == B_OK ) {
				if( (changes&T_BOOKMARK_CHILDREN_CHANGED) ) {
					PRINT(("Updating full bookmark list...\n"));
					UpdateBookmarkList();
					Invoke(msg);
				} else if( changes&T_BOOKMARK_NAME_CHANGED ) {
					PRINT(("Updating bookmark item name...\n"));
					for( int32 i=0; i<CountItems(); i++ ) {
						TBookmarkListItem* bmi =
							dynamic_cast<TBookmarkListItem*>(ItemAt(i));
						if( bmi && bmi->Bookmark() == bm ) {
							bmi->SetText(bm->Name());
							InvalidateItem(i);
							Invoke(msg);
						}
					}
				}
			}
		} break;
		
		default:
			inherited::MessageReceived(msg);
			break;
	}
}
		
void TBookmarkList::MouseDown(BPoint where)
{
	move_over_item(where, NULL);
	
	fHitItem = IndexOf(where);
	fPressingItem = false;
	
	if( fHitItem >= 0 ) {
		TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(fHitItem));
		TBookmarkListItem::command cmd = TBookmarkListItem::kNothing;
		if( bli ) cmd = bli->MouseDown(this, ItemFrame(fHitItem), where);
		if( cmd != TBookmarkListItem::kNothing ) {
			PRINT(("Mouse down inside item %p\n", bli));
			fPressingItem = true;
			fCanDrag = cmd == TBookmarkListItem::kDraggable ? true : false;
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			fInitPos = where;
			return;
		}
	}
	
	inherited::MouseDown(where);
}

void TBookmarkList::MouseMoved(BPoint where, uint32 code, const BMessage *msg)
{
	if( fPressingItem ) {
		TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(fHitItem));
		if( bli ) {
			if( fCanDrag ) {
				float threshold = bli->Height()/2;
				float xdelta = fabs(where.x-fInitPos.x);
				float ydelta = fabs(where.y-fInitPos.y);
				threshold *= threshold;
				xdelta *= xdelta;
				ydelta *= ydelta;
				if( (xdelta+ydelta) >= threshold) {
					// start dragging.
					BMessage dragMsg(BOOKMARKLIST_DRAG_MSG);
					dragMsg.AddPointer("bookmark", bli->Bookmark());
					const BBitmap* bm = bli->IconBitmap();
					if( bm ) {
						BBitmap* newbm = new BBitmap(bm);
						BPoint pt((bm->Bounds().Width()/2), (bm->Bounds().Height()/2));
						DragMessage(&dragMsg, newbm, B_OP_BLEND, pt);
					} else {
						BRect rc(-8, -8, 8, 8);
						DragMessage(&dragMsg, rc);
					}
					bli->MouseUp(this, ItemFrame(fHitItem), where);
					fPressingItem = false;
					fHitItem = -1;
					fCanDrag = false;
					
					move_over_item(where, &dragMsg);
					return;
				}
			}
			bli->MouseMoved(this, ItemFrame(fHitItem), where, B_INSIDE_VIEW, 0);
		}
		return;
	}
	
	if( fHitItem < 0 ) move_over_item(where, msg);
	
	inherited::MouseMoved(where, code, msg);
}

void TBookmarkList::MouseUp(BPoint where)
{
	if( fPressingItem ) {
		TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(fHitItem));
		PRINT(("Mouse up inside item %p\n", bli));
		TBookmarkListItem::command cmd = TBookmarkListItem::kNothing;
		if( bli ) cmd = bli->MouseUp(this, ItemFrame(fHitItem), where);
		PRINT(("Command from item: %d\n", cmd));
		if( cmd == TBookmarkListItem::kInfo ) {
			PRINT(("Toggling info state.\n"));
			if( bli->IsEditingInfo() ) SetCurrentEdit(-1);
			else {
				SetCurrentEdit(fHitItem);
			}
		} else if( cmd == TBookmarkListItem::kOpen ) {
			if( bli && bli->Bookmark() ) {
				Select(fHitItem);
				if( bli->Bookmark()->IsFolder() ) {
					SetCurrentEdit(-1);
					if( bli->IsExpanded() ) ContractItem(fHitItem);
					else ExpandItem(fHitItem);
					Invoke();
				} else {
					Invoke();
				}
			}
		} else if( cmd == TBookmarkListItem::kDelete ) {
			if( bli && bli->Bookmark() ) {
				Select(fHitItem);
				if( DeletionMessage() ) Invoke(DeletionMessage());
			}
		}
		fPressingItem = false;
		fHitItem = -1;
		
		move_over_item(where, NULL);
		
		return;
	}
	
	if( fHitItem < 0 ) move_over_item(where, NULL);

	inherited::MouseUp(where);
	
	fHitItem = -1;
}

void TBookmarkList::Draw(BRect updateRect)
{
	float itemHeight = -1;
	
	BRect bottom(Bounds());
	if( CountItems() > 0 ) {
		BRect frame = ItemFrame(CountItems()-1);
		bottom.top = frame.bottom+1;
		itemHeight = frame.Height();
	}
	if( bottom.IsValid() && updateRect.Intersects(bottom) ) {
		PushState();
		
		TBookmarkListItem emptyItem(0, fAttrs);
		BFont font;
		GetFont(&font);
		emptyItem.Update(this, &font);
		
		while( emptyItem.Height() < bottom.Height() ) {
			BRect frame(bottom.left, bottom.top,
						bottom.right, bottom.top+emptyItem.Height());
			emptyItem.DrawItem(this, frame, true);
			bottom.top = frame.bottom+1;
		}
		
		SetHighColor(fViewColor);
		FillRect(bottom);
	}
	inherited::Draw(updateRect);
}
	
void TBookmarkList::start_watching()
{
	BMessenger t(this);
	
	if( fBookmarks && fBookmarks->Root() ) fBookmarks->Root()->StartWatching(t);
	for( int32 i=0; i<CountItems(); i++ ) {
		TBookmarkListItem* b = dynamic_cast<TBookmarkListItem*>(ItemAt(i));
		if( b && b->IsExpanded() && b->Bookmark() ) {
			b->Bookmark()->StartWatching(t);
		}
	}
}

void TBookmarkList::stop_watching()
{
	BMessenger t(this);
	
	if( fBookmarks && fBookmarks->Root() ) fBookmarks->Root()->StopWatching(t);
	for( int32 i=0; i<CountItems(); i++ ) {
		TBookmarkListItem* b = dynamic_cast<TBookmarkListItem*>(ItemAt(i));
		if( b && b->IsExpanded() && b->Bookmark() ) {
			b->Bookmark()->StopWatching(t);
		}
	}
}

void TBookmarkList::move_over_item(BPoint where, const BMessage* drag)
{
	int32 item = IndexOf(where);
	if (drag) {
		// Find target folder for drop.
		TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(item));
		TBookmarkItem* it = bli ? bli->Bookmark() : NULL;
		while (it && !it->IsFolder()) it = it->Parent();
		if (!it) item = -1;
		
		// Find bookmark item for this folder.
		bool changed = false;
		if (!bli || bli->Bookmark() != it) {
			changed = true;
			while (item >= 0) {
				bli = dynamic_cast<TBookmarkListItem*>(ItemAt(item));
				if (bli && bli->Bookmark() == it) break;
				item--;
			}
		}
		
		// If drop target is different than mouse over item, change
		// the mouse position to pretend like it is over.
		if (changed && item >= 0) {
			BRect frm = ItemFrame(item);
			where.x = (frm.left+frm.right)/2;
			where.y = (frm.top+frm.bottom)/2;
		}
	}
	
	PRINT(("Mousing over: old=%ld, new=%ld, ", fOverItem, item));
	DEBUG_ONLY(where.PrintToStream());
	if( item != fOverItem ) {
		TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(fOverItem));
		if( bli ) bli->MouseMoved(this, ItemFrame(fOverItem), where, B_EXITED_VIEW, NULL);
	}
	
	TBookmarkListItem* bli = dynamic_cast<TBookmarkListItem*>(ItemAt(item));
	if( bli ) bli->MouseMoved(this, ItemFrame(item), where,
							  item != fOverItem ? B_ENTERED_VIEW : B_INSIDE_VIEW, drag);
	fOverItem = item;
}

// --------------- Local Functions -- To Avoid Being Inlined ---------------

float GetMenuFieldSize(BMenuField* field, float* width, float* height,
						bool set_divider)
{
	BMenu* popup = field->Menu();
	
	font_height fhs;
	field->GetFontHeight(&fhs);
	const float fh = fhs.ascent+fhs.descent+fhs.leading;
	float fw = field->StringWidth("WWWW");
	
	float pref_w=0;
	if( popup ) {
		int32 num = popup->CountItems();
		for( int32 i=0; i<num; i++ ) {
			BMenuItem* item = popup->ItemAt(i);
			if( item ) {
				const float w=field->StringWidth(item->Label());
				if( w > pref_w ) pref_w = w;
			}
		}
	}
	
	float lw = (field->Label() && *field->Label())
		? field->StringWidth(field->Label()) + field->StringWidth(" ") + 5
		: 0;
	if( set_divider ) field->SetDivider(lw);
	*width = floor((fw>pref_w?fw:pref_w) + 20 + lw + .5);
	*height = floor(fh + 8 + .5);
	return lw;
}

BBitmapButton* MakeBitmapButton(const char* name, int32 label,
								BMessage* message,
								const char* normalBM, const char* overBM,
								const char* pressedBM, const char* disabledBM,
								const char* disabledPressedBM)
{
	return new BBitmapButton(
			bogusRect, name, ResourceString(label), message,
			ResourceBitmap(normalBM), ResourceBitmap(overBM),
			ResourceBitmap(pressedBM),
			ResourceBitmap(disabledBM),
			ResourceBitmap(disabledPressedBM)
		);
}

const BBitmap* ResourceBitmap(const char* name)
{
	return Resources().FindBitmap(B_PNG_FORMAT, name);
}

const char* ResourceString(int32 id)
{
	return Resources().FindString(B_STRING_BLOCK_TYPE, SB_MAIN, id);
}

static BResourceSet gResources;
static bool gResInitialized = false;
static BLocker gResLocker;

BResourceSet& Resources()
{
	if( gResInitialized ) return gResources;
	
	BAutolock l(&gResLocker);
	if( gResInitialized ) return gResources;
	
	gResources.AddEnvDirectory("${/service/web/macros/RESOURCES}/Bookmarks",
								"/boot/custom/resources/en/Bookmarks");
	gResInitialized = true;
	
	return gResources;
}
