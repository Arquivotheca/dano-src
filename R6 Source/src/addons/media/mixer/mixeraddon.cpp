#include "mixeraddon.h"
#include "mixer.h"

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <ScrollView.h>
#include <Alert.h>
#include <MediaTheme.h>
#include <MediaRoster.h>
#include <stdio.h>
#include <string.h>

// NDEBUG is defined (or not) in the makefile
#if !NDEBUG
	#define PRINTF printf
	#define FPRINTF fprintf
	#define DEBUG 1
#else
// don't printf anything at all
	#define PRINTF(x...)
#endif


BMixerAddOn::BMixerAddOn(image_id myImage)
	: BMediaAddOn(myImage)
{
	PRINTF("BMixerAddOn::BMixerAddOn() - BEGIN\n");

	// we only ever return a single flavor_info structure, so we build
	// it once here and cache it.
	m_flavorInfo.internal_id = 0;
	m_flavorInfo.name = "Be Audio Mixer";
	m_flavorInfo.info = "The system-wide sound mixer of the future.";
	m_flavorInfo.kinds = B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER;
	m_flavorInfo.flavor_flags = 0;
	m_flavorInfo.possible_count = 1;
	
	// Input formats, for buffer consumer
	m_flavorInfo.in_format_count = 1;
	media_format * bFormat = new media_format;
	bFormat->type = B_MEDIA_RAW_AUDIO;
	bFormat->u.raw_audio = media_raw_audio_format::wildcard;
	m_flavorInfo.in_formats = bFormat;
	m_flavorInfo.in_format_flags = 0;
	
	// Output formats, only one
	m_flavorInfo.out_format_count = 1;
	media_format * aFormat = new media_format;
	aFormat->type = B_MEDIA_RAW_AUDIO;
	aFormat->u.raw_audio = media_raw_audio_format::wildcard;
	m_flavorInfo.out_formats = aFormat;
	m_flavorInfo.out_format_flags = 0;
}

BMixerAddOn::~BMixerAddOn()
{
	// delete the dynamically-allocated portions of the cached flavor_info structure
	delete m_flavorInfo.in_formats;
	delete m_flavorInfo.out_formats;
}

const char *errorString = "BMixerAddOn::Standard Error String";

status_t 
BMixerAddOn::InitCheck(const char **out_failure_text)
{
	*out_failure_text = errorString;
	
	return B_OK;
}

int32 
BMixerAddOn::CountFlavors()
{
	PRINTF("BMixerAddOn::CountFlavors()\n");

	return 1;
}

/*
	Note: n should be const
	Bugs: Server does not behave properly
	when status returned is B_ERROR, and *out_info == 0
*/

status_t 
BMixerAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	PRINTF("BMixerAddOn::GetFlavorInfo(%d,...)\n", n);

	if (n != 0)
		return B_ERROR;
		
	*out_info = &m_flavorInfo;
	
	return B_OK;
}

BMediaNode *
BMixerAddOn::InstantiateNodeFor(const flavor_info *info, BMessage */*config*/, status_t */*out_error*/)
{
	//PRINTF("BMixerAddOn::MakeFlavorInstance()\n");

	BMixer *aNode = new BMixer(info->name, info->internal_id, 3, 2, this);
	
	return aNode;
}

status_t
BMixerAddOn::GetConfigurationFor(BMediaNode * /*your_node*/, BMessage * /*into_message*/)
{
	return B_OK;
}

status_t 
BMixerAddOn::SaveConfigurationFor(BMediaNode */*your_node*/, BMessage */*into_message*/)
{
	//printf("BMixerAddOn::SaveConfigInfo()\n");
	// We don't really save anything at this point
	return B_OK;
}

bool 
BMixerAddOn::WantsAutoStart()
{
	//PRINTF("BMixerAddOn::WantsAutoStart()\n");
	return true;
}

status_t 
BMixerAddOn::AutoStart(int in_count, BMediaNode ** out_node,
	int32 * out_internal_id,
	bool * out_has_more)
{
	PRINTF("BMixerAddOn::AutoStart()\n");
	*out_has_more = false;
	if (in_count > 0) return B_ERROR;
	BMixer *aNode = new BMixer("Audio Mixer", in_count, 8, 2, this);
	*out_node = aNode;
	*out_internal_id = 0;
	
	return B_OK;
}

/*
	Function: make_media_addon
	
	This is the function that is exported for the add-on.
	The server looks for it so it can instantiate addons.
*/

BMediaAddOn *
make_media_addon(image_id myImage)
{
	return new BMixerAddOn(myImage);
}


/*
	Control Panels from here to insanity
*/


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
				BRect /*area*/)
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


class MixerWindow : public BWindow
{
public:
		MixerWindow(const BRect area, media_node_id id, const char * name);
		~MixerWindow();

virtual		void MessageReceived(BMessage * message);
virtual		bool QuitRequested();
virtual		void FrameResized(float width, float height);
virtual 	void UpdateScrollBars();
virtual		void SetMixerView();

private:
		media_node m_mixer_node;
		BParameterWeb *m_mixer_web;
		BView *m_mixer_view;
		BRect m_mixer_rect;
		BMediaRoster *m_roster;
};

MixerWindow::MixerWindow(
	const BRect area,
	media_node_id id,
	const char * name) : 
	BWindow(area, name, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	SetSizeLimits(320, 2048, 240, 1536);
	status_t err = B_OK;
	m_roster = BMediaRoster::Roster(&err);
	if (!m_roster || err) {
		char msg[300];
		if (err >= 0) {
			err = B_ERROR;
		}
		sprintf(msg, "Cannot connect to the media server:\n%s [%lx]",
			strerror(err), err);
		(new BAlert("", msg, "Quit"))->Go();
		Run();	/* so Lock()/Quit() will work */
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	PRINTF("one\n");
	if(id < 0){
		err = m_roster->GetAudioMixer(&m_mixer_node);
	}else{
		err = m_roster->GetNodeFor(id, &m_mixer_node);
	}
	if (err) {
		char msg[300];
		if (err >= 0) {
			err = B_ERROR;
		}
		sprintf(msg, "Cannot find the Mixer: node_id %ld\n%s [%lx]",
			id, strerror(err), err);
		(new BAlert("", msg, "Quit"))->Go();
		Run();	/* so Lock()/Quit() will work */
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	BRect fr = Bounds();
	fr.InsetBy(1,1);
	
	err = BMediaRoster::Roster()->GetParameterWebFor(m_mixer_node, &m_mixer_web);
	
	if (m_mixer_web) {
		BRect r2(fr);
		m_mixer_view = BMediaTheme::ViewFor(m_mixer_web, &r2);
	}
	if (!m_mixer_view) {
		char msg[300];
		if (err >= 0) {
			err = B_ERROR;
		}
		sprintf(msg, "Cannot get the Mixer's ParamterWeb:\n%s [%lx]",
			strerror(err), err);
		(new BAlert("", msg, "Quit"))->Go();
		Run();	/* so Lock()/Quit() will work */
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	m_mixer_view->ResizeTo(fr.Width(), fr.Height());
	m_mixer_view->SetName("Mixer");
	m_mixer_view->SetViewColor(216, 216, 216);
	
	AddChild(m_mixer_view);
	
	BMediaRoster::Roster()->StartWatching(BMessenger(0, this), m_mixer_node, B_MEDIA_WEB_CHANGED);
	
	Show();	/* when done, we do this */
}

MixerWindow::~MixerWindow()
{
}

void
MixerWindow::FrameResized(float /*width*/, float /*height*/)
{
	//UpdateScrollBars();
}

void
MixerWindow::UpdateScrollBars()
{
	BRect r = m_mixer_view->Bounds();
	m_mixer_view->ScrollBar(B_HORIZONTAL)->SetRange(0, MAX(m_mixer_rect.Width() - r.Width(), 0));
	m_mixer_view->ScrollBar(B_VERTICAL)->SetRange(0, MAX(m_mixer_rect.Height() - r.Height(), 0));		
	m_mixer_view->ScrollBar(B_HORIZONTAL)->SetProportion(r.Width()/m_mixer_rect.Width());
	m_mixer_view->ScrollBar(B_VERTICAL)->SetProportion(r.Height()/m_mixer_rect.Height());
	m_mixer_view->ScrollBar(B_HORIZONTAL)->SetSteps(25, r.Width());
	m_mixer_view->ScrollBar(B_VERTICAL)->SetSteps(20, r.Height());
}

void
MixerWindow::SetMixerView()
{
	PRINTF(("Updating Mixer View\n"));

	//BView *scrollView;
	
	BRect fr = Bounds();
	fr.InsetBy(1,1);
	m_mixer_web = NULL;
	//status_t err = m_roster->GetParameterWebFor(m_mixer_node, &m_mixer_web);

	BView *themeView = NULL;
	if (m_mixer_web) {
		themeView = BMediaTheme::ViewFor(m_mixer_web, &fr);
	}
	if (themeView && Lock()) {
		RemoveChild(m_mixer_view);
		delete m_mixer_view;
		m_mixer_view = themeView;
		m_mixer_rect = themeView->Frame();
		AddChild(m_mixer_view);
		Unlock();
	}
}

void
MixerWindow::MessageReceived(
	BMessage * message)
{
	switch(message->what){
		case B_MEDIA_WEB_CHANGED: {
			int32 nodeIndex = 0;
			media_node *node;
			ssize_t nodeSize;
			while (message->FindData("node", B_RAW_TYPE, nodeIndex, 
				(const void**) &node, &nodeSize) == B_OK) {
				nodeIndex++;
				media_node_id nodeID = node->node;
				if (nodeID == m_mixer_node.node){
					SetMixerView();
				}
			}	
			break;	
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool
MixerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}


class MixerApp : public BApplication {
public:
		MixerApp();
virtual void MessageReceived(BMessage *msg);
virtual	bool QuitRequested();
};

MixerApp::MixerApp() : BApplication("application/x-vnd.Be.MixerAddOn")
{
}

void
MixerApp::MessageReceived(BMessage *msg)
{
	PRINTF("AudioApp::MessageReceived  what:%x\n", msg->what);
	switch(msg->what){
		case B_LAUNCH_MIXER_CONTROL:{
			PRINTF("AudioApp::MessageReceived B_LAUNCH_MIXER_CONTROL  what:%x\n", msg->what);
			status_t err = B_ERROR;
			media_node_id id;
			err = msg->FindInt32("id", (int32 *)&id);
			if(err < B_OK){
				be_app->PostMessage(B_QUIT_REQUESTED);
				return;
			}
			new MixerWindow(BRect(64,64,594,364), id, "Mixer");
			break;
		}
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

bool
MixerApp::QuitRequested()
{
	return true;
}


int
main()
{
	MixerApp app;
	app.Run();
	return 0;
}
