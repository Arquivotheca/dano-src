#include "ResSupport.h"
#include "ResWindow.h"

#include <MenuField.h>
#include <MenuItem.h>

#include <Autolock.h>
#include <Debug.h>

#include <RecentItems.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// ---------------------------- ResEntry ----------------------------
	
ResEntry::ResEntry(BResourceCollection* collection)
	: ResourceEntry(collection),
	  fMakeSelected(false), fIsAttribute(false), fIsBody(false)
{
}

ResEntry::ResEntry(BResourceCollection* collection,
		 type_code type, int32 id, const char* name, const char* sym)
	: ResourceEntry(collection, type, id, name, sym),
	  fMakeSelected(false), fIsAttribute(false), fIsBody(false)
{
}

ResEntry::ResEntry(BResourceCollection* collection, BMessage* from)
	: ResourceEntry(collection, from),
	  fMakeSelected(false), fIsAttribute(false), fIsBody(false)
{
}

ResEntry::ResEntry(const BResourceItem& o)
	: ResourceEntry(o),
	  fMakeSelected(false), fIsAttribute(false), fIsBody(false)
{
}

ResEntry::ResEntry(const ResEntry& data)
	: ResourceEntry(data),
	  fMakeSelected(false),
	  fIsAttribute(data.fIsAttribute),
	  fIsBody(data.fIsBody)
{
	SetCollection(data.Collection());
	SetOwner(data.Owner());
}

void ResEntry::NoteChange(uint32 what)
{
	PRINT(("Noting change of item %p (%ld, %lx, %s, %s): 0x%lx, old: 0x%lx\n",
			this, ID(), Type(), Name(), Symbol(), what, Changes()));
	
	if( what ) SetModified(true);
	
	BUndoContext* undo = Collection()->UndoContext();
	if( ((~Changes())&what) != 0 && undo && what ) {
		// If this different than the last change,
		// stash new values into undo.
		PRINT(("New change: adding to undo manager.\n"));
		ResUndoEntry* u = dynamic_cast<ResUndoEntry*>(undo->LastOperation(this));
		if( !u ) {
			if( !Collection() ) debugger("Resource entry does not have collection");
			u = new ResUndoEntry(*Collection(), this);
			undo->AddOperation(u);
		}
		if( u ) {
			PRINT(("Updating changes 0x%lx, owner=%p\n",
					(~Changes())&what, u->Owner()));
			u->UpdateFrom(this, (~Changes())&what);
		}
	}
	
	ResourceEntry::NoteChange(what);
}

// ---------------------------- ResUndoEntry ----------------------------
	
bool ResRow::IsModified(const BResourceItem* item) const
{
	const ResEntry* e = dynamic_cast<const ResEntry*>(item);
	return e ? e->Modified() : false;
}

bool ResRow::IsAttribute(const BResourceItem* item) const
{
	const ResEntry* e = dynamic_cast<const ResEntry*>(item);
	return e ? e->IsAttribute() : false;
}

bool ResRow::IsBody(const BResourceItem* item) const
{
	const ResEntry* e = dynamic_cast<const ResEntry*>(item);
	return e ? e->IsBody() : false;
}

// ---------------------------- ResUndoEntry ----------------------------
	
ResUndoEntry::ResUndoEntry(BResourceCollection& collection, BResourceHandle owner)
	: fCollection(collection), fOwner(owner)
{
	fData.ClearChanges();
}

ResUndoEntry::ResUndoEntry(BResourceCollection& collection, ResourceEntry* data)
	: fCollection(collection), fOwner(data)
{
	fData.ClearChanges();
}

ResUndoEntry::~ResUndoEntry()
{
}

void ResUndoEntry::UpdateFrom(const BResourceItem* from, uint32 what)
{
	fData.UpdateFrom(from, what&(~fData.Changes()));
}

const void* ResUndoEntry::Owner() const
{
	return fOwner.ItemAddress();
}

bool ResUndoEntry::HasData() const
{
	return fData.Changes() != 0;
}

void ResUndoEntry::Commit()
{
}

void ResUndoEntry::Undo()
{
	SwapData();
}

void ResUndoEntry::Redo()
{
	SwapData();
}

void ResUndoEntry::SwapData()
{
	if( fOwner.IsValid() ) {
		BResourceItem* target = fCollection.WriteItem(fOwner);
		PRINT(("Undo: changing data in target %p\n", target));
		if( target ) {
			BResourceItem tmp;
			tmp.UpdateFrom(target, fData.Changes());
			target->UpdateFrom(&fData, fData.Changes());
			fData.UpdateFrom(&tmp, fData.Changes());
		}
	}
}

// ---------------------------- CollectionUndoEntry ----------------------------

CollectionUndoEntry::CollectionUndoEntry(ResWindow& window)
	: fWindow(window)
{
}

CollectionUndoEntry::~CollectionUndoEntry()
{
}

bool CollectionUndoEntry::AddItem(BResourceHandle item)
{
	fAdded.push_back(item);
	return true;
}

bool CollectionUndoEntry::RemoveItem(BResourceHandle item)
{
	fRemoved.push_back(item);
	return true;
}

bool CollectionUndoEntry::HasData() const
{
	return fAdded.size() != 0 || fRemoved.size() != 0;
}

void CollectionUndoEntry::Commit()
{
}

void CollectionUndoEntry::Undo()
{
	SwapData();
}

void CollectionUndoEntry::Redo()
{
	SwapData();
}

void CollectionUndoEntry::SwapData(bool swap)
{
	if( swap ) {
		fAdded.swap(fRemoved);
	}
	
	size_t i;
	for( i=0; i<fAdded.size(); i++ ) {
		ResEntry* e = dynamic_cast<ResEntry*>(fWindow.WriteItem(fAdded[i]));
		if( e ) {
			fWindow.SetCurrentItem(BResourceHandle());
			e->SetMakeSelected(true);
		}
		fWindow.PerformAdd(fAdded[i]);
	}
	for( i=0; i<fRemoved.size(); i++ ) {
		fWindow.PerformRemove(fRemoved[i]);
	}
}
	
// ---------------------------- ResParser ----------------------------
	
ResParser::ResParser(ResWindow& win, bool merge)
	: fWindow(win), fMerge(merge), fReadCount(0)
{
}

ResParser::~ResParser()
{
}

status_t ResParser::ReadItem(BResourceItem* item)
{
	status_t err = B_ERROR;
	
	ResEntry* e = dynamic_cast<ResEntry*>(item);
	if( e ) {
		e->SetModified(fMerge);
		err = fWindow.AddEntry(e, false);
	} else {
		TRESPASS();
		ResEntry* e = new ResEntry(&fWindow, item->Type(), item->ID(),
								   item->Name(), item->Symbol());
		if( e ) {
			e->SetData(item->Data(), item->Size());
			e->SetModified(fMerge);
			err = fWindow.AddEntry(e, false);
		}
		//BResourceHandle handle;
		//err = fWindow.AddItem(&handle, item->Type(), item->ID(),
		//				buf.String(), item->Data(), item->Size(), false);
		delete item;
	}
	
	if( err == B_OK ) fReadCount++;
	return err;
}

int32 ResParser::NumberRead() const
{
	return fReadCount;
}

BResourceItem* ResParser::NewItem(type_code type, int32 id,
								const char* name, const char* sym) const
{
	return new ResEntry(&fWindow, type, id, name, sym);
}

// ---------------------------- SaveOrder ----------------------------

SaveOrder::SaveOrder(const BResourceCollection& collection,
		  const vector<BResourceHandle>& items)
	: TOrderBase(items.size()),
	  fCollection(collection), fItems(items)
{
}

int SaveOrder::Compare(size_t e1, size_t e2) const
{
	const BResourceItem* i1 = fCollection.ReadItem(fItems[e1]);
	const BResourceItem* i2 = fCollection.ReadItem(fItems[e2]);
	if( i1 == 0 || i2 == 0 ) {
		if( i2 != 0 ) return -1;
		if( i1 != 0 ) return 1;
		return 0;
	}
	if( i1->Type() < i2->Type() ) return -1;
	if( i1->Type() > i2->Type() ) return 1;
	if( i1->ID() < i2->ID() ) return -1;
	if( i1->ID() > i2->ID() ) return 1;
	return 0;
}

// ---------------------------- AttributeOrder ----------------------------

AttributeOrder::AttributeOrder(const BResourceCollection& collection,
		  const vector<BResourceHandle>& items)
	: TOrderBase(items.size()),
	  fCollection(collection), fItems(items)
{
}

int AttributeOrder::Compare(size_t e1, size_t e2) const
{
	const ResEntry* i1 = dynamic_cast<const ResEntry*>(fCollection.ReadItem(fItems[e1]));
	const ResEntry* i2 = dynamic_cast<const ResEntry*>(fCollection.ReadItem(fItems[e2]));
	if( i1 == 0 || i2 == 0 ) {
		if( i2 != 0 ) return -1;
		if( i1 != 0 ) return 1;
		return 0;
	}
	if( i1->IsAttribute() != i2->IsAttribute() ) {
		if( i1->IsAttribute() ) return -1;
		return 1;
	}
	if( i1->Size() < i2->Size() ) return -1;
	if( i1->Size() > i2->Size() ) return 1;
	BString buf1, buf2;
	const char* n1 = i1->Name();
	const char* n2 = i2->Name();
	if (!n1 || !n2) {
		if (!n1 && !n2) {
			n1 = i1->Symbol();
			n2 = i2->Symbol();
		}
		if (!n1 || !n2) {
			n1 = i1->CreateLabel(&buf1);
			n2 = i2->CreateLabel(&buf2);
		}
	}
	if( n1 == 0 || n2 == 0 ) {
		if( n2 != 0 ) return -1;
		if( n1 != 0 ) return 1;
		return 0;
	}
	return strcmp(n1, n2);
}

// ---------------------------- ColumnContainer ----------------------------

ColumnContainer::ColumnContainer(BRect rect, const char* name, float spacer,
								uint32 resizeMask, uint32 flags)
	: BView(rect, name, resizeMask, flags),
	  fSpacer(spacer), fColumns(0)
{
	for( int32 i=0; i<MAX_BUTTONS; i++ ) fButtons[i] = 0;
}

ColumnContainer::~ColumnContainer()
{
}

void ColumnContainer::AttachedToWindow()
{
	if( Parent() ) SetViewColor(Parent()->ViewColor());
}

void ColumnContainer::AllAttached()
{
	Layout(Bounds().Width(), Bounds().Height());
}

void ColumnContainer::FrameResized(float width, float height)
{
	Layout(width, height);
}

void ColumnContainer::AddColumnView(BView* view)
{
	AddChild(view, ChildAt(0));
	fColumns = view;
}

void ColumnContainer::AddButton(BView* view)
{
	if( !view ) return;
	
	int32 i=0;
	while( i < MAX_BUTTONS && fButtons[i] != 0 ) i++;
	if( i < MAX_BUTTONS ) {
		fButtons[i] = view;
		AddChild(view);
	} else {
		TRESPASS();
	}
}

static void generic_preferred_size(BView* view, float* width, float* height,
								   bool layout = false)
{
	BMenuField* field = dynamic_cast<BMenuField*>(view);
	if( field ) {
		BMenu* popup = field->Menu();
		
		font_height fhs;
		field->GetFontHeight(&fhs);
		const float fh = fhs.ascent+fhs.descent+fhs.leading;
		float fw = 0;
		
		float pref_w = 0;
		if( popup && popup->IsLabelFromMarked() ) {
			fw = field->StringWidth("WWWW");
			int32 num = popup->CountItems();
			for( int32 i=0; i<num; i++ ) {
				BMenuItem* item = popup->ItemAt(i);
				if( item ) {
					const float w=field->StringWidth(item->Label());
					if( w > pref_w ) pref_w = w;
				}
			}
		} else {
			pref_w = field->StringWidth(popup->Name());
		}
		
		float lw = (field->Label() && *field->Label())
			? field->StringWidth(field->Label()) + field->StringWidth(" ") + 5
			: 0;
		if( layout ) field->SetDivider(lw);
		*width = floor((fw>pref_w?fw:pref_w) + 20 + lw + .5);
		*height = floor(fh + 8 + .5);
	}
	
	view->GetPreferredSize(width, height);
}

void ColumnContainer::Layout(float width, float height)
{
	if( Window() ) Window()->BeginViewTransaction();
	
	float butw=0, buth=0;
	GetButtonSize(&butw, &buth);

	if( fColumns ) {
		fColumns->ResizeTo(width, height-buth-fSpacer);
		fColumns->MoveTo(0, buth+fSpacer);
	}
	float extra = width-butw;
	
	int32 num=0;
	while( num<MAX_BUTTONS && fButtons[num] ) num++;
	
	float top=0;
	for( int32 i=0; i<MAX_BUTTONS && fButtons[i]; i++ ) {
		float w, h;
		generic_preferred_size(fButtons[i], &w, &h, true);
		fButtons[i]->MoveTo(floor(top), 0);
		fButtons[i]->ResizeTo(w, buth);
		top += w + fSpacer;
		if( num > 1 ) top += extra/(num-1);
	}
	
#if 0
	if( fColumns ) fColumns->ResizeTo(width-butw-fSpacer, height);
	float extra = height-buth;
	
	int32 num=0;
	while( num<MAX_BUTTONS && fButtons[num] ) num++;
	
	float top=0;
	for( int32 i=0; i<MAX_BUTTONS && fButtons[i]; i++ ) {
		float w, h;
		generic_preferred_size(fButtons[i], &w, &h, true);
		fButtons[i]->MoveTo(width-butw, floor(top));
		fButtons[i]->ResizeTo(butw, h);
		top += h + fSpacer;
		if( num > 1 ) top += extra/(num-1);
	}
#endif

	if( Window() ) Window()->EndViewTransaction();
}

void ColumnContainer::GetPreferredSize(float *width, float *height)
{
	*width = *height = 0;
	if( fColumns ) fColumns->GetPreferredSize(width, height);
	
	float butw=0, buth=0;
	GetButtonSize(&butw, &buth);
	
	*height += buth + fSpacer;
	if( butw > *width ) *width = butw;
#if 0
	*width = *height = 0;
	if( fColumns ) fColumns->GetPreferredSize(width, height);
	
	float butw=0, buth=0;
	GetButtonSize(&butw, &buth);
	
	*width += butw + fSpacer;
	if( buth > *height ) *height = buth;
#endif
}

void ColumnContainer::GetButtonSize(float *width, float *height)
{
	*width = *height = 0;
	for( int32 i=0; i<MAX_BUTTONS && fButtons[i]; i++ ) {
		float w, h;
		generic_preferred_size(fButtons[i], &w, &h);
		if( h > *height ) *height = h;
		*width += w + ( i > 0 ? fSpacer : 0 );
	}
#if 0
	*width = *height = 0;
	for( int32 i=0; i<MAX_BUTTONS && fButtons[i]; i++ ) {
		float w, h;
		generic_preferred_size(fButtons[i], &w, &h);
		if( w > *width ) *width = w;
		*height += h + ( i > 0 ? fSpacer : 0 );
	}
#endif
	
	PRINT(("Got button width %f, height %f\n", *width, *height));
}

// ---------------------------- ViewWrapper ----------------------------

ViewWrapper::ViewWrapper(BRect rect, const char* name,
						uint32 resizeMask, uint32 flags)
	: BView(rect, name, resizeMask, flags)
{
}

ViewWrapper::~ViewWrapper()
{
}

void ViewWrapper::AttachedToWindow()
{
	if( Parent() ) SetViewColor(Parent()->ViewColor());
}

void ViewWrapper::GetPreferredSize(float *width, float *height)
{
	BView* c = ChildAt(0);
	if( c ) c->GetPreferredSize(width, height);
	else *width = *height = 1;
}
