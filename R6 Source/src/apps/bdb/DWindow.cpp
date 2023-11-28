/*	$Id: DWindow.cpp,v 1.12 1999/05/11 21:31:09 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 12/16/98 21:45:27
*/

#include "bdb.h"
#include "DListBox.h"
#include "DWindow.h"
#include "DVariableItem.h"
#include "DMessages.h"
#include "DInternalProxy.h"
#include "DMemoryWindow.h"
#include "DTeam.h"
#include "DTeamWindow.h"
#include "DNub.h"
#include "DSettingsDialog.h"
#include "DSetWatchpoint.h"
#include "DWatchpoint.h"
#include "DViewAsDialog.h"
#include "dwarf2.h"
#include "DPointerType.h"
#include "DArrayType.h"
#include "DExprParser.h"
#include "DSourceView.h"
#include "DStackFrame.h"

#include <OutlineListView.h>
#include <Beep.h>
#include <Path.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Directory.h>
#include <String.h>
#include <Application.h>
#include <FindDirectory.h>
#include <Roster.h>

static bool sBuildingList = false;

class StListBuilder
{
  public:
	StListBuilder() { sBuildingList = true; }
	~StListBuilder() { sBuildingList = false; }
};

DWindow::DWindow(BRect frame, const char *title, window_type type, uint32 flags)
	: BWindow(frame, title, type, flags)
{
	AddShortcut('g', B_SHIFT_KEY, new BMessage(kMsgFindAgainBackward));
	AddShortcut('h', B_SHIFT_KEY, new BMessage(kMsgFindSelectionBackward));

//	AddShortcut('G', 0, new BMessage(kMsgFindAgainBackward));
//	AddShortcut('H', 0, new BMessage(kMsgFindSelectionBackward));

	AddShortcut('g', B_OPTION_KEY, new BMessage(kMsgFindAgainBackward));
	AddShortcut('h', B_OPTION_KEY, new BMessage(kMsgFindSelectionBackward));

// construct the format popup menu
	fFormatPopup = (BPopUpMenu*) HResources::GetMenu(2, true);

	// trim the trailing "sentinel" and put the addons on the popup menu
	BMenuItem* item = fFormatPopup->FindItem(kMsgAddExpression);
	FailNil(item);
	int32 index = fFormatPopup->IndexOf(item) + 1;
	do {
		item = fFormatPopup->RemoveItem(index);
		delete item;
	} while (item);
	AddAddons(fFormatPopup);

	// point it properly at this object
	fFormatPopup->SetRadioMode(false);
	fFormatPopup->SetTargetForItems(this);
} // DWindow::DWindow

DWindow::~DWindow()
{
	delete fFormatPopup;
}

void DWindow::MessageReceived(BMessage *msg)
{
	try
	{
		switch (msg->what)
		{
			case kMsgAbout:
				be_app->PostMessage(msg);
				break;

			case kMsgDoFormatPopup:
				{
					BPoint where;
					msg->FindPoint("where", &where);
					fFormatPopup->Go(where, true, true, true);	// send message, open anway, asynchronous
				}
				break;

			case kMsgFormatDefault:
				ChangeFormat(fmtDefault);
				break;
			
			case kMsgFormatSigned:
				ChangeFormat(fmtSigned);
				break;
			
			case kMsgFormatUnsigned:
				ChangeFormat(fmtUnsigned);
				break;
			
			case kMsgFormatHex:
				ChangeFormat(fmtHex);
				break;

			case kMsgFormatOctal:
				ChangeFormat(fmtOctal);
				break;
			
			case kMsgFormatChar:
				ChangeFormat(fmtChar);
				break;
			
			case kMsgFormatString:
				ChangeFormat(fmtString);
				break;
			
			case kMsgFormatFloat:
				ChangeFormat(fmtFloat);
				break;
			
			case kMsgFormatEnum:
				ChangeFormat(fmtEnum);
				break;
			
			case kMsgDumpMemory:
				DumpMemory();
				break;
			
			case kMsgAddWatchpoint:
				AddWatchpoint();
				break;
				
			case kMsgAddExpression:
				DoAddExpression();
				break;
			
			case kMsgDumpAddOn:
			{
				entry_ref ref;
				FailOSErr(msg->FindRef("refs", &ref));
				DumpAddOn(&ref);
				break;
			}
			
			case kMsgFind:
				beep();
				break;
			
			case kMsgFindAgainBackward:
				FindView("code view")->MessageReceived(msg);
				break;
			
			case kMsgFindSelectionBackward:
				FindView("code view")->MessageReceived(msg);
				break;
			
			case kMsgSelectWindow:
			{
				DWindow *w;
				FailOSErr(msg->FindPointer("window", (void **)&w));
				FailNil(w);
				w->Activate();
				break;
			}
			
			case kMsgSettings:
				DSettingsDialog::Display();
				break;
			
			case kMsgSetWatchpointCmd:
				DialogCreator<DSetWatchpoint>::CreateDialog(this);
				break;

			case kMsgSetWatchpoint:
			{
				ptr_t addr;
				FailOSErr(msg->FindInt32("address", (int32*)&addr));
				SetWatchpoint(addr);
				break;
			}
			
			case kMsgViewAsCmd:
				DialogCreator<DViewAsDialog>::CreateDialog(this);
				break;
			
			case kMsgViewAs:
			{
				const char *s;
				FailOSErr(msg->FindString("cast", &s));
				DoViewAs(s);
				break;
			}
			
			case kMsgViewAsArray:
				DoViewAsArray();
				break;
			
			default:
				BWindow::MessageReceived(msg);
				break;
		}
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} // DWindow::MessageReceived

void DWindow::ChangeFormat(EVarFormat format)
{
	int vIndex = fVariables->List()->FullListCurrentSelection();
	
	if (vIndex < 0)
		return;

	DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(vIndex));
	vItem->SetFormat(format);
	
	bool changed = vItem->Changed();
	
	DStackFrame *frame = GetStackFrame();
	if (frame)
		vItem->UpdateValue(*frame);
	else
		vItem->UpdateValue(GetTeam().GetNub());

	vItem->SetChanged(changed);

	fVariables->List()->Invalidate();
} // DWindow::ChangeFormat

bool DWindow::GetSelectedVariable(const DType *&type, ptr_t &addr, bool derefIfPointer)
{
	int vIndex = fVariables->List()->FullListCurrentSelection();
	
	if (vIndex < 0)
		return false;

	DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(vIndex));
	if (vItem == NULL)
		return false;

	DVariable *variable = vItem->Variable();
	DStackFrame *frame = GetStackFrame();

	// If we have a pointer or reference, then just get the value straight away
	// The reason for this is because the location of a pointer could be
	// a register which fouls us up.  GetValue will indirect through the register
	// automatically.
	type = variable->Type();
	if (frame && derefIfPointer && (type->IsPointer() || type->IsReference()))
	{
		uint32 size = sizeof(ptr_t);
		variable->GetValue(*frame, (void*) &addr, size);
		type = type->Deref();
	}
	else 
	{
		if (frame)
			addr = variable->GetLocation(*frame);
		else
			addr = vItem->GlobalAddress();
	}

	// if we are looking at a global variable, we still might have a pointer
	// and need to dereference
	if (frame == NULL && derefIfPointer && (type->IsPointer() || type->IsReference()))
	{
		DNub& nub(GetTeam().GetNub());
		BAutolock lock(nub);
		
		nub.Read(addr, addr);
		type = type->Deref();
	}

	return true;
} // DWindow::GetSelectedVariable

void DWindow::DumpMemory ()
{
	const DType *type;
	ptr_t addr;
	size_t size;
	bool gotVariable = true;

	try
	{
		// this will throw if it can't determine the size of the pointer's antecedent type.
		// we don't actually care about that size much, so we catch the exception and
		// just use size = 1; this guarantees that we can display void* contents etc.
		gotVariable = GetSelectedVariable(type, addr, true);
		size = type->Size();
	}
	catch (HErr& e)
	{
		size = 1;
	}

	if (!gotVariable)	// only true of GetSelectedVariable() returned false, i.e. no selection
	{
		size = 1;
		DStackFrame *frame = GetStackFrame();
		if (frame)
		{
			addr = frame->GetFP();
		}
		else 
		{
			addr = 0x80000000;
		}
	}
	
	new DMemoryWindow(GetTeam(), addr, size);

} // DWindow::DumpMemory

void DWindow::AddWatchpoint()
{
	const DType *type;
	ptr_t addr;
		
	if (GetSelectedVariable(type, addr, false))
		SetWatchpoint(addr);
} // DWindow::AddWatchpoint


static BListItem* UpdateFrame(BListItem *item, void *p)
{
	DVariableItem *vItem = dynamic_cast<DVariableItem*>(item);
	
	if (vItem)
	{
		vItem->UpdateValue(*static_cast<DStackFrame*>(p));
		vItem->SetChanged(false);
	}
	
	return NULL;
} // UpdateFrame

static BListItem* UpdateNub(BListItem *item, void *p)
{
	DVariableItem *vItem = dynamic_cast<DVariableItem*>(item);
	
	if (vItem)
	{
		vItem->UpdateValue(*static_cast<DNub*>(p));
		vItem->SetChanged(false);
	}
	
	return NULL;
} // UpdateNub

void DWindow::ExpandPointerVariable(DVariableItem *item)
{
	if (sBuildingList)
		return;
	
	StListBuilder build;

	if (GetStackFrame())
		fVariables->List()->EachItemUnder(item, false, UpdateFrame, GetStackFrame());
	else	// this one is tricky
		fVariables->List()->EachItemUnder(item, false, UpdateNub, &GetTeam().GetNub());
} // DWindow::ExpandPointerVariable

typedef status_t (*DumpVariable)(DProxy& proxy);

void DWindow::DumpAddOn(const entry_ref *ref)
{
	image_id ao = 0;

	DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(fVariables->List()->FullListCurrentSelection()));
	if (vItem == NULL)
		return;
	
	try
	{
		BEntry entry(ref);
		BPath path(&entry);
		
		ao = load_add_on(path.Path());
		if (ao < 0) THROW(("Error loading addon"));
		
		DumpVariable foo;
		FailOSErr(get_image_symbol(ao, "DumpVariable", B_SYMBOL_TYPE_TEXT, (void **)&foo));
		
		DVariable& var = *vItem->Variable();
		DStackFrame *frame = GetStackFrame();

		ptr_t addr = 0;
		if (frame)
			addr = var.GetLocation(*frame);
		else
			addr = vItem->GlobalAddress();

		DInternalProxy proxy(GetTeam().GetNub(), var.Type(), addr);
		(*foo)(proxy);
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	if (ao >= 0)
		unload_add_on(ao);
} // DWindow::DumpAddOn

static bool UpdateChangedFlag(BOutlineListView *list, BListItem *item)
{
	DVariableItem *vItem = dynamic_cast<DVariableItem*>(item);
	bool changed = false;
	
	if (vItem)
	{
		int cnt = list->CountItemsUnder(item, true);

		changed = vItem->Changed();
		
		for (int i = 0; i < cnt; i++)
		{
			if (UpdateChangedFlag(list, list->ItemUnderAt(item, true, i)))
				changed = true;
		}
		
		// const DType* type(vItem->Variable()->Type());
		
		if (changed && !vItem->Changed())// && ! type->IsPointer() && ! type->IsReference())
			vItem->SetChanged(true);
	}
	
	return changed;
} // void UpdateChangedFlag

void DWindow::UpdateVariableValues()
{
	BRect visible = fVariables->List()->Bounds();
	
	try
	{
		DStackFrame *frame = GetStackFrame();
		int i;

		if (fVariables->List()->FullListCountItems() < 200)
		{
			for (i = 0; i < fVariables->List()->FullListCountItems(); i++)
			{
				DVariableItem* item = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(i));
				
				if (item)
				{
					item->SetListBox(fVariables->List());
					
					if (frame)
						item->UpdateValue(*frame);
					else
						item->UpdateValue(GetTeam().GetNub());
				}
			}

			for (i = 0; i < fVariables->List()->CountItems(); i++)
			{
				if (fVariables->List()->ItemAt(i)->OutlineLevel() == 0)
					UpdateChangedFlag(fVariables->List(), fVariables->List()->ItemAt(i));
			}
		}
		else	// special case for Duncan's Mozilla...
		{
			BListItem *item = fVariables->List()->ItemAt(0);
	
			if (item == NULL)
				return;
			
			int32 s = std::max(fVariables->List()->IndexOf(visible.LeftTop()) - 1, 0L);
			int32 e = std::min(fVariables->List()->IndexOf(visible.LeftBottom()) + 1, fVariables->List()->CountItems());
					
			for (i = s; i < e; i++)
			{
				DVariableItem* item = dynamic_cast<DVariableItem*>(fVariables->List()->ItemAt(i));
				
				if (item)
				{
					item->SetListBox(fVariables->List());
	
					if (frame)
						item->UpdateValue(*frame);
					else
						item->UpdateValue(GetTeam().GetNub());
					
						// no more changed indication in this case, sorry...
					item->SetChanged(false);
				}
			}
		}
	}
	catch (HErr& e)
	{
		// ignore errors, what use would they be anyway?
	}
} // DWindow::UpdateVariableValues

DStackFrame *DWindow::GetStackFrame()
{
	return NULL;
} // DWindow::GetStackFrame

void DWindow::SetWatchpoint(ptr_t addr)
{
	fTeam->SetWatchpoint(DTracepoint::kPlain, addr);
} // DWindow::SetWatchpoint

void DWindow::MenusBeginning()
{
	try
	{
		BWindow::MenusBeginning();
		
		BMenuBar *mbar = static_cast<BMenuBar*>(FindView("mbar"));
		FailNil(mbar);
		
		BView *sv = FindView("code view");
		FailNil(sv);
	
		mbar->FindItem(kMsgFind)->SetTarget(sv);
		mbar->FindItem(kMsgFindAgain)->SetTarget(sv);
		mbar->FindItem(kMsgEnterSearchString)->SetTarget(sv);
		mbar->FindItem(kMsgFindSelection)->SetTarget(sv);
	
		// set up the window list
		BMenuItem *item = mbar->FindItem(kMsgSettings);
		FailNil(item);
		BMenu *menu = item->Menu();
		int32 index = menu->IndexOf(item) + 1;
		do {
			item = menu->RemoveItem(index);
			delete item;
		} while (item);
	
		DTeamWindow::FillWindowMenu(menu);
		
		// set up the addon list
		item = mbar->FindItem(kMsgAddExpression);
		FailNil(item);
		menu = item->Menu();
		index = menu->IndexOf(item) + 1;
		do {
			item = menu->RemoveItem(index);
			delete item;
		} while (item);
		
		AddAddons(menu);
	}
	catch (HErr& e)
	{
		e.DoError();
	}
} // DWindow::MenusBeginning

void
DWindow::AddAddons(BMenu *menu, const BPath &path, bool &addSeparator)
{
	BDirectory dir;
	BEntry entry;

	if (dir.SetTo(path.Path()) != B_OK)
		return;

	dir.Rewind();
	while (dir.GetNextEntry(&entry) == B_OK) {
	
		entry_ref ref;
		entry.GetRef(&ref);

		if (entry.IsSymLink()) {
			// resolve symlinks if needed
			entry.SetTo(&ref, true);
			entry.GetRef(&ref);
		}

		if (entry.InitCheck() != B_OK /*|| !entry.IsExecutable()*/)
			continue;

		// pick up the shortcut key
		uint32 key = 0;
		BString name(ref.name);
		if (name[name.Length() - 2] == '-') {
			key = name[name.Length() - 1];
			name.Truncate(name.Length() - 2);
		}
		
		if (addSeparator) {
			menu->AddSeparatorItem();
			addSeparator = false;
		}
		BMessage *message = new BMessage(kMsgDumpAddOn);
		message->AddRef("refs", &ref);
		menu->AddItem(new BMenuItem(name.String(), message, key, B_OPTION_KEY));
	}
}

void 
DWindow::AddAddons(BMenu *menu)
{
	// Addons can be at:
	// bdb_folder/AddOns/
	// ~/system/add-ons/bdb/
	// ~/config/add-ons/bdb/
	BPath path;
	bool addSeparator = true;

	app_info ai;
	FailOSErr(be_app->GetAppInfo(&ai));
	BDirectory d;
	BEntry e;

	FailOSErr(e.SetTo(&ai.ref));
	FailOSErr(e.GetParent(&d));

	if (d.FindEntry("AddOns", &e, true) == B_OK && e.GetPath(&path) == B_OK)
		AddAddons(menu, path, addSeparator);

	if (find_directory(B_BEOS_ADDONS_DIRECTORY, &path) == B_OK) {
		path.Append("bdb");
		AddAddons(menu, path, addSeparator);
	}
	
	if (find_directory(B_USER_ADDONS_DIRECTORY, &path) == B_OK) {
		path.Append("bdb");
		AddAddons(menu, path, addSeparator);
	}

}


void DWindow::DoViewAs(const char *s)
{
	int vIndex = std::max(fVariables->List()->FullListCurrentSelection(), 0L);
	DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(vIndex));
	
	if (vIndex < 0)
		return;

	string t(s);
	int i = 0, l;
	string name;
	DLocationString loc;

	vItem->Variable()->GetName(name);
	name = '(' + t + ')' + name;
	vItem->Variable()->GetLocationString(loc);
	
	l = t.length();
	while (l > 0 && t[l - 1] == '*')
	{
		l--;
		t.erase(l, 1);
		i++;
	}
	
	DType* type = GetTeam().GetSymWorld().GetType(t.c_str());
	
	while (i-- > 0)
		type = new DPointerType(type);
	
	if (vItem->OutlineLevel())
		fVariables->List()->AddUnder(vItem = DVariableItem::CreateItem(name, type, loc), fVariables->List()->Superitem(vItem));
	else
		fVariables->List()->AddItem(vItem = DVariableItem::CreateItem(name, type, loc), vIndex);
	
	if (vItem->Expandable())
	{
		fVariables->List()->AddUnder(new BStringItem(""), vItem);
		fVariables->List()->Collapse(vItem);
	}

	vItem->SetIsCast();

	UpdateVariableValues();
} // DWindow::DoViewAs

void DWindow::DoViewAsArray()
{
	int vIndex = fVariables->List()->FullListCurrentSelection();
	DVariableItem *vItem = dynamic_cast<DVariableItem*>(fVariables->List()->FullListItemAt(vIndex));
	
	if (vIndex < 0)
		return;
	
	if (!vItem->Variable()->Type()->IsPointer())
		THROW(("Can only cast a pointer to an array, sorry"));

	string name;
	DLocationString loc;

	vItem->Variable()->GetName(name);
	name += "[]";
	vItem->Variable()->GetLocationString(loc);
	loc.push_back(DW_OP_deref);
	
	DType *type = new DArrayType(vItem->Variable()->Type()->Deref());

	if (vItem->OutlineLevel())
		fVariables->List()->AddUnder(vItem = DVariableItem::CreateItem(name, type, loc), fVariables->List()->Superitem(vItem));
	else
		fVariables->List()->AddItem(vItem = DVariableItem::CreateItem(name, type, loc), vIndex);
	
	fVariables->List()->AddUnder(new BStringItem(""), vItem);
	fVariables->List()->Collapse(vItem);
	
	vItem->SetIsCast();

	UpdateVariableValues();
} // DWindow::DoViewAsArray

void DWindow::DoAddExpression()
{
	fVariables->List()->AddItem(new DExprVariableItem(*GetStackFrame(), "0"));
	UpdateVariableValues();
} // DWindow::DoAddExpression
