
/* As usual, there was nobody assigned to this task, so I had to hack something */
/* together over the week-end. The structure hereof is "interesting". */

#include <stdio.h>
#include <string.h>

#include "JoyCalib.h"

#include <Box.h>
#include <TextView.h>
#include <Font.h>
#include <Joystick.h>
#include <String.h>
#include <Screen.h>
#include <Button.h>
#include <Alert.h>


#define MIN_HEIGHT 17
#define MIN_WIDTH 60
#define BAR_WIDTH 187
#define GADGET_WIDTH 191
#define HAT_SQUARE 34

#define MODE 'mode'
#define NEXT 'next'
#define RECALC_BUTTONS 'recb'


static rgb_color lo = { 96, 96, 96, 255 };
static rgb_color lo_dis = { 160, 160, 160, 255 };
static rgb_color hi = { 255, 255, 255, 255 };
static rgb_color green_lo = { 32, 64, 32, 255 };
static rgb_color green_mid = { 48, 104, 48, 255 };
static rgb_color green_hi[3] = {
	{ 128, 255, 128, 255 },
	{ 96, 160, 96, 255 },
	{ 48, 104, 48, 255 }
};
static rgb_color red_lo = { 64, 32, 32, 255 };
static rgb_color red_hi = { 255, 128, 128, 255 };
static rgb_color blue_lo = { 32, 32, 64, 255 };
static rgb_color blue_hi = { 128, 128, 255, 255 };


class CalibAxis : public BView {
public:
		CalibAxis(
				BRect area,
				const char * name, 
				BJoystick & stick,
				int axis,
				float where,
				int16 & vptr,
				JoyCalib & calib) :
			BView(area, name, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW),
			m_stick(stick),
			m_value(vptr),
			m_calib(calib)
			{
				m_axis = axis;
				m_where = where;
			}
		~CalibAxis()
			{
			}

virtual	void MouseDown(
				BPoint pt)
			{
			}
virtual	void Draw(
				BRect area)
			{
				font_height fh;
				SetHighColor(0,0,0);
				be_plain_font->GetHeight(&fh);
				BString str;
				m_stick.GetAxisNameAt(m_axis, &str);
				DrawString(str.String(), BPoint(0, fh.ascent+fh.leading+1));
				BeginLineArray(4);
				float f = Bounds().bottom-3;
				AddLine(BPoint(m_where, f), BPoint(m_where+BAR_WIDTH, f), hi);
				AddLine(BPoint(m_where+BAR_WIDTH, 3), BPoint(m_where+BAR_WIDTH, f), hi);
				AddLine(BPoint(m_where, 3), BPoint(m_where+BAR_WIDTH, 3), lo);
				AddLine(BPoint(m_where, 3), BPoint(m_where, f), lo);
				EndLineArray();
				DrawValue();
			}
		void DrawValue()	/* CalibAxis */
			{
				float f = Bounds().bottom-3;
				BRect r(m_where+1, 4, m_where+BAR_WIDTH-1, f-1);
				SetHighColor(green_lo);
				FillRect(r, B_SOLID_HIGH);
				if (m_calib.Mode() < 4) {
					SetHighColor(green_mid);
					float x1 = (BAR_WIDTH/2-2);
					float x2 = x1;
					x1 += x1*(float)m_calib.m_ax_min[m_axis]/32768.0;
					x2 += x2*(float)m_calib.m_ax_max[m_axis]/32768.0;
					r.left = m_where+x1+2;
					r.right = m_where+x2;
					FillRect(r, B_SOLID_HIGH);
				}
				BeginLineArray(5);
				float g = (BAR_WIDTH/2-2);
				g += g*(float)m_value/32768.0;
				g = floor(g+2);
				AddLine(BPoint(m_where-1+g, 4), BPoint(m_where-1+g, f-1), green_hi[2]);
				AddLine(BPoint(m_where+g, 4), BPoint(m_where+g, f-1), green_hi[1]);
				AddLine(BPoint(m_where+1+g, 4), BPoint(m_where+1+g, f-1), green_hi[0]);
				AddLine(BPoint(m_where+2+g, 4), BPoint(m_where+2+g, f-1), green_hi[1]);
				AddLine(BPoint(m_where+3+g, 4), BPoint(m_where+3+g, f-1), green_hi[2]);
				EndLineArray();
			}
virtual	void MakeFocus(
				bool focus)
			{
				BView::MakeFocus(focus);
			}

private:
		BJoystick & m_stick;
		int m_axis;
		float m_where;
		int16 & m_value;
		JoyCalib & m_calib;
};

class CalibButton : public BView {
public:
		CalibButton(
				BRect area,
				const char * name, 
				BJoystick & stick,
				int button,
				float where,
				uint32 & vptr,
				JoyCalib & calib) :
			BView(area, name, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW),
			m_stick(stick),
			m_value(vptr),
			m_calib(calib)
			{
				m_button = button;
				m_where = where;
				m_auto.left = m_where+area.Height()+10;
				m_auto.top = 2;
				m_auto.right = m_auto.left+StringWidth("Auto")+7;
				m_auto.bottom = area.Height()-2;
				m_is_auto = false;
				m_latch = m_auto;
				m_latch.left = m_auto.right+10;
				m_latch.right = m_latch.left+StringWidth("Latch")+7;
				m_is_latch = false;
			}
		~CalibButton()
			{
			}

virtual	void MouseDown(
				BPoint pt)
			{
				if (m_calib.Mode() < 4) return;
				if (m_auto.Contains(pt)) {
					m_is_auto = !m_is_auto;
					Draw(m_auto);
					Window()->PostMessage(RECALC_BUTTONS);
				}
				else if (m_latch.Contains(pt)) {
					m_is_latch = !m_is_latch;
					Draw(m_latch);
					Window()->PostMessage(RECALC_BUTTONS);
				}
			}
virtual	void Draw(
				BRect area)
			{
				font_height fh;
				SetHighColor(0,0,0);
				be_plain_font->GetHeight(&fh);
				BString str;
				m_stick.GetButtonNameAt(m_button, &str);
				DrawString(str.String(), BPoint(0, fh.ascent+fh.leading+1));
				BeginLineArray(4);
				float f = Bounds().bottom-3;
				float bw = f-3;
				AddLine(BPoint(m_where, f), BPoint(m_where+bw, f), hi);
				AddLine(BPoint(m_where+bw, f), BPoint(m_where+bw, 3), hi);
				AddLine(BPoint(m_where, 3), BPoint(m_where+bw, 3), lo);
				AddLine(BPoint(m_where, 3), BPoint(m_where, f), lo);
				EndLineArray();

				DrawValue();

				BRect r = m_auto;
				bool b = m_is_auto;
				BeginLineArray(8);
				rgb_color lc = (m_calib.Mode() < 4) ? lo_dis : lo;
				AddLine(r.LeftTop(), r.RightTop(), lc);
				AddLine(r.LeftTop(), r.LeftBottom(), lc);
				AddLine(r.RightTop(), r.RightBottom(), lc);
				AddLine(r.LeftBottom(), r.RightBottom(), lc);
				r.InsetBy(1,1);
				AddLine(r.LeftTop(), r.RightTop(), b ? lc : hi);
				AddLine(r.LeftTop(), r.LeftBottom(), b ? lc : hi);
				AddLine(r.RightTop(), r.RightBottom(), b ? hi : lc);
				AddLine(r.LeftBottom(), r.RightBottom(), b ? hi : lc);
				EndLineArray();
				r.InsetBy(1,1);
				lc = ViewColor();
				if (m_is_auto) {
					lc.red = 255;
				}
				if (m_calib.Mode() < 4) {
					lc.red = (255+lc.red)/2;
					lc.green = (255+lc.green)/2;
					lc.blue = (255+lc.blue)/2;
				}
				SetLowColor(lc);
				FillRect(r, B_SOLID_LOW);
				float base = fh.ascent+fh.leading+1;
				if (base+fh.descent+1 < Bounds().Height()) {
					base += floor((Bounds().Height()-base-fh.descent-1)/2);
				}
				if (m_calib.Mode() < 4) {
					SetHighColor(lo);
				}
				else {
					SetHighColor(0, 0, 0);
				}
				DrawString("Auto", BPoint(m_auto.left+(m_is_auto ? 5 : 4), base + (m_is_auto ? 1 : 0)));

				r = m_latch;
				b = m_is_latch;
				lc = (m_calib.Mode() < 4) ? lo_dis : lo;
				BeginLineArray(8);
				AddLine(r.LeftTop(), r.RightTop(), lc);
				AddLine(r.LeftTop(), r.LeftBottom(), lc);
				AddLine(r.RightTop(), r.RightBottom(), lc);
				AddLine(r.LeftBottom(), r.RightBottom(), lc);
				r.InsetBy(1,1);
				AddLine(r.LeftTop(), r.RightTop(), b ? lc : hi);
				AddLine(r.LeftTop(), r.LeftBottom(), b ? lc : hi);
				AddLine(r.RightTop(), r.RightBottom(), b ? hi : lc);
				AddLine(r.LeftBottom(), r.RightBottom(), b ? hi : lc);
				EndLineArray();
				r.InsetBy(1,1);
				lc = ViewColor();
				if (m_is_latch) {
					lc.red = 255;
				}
				if (m_calib.Mode() < 4) {
					lc.red = (255+lc.red)/2;
					lc.green = (255+lc.green)/2;
					lc.blue = (255+lc.blue)/2;
				}
				SetLowColor(lc);
				FillRect(r, B_SOLID_LOW);
				if (m_calib.Mode() < 4) {
					SetHighColor(lo);
				}
				else {
					SetHighColor(0, 0, 0);
				}
				DrawString("Latch", BPoint(m_latch.left+(m_is_latch ? 5 : 4), base + (m_is_latch ? 1 : 0)));
			}
		void DrawValue()	/* CalibButton */
			{
				float f = Bounds().bottom-3;
				float bw = f-3;
				BRect r(m_where+1, 4, m_where+bw-1, f-1);
				if (m_value & (1 << m_button)) {
					SetHighColor(red_hi);
				}
				else {
					SetHighColor(red_lo);
				}
				FillRect(r, B_SOLID_HIGH);
			}
virtual	void MakeFocus(
				bool focus)
			{
				BView::MakeFocus(focus);
			}

private:
friend class JoyCalib;
		BJoystick & m_stick;
		JoyCalib & m_calib;
		int m_button;
		float m_where;
		uint32 & m_value;
		BRect m_auto;
		BRect m_latch;
		bool m_is_auto;
		bool m_is_latch;
};

class CalibHat : public BView {
public:
		CalibHat(
				BRect area,
				const char * name, 
				BJoystick & stick,
				int axis,
				float where,
				uint8 & vptr,
				JoyCalib & calib) :
			BView(area, name, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW),
			m_stick(stick),
			m_value(vptr),
			m_calib(calib),
			m_axis(axis),
			m_where(where)
			{
			}
		~CalibHat()
			{
			}

virtual	void MouseDown(
				BPoint pt)
			{
				&pt;
			}
virtual	void Draw(
				BRect area)
			{
				font_height fh;
				SetHighColor(0,0,0);
				be_plain_font->GetHeight(&fh);
				BString str;
				m_stick.GetHatNameAt(m_axis, &str);
				DrawString(str.String(), BPoint(0, fh.ascent+fh.leading+1));
				BeginLineArray(4);
				float f = Bounds().bottom-3;
				float bw = f-3;
				AddLine(BPoint(m_where, f), BPoint(m_where+bw, f), hi);
				AddLine(BPoint(m_where+bw, f), BPoint(m_where+bw, 3), hi);
				AddLine(BPoint(m_where, 3), BPoint(m_where+bw, 3), lo);
				AddLine(BPoint(m_where, 3), BPoint(m_where, f), lo);
				EndLineArray();

				DrawValue();
			}
		void DrawValue()	/* CalibHat */
			{
				float f = Bounds().bottom-3;
				float bw = f-3;
				BRect r(m_where+1, 4, m_where+bw-1, f-1);
				BRect r2(r);
				float fh = floor(r.Height()/2-1);
				r2.InsetBy(fh, fh);
				fh -= 1;
				switch (m_value) {
				case 1:
					r2.OffsetBy(0, -fh);
					break;
				case 2:
					r2.OffsetBy(fh*.75, -fh*.75);
					break;
				case 3:
					r2.OffsetBy(fh, 0);
					break;
				case 4:
					r2.OffsetBy(fh*.75, fh*.75);
					break;
				case 5:
					r2.OffsetBy(0, fh);
					break;
				case 6:
					r2.OffsetBy(-fh*.75, fh*.75);
					break;
				case 7:
					r2.OffsetBy(-fh, 0);
					break;
				case 8:
					r2.OffsetBy(-fh*.75, -fh*.75);
					break;
				}
				SetHighColor(blue_lo);
				FillRect(r, B_SOLID_HIGH);
				SetHighColor(blue_hi);
				FillRect(r2, B_SOLID_HIGH);
				r2.InsetBy(-1, -1);
				SetHighColor((blue_hi.red+blue_lo.red)/2, 
					(blue_hi.green+blue_lo.green)/2, 
					(blue_hi.blue+blue_lo.blue)/2, 
					255);
				StrokeRect(r2, B_SOLID_HIGH);
			}
virtual	void MakeFocus(
				bool focus)
			{
				BView::MakeFocus(focus);
			}

private:
friend class JoyCalib;
		BJoystick & m_stick;
		JoyCalib & m_calib;
		float m_where;
		uint8 & m_value;
		int m_axis;
};


static BRect GetFrame(BRect parent, BJoystick & stick, float * where) {
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float height = fh.ascent + fh.descent + fh.leading + 2;
	if (height < MIN_HEIGHT) height = MIN_HEIGHT;
	BString string;
	float width = MIN_WIDTH;
	for (int ix=0; ix<stick.CountAxes(); ix++) {
		stick.GetAxisNameAt(ix, &string);
		float x = be_plain_font->StringWidth(string.String());
		if (x > width) width = x;
	}
	for (int ix=0; ix<stick.CountButtons(); ix++) {
		stick.GetButtonNameAt(ix, &string);
		float x = be_plain_font->StringWidth(string.String());
		if (x > width) width = x;
	}
	*where = width+10;
	parent.bottom = parent.top+height*6+50 + stick.CountAxes()*height + 
		stick.CountButtons()*height + stick.CountHats()*HAT_SQUARE;
	parent.right = parent.left + width + 30 + GADGET_WIDTH;
	BScreen scrn;
	BRect f = scrn.Frame();
	if (parent.right > f.right-10) {
		parent.OffsetBy(f.right-10-parent.right, 0);
	}
	if (parent.bottom > f.bottom-10) {
		parent.OffsetBy(0, f.bottom-10-parent.bottom);
	}
	if (parent.left < f.left+10) {
		parent.OffsetBy(f.left+10-parent.left, 0);
	}
	if (parent.top < f.top+10) {
		parent.OffsetBy(0, f.top+10-parent.top);
	}
	return parent;
}


JoyCalib::JoyCalib(
	BRect parent,
	BJoystick & stick, 
	BWindow * show) :
	m_stick(stick),
	BWindow(GetFrame(parent, stick, &m_where), "Calibration", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, 
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE)
{
	m_stick.EnableCalibration(false);
	m_info = _BJoystickTweaker(m_stick).get_info();
//	m_info = new BJoystick::_joystick_info;
//	memcpy(m_info, _BJoystickTweaker(m_stick).get_info(), sizeof(*m_info));
	m_prev_info = *m_info;
	m_last_click = 0;
	m_show = show;
	BBox * bg = new BBox(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(bg);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	bg->SetLowColor(bg->ViewColor());
	BRect r(Bounds());
	r.InsetBy(10,10);
	font_height bfh;
	be_bold_font->GetHeight(&bfh);
	font_height pfh;
	be_plain_font->GetHeight(&pfh);
	r.bottom = r.top + bfh.ascent+bfh.descent+bfh.leading + 
		(pfh.ascent+pfh.descent+pfh.leading)*2+10;
	BBox * f = new BBox(r, "Instructions", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER);
	f->SetLabel("Instructions");
	bg->AddChild(f);
	f->SetViewColor(bg->ViewColor());
	f->SetLowColor(bg->ViewColor());
	r = f->Bounds();
	r.InsetBy(5,5);
	r.top += bfh.ascent+bfh.descent+bfh.leading;
	BRect r2(r);
	r2.OffsetTo(B_ORIGIN);
	m_instructions = new BTextView(r, "Text", r2, B_FOLLOW_ALL, B_WILL_DRAW);
	f->AddChild(m_instructions);
	m_instructions->MakeEditable(false);
	m_instructions->MakeSelectable(false);
	m_instructions->SetViewColor(bg->ViewColor());
	m_instructions->SetLowColor(bg->ViewColor());
	m_instructions->SetText("¡Hola!\n");

	r = f->Frame();
	r.InsetBy(2,0);
	r.top = r.bottom+10;
	int height = (int)(pfh.ascent+pfh.descent+pfh.leading+2);
	if (height < MIN_HEIGHT) height = MIN_HEIGHT;
	r.bottom = r.top+height;
	for (int ix=0; ix<stick.CountAxes(); ix++) {
		char nm[30];
		sprintf(nm, "axis %d", ix+1);
		CalibAxis * ca = new CalibAxis(r, nm, stick, ix, m_where, m_values[ix], *this);
		ca->SetViewColor(bg->ViewColor());
		ca->SetLowColor(bg->ViewColor());
		bg->AddChild(ca);
		m_calib_views.AddItem((BView *)ca);
		r.OffsetBy(0, r.Height()+1);
	}
	if (HAT_SQUARE > height) {
		r.bottom = r.top+HAT_SQUARE;
	}
	for (int ix=0; ix<stick.CountHats(); ix++) {
		char nm[30];
		sprintf(nm, "hat %d", ix+1);
		CalibHat * ch = new CalibHat(r, nm, stick, ix, m_where, m_hats[ix], *this);
		ch->SetViewColor(bg->ViewColor());
		ch->SetLowColor(bg->ViewColor());
		bg->AddChild(ch);
		m_calib_views.AddItem((BView *)ch);
		r.OffsetBy(0, r.Height()+1);
	}
	r.bottom = r.top+height;
	for (int ix=0; ix<stick.CountButtons(); ix++) {
		char nm[30];
		sprintf(nm, "button %d", ix+1);
		CalibButton * ca = new CalibButton(r, nm, stick, ix, m_where, m_buttons, *this);
		ca->m_is_auto = (m_info->button_autofire & (1 << ix)) ? true : false;
		ca->m_is_latch = (m_info->button_latch & (1 << ix)) ? true : false;
		ca->SetViewColor(bg->ViewColor());
		ca->SetLowColor(bg->ViewColor());
		bg->AddChild(ca);
		m_calib_views.AddItem((BView *)ca);
		r.OffsetBy(0, r.Height()+1);
	}
	r = Bounds();
	r.InsetBy(10,10);
	r.top = r.bottom-25;
	r.left = r.right-80;
	BButton * next = new BButton(r, "next", "Done", new BMessage(NEXT), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	bg->AddChild(next);
	next->SetEnabled(false);
	r.OffsetBy(10-r.left, 0);
	BMessage * msg2 = new BMessage(MODE);
	msg2->AddInt32("be:_mode", 1);
	BButton * reset = new BButton(r, "reset", "Revert", msg2, 
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	bg->AddChild(reset);
	m_mode = 0;
	Pulse();
	memcpy(m_ax_min, m_values, sizeof(m_ax_min));
	memcpy(m_ax_max, m_values, sizeof(m_ax_max));
	m_wait = create_sem(0, "Joystick Calibration");
	m_idler = spawn_thread(Idle, "Idle", B_LOW_PRIORITY, this);
	resume_thread(m_idler);
	BMessage msg(MODE);
	msg.AddInt32("be:_mode", 1);
	PostMessage(&msg);
}

JoyCalib::~JoyCalib()
{
	delete_sem(m_wait);
	status_t s;
	wait_for_thread(m_idler, &s);
	m_stick.Close();
//	if (m_show->Lock()) {
//		m_show->Show();
//		m_show->Unlock();
//	}
}

status_t
JoyCalib::Idle(
	void * data)
{
	while (acquire_sem_etc(((JoyCalib *)data)->m_wait, 1, B_TIMEOUT, 100000) == B_TIMED_OUT) {
		((JoyCalib *)data)->PostMessage('puls');
	}
	return 0;
}

bool 
JoyCalib::QuitRequested()
{
	if (Mode() > 4) {
		status_t err = _BJoystickTweaker(m_stick).save_config(NULL);
		if (err < B_OK) {
			char str[300];
			sprintf(str, "There was an error saving '%s': %s [%x]", 
				m_info->file_name, strerror(err), err);
			(new BAlert("", str, "Stop"))->Go();
		}
	}
	return true;
}

void 
JoyCalib::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	default:
		BWindow::MessageReceived(message);
		break;
	case NEXT: {
		EnterMode(m_mode+1);
		} break;
	case MODE: {
		int32 m;
		if (!message->FindInt32("be:_mode", &m)) {
			EnterMode(m);
		}
		} break;
	case RECALC_BUTTONS: {
		uint32 aut = 0;
		uint32 lat = 0;
		for (int ix=0; ix<m_stick.CountButtons(); ix++) {
			char str[30];
			sprintf(str, "button %d", ix+1);
			CalibButton * bb = dynamic_cast<CalibButton *>(FindView(str));
			if (bb) {
				if (bb->m_is_auto) {
					aut |= (1 << ix);
				}
				if (bb->m_is_latch) {
					lat |= (1 << ix);
				}
			}
		}
		m_info->button_autofire = aut;
		m_info->button_latch = lat;
		m_info->prev_latch &= lat;
//		fprintf(stderr, "%03x %03x\n", aut, lat);
		} break;
	case 'puls':
		Pulse();
		break;
	}
}

void
JoyCalib::EnterMode(
	int mode)
{
	if (mode < 1) mode = 1;
	if (mode > 5) mode = 5;
	bool redraw_buttons = false;
	if ((mode & 4) != (m_mode & 4)) {
		redraw_buttons = true;
	}
	m_mode = mode;
	switch (mode) {
	case 1:
		SetInstructions("Center: center your joystick, then press the trigger.");
		*m_info = m_prev_info;	/* reset */
		m_stick.EnableCalibration(false);
		Pulse();
		memcpy(m_ax_min, m_values, sizeof(m_ax_min));
		memcpy(m_ax_max, m_values, sizeof(m_ax_max));
		break;
	case 2:
/*	We skip deadzone right now	*/
//		SetInstructions("Deadzone: let your hand rest on the joystick, then press the trigger.");
//		break;
		m_mode = 3;
	case 3:
		SetInstructions("Maximum: move your joystick to each side in each direction, then press the trigger.");
		for (int ix=0; ix<m_stick.CountAxes(); ix++) {
			m_info->axis_calib[ix].bottom = -32767;
			m_info->axis_calib[ix].start_dead = m_ax_min[ix];
			m_info->axis_calib[ix].end_dead = m_ax_max[ix];
			m_info->axis_calib[ix].top = 32767;
			m_info->axis_calib[ix].bottom_mul = 128;
			m_info->axis_calib[ix].top_mul = 128;
		}
		memcpy(m_ax_min, m_values, sizeof(m_ax_min));
		memcpy(m_ax_max, m_values, sizeof(m_ax_max));
		break;
	case 4:
		SetInstructions("Calibration is complete. Press Done to save results.");
		for (int ix=0; ix<m_stick.CountAxes(); ix++) {
			if ((m_ax_min[ix] > m_info->axis_calib[ix].start_dead-3300) || 
				(m_ax_max[ix] < m_info->axis_calib[ix].end_dead+3300)) {
				memset(&m_info->axis_calib[ix], 0, sizeof(m_info->axis_calib[ix]));
//				fprintf(stderr, "Axis %d didn't move much\n", ix);
			}
			else {
				m_info->axis_calib[ix].bottom = m_ax_min[ix];
				m_info->axis_calib[ix].top = m_ax_max[ix];
				m_info->axis_calib[ix].bottom_mul = (int)ceil(128.0*32767/(m_info->axis_calib[ix].start_dead-
					m_info->axis_calib[ix].bottom));
				m_info->axis_calib[ix].top_mul = (int)ceil(128.0*32767/(m_info->axis_calib[ix].top-
					m_info->axis_calib[ix].end_dead));
			}
		}
		m_stick.EnableCalibration(true);
		break;
	case 5:
		PostMessage(B_QUIT_REQUESTED);
		break;
	}
	dynamic_cast<BButton *>(FindView("next"))->SetEnabled(mode > 3);
	m_stick.EnableCalibration(mode > 3);
	if (redraw_buttons) {
		for (int ix=0; ix<m_calib_views.CountItems(); ix++) {
			CalibButton * bb = dynamic_cast<CalibButton *>((BView *)m_calib_views.ItemAt(ix));
			if (bb != NULL) bb->Draw(bb->Bounds());
		}
		Sync();
	}
}

int
JoyCalib::Mode()
{
	return m_mode;
}

void
JoyCalib::Pulse()
{
	int i = m_stick.Update();
	i = m_stick.GetAxisValues(m_values, 0);
	m_buttons = m_stick.ButtonValues();
	if (m_stick.CountHats() > 0)
		i = m_stick.GetHatValues(m_hats, 0);
	for (int ix=0; ix<m_calib_views.CountItems(); ix++) {
		BView * v = (BView *)m_calib_views.ItemAt(ix);
		CalibAxis * ca = dynamic_cast<CalibAxis *>(v);
		if (ca) {
			ca->DrawValue();
		}
		else {
			CalibButton * cb = dynamic_cast<CalibButton *>(v);
			if (cb) {
				cb->DrawValue();
			}
			else {
				CalibHat * ch = dynamic_cast<CalibHat *>(v);
				if (ch) {
					ch->DrawValue();
				}
			}
		}
	}
	Sync();
	if ((m_buttons & 1) && (m_last_click < system_time()-700000) 
		&& (Mode() < 4)) {
		m_last_click = system_time();
		PostMessage(NEXT);
	}
	else if (!(m_buttons & 1) && (m_last_click < system_time()-250000)) {
		m_last_click = 0;	/* reset so new button click will register quickly */
	}
	for (int ix=0; ix<m_stick.CountAxes(); ix++) {
		if (m_ax_min[ix] > m_values[ix]) {
			m_ax_min[ix] = m_values[ix];
		}
		if (m_ax_max[ix] < m_values[ix]) {
			m_ax_max[ix] = m_values[ix];
		}
	}
}

BJoystick & 
JoyCalib::Stick()
{
	return m_stick;
}

void 
JoyCalib::SetInstructions(
	const char * instructions)
{
	m_instructions->SetText(instructions);
}


