
#include <Debug.h>

#include "BookmarkFile.h"
#include "BookmarkList.h"

#include "BookmarkEditor.h"

#include "util.h"

#include <Bitmap.h>
#include <ScrollView.h>
#include <View.h>
#include <Window.h>

#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include <Autolock.h>
#include <Debug.h>
#include <Locker.h>

#include <experimental/ResourceSet.h>

enum {
	// Messages that TBookmarkEditor internally sends to itself.
	ITEMSELECTED_MSG = 'itsl',
	OPEN_MSG = 'open',
	DELETE_MSG = 'dele',
	
	#if 0
	// Messages TBookmarkComponent sends to itself.
	RIGHT_TO_LEFT_MSG = 'rtlf',
	LEFT_TO_RIGHT_MSG = 'lfrt',
	UPDATETEXT_MSG = 'uptx',
	SETTEXT_MSG = 'sttx',
	UPDATEURL_MSG = 'upur',
	SETURL_MSG = 'stur'
	#endif
};

void SetOffsetBitmap(BView* view, const BBitmap* bitmap);
void SetViewAttributes(BView* view, const BFont* font,
						rgb_color background, rgb_color foreground);

static const BRect bogusRect(0, -400, 50, -350);

// ----------------------- TBookmarkEditor -----------------------

TBookmarkEditor::TBookmarkEditor(BRect frame, const char* name,
									const BMessage& attrs,
									TBookmarkFile* bookmarks,
									uint32 resizeFlags, uint32 flags)
	: BControl(frame, name, "", 0, resizeFlags, flags),
	  fBookmarks(0), fOwnBookmarks(false),
	  fBackground(0),
	  fHaveViewColor(false), fHaveHighColor(0),
	  fList(0), fScroller(0)
{
	rgb_color color;
	const char* str;
	if( attrs.FindInt32("bgcolor", (int32*)&color) == B_OK ) {
		SetViewColor(color);
	}
	if( attrs.FindInt32("text", (int32*)&color) == B_OK ) {
		SetHighColor(color);
	}
	if( attrs.FindString("background", &str) == B_OK ) {
		fBackground = ResourceBitmap(str);
	}
	
	if( attrs.FindString("buttonfont", &str) == B_OK ) {
		BFont font;
		GetFont(&font);
		if( decode_font(str, &font) == B_OK ) SetFont(&font);
	}
	
	BRect frame(bogusRect);
	frame.OffsetTo(0, 0);
	fList = new TBookmarkList(frame, "bookmark_list", attrs,
							  B_MULTIPLE_SELECTION_LIST);
	fList->SetSelectionMessage(new BMessage(ITEMSELECTED_MSG));
	fList->SetInvocationMessage(new BMessage(OPEN_MSG));
	fList->SetDeletionMessage(new BMessage(DELETE_MSG));
	fList->SetResizingMode(B_FOLLOW_ALL);
	fScroller = new BScrollView("bookmark_scroll", fList,
								B_FOLLOW_NONE, B_WILL_DRAW,
								false, true, B_NO_BORDER);
	AddChild(fScroller);
	
	bool own_bookmarks = false;
	if( !bookmarks ) {
		BPath path;
		if( find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK ) {
			if( path.Append("Favorites") == B_OK ) {
				BEntry entry(path.Path());
				if( entry.InitCheck() == B_OK ) {
					entry_ref ref;
					if( entry.GetRef(&ref) == B_OK ) {
						bookmarks = new TBookmarkFile;
						if( bookmarks->SetTo(&ref) != B_OK ) {
							delete bookmarks;
							bookmarks = 0;
						} else {
							own_bookmarks = true;
						}
					}
				}
			}
		}
	}
	
	SetBookmarks(bookmarks);
	fOwnBookmarks = own_bookmarks;
	
	Hide();
}

TBookmarkEditor::~TBookmarkEditor()
{
	SetBookmarks(0);
}

void TBookmarkEditor::SetBookmarks(TBookmarkFile* bookmarks)
{
	UpdateBookmarks();
	if( fOwnBookmarks ) {
		delete fBookmarks;
	}
	fBookmarks = bookmarks;
	fOwnBookmarks = false;
	if( fList ) fList->SetBookmarks(bookmarks);
	UpdateControlState();
}

TBookmarkFile* TBookmarkEditor::Bookmarks() const
{
	return fBookmarks;
}

void TBookmarkEditor::AttachedToWindow()
{
	inherited::AttachedToWindow();
	
	if( !fHaveViewColor ) {
		if( Parent() ) inherited::SetViewColor(Parent()->ViewColor());
		else inherited::SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
	
	SetLowColor(ViewColor());
	
	if( Parent() ) {
		if( !fHaveHighColor ) SetHighColor(Parent()->HighColor());
	}
	
	BFont font;
	GetFont(&font);
	
	if( fScroller ) fScroller->SetViewColor(ViewColor());
}

void TBookmarkEditor::AllAttached()
{
	inherited::AllAttached();

	LayoutViews(Bounds().Width(), Bounds().Height());
	
	BMessenger t(this);
	if( fList ) fList->SetTarget(t);
}

void TBookmarkEditor::DetachedFromWindow()
{
	UpdateBookmarks();
	inherited::DetachedFromWindow();
}

void TBookmarkEditor::MessageReceived(BMessage* msg)
{
	switch( msg->what ) {
		case T_BOOKMARK_SET_FRAME: {
			BRect frame;
			if( msg->FindRect("frame", &frame) == B_OK ) {
				MoveTo(frame.LeftTop());
				ResizeTo(frame.Width(), frame.Height());
				if( IsHidden() ) Show();
			}
		} break;
		
		case T_BOOKMARK_CHANGED: {
			UpdateBookmarks();
		} break;
		
		case ITEMSELECTED_MSG: {
			UpdateControlState();
		} break;
		
		case OPEN_MSG: {
			UpdateControlState();
			TBookmarkItem* it = fList ? fList->CurrentBookmark() : 0;
			if( it && !it->IsFolder() ) {
				OpenURL(it);
			}
		} break;
		
		case 'nfld': {
			if( fList && fList->CurrentFolder() ) {
				BList new_items;
				const char* name;
				if( msg->FindString("title", &name) != B_OK || !name || !*name ) {
					name = "New Folder";
				}
				BString buf(name);
				if( fBookmarks && fBookmarks->Root() ) {
					fBookmarks->Root()->MakeUniqueName(&buf);
				}
				new_items.AddItem(new TBookmarkItem(buf.String()));
				fList->AddBookmarks(&new_items, fBookmarks->Root());
			}
		} break;
		
		case 'abmk': {
			if( fList && fList->CurrentFolder() ) {
				BList new_items;
				const char* url;
				const char* name;
				if( msg->FindString("url", &url) != B_OK || !url || !*url ) {
					url = 0;
				}
				if( msg->FindString("title", &name) != B_OK || !name || !*name ) {
					name = url ? url : "New Bookmark";
				}
				if (!url) url = "http://";
				BString buf(name);
				if( fList && fList->CurrentFolder() ) {
					fList->CurrentFolder()->MakeUniqueName(&buf);
				}
				TBookmarkItem* item = new TBookmarkItem(buf.String(), url);
				new_items.AddItem(item);
				fList->AddBookmarks(&new_items);
				fList->SetCurrentEdit(item);
			}
		} break;
		
		case DELETE_MSG: {
			if( fList ) {
				BList rem_items;
				int32 pos = fList->RemoveSelected(&rem_items);
				for( int32 i=0; i<rem_items.CountItems(); i++ ) {
					delete (TBookmarkItem*)rem_items.ItemAt(i);
				}
				if( pos >= 0 ) fList->Select(pos);
			}
		} break;
		
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void TBookmarkEditor::FrameMoved(BPoint new_position)
{
	inherited::FrameMoved(new_position);
	
	if( Window() ) {
		Window()->DisableUpdates();
		Window()->BeginViewTransaction();
	}
	
	UpdateBackgrounds();
	
	if( Window() ) {
		Window()->EndViewTransaction();
		Window()->EnableUpdates();
	}
}

void TBookmarkEditor::FrameResized(float width, float height)
{
	inherited::FrameResized(width, height);
	LayoutViews(width, height);
}

static const float SPACER = 6;

void TBookmarkEditor::GetPreferredSize(float* width, float* height)
{
	*width = *height = 0;
	
	if( fList ) {
		float w = fList->StringWidth("WW")*6 + B_V_SCROLL_BAR_WIDTH;
		if( w > *width ) *width = w;
		font_height fh;
		fList->GetFontHeight(&fh);
		*height += SPACER + (fh.ascent+fh.descent+fh.leading)*5
				+ B_H_SCROLL_BAR_HEIGHT;
	}
}

void TBookmarkEditor::SetViewColor(rgb_color color)
{
	inherited::SetViewColor(color);
	fHaveViewColor = true;
}

void TBookmarkEditor::SetHighColor(rgb_color color)
{
	inherited::SetHighColor(color);
	fHaveHighColor = true;
}

void TBookmarkEditor::OpenURL(TBookmarkItem* /*it*/)
{
}

void TBookmarkEditor::LayoutViews(float width, float height)
{
	if( Window() ) {
		Window()->DisableUpdates();
		Window()->BeginViewTransaction();
	}
	
	if( fScroller ) {
		fScroller->ResizeTo(width, height);
		fScroller->MoveTo(0, 0);
	}
	
	UpdateBackgrounds();

	if( Window() ) {
		Window()->EndViewTransaction();
		Window()->EnableUpdates();
	}
}

void TBookmarkEditor::UpdateBackgrounds()
{
	if( !Window() ) return;
	
	SetOffsetBitmap(this, fBackground);
	if( fScroller ) SetOffsetBitmap(fScroller, fBackground);
}

void TBookmarkEditor::UpdateControlState()
{
}

void TBookmarkEditor::UpdateBookmarks()
{
	if( fBookmarks && fBookmarks->IsDirty() ) {
		// If bookmarks have changed...
		fBookmarks->Write();
		Invoke();
	}
}

// --------------- Local Functions -- To Avoid Being Inlined ---------------

void SetOffsetBitmap(BView* view, const BBitmap* bitmap)
{
	if( bitmap ) {
		BRect src(bitmap->Bounds());
		BRect dest(src);
		BView* v = view;
		while( v ) {
			dest.OffsetBy(-v->Frame().left, -v->Frame().top);
			v = v->Parent();
		}
		view->SetViewBitmap(bitmap, src, dest, B_FOLLOW_NONE);
	}
}

void SetViewAttributes(BView* view, const BFont* font,
						rgb_color background, rgb_color foreground)
{
	view->SetFont(font);
	view->SetViewColor(background);
	view->SetLowColor(background);
	view->SetHighColor(foreground);
}
