/*	$Id: DVariableItem.cpp,v 1.11 1999/05/03 13:09:59 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 11/16/98 10:58:38
*/

#include "bdb.h"
#include "DStackFrame.h"
#include "DVariable.h"
#include "DVariableItem.h"
#include "DThreadWindow.h"
#include "DTeam.h"
#include "DTeamWindow.h"
#include "DExprParser.h"
#include "DBasicType.h"

#include <OutlineListView.h>

using std::vector;

//using namespace dwarf;

// ---------------------------------------------------------------------------
// First we have a factory for creating variable items
// ---------------------------------------------------------------------------

DVariableItem* DVariableItem::CreateItem(const string& name, DType *type, const DLocationString& location)
{
	return CreateItem(new DVariable(name, type, location));
} // DVariableItem::CreateItem

DVariableItem* DVariableItem::CreateGlobalItem(DVariable *var, ptr_t baseAddr, DNub& /*nub*/)
{
	// Notice that the DVariableItem takes over ownership of the DVariable
	DVariableItem *item = CreateItem(var);

	if (item)
	{
		try
		{	
			item->fBaseAddress = baseAddr;
			item->fGlobal = true;
		}
		catch (HErr& e)
		{
			delete item;
			item = NULL;
		}
	}
	
	return item;
} // DVariableItem::CreateGlobalItem

// ---------------------------------------------------------------------------
// Local factory
// ---------------------------------------------------------------------------

DVariableItem* DVariableItem::CreateItem(DVariable *var)
{
	const DType* type(var->Type());
	DVariableItem *result;
	
	if (type->IsStruct() || type->IsUnion() || type->IsArray())
		result = new DStructVariableItem(var);
	else if (type->IsPointer() || type->IsReference())
		result = new DPointerVariableItem(var);
//	else if (type->IsArray())
//		result = new DArrayVariableItem(var);
	else
		result = new DVariableItem(var);
	
	return result;
} // DVariableItem::CreateItem

//#pragma mark -

DVariableItem::DVariableItem(DVariable *variable)
	: DListItem("")
	, fVariable(variable)
{
	string name;
	fVariable->GetName(name);
	SetText(name.c_str());

	fChanged = false;
	fIsCast = false;
	fFormat = fmtDefault;
	fListBox = NULL;
	fGlobal = false;
	fInScope = true;
	fLowScopePC = 0;
	fHighScopePC = 0;
} // DVariableItem::DVariableItem

DVariableItem::~DVariableItem()
{
	// Children point to their parent's DVariable
	if (OutlineLevel() == 0)
	{
		delete fVariable;
	}
} // DVariableItem::~DVariableItem

const char* DVariableItem::GetColumnText(int column)
{
	if (column == 1)
		return fValue.c_str();
	else
		return NULL;
} // DVariableItem::GetColumnText

void DVariableItem::SetColumnText(int column, const char *newText)
{
	if (column == 1 && fListBox)
	{
		DWindow *w = static_cast<DWindow*>(fListBox->Window());
		DStackFrame *frame = w->GetStackFrame();
		DNub *nub = &w->GetTeam().GetNub();
		
		if (fGlobal)
		{
			fVariable->SetValueAsText(*nub, fBaseAddress, newText, fFormat);
		}
		else
		{
			fVariable->SetValueAsText(*frame, newText, fFormat);
		}
			
		w->UpdateVariableValues();
	}
} // DVariableItem::SetColumnText

void DVariableItem::DrawItem(BView *owner, BRect bounds, bool /*complete*/, int column)
{
	fListBox = static_cast<BOutlineListView*>(owner);

	font_height fh;
	be_plain_font->GetHeight(&fh);

	owner->SetLowColor(tint_color(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR),
		IsSelected() ? B_HIGHLIGHT_BACKGROUND_TINT : B_NO_TINT));
	owner->FillRect(bounds, B_SOLID_LOW);
	
	if (column == 0)
	{
		owner->SetHighColor(tint_color(ui_color(B_UI_DOCUMENT_TEXT_COLOR),
			fInScope ? B_NO_TINT : B_DISABLED_LABEL_TINT));
		owner->SetFont(be_plain_font);
		owner->DrawString(Text(), BPoint(bounds.left + 2, bounds.bottom - fh.descent - 1));
	}
	else
	{
		if (fChanged)
			owner->SetHighColor(kRed);
		else
			owner->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
	
		owner->SetFont(be_fixed_font);
		owner->DrawString(fValue.c_str(),
			BPoint(bounds.left + 2, bounds.bottom - fh.descent - 1));
	}
	
	// reset colors now that everything's drawn
	owner->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
	owner->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
} // DVariableItem::DrawItem

bool DVariableItem::UpdateValue(DStackFrame& frame)
{
	ASSERT_OR_THROW (!fGlobal);

	string value;
	fVariable->GetValueAsText(frame, value, fFormat);

	fChanged = (value != fValue);
	
	if (fChanged)
		fValue = value;
	
	fInScope = IsInScope(frame.GetPC());
	
	if (fChanged && fListBox)
		fListBox->InvalidateItem(fListBox->IndexOf(this));
	
	return fChanged;
} // DVariableItem::UpdateValue

bool DVariableItem::UpdateValue(DNub& nub)
{
	// Because of timing, we might get here when we really do have a stack frame
	// (if we had just waited for a second)
	// Throw an error because we will catch it when updating variables
	// ...but don't assert
	
	if (fGlobal == false) {
		throw HErr("Updating local variable with no stack frame");
	}
	
	if (fListBox == NULL) return false;
	
	string value;

	try
	{
		fVariable->GetValueAsText(nub, fBaseAddress, value, fFormat);
	}
	catch (HErr& e)
	{
		// probably the app is no longer running, leave value empty
	}

	fChanged = (value != fValue);
	fInScope = true;
	
	if (fChanged)
		fValue = value;
	
	if (fChanged && fListBox)
		fListBox->InvalidateItem(fListBox->IndexOf(this));
	
	return fChanged;
} // DVariableItem::UpdateValue

bool DVariableItem::Expandable() const
{
	return false;
} // DVariableItem::Expandable

void DVariableItem::SetFormat(EVarFormat format)
{
	fFormat = format;
} // DVariableItem::SetFormat

bool DVariableItem::operator== (const DVariableItem& var) const
{
	ASSERT(!fGlobal);
	return fBaseAddress == var.fBaseAddress && *fVariable == *var.fVariable;
} // DVariableItem::Equals

bool DVariableItem::operator< (const DVariableItem& var) const
{
	string a, b;
	fVariable->GetName(a);
	var.fVariable->GetName(b);
	return a < b;
} // DVariableItem::operator<

//void DVariableItem::PrintToStream()
//{
//	string name;
//	fVariable->GetName(name);
//	cout << name;
//	
//	if (fGlobal)
//		cout << '\t' << fBaseAddress;
//	
//	cout << endl;
//} // DVariableItem::PrintToStream

void DVariableItem::AddToList(BOutlineListView *list, DVariableItem *vItem, DVariable *var)
{
	DVariable::member_reverse_iterator rvi;
	
	DNub& nub = static_cast<DWindow*>(list->Window())->GetTeam().GetNub();
	
	for (rvi = var->member_rbegin(); rvi != var->member_rend(); rvi++)
	{
		DVariableItem *mItem;
		
		if (dynamic_cast<DInheritedVariable*>(*rvi) != NULL)
			AddToList(list, vItem, *rvi);
		else
		{
			if (vItem->fGlobal)
				list->AddUnder(mItem = DVariableItem::CreateGlobalItem(*rvi, vItem->fBaseAddress, nub), vItem);
			else
				list->AddUnder(mItem = DVariableItem::CreateItem(*rvi), vItem);

			if ((*rvi)->IsExpandable())
			{
				list->AddUnder(new BStringItem(""), mItem);
				list->Collapse(mItem);
			}
		}
	}
	
	list->Collapse(vItem);
} // DVariableItem::AddToList

void DVariableItem::SetScope(ptr_t low, ptr_t high)
{
	fLowScopePC = low;
	fHighScopePC = high;
} // DVariableItem::SetScope

bool DVariableItem::IsInScope(ptr_t pc)
{
	if (fLowScopePC == 0 || fHighScopePC == 0)
		return true;
	
	pc &= 0x00ffffff;

	return pc >= fLowScopePC && pc < fHighScopePC;
} // DVariableItem::IsInScope

ptr_t DVariableItem::GlobalAddress() const
{
	ASSERT_OR_THROW (fGlobal);
	DNub& nub = static_cast<DWindow*>(fListBox->Window())->GetTeam().GetNub();
	return fVariable->GetLocation(nub, fBaseAddress);
}

//#pragma mark -

bool DPointerVariableItem::Expandable() const
{
	return true;
} // DPointerVariableItem::Expandable

void DPointerVariableItem::Expanded(BOutlineListView *list)
{
	BListItem *item = list->ItemUnderAt(this, true, 0);

	if (dynamic_cast<DVariableItem*>(item) == NULL)
	{
		DWindow *w = dynamic_cast<DWindow*>(list->Window());
		FailNilMsg(w, "Not a valid parent window");
		w->DisableUpdates();
		
		fVariable->Expand();
		
		DVariable::member_reverse_iterator mi = fVariable->member_rbegin();
//		ASSERT(mi != fVariable->member_rend());
//		ASSERT(mi + 1 == fVariable->member_rend());
		
		if (mi != fVariable->member_rend())
		{
			DVariable *vp = *mi;
			
			if (vp->IsExpandable() && !vp->Type()->IsPointer())
			{
				vp->Expand();
				AddToList(list, this, vp);
			}
			else if (fGlobal)
			{
				DNub& nub = w->GetTeam().GetNub();
				list->AddUnder(DVariableItem::CreateGlobalItem(vp, fBaseAddress, nub), this);
			}
			else
				list->AddUnder(DVariableItem::CreateItem(vp), this);
		}
		
		list->RemoveItem(item);
		delete item;

		w->EnableUpdates();
		w->ExpandPointerVariable(this);
	}
} // DPointerVariableItem::Expanded

//#pragma mark -

const char* DStructVariableItem::GetColumnText(int /*column*/)
{
	return NULL;
} // DStructVariableItem::GetColumnText

void DStructVariableItem::Expanded(BOutlineListView *list)
{
	BListItem *item = list->ItemUnderAt(this, true, 0);

	if (dynamic_cast<DVariableItem*>(item) == NULL)
	{
		DWindow *w = dynamic_cast<DWindow*>(list->Window());
		FailNilMsg(w, "Not a valid parent window");

		w->DisableUpdates();
		
		AddToList(list, this, fVariable);
		
		list->RemoveItem(item);
		delete item;
		
		w->EnableUpdates();
		w->ExpandPointerVariable(this);
	}
} // DStructVariableItem::Expanded

bool DStructVariableItem::Expandable() const
{
	return true;
} // DStructVariableItem::Expandable

//#pragma mark -

DExprVariableItem::DExprVariableItem(const DStackFrame& frame, const char *e)
	: DStructVariableItem(DExprParser(e).Parse(frame))
{
	SetText(e);
} // DExprVariableItem::DExprVariableItem

const char* DExprVariableItem::GetColumnText(int column)
{
	if (column == 0)
		return Text();
	else
		return NULL;
} // DExprVariableItem::GetColumnText

static BListItem* Nuke(BListItem *item, void *l)
{
	static_cast<vector<BListItem*>*>(l)->push_back(item);
	
	return NULL;
} // Nuke

void DExprVariableItem::SetColumnText(int column, const char *newText)
{
	if (column == 0 && fListBox)
	{
		DWindow *w = static_cast<DWindow*>(fListBox->Window());
		DStackFrame *frame = w->GetStackFrame();

		if (fVariable->IsExpandable())
			fListBox->Collapse(this);

		if (frame)
			fVariable = DExprParser(newText).Parse(*frame);
		else
			THROW(("Not implemented yet"));

		SetText(newText);

		if (fListBox->CountItemsUnder(this, false))
		{
			vector<BListItem*> l;
			fListBox->EachItemUnder(this, false, Nuke, &l);
			
			for (vector<BListItem*>::reverse_iterator i = l.rbegin(); i != l.rend(); i++)
			{
				fListBox->RemoveItem(*i);
				delete *i;
			}
		}
		
		if (fVariable->IsExpandable())
		{
			fVariable->Expand();
			fListBox->Collapse(this);
			fListBox->AddUnder(new BStringItem(" "), this);
		}
		
		w->UpdateVariableValues();
	}
} // DExprVariableItem::SetColumnText

bool DExprVariableItem::UpdateValue(DStackFrame& frame)
{
//	delete fVariable;
	try
	{
		fVariable = DExprParser(Text()).Parse(frame);
		DVariableItem::UpdateValue(frame);
	}
	catch (HErr& e)
	{
		e.DoError();

//		int64 x = 0;
//		fVariable = new DConstVariable("int", new DIntType(8, true), &x);
	}
	return fChanged;
} // DExprVariableItem::UpdateValue

bool DExprVariableItem::UpdateValue(DNub& /*nub*/)
{
	THROW(("Not possible yet"));
} // DExprVariableItem::UpdateValue

bool DExprVariableItem::Expandable() const
{
	return fVariable->IsExpandable();
} // DExprVariableItem::Expandable

