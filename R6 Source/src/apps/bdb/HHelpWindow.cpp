/*	$Id: HHelpWindow.cpp,v 1.3 1999/05/03 13:21:01 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 1/8/98
*/

#include "HHelpWindow.h"

#include <stdlib.h>
#include <string.h>

HHelpView::HHelpView(BRect r, const char *helptext)
	: BView(r, "help view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fHelpText = strdup(helptext);
	SetViewUIColor(B_UI_TOOLTIP_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_TOOLTIP_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_TOOLTIP_TEXT_COLOR);
	SetFont(be_plain_font);
} /* HHelpView::HHelpView */

HHelpView::~HHelpView()
{
	free(fHelpText);
} /* HHelpView::~HHelpView */
			
void HHelpView::Draw(BRect /*update*/)
{
	font_height fh;
	be_plain_font->GetHeight(&fh);
	
	DrawString(fHelpText, BPoint(2, fh.ascent - 1));
} /* HHelpView::Draw */
			
HHelpWindow::HHelpWindow(BRect r, const char *msg)
	: BWindow(r, "", B_BORDERED_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL, B_AVOID_FOCUS | B_AVOID_FRONT)
{
	r.OffsetTo(0, 0);
	AddChild(new HHelpView(r, msg));
	Show();
} /* HHelpWindow::HHelpWindow */

