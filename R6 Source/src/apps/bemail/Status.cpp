//--------------------------------------------------------------------
//	
//	Status.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <StorageKit.h>

#include "Mail.h"
#include "Status.h"


//====================================================================

TStatusWindow::TStatusWindow(BRect rect, BWindow *window, char *status)
			  :BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	BRect		r;

	r.Set(0, 0, STATUS_WIDTH, STATUS_HEIGHT);
	r.InsetBy(-1, -1);
	fView = new TStatusView(r, window, status);
	Lock();
	AddChild(fView);
	Unlock();
	Show();
}


//====================================================================

TStatusView::TStatusView(BRect rect, BWindow *window, char *status)
			:BBox(rect, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BFont		font = *be_plain_font;
	rgb_color	c;

	fWindow = window;
	fString = status;

	c.red = c.green = c.blue = VIEW_COLOR;
	SetViewColor(c);
	font.SetSize(FONT_SIZE);
	SetFont(&font);
}

//--------------------------------------------------------------------

void TStatusView::AttachedToWindow()
{
	BButton		*button;
	BFont		font = *be_plain_font;
	BRect		r;

	r.Set(STATUS_FIELD_H, STATUS_FIELD_V,
		  STATUS_FIELD_WIDTH,
		  STATUS_FIELD_V + STATUS_FIELD_HEIGHT);
	fStatus = new BTextControl(r, "", STATUS_TEXT, fString,
							new BMessage(STATUS));
	AddChild(fStatus);
	font.SetSize(FONT_SIZE);
	fStatus->SetFont(&font);
	fStatus->SetDivider(StringWidth(STATUS_TEXT) + 6);
	fStatus->BTextControl::MakeFocus(true);

	r.Set(S_OK_BUTTON_X1, S_OK_BUTTON_Y1, S_OK_BUTTON_X2, S_OK_BUTTON_Y2);
	button = new BButton(r, "", S_OK_BUTTON_TEXT, new BMessage(OK));
	AddChild(button);
	button->SetTarget(this);
	button->MakeDefault(true);

	r.Set(S_CANCEL_BUTTON_X1, S_CANCEL_BUTTON_Y1, S_CANCEL_BUTTON_X2, S_CANCEL_BUTTON_Y2);
	button = new BButton(r, "", S_CANCEL_BUTTON_TEXT, new BMessage(CANCEL));
	AddChild(button);
	button->SetTarget(this);
}

//--------------------------------------------------------------------

void TStatusView::MessageReceived(BMessage *msg)
{
	char			name[B_FILE_NAME_LENGTH];
	char			new_name[B_FILE_NAME_LENGTH];
	int32			index = 0;
	int32			loop;
	status_t		result;
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	BMessage		*message;
	BNodeInfo		*node;
	BPath			path;
	BVolume			vol;
	BVolumeRoster	roster;

	switch (msg->what) {
		case STATUS:
			break;

		case OK:
			if (!Exists(fStatus->Text())) {
				roster.GetBootVolume(&vol);
				fs_create_index(vol.Device(), INDEX_STATUS, B_STRING_TYPE, 0);
	
				find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
				dir.SetTo(path.Path());
				if (dir.FindEntry("bemail", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("bemail", &dir);
				if (dir.InitCheck() != B_NO_ERROR)
					goto err_exit;
				if (dir.FindEntry("status", &entry) == B_NO_ERROR)
					dir.SetTo(&entry);
				else
					dir.CreateDirectory("status", &dir);
				if (dir.InitCheck() == B_NO_ERROR) {
					sprintf(name, "%s", fStatus->Text());
					if (strlen(name) > B_FILE_NAME_LENGTH - 10)
						name[B_FILE_NAME_LENGTH - 10] = 0;
					for (loop = 0; loop < strlen(name); loop++) {
						if (name[loop] == '/')
							name[loop] = '\\';
					}
					strcpy(new_name, name);
					while(1) {
						if ((result = dir.CreateFile(new_name, &file, true)) == B_NO_ERROR)
							break;
						if (result != EEXIST)
							goto err_exit;
						sprintf(new_name, "%s_%d", name, index++);
					}
					dir.FindEntry(new_name, &entry);
					node = new BNodeInfo(&file);
					node->SetType("text/plain");
					delete node;
					file.Write(fStatus->Text(), strlen(fStatus->Text()) + 1);
					file.SetSize(file.Position());
					file.WriteAttr(INDEX_STATUS, B_STRING_TYPE, 0, fStatus->Text(),
									 strlen(fStatus->Text()) + 1);
				}
err_exit:;
			}
			message = new BMessage(M_CLOSE_CUSTOM);
			message->AddString("status", fStatus->Text());
			fWindow->PostMessage(message);
			// will fall through
		case CANCEL:
			Window()->Quit();
			break;
	}
}

//--------------------------------------------------------------------

bool TStatusView::Exists(const char *status)
{
	char			*predicate;
	BEntry			entry;
	BQuery			query;
	BVolume			vol;
	BVolumeRoster	volume;

	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	predicate = (char *)malloc(strlen(INDEX_STATUS) + strlen(status) + 4);
	sprintf(predicate, "%s = %s", INDEX_STATUS, status);
	query.SetPredicate(predicate);
	query.Fetch();
	free(predicate);

	if (query.GetNextEntry(&entry) == B_NO_ERROR)
		return true;
	return false;
}
