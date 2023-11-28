
#include <Application.h>
#include <Window.h>
#include <TextView.h>
#include <ScrollView.h>
#include <ChannelSlider.h>
#include <CheckBox.h>
#include <OptionPopUp.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>
#include <StringView.h>
#include <map>

#include "game_audio2.h"
#include "controls.h"


class LevelControl;
class MuxControl;
class EnableControl;



static void
tprintf(
	BTextView * tv,
	const char * fmt,
	...)
{
	va_list args;
	va_start(args, fmt);
	char str[1024];
	vsprintf(str, fmt, args);
	va_end(args);
	tv->Select(tv->TextLength(), tv->TextLength());
	tv->Insert(str);
	tv->Flush();
}

struct game_control_info : public game_mixer_control {
	game_control_info() {
		type = 0;
		control_id = 0;
		mixer_id = 0;
		parent_id = 0;
	}
	~game_control_info() {
		if (type == GAME_MIXER_CONTROL_IS_MUX)
			delete[] mux.items;
	}
	game_control_info(const game_control_info & other) : game_mixer_control(other) {
		switch (type) {
		case GAME_MIXER_CONTROL_IS_LEVEL:
			level = other.level;
			break;
		case GAME_MIXER_CONTROL_IS_MUX:
			mux = other.mux;
			mux.items = new game_mux_item[mux.out_actual_count];
			memcpy(mux.items, other.mux.items, sizeof(game_mux_item)*mux.out_actual_count);
			break;
		case GAME_MIXER_CONTROL_IS_ENABLE:
			enable = other.enable;
			break;
		}
	}
	game_control_info & operator=(const game_control_info & other) {
		if (type == GAME_MIXER_CONTROL_IS_MUX) {
			delete[] mux.items;
		}
		type = other.type;
		control_id = other.control_id;
		mixer_id = other.mixer_id;
		switch (type) {
		case GAME_MIXER_CONTROL_IS_LEVEL:
			level = other.level;
			break;
		case GAME_MIXER_CONTROL_IS_MUX:
			mux = other.mux;
			mux.items = new game_mux_item[mux.out_actual_count];
			memcpy(mux.items, other.mux.items, sizeof(game_mux_item)*mux.out_actual_count);
			break;
		case GAME_MIXER_CONTROL_IS_ENABLE:
			enable = other.enable;
			break;
		}
		return *this;
	}
	union {
		game_get_mixer_level_info level;
		game_get_mixer_mux_info mux;
		game_get_mixer_enable_info enable;
	};
	game_control_info & operator=(const game_get_mixer_level_info & nuLevel) {
		if (type == GAME_MIXER_CONTROL_IS_MUX)
			delete[] mux.items;
		type = GAME_MIXER_CONTROL_IS_LEVEL;
		level = nuLevel;
		control_id = nuLevel.control_id;
		mixer_id = nuLevel.mixer_id;
		return *this;
	}
	game_control_info & operator=(const game_get_mixer_mux_info & nuMux) {
		if (type == GAME_MIXER_CONTROL_IS_MUX)
			delete[] mux.items;
		type = GAME_MIXER_CONTROL_IS_MUX;
		mux = nuMux;
		mux.items = new game_mux_item[mux.out_actual_count];
		memcpy(mux.items, nuMux.items, sizeof(game_mux_item)*mux.out_actual_count);
		control_id = nuMux.control_id;
		mixer_id = nuMux.mixer_id;
		return *this;
	}
	game_control_info & operator=(const game_get_mixer_enable_info & nuEnable) {
		if (type == GAME_MIXER_CONTROL_IS_MUX)
			delete[] mux.items;
		type = GAME_MIXER_CONTROL_IS_ENABLE;
		enable = nuEnable;
		control_id = nuEnable.control_id;
		mixer_id = nuEnable.mixer_id;
		return *this;
	}
	game_control_info & operator=(const game_mixer_control & aControl) {
		*(game_mixer_control *)this = aControl;
		return *this;
	}
};


namespace gameaudiomix_cpp {

class MixerWindow : public BWindow {
public:
		MixerWindow(const BRect & frame, const char * name, int fd) :
			BWindow(frame, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
				B_ASYNCHRONOUS_CONTROLS),
			m_fd(fd) {
		}
		void SetViewFor(int id, GameMixerItem * item) {
			m_views[id] = item;
		}
		void Show() {
			Lock();
			for (map<int, GameMixerItem *>::iterator ptr(m_views.begin());
					ptr != m_views.end(); ptr++) {
				(*ptr).second->ReadValue(m_fd);
			}
			Unlock();
			BWindow::Show();
		}
		void MessageReceived(BMessage * msg) {
			if (msg->what == 'cch!') {
				int32 mixer = GAME_NO_ID;
				int32 control;
				if (!msg->FindInt32("mixer", &mixer)) {
					if (!msg->FindInt32("level", &control)) {
						m_views[control]->ApplyValue(m_fd);
					}
					else if (!msg->FindInt32("mux", &control)) {
						m_views[control]->ApplyValue(m_fd);
					}
					else if (!msg->FindInt32("enable", &control)) {
						m_views[control]->ApplyValue(m_fd);
					}
				}
			}
			else {
				BWindow::MessageReceived(msg);
			}
		}
private:
		int m_fd;
		map<int, GameMixerItem *> m_views;
};

}
using namespace gameaudiomix_cpp;

void
get_control_infos(
	int fd,
	const game_mixer_control * controls,
	game_control_info * infos,
	int count)
{
	for (int ix=0; ix<count; ix++) {
		switch (controls[ix].type) {
			case GAME_MIXER_CONTROL_IS_LEVEL: {
				G<game_get_mixer_level_info> ggmli;
				ggmli.control_id = controls[ix].control_id;
				ggmli.mixer_id = controls[ix].mixer_id;
				C(ioctl(fd, GAME_GET_MIXER_LEVEL_INFO, &ggmli));
				infos[ix] = ggmli;
			}
			break;
			case GAME_MIXER_CONTROL_IS_MUX: {
				G<game_get_mixer_mux_info> ggmmi;
				ggmmi.control_id = controls[ix].control_id;
				ggmmi.mixer_id = controls[ix].mixer_id;
				C(ioctl(fd, GAME_GET_MIXER_MUX_INFO, &ggmmi));
				ggmmi.in_request_count = ggmmi.out_actual_count;
				ggmmi.items = new game_mux_item[ggmmi.out_actual_count];
				C(ioctl(fd, GAME_GET_MIXER_MUX_INFO, &ggmmi));
				infos[ix] = ggmmi;
				delete[] ggmmi.items;
			}
			break;
			case GAME_MIXER_CONTROL_IS_ENABLE: {
				G<game_get_mixer_enable_info> ggmei;
				ggmei.control_id = controls[ix].control_id;
				ggmei.mixer_id = controls[ix].mixer_id;
				C(ioctl(fd, GAME_GET_MIXER_ENABLE_INFO, &ggmei));
				infos[ix] = ggmei;
			}
			break;
		}
		infos[ix] = controls[ix];
	}
}

static BView *
control_view_for(
	const game_control_info & control,
	int fd)
{
	BView * ret = 0;
	switch (control.type) {
		case GAME_MIXER_CONTROL_IS_LEVEL:
			if (control.flags & GAME_MIXER_CONTROL_AUXILIARY)
			{
				ret = new AuxLevelControl(control.level);
			}
			else
			{
				ret = new LevelControl(control.level);
			}
		break;
		case GAME_MIXER_CONTROL_IS_MUX:
			ret = new MuxControl(control.mux);
		break;
		case GAME_MIXER_CONTROL_IS_ENABLE:
			ret = new EnableControl(control.enable);
		break;
		default:
			ret = new BStringView(BRect(0,0,50,15), "unknown", "Unknown");
		break;
	}
	return ret;
}

static void
add_view_below(
	BView * child,
	BView * parent)
{
	if (!child) return;
	BRect r = parent->Bounds();
	float hor = 0;
	BRect r2 = child->Bounds();
	if (r2.right > r.right) {
		hor = r2.right-r.right;
	}
	parent->ResizeBy(hor, r2.bottom+1);
	child->MoveTo(0, r.bottom+1);
	parent->AddChild(child);
}

static BView *
make_control_view(
	const game_control_info * control,
	int count,
	const game_control_info & parent,
	int rec_level,
	int fd,
	MixerWindow * bw)
{
	if (rec_level > 20) {
		fprintf(stderr, "PANIC: recursion level in control hierarchy is too deep!\n");
		fprintf(stderr, "(you probably have a parent cycle somewhere)\n");
		fprintf(stderr, "Current control ID: %d (parent %d)\n", parent.control_id, parent.parent_id);
		exit(1);
	}
	BView * ret = control_view_for(parent, fd);
	bw->SetViewFor(parent.control_id, dynamic_cast<GameMixerItem *>(ret));
	bool isView = false;
	for (int ix=0; ix<count; ix++) {
		if ((control[ix].parent_id == parent.control_id) &&
			(control[ix].control_id != parent.control_id)) {
			if (!isView) {
				BView * r2 = new BView(ret->Bounds(), "bg", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW);
				r2->SetViewColor(192, 192, 192);
				r2->AddChild(ret);
				ret = r2;
				isView = true;
			}
			BView * sub = make_control_view(control, count, control[ix], rec_level+1, fd, bw);
			add_view_below(sub, ret);
		}
	}
	//	if (isView) {
	//		center_horiz(ret);
	//	}
	return ret;
}

static void
layout_controls(
	const game_control_info * control,
	int count,
	BView * parent,
	BTextView * tv,
	int fd,
	MixerWindow * bw)
{
	vector<BView *> strip;
	vector<BView *> aux;
	for (int ix=0; ix<count; ix++) {
		if ((control[ix].parent_id == GAME_NO_ID) || (control[ix].parent_id == control[ix].control_id)) {
			BView * v;
			if (control[ix].flags & GAME_MIXER_CONTROL_AUXILIARY) {
				aux.push_back(v = make_control_view(control, count, control[ix], 0, fd, bw));
			}
			else {
				strip.push_back(v = make_control_view(control, count, control[ix], 0, fd, bw));
			}
		}
	}
	float left = 0;
	float bottom = 0;
	//	add the strips at the top
	for (vector<BView *>::iterator ptr(strip.begin()); ptr != strip.end(); ptr++) {
		(*ptr)->MoveTo(left, 0);
		left = (*ptr)->Frame().right+1;
		if (bottom <= (*ptr)->Frame().bottom) {
			bottom = (*ptr)->Frame().bottom+1;
		}
		parent->AddChild(*ptr);
	}
	float top = bottom+10;
	float savetop = top;
	float right = 0;	//	how far we've gone
	float pos = 0;		//	where to put controls
	//	pack in the aux depending on their orientation, but in order
	for (vector<BView *>::iterator ptr(aux.begin()); ptr != aux.end(); ptr++) {
		//	stack horizontal controls on top of each other in groups;
		//	vertical controls beside each other in groups;
		//	if it's every other, it'll look bad but that's OK...
		bool vertical = (*ptr)->Frame().bottom > (*ptr)->Frame().right;
		if (vertical) {
again_vert:
			if ((*ptr)->Frame().right + pos >= left) {
				if (pos > 0) {
					top = bottom+10;
					savetop = top;
					pos = 0;
					goto again_vert;
				}
			}
			(*ptr)->MoveTo(pos, savetop);
			parent->AddChild(*ptr);
			if (right <= (*ptr)->Frame().right) {
				right = (*ptr)->Frame().right;
				if (left < right) {
					left = right;
				}
			}
			if (bottom <= (*ptr)->Frame().bottom) {
				bottom = (*ptr)->Frame().bottom+1;
			}
			top = savetop;
			pos = right;
		}
		else {
again_horiz:
			if ((*ptr)->Frame().right + pos >= left) {
				if (pos > 0) {
					top = bottom+10;
					savetop = top;
					pos = 0;
					goto again_horiz;
				}
			}
			(*ptr)->MoveTo(pos, top);
			parent->AddChild(*ptr);
			if (right <= (*ptr)->Frame().right) {
				right = (*ptr)->Frame().right;
				if (left < right) {
					left = right;
				}
			}
			top = (*ptr)->Frame().bottom+1;
		}
	}
}

static void
make_mixer_window(
	int fd,
	int ix,
	const game_get_info & ggi)
{
	BRect r(20+20*ix, 20+30*ix, 720+20*ix, 550+30*ix);
	char name[256];
	sprintf(name, "%s by %s mixer %d\n", ggi.name, ggi.vendor, ix);
	MixerWindow * bw = new MixerWindow(r, name, fd);
	// B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
	//		B_ASYNCHRONOUS_CONTROLS);
	r = bw->Bounds();
	r.InsetBy(10, 10);
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom = r.top+130;
	BRect r2(r);
	r2.OffsetTo(0, 0);
	r2.left += 3;
	r2.top += 3;
	r2.right -= 2;
	BTextView * tv = new BTextView(r, "text", r2, B_FOLLOW_ALL, B_WILL_DRAW);
	BScrollView * sv = new BScrollView("scroll", tv, B_FOLLOW_ALL, B_WILL_DRAW, false, true);
	bw->AddChild(sv);

        tprintf(tv, "name=%s\n", ggi.name); //        char    name[32];
        tprintf(tv, "vendor=%s\n", ggi.vendor); //    char    vendor[32];
        tprintf(tv, "ordinal=%d\n", ggi.ordinal); //  int16   ordinal;
        tprintf(tv, "version=%d\n", ggi.version); //  int16   version;
//
        tprintf(tv, "dac_count=%d\n", ggi.dac_count); //      int16   dac_count;
        tprintf(tv, "adc_count=%d\n", ggi.adc_count); //      int16   adc_count;
        tprintf(tv, "mixer_count=%d\n", ggi.mixer_count); //  int16   mixer_count;
        tprintf(tv, "buffer_count=%d\n", ggi.buffer_count); //        int16   buffer_count;
        tprintf(tv, "\n");

	G<game_get_mixer_infos> ggmis;
	H<game_mixer_info> gmi;
	gmi.mixer_id = GAME_MAKE_MIXER_ID(ix);
	ggmis.info = &gmi;
	ggmis.in_request_count = 1;
	C(ioctl(fd, GAME_GET_MIXER_INFOS, &ggmis));

        tprintf(tv, "mixer_id=%d\n", gmi.mixer_id); //        int16   mixer_id;
        tprintf(tv, "linked_codec_id=%d\n", gmi.linked_codec_id); //  int16   linked_codec_id;
//
        tprintf(tv, "name=%s\n", gmi.name); //        char    name[32];
        tprintf(tv, "control_count=%d\n", gmi.control_count); //      int16   control_count;
        tprintf(tv, "\n");

	G<game_get_mixer_controls> ggmcs;
	ggmcs.control = new H<game_mixer_control>[gmi.control_count];
	ggmcs.mixer_id = gmi.mixer_id;
	ggmcs.in_request_count = gmi.control_count;
	ggmcs.from_ordinal = 0;
	C(ioctl(fd, GAME_GET_MIXER_CONTROLS, &ggmcs));

	if (ggmcs.out_actual_count != ggmcs.in_request_count) {
		tprintf(tv, "WARNING: info said %d controls, but request says there are %d\n", ggmcs.in_request_count, ggmcs.out_actual_count);
		tprintf(tv, "\n");
		if (ggmcs.out_actual_count > ggmcs.in_request_count) {
			ggmcs.out_actual_count = ggmcs.in_request_count;
		}
	}
	for (int ci=0; ci<ggmcs.out_actual_count; ci++) {
        tprintf(tv, "control_id=%d\n", ggmcs.control[ci].control_id); //    int16   control_id;
        tprintf(tv, "mixer_id=%d\n", ggmcs.control[ci].mixer_id); //        int16   mixer_id;
//
        tprintf(tv, "type=%d\n", ggmcs.control[ci].type); //        int16   type;
        tprintf(tv, "flags=0x%x\n", ggmcs.control[ci].flags); //    uint16  flags;
        tprintf(tv, "parent_id=%d\n", ggmcs.control[ci].parent_id); //      int16   parent_id;
        tprintf(tv, "\n");
	}

	r = bw->Bounds();
	r.top = sv->Frame().bottom+10;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	BView * parent = new BView(r, "container", B_FOLLOW_ALL, B_WILL_DRAW);
	parent->SetViewColor(192, 192, 192);
	game_control_info * cinfo = new game_control_info[ggmcs.out_actual_count];
	get_control_infos(fd, ggmcs.control, cinfo, ggmcs.out_actual_count);
	layout_controls(cinfo, ggmcs.out_actual_count, parent, tv, fd, bw);

	bw->AddChild(parent);

	r.top = r.bottom+1;
	r.bottom = r.top+B_H_SCROLL_BAR_HEIGHT;
	r.left--;
	r.right++;
	BScrollBar * sb = new BScrollBar(r, "horizontal", parent, 0, 600, B_HORIZONTAL);
	bw->AddChild(sb);
	sb->SetSteps(10, r.Width()-10);

	bw->Show();

	delete[] cinfo;
	delete[] ggmcs.control;
}

static void
make_mixer_windows(
	int fd)
{
	G<game_get_info> ggi;
	C(ioctl(fd, GAME_GET_INFO, &ggi));
	for (int ix=0; ix<ggi.mixer_count; ix++) {
		make_mixer_window(fd, ix, ggi);
	}
}

int
main(
	int argc,
	char * argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: gameaudiomix device\n");
		return 1;
	}
	int fd;
	C(fd = open(argv[1], O_RDWR));
	BApplication app("application/x-vnd.Be.game-audio-mixer");
	make_mixer_windows(fd);
	app.Run();
	close(fd);
	return 0;
}
