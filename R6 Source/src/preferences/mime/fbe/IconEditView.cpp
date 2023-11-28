#include <Alert.h>
#include <Debug.h>

#include "IconEditView.h"
#include "IconWindow.h"

IconEditView::IconEditView(BRect rect, const char *name, BBitmap *largeIcon,
	BBitmap *miniIcon, bool owning, uint32 resize, uint32 flags)
	:	IconView(rect, name, largeIcon, miniIcon, owning, resize, flags)
{
	dirty = false;
	fIconWindow = NULL;
	fLastTime = system_time();
	fSelectedMimeType[0] = 0;
}

IconEditView::~IconEditView()
{
	ASSERT(!fIconWindow);
}

void
IconEditView::MouseDown(BPoint pt)
{
	IconView::MouseDown(pt);
	
	bigtime_t dblClickTime;
	get_click_speed(&dblClickTime);

	if ((system_time() - fLastTime) <= dblClickTime )
		ShowIconEditor();

	fLastTime = system_time();
}

void
IconEditView::ShowIconEditor()
{
	BBitmap* largeIcon = LargeIcon();
	BBitmap* miniIcon = MiniIcon();
	
	if (!fIconWindow) {
		char str[1024];
		if (fSelectedMimeType)
			sprintf(str,"The large or mini icon for the '%s' mime type is missing.", Name());
		else
			strcpy(str,"The large or mini icon is missing.");
		if ((!largeIcon || !miniIcon)
			&& (new BAlert("Ack!",str,
				"New Icon", "Cancel", NULL))->Go())
			return;

		fIconWindow = new TIconWindow(this, Name(), largeIcon, miniIcon);
		fIconWindow->Show();
	} else
		fIconWindow->Activate();
}

void
IconEditView::SetIconMimeType(char *mimeType)
{
	if (mimeType)
		strcpy(fSelectedMimeType,mimeType);
}

void 
IconEditView::ApplyChange(BMimeType*)
{
	if (!dirty)
		return;

	dirty = false;
}

void 
IconEditView::CutPasteSetLargeIcon(BBitmap *b)
{
	IconView::CutPasteSetLargeIcon(b);
}

void 
IconEditView:: CutPasteSetMiniIcon(BBitmap *b)
{
	IconView::CutPasteSetMiniIcon(b);
}

void 
IconEditView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kEditorClosing:
			IconEditorQuitRequested();
			break;
		default:
			IconView::MessageReceived(message);
	}
}

bool 
IconEditView::QuitRequested()
{
	return IconEditorQuitRequested();
}


bool 
IconEditView::IconEditorQuitRequested()
{
	if (!fIconWindow)
		return true;

	// we have a icon window and it needs saving, try saving 
	if (fIconWindow->Dirty()) {
		switch((new BAlert("", "Would you like to save changes to the icon?",
			"Don't save", "Cancel", "Save"))->Go()) {
			case 0:
				break;
			case 1:
				return false;
			case 2:
				if (!fIconWindow->Save())
					return false;
				break;
		}
	}
	fIconWindow->Lock();
	fIconWindow->Quit();
	fIconWindow = 0;
	be_app->SetCursor(B_HAND_CURSOR);
	return true;
}

