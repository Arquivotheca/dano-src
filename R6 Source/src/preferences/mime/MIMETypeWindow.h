//	(c) 1997 Be Incorporated
//
//	Classes in this file contain shared parts of the application and
//	file type window
//

#ifndef __MIMETYPEWINDOW__
#define __MIMETYPEWINDOW__

#ifndef _BE_H
#include <Box.h>
#include <Bitmap.h>
#include <CheckBox.h>
#include <OutlineListView.h>
#include <Mime.h>
#include <Menu.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <Window.h>
#endif

#include <NodeInfo.h>

#include "IconEditView.h"

class MimeTypeSubeditor : public BBox {
	// this is a cluster of controls that all support editing one part of
	// the mime attributes of a file
public:
	MimeTypeSubeditor(BRect, const char *title, uint32);
};

class TypeSignatureTextControl : public BTextControl {
	// this is used to edit the applications signature
public:
	TypeSignatureTextControl(BRect frame, const char *name, const char *label, 
		const char *initial_text, BMessage *message,
		uint32 rmask, uint32 flags)
		:	BTextControl(frame, name, label, initial_text, message, 
			rmask, flags)
		{ }
		
	virtual void MakeFocus(bool focusState = true);
};

class MIMETypeIconView : public IconEditView {
	// this is used to edit the file/apps icon
	// IconView is overriden to support Dirty/Apply
public:
	MIMETypeIconView(BRect rect, const char *name, 
		BBitmap *largeIcon, BBitmap *miniIcon,
		uint32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
		:	IconEditView(rect, name, largeIcon, miniIcon, true, resize, flags)
		{}

	virtual ~MIMETypeIconView() {}

	virtual bool Apply(BNodeInfo *, bool saveAs = false);
		// <saveAs> used to prevent false clearing of the dirty bit
		// when saving an unsaved panel into a resource file
	virtual void Set(const BNodeInfo *);
	virtual bool Dirty(const BNodeInfo *) const;

private:
	virtual void CutPasteSetLargeIcon(BBitmap *);
	virtual void CutPasteSetMiniIcon(BBitmap *);
//	bool dirty;
};


const int32 kRunSelectAppPanel = 'rsea';
const int32 kRunSameAsPanel = 'rsas';
const int32 kSupportingAppSelected = 'spas';
const int32 kSameAsAppSelected = 'smas';
const int32 kSupportingAppFromMenu = 'spms';

class PreferredAppEditor : public MimeTypeSubeditor {
public:
	PreferredAppEditor(BRect);
	virtual ~PreferredAppEditor()
		{}
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *);
protected:
	typedef MimeTypeSubeditor inherited;
	virtual bool TrySettingSupportingApp(const char *);
	bool SignatureCanBePreferredApp(const char *signature) const;
	void SetAndMarkPreferredSignature(const char *signature);
	void BuildSupportingAppMenu(BMenu *menu, const BMimeType *mime,
		const char *preferredAppSignature);
	BMenuItem *NewItemForSignature(const char *signature);
	
	virtual void SelectedPreferredAppFromMenu(const char *)
		{}

	virtual const char *SignatureFromMenu();

	virtual const char *CurrentDocumentSignature() const = 0;
		// used to check if a new supporting app really supports this
		// overriden in subclasses

	BMenuField *menuField;
	BButton *select;	
	BButton *sameAs;
	char signatureBuffer[B_MIME_TYPE_LENGTH];
};

class MIMETypeView : public BBox {
	// this is the entire app mime type panel
public:
	MIMETypeView(BRect, uint32 resizeFlags);
	virtual ~MIMETypeView();

	virtual void MessageReceived(BMessage *);
	virtual void MouseDown(BPoint);
	
	virtual bool Save() = 0;
		// called when switching to a different mime type or
		// when the panel is dismissed.  Returns true if the save
		// was done, false if the user canceled at some point
	virtual bool Dirty() const = 0;
		// return true if save needed

	virtual const char *Name() const = 0;

	//
	//	If this is displaying one file, returns the entry ref for it.
	//	returns NULL if multiple entry refs are being displayed.
	//
	virtual const entry_ref *Ref() const = 0;

	virtual bool QuitRequested()
		{ return true; }

protected:
	typedef BBox inherited;
};

class MIMETypeWindow : public BWindow {
public:
	MIMETypeWindow(BRect frame, uint32 resizeFlags);
	virtual ~MIMETypeWindow()
		{ }

	virtual bool QuitRequested();
	virtual MIMETypeView *Panel() const
		{ return panel; }

	virtual const char *Name() const
		{ return Panel()->Name(); }

	virtual const entry_ref* Ref() const
		{ return Panel()->Ref(); }

protected:
	MIMETypeView *panel;
};

#endif
