#ifndef QUICKRES_RES_SUPPORT_H
#define QUICKRES_RES_SUPPORT_H

// Various support classes used by ResWindow.

#include <ResourceEntry.h>
#include <ResourceListView.h>
#include <ResourceParser.h>
#include <ResourceRoster.h>
#include <UndoContext.h>

#include <experimental/Order.h>
#include <controls/ColumnTypes.h>

#include <Messenger.h>
#include <View.h>

#include <String.h>

#include <vector>

class ResWindow;

class ResEntry : public ResourceEntry
{
public:
	ResEntry(BResourceCollection* collection);
	ResEntry(BResourceCollection* collection,
			 type_code type, int32 id, const char* name, const char* sym);
	ResEntry(BResourceCollection* collection, BMessage* from);
	ResEntry(const BResourceItem& o);
	ResEntry(const ResEntry& data);

	virtual void NoteChange(uint32 what);
	
	void SetMakeSelected(bool state=true)		{ fMakeSelected = state; }
	bool MakeSelected() const					{ return fMakeSelected; }
	
	void SetIsAttribute(bool state)				{ fIsAttribute = state; }
	bool IsAttribute() const					{ return fIsAttribute; }
	
	void SetIsBody(bool state)					{ fIsBody = state; }
	bool IsBody() const							{ return fIsBody; }
	
	void SetModified(bool state) const			{ const_cast<ResEntry*>(this)->fModified = state; }
	bool Modified() const						{ return fModified; }
	
protected:
	virtual ~ResEntry()
	{
	}

private:
	bool fMakeSelected;
	bool fIsAttribute;
	bool fIsBody;
	bool fModified;
};

class ResRow : public ResourceRow
{
public:
	ResRow(ResourceEntry *e)	: ResourceRow(e) { }

protected:
	virtual bool IsModified(const BResourceItem* item) const;
	virtual bool IsAttribute(const BResourceItem* item) const;
	virtual bool IsBody(const BResourceItem* item) const;
};

class ResUndoEntry : public BUndoOperation
{
public:
	ResUndoEntry(BResourceCollection& collection, BResourceHandle owner);
	ResUndoEntry(BResourceCollection& collection, ResourceEntry* data);
	~ResUndoEntry();
	
	void UpdateFrom(const BResourceItem* from, uint32 what);
	
	virtual const void* Owner() const;

	virtual bool HasData() const;
	
	virtual void Commit();
	virtual void Undo();
	virtual void Redo();
	
	void SwapData();
	
private:
	BResourceCollection& fCollection;
	BResourceHandle fOwner;
	BResourceItem fData;
};

class CollectionUndoEntry : public BUndoOperation
{
public:
	CollectionUndoEntry(ResWindow& window);
	~CollectionUndoEntry();
	
	bool AddItem(BResourceHandle item);
	bool RemoveItem(BResourceHandle item);
	
	virtual const void* Owner() const			{ return &fWindow; }

	virtual bool HasData() const;
	
	virtual void Commit();
	virtual void Undo();
	virtual void Redo();
	void SwapData(bool swap=true);
	
private:
	ResWindow& fWindow;
	vector<BResourceHandle> fAdded;
	vector<BResourceHandle> fRemoved;
};

class ResParser : public BResourceParser
{
public:
	ResParser(ResWindow& win, bool merge);
	~ResParser();

	virtual status_t ReadItem(BResourceItem* item);
	
	int32 NumberRead() const;
	
protected:
	virtual BResourceItem* NewItem(type_code type = 0, int32 id = 0,
									const char* name = 0,
									const char* symbol = 0) const;
	
private:
	ResWindow& fWindow;
	bool fMerge;
	int32 fReadCount;
};

class SaveOrder : public TOrderBase
{
public:
	SaveOrder(const BResourceCollection& collection,
			  const vector<BResourceHandle>& items);
	
	virtual int Compare(size_t e1, size_t e2) const;
	
private:
	const BResourceCollection& fCollection;
	const vector<BResourceHandle>& fItems;
};

class AttributeOrder : public TOrderBase
{
public:
	AttributeOrder(const BResourceCollection& collection,
			  const vector<BResourceHandle>& items);
	
	virtual int Compare(size_t e1, size_t e2) const;
	
private:
	const BResourceCollection& fCollection;
	const vector<BResourceHandle>& fItems;
};

// ---------------------------- ColumnContainer ----------------------------

class ColumnContainer : public BView
{
public:
	ColumnContainer(BRect rect, const char* name, float spacer,
					uint32 resizeMask = B_FOLLOW_NONE,
					uint32 flags = B_FRAME_EVENTS);
	~ColumnContainer();
	
	virtual	void AttachedToWindow();
	virtual void AllAttached();
	virtual void FrameResized(float width, float height);
	
	void AddColumnView(BView* view);
	void AddButton(BView* view);
	
	void Layout(float width, float height);
	
	virtual	void GetPreferredSize(float *width, float *height);

	virtual	void GetButtonSize(float *width, float *height);

private:
	enum {
		MAX_BUTTONS = 10
	};
	
	float fSpacer;
	BView* fColumns;
	BView* fButtons[MAX_BUTTONS];
};

// ---------------------------- ViewWrapper ----------------------------

class ViewWrapper : public BView
{
public:
	ViewWrapper(BRect rect, const char* name,
					uint32 resizeMask = B_FOLLOW_NONE,
					uint32 flags = 0);
	~ViewWrapper();
	
	virtual	void AttachedToWindow();
	virtual	void GetPreferredSize(float *width, float *height);

private:
};

#endif
