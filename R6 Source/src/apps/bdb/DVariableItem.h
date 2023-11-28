/*	$Id: DVariableItem.h,v 1.9 1999/05/03 13:09:59 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 11/16/98 10:58:53
*/

#ifndef DVARIABLEITEM_H
#define DVARIABLEITEM_H

#include "DListBox.h"
#include "DVariable.h"

class DStackFrame;

class DVariableItem : public DListItem
{
  protected:
	DVariableItem(DVariable *variable);

  public:
  	virtual ~DVariableItem();
  	
		// a DVariableItem	factory
	static DVariableItem* CreateItem(const string& name, DType *type, const DLocationString& location);
	static DVariableItem* CreateGlobalItem(DVariable *var, ptr_t baseAddr, DNub& nub);
	
		// DListItem overrides
	virtual const char* GetColumnText(int column);
	virtual void SetColumnText(int column, const char *newText);
	virtual void DrawItem(BView *owner, BRect bounds, bool complete, int column);
	
	virtual bool Expandable() const;
	
		// DVariableItem specific
	virtual bool UpdateValue(DStackFrame& frame);
	virtual bool UpdateValue(DNub& nub);

	bool Changed() const 						{ return fChanged; }
	void SetChanged(bool changed)			{ fChanged = changed; }
	bool IsCast() const							{ return fIsCast; }
	void SetIsCast()							{ fIsCast = true; }
	
	void SetFormat(EVarFormat format);
	EVarFormat Format() const				{ return fFormat; }
	
	DVariable* Variable() const				{ return fVariable; }
	
	bool operator< (const DVariableItem& item) const;
	bool operator== (const DVariableItem& item) const;
	
	void PrintToStream();
	
	bool IsGlobal() const 						{ return fGlobal; }
	bool IsInScope(ptr_t pc);
	virtual ptr_t GlobalAddress() const;
	
	void SetScope(ptr_t low, ptr_t high);
	void SetListBox(BOutlineListView *listBox);
	
  protected:
  	
  	static void AddToList(BOutlineListView *list, DVariableItem *vItem, DVariable *var);
 	static DVariableItem* CreateItem(DVariable *var);
 	
	DVariable *fVariable;
	string fValue;
	bool fChanged, fGlobal, fInScope, fIsCast;
	ptr_t fBaseAddress;				// for globals
	ptr_t fLowScopePC, fHighScopePC;
	EVarFormat fFormat;
	BOutlineListView *fListBox;
};

typedef std::vector<DVariableItem*> varlist;

class DPointerVariableItem : public DVariableItem
{
  public:
	DPointerVariableItem(DVariable *variable)
		: DVariableItem(variable) {}
	
	virtual void Expanded(BOutlineListView *list);
	virtual bool Expandable() const;
};

class DStructVariableItem : public DVariableItem
{
  public:
	DStructVariableItem(DVariable *variable)
		: DVariableItem(variable) {}
	
	virtual const char* GetColumnText(int column);

	virtual void Expanded(BOutlineListView *list);
	virtual bool Expandable() const;
};

class DArrayVariableItem : public DVariableItem
{
  public:
	DArrayVariableItem(DVariable *variable)
		: DVariableItem(variable) {}
	
	virtual const char* GetColumnText(int column);

	virtual void Expanded(BOutlineListView *list);
	virtual bool Expandable() const;
};

class DExprVariableItem : public DStructVariableItem
{
  public:
	DExprVariableItem(const DStackFrame& frame, const char *e);
	
	virtual const char* GetColumnText(int column);
	virtual void SetColumnText(int column, const char *newText);

	virtual bool Expandable() const;

	virtual bool UpdateValue(DStackFrame& frame);
	virtual bool UpdateValue(DNub& nub);
};

inline void DVariableItem::SetListBox(BOutlineListView *listBox)
{
	fListBox = listBox;
} // DVariableItem::SetListBox

#endif
