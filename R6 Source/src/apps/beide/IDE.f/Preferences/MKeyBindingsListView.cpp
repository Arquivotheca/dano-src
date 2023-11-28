//==================================================================
//	MKeyBindingsListView.cpp
//	Copyright 97 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include "MKeyBindingsListView.h"
#include "MIDECommandList.h"
#include "MKeyIcons.h"
#include "IDEMessages.h"

#include <Looper.h>
#include <Message.h>

// ---------------------------------------------------------------------------
//		MKeyBindingsListView
// ---------------------------------------------------------------------------
//	Constructor

MKeyBindingsListView::MKeyBindingsListView(
	BRect			inFrame,
	const char*		inName,
	BView*			inOwner)
	: MTriangleListView(
		inFrame,
		inName),
	fOwner(inOwner), fInvokedRow(-1), fManager(nil)
{
}

// ---------------------------------------------------------------------------
//		~MKeyBindingsListView
// ---------------------------------------------------------------------------
//	Destructor

MKeyBindingsListView::~MKeyBindingsListView()
{
	// Need to delete it here because our DeleteItem won't be called
	// from the MListView destructor
	if (CountRows() > 0)
		DeleteRows(0, CountRows());
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------

const float kPrimaryLeft = 150.0;
const float kPrimaryRight = 200.0;
const float kSecondaryLeft = kPrimaryRight;
const float kSecondaryRight = 300.0;

void
MKeyBindingsListView::DrawRow(
	int32 	inRow,
	void * 	inData,
	BRect	inArea,
	BRect	inIntersection)
{
	ASSERT(fManager);

	// Draw the triangle if there is one and set the penposition to draw
	// the name
	MTriangleListView::DrawRow(inRow, inData, inArea, inIntersection);

	// The row draws its name and keybinding
	ListRowData*		rowData = (ListRowData*) inData;
	rowData->Draw(this, inArea, *fManager);
}

// ---------------------------------------------------------------------------
//		SetBindingManager
// ---------------------------------------------------------------------------

void
MKeyBindingsListView::SetBindingManager(
	MKeyBindingManager*		inManager)
{
	fManager = inManager;
}

// ---------------------------------------------------------------------------
//		DeleteItem
// ---------------------------------------------------------------------------

void
MKeyBindingsListView::DeleteItem(
	void * inItem)
{
	delete (BindingRowData*) inItem;
}

// ---------------------------------------------------------------------------
//		InvokeRow
// ---------------------------------------------------------------------------

void
MKeyBindingsListView::InvokeRow(
	int32	inRow)
{
	ASSERT(inRow >=0 && inRow < CountRows());
	ListRowData * 		item = (ListRowData *) GetList()->ItemAt(inRow);
	BindingRowData*		data = dynamic_cast<BindingRowData*>(item);

	// Ignore groups, for binding rows set up the binding window
	if (data)
	{
		PrefixRowData*	prefixRow = dynamic_cast<PrefixRowData*>(data);
		BMessage		msg(msgSetBinding);
			
		if (prefixRow)
		{
			KeyBindingInfo		info = prefixRow->info;
			
			info.binding1.prefixIndex = prefixRow->fIndex;
			msg.AddBool(kIsPrefixKey, true);
			msg.AddData(kBindingInfo, kBindInfoType, &info, sizeof(info));
		}
		else
			msg.AddData(kBindingInfo, kBindInfoType, &data->info, sizeof(data->info));

		msg.AddString(kName, data->name);
		msg.AddInt32(kBindingContext, data->context);

		fOwner->Looper()->PostMessage(&msg, fOwner);
		
		fInvokedRow = GetWideOpenIndex(inRow);
	}
}

// ---------------------------------------------------------------------------
//		UpdateBinding
// ---------------------------------------------------------------------------

void
MKeyBindingsListView::UpdateBinding(
	KeyBindingContext		/* inContext */,
	const KeyBindingInfo&	inInfo,
	MKeyBindingManager&		inManager)
{
	BindingRowData*		bindingRow = (BindingRowData*) RowData(fInvokedRow);

	bindingRow->UpdateBinding(inInfo, inManager);
	
	int32		visibleRow = GetVisibleIndex(fInvokedRow);
	
	if (visibleRow >= 0)
		InvalidateRow(visibleRow);
}

// ---------------------------------------------------------------------------
//		UpdateAllBindings
// ---------------------------------------------------------------------------

void
MKeyBindingsListView::UpdateAllBindings(
	const MKeyBindingManager&		inManager)
{
	KeyBindingInfo		binding;
	const int32			rowCount = WideOpenRowCount();

	// Update the regular bindings
	for (int i = 1; i < rowCount; ++i)
	{
		ListRowData*		row = static_cast<ListRowData*>(RowData(i));
		BindingRowData*		bindingRow = dynamic_cast<BindingRowData*>(row);

		if (bindingRow != nil)
		{
			if (! inManager.GetBinding(bindingRow->context, bindingRow->info.cmdNumber, binding))
			{
				binding.cmdNumber = bindingRow->info.cmdNumber;
				binding.binding1 = kEmptyBinding;
				binding.binding2 = kEmptyBinding;
			}
			
			bindingRow->info = binding;
		}
	}
	
	binding.binding2 = kEmptyBinding;

	// Update the prefix bindings
	for (int i = 0; i < kBindingPrefixCount; ++i)
	{
		ListRowData*		row = static_cast<ListRowData*>(RowData(fFirstPrefixRow + i));
		PrefixRowData*		bindingRow = dynamic_cast<PrefixRowData*>(row);

		ASSERT(bindingRow != nil);
	
		inManager.GetPrefixBinding(i+1, binding.binding1);
		
		bindingRow->info = binding;
	}
	
	Invalidate();
}

// ---------------------------------------------------------------------------
//		NewRowObject
// ---------------------------------------------------------------------------

BindingRowData*
MKeyBindingsListView::NewRowObject(
	CommandT		inCommand,
	const char *	inName,
	int32			inGroupID,
	int32			inIndex)		// index in the group
{
	BindingRowData*		rec;
	KeyBindingInfo		binding;
	KeyBindingContext	context = kBindingGlobal;

	switch (inGroupID)
	{
		case kPrefixGroupID:
			inIndex++;
			MKeyBindingManager::Manager().GetPrefixBinding(inIndex, binding.binding1);
			rec = new PrefixRowData(binding, inName, inIndex);		
			break;

		case kEditorGroupID:
			context = kBindingEditor;
			// fall through on purpose
		default:
			KeyBindingContext	context = kBindingGlobal;
			if (inGroupID == kEditorGroupID)
				context = kBindingEditor;
			if (! MKeyBindingManager::Manager().GetBinding(context, inCommand, binding))
			{
				binding.cmdNumber = inCommand;
				binding.binding1 = kEmptyBinding;
				binding.binding2 = kEmptyBinding;
			}
			rec = new BindingRowData(binding, context, inName);
			break;
	}

	return rec;
}

// ---------------------------------------------------------------------------
//		BuildBindingList
// ---------------------------------------------------------------------------

void
MKeyBindingsListView::BuildBindingList()
{
	CommandGroupInfo	info;
	int32				groupID = 0;

	while (MIDECommandList::GetNthCommandGroup(groupID, info))
	{
		GroupRowData*	rec = new GroupRowData(info.name);
		int32			index = CountRows();
		InsertCollapsableRow(index, rec);
		
		if (groupID == kPrefixGroupID)
			fFirstPrefixRow = WideOpenRowCount();
	
		for (int j = 0; j < info.count; ++j)
		{
			BindingRowData*	rec = NewRowObject(info.commands[j].command, info.commands[j].name, groupID, j);
			InsertRow(CountRows(), rec);
		}
	
		ContractRow(index);
		groupID++;
	}
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
ListRowData::Draw(
	MTriangleListView*			inView,
	BRect						/*inArea*/,
	const MKeyBindingManager&	/*inManager*/)
{
	// Draw the name
	inView->DrawString(name);
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
GroupRowData::Draw(
	MTriangleListView*			inView,
	BRect						/*inArea*/,
	const MKeyBindingManager&	/*inManager*/)
{
	// Draw the name
	inView->SetFont(be_bold_font);
	inView->DrawString(name);
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
BindingRowData::Draw(
	MTriangleListView*			inView,
	BRect						inArea,
	const MKeyBindingManager&	inManager)
{
	// Draw the name
	inView->SetFont(be_plain_font);
	inView->DrawString(name);
	
	// Draw the bindings
	inView->SetFont(be_bold_font);
	BRect		bindRect(kPrimaryLeft, inArea.top, kPrimaryRight, inArea.bottom);

	MKeyIcons::DrawKeyBinding(inView, bindRect, info.binding1, inManager, B_ALIGN_RIGHT);

	bindRect.left = kSecondaryLeft;
	bindRect.right = kSecondaryRight;
	MKeyIcons::DrawKeyBinding(inView, bindRect, info.binding2, inManager, B_ALIGN_RIGHT);
}

// ---------------------------------------------------------------------------
//		UpdateBinding
// ---------------------------------------------------------------------------

void
BindingRowData::UpdateBinding(
	const KeyBindingInfo&		inInfo,
	MKeyBindingManager&			inManager)
{
	info = inInfo;
	inManager.SetBinding(context, inInfo);
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
PrefixRowData::Draw(
	MTriangleListView*			inView,
	BRect						inArea,
	const MKeyBindingManager&	/*inManager*/)
{
	// Draw the name
	inView->SetFont(be_plain_font);
	inView->DrawString(name);
	
	// Draw the bindings
	inView->SetFont(be_bold_font);
	BRect		bindRect(kPrimaryLeft, inArea.top, kPrimaryRight, inArea.bottom);

	MKeyIcons::DrawKeyBinding(inView, bindRect, info.binding1, B_ALIGN_RIGHT);
}

// ---------------------------------------------------------------------------
//		UpdateBinding
// ---------------------------------------------------------------------------

void
PrefixRowData::UpdateBinding(
	const KeyBindingInfo&		inInfo,
	MKeyBindingManager&			inManager)
{
	info = inInfo;
	info.binding1.prefixIndex = 0;
	inManager.SetPrefixBinding(fIndex, info.binding1);
}

