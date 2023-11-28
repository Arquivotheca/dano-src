/*	$Id: DWindow.h,v 1.7 1999/05/03 13:10:00 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 12/16/98 21:45:08
*/

#ifndef DWINDOW_H
#define DWINDOW_H

class DListBox;
class DVariableItem;
class DTeam;

#include "DVariable.h"

#include <Window.h>
class BPopUpMenu;
class BPath;

class DWindow : public BWindow
{
  public:
	DWindow(BRect frame, const char *title, window_type type, uint32 flags);
	virtual ~DWindow();

	virtual void MessageReceived(BMessage *msg);
	virtual void MenusBeginning();
	virtual void ExpandPointerVariable(DVariableItem *item);
//	virtual void UpdateVariable(DVariableItem *item);
	
	DTeam& GetTeam() const { return *fTeam; }
  	virtual DStackFrame* GetStackFrame();

  	virtual void UpdateVariableValues();
  	
  protected:

	void ChangeFormat(EVarFormat format);
	void AddAddons(BMenu *);
	void AddAddons(BMenu *, const BPath &, bool &);
	void DumpMemory();
	void DumpAddOn(const entry_ref *);
	void AddWatchpoint();
 	void SetWatchpoint(ptr_t addr);
 	void DoViewAs(const char *cast);
 	void DoViewAsArray();
 	void DoAddExpression();
 	
	bool GetSelectedVariable(const DType *&type, ptr_t &addr, bool derefIfPointer);

	DListBox *fVariables;
	DTeam *fTeam;
	BPopUpMenu *fFormatPopup;
};

#endif
