#include <Application.h>
#include <Debug.h>
#include <String.h>
#include <StringView.h>
#include <ParameterWeb.h>
#include <ListView.h>
#include <ListItem.h>
#include <Box.h>
#include <MediaRoster.h>
#include <Alert.h>
#include <Application.h>
//#include <ScrollView.h>
#include <MediaTheme.h>
#include <Bitmap.h>
#include <MenuControl.h>
#include <MenuField.h>
#include <TabView.h>
#include <MediaAddOn.h>
#include <MenuItem.h>
#include <Font.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Audio.h"


#define MIN_WIDTH 320
#define MIN_HEIGHT 240

struct prefs {
	prefs(const BRect & ir) : rect(ir) { }
	BRect rect;
};

static prefs g_prefs(BRect(32,64,632,464));

static void read_prefs()
{
	BPath dir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir, true) < B_OK)
		return;
	dir.Append("AudioPrefs Settings");
	FILE * fil = fopen(dir.Path(), "r");
	if (!fil)
		return;
	char line[1024];
	while (!feof(fil) && !ferror(fil))
	{
		line[0] = 0;
		fgets(line, 1024, fil);
		if (!line[0])
			break;
		if (line[0] == '#')
			continue;
		if (line[0] == '\n')
			continue;
		if (sscanf(line, " rect = %f , %f , %f , %f",
				&g_prefs.rect.left, &g_prefs.rect.top, 
				&g_prefs.rect.right, &g_prefs.rect.bottom) > 0)
			continue;
		//	ignore line
		fprintf(stderr, "unknown setting: %s", line);
	}
	fclose(fil);
	BScreen scrn;
	BRect f = scrn.Frame();
	if (g_prefs.rect.right < g_prefs.rect.left + MIN_WIDTH) g_prefs.rect.right = g_prefs.rect.left+MIN_WIDTH;
	if (g_prefs.rect.bottom < g_prefs.rect.top + MIN_HEIGHT) g_prefs.rect.bottom = g_prefs.rect.top+MIN_HEIGHT;
	if (g_prefs.rect.Width() > f.Width()) g_prefs.rect.right = g_prefs.rect.left+f.Width();
	if (g_prefs.rect.Height() > f.Height()) g_prefs.rect.bottom = g_prefs.rect.top+f.Height();
	if (g_prefs.rect.right > f.right) g_prefs.rect.OffsetBy(f.right-g_prefs.rect.right, 0);
	if (g_prefs.rect.bottom > f.bottom) g_prefs.rect.OffsetBy(0, f.bottom-g_prefs.rect.bottom);
	if (g_prefs.rect.left < f.left) g_prefs.rect.OffsetBy(f.left-g_prefs.rect.left, 0);
	if (g_prefs.rect.top < f.top) g_prefs.rect.OffsetBy(0, f.top-g_prefs.rect.top);
}

static void write_prefs()
{
	BPath dir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir, true) < B_OK)
		return;
	dir.Append("AudioPrefs Settings");
	FILE * f = fopen(dir.Path(), "w");
	if (!f)
		return;
	fprintf(f, "# AudioPrefs Settings\n");
	fprintf(f, " rect = %.0f,%.0f,%.0f,%.0f\n", g_prefs.rect.left, g_prefs.rect.top,
			g_prefs.rect.right, g_prefs.rect.bottom);
	fclose(f);
}


//
//	When this is defined, menu fields are shown that allow the
//	user to select inputs and outputs
//
#define SHOW_HARDWARE_POPUPS 1
//

#define RESTART_SERVER 'resr'

class ResizeAllTabView : public BTabView {
public:
		ResizeAllTabView(
				const BRect & area,
				const char * name) :
			BTabView(area, name)
			{
			}


		//
		// When this view is selected, resize it to fit as it 
		// wasn't getting resized messages when it wasn't
		// showing.
		//
		virtual void Select(int32 tabNum) 
			{
				BTabView::Select(tabNum);
				BView * v = ViewForTab(tabNum);
				if (v) {
					BRect r(ContainerView()->Bounds());
					r.InsetBy(1,1);
					v->MoveTo(r.LeftTop());
					v->ResizeTo(r.Width(), r.Height());
				}
			}

		void AddTab(
				BView* tabContents,
				BTab* tab=NULL)
			{
				BRect r(ContainerView()->Bounds());
				r.InsetBy(1,1);
				tabContents->MoveTo(r.LeftTop());
				tabContents->ResizeTo(r.Width(), r.Height());
				BTabView::AddTab(tabContents, tab);
			}
};



class BEmptyView : public BView {
public:
		BEmptyView(
				const BRect area,
				const char * name,
				uint32 resize,
				uint32 flags) :
			BView(area, name, resize, flags | B_FULL_UPDATE_ON_RESIZE)
			{
			}
virtual	void Draw(
				BRect area)
			{
				SetLowColor(ViewColor());
				SetHighColor(0,0,0);
				font_height fh;
				GetFontHeight(&fh);
				const char * str = "This hardware has no controls.";
				float y = (int)(Bounds().Height()+fh.ascent-fh.descent-fh.leading)/2;
				float x = (int)(Bounds().Width()-StringWidth(str))/2;
				DrawString(str, BPoint(x, y));
			}
};


const int32 M_SETINPUT = 'seti';
const int32 M_SETOUTPUT = 'seto';

class HardwareView : public BView {
public:

					HardwareView(BRect);

#if SHOW_HARDWARE_POPUPS
	virtual void	MessageReceived(BMessage*); 	
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect area);
	virtual void	FrameResized(float width, float height);
#endif

private:

#if SHOW_HARDWARE_POPUPS
	BMenuField *outField;
	BMenuField *inField;
	bool m_drawWarning;
	BRect m_textRect;
#else
	BStringView *outField;
	BStringView *inField;
#endif
	

	BMenu *outMenu;
	BMenu *inMenu;

	dormant_node_info inNodeList[20];
	dormant_node_info outNodeList[20];

	BMediaRoster *roster;
};

HardwareView::HardwareView(BRect rect)
	:	BView(rect, "Hardware", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
{
	roster = BMediaRoster::Roster();

	int32 numNodes;
	media_format format;
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_raw_audio_format::wildcard;

	// Find out the currently active output node
	media_node outputNode;
	live_node_info outputNodeInfo;
	const char *currentOutputNodeName = "<none>";
	status_t err = roster->GetAudioOutput(&outputNode);
	PRINT(("Node: %d %d %x\n", outputNode.node, outputNode.port, outputNode.kind));
	if (err == B_OK) {
		err = roster->GetLiveNodeInfo(outputNode, &outputNodeInfo);
		if (err == B_OK) {
			currentOutputNodeName = outputNodeInfo.name;
		}
	}

	PRINT(("error %x\n", err));
	BRect outFieldRect(55, 35, Bounds().Width(), 55);

#if SHOW_HARDWARE_POPUPS

	outMenu = new BPopUpMenu(currentOutputNodeName);
	
	// Fill in the output menu with available dormant nodes
	numNodes = 20;
	status_t j_err = roster->GetDormantNodes(outNodeList, &numNodes, &format, 	
		NULL, NULL, B_PHYSICAL_OUTPUT); 
	for (int32 index = 0; index < numNodes; ++index) {
		BMessage *invokeMessage = new BMessage(M_SETOUTPUT);
		invokeMessage->AddInt32("index", index);
		outMenu->AddItem(new BMenuItem(outNodeList[index].name, invokeMessage));
	}

	outField = new BMenuField(outFieldRect, "out field", "Sound Out", outMenu);
	outField->SetDivider(55);

	m_drawWarning = false;

#else
	
	BString label = "Sound Out:   ";
	label += currentOutputNodeName ? currentOutputNodeName : "<none>";
	outField = new BStringView(outFieldRect, "out field", label.String());

#endif
	
	AddChild(outField);

	// Find of the currently active input node
	media_node inputNode;
	live_node_info inputNodeInfo;
	const char *currentInputNodeName = "<none>";
	err = roster->GetAudioInput(&inputNode);
	if (err == B_OK) {
		err = roster->GetLiveNodeInfo(inputNode, &inputNodeInfo);
		if (err == B_OK) {
			currentInputNodeName = inputNodeInfo.name;
		}
	}	

	if (err != B_OK)
		PRINT(("Couldn't get live node info: %x\n", err));

	BRect inFieldRect(55, 65, Bounds().Width(), 85);

#if SHOW_HARDWARE_POPUPS

	inMenu = new BPopUpMenu(currentInputNodeName);
	
	// Fill in the input menu with available dormant nodes
	numNodes = 20;
	roster->GetDormantNodes(inNodeList, &numNodes, NULL, &format, NULL, 
		B_PHYSICAL_INPUT); 
	for (int32 index = 0; index < numNodes; ++index) {
		BMessage *invokeMessage = new BMessage(M_SETINPUT);
		invokeMessage->AddInt32("index", index);
		inMenu->AddItem(new BMenuItem(inNodeList[index].name, invokeMessage));
	}
	
	inField = new BMenuField(inFieldRect, "in field", "Sound In", inMenu);
	inField->SetDivider(55);

#else

	label = "Sound In:   ";
	label += currentInputNodeName ? currentInputNodeName : "<none>";
	inField = new BStringView(inFieldRect, "in field", label.String());

#endif

	AddChild(inField);

	BRect b(Bounds());
	b.InsetBy(10, 10);
	b.left = b.right-180;
	b.top = b.bottom-21;
	BButton * restartButton = new BButton(b, "restart", "Restart Media Services",
		new BMessage(RESTART_SERVER), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(restartButton);
}


#if SHOW_HARDWARE_POPUPS
void HardwareView::AttachedToWindow()
{
	inMenu->SetTargetForItems(this);
	outMenu->SetTargetForItems(this);
}

void HardwareView::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case M_SETOUTPUT:
		{
			int32 index = 0;
			if (message->FindInt32("index", &index) == B_OK) {
				// Get old node
				media_node oldNode;
				bool releaseOldNode = (roster->GetAudioOutput(&oldNode) == B_OK);
				
				//	Create a new one
				media_node newNode;
				if (roster->InstantiateDormantNode(outNodeList[index],
					&newNode) != B_OK
					|| roster->SetAudioOutput(newNode) != B_OK) 
					releaseOldNode = false;

				// release old node					
				if (releaseOldNode)
					roster->ReleaseNode(oldNode);

				((AudioWindow*) Window())->SetOutputView(false);
			}
			bool prevWarning = m_drawWarning;
			m_drawWarning = true;
			if (!prevWarning) {
				Invalidate();
			}
		}
		break;
	
	
	case M_SETINPUT:
		{
			int32 index = 0;
			if (message->FindInt32("index", &index) == B_OK) {
				// Get current node
				media_node oldNode;
				bool releaseOldNode = (roster->GetAudioInput(&oldNode) == B_OK);
				
				// Create a new one
				media_node newNode;
				if (roster->InstantiateDormantNode(inNodeList[index],
					&newNode) != B_OK || roster->SetAudioInput(newNode)
					!= B_OK)
					releaseOldNode = false;

				// Release old node
				if (releaseOldNode)
					roster->ReleaseNode(oldNode);

				((AudioWindow*) Window())->SetInputView(false);
			}	
			bool prevWarning = m_drawWarning;
			m_drawWarning = true;
			if (!prevWarning) {
				Invalidate();
			}
		}
		break;
	
	default:
		BView::MessageReceived(message);
	}
}

void
HardwareView::Draw(BRect area)
{
	if (m_drawWarning)
	{
		static const char the_str[] = "Restart media to apply changes";
		BRect b(Bounds());
		BFont f;
		GetFont(&f);
		font_height fi;
		f.GetHeight(&fi);
		b.left += 20;
		b.bottom -= 10+fi.descent;
		b.right = b.left+f.StringWidth(the_str)+1;
		b.top = b.bottom-fi.ascent+fi.leading;
		SetLowColor(ViewColor());
		SetHighColor(192,0,0);
		DrawString(the_str, b.LeftBottom());
		SetHighColor(0,0,0);
		b.bottom += fi.descent;
		m_textRect = b;
	}
}

void
HardwareView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	if (m_drawWarning)
	{
		SetLowColor(ViewColor());
		FillRect(m_textRect, B_SOLID_LOW);
		Draw(Bounds());
		Flush();
	}
}

#endif

void
AudioWindow::SetOutputView(bool addTab)
{
	PRINT(("Updating Output Tab\n"));

	BRect fr = m_tabs->ContainerView()->Bounds();
	fr.InsetBy(1,1);

	status_t err = m_roster->GetAudioOutput(&m_output_node);
	m_output_web = NULL;
	if (m_output_node.kind & B_CONTROLLABLE) {
		err = m_roster->GetParameterWebFor(m_output_node, &m_output_web);
	}

	m_output_view = NULL;
	BView *themeView = NULL;
	if (m_output_web) {
		BRect r2(0,0,200,100);
		themeView = BMediaTheme::ViewFor(m_output_web, &r2);
		themeView->SetName("Output");
	}

	if (themeView) {
#if 0
		themeView->ResizeTo(fr.Width() - B_V_SCROLL_BAR_WIDTH - 2, 
			fr.Height() - B_H_SCROLL_BAR_HEIGHT - 2);

		m_output_view = new BScrollView("Output", themeView,
			B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW, true, true, 
			B_PLAIN_BORDER);
#endif
		m_output_view = themeView;
	} else
		m_output_view = new BEmptyView(fr, "Output", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);

	m_output_view->SetViewColor(216, 216, 216);

	if (addTab)
		m_tabs->AddTab(m_output_view);
	else {
		m_tabs->TabAt(1)->SetView(m_output_view);
		if (m_tabs->Selection() == 1)
			m_tabs->Select(1);
	}
}



void
AudioWindow::SetInputView(bool addTab)
{
	PRINT(("Updating Input Tab\n"));

	BRect fr = m_tabs->ContainerView()->Bounds();
	fr.InsetBy(1,1);
	status_t err = m_roster->GetAudioInput(&m_input_node);
	m_input_web = NULL;
	if (m_input_node.kind & B_CONTROLLABLE) {
		err = m_roster->GetParameterWebFor(m_input_node, &m_input_web);
	}
	m_input_view = NULL;

	BView *themeView = NULL;
	if (m_input_web)
	{
		themeView = BMediaTheme::ViewFor(m_input_web);
		themeView->SetName("Input");
	}


	if (themeView) {
#if 0
		themeView->ResizeTo(fr.Width() - B_V_SCROLL_BAR_WIDTH - 2,
			fr.Height() - B_H_SCROLL_BAR_HEIGHT - 2);

		m_input_view = new BScrollView("Input", themeView,
			B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW, true, true, 						B_PLAIN_BORDER);
#endif
		m_input_view = themeView;
	} else
		m_input_view = new BEmptyView(fr, "Input", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);

	m_input_view->SetViewColor(216, 216, 216);

	if (addTab)
		m_tabs->AddTab(m_input_view);
	else {
		m_tabs->TabAt(0)->SetView(m_input_view);
		if (m_tabs->Selection() == 0)
			m_tabs->Select(0);
	}
}


void
AudioWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);
	UpdateScrollBars();
	g_prefs.rect = Frame();
}

void
AudioWindow::FrameMoved(BPoint position)
{
	BWindow::FrameMoved(position);
	g_prefs.rect = Frame();
}

void
AudioWindow::UpdateScrollBars()
{
//	BRect r = m_mixer_view->Bounds();
//	m_input_view->ScrollBar(B_HORIZONTAL)->SetRange(0, MAX(m_mixer_rect.Width() - r.Width(), 0));
//	m_input_view->ScrollBar(B_VERTICAL)->SetRange(0, MAX(m_mixer_rect.Height() - r.Height(), 0));		
//	m_output_view->ScrollBar(B_HORIZONTAL)->SetProportion(r.Width()/m_mixer_rect.Width());
//	m_output_view->ScrollBar(B_VERTICAL)->SetProportion(r.Height()/m_mixer_rect.Height());
//	m_mixer_view->ScrollBar(B_HORIZONTAL)->SetSteps(25, r.Width());
//	m_mixer_view->ScrollBar(B_VERTICAL)->SetSteps(20, r.Height());
}

void
AudioWindow::SetMixerView(bool addTab)
{
	PRINT(("Updating Mixer Tab\n"));

	BView *scrollView;
	BRect fr = m_tabs->ContainerView()->Bounds();
	fr.InsetBy(1,1);
	status_t err = m_roster->GetAudioMixer(&m_mixer_node);
	m_mixer_web = NULL;
	err = m_roster->GetParameterWebFor(m_mixer_node, &m_mixer_web);
	m_mixer_view = NULL;

	BView *themeView = NULL;
	if (m_mixer_web) {
		themeView = BMediaTheme::ViewFor(m_mixer_web, &fr);
		themeView->SetName("Mixer");
	}
	if (themeView) {
		m_mixer_view = themeView;
		m_mixer_rect = themeView->Frame();
#if 0
		themeView->ResizeTo(fr.Width() - B_V_SCROLL_BAR_WIDTH - 2,
			fr.Height() - B_H_SCROLL_BAR_HEIGHT - 2);
		scrollView = new BScrollView("Mixer", m_mixer_view,
			B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW, true, true, B_PLAIN_BORDER);
#endif
		scrollView = m_mixer_view;
		UpdateScrollBars();		
	} else {
		scrollView = new BEmptyView(fr, "Mixer", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	}
	scrollView->SetViewColor(216, 216, 216);

	if (addTab)
		m_tabs->AddTab(scrollView);
	else {
		m_tabs->TabAt(2)->SetView(scrollView);
		if (m_tabs->Selection() == 2)
			m_tabs->Select(2);
	}
}

AudioWindow::AudioWindow(
	const BRect area,
	const char * name) : 
	BWindow(area, name, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	SetSizeLimits(MIN_WIDTH, 2048, MIN_HEIGHT, 1536);
	m_bg = new BBox(Bounds(), "background", B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	m_bg->SetViewColor(216, 216, 216);
	AddChild(m_bg);
	status_t err = B_OK;
	m_roster = BMediaRoster::Roster(&err);
	if (!m_roster || err) {
		char msg[300];
		if (err >= 0) {
			err = B_ERROR;
		}
		sprintf(msg, "Cannot connect to the media server:\n%s [%x]",
			strerror(err), err);
		(new BAlert("", msg, "Quit"))->Go();
		Run();	/* so Lock()/Quit() will work */
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	BRect r(Bounds());
	r.InsetBy(12, 12);
	m_tabs = new ResizeAllTabView(r, "Settings");

	BRect fr = m_tabs->ContainerView()->Bounds();
	fr.InsetBy(1,1);

	SetInputView(true);
	SetOutputView(true);
	SetMixerView(true);

	//	Hardware
	m_hardware_view = new HardwareView(fr);
	m_hardware_view->SetName("Hardware");
	m_hardware_view->SetViewColor(216, 216, 216);
	m_tabs->AddTab(m_hardware_view);

	BMediaRoster::Roster()->StartWatching(BMessenger(0, this));

	// 	work around a bug in BTabView by adding the view to the window 
	//	down here 
	m_bg->AddChild(m_tabs);

	// Focus the mixer view
	m_tabs->Select(2);

	Show();	/* when done, we do this */
}

AudioWindow::~AudioWindow()
{
}


static void
dump_web(
	BParameterWeb * web)
{
	printf("Web: %d parameters\n", web->CountParameters());
	for (int ix=0; ix<web->CountParameters(); ix++) {
		BParameter * p = web->ParameterAt(ix);
		BNullParameter * np;
		BDiscreteParameter * dp;
		BContinuousParameter * cp;
		if ((np = dynamic_cast<BNullParameter *>(p)) != NULL) {
			printf("%d: NullParameter '%s': %s\n", p->ID(), np->Name(), np->Kind());
		}
		else if ((dp = dynamic_cast<BDiscreteParameter *>(p)) != NULL) {
			printf("%d: DiscreteParameter '%s': %s\n", p->ID(), dp->Name(), dp->Kind());
		}
		else if ((cp = dynamic_cast<BContinuousParameter *>(p)) != NULL) {
			printf("%d: ContinuousParameter '%s': %s\n", p->ID(), cp->Name(), cp->Kind());
		}
		else if (p) {
			printf("%d: Unknown %s '%s': %s\n", p->ID(), typeid(*p).name(), p->Name(), p->Kind());
		}
		else {
			printf("NULL parameter at index %d!!!\n", ix);
			continue;
		}
		for (int i = 0; i<p->CountInputs(); i++) {
			if (i == 0) {
				printf("  inputs: ");
			}
			else {
				printf(", ");
			}
			printf("%d", p->InputAt(i)->ID());
		}
		if (p->CountInputs()) {
			printf("\n");
		}
		for (int i = 0; i<p->CountOutputs(); i++) {
			if (i == 0) {
				printf("  outputs: ");
			}
			else {
				printf(", ");
			}
			printf("%d", p->OutputAt(i)->ID());
		}
		if (p->CountOutputs()) {
			printf("\n");
		}
	}
}

void
AudioWindow::MessageReceived(BMessage * message) 
{
	switch (message->what) {
		case B_MEDIA_WEB_CHANGED: {
			int32 nodeIndex = 0;
			media_node *node;
			ssize_t nodeSize;
			while (message->FindData("node", B_RAW_TYPE, nodeIndex, 
				(const void**) &node, &nodeSize) == B_OK) {
				nodeIndex++;
				media_node_id nodeID = node->node;
				if (nodeID == m_mixer_node.node)
					SetMixerView(false);	
				else if (nodeID == m_input_node.node)
					SetInputView(false);
				else if (nodeID == m_output_node.node)
					SetOutputView(false);
			}	

			break;	
		}
		case RESTART_SERVER:
			RestartServer();
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

bool
AudioWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

	static bool
	callback(
		int stage,
		const char * message,
		void * cookie)
	{
		BStringView * sv = (BStringView *)cookie;
		sv->Window()->Lock();
		if (strcmp(message, sv->Text()))
		{
			sv->SetText(message);
			sv->Invalidate();
		}
		sv->Window()->Unlock();
		return true;
	}

void
AudioWindow::RestartServer()
{
	BRect r(0,0,300,60);
	r.OffsetBy(BAlert::AlertPosition(r.Width(), r.Height()));
	BWindow * w = new BWindow(r, "Please Wait", B_TITLED_WINDOW, B_NOT_MOVABLE |
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE);
	BView * v = new BView(w->Bounds(), "background", B_FOLLOW_NONE, B_WILL_DRAW);
	v->SetViewColor(216, 216, 216);
	w->AddChild(v);
	r = v->Bounds();
	r.InsetBy(10, 10);
	BStringView * sv = new BStringView(r, "message", "Stopping media server...");
	sv->SetFont(be_bold_font);
	sv->SetViewColor(v->ViewColor());
	sv->SetLowColor(sv->ViewColor());
	v->AddChild(sv);
	w->Show();

	status_t err = shutdown_media_server(6000000, callback, sv);
#if 0
	fprintf(stderr, "Shutting down current server...\n");
	system("/bin/kill -HUP media_server");
	snooze(4000000);
	fprintf(stderr, "Making sure...\n");
	w->Lock();
	sv->SetText("Starting new media server...");
	sv->Invalidate();
	w->Unlock();
	system("/bin/kill -9 media_server");
	system("/bin/kill -9 media_addon_server");
	system("/bin/kill -9 audio_server");
	snooze(1000000);
	fprintf(stderr, "Starting new server...\n");
	system("/system/servers/media_server &");
	fprintf(stderr, "Done.\n");
	w->Lock();
	sv->SetText("Done.");
	sv->Invalidate();
	w->Unlock();
	snooze(700000);
#endif
	if (err == B_OK)
	{
		w->Lock();
		sv->SetText("Starting new media_server.");
		sv->Invalidate();
		w->Unlock();
		err = startup_media_server();
	}
	w->PostMessage(B_QUIT_REQUESTED);
	if (err == B_OK)
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	else
	{
		char msg[1000];
		sprintf(msg, "There was an error restarting the media_server: %x\n%s.",
			err, strerror(err));
		(new BAlert("Restart Error", msg, "Stop"))->Go();
	}
}




class AudioApp : public BApplication {
public:
		AudioApp();
virtual	bool QuitRequested();
virtual	void MessageReceived(
				BMessage * message);
private:
		AudioWindow * w;
};

AudioApp::AudioApp() : BApplication("application/x-vnd.Be.AudioPrefs")
{
	read_prefs();
	w = new AudioWindow(g_prefs.rect, "Audio");
}

bool AudioApp::QuitRequested()
{
	if (w->Lock()) {
		w->Quit();
	}
	write_prefs();
	return true;
}

void AudioApp::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
		case RESTART_SERVER:
			w->RestartServer();
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

int
main(
	int argc,
	char * argv[])
{
	try {
		if (argc > 2 || (argc == 2 && strcmp(argv[1], "--restart"))) {
			fprintf(stderr, "usage: Audio [ --restart ]\n");
			return 1;
		}
		AudioApp app;
		if (argc == 2) {
			app.PostMessage(RESTART_SERVER);
			app.PostMessage(B_QUIT_REQUESTED);
		}
		app.Run();
	}
	catch (exception & e) {
		fprintf(stderr, "exception: %s\n", e.what());
		char msg[200];
		sprintf(msg, "An unexpected error has caused Audio to quit: %s", e.what());
		(new BAlert("", msg, "Quit"))->Go();
	}
	catch (...) {
		fprintf(stderr, "exception: unknowns\n");
		char msg[200];
		sprintf(msg, "An unexpected error has caused Audio to quit.");
		(new BAlert("", msg, "Quit"))->Go();
	}
	return 0;
}
