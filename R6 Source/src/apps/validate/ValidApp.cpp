#include <errno.h>
#include <fs_attr.h>
#include <net/if.h> 
#include <net/if_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <time.h>

#include <algorithm>
#include <ctype.h>
#include <image.h>
#include <vector>

#include <Alert.h>
#include <Autolock.h>
#include <Binder.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <Locker.h>
#include <Path.h>
#include <RadioButton.h>
#include <Screen.h>
#include <ScrollView.h>
#include <SoundPlayer.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>
#include <Roster.h>

#include "settings.h"
#include "Test.h"
#include "ValidateInterface.h"

#include "ValidApp.h"


#define RUN_ALL_TESTS 1

FILE * m_results;
BLocker StatusWindow::s_lock;
StatusWindow * StatusWindow::s_window;
int g_fails;
int g_phase;

#if MAKE_HORRID_BEEP_ON_FAILURE
static void
play_func(
	void *,
	void * buf,
	size_t size,
	const media_raw_audio_format & /* fmt */)
{
	int cnt = size/4;
	short * s = (short *)buf;
	int lim = 70 - g_fails * 3;
	while (cnt > 0)
	{
		if (g_phase < 0)
		{
			*s++ = -30000;
			*s++ = -20000;
		}
		else
		{
			*s++ = 20000;
			*s++ = 30000;
		}
		g_phase++;
		if (g_phase >= lim) g_phase = -lim;
		cnt--;
	}
}
#endif

static void
make_noise()
{
#if MAKE_HORRID_BEEP_ON_FAILURE
	media_raw_audio_format fmt;
	fmt.frame_rate = 44100;
	fmt.byte_order = B_MEDIA_HOST_ENDIAN;
	fmt.format = 0x2;
	fmt.buffer_size = 4096;
	fmt.channel_count = 2;

	BSoundPlayer bsp(&fmt, "make noise", play_func, 0, 0);
	bsp.SetHasData(true);
	bsp.Start();
	snooze(400000);
	bsp.Stop();
	snooze(100000);
#endif
}


void reset_failure()
{
	StatusWindow::AddText("RESET FAILURE (probably for retry on some test.)\n", 0);
	g_failed = false;
	g_fails--;
}

void fail(const char * fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	vfprintf(m_results ? m_results : stderr, fmt, list);
	StatusWindow::AddText("FAILURE: ", 0);
	StatusWindow::AddText(fmt, list);
	va_end(list);
	g_failed = true;
	g_fails++;
	make_noise();
}

void attempt(const char * fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	vfprintf(m_results ? m_results : stdout, fmt, list);
	StatusWindow::AddText(fmt, list);
	va_end(list);
}


void info(const char * fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	vfprintf(m_results ? m_results : stderr, fmt, list);
	StatusWindow::AddText(fmt, list);
	va_end(list);
}


// The log may contain some header/comment lines, these are marked with the kLogCommentChar
const char kLogCommentChar = '#';

Test * make_keyboard_key();
Test * make_mouse_click();
Test * make_external_sound_confirmation();
Test * make_sound_confirmation();
Test * make_modem();
Test * make_ethernet();
Test * make_file_read();
Test * make_file_write();
Test * make_acceptance();

#if 0
Test * make_vendor1();
Test * make_vendor2();
Test * make_vendor3();
#endif
Test * make_alternate_test(const char * name, BView* (*func)(const BRect &, ValidateInterface *));
Test * make_script_test(const char * script_name);

stage_t avail_stages[] = {
	/* Interactive tests first */
//	{ 5, "Vendor Initial Test", "vendor1", make_vendor1 },
	{ 10, "Keyboard Key Test", "keyboard", make_keyboard_key, NULL, 0, 0 },
	{ 20, "Mouse Click Test", "mouse", make_mouse_click, NULL, 0, 0 },
	{ 29, "External Speaker Test", "external_sound", make_external_sound_confirmation, NULL, 0, 0 },
	{ 30, "Internal Speaker Test", "sound", make_sound_confirmation, NULL, 0, 0 },
//	{ 35, "Vendor Second-stage Test", "vendor2", make_vendor2 },
	{ 40, "Modem Connection Test", "modem", make_modem, NULL, 0, 0 },
	{ 50, "Ethernet Connection Test", "ethernet", make_ethernet, NULL, 0, 0 },
	/* Sanity checks second */
	{ 1001, "File Read Test", "fileread", make_file_read, NULL, 0, 0 },
	{ 1002, "File Write Test", "filewrite", make_file_write, NULL, 0, 0 },
//	{ 9000, "Vendor Third-stage Test", "vendor3", make_vendor3 },
	/* Burn-in tests last */
	{ 9999, "Acceptance", "acceptance", make_acceptance, NULL, -1, 0 }
};

vector<pair<stage_t *, int> > s_stageArrays;
vector<pair<char *, image_id> > s_addons;
vector<stage_t *> s_stages;

static stage_t *
find_stage(
	const char * buf,
	int skip = 0)
{
	int wildcard = -1;
	const char * e = buf+strlen(buf);
	if (e > buf && e[-1] == '%')
	{
		wildcard = e-buf-1;
	}
	else if (skip > 0)
	{
		return NULL;
	}
	for (vector<pair<stage_t *, int> >::iterator ptr(s_stageArrays.begin());
		ptr != s_stageArrays.end();
		ptr++)
	{
		stage_t * stages = (*ptr).first;
		int size = (*ptr).second;
		for (int ix=0; ix<size; ix++)
		{
			if ((wildcard < 0) && !strcasecmp(stages[ix].short_name, buf))
			{
				return &stages[ix];
			}
			if ((wildcard >= 0) && !strncasecmp(stages[ix].short_name, buf, wildcard))
			{
				if (skip-- == 0)
				{
fprintf(stderr, "wildcard %s matches %s\n", buf, stages[ix].short_name);
					return &stages[ix];
				}
			}
		}
	}
	return NULL;
}

static void
setup_stages(ValidApp& theApp)
{
	s_stageArrays.push_back(pair<stage_t *, int>(avail_stages, sizeof(avail_stages)/sizeof(stage_t)));

	BString addon_directory(ValidApp::s_current_directory);
	addon_directory += "/add-ons";
	DIR * d = opendir(addon_directory.String());
	if (d != NULL)
	{
		struct dirent * dent;
		while ((dent = readdir(d)) != NULL)
		{
			if (dent->d_name[0] == '.') continue;
			image_id id = load_add_on(dent->d_name);
			if (id > 0)
			{
				fprintf(stderr, "loaded add-on %s: %ld\n", dent->d_name, id);
				stage_t * stages = 0;
				if (!get_image_symbol(id, "avail_stages", B_SYMBOL_TYPE_TEXT, (void **)&stages))
				{
					//	this is almost a leak, but this code only gets run once
					s_addons.push_back(pair<char *, image_id>(strdup(dent->d_name), id));
					stage_t * tmp = stages;
					while (tmp->name != NULL)
					{
						tmp++;
					}
					s_stageArrays.push_back(pair<stage_t *, int>(stages, tmp-stages));
				}
				else
				{
					fprintf(stderr, "%s didn't have avail_stages\n", dent->d_name);
					unload_add_on(id);
				}
				
				BMessage* (*vendor_function)();
				// now see if there is any vendor specific information that needs to be displayed
				if (get_image_symbol(id, "GetVendorVersionInfo", B_SYMBOL_TYPE_TEXT, (void **)&vendor_function) == B_OK)
				{
					theApp.AdoptVendorInformation((*vendor_function)());
				}
			}
			else
			{
				fprintf(stderr, "cannot load add-on %s: %s\n", dent->d_name, strerror(id));
			}
		}
		closedir(d);
	}
	const char * order = get_setting("testorder");
	if (!order)	//	run all tests
	{
		info("WARNING: no testorder specified; running these tests:\n");
		for (vector<pair<stage_t*, int> >::iterator ptr(s_stageArrays.begin());
			ptr != s_stageArrays.end();
			ptr++)
		{
			stage_t * stages = (*ptr).first;
			for (int ix=0; ix<(*ptr).second; ix++)
			{
				if (stages[ix].next_stage > -1)
				{
					info("%s\n", stages[ix].short_name);
					s_stages.push_back(&stages[ix]);
				}
			}
		}
	}
	while (order)
	{
		while (*order && isspace(*order))
		{
			order++;
		}
		//	extract semi-colon separated list of entries
		const char * end = strchr(order, ';');
		if (end == NULL)
		{
			end = order+strlen(order);
		}
		if (end == order)
		{
			if (!*end) break;
			order = end+1;
			continue;
		}
		char buf[200];
		strncpy(buf, order, end-order);
		buf[end-order] = 0;
		order = (*end ? end+1 : NULL);

		stage_t * s;
		int cnt = 0;
		while ((s = find_stage(buf, cnt++)) != NULL)
		{
			s_stages.push_back(s);
			fprintf(stderr, "Found %s at 0x%08lx\n", buf, (uint32)s);
		}
		if (!s && cnt == 1)
		{
			// see if this is a name for a vendor supplied script
			char script_name[256];
			if (get_setting(buf, script_name, 256) != NULL) {
				info("testorder includes: %s - including %s as a vendor script\n", buf, script_name);
				// this script_stage is never deleted, but it is needed throughout the
				// entire application anyway
	
				stage_t* script_stage = new stage_t;
				script_stage->name = strdup(buf);
				script_stage->short_name = strdup(script_name);
				script_stage->func = NULL;
				script_stage->make_view_func = NULL;
				script_stage->next_stage = 0;
				script_stage->option = kUnknown;			
				s_stages.push_back(script_stage);
			}
			else {
				info("testorder includes: %s - test not found\n", buf);
			}
		}
	}
fprintf(stderr, "testorder parsed\n");

	//	we must have acceptance in the test run
	if (s_stages.size() && !strcasecmp(s_stages.back()->short_name, "acceptance"))
	{
		goto have_acceptance;
	}
	//	add acceptance last
	s_stages.push_back(find_stage("acceptance"));
have_acceptance:
	;
fprintf(stderr, "acceptance is now the last item\n");
}


static void read_previous()
{
	// the previous state always lives on the machine in original test directory
	BString previous_file(ValidApp::s_test_directory);
	previous_file += "/previous-state.ini";
	FILE * f = fopen(previous_file.String(), "r");
	if (f)
	{
		char line[1024];
		while (1)
		{
			line[0] = 0;
			fgets(line, 1023, f);
			if (line[0] == 0) break;
			if (line[0] == kLogCommentChar) continue;
			int stage, option;
			if (sscanf(line, "%d %d", &stage, &option) != 2)
			{
				fprintf(stderr, "bad state file line: %s", line);
			}
			else
			{
				if (stage >= (int)s_stages.size())
				{
					info("bad previous-state file! (delete it when editing validate.ini)\n");
					break;
				}
				s_stages[stage]->option = option;
			}
		}
		fclose(f);
	}
}

static void write_previous()
{
	BString previous_file(ValidApp::s_test_directory);
	previous_file += "/previous-state.ini";
	FILE * f = fopen(previous_file.String(), "w");
	if (f)
	{
		// write out the header information...
		fprintf(f, "%c %s", kLogCommentChar, ValidApp::s_start_time);
	
		fprintf(f, "%c Kernel: %s version %lx (%s %s)\n",
				kLogCommentChar,
				ValidApp::s_system_data.kernel_name,
				(uint32)ValidApp::s_system_data.kernel_version, 
				ValidApp::s_system_data.kernel_build_date, 
				ValidApp::s_system_data.kernel_build_time);
			
		fprintf(f, "%c OS: %s - %s (%s)\n", 
				kLogCommentChar, 
				ValidApp::s_os_info.name.String(), 
				ValidApp::s_os_info.version.String(), 
				ValidApp::s_os_info.date.String());

		fprintf(f, "%c Machine ID: %s\n", kLogCommentChar, ValidApp::s_machine_id.String());

		fprintf(f, "%c BIOS: %s - %s (%s)\n", 
				kLogCommentChar,
				ValidApp::s_bios_version.String(), 
				ValidApp::s_bios_vendor.String(), 
				ValidApp::s_bios_date.String());

		fprintf(f, "%c MAC Address: %s\n",
				kLogCommentChar,
				ValidApp::s_mac_address.String());

		int32 mem_size = B_PAGE_SIZE*ValidApp::s_system_data.max_pages;
		fprintf(f, "%c Memory: %08lx (%ldMB)\n", kLogCommentChar, mem_size, mem_size/(1024*1024));
		
		// now write out the stage information
		for (uint32 ix=0; ix<s_stages.size(); ix++)
		{
			const char * str[] = {
				"run", "pass", "fail", "ignore",
			};
			fprintf(f, "%ld %d\t# %s %s\n", ix, s_stages[ix]->option, str[s_stages[ix]->option], s_stages[ix]->name);
		}
		fclose(f);
	}
}

const rgb_color kGray = { 216, 216, 216, 255 };
const rgb_color kBlueGray = { 180, 180, 216, 255 };

static void
SetGray(BView* inView)
{
	inView->SetViewColor(kGray);
	inView->SetLowColor(kGray);
}

static void
SetBlueGray(BView* inView)
{
	inView->SetViewColor(kBlueGray);
	inView->SetLowColor(kBlueGray);
}

const float kButtonWidth = 90.;
const float kButtonHeight = 30.;
const float kStageTitleWidth = 320.;
const float kStageTitleHeight = 24.;
const float kStageItemHeight = 26.;
const float kOptionWidth = 60.;
const float kOptionHeight = 24.;

const float kWindowWidth = 610.;
const float kWindowHeight = 480.;

const BRect kWindowRect(10., 10., 0.+kWindowWidth, 0.+kWindowHeight);

class StagesWindow : public BWindow
{
public:
	StagesWindow(bool firstTime, BMessage* vendorInfo) : BWindow(kWindowRect, "Run Tests", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE)
	{
		
		// The window needs to be expanded by the (variable) number 
		// of vendor specific items that we have
		type_code dummy;
		int32 vendorInfoCount = 0;
		vendorInfo->GetInfo("info", &dummy, &vendorInfoCount);
		this->ResizeBy(0., vendorInfoCount*kStageTitleHeight);

		m_marked_unknown = 0;
		m_marked_failed = 0;
		
		BView* parent = new BView(this->Bounds(), "parentView", B_FOLLOW_ALL, B_WILL_DRAW);
		AddChild(parent);
		SetGray(parent);
		
		BRect test_frame(8., 15., kWindowWidth-8., 15.+(kStageItemHeight*2)+(s_stages.size())*kStageItemHeight);
		BBox* testBox = new BBox(test_frame, "testinfo");
		parent->AddChild(testBox);
		testBox->SetFontSize(18.);
		testBox->SetLabel("Test Selection");		

		read_previous();
		for (uint32 ix=0; ix<s_stages.size(); ix++)
		{
			const char * str = get_setting(s_stages[ix]->short_name);
			// once the test has been run (and they choose to run a test previously marked ignore) 
			// then we show the last result
			if (str != 0 && s_stages[ix]->option == kUnknown)
			{
				if (!strcasecmp(str, "ignore") || !strcasecmp(str, "no") ||
					!strcasecmp(str, "0") || !strcasecmp(str, "pass"))
				{
					s_stages[ix]->option = kIgnore;
				}
				else if (strcasecmp(str, "run") && strcasecmp(str, "yes") &&
					strcasecmp(str, "1"))
				{
					fprintf(stderr, "bad settings value for test '%s': %s\n",
						s_stages[ix]->short_name, str);
				}
				else
				{
					s_stages[ix]->option = kUnknown;
				}
			}
		}
		BStringView* sv;
		float top = kStageItemHeight;
		float left = 10.;
		BRect headingRect(left, top, left+kOptionWidth-1., top+kOptionHeight);
		sv = new BStringView(headingRect, "", "Test");
		testBox->AddChild(sv);
		sv->SetFontSize(18.);
		headingRect.OffsetBy(kStageTitleWidth, 0.);
		
		sv = new BStringView(headingRect, "", "Run");
		testBox->AddChild(sv);
		sv->SetFontSize(18.);
		headingRect.OffsetBy(kOptionWidth, 0.);
		
		sv = new BStringView(headingRect, "", "Pass");
		testBox->AddChild(sv);
		sv->SetFontSize(18.);
		headingRect.OffsetBy(kOptionWidth, 0.);
		
		sv = new BStringView(headingRect, "", "Fail");
		testBox->AddChild(sv);
		sv->SetFontSize(18.);
		headingRect.OffsetBy(kOptionWidth, 0.);

		sv = new BStringView(headingRect, "", "Ignore");
		testBox->AddChild(sv);
		sv->SetFontSize(18.);

		top += kStageItemHeight*1.25;
		bool stripe = false;	
		for (uint32 ix=0; ix<s_stages.size(); ix++)
		{
			if (s_stages[ix]->next_stage != -1)
			{
				BRect titleRect(left, top, left+kStageTitleWidth-1., top+kStageTitleHeight);
				sv = new BStringView(titleRect, "", s_stages[ix]->name);
				testBox->AddChild(sv);
				sv->SetFontSize(18.);
				BMessage msg('foo ');
				msg.AddInt32("stage", ix);
				msg.AddInt32("option", kUnknown);
				
				BView * v = new BView(BRect(left+kStageTitleWidth, top, left+kStageTitleWidth+(kOptionWidth*4), top+kStageTitleHeight), "", B_FOLLOW_NONE, B_WILL_DRAW);
				testBox->AddChild(v);
				
				stripe = !stripe;
				if( stripe )
				{
					SetBlueGray(v);
					SetBlueGray(sv);
				}
				else
				{
					SetGray(v);
					SetGray(sv);
				}
				
				top += kStageItemHeight;
				BRect buttonRect(0., 0., kOptionWidth-1., kOptionHeight);
				BRadioButton * rb = new BRadioButton(buttonRect, "", "", new BMessage(msg));
				if (s_stages[ix]->option == kUnknown) {
					// if any stages are marked run, then we don't know the valid state
					rb->SetValue(1);
					m_marked_unknown += 1;
				}
				v->AddChild(rb);
				rb->SetFontSize(18.);
				buttonRect.OffsetBy(kOptionWidth, 0.);

				msg.ReplaceInt32("option", kPass);				
				rb = new BRadioButton(buttonRect, "", "", new BMessage(msg));
				if (s_stages[ix]->option == kPass) {
					rb->SetValue(1);
				}
				v->AddChild(rb);
				rb->SetFontSize(18.);
				buttonRect.OffsetBy(kOptionWidth, 0.);
				
				msg.ReplaceInt32("option", kFail);
				rb = new BRadioButton(buttonRect, "", "", new BMessage(msg));
				if (s_stages[ix]->option == kFail) {
					// if any stages are marked fail, then our current state is failed
					rb->SetValue(1);
					m_marked_failed += 1;
				}
				v->AddChild(rb);
				rb->SetFontSize(18.);
				buttonRect.OffsetBy(kOptionWidth, 0.);

				msg.ReplaceInt32("option", kIgnore);
				rb = new BRadioButton(buttonRect, "", "", new BMessage(msg));
				if (s_stages[ix]->option == kIgnore) {
					rb->SetValue(1);
				}
				v->AddChild(rb);
				rb->SetFontSize(18.);
			}
		}
		
		top = test_frame.bottom + 15;
		float versionWidth=500.;
		BRect box_frame(8., top, kWindowWidth-8., top+(kStageTitleHeight*(7+vendorInfoCount)));
		BBox* versionBox = new BBox(box_frame, "versioninfo");
		parent->AddChild(versionBox);
		versionBox->SetFontSize(18.);
		versionBox->SetLabel("Version Information");		
		float inbox_top = kStageTitleHeight;
		BString beia_info("OS: ");
		beia_info += ValidApp::s_os_info.name;
		beia_info += " - ";
		beia_info += ValidApp::s_os_info.version;
		beia_info += " (";
		beia_info += ValidApp::s_os_info.date;
		beia_info += ")";
		BStringView* infoView = new BStringView(BRect(10.,inbox_top,10.+versionWidth,inbox_top+kStageTitleHeight), "", beia_info.String());
		versionBox->AddChild(infoView);
		infoView->SetFontSize(18.);
		inbox_top += kStageTitleHeight;
		BString machine("Machine ID: ");
		machine += ValidApp::s_machine_id;
		
		infoView = new BStringView(BRect(10.,inbox_top,10.+versionWidth,inbox_top+kStageTitleHeight), "", machine.String());
		versionBox->AddChild(infoView);
		infoView->SetFontSize(18.);
		
		inbox_top += kStageTitleHeight;
		BString bios_info("BIOS: ");
		bios_info += ValidApp::s_bios_version;
		bios_info += " - ";
		bios_info += ValidApp::s_bios_vendor;
		bios_info += " (";
		bios_info += ValidApp::s_bios_date;
		bios_info += ")";
		infoView = new BStringView(BRect(10.,inbox_top,10.+versionWidth,inbox_top+kStageTitleHeight), "", bios_info.String());
		versionBox->AddChild(infoView);
		infoView->SetFontSize(18.);

		inbox_top += kStageTitleHeight;
		BString mac_address("MAC Address: ");
		mac_address += ValidApp::s_mac_address;
		infoView = new BStringView(BRect(10.,inbox_top,10.+versionWidth,inbox_top+kStageTitleHeight), "", mac_address.String());
		versionBox->AddChild(infoView);
		infoView->SetFontSize(18.);
		
		for (int i = 0; i < vendorInfoCount; i++) {
			const char* aVendorString = vendorInfo->FindString("info", i);
			inbox_top += kStageTitleHeight;
			infoView = new BStringView(BRect(10.,inbox_top,10.+versionWidth,inbox_top+kStageTitleHeight), "", aVendorString);
			versionBox->AddChild(infoView);
			infoView->SetFontSize(18.);
		}
		
		inbox_top += kStageTitleHeight+12.;
		BString test_info("(Test Location: ");
		test_info += ValidApp::s_current_directory;
		test_info += " - settings = ";
		test_info += get_settings_file();
		test_info += ")";
		infoView = new BStringView(BRect(10.,inbox_top, versionBox->StringWidth(test_info.String()),
		                           inbox_top+kStageTitleHeight), "", test_info.String());
		versionBox->AddChild(infoView);
		infoView->SetFontSize(18.);


		// Conditional UI adjustment for device screen size.
		bool windowedInfo = false;
		
		// Center the window horizontally
		BRect screen(BScreen().Frame());
		MoveTo((screen.Width() / 2) - (Bounds().Width() / 2), 25.);
		
		if( (screen.bottom < box_frame.bottom + kButtonHeight + 10.0) && (NULL == m_version_window) ) 
		{
			versionBox->ResizeBy(1., 1.);
			m_version_window = new BWindow(versionBox->Frame(), versionBox->Label(), B_TITLED_WINDOW,
										 B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE);
			
			versionBox->SetLabel(""); 							 
			parent->RemoveChild(versionBox);
			versionBox->MoveTo(1., 1.);
			versionBox->ResizeBy(-1., -1.);
			m_version_window->MoveTo(Frame().LeftTop());
			m_version_window->AddChild(versionBox);
			m_version_window->Hide();
			m_version_window->Show();
			box_frame.bottom = test_frame.bottom;
			windowedInfo = true;
		}


		top = box_frame.bottom + kStageTitleHeight;
		BRect bRect(25., top, 25.+kButtonWidth, top+kButtonHeight);
		BButton* testButton = new BButton(bRect, "", "Test", new BMessage('test'));
		parent->AddChild(testButton);
		testButton->SetFontSize(18.);

		bRect.OffsetBy(kButtonWidth+(kButtonWidth/2), 0.);
		m_prepare_button = new BButton(bRect, "", "Prepare", new BMessage('prep'));
		parent->AddChild(m_prepare_button);
		m_prepare_button->SetFontSize(18.);
		m_prepare_button->SetEnabled(m_marked_failed + m_marked_unknown == 0);

		if( windowedInfo )
		{
			bRect.OffsetBy(kButtonWidth+(kButtonWidth/2), 0.);
			BButton* versionButton = new BButton(bRect, "", "Version", new BMessage('vnfo'));
			parent->AddChild(versionButton);
			versionButton->SetFontSize(18.);
		}
		
		bRect.OffsetTo(kWindowWidth-kButtonWidth-25., top);
		BButton* quitButton = new BButton(bRect, "", "Quit", new BMessage('quit'));
		parent->AddChild(quitButton);
		quitButton->SetFontSize(18.);

		// set up default button
		if (firstTime) {
			testButton->MakeDefault(true);
		}
		else if (m_marked_failed + m_marked_unknown == 0) {
			m_prepare_button->MakeDefault(true);
			// if everything is ready to go, check auto_prepare
			// and just do go on to prepare if enabled
			if (get_setting_value("validate.autoprepare", 0)) {
				this->PostMessage('prep');
			}
		}
		else {
			quitButton->MakeDefault(true);
		}
		
		top += kButtonHeight + 10.;
		
		// Make sure we fit everything in the window...
		ResizeTo(kWindowWidth, top);
	}
		
	void MessageReceived(BMessage * msg)
	{
		switch (msg->what) {
			case 'foo ':
			{
				int32 stage = 0;
				int32 option = 0;
				msg->FindInt32("stage", &stage);
				msg->FindInt32("option", &option);
				int old_option = s_stages[stage]->option;
				s_stages[stage]->option = option;
				// keep track of how many are failed or unkown
				switch (option) {
					case kFail:
						m_marked_failed += 1;
						break;
					case kUnknown:
						m_marked_unknown += 1;
						break;
					default:
						break;
				}
				switch (old_option) {
					case kFail:
						m_marked_failed -= 1;
						break;
					case kUnknown:
						m_marked_unknown -= 1;
						break;
					default:
						break;
				}
				
				m_prepare_button->SetEnabled(m_marked_failed + m_marked_unknown == 0);
				break;
			}
			case 'test':
			{
				// save the current state before any tests are run
				// and always run the acceptance test - so mark that kUnknown
				// and start with a clean failure state
				write_previous();
				find_stage("acceptance")->option = kUnknown;
				g_failed = false;
				BMessage msg(ENTER_STAGE);
				msg.AddInt32("stage", 0);
				be_app->PostMessage(&msg);
				this->PostMessage(B_QUIT_REQUESTED);
				break;
			}	
			case 'quit':
				g_return_code = 1;
				be_app->PostMessage(B_QUIT_REQUESTED);
				break;
	
			case 'prep':
				// the only way the user can get here is if all the tests are marked pass
				// believe them...
				g_failed = false;
				g_return_code = 0;
				be_app->PostMessage(B_QUIT_REQUESTED);
				break;
			
			case 'vnfo':
				if( m_version_window->Lock() )
				{
					if( true == m_version_window->IsHidden() )
					{
						m_version_window->Show();
					}
					
					if( true == m_version_window->IsMinimized() )
					{
						m_version_window->Minimize(false);
					}
					
					if( false == m_version_window->IsActive() )
					{
						m_version_window->Activate(true);
					}
					
					m_version_window->Unlock();
				}
				break;
			
			default:	
				BWindow::MessageReceived(msg);
				break;
		}
	}

private:
	BButton*	m_prepare_button;
	static BWindow*	m_version_window;
	int32		m_marked_unknown;
	int32		m_marked_failed;
	bool		m_quit_application;
};

BWindow* StagesWindow::m_version_window = NULL;

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ether_init loader
class MacInit
{
	public:
		MacInit();
		~MacInit();
	
	private:
		void SetStringPropertyWrapper(const char* NextPropertyName,
		                              BString& Value);
		                              
		void SetProfile(BString& ProfileName, bool UpInterface = false);
	
	private:
		BString fPreviousProfile;
		BinderNode::property fControlNode;
};

// ------------------------------------------------------------------------------------
inline MacInit::MacInit()
{
	// Save old profile.
	fControlNode = BinderNode::Root() / "service" / "network"
	                                  / "control";
	
	fPreviousProfile = (fControlNode / "status" / "profile");
	BString ether("ether_init");
	SetProfile(ether, true);
}
// ------------------------------------------------------------------------------------
inline MacInit::~MacInit()
{
	SetProfile(fPreviousProfile);
}
// ------------------------------------------------------------------------------------
inline void MacInit::SetStringPropertyWrapper(const char* PropertyName,
                                              BString& Value)
{
	BinderNode::property returnVal;
	BinderNode::property arg(Value);
	BinderNode::property_list argList;
	argList.AddItem(&arg);
	fControlNode->GetProperty(PropertyName, returnVal, argList);
}
// ------------------------------------------------------------------------------------
inline void MacInit::SetProfile(BString& ProfileName, bool UpInterface)
{
	fControlNode["down"]();
	SetStringPropertyWrapper("adopt", ProfileName);
	
	if( true == UpInterface )
	{
		fControlNode["up"]();
	}
}
// ------------------------------------------------------------------------------------

static void get_mac_address(BString& address)
{
	// Get the MAC address of this machine (code taken from ifconfig.c)
	// ...bone specific...

	// Stack based setter and resetter of config to initialize MAC address
	MacInit mi;

	// default result for all error cases
	address = "<unavailable>";
	
	struct ifconf ifc;
	struct ifreq ifr;
	
	memset(&ifc, 0, sizeof(ifc));
	memset(&ifr, 0, sizeof(ifr));
	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_req = &ifr;

	int sock = socket(AF_LINK, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf(stderr, "error opening socket = %s (%x)\n", strerror(errno), errno);
		return;
	}

	ifc.ifc_len = sizeof(int32);
	status_t status = ioctl(sock, SIOCGIFCOUNT, &ifc, sizeof(ifc));
	if(status != 0) {
		fprintf(stderr, "error interface count = %s (%x)\n", strerror(errno), errno);
		close(sock);
		return;	
	}
	
	int32 count = ifc.ifc_val;
	if (count > 0) {
		struct ifreq* ifs = new (struct ifreq)[count];
		ifc.ifc_len = sizeof(struct ifreq) * count;
		ifc.ifc_req = ifs;
		status = ioctl(sock, SIOCGIFCONF, &ifc, sizeof(ifc));
		if (status == 0) {
			unsigned char* addr = (unsigned char*) ifc.ifc_req[0].ifr_hwaddr;
			char buffer[256];
			sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
			address = buffer;
		}
		else {
			fprintf(stderr, "error getting MAC address = %s (%x)\n", strerror(errno), errno);
		}
		delete [] ifs;
	}

	// we're done with the AF_INET domain
	close(sock);
}

static void
get_os_info(OSInfo& info)
{
	// The OS information is found in /boot/home/config/settings/beia-version
	// Format of the file is
	//		product="BeIA"
	//		version=""
	//		build="2000.11.20"
	// ...but the information is also in the binder node service/system
	
	BinderNode::property product = BinderNode::Root() / "service" / "system" / "product";
	info.name = product.String();
	
	BinderNode::property version = BinderNode::Root() / "service" / "system" / "version";
	info.version = version.String();

	BinderNode::property build = BinderNode::Root() / "service" / "system" / "build";
	info.date = build.String();	
}

static void
print_system_info()
{
	info("SYSTEMCLOCK=%s\n", ValidApp::s_start_time);
	
	info("KERNEL=%s version %lx (%s %s)\n", 
			ValidApp::s_system_data.kernel_name,
			ValidApp::s_system_data.kernel_version, 
			ValidApp::s_system_data.kernel_build_date, 
			ValidApp::s_system_data.kernel_build_time);
			
	info("OS=%s - %s (%s)\n", ValidApp::s_os_info.name.String(), ValidApp::s_os_info.version.String(), ValidApp::s_os_info.date.String());

	info("Machine ID=%s\n", ValidApp::s_machine_id.String());

	info("BIOS=%s - %s (%s)\n", ValidApp::s_bios_version.String(), ValidApp::s_bios_vendor.String(), ValidApp::s_bios_date.String());

	info("MAC Address=%s\n", ValidApp::s_mac_address.String());
	
	int32 mem_size = B_PAGE_SIZE*ValidApp::s_system_data.max_pages;
	info("MEMSIZE=%08x (%dMB)\n", mem_size, mem_size/(1024*1024));
	info("(Test Location: %s - settings = %s)\n", ValidApp::s_current_directory.String(), get_settings_file());
	info("\n");
}

// ---------------------------------------------------------------------------

BString ValidApp::s_machine_id(getenv("MACHINE_ID"));
BString ValidApp::s_bios_version(getenv("BIOS_VERSION"));
BString ValidApp::s_bios_vendor(getenv("BIOS_VENDOR"));
BString ValidApp::s_bios_date(getenv("BIOS_DATE"));
BString ValidApp::s_mac_address("<not known>");
OSInfo ValidApp::s_os_info;
BString ValidApp::s_current_directory;
BString ValidApp::s_test_directory;

char ValidApp::s_start_time[32];
system_info	ValidApp::s_system_data;

// ---------------------------------------------------------------------------

ValidApp::ValidApp()
         :BApplication("application/x-vnd.be.validation-app")
{
	// waste a dummy BMessage so that we don't have to worry about NULL
	m_vendor_info = new BMessage;

	// Set up the directories before we try to read from them...
	app_info appInfo;
	BApplication::GetAppInfo(&appInfo);
	BEntry theApp(&appInfo.ref);
	BDirectory appDir;
	theApp.GetParent(&appDir);
	BPath appPath(&appDir, NULL);

	s_current_directory = appPath.Path();
	s_test_directory = "/boot/test";

	// now that we know where to get them from, read the settings
	status_t status = read_settings("validate.ini");
	if (status != B_OK) {
		info("Warning: validate.ini file could not be opened (%x - %s)", status, strerror(status));
	}

	// get system level information for various status/logging needs
	get_os_info(s_os_info);
	get_mac_address(s_mac_address);
	setup_stages(*this);

	time_t clock;
	time(&clock);
	strcpy(s_start_time, ctime(&clock));

	get_system_info(&s_system_data);

	m_curTest = 0;
	m_stage = 0;
	m_results = fopen("/tmp/valid-results.txt", "w");
	if (m_results == NULL) {
		(new BAlert("", "Cannot open result file", "Fail"))->Go();
		PostMessage(B_QUIT_REQUESTED);
		g_failed = true;
		return;
	}
	fs_write_attr(fileno(m_results), "BEOS:TYPE", B_MIME_TYPE, 0, "text/plain", 11);
	print_system_info();
}

// ---------------------------------------------------------------------------

ValidApp::~ValidApp()
{
	delete m_curTest;
	delete m_vendor_info;
	
	EnableScreenSaver(true);
}

// ---------------------------------------------------------------------------

void ValidApp::Pulse()
{
}

void ValidApp::AdoptVendorInformation(BMessage* msg)
{
	delete m_vendor_info;
	m_vendor_info = msg;
}

// ---------------------------------------------------------------------------

void ValidApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case STOP_TEST: {
			// abort the current test
			// and then just go back to stages window
			PostMessage(ALL_STAGES_DONE);
		}
		break;
	case ENTER_STAGE: {
			int32 stage = 0;
			msg->FindInt32("stage", &stage);
			EnterStage(stage);
		}
		break;
	case COMPLETED_STAGE: {
			CompletedStage();
		}
		break;
	case ALL_STAGES_DONE: {
			// clean up the current test
			delete m_curTest;
			m_curTest = 0;

			// show the stages window so they can see what has happened (and decide to prepare or not)
			m_stages_window = new StagesWindow(false, m_vendor_info);
			m_stages_window->Show();
		}
		break;
	default: {
		BApplication::MessageReceived(msg);
		}
		break;
	}
}

// ---------------------------------------------------------------------------

void ValidApp::ReadyToRun()
{
	m_stages_window = new StagesWindow(true, m_vendor_info);
	m_stages_window->Show();
	SetCursor(B_HAND_CURSOR);
	
	EnableScreenSaver(false);
}

// ---------------------------------------------------------------------------

void ValidApp::EnterStage(
	int32 stage)
{
	delete m_curTest;
	m_curTest = 0;
	bool skipped = false;

fprintf(stderr, "EnterStage(%ld)\n", stage);

	while (stage < (int32)s_stages.size())
	{
		if ((stage >= 0) && s_stages[stage]->option)
		{
fprintf(stderr, "Skipping stage '%s' (%d)\n", s_stages[stage]->name, s_stages[stage]->option);
			if (s_stages[stage]->option == kFail) g_failed = true;
			skipped = true;
		}
		else
		{
			m_stage = stage;
			attempt("Starting %s\n", s_stages[stage]->name);
			if (s_stages[stage]->func)
			{
				m_curTest = s_stages[stage]->func();
			}
			else if (s_stages[stage]->make_view_func)
			{
				m_curTest = make_alternate_test(s_stages[stage]->name, s_stages[stage]->make_view_func);
			}
			else
			{
				m_curTest = make_script_test(s_stages[stage]->short_name);
			}
		}
		if (m_curTest != NULL)
		{
			fprintf(stderr, "calling Test::Start()\n");
			m_curTest->Start();
			return;
		}
		stage++;
	}
	if (skipped)
	{
		m_stage = stage-1;
		CompletedStage();
		return;
	}
fprintf(stderr, "Falling off the end of s_stages\n");
PostMessage(B_QUIT_REQUESTED);
}

// ---------------------------------------------------------------------------

void ValidApp::CompletedStage()
{
	ASSERT(m_curTest != 0);

	if (m_curTest && !m_curTest->Successful())
	{
		fail("%s FAILED\n", s_stages[m_stage]->name);
		s_stages[m_stage]->option = kFail;
		g_failed = true;
#if !RUN_ALL_TESTS
		write_previous();
		PostMessage(B_QUIT_REQUESTED);
		return;
#endif
	}
	else
	{
		s_stages[m_stage]->option = kPass;
	}

	// Write out current results
	write_previous();

	if (s_stages[m_stage]->next_stage == -1)	//	we're done!
	{
		fprintf(stderr, "acceptance: all tests completed\n");
		s_stages[m_stage]->option = kUnknown;
		PostMessage(ALL_STAGES_DONE);
	}
	else
	{
		fprintf(stderr, "moving on to stage %d\n", m_stage+1);
		BMessage msg(ENTER_STAGE);
		msg.AddInt32("stage", m_stage+1);
		PostMessage(&msg);
	}
}

// ---------------------------------------------------------------------------

bool ValidApp::QuitRequested()
{
	if (m_stages_window->Lock()) {
		m_stages_window->Quit();
	}
	m_stages_window = NULL;

	return true;
}

// ---------------------------------------------------------------------------

void ValidApp::EnableScreenSaver(bool Enable)
{
	// Wait until this property gets defined before we proceed.
	// Have a 25 second timout because it seems like a Good Idea(tm)
	const uint32 timeout = 100;	// 100 * 0.25 sec. = 25 sec.
	uint32 i = 0;
	for (i = 0; true == (BinderNode::Root()
		/ "service" / "screensaver" / "settings" / "no_blank")().IsUndefined() && (i < timeout); i++ )
	{
		snooze(250000); // wait a quarter sec. 
	}
	
	if( i == timeout )
	{
		fprintf(stderr, "\n!!! Timeout while waiting for 'service/screensaver/"
		                "settings/no_blank' to become valid.\n");
		return;
	}

	// This may seem a bit confusing, but that's the way they wanted it:
	// To disable the screen saver, you set 'no_blank' to 'true'.
	(BinderNode::Root() / "service" / "screensaver" /
	                      "settings" / "no_blank") =
	                      Enable ? "false" : "true" ;
}

// ---------------------------------------------------------------------------



// ---------------------------------------------------------------------------
// Utility for making big alerts
// ---------------------------------------------------------------------------

static void SetLargeAlertFont(BView* view)
{
	// I guess the monitors sit a long way away from the factory technicians
	// Sony wanted 18 point fonts.
	view->SetFontSize(18.0);
	BTextView* textView = dynamic_cast<BTextView *>(view);
	if (textView != NULL) {
		BFont font(be_plain_font);
		font.SetSize(18.0);
		textView->SetFontAndColor(&font);
		BRect bounds = textView->Bounds();
		// resize textview so that all lines are visible.
		float height = textView->CountLines() * textView->LineHeight();
		float diffY = height - bounds.bottom;
		textView->ResizeTo(bounds.Width(), bounds.Height());
		textView->Window()->ResizeBy(0., diffY);
	}
	for (int i = 0; i < view->CountChildren(); i++) {
		SetLargeAlertFont(view->ChildAt(i));
	}
}

// ---------------------------------------------------------------------------

void SetLargeAlertFont(BWindow* aWindow)
{
	if (aWindow->Lock()) {
		aWindow->BeginViewTransaction();
		for (int i = 0; i < aWindow->CountChildren(); i++) {
			SetLargeAlertFont(aWindow->ChildAt(i));
		}
		aWindow->EndViewTransaction();
		aWindow->Unlock();
	}
}
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
