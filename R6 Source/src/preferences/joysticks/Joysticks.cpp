#include <View.h>
#include <Window.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Application.h>
#include <Box.h>
#include <Joystick.h>
#include <ListItem.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Entry.h>
#include <Alert.h>
#include <StringView.h>
#include <CheckBox.h>
#include <Button.h>
#include <TextView.h>
#include <String.h>
#include <stdlib.h>
#include "JoystickTweaker.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "JoyCalib.h"


enum {
	DEV_SELECTED = 'devs',
	DEV_INVOKE = 'devi',
	FILE_SELECTED = 'file',
	FILE_INVOKE = 'fili',
	DISABLED = 'disa'
};


class FileItem : public BStringItem {
public:
	FileItem(BPath & p) :
		BStringItem(p.Leaf()), path(p)
		{
			strcpy(file_name, p.Leaf());
		}
	BPath path;
	char file_name[256];
};

class DevItem : public BStringItem {
public:
static	const char * get_display(const char * d)
		{
			if (!strncmp(d, "/dev/", 5)) {
				d += 5;
			}
			if (!strncmp(d, "joysticks/", 9)) {
				d += 9;
			}
			return d;
		}
	DevItem(const char * dev) :
		BStringItem(get_display(dev))
		{
			m_enabled = true;
			const char * d = get_display(dev);
			find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
			path.Append("joysticks");
			path.Append(d);
			BEntry ent;
			if (!ent.SetTo(path.Path(), true)) {
				BPath path2;
				ent.GetPath(&path2);	/* resolve link */
				strcpy(file_name, path2.Leaf());
			}
			else {
				file_name[0] = 0;
			}
			FILE * f = fopen(path.Path(), "r");
			if (f) {
				char line[300];
				char arg[256];
				while (true) {
					line[0] = 0;
					fgets(line, 299, f);
					if (!line[0]) break;
					char * ptr = line;
					while (*ptr && isspace(*ptr)) ptr++;
					if (!*ptr || (*ptr == '#')) continue;
					if (1 == sscanf(ptr, "filename = \"%255[^\"\n]\"", arg)) {
						strncpy(file_name, arg, 255);
						file_name[255] = 0;
					}
				}
				fclose(f);
			}
		}
	void DrawItem(BView *owner, BRect frame, bool complete = false)
		{
			if (!m_enabled) {
				owner->SetHighColor(192, 192, 192, 0);
			}
			else {
				owner->SetHighColor(0, 0, 0);
			}
			BStringItem::DrawItem(owner, frame, complete);
		}
	BPath path;
	char file_name[256];
	bool m_enabled;
};


class W : public BWindow {
public:
	W(const BRect & area) :
		BWindow(area, "Joysticks", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
		{
			_BJoystickTweaker tweak(m_stick);
			tweak.scan_including_disabled();
			BBox * b = new BBox(Bounds(), "box", B_FOLLOW_ALL, B_WILL_DRAW | 
				B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE, B_PLAIN_BORDER);
			AddChild(b);
			b->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			font_height fh;
			be_bold_font->GetHeight(&fh);
			BStringView * sv = new BStringView(BRect(12,12,132,12+fh.ascent+fh.descent+fh.leading), 
				"DevLabel", "Game Port");
			sv->SetFont(be_bold_font);
			b->AddChild(sv);
			m_devs = new BListView(BRect(12,sv->Frame().bottom+3,132,248), "Ports", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
			BScrollView * scv = new BScrollView("PortScroll", m_devs, B_FOLLOW_LEFT |
				B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, false, true);
			b->AddChild(scv);
			BRect r=m_devs->Frame();
			r.OffsetBy(scv->Frame().RightTop());
			r.OffsetBy(11, 0);
			r.right = Bounds().right - (scv->Bounds().right-m_devs->Frame().right) - 12;
			sv = new BStringView(BRect(r.left,12,r.right,12+fh.ascent+fh.descent+fh.leading), 
				"GadLabel", "Connected Controller");
			sv->SetFont(be_bold_font);
			b->AddChild(sv);
			m_files = new BListView(r, "Gadgets", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
			scv = new BScrollView("GadScroll", m_files, B_FOLLOW_ALL, B_WILL_DRAW, 
				false, true);
			b->AddChild(scv);

			r = m_devs->Frame();
			m_devs->Parent()->ConvertToParent(&r);
			r.top = r.bottom+12;
			r.bottom = r.top+19;
			r.right = r.left+80;
			BButton * bt = new BButton(r, "probe", "Probe", new BMessage(DEV_INVOKE));
			b->AddChild(bt);

			r.left = r.right+6;
			r.right = r.left+80;
			r.top += 2;
			r.bottom -= 2;
			BCheckBox * disable = new BCheckBox(r, "disabled", "Disabled", new BMessage(DISABLED));
			b->AddChild(disable);

			r = m_files->Frame();
			m_files->Parent()->ConvertToParent(&r);
			r.top = r.bottom+12;
			r.bottom = r.top+19;
			r.left = r.right-100;
			bt = new BButton(r, "calibrate", "Calibrate", new BMessage(FILE_INVOKE));
			b->AddChild(bt);

			m_devs->SetSelectionMessage(new BMessage(DEV_SELECTED));
			m_devs->SetInvocationMessage(new BMessage(DEV_INVOKE));
			m_files->SetSelectionMessage(new BMessage(FILE_SELECTED));
			m_files->SetInvocationMessage(new BMessage(FILE_INVOKE));

			SlurpSettingsFiles();
			SlurpDevices();

			if (m_devs->CountItems() > 0) {
				cur_dev = 0;
				m_devs->Select(0);
			}
			else {
				// shouldn't really come here because of check in main()
				cur_dev = -1;
			}
			pending_change = -2;
		}

	void WindowActivated(
			bool to_active)
		{
			BWindow::WindowActivated(to_active);
			if (to_active && CurrentFocus() != m_files)
				m_devs->MakeFocus(true);
		}
	bool QuitRequested()
		{
			ApplyChanges();
			{
				BPath path;
				if (!find_directory(B_COMMON_SETTINGS_DIRECTORY, &path)) {
					path.Append("disabled_joysticks");
					FILE * f = fopen(path.Path(), "w");
					if (f) {
						fprintf(f, "# This is a list of disabled joystick devices.\n");
						fprintf(f, "# Do not include the /dev/joystick/ part of the device name.\n");
						for (int ix=0; ix<m_devs->CountItems(); ix++) {
							DevItem * di = dynamic_cast<DevItem *>(m_devs->ItemAt(ix));
							if (di && !di->m_enabled) {
								fprintf(f, "disable = \"%s\"\n", di->Text());
							}
						}
						fclose(f);
					}
				}
			}
			be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
		}
	void MessageReceived(BMessage * message)
		{
			switch (message->what) {
			case DEV_SELECTED: {
				DevSelected();
				} break;
			case DEV_INVOKE: {
				DevInvoked();
				} break;
			case FILE_SELECTED: {
				FileSelected();
				} break;
			case FILE_INVOKE: {
				FileInvoked();
				} break;
			case DISABLED: {
				Disabled();
				} break;
			default:
				BWindow::MessageReceived(message);
				break;
			}
		}
	void SlurpDevices()
		{
			char buf[256];
			int ix = 0;
			while (!m_stick.GetDeviceName(ix, buf, 256)) {
				DevItem * i = new DevItem(buf);
				m_devs->AddItem(i);
				ix++;
			}
			BPath path;
			if (!find_directory(B_COMMON_SETTINGS_DIRECTORY, &path)) {
				path.Append("disabled_joysticks");
				FILE * f = fopen(path.Path(), "r");
				if (f) {
					char line[300];
					char arg[300];
					while (true) {
						line[0] = 0;
						fgets(line, 299, f);
						if (!line[0]) break;
						char * ptr = line;
						while (*ptr && isspace(*ptr)) ptr++;
						if (!*ptr || *ptr == '#') {
							continue;
						}
						if (1 == sscanf(ptr, "disable = \"%299[^\"\n]\"", arg)) {
							puts(arg);
							for (int ix=0; ix<m_devs->CountItems(); ix++) {
								DevItem * di = dynamic_cast<DevItem *>(m_devs->ItemAt(ix));
								if (di && !strcmp(arg, di->Text())) {
									di->m_enabled = false;
									break;
								}
							}
						}
					}
					fclose(f);
				}
			}
		}
	void SlurpSettingsFiles()
		{
			BPath path;
			if (find_directory(B_BEOS_ETC_DIRECTORY, &path)) {
				path.SetTo("/boot/beos/etc/joysticks");
			}
			else {
				path.Append("joysticks");
			}
			SlurpSettingsPath(path);
			if (find_directory(B_COMMON_ETC_DIRECTORY, &path)) {
				path.SetTo("/boot/home/config/etc/joysticks");
			}
			else {
				path.Append("joysticks");
			}
			SlurpSettingsPath(path);
		}
	void SlurpSettingsPath(BPath & path)
		{
			BDirectory dir;
			if (dir.SetTo(path.Path())) {
				return;
			}
			BEntry ent;
			while (!dir.GetNextEntry(&ent)) {
				if (!ent.GetPath(&path)) {
					bool found = false;
					for (int ix=0; ix<m_files->CountItems(); ix++) {
						FileItem * i = (FileItem *)m_files->ItemAt(ix);
						if (!strcmp(i->file_name, path.Leaf())) {
							found = true;
							break;
						}
					}
					if (!found) {
						FileItem *i = new FileItem(path);
						m_files->AddItem(i);
					}
				}
			}
		}
	void make_path_items(const char * path)
		{
			char * data = strdup(path);
			char * e = data;
			struct stat st;
			while ((e = strchr(e, '/')) != NULL) {
				if (e != data) {
					*e = 0;
					if (stat(data, &st) < 0) {
						int res = mkdir(data, 0777);
//						printf("mkdir(%s): %d\n", data, res);
					}
					*e = '/';
				}
				e = e+1;
			}
			free(data);
		}
	void ApplyChanges()
		{
			if (cur_dev >= 0) {
				DevItem * d = (DevItem *)m_devs->ItemAt(cur_dev);
				if (pending_change >= 0) {
					FileItem * j = (FileItem *)m_files->ItemAt(pending_change);
					if (!strcmp(j->file_name, d->file_name)) {
						goto no_change;
					}
				}
				if (pending_change > -2) {
					unlink(d->path.Path());
					d->file_name[0] = 0;
				}
				if (pending_change >= 0) {
					FileItem * j = (FileItem *)m_files->ItemAt(pending_change);
					make_path_items(d->path.Path());
//					printf("symlink(%s, %s)\n", j->path.Path(), d->path.Path());
					symlink(j->path.Path(), d->path.Path());
					strcpy(d->file_name, j->path.Leaf());
				}
			}
		no_change:
			pending_change = -2;
		}
	void DevSelected()
		{
			ApplyChanges();
			int32 s = m_devs->CurrentSelection(0);
			cur_dev = s;
			BMessage msg(*m_files->SelectionMessage());
			m_files->SetSelectionMessage(new BMessage(-1));
			if (s < 0) {
				m_files->DeselectAll();
			}
			else {
				bool full_inval = false;
				DevItem * i = (DevItem *)m_devs->ItemAt(s);
				BCheckBox * dis = dynamic_cast<BCheckBox *>(FindView("disabled"));
				if (dis) {
					dis->SetValue(i->m_enabled ? 0 : 1);
				}
				bool gotit = false;
				for (int ix=0; ix<m_files->CountItems(); ix++) {
					FileItem * j = (FileItem *)m_files->ItemAt(ix);
					if (ix == 0) {
						full_inval = !j->IsEnabled() != !i->m_enabled;
					}
					j->SetEnabled(i->m_enabled);
					if (i->m_enabled && !strcmp(i->file_name, j->file_name)) {
						m_files->Select(ix);
						gotit = true;
					}
				}
				if (!gotit && i->file_name[0] && i->m_enabled) {
					char str[400];
					sprintf(str, "The file '%s' used by '%s' cannot be found.\n" 
					"Do you want to try to auto-detect a joystick for this port?", 
						i->file_name, i->Text());
					if (1 == (new BAlert("", str, "Stop", "Probe"))->Go()) {
						PostMessage(DEV_INVOKE);
					}
				}
				if (!gotit) {
					m_files->DeselectAll();
				}
				if (full_inval) {
					m_files->Invalidate();
				}
			}
		done:
			m_files->SetSelectionMessage(new BMessage(msg));
		}
	void FileSelected()
		{
			pending_change = m_files->CurrentSelection(0);
		}
	void DevInvoked()
		{
			char str[400];
			DevItem * dev = dynamic_cast<DevItem *>(m_devs->ItemAt(m_devs->CurrentSelection()));
			if (!dev) {
				(new BAlert("", "Select a game port first.", "OK"))->Go();
				return;
			}
			const char * jstr = strrchr(dev->Text(), '/');
			if (!jstr) {
				jstr = dev->Text();
			}
			else {
				jstr++;
			}
			if (!strncmp(jstr, "joystick_", 9)) {
				sprintf(str, "The port '%s' is a BeBox joystick port, which does not support "
					"enhanced (digital) joystick hardware or multi-axis joysticks.", jstr);
				(new BAlert("", str, "Stop"))->Go();
				return;
			}
			if (m_stick.Open(dev->Text(), false) < 0) {
				sprintf(str, "Cannot open the joystick port '%s'.", dev->Text());
				(new BAlert("", str, "OK"))->Go();
				return;
			}
			sprintf(str, "An attempt will be made to probe the port '%s' to try "
				"to figure out what kind of joystick (if any) is attached thereto. "
				"There is a small chance that this process might cause your machine to "
				"lock up and require a reboot. Make sure you have saved changes in "
				"all open applications before you Probe.", dev->Text());
			if (0 != (new BAlert("", str, "Probe", "Cancel"))->Go()) {
				m_stick.Close();
				return;
			}

			BRect r = Frame();
			r.InsetBy(floor((r.Width()-400)/2), floor((r.Height()-100)/2));
			BWindow * progress = new BWindow(r, "Probing", B_MODAL_WINDOW_LOOK, 
				B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
			BBox * bx = new BBox(progress->Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW, 
				B_PLAIN_BORDER);
			progress->AddChild(bx);
			bx->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			bx->SetLowColor(bx->ViewColor());
			BRect rr = bx->Bounds();
			rr.InsetBy(10, 10);
			BRect rrr(rr);
			rrr.OffsetTo(B_ORIGIN);
			BTextView * sv = new BTextView(rr, "message", rrr, B_FOLLOW_NONE, B_WILL_DRAW);
			bx->AddChild(sv);
			sv->MakeEditable(false);
			sv->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			sv->SetLowColor(sv->ViewColor());
			progress->Show();

			BMessage sel(*m_files->SelectionMessage());
			m_files->SetSelectionMessage(new BMessage(-1));
			int detected = 0;
			int full_match = -1;
			BList detectedList;
			/* Going backwards in the list means the analog sticks come last, which is a Good Thing (tm) */
			for (int ix=m_files->CountItems()-1; ix>=0; ix--) {
				FileItem * fi = dynamic_cast<FileItem *>(m_files->ItemAt(ix));
				if (fi) {
					m_files->Select(ix);
					UpdateIfNeeded();
					sprintf(str, "Looking for: '%s'", fi->Text());
					progress->Lock();
					sv->SetText(str);
					sv->Flush();
					progress->Unlock();
					entry_ref ref;
					status_t ref_err = get_ref_for_path(fi->path.Path(), &ref);
					if (!ref_err && m_stick.EnterEnhancedMode(&ref)) {
						detectedList.AddItem(fi);
						detected++;
						BString gadget;
						m_stick.GetControllerName(&gadget);
						sprintf(str, "Found joystick '%s'", gadget.String());
						progress->Lock();
						sv->SetText(str);
						sv->Flush();
						progress->Unlock();
						if ((full_match < 0) && check_full_match(fi->path.Path(), m_stick)) {
							full_match = ix;
						}
						m_stick.Close();
						m_stick.Open(dev->Text(), false);
						UpdateIfNeeded();
					}
					snooze(50000);
				}
			}
			m_files->SetSelectionMessage(new BMessage(sel));
			progress->Lock();
			progress->Quit();
			if (detected > 0) {
				if (full_match > -1) {
					m_files->Select(m_files->IndexOf((FileItem *)m_files->ItemAt(full_match)));
				}
				else {
					m_files->Select(m_files->IndexOf((FileItem *)detectedList.ItemAt(0)));
				}
				if (detected > 1) {
					BString msg("Auto-detection found more than one possible joystick. "
						"While choosing any one of these will work, labels and calibration "
						"will be optimal of you select the one that best matches the actual "
						"joystick connected among the following:\n");
					for (int ix=0; ix<detectedList.CountItems(); ix++) {
						msg += ((FileItem *)detectedList.ItemAt(ix))->Text();
						msg += "\n";
					}
					(new BAlert("Multiple detected", msg.String(), "OK"))->Go();
				}
			}
			else {
				m_files->DeselectAll();
				(new BAlert("None detected", m_devs->CountItems() > 1 ? 
					"There were no compatible joysticks detected on this game port. Try another port, "
					"or ask the manufacturer of your joystick for a driver designed for BeOS." : 
					"There were no compatible joysticks detected on this game port. "
					"Ask the manufacturer of your joystick for a driver designed for BeOS.", "OK"))->Go();
			}
		}
	void FileInvoked()
		{
			ApplyChanges();
			char str[400];
			DevItem * dev = dynamic_cast<DevItem *>(m_devs->ItemAt(m_devs->CurrentSelection()));
			if (!dev) {
				(new BAlert("", "Select a game port first.", "OK"))->Go();
				return;
			}
			if (m_stick.Open(dev->Text(), true) < 0) {
				sprintf(str, "Cannot open the joystick port '%s'.", dev->Text());
				(new BAlert("", str, "OK"))->Go();
				return;
			}
			snooze(10000);
			if (m_stick.EnterEnhancedMode()) {
//				printf("Enhanced mode\n");
			}
			else {
				FileItem * fil = dynamic_cast<FileItem *>(m_files->ItemAt(m_files->CurrentSelection()));
				sprintf(str, "There does not appear to be a %s connected to the port '%s'.", 
					fil ? fil->file_name : "known device", dev->Text());
				(new BAlert("", str, "Stop"))->Go();
				return;
			}
			m_stick.Update();
			/* calibrate */
			JoyCalib * c = new JoyCalib(Frame(), m_stick, this);
			c->Show();
		}
	void Disabled()
		{
			DevItem * di = dynamic_cast<DevItem *>(m_devs->ItemAt(m_devs->CurrentSelection()));
			BCheckBox * cb = dynamic_cast<BCheckBox *>(FindView("disabled"));
			if (di && cb) {
				di->m_enabled = !cb->Value();
				m_devs->InvalidateItem(m_devs->CurrentSelection());
				for (int ix=0; ix<m_files->CountItems(); ix++) {
					FileItem * fi = dynamic_cast<FileItem *>(m_files->ItemAt(ix));
					fi->SetEnabled(di->m_enabled);
				}
				if (di->m_enabled) {
					DevInvoked();
				}
			}
		}
private:

	BJoystick m_stick;
	BListView * m_devs;
	BListView * m_files;
	int32 pending_change;
	int32 cur_dev;

	bool check_full_match(const char * path, BJoystick & stick)
		{
			char line[300];
			char arg[300];
			FILE * f = fopen(path, "r");
			if (!f) return false;
			bool match = false;
			int axes = -1, buttons = -1;
			int sticks = 1;
			int val;
			BString gadget;
			stick.GetControllerName(&gadget);
			while (true) {
				char * ptr;
				line[0] = 0;
				fgets(line, 299, f);
				if (!line[0]) break;
				ptr = line;
				while (*ptr && isspace(*ptr)) ptr++;
				if (!*ptr || *ptr=='#') continue;
				if (1 == sscanf(ptr, "gadget = \"%299[^\"\n]\"", arg)) {
					if (!strcmp(arg, gadget.String())) {
						match = true;
						break;
					}
				}
				else if (1 == sscanf(ptr, "num_buttons = %d", &val)) {
					buttons = val;
				}
				else if (1 == sscanf(ptr, "num_axes = %d", &val)) {
					axes = val;
				}
				else if (1 == sscanf(ptr, "num_sticks = %d", &val)) {
					sticks = val;
				}
				else {
					/* nothing */
				}
			}
			fclose(f);
			if (!match) {
				/* if the name doesn't match, we use this anyway if it has the right # parameters */
				if ((buttons == stick.CountButtons()) && (axes == stick.CountAxes()) && (stick.CountSticks() == sticks)) {
//					printf("buttons and axes and sticks match\n");
					match = true;
				}
			}
			return match;
		}
};

int
main()
{
	BApplication app("application/x-vnd.be.preferences.joysticks");
	{
	}
	W * w = new W(BRect(100,100,500,400));
	w->Show();
	app.Run();
	return 0;
}
