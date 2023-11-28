/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <ResourceEditor.h>

#include <TextControl.h>
#include <Window.h>
#include <Autolock.h>
#include <Debug.h>
#include <TypeConstants.h>
#include <RadioButton.h>
#include <CheckBox.h>
#include <Roster.h>
#include <stdio.h>

enum {
	B_APPLICATION_FLAGS_TYPE		= 'APPF'
};

// -----------------------------------------------------------------------------

// The application flags resource mini editor.

class AppFlagsMiniEditor : public BMiniItemEditor
{
public:
	AppFlagsMiniEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
	 : BMiniItemEditor(args, primaryItem)
	{
	}

	~AppFlagsMiniEditor()
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

// -----------------------------------------------------------------------------

// AppFlags resource full editor.

// This is the full app flags resource editor.  It is a mix-in of
// a BFullItemEditor and BView, so that we can do all the needed
// subclassing in one place.

class AppFlagsFullEditor : public BFullItemEditor, public BView
{
public:
	AppFlagsFullEditor(const BResourceAddonArgs& args, BResourceHandle primaryItem)
		: BFullItemEditor(args, primaryItem, this),
		  BView(BRect(0, 0, 50, 50), "AppFlagsEditor",
				B_FOLLOW_ALL_SIDES, B_NAVIGABLE_JUMP),
		  fPrefWidth(0), fPrefHeight(0),
		  fFlags(0), fOrigFlags(0)
	{
		float pw, ph;
		
		BRect radioFrame(6, 6, 0, 0);
		radioFrame.bottom = radioFrame.top + 16;
		radioFrame.right = radioFrame.left + 100;

		fSingleLaunch = new BRadioButton(radioFrame, "single", "Single Launch",
										 new BMessage('sngl'));
		AddChild(fSingleLaunch);
		
		fSingleLaunch->ResizeToPreferred();
		fSingleLaunch->GetPreferredSize(&pw, &ph);
		if( pw > fPrefWidth ) fPrefWidth = pw;
		
		radioFrame.OffsetBy(0, ph + 4);
		fMultipleLaunch = new BRadioButton(radioFrame, "multiple", "Multiple Launch",
										   new BMessage('mult'));
		AddChild(fMultipleLaunch);

		fMultipleLaunch->ResizeToPreferred();
		fMultipleLaunch->GetPreferredSize(&pw, &ph);
		if( pw > fPrefWidth ) fPrefWidth = pw;
		
		radioFrame.OffsetBy(0, ph + 4);
		fExclusiveLaunch = new BRadioButton(radioFrame, "exclusive", "Exclusive Launch",
											new BMessage('excl'));
		AddChild(fExclusiveLaunch);

		fExclusiveLaunch->ResizeToPreferred();
		fExclusiveLaunch->GetPreferredSize(&pw, &ph);
		if( pw > fPrefWidth ) fPrefWidth = pw;
		
		fPrefHeight = radioFrame.top + ph + 6;
		
		fSingleLaunch->SetValue(1);

		BRect checkBoxFrame(radioFrame.left*2 + fPrefWidth, 6, 0, 0);
		checkBoxFrame.bottom = checkBoxFrame.top + 16;
		checkBoxFrame.right = checkBoxFrame.left + 100;

		fArgvOnlyApp = new BCheckBox(checkBoxFrame, "argv", "Argv Only",
									 new BMessage('argv'));
		AddChild(fArgvOnlyApp);

		float rw = 0;
		
		fArgvOnlyApp->ResizeToPreferred();
		fArgvOnlyApp->GetPreferredSize(&pw, &ph);
		if( pw > rw ) rw = pw;
		
		checkBoxFrame.OffsetBy(0, ph + 4);
		fBackgroundApp = new BCheckBox(checkBoxFrame, "background", "Background App",
									   new BMessage('back'));
		AddChild(fBackgroundApp);
		
		fBackgroundApp->ResizeToPreferred();
		fBackgroundApp->GetPreferredSize(&pw, &ph);
		if( pw > rw ) rw = pw;
		
		fPrefWidth += radioFrame.left*3 + rw;
		
		const BResourceCollection* c = ReadLock();
		if( c ) {
			ExecDataChanged(c, PrimaryItem(), B_RES_ALL_CHANGED);
			ReadUnlock(c);
		}
	}

	~AppFlagsFullEditor()
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
		fSingleLaunch->SetTarget(this);
		fMultipleLaunch->SetTarget(this);
		fExclusiveLaunch->SetTarget(this);
		fArgvOnlyApp->SetTarget(this);
		fBackgroundApp->SetTarget(this);
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
		*width = fPrefWidth;
		*height = fPrefHeight;
	}
	
	void MakeFocus(bool on)
	{
		if( on && fSingleLaunch ) fSingleLaunch->MakeFocus(on);
		else if( on != IsFocus() ) BView::MakeFocus(on);
	}

	// Copy current flags data (if it has changed) into the resource
	// item being edited.
	
	void UpdateValue(const char* name = "Change Flags")
	{
		if( fFlags != fOrigFlags ) {
			BResourceCollection* c = WriteLock(name);
			
			if( c ) {
				BResourceItem* it = c->WriteItem(PrimaryItem(), this);
				if( it ) {
					it->SetData(&fFlags, sizeof(fFlags));
					fOrigFlags = fFlags;
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
			
			case 'sngl' :
				fFlags = (fFlags & (~B_LAUNCH_MASK)) | B_SINGLE_LAUNCH;
				UpdateValue("Make Single Launch");
				break;

			case 'mult' :
				fFlags = (fFlags & (~B_LAUNCH_MASK)) | B_MULTIPLE_LAUNCH;
				UpdateValue("Make Multiple Launch");
				break;

			case 'excl' :
				fFlags = (fFlags & (~B_LAUNCH_MASK)) | B_EXCLUSIVE_LAUNCH;
				UpdateValue("Make Exclusive Launch");
				break;

			case 'argv' :
				fFlags &= ~B_ARGV_ONLY;
				if(fArgvOnlyApp->Value())
					fFlags |= B_ARGV_ONLY;
				UpdateValue("Toggle Argv Flag");
				break;

			case 'back' :
				fFlags &= ~B_BACKGROUND_APP;
				if(fBackgroundApp->Value())
					fFlags |= B_BACKGROUND_APP;
				UpdateValue("Toggle Background Flag");
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
		if( item != PrimaryItem() || changes == 0 ) return;
		
		if( Window() && !Window()->IsLocked() ) {
			debugger("ExecDataChanged() called without window locked");
			return;
		}
		
		// Retrieve resource item, and copy it into the text editor
		// if its data has changed.
		const BResourceItem* it = c->ReadItem(PrimaryItem());
		PRINT(("Doing change %lx (data=%lx)\n", changes, B_RES_DATA_CHANGED));
		if( (changes&B_RES_DATA_CHANGED) != 0 && it->Size() >= sizeof(fFlags) ) {
			fFlags = fOrigFlags = *(const uint32*)it->Data();
			fSingleLaunch->SetValue((fFlags & B_LAUNCH_MASK) == B_SINGLE_LAUNCH);
			fMultipleLaunch->SetValue((fFlags & B_LAUNCH_MASK) == B_MULTIPLE_LAUNCH);
			fExclusiveLaunch->SetValue((fFlags & B_LAUNCH_MASK) == B_EXCLUSIVE_LAUNCH);
	
			fArgvOnlyApp->SetValue((fFlags & B_ARGV_ONLY) != 0);
			fBackgroundApp->SetValue((fFlags & B_BACKGROUND_APP) != 0);
		}
	}

	// Return the view for this editor.  Which is just myself.
	
	virtual BView* View()
	{
		return this;
	}

private:
	BRadioButton	*fSingleLaunch;
	BRadioButton	*fMultipleLaunch;
	BRadioButton	*fExclusiveLaunch;

	BCheckBox		*fArgvOnlyApp;
	BCheckBox		*fBackgroundApp;

	float			fPrefWidth, fPrefHeight;
	
	uint32			fFlags;
	uint32			fOrigFlags;
};

// -----------------------------------------------------------------------------

// This is the top-level application flags editor add-on.

struct generate_info {
	const char* name;
	type_code type;
};

static generate_info generators[] = {
	{ "Application Flags", B_APPLICATION_FLAGS_TYPE },
};

#define NUM_GENERATORS (sizeof(generators) / sizeof(generate_info))

class AppFlagsAddon : public BResourceAddon
{
public:
	AppFlagsAddon(const BResourceAddonArgs& args)
		: BResourceAddon(args)
	{
	}
	
	~AppFlagsAddon()
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
			case B_APPLICATION_FLAGS_TYPE: {
				uint32 flags = 0;
				err = c->AddItem(out_item, type, id, name,
								 &flags, sizeof(flags), make_selected, resol);
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
		if( item->Type() == B_APPLICATION_FLAGS_TYPE &&
				item->Size() == sizeof(uint32) ) {
			// We're pretty damn good at application flags.
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
		return new AppFlagsMiniEditor(args, primaryItem);
	}
	
	// Return a full editor for string data.
	virtual BFullItemEditor* MakeFullEditor(const BResourceAddonArgs& args,
											BResourceHandle primaryItem,
											const BMessage* /*configuration*/)
	{
		return new AppFlagsFullEditor(args, primaryItem);
	}
	
	// Ooops, not implemented yet.
	virtual status_t HandleDrop(const BMessage* /*drop*/)
	{
		return B_ERROR;
	}
};

extern "C" BResourceAddon* make_nth_resourcer(int32 n, image_id /*you*/, const BResourceAddonArgs& args, uint32 /*flags*/, ...)
{
	if( n == 0 ) return new AppFlagsAddon(args);
	return 0;
}
