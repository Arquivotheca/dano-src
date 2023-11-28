//
//	(c) 1997 Be Incorporated
//

#ifndef __MIME_PREFS_DIALOGS__
#define __MIME_PREFS_DIALOGS__

#ifndef _BE_H
#include <ListView.h>
#include <Message.h>
#include <MenuField.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>
#include <MessageFilter.h>
#endif


class MIMEPanel;
class AttributeEditor;
class BTextControl;
class BMenuField;
class BCheckBox;

const int32 M_MIME_TYPE_ADD = 'tpad';
const int32 M_MIME_TYPE_CHANGE = 'tpch';
const int32 M_XTENSION_ADD = 'xtad';
const int32 M_XTENSION_CHANGE = 'xtch';
const int32 M_ATTRIBUTE_ADD = 'atad';
const int32 M_ATTRIBUTE_CHANGE = 'atch';
const int32 kMimeTypeSelected = 'tpsl';

class DialogFilter : public BMessageFilter {
public:
	DialogFilter(BWindow *target);
	virtual	filter_result Filter(BMessage *, BHandler **);

private:
	BWindow *filterTarget;
};


BRect CenterRectIn(BRect rectToCenter, BRect centerIn);

class GenericEditDialog : public BWindow {
public:
	GenericEditDialog(BRect frame);
	virtual ~GenericEditDialog();

	void AddOkCancel(bool changeButton, bool cancel = true,
		bool initialyEnabled = false);
private:
	typedef BWindow inherited;
	// override these to get nontrivial functionality
	virtual bool Done();
		// called when OK hit, return true if should close dialog
	virtual bool ShouldEnableOK();
		// return true if OK should be active

	virtual void MessageReceived(BMessage *);
protected:
	BButton *okButton;
	
//friend class AttributeEditor;
//friend class AddExtensionEditor;
//friend class AddAttributeEditor;
};

class AttributeEditDialog : public GenericEditDialog {
public:
	AttributeEditDialog(BRect frame, AttributeEditor *target);
	~AttributeEditDialog();
private:
	virtual bool Done();
	virtual bool ShouldEnableOK();
	AttributeEditor *target;
};

class AttributeEditor {
public:
	AttributeEditor(BView *target, bool modification);
	virtual ~AttributeEditor() {}

	void Edit();
	virtual bool ReturnResult() = 0;
		// done packages results in a message if valid, quits the
		// dialog and gets deleted by the dialog
		// returns false if result not valid and user should correct 
		// data in the dialog
	virtual bool ShouldEnableAdd() const = 0;

protected:

	GenericEditDialog *dialog;
	BView *target;
	bool modification;
};

class MIMEExtraAttribute;

class AddAttributeEditor : public AttributeEditor {
public:
	AddAttributeEditor(BRect frame, BView *target, bool modification,
		const MIMEExtraAttribute *);
	virtual ~AddAttributeEditor() {}
	
	void SetUpData(const MIMEExtraAttribute *);
	virtual bool ShouldEnableAdd() const;
	
	virtual bool ReturnResult();

	static bool HandleResults(BMessage *result, MIMEPanel *target);
private:
	bool CheckNameUnique(const char *name, bool internal) const;
	static int32 ModificationSignature()
		{ return M_ATTRIBUTE_CHANGE; }
	static int32 AdditionSignature()
		{ return M_ATTRIBUTE_ADD; }

	BTextControl *textItem;
	BTextControl *internalNameTextItem;
	BMenuField *typeMenu;
	BCheckBox *editableCheckBox;
	BCheckBox *viewableCheckBox;
	BCheckBox *extraCheckBox;
	BTextControl *trackerWidth;
	BMenuField *alignmentMenu;
};

class TypeNameEditor : public AttributeEditor , private BHandler {
public:
	TypeNameEditor(BRect frame, BView *target, bool modification,
		const char *fullSignature, const char *shortName, 
		const char *superType);
	virtual ~TypeNameEditor();
	
	void SetUpData(const char *fullSignature, const char *shortName, 
		const char *superType);
	virtual bool ShouldEnableAdd() const;
	
	virtual bool ReturnResult();	
	static bool HandleResults(BMessage *result, MIMEPanel *target);

private:
	bool CheckNameUnique(const char *name) const;
	bool CheckGroupUnique(const char *group) const;

	static uint32 ModificationSignature()
		{ return M_MIME_TYPE_CHANGE; }
	static uint32 AdditionSignature()
		{ return M_MIME_TYPE_ADD; }

	virtual void MessageReceived(BMessage *);

	BTextControl *textItem;
	BTextControl *crypticEditableName;
	BStringView *crypticName;
	BMenuField *superTypeMenu;
	BStringView *superTypeName;

};

class TypeSelector : public AttributeEditor {
public:
	TypeSelector(BRect frame, BView *target, bool showSupertypes,
		bool adding = false);
	virtual ~TypeSelector() {}
	
	virtual bool ShouldEnableAdd() const;
	
	virtual bool ReturnResult();

	static bool HandleResults(BMessage *result, MIMEPanel *target);
private:

	BListView *list;
};

class ReadOnlyTypeNameDialog : public GenericEditDialog {
public:
	ReadOnlyTypeNameDialog(BRect centerInRect,
		const char *fullSignature, const char *shortName, 
		const char *superType);
	
};

class ReadOnlyGroupNameDialog : public GenericEditDialog {
public:
	ReadOnlyGroupNameDialog(BRect centerInRect, const char *groupName);
};

class GroupNameDialog : public GenericEditDialog {
public:
	GroupNameDialog(BRect centerInRect, BHandler *target);
private:
	virtual bool Done();
	virtual bool ShouldEnableOK();
	
	BTextControl *groupName;
	BHandler *target;
};


#endif
