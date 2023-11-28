
#include "ZoomControl.h"

#include <MenuItem.h>

ZoomControl::ZoomControl(BRect frame, const char *name) :
	BMenuBar(frame, name),
	fMenu(NULL)
{
}

void 
ZoomControl::AttachedToWindow()
{
	if (!fMenu) {
		fMenu = new BMenu("Scale");
		fMenu->SetRadioMode(true);
		fMenu->SetLabelFromMarked(true);

		BMessage msg('zoom');

		msg.AddFloat("scale", 5.0);
		BMenuItem *item = new BMenuItem("500%", new BMessage(msg));
		fMenu->AddItem(item);

		msg.ReplaceFloat("scale", 4.0);
		item = new BMenuItem("400%", new BMessage(msg));
		fMenu->AddItem(item);

		msg.ReplaceFloat("scale", 3.0);
		item = new BMenuItem("300%", new BMessage(msg));
		fMenu->AddItem(item);

		msg.ReplaceFloat("scale", 2.0);
		item = new BMenuItem("200%", new BMessage(msg));
		fMenu->AddItem(item);
		
		msg.ReplaceFloat("scale", 1.75);
		item = new BMenuItem("175%", new BMessage(msg));
		fMenu->AddItem(item);
		
		msg.ReplaceFloat("scale", 1.5);
		item = new BMenuItem("150%", new BMessage(msg));
		fMenu->AddItem(item);
	
		msg.ReplaceFloat("scale", 1.25);
		item = new BMenuItem("125%", new BMessage(msg));
		fMenu->AddItem(item);

		msg.ReplaceFloat("scale", 1.0);
		item = new BMenuItem("100%", new BMessage(msg));
		item->SetMarked(true);
		fMenu->AddItem(item);
		
		msg.ReplaceFloat("scale", .75);
		item = new BMenuItem("75%", new BMessage(msg));
		fMenu->AddItem(item);
		
		msg.ReplaceFloat("scale", .50);
		item = new BMenuItem("50%", new BMessage(msg));
		fMenu->AddItem(item);

		msg.ReplaceFloat("scale", .25);
		item = new BMenuItem("25%", new BMessage(msg));
		fMenu->AddItem(item);
		
		msg.ReplaceFloat("scale", .125);
		item = new BMenuItem("12.5%", new BMessage(msg));
		fMenu->AddItem(item);

		AddItem(fMenu);
	}
}

status_t 
ZoomControl::SetTarget(BHandler *handler)
{
	return fMenu->SetTargetForItems(handler);
}

