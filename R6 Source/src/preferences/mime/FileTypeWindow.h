#ifndef __FILE_TYPE_WINDOW__
#define __FILE_TYPE_WINDOW___

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
#include "MIMETypeWindow.h"

class TypeEditor : public MimeTypeSubeditor {
	// this is used to edit the applications signature
public:
	TypeEditor(BRect frame);

	virtual void SetDefault(const char*);		
	virtual bool Apply(BNodeInfo *);
	virtual bool Dirty() const;
	const char *GetType() const;
	virtual void MessageReceived(BMessage *);
	virtual void AttachedToWindow();

private:
	typedef MimeTypeSubeditor inherited;
	bool TrySettingType(const char *);

	BTextControl *textControl;
	BButton *select;
	BButton *sameAs;
	char defaultValue[B_MIME_TYPE_LENGTH];
};

class FilePreferredAppEditor : public PreferredAppEditor {
public:
	FilePreferredAppEditor(BRect);
	virtual ~FilePreferredAppEditor()
		{}
	
	virtual void Set(const BNodeInfo *);
	virtual bool Apply(BNodeInfo *);
	virtual bool Dirty(const BNodeInfo *) const;

private:
	virtual const char *CurrentDocumentSignature() const;	
};

class FileTypeView : public MIMETypeView {
	// this is the entire app mime type panel
public:
	FileTypeView(BRect, BList *refList, uint32 resizeFlags = B_FOLLOW_ALL);
	virtual ~FileTypeView()
		{}

	virtual void MessageReceived(BMessage *);

	virtual const char *Name() const;
	
	virtual bool Save();
		// called when switching to a different mime type or
		// when the panel is dismissed
	virtual bool Dirty() const;
		// return true if save needed
	virtual bool QuitRequested();
	virtual void AttachedToWindow();
	virtual void Draw(BRect);
	virtual const entry_ref* Ref() const;


private:
	typedef MIMETypeView inherited;
	virtual void SetMime();

	TypeEditor *type;
	MIMETypeIconView *iconItem;
	FilePreferredAppEditor *preferredAppItem;
	BList *refs;

	BBitmap *multipleFileIcon;
};

class FileTypeWindow : public MIMETypeWindow {
public:
	FileTypeWindow(BPoint leftTop, BList *refList);
	virtual ~FileTypeWindow()
		{ }

};

#endif
