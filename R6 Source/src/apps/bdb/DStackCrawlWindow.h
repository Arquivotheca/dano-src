/*	$Id: DStackCrawlWindow.h,v 1.1 1999/03/13 16:12:59 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/13/99 16:20:26
*/

#ifndef DSTACKCRAWLWINDOW_H
#define DSTACKCRAWLWINDOW_H

#include <Window.h>

class DSourceView;
class DListBox;
class DTeam;
class DStackCrawl;

class DStackCrawlWindow : public BWindow
{
  public:
	DStackCrawlWindow(const char *name, DTeam& team, DStackCrawl& sc);
	
	virtual void MessageReceived(BMessage *msg);

  private:
	DSourceView *fSource;
	DListBox *fFiles;
};

#endif
