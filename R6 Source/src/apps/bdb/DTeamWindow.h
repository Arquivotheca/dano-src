/*	$Id: DTeamWindow.h,v 1.11 1999/05/05 19:48:39 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 08:56:59
*/

#ifndef DTEAMWINDOW_H
#define DTEAMWINDOW_H

#include "DWindow.h"

#include "HButtonBar.h"

#include <map>
#include <list>

#include <Entry.h>

class DThreadWindow;
class DSourceView;

struct DStatement;

class DTeamWindow : public DWindow
{
  public:
	DTeamWindow(entry_ref& ref, int argc = 0, char **argv = NULL);
	DTeamWindow(team_id id, port_id port = -1);
	DTeamWindow(const char* host, uint16 port = 5038);
	~DTeamWindow();
	
	virtual bool QuitRequested();
	
	static DTeamWindow* GetNextTeamWindow(uint32& cookie);

		// to search for the window of an already running team
	static DTeamWindow* GetTeamWindow(team_id);
	
	static void FillWindowMenu(BMenu *menu);

  private:
	virtual void MessageReceived(BMessage *msg);
	virtual void MenusBeginning();

	void BuildWindow();

	void ListThreads();
	void ListFiles();
	void ListGlobals();
	void DoKill();
	void Detach();
	void DoDebugThread();
	void DoWhereIs();
	
	void ShowAddress(ptr_t addr);
	void ShowFunction(const char* name);
	
	entry_ref fTargetRef;
	DListBox *fThreads, *fFiles;
	DSourceView *fSource;
	std::map<thread_id,DThreadWindow*> fThreadWindows;
	HButtonBar *fButtonBar;
	BWindow *fBreakpointWindow;
	BWindow *fWatchpointWindow;
	static std::list<DTeamWindow*> sfWindows;
	bigtime_t fLastGlobalUpdate;
	
	static BRect GetNextTeamRect();
};

#endif
