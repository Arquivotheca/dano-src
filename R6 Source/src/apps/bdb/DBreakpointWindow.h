/*	$Id: DBreakpointWindow.h,v 1.3 1998/11/04 11:07:37 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman

*/

#ifndef DBREAKPOINTWINDOW_H
#define DBREAKPOINTWINDOW_H

#include <Window.h>

class DListBox;

class DBreakpointWindow : public BWindow
{
  public:
	DBreakpointWindow(BWindow *owner);

	bool QuitRequested();
	
	void MessageReceived(BMessage *msg);

	void ClearBreakpoint(ptr_t pc);
	void EnableBreakpoint(ptr_t pc);
	void DisableBreakpoint(ptr_t pc);
	
	void SetBreakpointCondition(ptr_t pc, const char* condition);
	void SetBreakpointSkipCount(ptr_t pc, unsigned long skipCount);
	void SetBreakpointHitCount(ptr_t pc, unsigned long hitCount);
	
  private:

	void ReloadBreakpoints();
	void DisplayBreakpoint();
	void DeleteBreakpoint();
	void ToggleBreakpoint();

	DListBox *fList;
	BWindow *fOwner;
};

#endif
