//	(c) 1997 Be Incorporated
//
//	Classes in this file contain shared parts of the application and
//	file type window
//

#ifndef _BE_H
#include <Alert.h>
#include <Button.h>
#include <MenuItem.h>
#include <Roster.h>
#include <stdio.h>
#include <string.h>
#endif

#include "AutoLock.h"

#include <Debug.h>

#include "MIMETypeWindow.h"
#include "MIMEPanel.h"
#include "MIMEApp.h"
#include <AppFileInfo.h>

const char *kTypesDescriptor = "types";
const char *kSuperTypesDescriptor = "super_types";

// item layout geometries (more in MIMEPanel.h)
const BPoint kButtonSize(75, 20);
const BPoint kSmallButtonSize(70, 20);

MimeTypeSubeditor::MimeTypeSubeditor(BRect frame, const char *title, 
	uint32 followFlags)
	:	BBox(frame, "", followFlags, B_WILL_DRAW | B_FRAME_EVENTS
			| B_PULSE_NEEDED | B_NAVIGABLE_JUMP)
{
	SetLabel(title);
}

MIMETypeListItem::MIMETypeListItem(const char *text, 
	const char *newMIMESignature, int32 level, bool expanded)
	:	BStringItem(text, level, expanded)
#if !_PR3_COMPATIBLE_ && defined DYNAMIC_LIST_POPULATE
		,subtypesQueried(false)
#endif
{
	ASSERT(newMIMESignature);
	mimeSignature = new char[strlen(newMIMESignature) + 1];
	strcpy(mimeSignature, newMIMESignature);
	superTypeName = NULL;
}

MIMETypeListItem::~MIMETypeListItem()
{
	delete [] mimeSignature;
}

const char *
MIMETypeListItem::SuperTypeName()
{
	if (superTypeName)
		return superTypeName;
	
	//printf("mime signature %s\n", mimeSignature);

	int32 superTypeLength = 0;
	for (int32 index = 0; ; index++) {
		if (mimeSignature[index] == '\0')
			break;
		if(mimeSignature[index] == '/')
			superTypeLength = index;
	}
	
	superTypeName = new char[superTypeLength + 1];
	strncpy(superTypeName, mimeSignature, superTypeLength + 1);
	superTypeName[superTypeLength] = '\0';

	return superTypeName;
}

const char *
MIMETypeListItem::MIMESignature() const
{
	const char *tmp = mimeSignature;
	const char * afterLastSlash = tmp;

	for (;;) {
		if (*tmp == '\0')
			break;
		
		if(*tmp++ == '/')
			afterLastSlash = tmp;
	}
	return afterLastSlash;
}

bool 
MIMETypeListItem::IsSuperType() const
{
	const char *tmp = mimeSignature;
	for (;;) {
		if (*tmp == '\0')
			break;
		
		if(*tmp++ == '/')
			return false;
	}
	return true;
}

void
MIMETypeListItem::InitiateDragCommon(BView *owner, BRect frame)
{
	// create a new drag message
	BMessage drag(B_SIMPLE_DATA);

	// embed the mime signature
	drag.AddData("mimeSignature", 'CSTR', MIMEFullSignature(), 
			  	 strlen(MIMEFullSignature()));
	
	// make sure we have the right cursor
	be_app->SetCursor(B_HAND_CURSOR);
	// start a drag
	owner->DragMessage(&drag, frame);
}

void
TypeSignatureTextControl::MakeFocus(bool focusState)
{
//	if (!focusState)
//		Apply();
	BTextControl::MakeFocus(focusState);
}

bool 
MIMETypeIconView::Apply(BNodeInfo *mime, bool saveAs)
{
//	PRINT(("applying app icon \n"));

	if (!saveAs && !dirty)
		return false;
	mime->SetIcon(LargeIcon(), B_LARGE_ICON);
	mime->SetIcon(MiniIcon(), B_MINI_ICON);
	
	if (!saveAs)
		dirty = false;

	return true;
}

void 
MIMETypeIconView::Set(const BNodeInfo *mime)
{
	BBitmap *icon = new BBitmap(kLargeIconRect, B_COLOR_8_BIT);
	if (mime->GetIcon(icon, B_LARGE_ICON) == B_NO_ERROR)
		SetLargeIcon(icon);
	else {
		SetLargeIcon(NULL);
		delete icon;
	}
	icon = new BBitmap(kMiniIconRect, B_COLOR_8_BIT);
	if (mime->GetIcon(icon, B_MINI_ICON) == B_NO_ERROR) 
		SetMiniIcon(icon);
	else {
		SetMiniIcon(NULL);
		delete icon;
	}
}

bool 
MIMETypeIconView::Dirty(const BNodeInfo *) const
{
	return dirty;
}

void 
MIMETypeIconView::CutPasteSetLargeIcon(BBitmap *bitmap)
{
	IconEditView::CutPasteSetLargeIcon(bitmap);
	dirty = true;
}

void 
MIMETypeIconView::CutPasteSetMiniIcon(BBitmap *bitmap)
{
	IconEditView::CutPasteSetMiniIcon(bitmap);
	dirty = true;
}

PreferredAppEditor::PreferredAppEditor(BRect frame)
	:	MimeTypeSubeditor(frame, "Preferred Application", 
			B_FOLLOW_TOP | B_FOLLOW_LEFT)
{
}

void 
PreferredAppEditor::AttachedToWindow()
{
	select->SetTarget(this);
	sameAs->SetTarget(this);
}

BMenuItem *
PreferredAppEditor::NewItemForSignature(const char *signature)
{
	BMenuItem *result;

	BMessage *itemMessage = new BMessage(kSupportingAppFromMenu);
	itemMessage->AddString("signature", signature);

	status_t err = B_ERROR;
	entry_ref entry;
	
	if (signature && signature[0]) {
//		printf("loking for app with signature %s\n", signature);
		err = be_roster->FindApp(signature, &entry);
	}
	
	if (err != B_OK) {
//		printf("didn't find app for signature %s\n", signature);
		result = new BMenuItem(signature, itemMessage);
	} else 
		result = new BMenuItem(entry.name, itemMessage);

	result->SetTarget(this);

	return result;
}

const char *
PreferredAppEditor::SignatureFromMenu()
{
	BMenu *menu = menuField->Menu();
	ASSERT(menu);
	BMenuItem *item = menu->FindMarked();
	if (!item)
		return NULL;

	const char *itemSignature = NULL;

	ASSERT(item->Message());
	item->Message()->FindString("signature", &itemSignature);
	
	return itemSignature;
}

void
PreferredAppEditor::SetAndMarkPreferredSignature(const char *signature)
{
	ASSERT(signature);

	if (!signature[0])
		return;

	BMenu *menu = menuField->Menu();
	status_t result;
	int32 count = menu->CountItems();
	// if in the menu already, only mark
	for (int32 index = 0; index < count; index++) {
		if (!menu->ItemAt(index)->Message())
			// separator items, etc.
			continue;

		const char *itemSignature;
		result = menu->ItemAt(index)->Message()->FindString("signature",
			&itemSignature);
		if (result != B_OK)
			// no preferred app has no signature
			continue;

		if (strcmp(signature, itemSignature) == 0) {
			menu->ItemAt(index)->SetMarked(true);
			return;
		}
	}

//*	printf("preferred app not in supporting menu, adding, %s\n", signature);
	BMenuItem *item = NewItemForSignature(signature);
	item->SetMarked(true);
	menu->AddItem(item);
}

void
PreferredAppEditor::BuildSupportingAppMenu(BMenu *menu, const BMimeType *mime,
	const char *preferredAppSignature)
{
	ASSERT(preferredAppSignature);

	BMenuItem *item =	new BMenuItem("None",
		new BMessage(kSupportingAppFromMenu));
	item->SetTarget(this);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	BMessage message;
	mime->GetSupportingApps(&message);
	for (int32 index =0; ; index++) {
		const char *signature;
		long length;
		// the signatures used here will possibly change
		status_t reply = message.FindData("applications", 'CSTR',
			index, (const void **)&signature, &length);
		
		if (reply != B_NO_ERROR)
			break;

//*		PRINT(("adding, %s\n", signature));
		item = NewItemForSignature(signature);
		menu->AddItem(item);
	}

	SetAndMarkPreferredSignature(preferredAppSignature);
	
	if (menu->FindMarked() == NULL)
		// mark "no preferred app"
		menu->ItemAt(0)->SetMarked(true);
}

bool
PreferredAppEditor::SignatureCanBePreferredApp(const char *signature) const
{
	ASSERT(signature);

	// setting no preferred app is always ok
	if (!signature[0])
		return true;

	const char *currentSignature = CurrentDocumentSignature();

	//printf("examining signature %s as handler for %s\n",
	//	signature, currentSignature);

	entry_ref ref;
	if (be_roster->FindApp(signature, &ref) != B_NO_ERROR)
		return false;

//	PRINT(("found app %s\n", ref.name));

	BFile file(&ref, O_RDONLY);
	if (file.InitCheck() != B_NO_ERROR)
		return false;

	BAppFileInfo supportingAppMime(&file);
	BMessage message;

	status_t error = supportingAppMime.GetSupportedTypes(&message);
	if (error != B_NO_ERROR) {
		// PRINT(("no supported types in app\n"));
		return false;
	}

	const char *mimeSignature;
	int32 bufferLength;
				

	// supported type signatures are embedded as a sequence of C strings;
	// find them all and extract their icons
	for (int32 index = 0; ; index++) {
		error = message.FindData("types", 'CSTR', index, (const void **)&mimeSignature, 
			&bufferLength);
		if (error != B_OK)
			return false;
		
//		PRINT(("checking supported signature %s against %s\n", 
//			mimeSignature, currentSignature));
		if (strcmp(mimeSignature, currentSignature) == 0) {
			// looks good
			return true;
		}
	}

	return false;
}

bool 
PreferredAppEditor::TrySettingSupportingApp(const char *signature)
{	
	if (!SignatureCanBePreferredApp(signature) &&
		(new BAlert("", "The application that you selected as preferred "
			"does not seem to handle this file. Do you still want to "
			"set it as preferred?", "Set anyway", "Don't set"))->Go() == 1)
		return false;

	
//	PRINT(("setting preferred app signature %s\n", signature));
	SetAndMarkPreferredSignature(signature);	
	return true;
}

static status_t
AppSignatureFromRefInMessage(const BMessage *message, char *buffer)
{
	entry_ref ref;
	status_t result = message->FindRef("refs", &ref);
	
	if (result != B_NO_ERROR)
		return result;
		
	BFile file(&ref, O_RDONLY);
	result = file.InitCheck();
	
	if (result != B_NO_ERROR)
		return result;

//	PRINT(("found app signature\n"));
	BAppFileInfo appInfo(&file);
	return appInfo.GetSignature(buffer);
}

void 
PreferredAppEditor::MessageReceived(BMessage *message)
{
	char buffer[255];
	
	if (message->WasDropped()) {
//		PRINT(("preferred app editor got a drop\n"));
		int32 length;
		char *text;
		if (((message->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &length)
				== B_NO_ERROR)
			|| (message->FindData("mimeSignature", 'CSTR', (const void **)&text, &length)
				== B_NO_ERROR))
					&& text) {
			text[length] = 0;
//			PRINT(("got a drop message %s - will try to add as supporting app\n", 
//				text));
			if (TrySettingSupportingApp(text))
				// we handled the drag, done with message
				return;
		}
		
		if ((AppSignatureFromRefInMessage(message, buffer) == B_NO_ERROR)
			&& TrySettingSupportingApp(buffer))
			return;
		
		// do not propagate a drop to the superview
		return;
	}
	
	switch(message->what) {
		case kSupportingAppFromMenu:
			SelectedPreferredAppFromMenu(SignatureFromMenu());
			break;
		case kRunSelectAppPanel:
			{
				BMessenger self(this);
				BFilePanel *panel = FileTyperOpenPanel();
				panel->SetTarget(self);
				BMessage newMsg(kSupportingAppSelected);
				panel->SetMessage(&newMsg);
				panel->Window()->SetTitle("Choose a supporting "
				  "application:");
				panel->Show();
				return;
			}
		case kRunSameAsPanel:
			{
				BMessenger self(this);
				BFilePanel *panel = FileTyperOpenPanel();
				panel->SetTarget(self);
				BMessage newMsg(kSameAsAppSelected);
				panel->SetMessage(&newMsg);
				panel->Window()->SetTitle("Set Supporting "
				  "Application Same As:");
				panel->Show();
				return;
			}
		case kSupportingAppSelected:
			if (AppSignatureFromRefInMessage(message, buffer) == B_NO_ERROR)
				TrySettingSupportingApp(buffer);
				
			return;
		case kSameAsAppSelected:
		
			// if message decoding fails, do not propagate to superview
			// since that would cause the file to get opened
			entry_ref ref;
			status_t result = message->FindRef("refs", &ref);
			
			if (result != B_NO_ERROR)
				return;
				
//			PRINT(("setting preferred app same as %s\n", ref.name));

			BFile file(&ref, O_RDONLY);
			result = file.InitCheck();
			
			if (result != B_NO_ERROR)
				return;
			
			BNodeInfo info(&file);
			// anticipate getting no preferred app
			buffer[0] = '\0';
			info.GetPreferredApp(buffer);
			TrySettingSupportingApp(buffer);

			return;
	}
	inherited::MessageReceived(message);
}

MIMETypeView::MIMETypeView(BRect frame, uint32 resizeFlags)
	:	BBox(frame, "", resizeFlags, 
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP | B_PULSE_NEEDED, 
			B_PLAIN_BORDER)
{	
	SetViewColor(kLightGray);
}

MIMETypeView::~MIMETypeView()
{
}


void
MIMETypeView::MouseDown(BPoint where)
{
	if (!Window()->IsActive())
		Window()->Activate();
	
	inherited::MouseDown(where);
}

void 
MIMETypeView::MessageReceived(BMessage *message)
{
	if (message->WasDropped() && message->HasRef("refs")) {
		entry_ref ref;
		MIMEApp *app = dynamic_cast<MIMEApp *>(be_app);
		ASSERT(app);
		if (!app)
			return;

		app->OpenWindow(message);
	}

	switch (message->what) {
		case kSaveFile:
			Save();
			break;
		default:
			inherited::MessageReceived(message);
			break;
	}
}

MIMETypeWindow::MIMETypeWindow(BRect frame, uint32 flags)
	:	BWindow(frame, "", B_TITLED_WINDOW, flags)
{
}

bool 
MIMETypeWindow::QuitRequested()
{
	AutoLock<BWindow> lock(this);

	if (!panel->QuitRequested())
		return false;

	if (panel->Dirty()) {
		char buffer[256];
		sprintf(buffer, "Would you like to save changes to file type attributes"
						" of %s?", panel->Name());
			
		switch ((new BAlert(B_EMPTY_STRING, buffer, "Cancel", "Don't Save",
		  "Save", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go()) {
			case 2:
				if (!panel->Save())	// Save
					return false;
				break;

			case 1:
				break;				// Don't Save

			case 0:
				return false; 		// Cancel
		}
	}
	return true;
}

static int
CompareItems(const BListItem *a, const BListItem *b)
{
	ASSERT(dynamic_cast<const BStringItem *>(a)
		&& dynamic_cast<const BStringItem *>(b));
		// only do a debug check here for speed

	BStringItem *item1 = (BStringItem *)a;
	BStringItem *item2 = (BStringItem *)b;
	
#if xDEBUG
	// make sort do more worsk
	return strcasecmp(item2->Text(), item1->Text());
#else
	return strcasecmp(item1->Text(), item2->Text());
#endif
}

inline int
CompareItemsGlue(const void *a, const void *b)
{
	return CompareItems(*(const BListItem **)a, *(const BListItem **)b);
}

void
BuildMimeTypeList(BListView *list, bool includeSupertypes,
	bool includeInternalTypesSignatures)
{
	BMessage supertypes;
	BMimeType::GetInstalledSupertypes(&supertypes);

	int32 supertypeCount;
	uint32 type = B_STRING_TYPE;
	supertypes.GetInfo(kSuperTypesDescriptor, &type, &supertypeCount);
	for (int32 supertypeIndex = 0; supertypeIndex < supertypeCount; 
		supertypeIndex++) {
		const char *supertypeName;
		supertypes.FindString(kSuperTypesDescriptor, supertypeIndex, 
			&supertypeName);
	
		if (includeSupertypes) {
			MIMETypeListItem *item = new MIMETypeListItem(supertypeName, supertypeName, 0);
#define COLLAPSE_GROUPS
#ifdef COLLAPSE_GROUPS
			item->SetExpanded(false);
#endif
			list->AddItem(item);
		}

		// ask for all the mime types and stuff their names 
		// into the list
		BMessage mimeTypes;
		BMimeType::GetInstalledTypes(supertypeName, &mimeTypes);
		
		int32 count;
		mimeTypes.GetInfo(kTypesDescriptor, &type, &count);
		
		for (int32 index = 0; index < count; index++) {
			const char *mimeTypeName;
			mimeTypes.FindString(kTypesDescriptor, index, &mimeTypeName);

			// we will either use the short description in the list view
			// item or the mime type signature if there is no description
			
			const char *afterLastSlash = NULL;

			BMimeType currentMime(mimeTypeName);
			char buffer[B_MIME_TYPE_LENGTH];

			if (!includeInternalTypesSignatures) {
				// weed out apps - their preferred handler is themselves
				if (currentMime.GetPreferredApp(buffer) == B_NO_ERROR
					&& strcasecmp(buffer, mimeTypeName) == 0)
					continue;
				// weed out HFS types
				if (strstr(mimeTypeName, "application/x-MacOS-"))
					continue;
			}

			if (currentMime.GetShortDescription(buffer) != B_NO_ERROR 
				|| buffer[0] == '\0') {

				buffer[0] = '\0';

				// no description, we will use the mime file signature instead
				// removing the supertypename from it
				const char *tmp = mimeTypeName;
				afterLastSlash = tmp;
				if (includeSupertypes)
					// if not showing supertypes, just add the whole signature
					for (;;) {
						if (*tmp == '\0')
							break;
						
						if(*tmp++ == '/')
							afterLastSlash = tmp;
					}
			}

			list->AddItem(new MIMETypeListItem(buffer[0] != '\0' ? buffer 
				: afterLastSlash, mimeTypeName, 1));
		}
	}

	BOutlineListView *outlineList = dynamic_cast<BOutlineListView *>(list);
	if (outlineList)
		outlineList->FullListSortItems(CompareItems);
	else
		list->SortItems(CompareItemsGlue);
}

