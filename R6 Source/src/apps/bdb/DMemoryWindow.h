/*	$Id: DMemoryWindow.h,v 1.3 1998/11/17 12:16:36 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#ifndef DMEMORYWINDOW_H
#define DMEMORYWINDOW_H

#include <Window.h>

class DTeam;
class XView;

class DMemoryWindow : public BWindow
{
  public:
	DMemoryWindow(DTeam& team, ptr_t addr, size_t size);
	
	void MessageReceived(BMessage *msg);

  private:
  	void DoRefreshAt(ptr_t addr);
  	
	XView* 	fEditor;
	ptr_t	fAddress;
	DTeam&	fTeam;
};

#endif
