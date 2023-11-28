/*	$Id: DMainWindow.h,v 1.1 1999/01/21 15:18:44 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 01/20/99 14:49:18
*/

#ifndef DMAINWINDOW_H
#define DMAINWINDOW_H

#include <Window.h>

class DListBox;

class DMainWindow : public BWindow
{
	DMainWindow();
	~DMainWindow();
	
  public:
	virtual bool QuitRequested();

	static DMainWindow* Instance();
	
	virtual void MessageReceived(BMessage *msg);
	
  private:
  	
  	void UpdateTeamList();
  	
	static DMainWindow *sWindow;
	DListBox *fTeams;
};

#endif
