/*	$Id: HDlogView.cpp,v 1.1 1998/11/14 14:20:52 maarten Exp $
	
	HDlogView.cpp, Copyright Hekkelman Programmatuur
	Created: 11/27/97 22:08:39 by Maarten Hekkelman

	$Log: HDlogView.cpp,v $
	Revision 1.1  1998/11/14 14:20:52  maarten
	Changed the names of almost all the library files
	
	Revision 1.4  1998/10/06 18:40:51  maarten
	Changes caused by porting to MacOS
	
	Revision 1.3  1998/01/16 14:13:53  hekkel
	some changes
	
	Revision 1.2  1998/01/09 19:21:56  hekkel
	fixes for intel release

	Revision 1.1  1997/12/04 20:09:42  hekkel
	added background view
	
*/

#include "HDlogView.h"
#include "lib.h"

HDlogView::HDlogView(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
} /* HDlogView::HDlogView */
		
HDlogView::~HDlogView()
{
} /* HDlogView::~HDlogView */

void HDlogView::Draw(BRect update)
{
	BView::Draw(update);

	if (fLines.size() > 0)
	{
		vector<BRect>::iterator ri;
		
		BeginLineArray(fLines.size() * 2);
		for (ri = fLines.begin(); ri != fLines.end(); ri++)
		{
			BRect r = *ri;
			if (r.Width() > r.Height())
			{
				AddLine(r.LeftTop(), r.RightTop(), kShadow);
				AddLine(r.LeftBottom(), r.RightBottom(), kWhite);
			}
			else
			{
				AddLine(r.LeftTop(), r.LeftBottom(), kShadow);
				AddLine(r.RightTop(), r.RightBottom(), kWhite);
			}
		}
		EndLineArray();
	}
} /* HDlogView::Draw */

void HDlogView::AddMyLine(BRect r)
{
	fLines.push_back(r);
} /* HDlogView::AddMyLine */
