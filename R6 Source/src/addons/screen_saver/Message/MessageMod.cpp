//	SaverModule.cp
//	©1996 Jon Watte
// Ported to the BScreenSaver API Jan 1999 by Duncan Wilcox
// Brought out of the stone age Feb 2001 by Dan Sandler

#include <OS.h>
#include <Bitmap.h>
#include <StringView.h>
#include <Screen.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Box.h>

#include <Debug.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "MessageMod.h"

//	features

#define checkpoint(x)

//	constants

#define MINNMSG 8
#define	MAXNMSG 21
#define DEFAULT_SNOOZE_TIME 22000000

#define WHITESPACE(c) (c==' '||c=='\t'||c=='\n')

//	globals

int actr;

static char const * kAttributions[] = {
	//   |----------------------------------|
		"Jon Watte 1996 - original creator",
		"Duncan Wilcox 1999 - BScreenSaver",
		"Dan Sandler 2001 - feature bloat"
	};
static int const kNumAttributions = 3;
static int const kTopBilledAttribution = 0;


//	for debugging

#ifndef checkpoint
static void
checkpoint(
	const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	FILE *log = fopen("checkpoints", "r+");
	if (log == NULL)
		log = fopen("checkpoints", "w");
	if (log) {
		fprintf(log, "%s\n", msg);
		fflush(log);
		fclose(log);
	}
}
#endif

//	utility
static float
fl_rand(
	float base,
	float top)
{
	float r = (rand()&0x7fff)/32768.0*(top-base)+base;
	return r;
}


// ==== MessageSetupView class ====

class MessageSetupView :
	public BView
{
public:
								MessageSetupView(
									BRect area,
									const char *name,
									ulong resize,
									ulong flags,
									Message *_instance);

		void					MessageReceived(
									BMessage *message);
									
		virtual void 			AttachedToWindow();
private:
	BMessage 		*params;
	Message 		*instance;
	BCheckBox 		*m_cbSubtitle;
	BTextControl 	*m_tcCommand;
	BPopUpMenu 		*m_puFortunePopup;
};

MessageSetupView::MessageSetupView(
	BRect area,
	const char *name,
	ulong resize,
	ulong flags,
	Message *_instance
)
	: BView(area, name, resize, flags),
	  instance(_instance)
{
}

void MessageSetupView::AttachedToWindow() {
	BView *view = Parent();

	BRect parentarea(view->Bounds());
	BRect area = BRect(parentarea.left + 20, parentarea.top,
		parentarea.right - 20, parentarea.top + 18); // the area of one control


	// attribution (top billing)

	BStringView *attr;

	attr = new BStringView(area, "attribution headline", "Message, by:");
	AddChild(attr);

	area.OffsetBy(0,18);
	

	int a = (int)(((double)rand() / (double)RAND_MAX) * (double)kNumAttributions);
	attr = new BStringView(area.OffsetByCopy(10,0),
		"attribution", kAttributions[a]);
	AddChild(attr);
	
	area.OffsetBy(0,18);

	attr = new BStringView(area.OffsetByCopy(10,0), "attribution epilogue",
		B_UTF8_ELLIPSIS"and others");
	AddChild(attr);


	area.OffsetBy(0,40);

	// Use-subtitles checkbox
	m_cbSubtitle = new BCheckBox
		(area, "subtitles", "Subtitle the complete message", new BMessage('subt'));
	m_cbSubtitle->SetValue(instance->GetSubtitles());
	m_cbSubtitle->SetTarget(this);
	AddChild(m_cbSubtitle);

	area.OffsetBy(0,25);

	// Fortune command
	BStringView *text2 = new BStringView
		(area, "exec-hdr", "Display text comes from"B_UTF8_ELLIPSIS);
	AddChild(text2);

	area.OffsetBy(0,20);

	m_puFortunePopup = new BPopUpMenu("fortune-popup");
	
	Message::execStyle oldstyle = instance->GetExecStyle();
	
	BMessage *msg;
	BMenuItem *mi;
	{
		msg = new BMessage('exec');
		msg->AddInt32("exec_style", Message::execCommand);
		mi = new BMenuItem("a command like 'fortune':", msg);
		mi->SetMarked(oldstyle == Message::execCommand);
		m_puFortunePopup->AddItem(mi);
	
		// ---

		msg = new BMessage('exec');
		msg->AddInt32("exec_style", Message::execEcho);
		mi = new BMenuItem("a short message:", msg);
		mi->SetMarked(oldstyle == Message::execEcho);
		m_puFortunePopup->AddItem(mi);
	}
	
	m_puFortunePopup->SetTargetForItems(this);
	BMenuField *style_mf = new BMenuField(area, "style_mfield",
		NULL, m_puFortunePopup);
	// AddChild(style_mf);

	BBox *box = new BBox(BRect(area.left, area.top, area.right, area.top + 60),
		"argv-box");
	box->SetLabel(style_mf);

	{
		m_tcCommand = new BTextControl
			(BRect(10,30,area.right - 30, 52),
			"command-string", NULL, instance->GetMessageString(),
			new BMessage('argv'));
		m_tcCommand->SetModificationMessage(new BMessage('argv'));
		m_tcCommand->SetTarget(this);
		box->AddChild(m_tcCommand);
	}
	AddChild(box);

	area.OffsetBy(0,65);
	
	// "32-bit" footnote
	BRect footnote(area.OffsetToCopy(20, view->Bounds().bottom-20));
	BStringView *text = new BStringView(footnote, "32bit",
		"Looks best in 32-bit color");
	AddChild(text);
}

void
MessageSetupView::MessageReceived(
	BMessage *msg)
{
	int32 idata;
	char const * cdata;
	switch(msg->what) {
		case 'argv':
			PRINT(("[MessageSetupView::MessageReceived] caught 'argv'\n"));
			cdata = m_tcCommand->Text();
			if (instance)
				instance->SetMessageString(cdata);
			else
				PRINT(("[MessageSetupView::MessageReceived] error: no instance ptr\n"));
			break;
		case 'exec':
			PRINT(("[MessageSetupView::MessageReceived] caught 'exec'\n"));
			if (msg->FindInt32("exec_style", &idata) == B_OK) {
				if (instance) {
					instance->SetExecStyle((Message::execStyle)idata);
					// ** not implemented yet:
					// m_tcCommand->SetText(instance->GetMessageString());
				} else {
					PRINT(("[MessageSetupView::MessageReceived] error: no instance ptr\n"));
				}
			}
			break;
		case 'subt':
			PRINT(("[MessageSetupView::MessageReceived] caught 'subt'\n"));
			idata = m_cbSubtitle->Value();
			if (instance)
				instance->SetSubtitles((bool)idata);
			else
				PRINT(("[MessageSetupView::MessageReceived] error: no instance ptr\n"));
			break;
		default:
			BView::MessageReceived(msg);
			break;
	}
}

// ==== Message class ====

// ** functions used for the screensaver display (ported from original
//    screensaver implementation):
//    setup, teardown, get_message, random_font, step, draw

//	initialize stuff needed for animation
void
Message::setup(
	BView *view)
{
	actr = 0;
	srand(find_thread(NULL));

	BRect area = view->Bounds();

	BScreen *s = new BScreen();
	color_space cs = s->ColorSpace();
	if ((cs != B_RGB_32_BIT) && (cs != B_COLOR_8_BIT)) {
		checkpoint("bad color karma");
		cs = B_RGB_32_BIT;
	}
	m_bitmap = new BBitmap(area, cs, TRUE);	/* pretty large bitmap */
	m_bmview = new BView(area, "bmview", 0, 0);
	m_bitmap->AddChild(m_bmview);
}

//	terminate all animation

void
Message::teardown()
{
	checkpoint("teardown");
	if (m_message) { free(m_message); m_message = NULL; }
	if (m_bitmap) { delete m_bitmap; m_bitmap = NULL; }
//	printf("Message::teardown: this = %p\n", this);
	m_bmview = NULL;
}

//	scale a color
static rgb_color
scale_rgb(
	rgb_color in,
	float scaling)
{
	in.red = (uint8)(in.red * scaling);
	in.green = (uint8)(in.green * scaling);
	in.blue = (uint8)(in.blue * scaling);
	return in;
}

//	find a message
char *
Message::get_message()
{
	checkpoint("get_message");
	char *msg;
	if (m_execStyle == Message::execCommand) {
		BString cmd(m_messageString);
		cmd.Append(" > /tmp/fortune_msg");
		int exitcode = system(cmd.String());
		if (exitcode != 0) {
			return strdup("Message error: couldn't run fortune command.");
		}
		FILE *f = fopen("/tmp/fortune_msg", "r");
		if (f) {
			msg = (char *)malloc(sizeof(char) * 1024);
			memset(msg, 0, 1024);
			fread(msg, 1, 1023, f);
			fclose(f);
			remove("/tmp/fortune_msg");
		} else {
			return strdup("Message error: couldn't read output of fortune command.");
		}
		msg[1023] = 0;
		for (unsigned char *ptr = (unsigned char *)msg; *ptr; ptr++) {
			if (*ptr < 32) *ptr = 32;
		}
		return msg;
	} else if (m_execStyle == Message::execEcho) {
		msg = (char *)malloc(sizeof(char) * 1024);
		strcpy(msg, m_messageString.String());
		return msg;
	} else { // default!
		switch ((int)fl_rand(0, 5)) {
		case 0:
			return strdup("Gone for lunch. Back in three days!");
			break;
		case 1:
			return strdup("Gone fishing. Leave a message!");
			break;
		case 2:
			return strdup("Gone golfing. Cell phone is off!");
			break;
		case 3:
			return strdup("Back in a flash. However long that is!");
			break;
		default:
// BeBox certainly not, updating... DW
//			return "BeBox - not fit for consumption by normal human beings!";
			return strdup("BeOS - not fit for consumption by normal human beings!");
			break;
		}
	}
	return NULL;
}

//	set the given view to a random font

void
Message::random_font(
	BView *view)
{
	checkpoint("random_font");

	int32 numFamilies = count_font_families();

	int theFont;
	int theStyle;
	font_family family; 
	font_style style; 
	uint32 flags; 
	while((theFont = (int)fl_rand(0, numFamilies),
		get_font_family(theFont, &family, &flags) != B_OK) ||
		strstr(family, "ymbol") != 0 || (flags & B_IS_FIXED) != 0)
		;

	int32 numStyles = count_font_styles(family); 
	while(theStyle = (int)fl_rand(0, numStyles),
		get_font_style(family, theStyle, &style, &flags) != B_OK)
		;

	BFont	f;
	f.SetFamilyAndStyle(family, style);

	checkpoint(family);
	view->SetFont(&f);

}

//	step the animation

void
Message::step()
{
	checkpoint("step");
	rgb_color theme = { (uint8)fl_rand(128,256), (uint8)fl_rand(128,256), (uint8)fl_rand(128,256), 255 };
	m_message = get_message();
	checkpoint(m_message);
	rgb_color bglo = scale_rgb(theme, 0.1);
	rgb_color bghi = scale_rgb(theme, 0.2);
	pattern bgpat = {{ 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc }};

	m_bitmap->Lock();

	m_bmview->SetLowColor(bglo);
	m_bmview->SetHighColor(bghi);
	m_bmview->SetDrawingMode(B_OP_COPY);
	m_bmview->FillRect(m_bmview->Bounds(), bgpat);

	m_bmview->SetDrawingMode(B_OP_OVER);
	random_font(m_bmview);

	BRect bounds = m_bmview->Bounds();
	char excerpt[64];
	int len = strlen(m_message);
	if (len < 4) len = 4;	//	 so what?

	int NMSG = (int)fl_rand(MINNMSG, MAXNMSG);

	for (int ix=1; ix<=NMSG; ix++) {
		int pos = (int)fl_rand(0, len-3);
		int xlen = (int)fl_rand(1, len-pos);
		if (xlen > 63)
			xlen = 63;
		strncpy(excerpt, m_message+pos, xlen);
		excerpt[xlen] = 0;
		m_bmview->SetHighColor(scale_rgb(theme, ix/(NMSG*2.0)));
		m_bmview->SetFontSize(5.0*bounds.Height()/(10.0+xlen));
		float shr = fl_rand(0, 7);
		if (shr < 1) {
			BFont f;
			m_bmview->GetFont(&f);
			f.SetShear(fl_rand(45, 80));
			m_bmview->SetFont(&f);
		} else if (shr < 2) {
			BFont f;
			m_bmview->GetFont(&f);
			f.SetShear(fl_rand(100, 135));
			m_bmview->SetFont(&f);
		} else {
			BFont f;
			m_bmview->GetFont(&f);
			f.SetShear(90);
			m_bmview->SetFont(&f);
		}
		float width = m_bmview->StringWidth(excerpt);
		m_bmview->MovePenTo(fl_rand(-width/2.0, bounds.Width()-width/2.0),
			fl_rand(0, bounds.Height()));
		m_bmview->DrawString(excerpt);
	}
	
	// add subtitles
	if (m_useSubtitles) {
		rgb_color subc = { 170, 170, 170, 255 };
		rgb_color shadowc = { 0, 0, 0, 96 };
		BFont subf(be_fixed_font); // start with user's fixed-font choice
		subf.SetSpacing(B_FIXED_SPACING); // assume monospace
		// subf.SetSize(13.0);

		char *lines[32];
		int line_i = 0;
		lines[line_i] = new char[1024];
		char *from = m_message;
		char *to = lines[line_i];
		int column = 0;
		float maxwidth = 0.0;
		float curwidth = 0.0;
		while(*from != '\0' && line_i < 31) {
			if (*from == '\n' || (column >= 80 && WHITESPACE(*from))) {
				if (*from != '\n') {
					*to = *from;
					to++;
				}
				*to = '\0';
				if ((curwidth = subf.StringWidth(lines[line_i])) > maxwidth)
					maxwidth = curwidth;
				line_i++;
				lines[line_i] = new char[1024];
				column = 0;
				to = lines[line_i];
			} else {
				*to = *from;
				to++;
				column++;
			}
			from++;
		}
		*to = '\0'; // clean up the last line
		if ((curwidth = subf.StringWidth(lines[line_i])) > maxwidth)
			maxwidth = curwidth;
		
		font_height height;
		subf.GetHeight(&height);
		float lineheight = height.leading + height.ascent + height.descent;
		float leftmargin = (bounds.Width() - maxwidth) * 0.5; // fix later
		float texttop = (bounds.Height()*0.85) - (lineheight * line_i * 0.5);

		m_bmview->SetDrawingMode(B_OP_ALPHA);
		m_bmview->SetFont(&subf);

		for(int i=0; i<=line_i; i++) {
			m_bmview->SetHighColor(shadowc);
			m_bmview->MovePenTo(leftmargin + 1, texttop + lineheight * i + 1);
			m_bmview->DrawString(lines[i]);

			m_bmview->SetHighColor(subc);
			m_bmview->MovePenTo(leftmargin, texttop + lineheight * i);
			m_bmview->DrawString(lines[i]);

			PRINT(("[MessageMod::step] deleting line array #%d : %p\n", i, lines[i]));
			delete [] lines[i];
		}

		#if DEBUG
			rgb_color debugc = { (uint8)102, (uint8)102, (uint8)102, 255 };
			m_bmview->SetHighColor(debugc);
			BString debuginfo;
			debuginfo << "leftmargin: " << leftmargin
				<< ", texttop: " << texttop
				<< ", maxwidth: " << maxwidth
				<< ", lineheight: " << lineheight
				<< ", line_i: " << line_i;
			m_bmview->MovePenTo(leftmargin, texttop + lineheight * line_i + 20);
			m_bmview->DrawString(debuginfo.String());
		#endif
	}
	
	m_bmview->Sync();

	m_bitmap->Unlock();
}

//	draw the buffer

void
Message::draw(
	BView *view)
{
	checkpoint("draw");
	if (view->Window()->LockingThread() != find_thread(0)) {
		checkpoint("Strange lock owner!\n");
	}
	view->DrawBitmap(m_bitmap, BPoint(B_ORIGIN));
}


// ** methods exposed as part of the BScreenSaver API:
//    ctor, dtor, StartConfig, StartSaver, StopSaver, SaveState, Draw

Message::Message(BMessage *message, image_id image)
 : BScreenSaver(message, image),

   m_bmview(NULL),
   m_bitmap(NULL),
   m_message(NULL),
   m_useSubtitles(false), 					// user default: no subtitles
   m_messageString("fortune"),				// user default: use "fortune"
   m_execStyle(Message::execCommand),
   m_sleep_msec(DEFAULT_SNOOZE_TIME)
{
	int32 idata;
	if (message->FindInt32("use_subtitles", &idata) == B_OK)
		m_useSubtitles = (1==idata);
	if (message->FindInt32("exec_style", &idata) == B_OK)
		m_execStyle = (Message::execStyle)idata;
	if (message->HasString("fortune_cmd")) {
		const char *data = message->FindString("fortune_cmd");
		m_messageString = data; // copy
	}
}

void Message::StartConfig(BView *view)
{
	BView *mView = new MessageSetupView(view->Bounds(), "mView",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW,
		this);
	mView->SetViewColor(view->ViewColor());
	view->AddChild(mView);

}

status_t Message::StartSaver(BView *, bool /* preview */)
{
	SetTickSize(1000000);
	return B_OK;
}

void Message::StopSaver()
{
	teardown();
}

status_t Message::SaveState(BMessage *state) const {
	PRINT(("[Message::SaveState] storing preferences\n"));
	state->AddInt32("use_subtitles", m_useSubtitles?1:0);
	state->AddInt32("exec_style", (int32)m_execStyle);
	state->AddString("fortune_cmd", m_messageString);
	// ** not implemented yet:
	// save the contents of m_lastStrings[]
	return B_OK;
}

void Message::Draw(BView *view, int32 frame)
{
	if(frame == 0)
	{
		setup(view);
		view->FillRect(view->Bounds(), B_SOLID_LOW);	// Erase
		SetTickSize(m_sleep_msec);
	}

	checkpoint("animate");
	step();
	draw(view);
}

// ** new methods built to interact with MessageSetupView:
//    Set/GetSubtitles, Set/GetMessageString

void Message::SetSubtitles(bool on)
{
	PRINT(("[Message::SetSubtitles] will now %s subtitles\n",
		(on?"show":"not show")));
	m_useSubtitles = on;
}

bool 
Message::GetSubtitles()
{
	return m_useSubtitles;
}

void Message::SetMessageString(char const * const newcmd) {
	m_messageString = newcmd;
}

char const * const
Message::GetMessageString()
{
	return m_messageString.String();
}

void Message::SetExecStyle(Message::execStyle s)
{
	// ** not implemented yet:
	//	if (m_execStyle != s) {
	//		m_lastStrings[(int)m_execStyle] = f_messageString;
	//	}
	m_execStyle = s;
	//	m_messageString = m_lastStrings[(int)s];
}

Message::execStyle Message::GetExecStyle()
{
	return m_execStyle;
}


// MAIN INSTANTIATION FUNCTION - ScreenSaver add-on factory
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Message(message, image);
}

