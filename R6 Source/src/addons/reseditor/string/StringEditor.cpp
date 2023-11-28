/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <ResourceEditor.h>

#include <Invoker.h>
#include <TextView.h>
#include <ScrollView.h>
#include <Window.h>
#include <Mime.h>
#include <Autolock.h>
#include <Debug.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <malloc.h>

// -----------------------------------------------------------------------------

// Support tools for string editing

static inline int8 hexval(char c)
{
	if( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
	else if( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
	else if( c >= '0' && c <= '9' ) return c - '0';
	return 0;
}

static size_t ExpandRawText(char* out, const void* data, size_t size,
							bool one_line = false)
{
	const char* in = (const char*)data;
	const char* end = in + size;
	size_t amount = 0;
	while( in < end ) {
		if( (*in >= ' ' && *in < 127 && *in != '\\')
			|| (*in == '\n' && !one_line) || (*in == '\t' && !one_line)
			|| (in == (end-1) && !*in) ) {
			if( out ) *out++ = *in;
			amount++;
		// Unicode
		} else if( ((*in)&0xE0) == 0xC0 && ((*(in+1))&0xC0) == 0x80 ) {
			if( out ) {
				*out++ = *in++;
				*out++ = *in;
			} else in += 1;
			amount += 2;
		} else if( ((*in)&0xF0) == 0xE0 && ((*(in+1))&0xC0) == 0x80
					&& ((*(in+2))&0xC0) == 0x80 ) {
			if( out ) {
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in;
			} else in += 2;
			amount += 3;
		// Control characters
		} else {
			char rep = 0;
			switch( *in ) {
				case '\\':		rep = '\\';		break;
				case '\n':		rep = 'n';		break;
				case '\r':		rep = 'r';		break;
				case '\b':		rep = 'b';		break;
				case '\t':		rep = 't';		break;
				case 0:			rep = '@';		break;
			}
			
			if( rep ) {
				if( out ) {
					*out++ = '\\';
					*out++ = rep;
				}
				amount += 2;
			} else {
				if( out ) {
					*out++ = '\\';
					*out++ = 'x';
					*out++ = "0123456789ABCDEF"[((*in)>>4)&0xF];
					*out++ = "0123456789ABCDEF"[(*in)&0xF];
				}
				amount += 4;
			}
		}
		in++;
	}
	
	return amount;
}

static size_t ContractRawText(void* out, const char* text)
{
	char* buf = (char*)out;
	size_t amount = 0;
	while( *text ) {
		if( *text != '\\' ) {
			if( buf ) *buf++ = *text;
			text++;
			amount++;
		} else {
			text++;
			char c = 0;
			switch( *text ) {
				case '\\':		c = '\\';		break;
				case 'n':		c = '\n';		break;
				case 'r':		c = '\r';		break;
				case 'b':		c = '\b';		break;
				case 't':		c = '\t';		break;
				case '@':		c = 0;			break;
				case 'x': {
					text++;
					if( *text ) {
						c = hexval(*text);
						text++;
						if( *text ) {
							c = (c<<4) | hexval(*text);
						}
					}
				} break;
				default:		c = *text;		break;
			};
			if( buf ) *buf++ = c;
			text++;
			amount++;
		}
	}
	
	if( buf ) *buf = 0;
	return amount+1;
}

// Stolen from BResourceParser.  (Just so we don't have to link with
// the resource parser library.)

static bool LooksLikeString(const void* data, size_t size, bool* out_isArray = 0)
{
	if( size <= 0 ) return false;
	
	// Does this look like a string?
	const uint8* c = (const uint8*)data;
	const uint8* e = c+size;
	bool isString = c[size-1] == 0 ? true : false;
	bool isArray = false;
	int32 quality = 1;
	while( c < e && isString ) {
		if( *c == 0 ) {
			isArray = true;
			quality--;
		} else if( *c >= ' ' && *c < 127 ) {
			quality++;
		} else if( ((*c)&0xE0) == 0xC0 ) {
			c++;
			if( ((*c)&0xC0) == 0x80 ) {
				c++;
				quality += 2;
			}
		} else if( ((*c)&0xF0) == 0xE0 ) {
			c++;
			if( ((*c)&0xC0) == 0x80 ) {
				c++;
				if( ((*c)&0xC0) == 0x80 ) {
					c++;
					quality += 3;
				}
			}
		} else if( *c != '\n' && *c != '\t' ) {
			isString = false;
		}
		c++;
	}
	
	if( out_isArray ) *out_isArray = isArray;
	
	if( quality < 0 ) return false;
	return isString;
}

// -----------------------------------------------------------------------------

// The string resource mini editor.

class StringMiniEditor : public BMiniItemEditor
{
public:
	StringMiniEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
	 : BMiniItemEditor(args, primaryItem)
	{
	}

	~StringMiniEditor()
	{
	}
	
	// String data can be edited in the standard text editor, so there
	// is no need to implement MakeEditView().  Instead, this function
	// returns a text string to edit in-place, and the following function
	// copies an edited text string back into the resource.
	
	virtual status_t ReadData(BString* out) const
	{
		status_t err = B_ERROR;
		
		// Lock resources for reading.
		const BResourceCollection* c = ReadLock();
		if( c ) {
		
			// Retrieve resource data that our editor is looking at.
			const BResourceItem* it = c->ReadItem(PrimaryItem());
			if( it ) {
				// Convert data into a legit C string and write into
				// buffer.
				size_t len = ExpandRawText(0, it->Data(), it->Size(), true);
				if (len > 0) {
					char* str = out->LockBuffer(len);
					if( str ) {
						ExpandRawText(str, it->Data(), it->Size(), true);
						err = B_OK;
					}
					out->UnlockBuffer(len);
				} else {
					*out = "";
				}
			}
			
			// Done reading data.
			ReadUnlock(c);
		}
		
		return err;
	}
	
	virtual status_t WriteData(const char* in)
	{
		status_t err = B_ERROR;
		
		// Lock resources for writing.  This action is shown as
		// "Edit Text" to the user.
		BResourceCollection* c = WriteLock("Edit Text");
		if( c ) {
		
			// Retrieve resource data that our editor is looking at.
			BResourceItem* it = c->WriteItem(PrimaryItem());
			if( it ) {
				// Convert plain C string back into raw resource data
				// and write back.
				size_t len = ContractRawText(0, in);
				void* buf = malloc(len);
				len = ContractRawText(buf, in);
				it->SetData(buf, len);
				free(buf);
			}
			
			// Done writing data.
			WriteUnlock(c);
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
		ReadData(out);
	}
	
private:
};

// -----------------------------------------------------------------------------

// String resource full editor.

// First is a special BTextView that the editor uses, which watches user
// interaction and reports changes to the editor at key times.

class myTextView : public BTextView, public BInvoker
{
public:
	myTextView(BRect frame, const char* name, BMessage* mod, uint32 resizeMask)
		: BTextView(frame, name, frame, resizeMask,
					B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED | B_NAVIGABLE),
		  BInvoker(mod, 0, 0),
		  fModMessage(0), fSupressInvoke(false),
		  fBatchChanged(false), fHasChanged(false),
		  fInserted(false), fDeleted(false)
	{
		frame.OffsetTo(B_ORIGIN);
		frame.InsetBy(3.0, 3.0);
		SetTextRect(frame);
		SetStylable(false);
		MakeSelectable();
		MakeEditable();
		SetWordWrap(true);
	}
	
	~myTextView()
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(Parent()->ViewColor());
	}

	void ReportBatchChange()
	{
		if( fBatchChanged ) {
			fBatchChanged = false;
			fInserted = fDeleted = false;
			InvokeNotify(Message(), B_CONTROL_INVOKED);
		}
	}
	
	void ReportIfNeeded()
	{
		if( fInserted && fDeleted ) {
			ReportBatchChange();
		}
	}
	
	void MakeFocus(bool focusState = true)
	{
		ReportBatchChange();
		BTextView::MakeFocus(focusState);
	}
	
	void MessageReceived(BMessage *msg)
	{
		switch(msg->what)
		{
			case B_CLEAR: {
				Clear();
				ReportBatchChange();
			} break;

			case B_CUT:
			case B_PASTE: {
				BTextView::MessageReceived(msg);
				ReportBatchChange();
			} break;
			
			default :
				BTextView::MessageReceived(msg);
				ReportIfNeeded();
				break;
		}
	}

	virtual	void KeyDown(const char *bytes, int32 numBytes)
	{
		bool batchChange = false;
		
		if( numBytes == 1 ) {
			switch( *bytes ) {
				case B_RETURN:
				case B_DOWN_ARROW:
				case B_UP_ARROW:
				case B_LEFT_ARROW:
				case B_RIGHT_ARROW:
				case B_ESCAPE:
				case B_HOME:
				case B_END:
				case B_PAGE_DOWN:
				case B_PAGE_UP:
				case B_FUNCTION_KEY:
					batchChange = true;
					break;
			}
		}
		
		BTextView::KeyDown(bytes, numBytes);
		
		if( batchChange ) ReportBatchChange();
		else ReportIfNeeded();
	}
	
	void SetModicationMessage(BMessage* msg)
	{
		delete fModMessage;
		fModMessage = msg;
	}
	
	BMessage* ModificationMessage() const
	{
		return fModMessage;
	}
	
	void SetRawText(const void* data, size_t size)
	{
		size_t len = ExpandRawText(0, data, size);
		char* buf = (char*)malloc(len+1);
		ExpandRawText(buf, data, size);
		buf[len] = 0;
		int32 start, end;
		GetSelection(&start, &end);
		SetText(buf);
		Select(start, end);
		ScrollToSelection();
		free(buf);
	}
	
	void* GetRawText(size_t* size) const
	{
		size_t len = ContractRawText(0, Text());
		void* buf = malloc(len);
		len = ContractRawText(buf, Text());
		if( size ) *size = len;
		return buf;
	}
	
	void SupressInvoke(bool state)
	{
		fSupressInvoke = state;
	}
	
	bool HasChanged()
	{
		bool changed = fHasChanged;
		fHasChanged = false;
		return changed;
	}
	
	virtual	void FrameResized(float width, float height)
	{
		BTextView::FrameResized(width, height);
		BRect frame(Bounds());
		frame.InsetBy(3.0, 3.0);
		SetTextRect(frame);
	}
	
protected:
	virtual	void InsertText(const char* inText, int32 inLength, 
							int32 inOffset, const text_run_array* inRuns)
	{
		BTextView::InsertText(inText, inLength, inOffset, inRuns);
		if( !fSupressInvoke ) {
			fHasChanged = fBatchChanged = true;
			fInserted = true;
			if( ModificationMessage() ) {
				InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
			}
		}
	}
	
	virtual	void DeleteText(int32 fromOffset, int32 toOffset)
	{
		BTextView::DeleteText(fromOffset, toOffset);
		if( !fSupressInvoke ) {
			fHasChanged = fBatchChanged = true;
			fDeleted = true;
			if( ModificationMessage() ) {
				InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
			}
		}
	}

private:
	BMessage* fModMessage;
	bool fSupressInvoke;
	bool fBatchChanged;
	bool fHasChanged;
	bool fInserted;
	bool fDeleted;
};

// This is the full string resource editor.  It is a mix-in of
// a BFullItemEditor and BView, so that we can do all the needed
// subclassing in one place.

class StringFullEditor : public BFullItemEditor, public BView
{
public:
	StringFullEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
		: BFullItemEditor(args, primaryItem, this),
		  BView(BRect(0, 0, 50, 50), "StringEditor",
				B_FOLLOW_ALL_SIDES, B_NAVIGABLE_JUMP)
	{
		// Create text editor view.
		
		BRect frame(Bounds());
		frame.left += 2;
		frame.top += 2;
		frame.right -= B_V_SCROLL_BAR_WIDTH + 2;
		frame.bottom -= B_H_SCROLL_BAR_HEIGHT + 2;
		fTextView = new myTextView(frame, "StringTextView",
								   new BMessage('text'), B_FOLLOW_ALL_SIDES);
		BScrollView* s;
		s = new BScrollView("StringScroll", fTextView,
							B_FOLLOW_ALL_SIDES, B_WILL_DRAW,
							true, true, B_FANCY_BORDER);
		AddChild(s);
		
		// Copy current resource data into text editor.
		
		const BResourceCollection* c = ReadLock();
		if( c ) {
			const BResourceItem* it = c->ReadItem(PrimaryItem());
			fTextView->SupressInvoke(true);
			fTextView->SetRawText(it->Data(), it->Size());
			fTextView->SupressInvoke(false);
			ReadUnlock(c);
		}
	}

	~StringFullEditor()
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
		if( fTextView ) fTextView->SetTarget(this);
		SetViewColor(Parent()->ViewColor());
	}

	void DetachedFromWindow()
	{
		// Before detaching from window, make sure that any changes the
		// user made are placed back into the resource.
		UpdateText();
		BView::DetachedFromWindow();
	}
	
	void GetPreferredSize(float *width, float *height)
	{
		font_height fh;
		GetFontHeight(&fh);
		*width = B_V_SCROLL_BAR_WIDTH + 2 + StringWidth("WW")*10;
		*height = B_H_SCROLL_BAR_HEIGHT + 2 + (fh.ascent+fh.descent+fh.leading)*5;
	}
	
	void MakeFocus(bool on)
	{
		if( on && fTextView ) fTextView->MakeFocus(on);
		else if( on != IsFocus() ) BView::MakeFocus(on);
	}

	// Copy current text data (if it has changed) into the resource
	// item being edited.
	
	void UpdateText()
	{
		if( fTextView && fTextView->HasChanged() ) {
		
			// Lock resources for writing.  This action is shown
			// to the user as "Edit Text".
			BResourceCollection* c = WriteLock("Edit Text");
			
			if( c ) {
				// Retrieve the data from the text view, and copy it
				// into the resource being edited.
				size_t size = 0;
				void* buf = fTextView->GetRawText(&size);
				if( buf ) {
					BResourceItem* it = c->WriteItem(PrimaryItem(), this);
					it->SetData(buf, size);
					free(buf);
				}
				
				// Finished writing data.
				WriteUnlock(c);
			}
		}
	}
	
	void MessageReceived(BMessage *msg)
	{
		switch(msg->what)
		{
			case 'text': {
				// Text editor reported a change, handle it.
				UpdateText();
			} break;

			case B_RESOURCE_DATA_CHANGED: {
				const BResourceCollection* c = ReadLock();
				if( c ) {
					BResourceHandle h;
					uint32 changes;
					while( (h=c->GetNextChange(this, &changes)).IsValid() ) {
						PRINT(("Executing object %p change %lx\n",
								c->ReadItem(h), changes));
						ExecDataChanged(c, h, changes);
					}
					ReadUnlock(c);
				}
			} break;
			
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
		if( !fTextView ) return;
		
		PRINT(("ExecDataChanged(): item=%p, PrimaryItem()=%p\n",
				c->ReadItem(item), c->ReadItem(PrimaryItem())));
		if( item != PrimaryItem() || changes == 0 ) return;
		
		if( Window() && !Window()->IsLocked() ) {
			debugger("ExecDataChanged() called without window locked");
			return;
		}
		
		// Retrieve resource item, and copy it into the text editor
		// if its data has changed.
		const BResourceItem* it = c->ReadItem(PrimaryItem());
		PRINT(("Doing change %lx (data=%lx)\n", changes, B_RES_DATA_CHANGED));
		if( (changes&B_RES_DATA_CHANGED) != 0 ) {
			fTextView->SupressInvoke(true);
			PRINT(("Setting data to %s\n", (const char*)it->Data()));
			fTextView->SetRawText(it->Data(), it->Size());
			fTextView->SupressInvoke(false);
		}
	}

	// Return the view for this editor.  Which is just myself.
	
	virtual BView* View()
	{
		return this;
	}

private:
	myTextView* fTextView;
};

// -----------------------------------------------------------------------------

// This is the top-level string editor add-on.

struct generate_info {
	const char* name;
	type_code type;
};

static generate_info generators[] = {
	{ "String", B_STRING_TYPE },
	{ "MIME String", B_MIME_STRING_TYPE },
};

#define NUM_GENERATORS (sizeof(generators) / sizeof(generate_info))

class StringAddon : public BResourceAddon
{
public:
	StringAddon(const BResourceAddonArgs& args)
		: BResourceAddon(args)
	{
	}
	
	~StringAddon()
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
			case B_STRING_TYPE:
				err = c->AddItem(out_item, B_STRING_TYPE, id, name,
						 		 "", 1, make_selected, resol);
				break;
						 
			case B_MIME_STRING_TYPE:
				err = c->AddItem(out_item, B_MIME_STRING_TYPE, id, name,
						 		 "application/octet-stream", 25, make_selected, resol);
				break;
			
			default:
				err = B_BAD_TYPE;
				break;
		}
					 
		WriteUnlock(c);
		return err;
	}
	
	// Return the quality of editor we think we are for this data,
	// as fast as possible.
	virtual float QuickQuality(const BResourceItem* item) const
	{
		if( item->Type() == B_STRING_TYPE ) return 0.5;
		if( item->Type() == B_MIME_STRING_TYPE ) return 0.3;
		return -1;
	}
	
	// Return the quality of editor we think we are for this data,
	// as accurate as possible.
	virtual float PreciseQuality(const BResourceItem* item) const
	{
		float q = QuickQuality(item);
		if( q >= 0 ) return q;
		if( LooksLikeString(item->Data(), item->Size()) ) {
			return 0.4;
		}
		return -1;
	}
	
	// Return a mini editor for string data.
	virtual BMiniItemEditor* MakeMiniEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		return new StringMiniEditor(args, primaryItem);
	}
	
	// Return a full editor for string data.
	virtual BFullItemEditor* MakeFullEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		return new StringFullEditor(args, primaryItem);
	}
	
	virtual float PasteQuality(const BMessage* data) const
	{
		return DropQuality(data);
	}
	
	virtual status_t HandlePaste(const BMessage* data)
	{
		return HandleDrop(data);
	}
	
	virtual float DropQuality(const BMessage* drop) const
	{
		const void* data = 0;
		ssize_t size = 0;
		if( drop->FindData("text/plain", B_MIME_TYPE, &data, &size) == B_OK ) {
			// This is what we were made for.
			return 0.8;
		}
		
		const int32 N = drop->CountNames(B_MIME_TYPE);
		for( int32 i=0; i<N; i++ ) {
#if B_BEOS_VERSION_DANO
			const char* name;
#else
			char* name;
#endif
			type_code retType;
			if( drop->GetInfo(B_MIME_TYPE, i, &name, &retType) == B_OK ) {
				if( strcmp(name, "text") == B_OK ||
						strncmp(name, "text/", 5) == B_OK ) {
					// We can at least handle this.
					return 0.6;
				}
			}
		}
		
		return -1;
	}
	
	virtual status_t HandleDrop(const BMessage* drop)
	{
		status_t err = B_BAD_VALUE;
		
		const void* data = 0;
		ssize_t size = 0;
		if( drop->FindData("text/plain", B_MIME_TYPE, &data, &size) != B_OK ) {
		
			const int32 N = drop->CountNames(B_MIME_TYPE);
			data = NULL;
			for( int32 i=0; !data && i<N; i++ ) {
#if B_BEOS_VERSION_DANO
				const char* name;
#else
				char* name;
#endif
				type_code retType;
				if( drop->GetInfo(B_MIME_TYPE, i, &name, &retType) == B_OK ) {
					if( strcmp(name, "text") == B_OK ||
							strncmp(name, "text/", 5) == B_OK ) {
						if( drop->FindData(name, B_MIME_TYPE, &data, &size) != B_OK ) {
							data = NULL;
						}
					}
				}
			}
		}
		
		BResourceCollection* c;
		if( data && (c=WriteLock()) != 0 ) {
			int32 iconid = c->UniqueIDForType(B_STRING_TYPE, 1);
			BResourceHandle h;
			c->AddItem(&h, B_STRING_TYPE, iconid, "New String",
						data, size, true, c->B_RENAME_NEW_ITEM);
			WriteUnlock(c);
			err = B_OK;
		}
		
		return err;
	}
};

extern "C" BResourceAddon* make_nth_resourcer(int32 n, image_id /*you*/, const BResourceAddonArgs& args, uint32 /*flags*/, ...)
{
	if( n == 0 ) return new StringAddon(args);
	return 0;
}
