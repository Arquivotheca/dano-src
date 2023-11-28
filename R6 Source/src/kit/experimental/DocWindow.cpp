#include <experimental/DocWindow.h>
#include <experimental/DocApplication.h>

#include <Alert.h>
#include <stdlib.h>
#include <Entry.h>
#include <Directory.h>
#include <FilePanel.h>
#include <stdio.h>

#include <Path.h>

DocWindow::DocWindow(WindowRoster *wr, entry_ref *ref, BRect frame, const char *title,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
 : BWindow(frame, title, look, feel, flags, workspace), windowroster(wr),
   dirty(false), untitled(ref ? false : true), savepanel(0), waitforsave(false)
{
	AddMe(ref);
}

DocWindow::~DocWindow()
{
	RemoveMe();
}

bool DocWindow::QuitRequested()
{
	bool result = true;
	waitforsave = false;

	if(IsDirty())
	{
const char*		kSave1		= "Save changes to the document "B_UTF8_OPEN_QUOTE;
const char*		kSave2		= B_UTF8_CLOSE_QUOTE"?";
const char*		kDontSave	= "Don't Save";
const char*		kCancel		= "Cancel";
const char*		kSave		= "Save";

		// code ripped from StyledEdit
		const char 	*text1 = kSave1;
		const char 	*text2 = kSave2;
		long		len = strlen(text1) + B_FILE_NAME_LENGTH + strlen(text2); 
		char		*title = (char *)malloc(len + 1);
		sprintf(title, "%s%s%s", text1, Title(), text2);

		BAlert *alert = new BAlert(B_EMPTY_STRING, title,
			kCancel, kDontSave, kSave,
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		switch(alert->Go())
		{
			case 0 :
				// Cancel
				result = false;
				break;
				
			case 1 :
				// Don't Save
				break;
				
			default :
				if(untitled)
				{
					result = false;
					waitforsave = true;
					SaveAs();
				}
				else
				{
					BEntry	e(&fileref);
					if(Save(&e) != B_OK)
						result = false;
				}
				break;
		}
	}

	return result;
}

void DocWindow::AddMe(entry_ref *ref)
{
	if(ref)
		fileref = *ref;
	if(windowroster)
		windowroster->AddWindow(this, ref ? &fileref : 0);
}

void DocWindow::RemoveMe()
{
	if(windowroster)
		windowroster->RemoveWindow(this);
}

void DocWindow::EntryChanged(BMessage */*msg*/)
{
}

bool DocWindow::IsDirty()
{
	return dirty;
}

void DocWindow::SetDirty(bool flag)
{
	dirty = flag;
}

entry_ref DocWindow::FileRef() const
{
	if( !untitled ) return fileref;
	return entry_ref();
}

status_t DocWindow::Load(BEntry */*e*/)
{
	return B_OK;
}

status_t DocWindow::Save(BEntry */*e*/, const BMessage */*args*/)
{
	return B_OK;
}

void DocWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case DOC_WIN_SAVE :
			// is it saved?
			if(! untitled)
			{
				BEntry	e(&fileref);
				Save(&e, msg);
			}
			else
				SaveAs();
			break;

		case DOC_WIN_SAVE_AS :
			// open save panel
			SaveAs();
			break;

		case B_SAVE_REQUESTED :
			{
			entry_ref dirref;
			msg->FindRef("directory", &dirref);

			const char *name = NULL;
			msg->FindString("name", &name);

			BDirectory dir(&dirref);
			BEntry	e(&dir, name);

			if(Save(&e, msg) == B_OK)
			{
				e.GetRef(&fileref);
				windowroster->ChangeWindow(this, &fileref);

				SetTitle(name);
				untitled = false;

				if(waitforsave)
					PostMessage(B_QUIT_REQUESTED);
			}
			}
			break;

		default :
			BWindow::MessageReceived(msg);
	}
}

void DocWindow::MenusBeginning()
{
	windowroster->UpdateWindowMenu(this);
}

void DocWindow::SaveAs()
{
	if(! savepanel)
	{
		savepanel = CreateSavePanel();
		savepanel->SetSaveText(Title());
		savepanel->Window()->SetTitle("Save As");
	}

	savepanel->Show();
}

BFilePanel* DocWindow::CreateSavePanel() const
{
	BMessenger self(this);
	return new BFilePanel(B_SAVE_PANEL, &self);
}

void DocWindow::WindowFrame(BRect */*proposed*/)
{
}
