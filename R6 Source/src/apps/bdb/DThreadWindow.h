/*	$Id: DThreadWindow.h,v 1.8 1999/02/03 08:30:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/09/98 13:14:32
*/

#ifndef DTHREADWINDOW_H
#define DTHREADWINDOW_H

#include "DStatement.h"
#include "DVariable.h"
#include "DWindow.h"
#include <String.h>

class DStackCrawlView;
class DSourceView;
class DTeamWindow;
class DThread;
class DListBox;
class DVariableItem;
class HButtonBar;
class BFilePanel;

class DThreadWindow : public DWindow
{
  public:
	DThreadWindow(DThread& thread);

	virtual bool QuitRequested();
	virtual DStackFrame* GetStackFrame();
	DThread& GetThread()	{ return fThread; }

  protected:

	virtual void MessageReceived(BMessage *msg);
	virtual void MenusBeginning();

	void DoRun(bool switchWorkspace);
	void DoStep(bool switchWorkspace);
	void DoStepOver(bool switchWorkspace);
	void DoStepOut(bool switchWorkspace);
	void DoKill();
	void DoStop();
	
	void ThreadStopped();
	void TellMeWhy();
	void UpdateSource();

	void ClearVariables();
	void UpdateVariables();
	
	void ShowAssembly(bool newShowState);
	
	void SaveState(const entry_ref& dir, const char *name);
	
  private:
	DThread& fThread;
	BWindow *fRegs;
	DStackCrawlView *fSC;
	DSourceView *fSource;
	bool fShowRegs, fShowAsm, fChangingStackList;
	DTeamWindow *fTeamWindow;
	ptr_t fCurrentFunctionLowPC;		// this is to keep track of the function we're in
	int fCurrentStackIndx;				// together with previous value to make 'm unique in recursions
	HButtonBar *fButtonBar;
	BFilePanel *fPanel;
	BMenuBar* fMenuBar;
	BString fDebuggerMsg;				// the debugger() message, if any
};

#endif
