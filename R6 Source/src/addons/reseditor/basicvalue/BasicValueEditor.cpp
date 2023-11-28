/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "BasicValue.h"

#include <ResourceEditor.h>

#include <TextControl.h>
#include <Window.h>
#include <Autolock.h>
#include <Debug.h>
#include <TypeConstants.h>
#include <stdio.h>

// -----------------------------------------------------------------------------

// The basic value resource mini editor.

class BasicValueMiniEditor : public BMiniItemEditor
{
public:
	BasicValueMiniEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
	 : BMiniItemEditor(args, primaryItem)
	{
	}

	~BasicValueMiniEditor()
	{
	}
	
	virtual status_t ReadData(BString* out) const
	{
		status_t err = B_ERROR;
		
		// Lock resources for reading.
		const BResourceCollection* c = ReadLock();
		if( c ) {
		
			// Retrieve resource data that our editor is looking at.
			const BResourceItem* it = c->ReadItem(PrimaryItem());
			if( it ) {
				BasicValue value(it->Type(), it->Data(), it->Size());
				err = value.SetStringFromValue(out);
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
				BasicValue value(it->Type(), it->Data(), it->Size());
				err = value.SetValueFromString(in);
				if( err == B_OK ) {
					it->SetData(value.Value(), value.Size());
				}
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

// BasicValue resource full editor.

// This is the full number resource editor.  It is a mix-in of
// a BFullItemEditor and BView, so that we can do all the needed
// subclassing in one place.

class BasicValueFullEditor : public BFullItemEditor, public BView
{
public:
	BasicValueFullEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
		: BFullItemEditor(args, primaryItem, this),
		  BView(BRect(0, 0, 50, 50), "BasicValueEditor",
				B_FOLLOW_ALL_SIDES, B_NAVIGABLE_JUMP)
	{
		fTextControl = new BTextControl(Bounds(), "BasicValueEntry",
										"", "", new BMessage('upvl'),
										B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
		AddChild(fTextControl);
		fTextControl->ResizeToPreferred();
		fTextControl->ResizeTo(Bounds().Width(), fTextControl->Bounds().Height());
		
		const BResourceCollection* c = ReadLock();
		if( c ) {
			ExecDataChanged(c, PrimaryItem(), B_RES_ALL_CHANGED);
			ReadUnlock(c);
		}
	}

	~BasicValueFullEditor()
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
		if( fTextControl ) fTextControl->SetTarget(this);
		SetViewColor(Parent()->ViewColor());
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
		fTextControl->GetPreferredSize(width, height);
		float w = StringWidth("WWW")*8;
		if( w > *width ) *width = w;
	}
	
	void MakeFocus(bool on)
	{
		if( on && fTextControl ) fTextControl->MakeFocus(on);
		else if( on != IsFocus() ) BView::MakeFocus(on);
	}

	// Copy current text data (if it has changed) into the resource
	// item being edited.
	
	void UpdateValue()
	{
		if( !fTextControl ) return;
		BasicValue newValue;
		status_t err = newValue.SetValueFromString(fTextControl->Text());
		if( err != B_OK ) return;
		
		if( newValue != fValue ) {
			fValue = newValue;
			
			BResourceCollection* c = WriteLock("Change Value");
			
			if( c ) {
				BResourceItem* it = c->WriteItem(PrimaryItem(), this);
				if( it ) {
					it->SetData(fValue.Value(), fValue.Size());
					if( it->Type() != fValue.Type() ) {
						it->SetType(fValue.Type());
					}
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
			case 'upvl': {
				// Text editor reported a change, handle it.
				UpdateValue();
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
		if( !fTextControl ) return;
		
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
			fValue.SetValue(it->Type(), it->Data(), it->Size());
			BString str;
			fValue.SetStringFromValue(&str);
			fTextControl->SetText(str.String());
		}
	}

	// Return the view for this editor.  Which is just myself.
	
	virtual BView* View()
	{
		return this;
	}

private:
	BasicValue fValue;
	BTextControl* fTextControl;
};

// -----------------------------------------------------------------------------

// This is the top-level basic value editor add-on.

struct generate_info {
	const char* name;
	type_code type;
};

static generate_info generators[] = {
	{ "Boolean", B_BOOL_TYPE },
	{ "Character", B_CHAR_TYPE },
	{ "Integer: 8 Bit Signed", B_INT8_TYPE },
	{ "Integer: 16 Bit Signed", B_INT16_TYPE },
	{ "Integer: 32 Bit Signed", B_INT32_TYPE },
	{ "Integer: 64 Bit Signed", B_INT64_TYPE },
	{ "Integer: 8 Bit Unsigned", B_UINT8_TYPE },
	{ "Integer: 16 Bit Unsigned", B_UINT16_TYPE },
	{ "Integer: 32 Bit Unsigned", B_UINT32_TYPE },
	{ "Integer: 64 Bit Unsigned", B_UINT64_TYPE },
	{ "Floating Point: 4 Bytes", B_FLOAT_TYPE },
	{ "Floating Point: 8 Bytes", B_DOUBLE_TYPE },
	{ "Size", B_SIZE_T_TYPE },
	{ "Status and Size", B_SSIZE_T_TYPE },
	{ "Time", B_TIME_TYPE },
	{ "Offset", B_OFF_T_TYPE },
	{ "Pointer", B_POINTER_TYPE },
	{ "BRect", B_RECT_TYPE },
	{ "BPoint", B_POINT_TYPE },
	{ "RGB Color", B_RGB_COLOR_TYPE }
};

#define NUM_GENERATORS (sizeof(generators) / sizeof(generate_info))

class BasicValueAddon : public BResourceAddon
{
public:
	BasicValueAddon(const BResourceAddonArgs& args)
		: BResourceAddon(args)
	{
	}
	
	~BasicValueAddon()
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
		
		BasicValue value;
		err = value.SetValue(type, 0, 0);
		if( err != B_OK ) return err;
		
		BResourceCollection* c = WriteLock();
		if( !c ) return B_ERROR;
		
		err = c->AddItem(out_item, value.Type(), id, name,
						 value.Value(), value.Size(), make_selected, resol);
						 
		WriteUnlock(c);
		return err;
	}
	
	// Return the quality of editor we think we are for this data,
	// as fast as possible.
	virtual float QuickQuality(const BResourceItem* item) const
	{
		BasicValue value;
		status_t err = value.SetValue(item->Type(), item->Data(), item->Size());
		if( err == B_OK ) {
			// We're pretty damn good at any basic data type that is
			// understood.
			return 0.6;
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
		return new BasicValueMiniEditor(args, primaryItem);
	}
	
	// Return a full editor for string data.
	virtual BFullItemEditor* MakeFullEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		return new BasicValueFullEditor(args, primaryItem);
	}
	
	// Ooops, not implemented yet.
	virtual status_t HandleDrop(const BMessage* /*drop*/)
	{
		return B_ERROR;
	}
};

extern "C" BResourceAddon* make_nth_resourcer(int32 n, image_id /*you*/, const BResourceAddonArgs& args, uint32 /*flags*/, ...)
{
	if( n == 0 ) return new BasicValueAddon(args);
	return 0;
}
