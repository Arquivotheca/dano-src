//	(c) 1997, Be Incorporated
//
//

#ifndef __APP_MIME_WINDOW__
#define __APP_MIME_WINDOW__

#ifndef _BE_H
#include <Box.h>
#include <Bitmap.h>
#include <CheckBox.h>
#include <ListView.h>
#include <Mime.h>
#include <Menu.h>
#include <MenuField.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <CheckBox.h>
#include <Window.h>
#endif

#include <AppFileInfo.h>

#include "MIMEPrefsDialogs.h"
#include "MIMETypeWindow.h"

class MIMEListItemWithIcon : public MIMETypeListItem {
	// when a new icon is pasted for a corresponding supported
	// type, it is stored in the list item till the next save
public:
	MIMEListItemWithIcon(const char *text, const char *mimeSignature,
		BBitmap *largeIcon, BBitmap *miniIcon,
		int32 level, bool expanded = true);
	virtual ~MIMEListItemWithIcon();

	BBitmap *LargeIcon() const;
	BBitmap *MiniIcon() const;
	void SetLargeIcon(BBitmap *icon);
	void SetMiniIcon(BBitmap *icon);
	bool HasChanges() const
		{ return hasChanges; }
	void SetHasChanges(bool set = true)
		{ hasChanges = set; }
	
private:
	BBitmap *largeIcon;
	BBitmap *miniIcon;
	bool hasChanges;
};

class SupportedTypesEditor;

class SupportedTypesIconView : public IconEditView {
	// this is used to edit the supported type icon
	// IconView is overriden to support ownership of 
	// bitmap data by a SupportedTypesEditor, which
	// keeps the changed icons cached up in MIMEListItemWithIcon
	// till the next save
public:
	SupportedTypesIconView(BRect rect, const char *name, 
		BBitmap *largeIcon, BBitmap *miniIcon, SupportedTypesEditor *owner,
		uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
		:	IconEditView(rect, name, largeIcon, miniIcon, false, resize, flags),
			bitmapOwner(owner)
		{}

	virtual ~SupportedTypesIconView() {}

	void MouseDown(BPoint pt);

	virtual bool AcceptsDrop(const BMessage *) const;
	virtual bool AcceptsPaste() const;
		// say no in the accepts calls if no supported type 
		// selected

private:

	typedef IconEditView inherited;

	virtual void CutPasteSetLargeIcon(BBitmap *);
	virtual void CutPasteSetMiniIcon(BBitmap *);

//	bool dirty;
	SupportedTypesEditor *bitmapOwner;
};

class SupportedTypesListView : public BListView {
// override BListView to implement InitiateDraw
public:
	SupportedTypesListView(BRect frame, const char *name)
		:	BListView(frame, name)
		{}
		
	virtual bool InitiateDrag(BPoint, int32 index, bool initialySelected);
};

class SupportedTypesEditor : public MimeTypeSubeditor {
	// let's you display and add supported types
	// handles drops
public:
	SupportedTypesEditor(BRect);
	virtual ~SupportedTypesEditor();
	
	virtual void Set(const BAppFileInfo *appMime);
	virtual bool Apply(BAppFileInfo *appMime, bool as = false);
	virtual bool Dirty(const BAppFileInfo *) const;
	
	virtual void AttachedToWindow();
	virtual void Pulse();

	virtual bool QuitRequested();

	void ShowCurrentIcon(int32 index);
		// common routine used to decide which icon to display in
		// the IconView
		// will either use an icon from a current MIMEListItemWithIcon
		// or an icon obtained by calling the mime API
		
	MIMEListItemWithIcon *CurrentItem();
private:

	typedef MimeTypeSubeditor inherited;

	void PastedLargeIcon(BBitmap *icon);
	void PastedMiniIcon(BBitmap *icon);
		// called by SupportedTypesIconView during a paste

	void SelectSignature(const char *signature);
		// selects the corresponding item in list and calls
		// ShowCurrentIcon
	void Select(int32 index);
		// selects item at index and calls ShowCurrentIcon
	void AddMimeType(const char *signature);
		// as result of drag/drop or by adding from a dialog
	bool TryAddingMimeType(const char *signature);
		// as result of drag/drop or by adding from a dialog
	void RemoveCurrentMimeType();
	virtual void MessageReceived(BMessage *message);

	int32 AddMimeTypeToListCommon(const char *signature);
		// shared between Set and AddMimeType
	
	SupportedTypesListView *supportedTypesList;
	BButton *addButton;
	BButton *removeButton;
//	BCheckBox *syncAllCheckBox;
	SupportedTypesIconView *iconItem;
	
	const BAppFileInfo *mime;		
						// need to cache current mime type
						// here so we can fetch icons for suppported
						// types and add supported types
	bool dirty;

friend class SupportedTypesIconView;
};

class NumericTextControl : public BTextControl {
	// used for the different version number fields
	// for now just calls BTextControl
public:
	NumericTextControl(BRect frame, const char *name,
		const char *label, const char *initialText, 
		BMessage *message,
		uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
};

class VersionEditor : public MimeTypeSubeditor {
	// lets you edit the version attribute
public:
	VersionEditor(BRect);
	virtual ~VersionEditor();
	
	virtual void Set(const BAppFileInfo *appMime);
	virtual bool Apply(BAppFileInfo *appMime, bool as = false);
	virtual bool Dirty(const BAppFileInfo *) const;
private:
	void MessageReceived(BMessage *);
	void AttachedToWindow();
	void CurrentVersionInfo(version_info *) const;
	bool Dirty(const BAppFileInfo *, version_kind) const;

	void ShowKind(version_kind);

	NumericTextControl *major;
	NumericTextControl *middle;
	NumericTextControl *minor;
	BMenuField *variety;
	NumericTextControl *internal;
	
	BTextControl *shortInfo;
	BTextView *longInfo;
	BMenuField *appOrSystemSelector;
	
	version_info systemVersion;
	version_info appVersion;
	bool showingSystem;

	typedef MimeTypeSubeditor _inherited;
};

const int32 M_SETFLAGS = 'stfl';

class ApplicationFlagsEditor : public MimeTypeSubeditor {
	// lets you edit the app flags such as single lauch,
	// argv only, etc.
public:
	ApplicationFlagsEditor(BRect);
	virtual ~ApplicationFlagsEditor();
	
	virtual void Set(const BAppFileInfo *appMime);
	virtual bool Apply(BFile *file, bool as = false);
	virtual bool Dirty(const BAppFileInfo *) const;
	virtual void MessageReceived(BMessage *msg);
	virtual void AttachedToWindow();

private:
	typedef MimeTypeSubeditor inherited;
	void CurrentFlags(uint32 *flags) const;

	BRadioButton *singleLaunch;
	BRadioButton *multipleLaunch;
	BRadioButton *exclusiveLaunch;

	BCheckBox *setFlags;	
	BCheckBox *argvOnlyApp;
	BCheckBox *backgroundApp;

	bool fileHasFlags;
};

class SignatureTextControl : public TypeSignatureTextControl {
	// this is used to edit the applications signature
public:
	SignatureTextControl(BRect frame, const char *name, 
		const char *initial_text, BMessage *message,
		uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
		:	TypeSignatureTextControl(frame, name, "Signature:", 
			initial_text, message, rmask, flags)
		{ }
	
	virtual void Set(const BNodeInfo *);
	virtual bool Apply(BAppFileInfo *, bool as = false);
	virtual bool Apply(bool as = false);
	virtual bool Dirty(const BNodeInfo *) const;
};

class AppMimeView : public MIMETypeView {
	// this is the entire app mime type panel
public:
	AppMimeView(BRect, const entry_ref *ref, uint32 resizeFlags = B_FOLLOW_ALL);
	virtual ~AppMimeView();

	virtual void MessageReceived(BMessage *);
	
	bool Save();
		// called when switching to a different mime type or
		// when the panel is dismissed
	virtual bool Dirty() const;
		// return true if save needed
	
	status_t SaveAsResourceFile(const entry_ref *dir, const char *name);
		// used to create a resource file with current mime settings
	
	void InitiateSaveAsResourceFile();
		// for development-save file as a resource that can be used
		// in a project
	
	virtual bool QuitRequested();
	virtual BNodeInfo* CurrentMime() const
		{ return currentMime; }
	virtual const char* Name() const
		{ 
			if (currentMime != NULL)
				return currentRef.name; 

			return "[untitled]";
		}

	virtual const entry_ref* Ref() const
		{ return &currentRef; }

	virtual void AttachedToWindow();

private:
	BNodeInfo *currentMime;
	BFile file;
	entry_ref currentRef;
	typedef MIMETypeView inherited;

	virtual void SetMime(const BNodeInfo *appMime);

	SignatureTextControl *signature;
	MIMETypeIconView *iconItem;
	SupportedTypesEditor *supportedTypes;
	VersionEditor *version;
	ApplicationFlagsEditor *applicationFlags;

	//
	//	This is a hack.  The user is prompted to close on save, and may
	//	be presented with a file panel.  If this is the case, our looper
	//	must continue running until the file panel tells us that a file
	//	has been chosen.  At that point we should save.
	//	
	bool closeAfterSave;
};

class AppMimeTypeWindow : public MIMETypeWindow {
public:
	AppMimeTypeWindow(BPoint leftTop, const entry_ref *ref);
	~AppMimeTypeWindow()
		{ }

};


#endif

