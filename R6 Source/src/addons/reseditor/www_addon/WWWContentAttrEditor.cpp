/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <ResourceEditor.h>

#include <TextControl.h>
#include <Window.h>
#include <Autolock.h>
#include <DataIO.h>
#include <Debug.h>
#include <TypeConstants.h>
#include <ListView.h>
#include <ListItem.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <Button.h>
#include <Roster.h>
#include <stdio.h>

#define S_CONTENT_MIME_TYPES 			"mime_types"
#define S_CONTENT_EXTENSIONS 			"extensions"
#define S_CONTENT_PLUGIN_IDS 			"plugin_ids"
#define S_CONTENT_PLUGIN_DESCRIPTION 	"plugin_desc"

// -----------------------------------------------------------------------------

static bool compare_messages(const BMessage& m1, const BMessage& m2, bool recurse)
{
	type_code type;
#if B_BEOS_VERSION_DANO
	const char* name;
#else
	char* name;
#endif
	int32 c;
	int32 i=0;
	if (m1.what != m2.what) return false;
	while (m1.GetInfo(B_ANY_TYPE, i++, &name, &type, &c) == B_OK) {
		for (int32 j=0; j<c; j++) {
			if (type == B_RAW_TYPE && recurse) {
				BMessage s1;
				BMessage s2;
				if (m1.FindMessage(name, j, &s1) < B_OK)
					return false;
				if (m2.FindMessage(name, j, &s2) < B_OK)
					return false;
				if (!compare_messages(s1, s2, recurse))
					return false;
			} else {
				const void* d1;
				const void* d2;
				ssize_t s1;
				ssize_t s2;
				if (m1.FindData(name, type, j, &d1, &s1) < B_OK)
					return false;
				if (m2.FindData(name, type, j, &d2, &s2) < B_OK)
					return false;
				if (s1 != s2)
					return false;
				if (memcmp(d1, d2, s1) != 0)
					return false;
			}
		}
	}
	return true;
}

// -----------------------------------------------------------------------------

// The application flags resource mini editor.

#if 0
class AttrMiniEditor : public BMiniItemEditor
{
public:
	AttrMiniEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
	 : BMiniItemEditor(args, primaryItem)
	{
	}

	~AttrMiniEditor()
	{
	}
	
	// For now, only display flags.  In the future we want to allow the
	// user to edit them here, as well.
	
	// Convert flags into a string to display to user.
	virtual status_t ReadDataPrivate(BString* out) const
	{
		status_t err = B_ERROR;
		
		// Lock resources for reading.
		const BResourceCollection* c = ReadLock();
		if( c ) {
		
			// Retrieve resource data that our editor is looking at.
			const BResourceItem* it = c->ReadItem(PrimaryItem());
			if( it ) {
				if( it->Size() >= sizeof(uint32) ) {
					uint32 flags = *(const uint32*)it->Data();
					*out = "";
					switch( flags&B_LAUNCH_MASK ) {
						case B_SINGLE_LAUNCH:		*out << "Single";		break;
						case B_MULTIPLE_LAUNCH:		*out << "Multiple";		break;
						case B_EXCLUSIVE_LAUNCH:	*out << "Exclusive";	break;
					}

					if( flags&B_ARGV_ONLY ) {
						if( out->Length() > 0 ) *out << ", ";
						*out << "Argv Only";
					}
					
					if( flags&B_BACKGROUND_APP ) {
						if( out->Length() > 0 ) *out << ", ";
						*out << "Background";
					}
					
					err = B_OK;
				}
			}
			
			// Done reading data.
			ReadUnlock(c);
		}
		
		return err;
	}
	
	// Draw this data value.  Just draw a standard text line, cacheing
	// computed data.
	virtual void DrawData(BView* into, BRect frame, float baseline) const
	{
		DrawText(into, frame, baseline, 0, B_TRUNCATE_MIDDLE);
	}
	
	// Compute data to draw in DrawText().
	virtual void MakeText(BString* out) const
	{
		ReadDataPrivate(out);
	}
	
private:
};
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// Attr resource full editor.

// This is the full resource editor.  It is a mix-in of
// a BFullItemEditor and BView, so that we can do all the needed
// subclassing in one place.

enum {
	SPACE = 5,
	GAP = 5,
	FRAME = 3
};

static int sort_func(const void* v1, const void* v2)
{
	//printf("Comparing %p with %p\n", v1, v2);
	const char* n1 = (*(BStringItem**)v1)->Text();
	const char* n2 = (*(BStringItem**)v2)->Text();
	//printf("Names %s with %s\n", n1, n2);
	if (n1 == n2) return 0;
	if (n1 == NULL) return -1;
	if (n2 == NULL) return 2;
	return strcmp(n1, n2);
}

class StringListControl : public BControl
{
public:
	StringListControl(BRect frame, const char* name, const char* label,
						const char* proto, BMessage* report, uint32 resizeMask,
						uint32 flags = B_NAVIGABLE_JUMP|B_WILL_DRAW|B_FRAME_EVENTS)
		:	BControl(frame, name, label, report, resizeMask, flags),
			fPrototype(proto), fCurrent(NULL), fValidSize(false)
	{
		BRect bogusRect(-100, -100, -10, -10);
		BRect innerRect(bogusRect.left+2, bogusRect.top+2,
						bogusRect.right-B_V_SCROLL_BAR_WIDTH-6, bogusRect.bottom-2);
		fList = new BListView(innerRect, "List", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
		fList->SetSelectionMessage(new BMessage('sele'));
		fScroll = new BScrollView("Scroll", fList, B_FOLLOW_NONE, B_WILL_DRAW,
									false, true);
		AddChild(fScroll);
		AddChild(fText = new BTextControl(bogusRect, "Text", "", "",
										  new BMessage('text'), B_FOLLOW_NONE));
		fText->SetDivider(-1);
		AddChild(fAdd = new BButton(bogusRect, "Add", "Add", new BMessage('add '),
									B_FOLLOW_NONE));
		AddChild(fRem = new BButton(bogusRect, "Rem", "Rem", new BMessage('rem '),
									B_FOLLOW_NONE));
	}

	~StringListControl()
	{
		EmptyList();
	}
	
	virtual void AttachedToWindow()
	{
		fValidSize = false;
		BMessenger m(this);
		fList->SetTarget(m);
		fText->SetTarget(m);
		fAdd->SetTarget(m);
		fRem->SetTarget(m);
		if( Parent() ) SetViewColor(Parent()->ViewColor());
		UpdateState();
		LayoutViews();
	}
	
	virtual void FrameResized(float , float )
	{
		LayoutViews(true);
	}
	
	virtual void MessageReceived(BMessage* msg)
	{
		switch (msg->what) {
			case 'sele':
				fCurrent = dynamic_cast<BStringItem*>(fList->ItemAt(fList->CurrentSelection()));
				UpdateState();
				break;
			case 'add ':
				fCurrent = new BStringItem(fPrototype.String());
				fItems.AddItem(fCurrent);
				fList->AddItem(fCurrent);
				fList->SortItems(sort_func);
				fList->Select(fList->IndexOf(fCurrent));
				UpdateState();
				Invoke();
				break;
			case 'rem ':
				if (fCurrent) {
					int32 pos = fList->IndexOf(fCurrent);
					fList->RemoveItem(fCurrent);
					fItems.RemoveItem(fCurrent);
					delete fCurrent;
					fCurrent = NULL;
					if (pos < 0) pos = 0;
					else if (pos >= fList->CountItems()) pos = fList->CountItems()-1;
					fList->Select(pos);
					fCurrent = dynamic_cast<BStringItem*>(fList->ItemAt(fList->CurrentSelection()));
					UpdateState();
					Invoke();
				}
				break;
			case 'text':
				UpdateValue();
				break;
				
			default:
				BControl::MessageReceived(msg);
		}
	}
	
	virtual void Draw(BRect)
	{
		font_height fh;
		GetFontHeight(&fh);
		DrawString(Label(), BPoint(0, fh.ascent));
	}
	
	void GetPreferredSize(float *width, float *height)
	{
		if (!fValidSize) LayoutViews(false);
		*width = fPrefWidth;
		*height = fPrefHeight;
	}
	
	void InstallData(const BMessage& src, const char* field)
	{
		EmptyList();
		
		type_code type;
		int32 N = 0;
		src.GetInfo(field, &type, &N);
		for (int32 i=0; i<N; i++) {
			const char* str;
			if (src.FindString(field, i, &str) == B_OK) {
				fItems.AddItem(new BStringItem(str));
			}
		}
		fList->AddList(&fItems);
		fList->SortItems(sort_func);
	}
	
	void RetrieveData(BMessage* target, const char* field)
	{
		target->RemoveName(field);
		const int32 N = fList->CountItems();
		for (int32 i=0; i<N; i++) {
			BStringItem* it = dynamic_cast<BStringItem*>(fList->ItemAt(i));
			if (it)
				target->AddString(field, it->Text());
		}
	}
	
	void UpdateValue()
	{
		if (fCurrent && strcmp(fCurrent->Text(), fText->Text()) != 0) {
			fCurrent->SetText(fText->Text());
			fList->SortItems(sort_func);
			fList->Select(fList->IndexOf(fCurrent));
			//fList->Invalidate(fList->ItemFrame(fList->IndexOf(fCurrent)));
			Invoke();
		}
	}
				
private:
	void LayoutViews(bool position=true)
	{
		float aw, ah, rw, rh, tw, th;
		fAdd->GetPreferredSize(&aw, &ah);
		fRem->GetPreferredSize(&rw, &rh);
		fText->GetPreferredSize(&tw, &th);
		aw = fAdd->StringWidth(fAdd->Label()) + 15;
		rw = fRem->StringWidth(fRem->Label()) + 15;
		
		float both = th;
		if (both < rh) both = rh;
		
		font_height fh;
		GetFontHeight(&fh);
		
		if (position) {
			if (Window()) Window()->BeginViewTransaction();
		
			const BRect b(Bounds());
			fRem->MoveTo(b.right-rw, b.bottom-rh - floor((both-rh)/2));
			fRem->ResizeTo(rw, rh);
			fAdd->MoveTo(b.right-rw-SPACE-aw, b.bottom-ah - floor((both-ah)/2));
			fAdd->ResizeTo(aw, ah);
			fText->MoveTo(b.left, b.bottom-th - floor((both-th)/2));
			fText->ResizeTo(b.right-rw-SPACE-aw-SPACE-b.left+1, th);
			
			const float stop = b.top+fh.ascent+fh.descent+SPACE;
			fScroll->MoveTo(b.left, stop);
			fScroll->ResizeTo(b.right-b.left, b.bottom-both-SPACE-stop+1);
			
			if (Window()) Window()->EndViewTransaction();
		}
		
		fValidSize = true;
		fPrefWidth = aw + rw + 100;
		fPrefHeight = fh.ascent+fh.descent+both+100;
	}
	
	void UpdateState()
	{
		if (fCurrent) {
			fRem->SetEnabled(true);
			fText->SetText(fCurrent->Text());
			fText->SetEnabled(true);
		} else {
			fRem->SetEnabled(false);
			fText->SetText("");
			fText->SetEnabled(false);
		}
	}
	
	void EmptyList()
	{
		fList->MakeEmpty();
		const int32 N = fItems.CountItems();
		for (int32 i=0; i<N; i++)
			delete (BListItem*)fItems.ItemAt(i);
		fItems.MakeEmpty();
		fCurrent = NULL;
	}
	
	BString			fPrototype;
	
	BList			fItems;
	
	BStringItem*	fCurrent;
	
	BListView*		fList;
	BScrollView*	fScroll;
	BTextControl*	fText;
	BButton*		fAdd;
	BButton*		fRem;
	
	bool			fValidSize;
	float			fPrefWidth, fPrefHeight;
};

// -----------------------------------------------------------------------------

class AttrFullEditor : public BFullItemEditor, public BView
{
public:
	AttrFullEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
		: BFullItemEditor(args, primaryItem, this),
		  BView(BRect(0, 0, 50, 50), "AttrEditor",
				B_FOLLOW_ALL_SIDES, B_NAVIGABLE_JUMP|B_FRAME_EVENTS),
		  fValidSize(false), fPrefWidth(0), fPrefHeight(0)
	{
		BRect bogusRect(-100, -100, 10, 10);
		AddChild(fMime = new StringListControl(bogusRect, "Mime", "MIME Types",
							"type/subtype", new BMessage('mime'), B_FOLLOW_NONE));
		AddChild(fExt = new StringListControl(bogusRect, "Ext", "Filename Extensions",
							".ext", new BMessage('ext '), B_FOLLOW_NONE));
		AddChild(fPluginID = new BTextControl(bogusRect, "PluginID", "ID: ", "",
										  new BMessage('plid'), B_FOLLOW_NONE));
		AddChild(fDescription = new BTextControl(bogusRect, "Description", "Description: ", "",
										  new BMessage('desc'), B_FOLLOW_NONE));
		
		const BResourceCollection* c = ReadLock();
		if( c ) {
			ExecDataChanged(c, PrimaryItem(), B_RES_ALL_CHANGED);
			ReadUnlock(c);
		}
	}

	~AttrFullEditor()
	{
	}
	
	virtual status_t Retarget(BResourceHandle new_item)
	{
		SetPrimaryItem(new_item);
		const BResourceCollection* c = ReadLock();
		if( c ) {
			ExecDataChanged(c, PrimaryItem(), B_RES_ALL_CHANGED);
			ReadUnlock(c);
		}
		return B_OK;
	}
	
	void AttachedToWindow()
	{
		BView::AttachedToWindow();
		fValidSize = false;
		LayoutViews(true);
		BMessenger m(this);
		fMime->SetTarget(m);
		fExt->SetTarget(m);
		fDescription->SetTarget(m);
		fPluginID->SetTarget(m);
		if( Parent() ) SetViewColor(Parent()->ViewColor());
	}

	void DetachedFromWindow()
	{
		// Before detaching from window, make sure that any changes the
		// user made are placed back into the resource.
		UpdateValue();
		BView::DetachedFromWindow();
	}
	
	void GetPreferredSize(float *width, float *height)
	{
		if (!fValidSize) LayoutViews(false);
		*width = fPrefWidth;
		*height = fPrefHeight;
	}
	
	virtual void FrameResized(float , float )
	{
		LayoutViews(true);
	}
	
	void MakeFocus(bool on)
	{
		if (on && fDescription) fDescription->MakeFocus(on);
		else if (on != IsFocus()) BView::MakeFocus(on);
	}

	// Copy current flags data (if it has changed) into the resource
	// item being edited.
	
	void UpdateValue(const char* name = "Change Content")
	{
		fMime->UpdateValue();
		fMime->RetrieveData(&fData, S_CONTENT_MIME_TYPES);
		fExt->UpdateValue();
		fExt->RetrieveData(&fData, S_CONTENT_EXTENSIONS);
		
		if (fData.ReplaceString(S_CONTENT_PLUGIN_IDS,
								fPluginID->Text()) != B_OK) {
			fData.AddString(S_CONTENT_PLUGIN_IDS,
							fPluginID->Text());
		}
		if (fData.ReplaceString(S_CONTENT_PLUGIN_DESCRIPTION,
								fDescription->Text()) != B_OK) {
			fData.AddString(S_CONTENT_PLUGIN_DESCRIPTION,
							fDescription->Text());
		}
				
		if (!compare_messages(fData, fOrigData, true)) {
			BResourceCollection* c = WriteLock(name);
			
			if (c) {
				BResourceItem* it = c->WriteItem(PrimaryItem(), this);
				if (it) {
					BMallocIO io;
					time_t time = 0;
					io.Write(&time, sizeof(time));
					fData.Flatten(&io);
					it->SetData(io.Buffer(), io.BufferLength());
					fOrigData = fData;
				}
				
				// Finished writing data.
				WriteUnlock(c);
			}
		}
	}
	
	void MessageReceived(BMessage *msg)
	{
		switch (msg->what)
		{
			case B_RESOURCE_DATA_CHANGED: {
				const BResourceCollection* c = ReadLock();
				if (c) {
					BResourceHandle h;
					uint32 changes;
					while ((h=c->GetNextChange(this, &changes)).IsValid()) {
						PRINT(("Executing object %p change %lx\n",
								c->ReadItem(h), changes));
						ExecDataChanged(c, h, changes);
					}
					ReadUnlock(c);
				}
			} break;
			
			case 'plid':
				UpdateValue("Change ID");
				break;

			case 'desc':
				UpdateValue("Change Description");
				break;

			case 'mime':
				UpdateValue("Change MIME Types");
				break;
				
			case 'ext ':
				UpdateValue("Change Extensions");
				break;
				
			default :
				BView::MessageReceived(msg);
				break;
		}
	}

	// This is called whenever something about a resource we are
	// subscribed to has changed.
	
	void ExecDataChanged(const BResourceCollection* c,
						 BResourceHandle& item, uint32 changes)
	{
		PRINT(("ExecDataChanged(): item=%p, PrimaryItem()=%p\n",
				c->ReadItem(item), c->ReadItem(PrimaryItem())));
		if (item != PrimaryItem() || changes == 0) return;
		
		if (Window() && !Window()->IsLocked()) {
			debugger("ExecDataChanged() called without window locked");
			return;
		}
		
		// Retrieve resource item, and copy it into the text editor
		// if its data has changed.
		const BResourceItem* it = c->ReadItem(PrimaryItem());
		PRINT(("Doing change %lx (data=%lx)\n", changes, B_RES_DATA_CHANGED));
		if ((changes&B_RES_DATA_CHANGED) != 0) {
			if (it->Size() > sizeof(time_t))
				fOrigData.Unflatten((const char*)it->Data()+sizeof(time_t));
			else
				fOrigData = BMessage();
			fData = fOrigData;
			const char* text = NULL;
			fData.FindString(S_CONTENT_PLUGIN_DESCRIPTION, &text);
			fDescription->SetText(text);
			fData.FindString(S_CONTENT_PLUGIN_IDS, &text);
			fPluginID->SetText(text);
			fMime->InstallData(fData, S_CONTENT_MIME_TYPES);
			fExt->InstallData(fData, S_CONTENT_EXTENSIONS);
		}
	}

	// Return the view for this editor.  Which is just myself.
	
	virtual BView* View()
	{
		return this;
	}

private:
	void LayoutViews(bool position=true)
	{
		float mw, mh, ew, eh, pw, ph, dw, dh;
		fMime->GetPreferredSize(&mw, &mh);
		fExt->GetPreferredSize(&ew, &eh);
		fPluginID->GetPreferredSize(&pw, &ph);
		fDescription->GetPreferredSize(&dw, &dh);
		
		if (position) {
			if (Window()) Window()->BeginViewTransaction();
		
			const BRect b(Bounds());
			fDescription->MoveTo(b.left+FRAME, b.bottom-FRAME-dh);
			fDescription->ResizeToPreferred();
			fDescription->ResizeTo(b.right-b.left+1-B_V_SCROLL_BAR_WIDTH-FRAME*2, dh);
			
			fPluginID->MoveTo(b.left+FRAME, b.bottom-FRAME-dh-ph-SPACE);
			fPluginID->ResizeToPreferred();
			fPluginID->ResizeTo(b.right-b.left+1-B_V_SCROLL_BAR_WIDTH-FRAME*2, ph);
			
			float middle = floor((b.right-b.left)/2);
			
			fMime->MoveTo(b.left+FRAME, b.top+FRAME);
			fMime->ResizeTo(middle, b.bottom-FRAME-dh-SPACE-ph-GAP-b.top-1);
			fExt->MoveTo(b.left+FRAME+middle+GAP, b.top+FRAME);
			fExt->ResizeTo(middle-FRAME*2-GAP-b.left,
							b.bottom-FRAME-dh-SPACE-ph-GAP-b.top-1);
			
			if (Window()) Window()->EndViewTransaction();
		}
		
		fValidSize = true;
		fPrefWidth = mw + ew + GAP + FRAME*2;
		fPrefHeight = mh + GAP + dh + SPACE + ph + FRAME*2;
	}
	
	BMessage			fData;
	BMessage			fOrigData;
	
	StringListControl*	fMime;
	StringListControl*	fExt;
	BTextControl*		fDescription;
	BTextControl*		fPluginID;

	bool				fValidSize;
	float				fPrefWidth, fPrefHeight;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// This is the top-level application flags editor add-on.

struct generate_info {
	const char* name;
	type_code type;
};

static generate_info generators[] = {
	{ "Wagner Add-On Attribute", B_RAW_TYPE },
};

#define NUM_GENERATORS (sizeof(generators) / sizeof(generate_info))

class AttrAddon : public BResourceAddon
{
public:
	AttrAddon(const BResourceAddonArgs& args)
		: BResourceAddon(args)
	{
	}
	
	~AttrAddon()
	{
	}
	
	virtual status_t GetNthGenerateInfo(int32 n,
										BMessage* out_info) const
	{
		if( n < 0 || (size_t)n >= NUM_GENERATORS ) return B_ERROR;
		out_info->AddString(B_GENERATE_NAME, generators[n].name);
		out_info->AddInt32(B_GENERATE_TYPE, generators[n].type);
		return B_OK;
	}
	
	virtual status_t GenerateResource(BResourceHandle* out_item,
									  const BMessage* info,
									  int32 id, const char* name,
									  bool make_selected = true,
									  BResourceCollection::conflict_resolution
									  resol = BResourceCollection::B_ASK_USER)
	{
		type_code type;
		status_t err = info->FindInt32(B_GENERATE_TYPE, (int32*)&type);
		if( err != B_OK ) return err;
		
		BResourceCollection* c = WriteLock();
		if( !c ) return B_ERROR;
		
		switch( type ) {
			case B_RAW_TYPE: {
				BMessage initial;
				initial.AddString(S_CONTENT_PLUGIN_DESCRIPTION, "");
				BMallocIO io;
				time_t time = 0;
				io.Write(&time, sizeof(time));
				initial.Flatten(&io);
				err = c->AddItem(out_item, type, id, name,
								 io.Buffer(), io.BufferLength(), make_selected, resol);
			} break;
			
			default:
				err = B_ERROR;
		}
						 
		WriteUnlock(c);
		return err;
	}
	
	// Return the quality of editor we think we are for this data,
	// as fast as possible.
	virtual float QuickQuality(const BResourceItem* item) const
	{
		if (item->Type() == B_RAW_TYPE && item->Size() > sizeof(time_t)) {
			BMessage msg;
			msg.Unflatten((const char*)item->Data()+sizeof(time_t));
			const int32 N = msg.CountNames(B_ANY_TYPE);
			bool bad = false, good = false;
			for (int32 i=0; i<N && !bad; i++) {
#if B_BEOS_VERSION_DANO
				const char* name;
#else
				char* name;
#endif
				type_code type;
				if (msg.GetInfo(B_ANY_TYPE, i, &name, &type) == B_OK) {
					if (strcmp(name, S_CONTENT_MIME_TYPES) == 0
							|| strcmp(name, S_CONTENT_EXTENSIONS) == 0
							|| strcmp(name, S_CONTENT_PLUGIN_IDS) == 0
							|| strcmp(name, S_CONTENT_PLUGIN_DESCRIPTION) == 0) {
						if (type == B_STRING_TYPE) good = true;
						else bad = true;
					} else {
						bad = true;
					}
				}
			}
			// We're specifically for editing this kind of data!
			return (!bad && good) ? 0.8 : -1;
		}
		return -1;
	}
	
	// Return the quality of editor we think we are for this data,
	// as accurate as possible.
	virtual float PreciseQuality(const BResourceItem* item) const
	{
		return QuickQuality(item);
	}
	
	// Return a mini editor for string data.
	virtual BMiniItemEditor* MakeMiniEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		//return new AttrMiniEditor(args, primaryItem);
		return NULL;
	}
	
	// Return a full editor for string data.
	virtual BFullItemEditor* MakeFullEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		return new AttrFullEditor(args, primaryItem);
	}
	
	// Ooops, not implemented yet.
	virtual status_t HandleDrop(const BMessage* /*drop*/)
	{
		return B_ERROR;
	}
};

extern "C" BResourceAddon* make_nth_resourcer(int32 n, image_id /*you*/, const BResourceAddonArgs& args, uint32 /*flags*/, ...)
{
	if( n == 0 ) return new AttrAddon(args);
	return 0;
}
