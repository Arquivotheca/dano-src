/*	$Id: DStackCrawlView.h,v 1.1 1998/10/21 12:03:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 04/10/98 14:38:10
*/

#ifndef DSTACKCRAWLVIEW_H
#define DSTACKCRAWLVIEW_H

#include <View.h>
#include <ListView.h>

class DStackCrawl;

class DStackCrawlView : public BView
{
  public:
	DStackCrawlView(BRect frame, const char *name, 
		DStackCrawl& sc, unsigned long resizeMask);
	
	virtual void Draw(BRect update);
	virtual void Update();
	virtual void FrameResized(float w, float h);
	void Clear();
	int32 CountItems()			{ return fStack->CountItems(); };
	int32 CurrentSelection()		{ return fStack->CurrentSelection(); };
	void Select(int32 ix)			{ fStack->Select(ix); };
	void ScrollToSelection()		{ fStack->ScrollToSelection(); }

  private:
	BListView *fStack;
	DStackCrawl& fStackCrawl;
};

#endif
