#include "DirPanel.h"
#include <Application.h>
#include <Path.h>
#include <Node.h>
#include <Messenger.h>
#include <Button.h>
#include <Window.h>
#include <stdio.h>

class SelectButton : public BButton
{
	BFilePanel	*pan;

public:
	SelectButton(BFilePanel *panel, BRect frame, const char *name,
		const char *label, BMessage *message,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
	 : BButton(frame, name, label, message, resizingMode, flags), pan(panel)
	{
	}

	status_t Invoke(BMessage *message)
	{
		if(pan->HidesWhenDone())
			pan->Hide();
		return BButton::Invoke();
	}

	// this button puts it's text inside "Select ''" and chops it to the button width
	void SetLabel(const char *text)
	{
		BRect	rect = Bounds();
		char	buffer[B_FILE_NAME_LENGTH + 15];
		char	trunc[B_FILE_NAME_LENGTH + 15];

		sprintf(buffer, "Select '%s'", text);

		if(StringWidth(buffer) > rect.Width() - 10)
		{
			BFont font;
			GetFont(&font);

			float fixed = StringWidth("Select ''");

			const char *src[1];
			char *results[1];
			src[0] = text;
			results[0] = trunc;
		    font.GetTruncatedStrings(src, 1, B_TRUNCATE_END, rect.Width() - fixed - 10, results);
			text = trunc;

			sprintf(buffer, "Select '%s'", trunc);
		}

		BButton::SetLabel(buffer);
	}
};

bool DirFilter::Filter(const entry_ref *, BNode *node, struct stat *, const char *)
{
	return node->IsDirectory();
}

DirPanel::DirPanel(BMessenger *target, entry_ref *start_directory,
	BMessage *message, BRefFilter *rf)
 : BFilePanel(B_OPEN_PANEL, target, start_directory, B_DIRECTORY_NODE,
 	false, message, rf, false, true)
{
	BWindow	*w = Window();
	if(w->Lock())
	{
		SetButtonLabel(B_DEFAULT_BUTTON, "Select");
		BView *panelback = w->ChildAt(0);

		// create new 'select this' button
		BRect clone = panelback->FindView("cancel button")->Frame();
		clone.left -= 180;
		clone.right -= 80;
		selbutton = new SelectButton(this, clone, "this", "", 0, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		panelback->AddChild(selbutton);
		selbutton->SetTarget(target ? *target : be_app_messenger);

		// set button text
		entry_ref dir;
		if(start_directory)
			dir = *start_directory;
		else
			GetPanelDirectory(&dir);
		SetDir(&dir, message);
		w->Unlock();
	}
}

void DirPanel::SetPanelDirectory(entry_ref *dir)
{
	BFilePanel::SetPanelDirectory(dir);
	BWindow *w = Window();
	if(w->Lock())
	{
		SetDir(dir, selbutton->Message());
		w->Unlock();
	}
}

void DirPanel::SetTarget(BMessenger bellhop)
{
	BFilePanel::SetTarget(bellhop);
	selbutton->SetTarget(bellhop);
}

void DirPanel::SelectionChanged()
{
	entry_ref r;
	GetPanelDirectory(&r);
	SetDir(&r, selbutton->Message());
	BFilePanel::SelectionChanged();
}

void DirPanel::SetDir(entry_ref *r, BMessage *msg)
{
	selbutton->SetLabel(r->name);
	BMessage *templ = msg ? (new BMessage(*msg)) : (new BMessage());
	templ->RemoveName("refs");
	templ->AddRef("refs", r);
	selbutton->SetMessage(templ);
}
