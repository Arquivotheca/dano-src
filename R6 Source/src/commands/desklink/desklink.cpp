
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Message.h>
#include <Messenger.h>
#include <Bitmap.h>
#include <NodeInfo.h>
#include <Entry.h>
#include <Font.h>
#include <StringView.h>
#include <Screen.h>
#include <Beep.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <Deskbar.h>
#include <MessageRunner.h>

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <list>
#include <algorithm>
#include <string>

#include "VolumeMenuControl.h"

#define DEFAULT_SHOW_DELAY 1000000LL
#define HIDE_DELAY 3000000LL

bool g_verbose = (getenv("LINKTO_DEBUG") != NULL);

#if defined (__POWERPC__)
#define _EXPORT __declspec(dllexport)
#else
#define _EXPORT
#endif

class _EXPORT LinkView;


class BS : public BStringView
{
public:
	BS(
		BRect area,
		const char * name) :
	BStringView(area, "test", name, B_FOLLOW_ALL, B_WILL_DRAW|B_PULSE_NEEDED)
	{
	}
	void Pulse()
	{
		Window()->PostMessage(1);
	}
};

class ToolTipWindow : public BWindow
{
public:
	ToolTipWindow(
		BPoint mouse,
		const char * name) :
	BWindow(
		get_frame(mouse, name),
		name,
		B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_AVOID_FOCUS)
	{
		AddChild(new BS(Bounds(), name));
		ChildAt(0)->SetViewColor(255,255,192);
		ChildAt(0)->SetLowColor(255,255,192);
		SetPulseRate(300000);
		m_time = system_time();
	}
	void MessageReceived(
		BMessage * msg)
	{
		switch (msg->what)
		{
		case 1:
			if (system_time() > m_time+HIDE_DELAY)
				Quit();
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
		}
	}
private:
	static BRect get_frame(
		BPoint where,
		const char * name)
	{
		BRect f = BScreen().Frame();
		float width = be_plain_font->StringWidth(name);
		font_height fh;
		be_plain_font->GetHeight(&fh);
		BRect r(0,0,width+3,fh.ascent+fh.descent+fh.leading+3);
		r.OffsetTo(floor(where.x-r.Width()/2), where.y+8);
		if (r.left < f.left) r.OffsetBy(f.left-r.left, 0);
		if (r.right > f.right) r.OffsetBy(f.right-r.right, 0);
		if (r.bottom > f.bottom) r.OffsetBy(0,-16-r.IntegerHeight());
		return r;
	}
	bigtime_t m_time;
};

#define XBORDER 4
#define YBORDER 4
#define KNOBWIDTH 7

#define LONGTIME 3000000LL
#define SHORTTIME 100000LL

#ifdef MAGIC
class MagicView : public BView
{
public:
		MagicView(BRect frame, const char * name) :
			BView(frame, name, 0, B_WILL_DRAW | B_PULSE_NEEDED)
		{
			SetViewColor(216, 216, 216);
			m_volume = 0.4;
			m_bitmap = new BBitmap(Bounds(), B_CMAP8, true);
			m_view = new BView(Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
			m_bitmap->Lock();
			m_bitmap->AddChild(m_view);
			m_view->SetLowColor(ViewColor());
			m_view->SetHighColor(255, 255, 255);
			m_bitmap->Unlock();
			m_roster = BMediaRoster::Roster();
			m_masterGain = NULL;
			if (m_roster != NULL) {
				media_node mixer;
				status_t err = m_roster->GetAudioMixer(&mixer);
				if (err != B_OK) {
					errno = err;
					if (g_verbose) perror("GetAudioMixer()");
					if (err == 0x80001103) {
						if (g_verbose) fprintf(stderr, "Volume Control: Recovering MediaRoster\n");
						m_roster->Lock();
						m_roster->Quit();
						m_roster = BMediaRoster::Roster();
						if (m_roster != NULL) {
							err = m_roster->GetAudioMixer(&mixer);
							if (err == B_OK) {
								if (g_verbose) fprintf(stderr, "Volume Control: Recovery complete\n");
								goto recovered;
							}
							if (g_verbose) fprintf(stderr, "Volume Control: Recovery failed (0x%x)\n", err);
						}
					}
				}
				else {
	recovered:
					BParameterWeb * web = NULL;
					err = m_roster->GetParameterWebFor(mixer, &web);
					if (web != NULL) {
						BParameter * p;
						for (int ix=0; (p = web->ParameterAt(ix)) != NULL; ix++) {
							if (!strcmp(p->Kind(), B_MASTER_GAIN)) {
								m_masterGain = p;
								break;
							}
						}
					}
					else {
						errno = err;
						if (g_verbose) perror("GetParameterWebFor()");
					}
					if (m_masterGain == NULL) {
						delete web;
					}
				}
				m_roster->ReleaseNode(mixer);
			}
			if (m_masterGain) {
				BContinuousParameter * bcp = dynamic_cast<BContinuousParameter *>(m_masterGain);
				if (bcp == NULL) {
					delete m_masterGain->Web();
					m_masterGain = NULL;
				}
				else {
					float v = 0.0;
					bigtime_t when;
					size_t size = sizeof(v);
					bcp->GetValue(&v, &size, &when);
					float mi = bcp->MinValue();
					float ma = bcp->MaxValue();
					m_volume = (v-mi)/(ma-mi);
					if (m_volume < 0.0) m_volume = 0.0;
					if (m_volume > 1.0) m_volume = 1.0;
				}
			}
		}
		~MagicView()
		{
			delete m_bitmap;
			if (m_masterGain) {
				delete m_masterGain->Web();
			}
		}
		void Draw(BRect area)
		{
			DrawBitmap(m_bitmap, area, area);
		}
		void RegenBitmap()
		{
			static rgb_color hi = { 255, 255, 255, 255 };
			static rgb_color lo = { 96, 96, 96, 255 };
			static rgb_color hi2 = { 240, 240, 240, 255 };
			static rgb_color lo2 = { 140, 140, 140, 255 };
			m_bitmap->Lock();
			BRect b(m_view->Bounds());
			BRect c(b.InsetByCopy(XBORDER, YBORDER));
			BRect d(c.InsetByCopy(1, 1));
			BRect e(d);
			BRect f(d);
			d.right = d.left + KNOBWIDTH;
			d.OffsetBy((f.Width()-d.Width())*m_volume, 0);
			BRect g(e);
			g.left = floor(g.left+10*g.Width()/13-d.Width());
			e.right = d.left-1;
			bool do_g = false;
			if (e.right >= g.left) {
				g.right = e.right;
				e.right = g.left-1;
				do_g = true;
			}
			f.left = d.right+1;
			m_view->SetDrawingMode(B_OP_COPY);
			m_view->SetLowColor(ViewColor());
			m_view->FillRect(b, B_SOLID_LOW);

			m_view->SetHighColor(128, 255, 128);
			m_view->FillRect(e, B_SOLID_HIGH);
			if (do_g) {
				m_view->SetHighColor(255, 128, 128);
				m_view->FillRect(g, B_SOLID_HIGH);
			}
			m_view->SetHighColor(128, 128, 128);
			m_view->FillRect(f, B_SOLID_HIGH);
			font_height fh;
			be_plain_font->GetHeight(&fh);
			const char * str = "Volume";
			if (!m_roster) {
				str = "No Media Server";
			}
			else if (!m_masterGain) {
				str = "No Volume";
			}
			float sw = be_plain_font->StringWidth(str);
			BPoint sp(floor((c.Width()-sw)/2)+c.left,
					floor((c.Height()-(fh.leading+fh.descent+fh.ascent))/2)+
					fh.leading+fh.ascent+c.top);
			BRegion rgn;
			rgn.Set(e);
			m_view->ConstrainClippingRegion(&rgn);
			m_view->SetLowColor(128, 255, 128);
			m_view->SetHighColor(64, 128, 64);
			m_view->DrawString(str, sp);
			if (do_g) {
				rgn.Set(g);
				m_view->ConstrainClippingRegion(&rgn);
				m_view->SetLowColor(255, 128, 128);
				m_view->SetHighColor(128, 64, 64);
				m_view->DrawString(str, sp);
			}
			rgn.Set(f);
			m_view->ConstrainClippingRegion(&rgn);
			m_view->SetLowColor(128, 128, 128);
			m_view->SetHighColor(ViewColor());
			m_view->DrawString(str, sp);
			rgn.Set(m_view->Bounds());
			m_view->ConstrainClippingRegion(&rgn);
			m_view->BeginLineArray(16);
			m_view->AddLine(b.LeftTop(), b.RightTop(), hi);
			m_view->AddLine(b.LeftTop(), b.LeftBottom(), hi);
			m_view->AddLine(b.LeftBottom(), b.RightBottom(), lo);
			m_view->AddLine(b.RightBottom(), b.RightTop(), lo);
			m_view->AddLine(c.LeftBottom(), c.RightBottom(), hi);
			m_view->AddLine(c.RightBottom(), c.RightTop(), hi);
			m_view->AddLine(c.LeftTop(), c.RightTop(), lo);
			m_view->AddLine(c.LeftTop(), c.LeftBottom(), lo);
			m_view->AddLine(d.LeftTop(), d.RightTop(), hi);
			m_view->AddLine(d.LeftTop(), d.LeftBottom(), hi);
			m_view->AddLine(d.LeftBottom(), d.RightBottom(), lo);
			m_view->AddLine(d.RightBottom(), d.RightTop(), lo);
			d.InsetBy(1, 1);
			m_view->AddLine(d.LeftTop(), d.RightTop(), hi2);
			m_view->AddLine(d.LeftTop(), d.LeftBottom(), hi2);
			m_view->AddLine(d.LeftBottom(), d.RightBottom(), lo2);
			m_view->AddLine(d.RightBottom(), d.RightTop(), lo2);
			m_view->EndLineArray();
			m_view->SetLowColor(ViewColor());
			d.InsetBy(1, 1);
			m_view->FillRect(d, B_SOLID_LOW);
			m_view->Sync();
			m_bitmap->Unlock();
		}
		void MouseDown(BPoint where)
		{
			MouseMoved(where, B_INSIDE_VIEW, NULL);
		}
		void MouseMoved(BPoint where, uint32 code, const BMessage * message)
		{
			switch (code) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW: {
					uint32 buttons;
					BPoint junk;
					GetMouse(&junk, &buttons);
					if (buttons) {
						BRect c(Bounds().InsetByCopy(XBORDER, YBORDER));
						BRect d(c.InsetByCopy(1, 1));
						d.right = d.left + KNOBWIDTH;
						float left = floor(d.left+d.right)/2;
						float right = left+c.Width()-d.Width();
						m_volume = (where.x-left)/(right-left);
						if (m_volume < 0.0) m_volume = 0.0;
						if (m_volume > 1.0) m_volume = 1.0;
						RegenBitmap();
						Draw(Bounds());
						Sync();
						if (m_masterGain != 0) {
							BContinuousParameter * bcp = dynamic_cast<BContinuousParameter *>(m_masterGain);
							if (bcp != 0) {
								float mi, ma;
								mi = bcp->MinValue();
								ma = bcp->MaxValue();
								float v[8];
								for (int ix=0; ix<8; ix++) {
									v[ix] = m_volume*(ma-mi)+mi;
								}
								m_masterGain->SetValue(v, sizeof(v), system_time());
							}
						}
					}
					m_insideView = true;
					m_openTime = system_time();
				}
				break;
			case B_EXITED_VIEW:
				m_insideView = false;
				m_openTime = system_time();
				break;
			}
		}
		void AllAttached()
		{
			BView::AllAttached();
			Window()->SetPulseRate(100000);
			MakeFocus(true);
			m_openTime = system_time();
			m_insideView = true;
			RegenBitmap();
		}
		void Pulse()
		{
			bool dur = m_insideView;
			BPoint pt; uint32 bt;
			GetMouse(&pt, &bt);
			if (bt && m_insideView) return;	//	no auto-hide
			if (bt) dur = true;
			if (system_time() > m_openTime + (dur ? LONGTIME : SHORTTIME)) {
				Window()->Quit();	//	bye-bye!
			}
		}
private:
		bigtime_t m_openTime;
		bool m_insideView;
		BBitmap * m_bitmap;
		BView * m_view;
		float m_volume;
		BMediaRoster * m_roster;
		BParameter * m_masterGain;		
};
#endif

#ifdef MAGIC
class MagicWindow : public BWindow
{
public:
		//	obnoxious window flags
		MagicWindow(BRect frame, const char * name) :
			BWindow(frame, name, B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL, 
				B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MOVABLE | B_NOT_ZOOMABLE)
		{
			AddChild(new MagicView(Bounds(), name));
		}
		~MagicWindow();
};
#endif

#define WIN_WIDTH 150
#define WIN_HEIGHT 23

class LinkView : public BView
{
	bigtime_t m_delay;
	bigtime_t m_showTime;
public:
	LinkView(
		const char * path,
		const char * label,
		bool magic) : 
	BView(BRect(0,0,15,15), path, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW)
	{
		m_bitmap = 0;
		m_name = strdup(path);
		m_window = NULL;
		m_selected = false;
		m_selectedBitmap = 0;
		if (label) {
			m_label = strdup(label);
		}
		else {
			m_label = NULL;
		}
		m_magic = magic;
		m_showTime = B_INFINITE_TIMEOUT;
		m_delay = DEFAULT_SHOW_DELAY;
		m_runner = 0;
	}
	~LinkView()
	{
		delete m_runner;
		free(m_name);
		free(m_label);
		delete m_bitmap;
		delete m_selectedBitmap;
	}
	LinkView(
		BMessage * message) : 
	BView(message)
	{
		const char * name = 0;
		message->FindString("be:path", &name);
		entry_ref ref;
		m_name = 0;
		m_bitmap = 0;
		m_window = 0;
		if (name != 0) {
			m_name = strdup(name);
			if (g_verbose) fprintf(stderr, "desklink target: %s\n", name);
		}
		const char * label = 0;
		message->FindString("be:label", &label);
		if (label) {
			m_label = strdup(label);
		}
		else {
			m_label = 0;
		}
		m_magic = false;
		message->FindBool("be:_magic", &m_magic);
		if ((name != 0) && !get_ref_for_path(name, &ref)) {
			m_bitmap = new BBitmap(BRect(0,0,15,15), B_CMAP8);
			BNodeInfo::GetTrackerIcon((const entry_ref *)&ref, m_bitmap, B_MINI_ICON);
		}
		BBitmap * bm = new BBitmap(m_bitmap);
		uchar * bits = (uchar *)bm->Bits();
		uchar * end = bm->BitsLength()+bits;
		BScreen scrn;
		while (bits < end) {
			if (*bits != B_TRANSPARENT_8_BIT) {
				rgb_color c = scrn.ColorForIndex(*bits);
				c.red >>= 1;
				c.green >>= 1;
				c.blue >>= 1;
				*bits = scrn.IndexForColor(c);
			}
			bits++;
		}
		m_selectedBitmap = bm;
		m_selected = false;
		const char * menuitem;
		const char * menuaction;
		for (int ix=0; !message->FindString("be:menuitem", ix, &menuitem) &&
				!message->FindString("be:menuaction", ix, &menuaction); ix++) {
			m_items.push_back(pair<string,string>(menuitem, menuaction));
		}
		message->FindInt64("be:delay", &m_delay);
		m_runner = 0;
	}
	status_t Archive(
		BMessage * into, 
		bool deep) const
	{
		status_t err = BView::Archive(into, deep);
		if (err < 0)
			return err;
		if (m_name) {
			into->AddString("add_on", "application/x-vnd.be.desklink");
			for (list<pair<string,string> >::iterator ptr = m_items.begin(); ptr != m_items.end(); ptr++) {
				into->AddString("be:menuitem", (*ptr).first.c_str());
				into->AddString("be:menuaction", (*ptr).second.c_str());
			}
			into->AddBool("be:_magic", m_magic);
			if (m_label) into->AddString("be:label", m_label);
			into->AddInt64("be:delay", m_delay);
			return into->AddString("be:path", m_name);
		}
		return B_ERROR;
	}
static	BArchivable * Instantiate(
		BMessage * from);
	void Draw(
		BRect area)
	{
		SetDrawingMode(B_OP_OVER);
		if (m_bitmap != 0) {
			if (m_selected) {
				DrawBitmapAsync(m_selectedBitmap, BPoint(0,0));
			}
			else {
				DrawBitmapAsync(m_bitmap, BPoint(0,0));
			}
		}
	}
	void AttachedToWindow()
	{
		BView::AttachedToWindow();
		if (Parent() != 0)
			SetViewColor(Parent()->ViewColor());
	}
	void MouseDown(
		BPoint where)
	{
		uint32 buttons = 0;
		Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
		int32 modifiers = 0;
		Window()->CurrentMessage()->FindInt32("modifiers", &modifiers);
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) && !(modifiers & B_CONTROL_KEY)) {
			if (m_magic) {
#ifdef MAGIC
				RunMagic(where);
#endif
			}
			else {
				bool prev = false;
				while (buttons != 0) {
					GetMouse(&where, &buttons);
					m_selected = Bounds().Contains(where);
					if (prev != m_selected) {
						Draw(Bounds());
						Flush();
						prev = m_selected;
					}
					else if (buttons) {
						snooze(30000);
					}
				}
				if (m_selected) {
					m_selected = false;
					Draw(Bounds());
					Flush();
					BMessage msg('open');
					msg.AddString("be:path", m_name);
					Window()->PostMessage(&msg, this);
				}
			}
		}
		else {
			RunMenu(where);
		}
	}
	void RunMenu(
		BPoint where)
	{
		const char * tip = tip_name(false);
		BPopUpMenu * m = new BPopUpMenu(tip, false, false);
		char name[1200];
		sprintf(name, "Open %s", tip);
		BMessage * msg = new BMessage('open');
		msg->AddString("be:path", m_name);
		m->AddItem(new BMenuItem(name, msg));
		if (m_items.size() > 0) {
			m->AddSeparatorItem();
			for (list<pair<string,string> >::iterator ptr(m_items.begin()); ptr != m_items.end(); ptr++) {
				msg = new BMessage('run ');
				msg->AddString("be:script", (*ptr).second.c_str());
				m->AddItem(new BMenuItem((*ptr).first.c_str(), msg));
			}
		}
		for (int ix=0; ix<m->CountItems(); ix++)
			m->ItemAt(ix)->SetTarget(this, Window());
		BPoint p(ConvertToScreen(where));
		p.x -= 8;
		p.y -= 8;
		(void)m->Go(p, true, true, true);
	}
#ifdef MAGIC
	void RunMagic(BPoint where)
	{
		if (m_magic_showing) return;
		m_magic_showing = true;
		BPoint p(ConvertToScreen(where));
		BRect area(0,0,WIN_WIDTH,WIN_HEIGHT);
		BScreen scrn(Window());
		BRect f(scrn.Frame());
		if (p.y < 200) {
			area.OffsetBy(p.x-75, p.y+8);
		}
		else {
			area.OffsetBy(p.x-75, p.y-32);
		}
		if (area.right > f.right) {
			area.OffsetBy(f.right-area.right, 0);
		}
		if (area.bottom > f.bottom) {
			area.OffsetBy(0, f.bottom-area.bottom);
		}
		if (area.left < f.left) {
			area.OffsetBy(f.left-area.left, 0);
		}
		if (area.top < f.top) {
			area.OffsetBy(0, f.top-area.top);
		}
		(new MagicWindow(area, tip_name(true)))->Show();
	}
#endif
	const char * tip_name(bool allow_label)
	{
		if (allow_label && (m_label != NULL)) {
			return m_label;
		}
		char * tip = strrchr(m_name, '/');
		if (tip && tip[1]) {
			tip++;
		}
		else {
			tip = m_name;
		}
		return tip;
	}
	void MouseMoved(
		BPoint where,
		uint32 code,
		const BMessage * message)
	{
		if (code == B_ENTERED_VIEW) {
			if (!m_window->Lock()) {
				m_showTime = system_time()+m_delay;
				m_window = NULL;
				BMessage msg('puls');
				m_runner = new BMessageRunner(BMessenger(this), &msg, 200000LL);
			}
			else {
				m_window->Unlock();
			}
		}
		else if ((code == B_EXITED_VIEW) || (code == B_OUTSIDE_VIEW)) {
			if (m_window->Lock()) {
				m_window->Quit();
				m_window = NULL;
			}
			m_showTime = B_INFINITE_TIMEOUT;
			delete m_runner;
			m_runner = 0;
		}
	}
	void MessageReceived(
		BMessage * msg)
	{
		const char * path = NULL;
		if ((msg->what == 'open') && !msg->FindString("be:path", &path)) {
			BMessage msg(B_REFS_RECEIVED);
			entry_ref ref;
			status_t err;
			if ((err = get_ref_for_path(path, &ref)) == B_OK) {
				if (g_verbose) fprintf(stderr, "LinkView: open(%s)\n", path);
				msg.AddRef("refs", &ref);
				BMessenger msgr("application/x-vnd.Be-TRAK");
				err = msgr.SendMessage(&msg);
			}
			if (err < B_OK) {
				beep();
				if (g_verbose) fprintf(stderr, "LinkView: %s: %s\n", m_name, strerror(err));
			}
		}
		else if ((msg->what == 'run ') && !msg->FindString("be:script", &path)) {
			if (g_verbose) fprintf(stderr, "LinkView: system(%s)\n", path);
			system(path);
		}
		else if ((msg->what == 'puls') && (system_time() > m_showTime)) {
//			printf("time %Ld, m_showTime %Ld, m_delay %Ld\n", system_time(), m_showTime, m_delay);
			m_showTime = B_INFINITE_TIMEOUT;
			const char * tip = tip_name(true);
			BPoint center(8, 8);
			m_window = new ToolTipWindow(ConvertToScreen(center), tip);
			m_window->Show();
			delete m_runner;
			m_runner = 0;
		}
	}
	void SetItems(
		const list<pair<string,string> > & list)
	{
		m_items = list;
	}
	void SetShowDelay(
		bigtime_t delay)
	{
		m_delay = delay;
	}
private:
#ifdef MAGIC
	friend class MagicWindow;
#endif
	char * m_name;
	char * m_label;
	BBitmap * m_bitmap;
	BBitmap * m_selectedBitmap;
	BWindow * m_window;
	bool m_selected;
	bool m_magic;
	BMessageRunner * m_runner;
#ifdef MAGIC
static bool m_magic_showing;
#endif
	mutable list<pair<string,string> > m_items;
};

#ifdef MAGIC
bool LinkView::m_magic_showing = false;


MagicWindow::~MagicWindow()
{
	LinkView::m_magic_showing = false;
}
#endif

int
main(
	int argc, 
	char * argv[])
{
	if (argc < 2) {
		fprintf(stderr, "usage: desklink { [ cmd=title:action ... ] path } ...\n");
		return 1;
	}
	BApplication app("application/x-vnd.be.desklink");
	BDeskbar deskbar;
	int ok = 0;
	int32 delay = DEFAULT_SHOW_DELAY;
	list<pair<string,string> > pending;
	const char * label = NULL;
	bool magic = false;
	for (int ix=1; ix<argc; ix++) {
		if (!strncmp(argv[ix], "cmd=", 4)) {
			char * term = strchr(&argv[ix][4], ':');
			if (term) {
				char * name = (char *)malloc(1+(term-&argv[ix][4]));
				if (!name) {
					perror(argv[ix]);
					exit(1);
				}
				strncpy(name, &argv[ix][4], term-&argv[ix][4]);
				name[term-&argv[ix][4]] = 0;
				term++;
				pending.push_back(pair<string,string>(name, term));
				free(name);
			}
			else {
				fprintf(stderr, "desklink: usage: cmd=title:action\n");
			}
			continue;
		}
		else if (!strncmp(argv[ix], "label=", 6)) {
			if (label) {
				fprintf(stderr, "desklink: warning: label already specified as '%s'\n", label);
			}
			label = &argv[ix][6];
			continue;
		}
		else if (!strcmp(argv[ix], "magic=media")) {
			show_deskbar_icon(true);
			magic = true;
			return 1;
		}
		else if (!strncmp(argv[ix], "remove=", 7)) {
			status_t err = deskbar.RemoveItem(&argv[ix][7]);
			if (err < B_OK) {
				fprintf(stderr, "desklink: cannot remove '%s': %s\n", &argv[ix][7], strerror(err));
			}
			continue;
		}
		else if (!strncmp(argv[ix], "delay=", 6)) {
			delay = atoi(&argv[ix][6]);
			if (delay < 0) {
				fprintf(stderr, "desklink: can't use negative delay %ld\n", delay);
				delay = DEFAULT_SHOW_DELAY;
			}
			continue;
		}
		struct stat st;
		if (stat(argv[ix], &st) < 0) {
			fprintf(stderr, "desklink: cannot find '%s'\n", argv[ix]);
			continue;
		}

		LinkView * view = new LinkView(argv[ix], label, magic);		
		label = NULL;
		magic = false;
		view->SetItems(pending);
		pending.clear();
		view->SetShowDelay(delay);
#if OLD_WAY
		BMessage archive(B_ARCHIVED_OBJECT);
		view->Archive(&archive, true);
		BMessenger msgr("application/x-vnd.Be-TSKB");
		BMessage reply;
		int32 error = 0;
		//archive.PrintToStream();
		if (msgr.SendMessage(&archive, &reply) < B_OK) {
			fprintf(stderr, "desklink: error installing '%s'\n", argv[ix]);
		}
		else if (!reply.FindInt32("error", &error) && (error < 0)) {
			fprintf(stderr, "desklink: Deskbar refuses link to '%s': %s\n", argv[ix], strerror(error));
			//reply.PrintToStream();
		}
		else {
			ok++;
		}
#else
		status_t error = deskbar.AddItem(view);
		if (error < B_OK) {
			fprintf(stderr, "desklink: Deskbar refuses link to '%s': %s\n", argv[ix], strerror(error));
		}
		else {
			ok++;
		}
#endif
		delay = DEFAULT_SHOW_DELAY;
	}
	if (pending.size() > 0) {
		fprintf(stderr, "desklink: commands were given at end of line (no path); they were ignored.\n");
	}
	//	force a reference to Instantiate
	return (LinkView::Instantiate(0) == 0) ? ((ok > 0) ? 0 : 1) : 1;
}


/*	put it after so f*cking GCC doesn't inline it	*/
BArchivable *
LinkView::Instantiate(
	BMessage * from)
{
	if (!from) return NULL;
	const char * name = 0;
	from->FindString("be:path", &name);
	if (!name) return NULL;
	struct stat st;
	if (stat(name, &st) < 0) {
		if (g_verbose) fprintf(stderr, "LinkView: stat(%s) failed\n", name);
		return 0;
	}
	return new LinkView(from);
}
