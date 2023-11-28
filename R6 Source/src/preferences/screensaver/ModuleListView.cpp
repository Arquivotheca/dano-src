#include "Blanket.h"
#include "ModuleListView.h"
#include "ssdefs.h"
#include "ModuleRoster.h"

#include <FindDirectory.h>
#include <ScrollView.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Font.h>
#include <SymLink.h>

ModuleListView::ModuleListView(BRect frame, char *name)
 : BListView(frame, name), sv(0)
{
}

void ModuleListView::TargetedByScrollView(BScrollView *v)
{
	sv = v;
}

void ModuleListView::MouseMoved(BPoint, uint32 transit, const BMessage *message)
{
	if(message && message->HasRef("refs") && transit != B_INSIDE_VIEW && sv != 0)
	{
		if(transit == B_ENTERED_VIEW)
			sv->SetBorderHighlighted(true);
		if(transit == B_EXITED_VIEW)
			sv->SetBorderHighlighted(false);
	}
}

void ModuleListView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_SIMPLE_DATA :
			if(msg->WasDropped())
			{
				if(sv)
					sv->SetBorderHighlighted(false);

				ModuleRoster::Install(msg);
			}
			break;

		default :
			BListView::MessageReceived(msg);
			break;
	}
}
