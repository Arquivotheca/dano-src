#ifndef _RESOURCE_LIST_VIEW_H
#define _RESOURCE_LIST_VIEW_H

#include <ResourceEntry.h>
#include <ResourceRoster.h>
#include <ResourceEditor.h>

#include <ToolTipHook.h>

#include <controls/ColumnListView.h>
#include <controls/ColumnTypes.h>

#include <String.h>
#include <MessageRunner.h>

namespace BPrivate {

enum {
	kIntermediateChange = 'ichg',
	kFinalChange = 'chng',
	kAddItem = 'add ',
	kBackspaceItem = 'bspc',
	kDuplicateItem = 'dup '
};

class ColumnEditView;

// --------------------------------------------------------------------------
// A protocol for CLV BColumn subclasses that allow inplace editing, and
// implementations of it for string and integer columns.
// --------------------------------------------------------------------------

class EditColumnProtocol
{
public:
	EditColumnProtocol();
	virtual ~EditColumnProtocol();
	
	virtual void DrawEditField(BField *field, BRect rect, BView *parent);
	
	virtual bool BeginEdit(BMessenger owner, BField* field,
						   BRect rect, BColumnListView* parent);
	virtual bool ContinueEdit();
	virtual bool UpdateEdit(bool final=true);
	virtual bool EndEdit(bool final=true, bool update=true);
	
	bool HasFocus() const;
	bool HasFocus(BView* v) const;
	
protected:
	float BaselineInFrame(const BFont* font, const BRect& frame) const;
	float BaselineInFrame(const BView* owner, const BRect& frame) const;
	
	// Must be implemented by subclass.
	virtual status_t GetFieldText(BField* field, BString* to) const = 0;
	virtual status_t SetFieldText(BField* field, const char* text) = 0;
	
private:
	typedef BStringColumn inherited;
	
	BView* fParent;
	ColumnEditView* fEditView;
	BField* fEditField;
	BString fOrigText;
};

class StringEditColumn : public BStringColumn, public EditColumnProtocol
{
public:
	StringEditColumn(const char *title, float width, float maxWidth, float minWidth,
		uint32 truncate);
	~StringEditColumn();
	
	virtual void DrawField(BField *field, BRect rect, BView *parent);
	
protected:
	virtual status_t GetFieldText(BField* field, BString* to) const;
	virtual status_t SetFieldText(BField* field, const char* text);
	
private:
	typedef BStringColumn inherited;
};

class IntegerEditColumn : public BIntegerColumn, public EditColumnProtocol
{
public:
	IntegerEditColumn(const char *title, float width, float maxWidth, float minWidth);
	~IntegerEditColumn();
	
	virtual void DrawField(BField *field, BRect rect, BView *parent);
	
protected:
	virtual status_t GetFieldText(BField* field, BString* to) const;
	virtual status_t SetFieldText(BField* field, const char* text);
	
private:
	typedef BIntegerColumn inherited;
};

// --------------------------------------------------------------------------
// CLV field and column for resource IDs.
// --------------------------------------------------------------------------

class IDField : public BIntegerField {
public:
	IDField(int32 number, bool is_attribute=false);
	
	bool fAttribute;
	bool fBody;
};

class IDEditColumn : public IntegerEditColumn
{
public:
	IDEditColumn(const char *title, float width, float maxWidth, float minWidth);
	~IDEditColumn();
	
	virtual void DrawField(BField *field, BRect rect, BView *parent);
	
	virtual bool BeginEdit(BMessenger owner, BField* field,
						   BRect rect, BColumnListView* parent);
						   
	virtual int CompareFields(BField *field1, BField *field2);
	
private:
	typedef IntegerEditColumn inherited;
};

// --------------------------------------------------------------------------
// CLV field and column for resource data (displaying a mini-editor).
// --------------------------------------------------------------------------

class ResourceField : public BField
{
public:
	ResourceField(BResourceHandle& handle);
	virtual ~ResourceField();
	
	const BResourceHandle& Handle() const;
	BResourceHandle& Handle();
	
	void DataChanged();
	void TypeChanged();
	
	void SetCurrentAddon(BResourceAddon* addon);
	BResourceAddon* CurrentAddon(const BResourceAddonBase& base,
								 BResourceRoster& roster) const;
	
	BMiniItemEditor* Editor(const BResourceAddonBase& base,
							BResourceRoster& roster) const;
	BMiniItemEditor* Editor() const;
	
private:
	BResourceHandle& fHandle;
	BResourceAddon* fAddon;
	BMiniItemEditor* fEditor;
};

class ResourceEditColumn : public BTitledColumn,
						   public EditColumnProtocol,
						   public BResourceAddonBase
{
public:
	ResourceEditColumn(const char* title, float width, float maxWidth, float minWidth,
					   const BResourceAddonArgs& args,
					   BResourceRoster& roster);
	~ResourceEditColumn();
	
	virtual void DrawField(BField *field, BRect rect, BView *targetView);
	virtual int CompareFields(BField *field1, BField *field2);
	
protected:
	virtual status_t GetFieldText(BField* field, BString* to) const;
	virtual status_t SetFieldText(BField* field, const char* text);
	
private:
	typedef BTitledColumn inherited;
	
	BResourceRoster& fRoster;
};

// --------------------------------------------------------------------------
// CLV row for a single BResourceItem, which automatically generates fields
// for the following COLUMN_* IDs.
// --------------------------------------------------------------------------

// indicies of columns in list view
enum {
	COLUMN_MOD = 0,
	COLUMN_TYPE,
	COLUMN_ID,
	COLUMN_NAME,
	COLUMN_DATA,
	COLUMN_SIZE
};

class ResourceRow : public BRow, public BResourceHandle
{
public:
	ResourceRow(ResourceEntry *e);

	bool Update(const BResourceAddonBase& acc, uint32 changes, bool force = false);
	const char* Write(BResourceItem* item);
	
	bool ApplyChanges();
	
	void SetCurrentAddon(BResourceAddon* addon);
	BResourceAddon* CurrentAddon(const BResourceAddonBase& base,
								 BResourceRoster& roster);
	
	void SetWindowEditor(BMessenger who)		{ fWindowEditor = who; }
	BMessenger WindowEditor() const				{ return fWindowEditor; }

protected:
	// Hooks to return extended information about the row's resource item.
	virtual bool IsModified(const BResourceItem* /*item*/) const	{ return false; }
	virtual bool IsAttribute(const BResourceItem* /*item*/) const	{ return false; }
	virtual bool IsBody(const BResourceItem* /*item*/) const		{ return false; }
	
private:
	ResourceField* fResField;
	BMessenger fWindowEditor;
};

// --------------------------------------------------------------------------
// BColumnListView for displaying resource data.
// --------------------------------------------------------------------------

// flags for SetupColumns()
enum {
	COLUMN_MOD_MASK = 1<<COLUMN_MOD,
	COLUMN_TYPE_MASK = 1<<COLUMN_TYPE,
	COLUMN_ID_MASK = 1<<COLUMN_ID,
	COLUMN_NAME_MASK = 1<<COLUMN_NAME,
	COLUMN_DATA_MASK = 1<<COLUMN_DATA,
	COLUMN_SIZE_MASK = 1<<COLUMN_SIZE,
	COLUMN_ALL_MASK = 0xff
};

class ResourceListView : public BColumnListView, public BToolTipable
{
public:
	ResourceListView(BRect frame,
					const char *name,
					uint32 resizingMode,
					uint32 drawFlags = B_NAVIGABLE,
					border_style style = B_FANCY_BORDER);
	~ResourceListView();
	
	void SetupColumns(BResourceCollection& col, BResourceRoster& roster,
					  uint32 flags=COLUMN_ALL_MASK);
	
	virtual	void KeyDown(const char *bytes, int32 numBytes);
	virtual void MessageReceived(BMessage* msg);
	virtual void MessageDropped(BMessage* msg, BPoint point);
	virtual void MakeFocus(bool focusState=true);
	virtual void SelectionChanged();
	virtual	void GetPreferredSize(float *width, float *height);
	
	virtual void ItemInvoked();
	
	virtual status_t GetToolTipInfo(BPoint where, BRect* out_region,
									BResEditor::BToolTipInfo* out_info = 0);
	
	#if defined(B_BEOS_VERSION_DANO)
	// SHHHHH!
	virtual status_t GetToolTipInfo(BPoint where, BRect* out_region,
									::BToolTipInfo* out_info = 0)
	{
		return BView::GetToolTipInfo(where, out_region, out_info);
	}
	#endif
	
	// NOTE: This should be made virtual in BColumnListView.
	void UpdateRow(BRow*);
	
	void BeginEdit(bool force=true);
	void UpdateEdit(BMessage* report = 0, bool final=true);
	bool ContinueEdit();
	void EndEdit(bool update=true);
	
	int32 FocusColumn() const;
	
	const BRow* EditRow() const;
	int32 EditIndex() const;
	
private:
	typedef BColumnListView inherited;
	int32 fEditRow;
	BMessageRunner* fClickDelay;
	bool fQuickSelect;
	BRow* fLastSelection;
	
	// Last edit state.
	int32 fLastColumn;
	int32 fLastFocus;
};

}	// namespace BPrivate
using namespace BPrivate;

#endif
