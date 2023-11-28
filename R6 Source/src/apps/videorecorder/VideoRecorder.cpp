#include "VideoRecorder.h"
#include "CaptureSizeDialog.h"
#include "SettingsWindow.h"

#include <Alert.h>
#include <stdio.h>
#include <string.h>
#include <Button.h>
#include <TabView.h>
#include <MenuBar.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MediaDefs.h>
#include <MediaNode.h>
#include <scheduler.h>
#include <MediaTheme.h>
#include <TimeSource.h>
#include <MediaRoster.h>
#include <TextControl.h>
#include <Slider.h>
#include <CheckBox.h>
#include <TranslationKit.h>
#include <MediaEncoder.h>

const char * const kColorSpace[] = {
	"GRAY1",
	"GRAY8",
	"CMAP8",
	"RGB15",
	"RGBA15",
	"RGB16",
	"RGB24",
	"RGB32",
	"RGBA32",
	"RGB15_BIG",
	"RGBA15_BIG",
	"RGB16_BIG",
	"RGB24_BIG",
	"RGB32_BIG",
	"RGBA32_BIG",
	"YCbCr411",
	"YCbCr420",
	"YCbCr422",
	"YCbCr444",
	"YUV411",
	"YUV420",
	"YUV422",
	"YUV444",
	"YUV9",
	"YUV12",
	"UVL24",
	"UVL32",
	"UVLA32",
	"LAB24",
	"LAB32",
	"LABA32",
	"HSI24",
	"HSI32",
	"HSIA32",
	"HSV24",
	"HSV32",
	"HSVA32",
	"HLS24",
	"HLS32",
	"HLSA32",
	"CMY24",
	"CMY32",
	"CMYA32",
	"CMYK32",
	0
};

const color_space kColorSpaceValue[] = {
	B_GRAY1,
	B_GRAY8,
	B_CMAP8,
	B_RGB15,
	B_RGBA15,
	B_RGB16,
	B_RGB24,
	B_RGB32,
	B_RGBA32,
	B_RGB15_BIG,
	B_RGBA15_BIG,
	B_RGB16_BIG,
	B_RGB24_BIG,
	B_RGB32_BIG,
	B_RGBA32_BIG,
	B_YCbCr411,
	B_YCbCr420,
	B_YCbCr422,
	B_YCbCr444,
	B_YUV411,
	B_YUV420,
	B_YUV422,
	B_YUV444,
	B_YUV9,
	B_YUV12,
	B_UVL24,
	B_UVL32,
	B_UVLA32,
	B_LAB24,
	B_LAB32,
	B_LABA32,
	B_HSI24,
	B_HSI32,
	B_HSIA32,
	B_HSV24,
	B_HSV32,
	B_HSVA32,
	B_HLS24,
	B_HLS32,
	B_HLSA32,
	B_CMY24,
	B_CMY32,
	B_CMYA32,
	B_CMYK32,
	B_NO_COLOR_SPACE
};



#define VIDEO_SIZE_X 320
#define VIDEO_SIZE_Y 240

//#define CONTROL_SIZE_Y 230
#define CONTROL_SIZE_Y 250

#define WINDOW_SIZE_X (VIDEO_SIZE_X + 80)
#define WINDOW_SIZE_Y (VIDEO_SIZE_Y + CONTROL_SIZE_Y)

#define WINDOW_OFFSET_X 28
#define WINDOW_OFFSET_Y 28

const int32 kBtnHeight = 20;
const int32 kBtnWidth = 60;
const int32 kBtnBuffer = 25;
const int32 kXBuffer = 10;
const int32 kYBuffer = 10;
const int32 kMenuHeight = 15;
const int32 kCheckBoxHeight = 10;
const int32 kButtonHeight = 15;
const int32 kSliderViewRectHeight = 40;

static void ErrorAlert(const char * message, status_t err);
static void WarnAlert(const char * message, status_t err);

// Quality Slider

class QualitySlider : public BSlider {
	public:
			QualitySlider(BRect frame, BMessage* message);
			~QualitySlider();
			
			//void DrawText();			
			virtual char *UpdateText() const;
	private:
		char string[64];
};

QualitySlider::QualitySlider(BRect frame, BMessage* message)
	: BSlider(frame, "Quality", "Encode Quality:", message, -1, 100, B_BLOCK_THUMB, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP)
{
}

QualitySlider::~QualitySlider()
{
}

char*
QualitySlider::UpdateText() const
{
	if (Value() < 0)
		sprintf((char *)string, "Unspecified");
	else
		sprintf((char *)string, "%ld%%", Value());
	return (char*)string;
}


class ColorMenuItem : public BMenuItem {
	public:
		ColorMenuItem(const char *label, BMessage *message, int weight)
			: BMenuItem(label, message)
			, fWeight(weight) {}
		void DrawContent();
	
		int fWeight;
};

void 
ColorMenuItem::DrawContent()
{
	BMenu *m = Menu();
	rgb_color save_color;
	if(fWeight != 0) {
		save_color = m->HighColor();
		rgb_color new_color;
		new_color.alpha = 255;
		if(fWeight < -1) {
			new_color.red = 255;
			new_color.green = 16;
			new_color.blue = 16;
		}
		else if(fWeight < 0) {
			new_color.red = 128;
			new_color.green = 48;
			new_color.blue = 48;
		}
		else /*if(fWeight > 0)*/ {
			new_color.red = 0;
			new_color.green = 128;
			new_color.blue = 0;
		}
		m->SetHighColor(new_color);
	}
	BMenuItem::DrawContent();
	if(fWeight != 0)
		m->SetHighColor(save_color);
}


//---------------------------------------------------------------
// The Application
//---------------------------------------------------------------

int
main() {
	VideoRecorder app;
	app.Run();
	return 0;	
}

//---------------------------------------------------------------

VideoRecorder::VideoRecorder() :
	BApplication("application/x-vnd.Be.VideoRecorder"),

	fMediaRoster(NULL),

	fVideoConsumer(NULL),

	have_audio(false),
	audio_ready(false),
	is_recording(false),

	fWindow(NULL),

	fPort(0),

	mVideoControlWindow(NULL),
	mAudioControlWindow(NULL)
{
}

//---------------------------------------------------------------

VideoRecorder::~VideoRecorder()
{
	//printf("VideoRecorder::~VideoRecorder\n");

	// release the video consumer node
	// the consumer node cleans up the window
	if (fVideoConsumer) {
		fMediaRoster->ReleaseNode(fVideoConsumer->Node());
		//fVideoConsumer->Release();
		fVideoConsumer = NULL;
	}
		
	//printf("VideoRecorder::~VideoRecorder - EXIT\n");
}

//---------------------------------------------------------------

void 
VideoRecorder::ReadyToRun()
{
	status_t err;

	/* find the media roster */
	fMediaRoster = BMediaRoster::Roster(&err);
	if(err != B_OK) {
		ErrorAlert("Can't find the media roster", err);
		return;
	}

	/* find the time source */
	err = fMediaRoster->GetTimeSource(&fTimeSourceNode);
	if (err != B_OK) {
		ErrorAlert("Can't get a time source", err);
		return;
	}
	/* find a video producer node */
	err = fMediaRoster->GetVideoInput(&fVideoProducerNode);
	if (err != B_OK) {
		ErrorAlert("Can't find a video input!", err);
		return;
	}
	
	FindVideoOutput();
	
	/* create the window for the app */
	fWindow = new VideoWindow(
		BRect(28, 28, 28 + (WINDOW_SIZE_X-1), 28 + (WINDOW_SIZE_Y-1)),
		(const char *)"VideoRecorder", B_TITLED_WINDOW,
		/*B_NOT_RESIZABLE | B_NOT_ZOOMABLE |*/ B_ASYNCHRONOUS_CONTROLS,
		&fPort, &fVideoProducerOut);
	fWindow->SetSizeLimits(300, 1280, 280, 1024);
	fWindow->SetZoomLimits(WINDOW_SIZE_X-1, WINDOW_SIZE_Y-1);
	/* set up the node connections */
	status_t status = SetUpNodes();
	if (status != B_OK)
	{
		ErrorAlert("Error setting up nodes", status);
		return;
	}
	//((VideoWindow*)fWindow)->fConsumer = fVideoConsumer->Node();

//printf("ApplyControls\n");
	fWindow->ApplyControls();
//printf("connect\n");
	
	fWindow->PostMessage(new BMessage(msg_restart));
	
	//ChangeConnection();
}

//---------------------------------------------------------------

bool 
VideoRecorder::QuitRequested()
{
	TearDownNodes();
	
	return true;
}

//---------------------------------------------------------------

void
VideoRecorder::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
			
		case msg_restart:
			//printf("VideoRecorder::MessageReceived msg_restart\n");
			ChangeVideoConnection();
			break;

		case msg_record:
			//printf("VideoRecorder::MessageReceived msg_record\n");
			if(B_OK==StartRecording())
				message->SendReply(msg_recording);
			break;

		case msg_finish:
			//printf("VideoRecorder::MessageReceived msg_finish\n");
			StopRecording();
			message->SendReply(msg_finished);
			break;

		case msg_video:
		{
			if (mVideoControlWindow) {
				mVideoControlWindow->Activate();
				break;
			}
			BParameterWeb * web = NULL;
			BView * view = NULL;
			media_node node = fVideoProducerNode;
			status_t err = fMediaRoster->GetParameterWebFor(node, &web);
			if ((err >= B_OK) &&
				(web != NULL))
			{
				view = BMediaTheme::ViewFor(web);
				mVideoControlWindow = new ControlWindow(
							BRect(2*WINDOW_OFFSET_X + WINDOW_SIZE_X, WINDOW_OFFSET_Y,
								2*WINDOW_OFFSET_X + WINDOW_SIZE_X + view->Bounds().right, WINDOW_OFFSET_Y + view->Bounds().bottom),
							view, node, false);
				fMediaRoster->StartWatching(BMessenger(NULL, mVideoControlWindow), node, B_MEDIA_WEB_CHANGED);
				mVideoControlWindow->Show();
			}
			break;
		}
		case msg_audiopref:
		{
			if (mAudioControlWindow) {
				mAudioControlWindow->Activate();
				break;
			}
			BParameterWeb * web = NULL;
			BView * view = NULL;
			media_node node = fAudioProducerNode;
			status_t err = fMediaRoster->GetParameterWebFor(node, &web);
			if ((err >= B_OK) &&
				(web != NULL))
			{
				view = BMediaTheme::ViewFor(web);
				mAudioControlWindow = new ControlWindow(
							BRect(2*WINDOW_OFFSET_X + WINDOW_SIZE_X, WINDOW_OFFSET_Y,
								2*WINDOW_OFFSET_X + WINDOW_SIZE_X + view->Bounds().right, WINDOW_OFFSET_Y + view->Bounds().bottom),
							view, node, true);
				fMediaRoster->StartWatching(BMessenger(NULL, mAudioControlWindow), node, B_MEDIA_WEB_CHANGED);
				mAudioControlWindow->Show();
			}
			break;
		}

		case msg_about:
			(new BAlert("About VideoRecorder", "VideoRecorder\n", "Close"))->Go();
			break;

		case msg_video_control_win:
			// our video control window is being asked to go away
			// set our pointer to NULL
			mVideoControlWindow = NULL;
			break;

		case msg_audio_control_win:
			// our audio control window is being asked to go away
			// set our pointer to NULL
			mAudioControlWindow = NULL;
			break;
		
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

//---------------------------------------------------------------

static
size_t get_bytes_per_row(color_space cs, uint32 width)
{
	size_t pixelChunk = 0;
	size_t rowAlignment = 0;
	size_t pixelsPerChunk = 0;
	size_t bytes_per_row;
	status_t err = get_pixel_size_for(cs, &pixelChunk,
		&rowAlignment, &pixelsPerChunk);
	if(err != B_NO_ERROR)
		return 0;
		
	bytes_per_row = (width * pixelChunk) / pixelsPerChunk;
	bytes_per_row +=  -bytes_per_row % rowAlignment;
	return bytes_per_row;
}		
//---------------------------------------------------------------

status_t 
VideoRecorder::StartRecording()
{
	status_t err;
	
	if(is_recording) {
		UpdateStatus("Recording already in progress");
		return B_ERROR;
	}

	if(fWindow->fRecordaudio) {
		if(!have_audio) {
			WarnAlert("No audio input", B_ERROR);
			return B_ERROR;
		}
		/* find free producer output */
		int32 count = 0;
		err = fMediaRoster->GetFreeOutputsFor(fAudioProducerNode, &fAudioProducerOut, 1,
		                                     &count, B_MEDIA_RAW_AUDIO);
		if(err != B_OK || count < 1) {
			UpdateStatus("Could not get free outputs from audio input node");
			if(err == B_NO_ERROR)
				err = B_ERROR;
			return err;
		}
	
		/* find free consumer input */
		count = 0;
		err = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(),
		                                    &fAudioConsumerIn, 1, &count,
		                                    B_MEDIA_RAW_AUDIO);
		if (err != B_OK || count < 1) {
			UpdateStatus("Cound not get free input for audio");
			if(err == B_NO_ERROR)
				err = B_ERROR;
			return err;
		}
	
		/* Connect The Nodes!!! */
		media_format format;
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;
		
		/* connect producer to consumer */
		printf("connect audio\n");
		err = fMediaRoster->Connect(fAudioProducerOut.source,
		                            fAudioConsumerIn.destination,
		                            &format, &fAudioProducerOut, &fAudioConsumerIn);
		if(err != B_OK) {
			UpdateStatus("Cound not connect audio");
			return err;
		}
	
		/* start the nodes */
		err = fMediaRoster->StartNode(fAudioProducerNode, 0 /* now ??? */ );
		if(err != B_OK) {
			UpdateStatus("Can't start the audio source");
			return err;
		}
		audio_ready = true;
		printf("audio ready\n");
	}
	printf("Preroll consumer\n");
	err = fMediaRoster->PrerollNode(fVideoConsumer->Node());
	if(err != B_OK) {
		UpdateStatus("Can't preroll consumer");
		StopRecording();
		return err;
	}
	printf("Consumer prerolled\n");
	err = fVideoConsumer->PrerollStatus;
	if(err != B_OK) {
		StopRecording();
		return err;
	}
	
	err = fMediaRoster->StartNode(fVideoConsumer->Node(), 0 /* now ??? */ );
	if(err != B_OK) {
		UpdateStatus("Can't start the consumer");
		StopRecording();
		return err;
	}
	is_recording = true;
	return B_NO_ERROR;
}

status_t 
VideoRecorder::StopRecording()
{
	status_t err = B_ERROR;

	if(is_recording) {
		printf("Stop recording\n");
		err = fMediaRoster->StopNode(fVideoConsumer->Node(), 0, true);
		if (err < B_OK) printf("stop consumer: %s\n", strerror(err));
		is_recording = false;
		printf("Recording stopped\n");
	}

	if(audio_ready) {
		printf("VideoWindow::DisonnectAudio()\n");
		err = fMediaRoster->Disconnect(fAudioProducerOut.node.node,
		                               fAudioProducerOut.source,
		                               fAudioConsumerIn.node.node,
		                               fAudioConsumerIn.destination);
		printf("VideoWindow::DisonnectAudio() done\n");
		audio_ready = false;
	}
	if(video_settings_changed)
		ChangeVideoConnection();
	return err;
}


status_t 
VideoRecorder::FindVideoOutput()
{
	status_t err;
	/* find free producer output */
	int32 cnt = 0;
	err = fMediaRoster->GetFreeOutputsFor(fVideoProducerNode,
	                                      &fVideoProducerOut, 1, &cnt,
	                                      B_MEDIA_RAW_VIDEO);
	if(err != B_OK || cnt < 1) {
		err = fMediaRoster->GetFreeOutputsFor(fVideoProducerNode,
	                                      &fVideoProducerOut, 1, &cnt,
	                                      B_MEDIA_ENCODED_VIDEO);
	}
	if(err != B_OK || cnt < 1) {
		WarnAlert("Can't find an available video stream", err);
		if(err != B_NO_ERROR)
			err = B_ERROR;
		return err;
	}

	return B_NO_ERROR;
}

// assumes fProducerOut is a media_output
//			fConsumerIn is a media_input

status_t 
VideoRecorder::ChangeVideoConnection()
{
	status_t err;
	if(is_recording) {
		video_settings_changed = true;
		return B_ERROR;
	}

	video_settings_changed = false;
	err = fMediaRoster->Disconnect(fVideoProducerOut.node.node,
	                               fVideoProducerOut.source,
	                               fVideoConsumerIn.node.node,
	                               fVideoConsumerIn.destination);
	
	err = FindVideoOutput();
	if(err != B_OK) {
		return err;
	}

	/* find free consumer input */
	int32 cnt = 0;
	err = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(),
	                                     &fVideoConsumerIn, 1, &cnt,
	                                     B_MEDIA_RAW_VIDEO);
	if(err != B_OK || cnt < 1)
		err = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(),
	                                     &fVideoConsumerIn, 1, &cnt,
	                                     B_MEDIA_ENCODED_VIDEO);
	if(err != B_OK || cnt < 1) {
		WarnAlert("Can't find an available connection to the video window", err);
		if(err != B_NO_ERROR)
			err = B_ERROR;
		return err;
	}

	/* Connect The Nodes!!! */
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	
	///writer_msg_info WriterInfo = fWindow->fWriterInfo;
	//media_raw_video_format vid_format = 
	//	{ 0, 1, 0, WriterInfo.height-1, B_VIDEO_TOP_LEFT_RIGHT, 1, 1,
	//	  {WriterInfo.colorspace, WriterInfo.width, WriterInfo.height,
	//	   get_bytes_per_row(WriterInfo.colorspace, WriterInfo.width), 0, 0}};

	media_raw_video_format vid_format = 
		{ 0, 0, /* field_rate, interlace */
		  0, 0, /* first_active, last_active */
		  B_VIDEO_TOP_LEFT_RIGHT, 0, 0, /* orientation, pixel_width_aspect, pixel_height_aspect */
		  fWindow->fDisplay /* display */ };

	format.u.raw_video = vid_format; 

	if (fVideoProducerOut.format.type == B_MEDIA_ENCODED_VIDEO) {
		format.type = B_MEDIA_ENCODED_VIDEO;
		format.u.encoded_video = media_encoded_video_format::wildcard;
		format.u.encoded_video.output = vid_format;
	}
	
	/* connect producer to consumer */
	err = fMediaRoster->Connect(fVideoProducerOut.source,
	                            fVideoConsumerIn.destination, &format,
	                            &fVideoProducerOut, &fVideoConsumerIn);
	if (err != B_OK) {
		WarnAlert("Can't connect the video source to the video window", err);
		return err;
	}
	fWindow->fVideoFormat = format;

	return B_NO_ERROR;
}

//---------------------------------------------------------------

status_t 
VideoRecorder::SetUpNodes()
{
	status_t status = B_OK;

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer("VideoRecorder", fWindow->VideoView(), fWindow->StatusLine(), NULL, 0);
	if (!fVideoConsumer) {
		ErrorAlert("Can't create a video window", B_ERROR);
		return B_ERROR;
	}
	
	/* register the node */
	status = fMediaRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		ErrorAlert("Can't register the video window", status);
		return status;
	}
	fPort = fVideoConsumer->ControlPort();
	
	/* set time sources */
	status = fMediaRoster->SetTimeSourceFor(fVideoProducerNode.node, fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video source", status);
		return status;
	}
	
	status = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(), fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video window", status);
		return status;
	}
	

	/* start the nodes */
	
	BTimeSource *timeSource = fMediaRoster->MakeTimeSourceFor(fVideoProducerNode);
	if(timeSource == NULL) {
		ErrorAlert("MakeTimeSourceFor failed", B_ERROR);
		return B_ERROR;
	}
	
	bool running = timeSource->IsRunning();
	
	/* workaround for people without sound cards */
	/* because the system time source won't be running */
	bigtime_t real = BTimeSource::RealTime();
	if (!running)
	{
		status = fMediaRoster->StartTimeSource(fTimeSourceNode, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot start time source!", status);
			return status;
		}
		status = fMediaRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot seek time source!", status);
			return status;
		}
	}
	/* figure out what recording delay to use */
	bigtime_t latency = 0;
	status = fMediaRoster->GetLatencyFor(fVideoProducerNode, &latency);
	status = fMediaRoster->SetProducerRunModeDelay(fVideoProducerNode, latency);

	bigtime_t initLatency = 0;
	fMediaRoster->GetInitialLatencyFor(fVideoProducerNode, &initLatency);
	initLatency += estimate_max_scheduling_latency();
	
	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency + initLatency);
	timeSource->Release();
	
	/* start the nodes */
	status = fMediaRoster->StartNode(fVideoProducerNode, perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video source", status);
		return status;
	}
//	status = fMediaRoster->StartNode(fVideoConsumer->Node(), perf);
//	if (status != B_OK) {
//		ErrorAlert("Can't start the video window", status);
//		return status;
//	}

	/* find Audio input */

	status = fMediaRoster->GetAudioInput(&fAudioProducerNode);
	if(status != B_OK ) {
		WarnAlert("Cound not find audio input", status);
		have_audio = false;
	}
	else {
		have_audio = true;
		status = fMediaRoster->SetTimeSourceFor(fAudioProducerNode.node, fTimeSourceNode.node);
		if (status != B_OK) {
			WarnAlert("Can't set the timesource for the audio source", status);
		}
	}
	
	return status;
}

//---------------------------------------------------------------

void 
VideoRecorder::TearDownNodes()
{
	if (!fMediaRoster)
		return;
	
	if (fVideoConsumer)
	{
		/* stop */	
		//printf("stopping nodes!\n");
		status_t err = B_OK;
		//err = fMediaRoster->StopNode(fProducerNode, 0, true);
		//if (err < B_OK) printf("stop producer: %s\n", strerror(err));
		err = fMediaRoster->StopNode(fVideoConsumer->Node(), 0, true);
		if (err < B_OK) printf("stop consumer: %s\n", strerror(err));
	
		/* disconnect */
		err = fMediaRoster->Disconnect(fVideoProducerOut.node.node,
		                               fVideoProducerOut.source,
		                               fVideoConsumerIn.node.node,
		                               fVideoConsumerIn.destination);
		if (err < B_OK) printf("disconnect: %s\n", strerror(err));
								
		err = fMediaRoster->ReleaseNode(fVideoProducerNode);
		if (err < B_OK) printf("release producer: %s\n", strerror(err));
		err = fMediaRoster->ReleaseNode(fVideoConsumer->Node());	
		if (err < B_OK) printf("release consumer: %s\n", strerror(err));
		fVideoConsumer = NULL;
	}
	//printf("done tearing down nodes\n");
}


void
VideoRecorder::UpdateStatus(const char *status)
{
	//printf("FTP STATUS: %s\n",status);
	if (fWindow->Lock())
	{
		fWindow->UpdateStatus(status);
		fWindow->Unlock();
	}
}

//---------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------

static void
ErrorAlert(const char * message, status_t err)
{
	char msg[256];
	sprintf(msg, "%s\n%s [%lx]", message, strerror(err), err);
	(new BAlert("", msg, "Quit"))->Go();
	be_app->PostMessage(B_QUIT_REQUESTED);
}

static void
WarnAlert(const char * message, status_t err)
{
	char msg[256];
	sprintf(msg, "%s\n%s [%lx]", message, strerror(err), err);
	(new BAlert("", msg, "OK"))->Go();
}

//---------------------------------------------------------------

static bool
GetNextColorMenuItem(int *cookie, BMenu *menu, ColorMenuItem **mi,
                     BMessage **message)
{
	if(menu == NULL)
		return false;
	while(*cookie < menu->CountItems()) {
		*mi = dynamic_cast<ColorMenuItem *>(menu->ItemAt(*cookie));
		(*cookie)++;
		if(*mi == NULL) {
			printf("bad menuitem at %d, not a ColorMenuItem\n", *cookie-1);
			continue;
		}

		*message = (*mi)->Message();
		if(*message == NULL) {
			printf("bad menuitem at %d, no message\n", *cookie-1);
			continue;
		}
		return true;
	}
	return false;
}

static bool
TestFormat(BMediaEncoder *encoder, const media_format *format,
           media_format *format_used, /*media_format *output_format,*/
           const media_file_format *mfi)
{
	media_format output_format;
	*format_used = *format;
	if(encoder->SetFormat(format_used, &output_format) != B_NO_ERROR) {
		printf("bad encoder, set format failed\n");
		return false;
	}
	else {
		if(format->Matches(format_used))
			return does_file_accept_format(mfi, &output_format);
		else
			return false;
	}
}

void 
VideoWindow::UpdateMenus()
{
printf("VideoWindow::UpdateMenus\n");
	BMediaBufferEncoder encoder;
	media_format format_used, default_format, output_format;
	media_format format;
	int cookie;

	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video = media_raw_video_format::wildcard;
	format.u.raw_video.display = fDisplay;
	
	cookie = 0;
	ColorMenuItem *mi;
	BMessage *message;
	while(GetNextColorMenuItem(&cookie, fVideoEncoderMenu, &mi, &message)) {
		const media_codec_info *ei;
		ssize_t datasize;

		if(message->FindData("ei", B_RAW_TYPE, (const void**)&ei, &datasize) != B_NO_ERROR) {
			printf("bad menuitem at %d, no mfi\n", cookie-1);
			continue;
		}
		if(datasize != sizeof(media_codec_info)) {
			printf("bad menuitem at %d, bad ei size\n", cookie-1);
			continue;
		}
		if(encoder.SetTo(ei) != B_NO_ERROR) {
			printf("bad encoder at %d\n", cookie-1);
			mi->fWeight = -2;
			continue;
		}
		format_used = format;
		if(encoder.SetFormat(&format_used, &output_format) != B_NO_ERROR) {
			printf("bad encoder at %d, set format failed\n", cookie-1);
			mi->fWeight = -2;
			continue;
		}

		if(fVideoFormat.type == B_MEDIA_ENCODED_VIDEO) {
			if(fVideoFormat.Matches(&output_format)) {
				mi->fWeight = 1;
			}
			else {
				mi->fWeight = -2;
			}
		}
		else {
			if(!format.Matches(&format_used)) {
				mi->fWeight = -2;
				continue;
			}
			if(!does_file_accept_format(&fWriterInfo.config.fileformat,
			                            &output_format)) {
				mi->fWeight = -2;
				continue;
			}
			mi->fWeight = 0;
		}
	}

	bool current_format_good = true;
	if(encoder.SetTo(&fWriterInfo.config.videoencoder) != B_NO_ERROR) {
		printf("bad encoder selected\n");
		current_format_good = false;
	}
	default_format.type = B_MEDIA_RAW_VIDEO;
	default_format.u.raw_video = media_raw_video_format::wildcard;
	if(encoder.SetFormat(&default_format, &output_format) != B_NO_ERROR) {
		printf("bad encoder selected, set format failed\n");
	}
	format_used = format;
	if(encoder.SetFormat(&format_used, &output_format) != B_NO_ERROR) {
		printf("bad encoder selected, set format failed\n");
		current_format_good = false;
	}
	if(!format.Matches(&format_used)) {
		current_format_good = false;
	}
	cookie = 0;
	while(GetNextColorMenuItem(&cookie, fFileFormatMenu, &mi, &message)) {
		media_file_format	*mfi;
		ssize_t datasize;
		if(message->FindData("mfi", B_RAW_TYPE, (const void**)&mfi, &datasize) != B_NO_ERROR) {
			printf("bad menuitem at %d, no mfi\n", cookie-1);
			continue;
		}
		if(datasize != sizeof(media_file_format)) {
			printf("bad menuitem at %d, bad mfi size\n", cookie-1);
			continue;
		}
		if(does_file_accept_format(mfi, &output_format)) {
			mi->fWeight = current_format_good ? 0 : -1;
		}
		else {
			mi->fWeight = -2;
		}
	}

	cookie = 0;
	while(GetNextColorMenuItem(&cookie, fColorSpaceMenu, &mi, &message)) {
		color_space cs;
		if(message->FindInt32("colorspace", (int32*)&cs) != B_NO_ERROR) {
			printf("bad menuitem at %d, no colorspace\n", cookie-1);
			continue;
		}
		format.u.raw_video.display.format = fDisplay.format;
		format.u.raw_video.display.format = cs;

		format_used = format;
		if(encoder.SetFormat(&format_used, &output_format) != B_NO_ERROR) {
			printf("bad encoder, set format failed\n");
			mi->fWeight = -2;
			continue;
		}

		if(format.Matches(&format_used)) {
			if(does_file_accept_format(&fWriterInfo.config.fileformat,
			                           &output_format)) {
				if(cs == default_format.u.raw_video.display.format)
					mi->fWeight = 1;
				else
					mi->fWeight = 0;
			}
			else 
				mi->fWeight = -1;
		}
		else {
			if(cs == format_used.u.raw_video.display.format)
				mi->fWeight = -1;
			else
				mi->fWeight = -2;
		}
	}

	cookie = 0;
	while(GetNextColorMenuItem(&cookie, fResolutionMenu, &mi, &message)) {
		uint32 width;
		uint32 height;
		if(message->FindInt32("width", (int32*)&width) != B_NO_ERROR) {
			printf("bad menuitem at %d, no width\n", cookie-1);
			continue;
		}
		if(message->FindInt32("height", (int32*)&height) != B_NO_ERROR) {
			printf("bad menuitem at %d, no height\n", cookie-1);
			continue;
		}
		if(width == 0 || height == 0) {
			// other menu.
			width = fDisplay.line_width;
			height = fDisplay.line_count;
		}
		format.u.raw_video.display = fDisplay;
		format.u.raw_video.display.line_width = width;
		format.u.raw_video.display.line_count = height;

		format_used = format;
		if(encoder.SetFormat(&format_used, &output_format) != B_NO_ERROR) {
			printf("bad encoder, set format failed\n");
			mi->fWeight = -2;
			continue;
		}
		if(format.Matches(&format_used)) {
			if(does_file_accept_format(&fWriterInfo.config.fileformat,
			                           &output_format)) {
				if(width == default_format.u.raw_video.display.line_width &&
				   height == default_format.u.raw_video.display.line_count)
					mi->fWeight = 1;
				else
					mi->fWeight = 0;
			}
			else
				mi->fWeight = -1;
		}
		else {
			if(width == format_used.u.raw_video.display.line_width &&
			   height == format_used.u.raw_video.display.line_count)
				mi->fWeight = -1;
			else
				mi->fWeight = -2;
		}
	}

	format.u.raw_video.display = fDisplay;

	if(fVideoFormat.type == B_MEDIA_ENCODED_VIDEO) {
		format_used = format;
		if(does_file_accept_format(&fWriterInfo.config.fileformat, &fVideoFormat)) {
			if(encoder.SetFormat(&format_used, &output_format) != B_NO_ERROR) {
				UpdateStatus("Selected encoder does not reflect actual data produced");
			}
			else {
				if(output_format.Matches(&fVideoFormat))
					UpdateStatus("Ready");
				else
					UpdateStatus("Selected encoder does not reflect actual data produced");
			}
		}
		else {
			UpdateStatus("Select a supported file format");
		}
	}
	else {
		if(TestFormat(&encoder, &format, &format_used,
			          &fWriterInfo.config.fileformat)) {
			UpdateStatus("Ready");
		}
		else {
			UpdateStatus("Select a supported format and encoder");
		}
	}
}

status_t 
AddFileformatItems( BMenu * intoMenu, const char *selected, media_file_format *activeformat,
                   media_video_display_info *display)
{
	media_file_format	mfi;
	int32				cookie = 0;
	media_format		format;
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video = media_raw_video_format::wildcard;
	format.u.raw_video.display = *display;

	intoMenu->RemoveItems(0, intoMenu->CountItems(), true);

	while(get_next_file_format(&cookie, &mfi) == B_OK) {
		//printf("got '%s': %x\n", mfi.pretty_name, mfi.capabilities);
		//printf("mime type: %s\n", mfi.mime_type);
		//printf("extension: %s\n", mfi.file_extension);
		if((mfi.capabilities & media_file_format::B_WRITABLE) &&
		   ((mfi.capabilities & media_file_format::B_KNOWS_RAW_VIDEO) ||
		    (mfi.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO))) {
			BMessage * itemmsg; 
			itemmsg = new BMessage(msg_format);
			itemmsg->AddString("short_name", mfi.short_name);
			itemmsg->AddData("mfi", B_RAW_TYPE, &mfi, sizeof(mfi));
			//BMenuItem *m = new BMenuItem(mfi.pretty_name, itemmsg);
			BMenuItem *m = new ColorMenuItem(mfi.pretty_name, itemmsg, -2);
			intoMenu->AddItem(m);
			if(strcmp(selected, mfi.pretty_name) == 0)
			{
				m->SetMarked(true);
				*activeformat=mfi;
			}
		}
	}
	return B_NO_ERROR;
}

status_t 
AddVideoEncoderItems(BMenu * intoMenu, media_video_display_info *display,
                media_file_format *mfi,
                media_codec_info *selected)
{
	media_format		format, video_format;
	media_codec_info    ei;
	int32				cookie = 0;
	intoMenu->RemoveItems(0, intoMenu->CountItems(), true);

	if(display == NULL || mfi == NULL)
		return B_BAD_VALUE;

	video_format.type = B_MEDIA_RAW_VIDEO;
	video_format.u.raw_video = media_raw_video_format::wildcard;

	format.type = B_MEDIA_RAW_VIDEO;
	
	media_raw_video_format vid_format = 
		{ 0, 0, /* field_rate, interlace */
		  0, display->line_count-1, /* first_active, last_active */
		  B_VIDEO_TOP_LEFT_RIGHT, 1, 1, /* orientation, pixel_width_aspect, pixel_height_aspect */
		  *display /* display */ };

	format.u.raw_video = vid_format;
	
	{
		BMenu *bar = intoMenu->Supermenu();
		if(bar) {
			BMenuItem *m = bar->ItemAt(0);
			m->SetLabel("Select Encoder");
		}
	}

	while (get_next_encoder(&cookie, mfi, &video_format, NULL, &ei, NULL, NULL) == B_OK) {
		BMessage * itemmsg; 
		itemmsg = new BMessage(msg_videncoder);
		itemmsg->AddString("short_name", ei.short_name);
		itemmsg->AddData("ei", B_RAW_TYPE, &ei, sizeof(ei));
		BMenuItem *m = new ColorMenuItem(ei.pretty_name, itemmsg, -2);
		intoMenu->AddItem(m);
		if(strcmp(selected->pretty_name, ei.pretty_name) == 0) {
			memcpy(selected, &ei, sizeof(ei));
			m->SetMarked(true);
		}
		
	}
	return B_NO_ERROR;
}

status_t
AddAudioEncoderItems(BMenu * intoMenu, 
                media_file_format *mfi,
                media_codec_info *selected)
{
	media_format		format, audio_format;
	media_codec_info    ei;
	int32				cookie = 0;
	intoMenu->RemoveItems(0, intoMenu->CountItems(), true);

	if(mfi == NULL)
		return B_BAD_VALUE;

	memset(&audio_format, 0, sizeof(audio_format));
	audio_format.type = B_MEDIA_RAW_AUDIO;
	audio_format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	audio_format.u.raw_audio.channel_count = 2;

	{
		BMenu *bar = intoMenu->Supermenu();
		if(bar) {
			BMenuItem *m = bar->ItemAt(0);
			m->SetLabel("Select Encoder");
		}
	}

	while (get_next_encoder(&cookie, mfi, &audio_format, NULL, &ei, NULL, NULL) == B_OK) {
		BMessage * itemmsg; 
		itemmsg = new BMessage(msg_audencoder);
		itemmsg->AddString("short_name", ei.short_name);
		itemmsg->AddData("ei", B_RAW_TYPE, &ei, sizeof(ei));
		BMenuItem *m = new ColorMenuItem(ei.pretty_name, itemmsg, 0);
		intoMenu->AddItem(m);
		if(strcmp(selected->pretty_name, ei.pretty_name) == 0) {
			memcpy(selected, &ei, sizeof(ei));
			m->SetMarked(true);
		}
		
	}
	return B_NO_ERROR;
}

//---------------------------------------------------------------
//  Video Window Class
//---------------------------------------------------------------

VideoWindow::VideoWindow(BRect frame, const char *title, window_type type,
                         uint32 flags, port_id * consumerport,
                         media_output *video_producer_out_ptr)
	: BWindow(frame,title,type,flags)
	, fPortPtr(consumerport)
	, fVideoProducerOutPtr(video_producer_out_ptr)
	, fView(NULL)
	, fVideoView(NULL)
{
	memset(&fWriterInfo, 0, sizeof(fWriterInfo));
	memset(&fDisplay, 0, sizeof(fDisplay));
	fRecordaudio = false;

	fVideoFormat.type = fVideoProducerOutPtr->format.type;

	SetUpSettings("videorecorder", "");	

	BMenuBar* menuBar = new BMenuBar(BRect(0,0,0,0), "menu bar");
	AddChild(menuBar);
	
	BMenuItem* menuItem;
	BMenu* menu = new BMenu("File");

	menuItem = new BMenuItem("Video Preferences"B_UTF8_ELLIPSIS, new BMessage(msg_video), 'P');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuItem = new BMenuItem("Audio Preferences"B_UTF8_ELLIPSIS, new BMessage(msg_audiopref), 'A');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuItem = new BMenuItem("Settings"B_UTF8_ELLIPSIS, new BMessage(msg_setup), 'S');
	menuItem->SetTarget(this);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem("About VideoRecorder"B_UTF8_ELLIPSIS, new BMessage(msg_about));
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menu->AddSeparatorItem();

	menuItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	menuItem->SetTarget(be_app);
	menu->AddItem(menuItem);

	menuBar->AddItem(menu);

	/* give it a gray background view */
	BRect aRect;
	aRect = Frame();
	aRect.OffsetTo(B_ORIGIN);
	aRect.top += menuBar->Frame().Height() + 1;
	fView	= new BView(aRect, "Background View", B_FOLLOW_ALL, B_WILL_DRAW);
	fView->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	AddChild(fView);
	
	/* add some controls */
	BuildCaptureControls(fView);
	
	/* add another view to hold the video image */
	aRect = BRect(0, 0, VIDEO_SIZE_X - 1, VIDEO_SIZE_Y - 1);
	aRect.OffsetBy((WINDOW_SIZE_X - VIDEO_SIZE_X)/2, kYBuffer);
	
	fVideoView	= new BView(aRect, "Video View", B_FOLLOW_ALL, B_WILL_DRAW);
	fView->AddChild(fVideoView);

	Show();
}

//---------------------------------------------------------------

VideoWindow::~VideoWindow()
{
	QuitSettings();
}

//---------------------------------------------------------------

bool
VideoWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

//---------------------------------------------------------------

void
VideoWindow::MessageReceived(BMessage *message)
{
	BControl	*p;
	bool		update_menus = false;
	bool		init = false;

	p = NULL;
	message->FindPointer((const char *)"source",(void **)&p);

	message->FindBool("ApplyControls", &init);
	
	switch (message->what)
	{
		case msg_restart:
			be_app->PostMessage(message);
			return;

		case msg_record:
			be_app->PostMessage(msg_record,NULL,this);
			return;

		case msg_recording:
			fStartRecord->SetEnabled(false);
			fStopRecord->SetEnabled(true);
			break;

		case msg_finish:
			be_app->PostMessage(msg_finish,NULL,this);
			return;
		
		case msg_finished:
			fStartRecord->SetEnabled(true);
			fStopRecord->SetEnabled(false);
			break;

		case msg_filename:
			if (p != NULL)
			{
				strncpy(fWriterInfo.config.filename, ((BTextControl *)p)->Text(), 63);
				//printf("file is '%s'\n", fWriterInfo.config.filename);
			}
			break;

		case msg_format: {
			//const char *short_name;
			const void *data;
			ssize_t	datasize;
			//if(message->FindString("short_name", &short_name) == B_NO_ERROR) {
			if(message->FindData("mfi", B_RAW_TYPE, &data, &datasize) == B_NO_ERROR) {
				if(datasize != sizeof(media_file_format)) {
					printf("bad msg_format message\n");
				}
				else {
					update_menus = true;
					memcpy(&fWriterInfo.config.fileformat, data, sizeof(media_file_format));

					AddVideoEncoderItems(fVideoEncoderMenu, &fDisplay,
					                &fWriterInfo.config.fileformat,
					                &fWriterInfo.config.videoencoder);
					fVideoEncoderMenu->SetTargetForItems(this);

					AddAudioEncoderItems(fAudioEncoderMenu, 
					                &fWriterInfo.config.fileformat,
					                &fWriterInfo.config.audioencoder);
					fAudioEncoderMenu->SetTargetForItems(this);
					//printf("fileformat is '%s'\n", fWriterInfo.config.fileformat.pretty_name);
				}
			}
			else
				printf("bad fileformat message\n");
		} break;

		case msg_videncoder: {
			const void *data;
			ssize_t	datasize;
			//if(message->FindString("short_name", &short_name) == B_NO_ERROR) {
			if(message->FindData("ei", B_RAW_TYPE, &data, &datasize) == B_NO_ERROR) {
				if(datasize != sizeof(media_codec_info)) {
					printf("bad msg_videncoder message\n");
				}
				else {
					update_menus = true;
					memcpy(&fWriterInfo.config.videoencoder, data, sizeof(media_codec_info));
					//printf("encoder is '%s'\n", fWriterInfo.config.encoder.pretty_name);
				}
			}
			else
				printf("bad encoder message\n");
		} break;

		case msg_audencoder: {
			const void *data;
			ssize_t	datasize;
			//if(message->FindString("short_name", &short_name) == B_NO_ERROR) {
			if(message->FindData("ei", B_RAW_TYPE, &data, &datasize) == B_NO_ERROR) {
				if(datasize != sizeof(media_codec_info)) {
					printf("bad msg_audencoder message\n");
				}
				else {
					update_menus = true;
					memcpy(&fWriterInfo.config.audioencoder, data, sizeof(media_codec_info));
					//printf("encoder is '%s'\n", fWriterInfo.config.encoder.pretty_name);
				}
			}
			else
				printf("bad encoder message\n");
		} break;

		case msg_quality:
			if (p != NULL)
			{
				int value = fQualitySlider->Value();
				fWriterInfo.config.quality = value/100.0;
				/*((BSlider *)p)->Value()/100.0;*/
				//if(value < 0)
				//	fQualitySlider->SetLabel("Quality: Encoder Default");
				//else {
				//	char label[64];
				//	sprintf(label, "Quality: %d%%", value);
				//	fQualitySlider->SetLabel(label);
				//}
				//printf("quality is '%.2f'\n", fWriterInfo.config.quality);
			}
			break;

		case msg_preview:
			if (p != NULL)
			{
				fWriterInfo.config.preview = ((BCheckBox *)p)->Value();
				//printf("preview is '%d'\n", fWriterInfo.config.preview);
			}
			break;
			
		case msg_audio:
			if (p != NULL)
			{
				fRecordaudio = ((BCheckBox *)p)->Value();
				//printf("audio is '%d'\n", fRecordaudio);
			}
			break;
			
		case msg_colorspace: {
			color_space cs;
			if(message->FindInt32("colorspace", (int32*)&cs) == B_NO_ERROR) {
				fDisplay.format = cs;
				update_menus = true;
				//printf("colorspace is %x\n", fDisplay.format);
				be_app->PostMessage(msg_restart);
			}
			else
				printf("bad colorspace message\n");
		} break;

		case msg_capturesize: {
			int32 width, height;
			if(message->FindInt32("width", &width) == B_NO_ERROR &&
			   message->FindInt32("height", &height) == B_NO_ERROR ) {
				if(width == 0 || height == 0) {
					if(!init) {
						CaptureSizeDialog *t = new CaptureSizeDialog(
							this, fDisplay.line_width, fDisplay.line_count);
						t->Show();
					}
				}
				else {
					fDisplay.line_width = width;
					fDisplay.line_count = height;
					if(!init) {
						update_menus = true;
						bool other;
						if(message->FindBool("other", &other) == B_OK) {
							char menutext[256];
							sprintf(menutext, "%ldx%ld", width, height);
							BMenuItem* mitem = fResolutionMenu->ItemAt(fResolutionMenu->CountItems()-1);
							mitem->SetLabel(menutext);
							mitem->SetMarked(true);
							sprintf(menutext, "%ldx%ld/Other...", width, height);
							mitem->SetLabel(menutext);
						}
						else {
							BMenuItem* mitem = fResolutionMenu->ItemAt(fResolutionMenu->CountItems()-1);
							mitem->SetLabel("Other...");
						}
						//printf("capturesize is %d*%d\n", width, height);
						be_app->PostMessage(msg_restart);
					}
				}
			}
			else
				printf("bad capturesize message\n");
		} break;

		case msg_capture_interval: {
			int32 interval;
			if(message->FindInt32("interval", &interval) == B_NO_ERROR)
				fWriterInfo.config.interval = interval;
			else
				printf("bad msg_capture_interval message\n");
		} break;
		
		case msg_setup:
			(new SettingsWindow(this, fAudioJitterSetting))->Show();

		case msg_setup_done:
			fWriterInfo.config.audio_jitter = fAudioJitterSetting->Value() / 1000000.0;
			break;

		default:
			BWindow::MessageReceived(message);
			return;
	}

	if(update_menus) {
		UpdateMenus();
	}

	//if (*fPortPtr)
	//	write_port(*fPortPtr, FTP_INFO, (void *)&fFtpInfo, sizeof(ftp_msg_info));
	if (*fPortPtr)
		write_port(*fPortPtr, WRITER_INFO, (void *)&fWriterInfo, sizeof(writer_msg_info));

}

//---------------------------------------------------------------

BView *
VideoWindow::VideoView()
{
	return fVideoView;
}

//---------------------------------------------------------------

BStringView *
VideoWindow::StatusLine()
{
	return fStatusLine;
}

void 
VideoWindow::UpdateStatus(const char *text)
{
	fStatusLine->SetText(text);
}


//---------------------------------------------------------------

void
VideoWindow::BuildCaptureControls(BView *theView)
{
	BRect aFrame, bFrame, theFrame;
	BBox *box;
	const int addright = 30;

	theFrame = theView->Bounds();
	theFrame.top += VIDEO_SIZE_Y + 2*kYBuffer + 40;
	theFrame.left += kXBuffer;
	theFrame.right -= (WINDOW_SIZE_X/2 + 5 + addright);
	theFrame.bottom -= kXBuffer;
	
	// Capture controls

	box = new BBox( theFrame, "Capture Controls", B_FOLLOW_BOTTOM);
	box->SetLabel("Capture Controls");
	fCaptureSetupBox = box;
	theView->AddChild(box);
	
	aFrame = box->Bounds();
	aFrame.InsetBy(kXBuffer,kYBuffer);	
	aFrame.top += kYBuffer/2;
	aFrame.bottom = aFrame.top + kButtonHeight;
	
	bFrame = aFrame;
	bFrame.right -= aFrame.Width() / 2;
	fStartRecord = new BButton(bFrame, "StartRecord", "Record", new BMessage(msg_record));
	box->AddChild(fStartRecord);
	
	bFrame.left += aFrame.Width() / 2;
	bFrame.right += aFrame.Width() / 2;

	fStopRecord = new BButton(bFrame, "StopRecord", "Stop", new BMessage(msg_finish));
	box->AddChild(fStopRecord);
	fStopRecord->SetEnabled(false);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kCheckBoxHeight;
	
	fPreviewCheckBox = new BCheckBox(aFrame, "Preview", "Preview While Recording",
	                             new BMessage(msg_preview));
	fPreviewCheckBox->SetTarget(this);
	fPreviewCheckBox->SetValue(fPreviewSetting->Value());
	box->AddChild(fPreviewCheckBox);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kCheckBoxHeight;

	fAudioCheckBox = new BCheckBox(aFrame, "Audio", "Record Audio",
	                             new BMessage(msg_audio));
	fAudioCheckBox->SetTarget(this);
	fAudioCheckBox->SetValue(fAudioSetting->Value());
	box->AddChild(fAudioCheckBox);
	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;

	if(fVideoFormat.type == B_MEDIA_RAW_VIDEO) {
		fColorSpaceMenu = new BPopUpMenu("Color Space Menu");
		media_format format;
		for(int i = 0; kColorSpace[i] != 0; i++) {
			format.type = B_MEDIA_RAW_VIDEO;
			format.u.raw_video = media_raw_video_format::wildcard;
			format.u.raw_video.display.format = kColorSpaceValue[i];
			if(BMediaRoster::Roster()->GetFormatFor(*fVideoProducerOutPtr,
			                                        &format) == B_NO_ERROR) {
				BMessage *msg = new BMessage(msg_colorspace);
				msg->AddInt32("colorspace", kColorSpaceValue[i]);
				fColorSpaceMenu->AddItem(new ColorMenuItem(kColorSpace[i], msg, 0));
			}
		}
		fColorSpaceMenu->SetTargetForItems(this);
		{
			BMenuItem *m = fColorSpaceMenu->FindItem(fColorSpaceSetting->Value());
			if(m == NULL) {
				printf("no item matching %s\n", fColorSpaceSetting->Value());
			}
			else {
				m->SetMarked(true);
			}
		}
		fColorSpaceSelector =
			new BMenuField(aFrame, "ColorSpace", "ColorSpace:", fColorSpaceMenu);
		//fColorSpaceSelector->SetDivider(fColorSpaceSelector->Divider() - 30);
		box->AddChild(fColorSpaceSelector);
		
		aFrame.top = aFrame.bottom + kYBuffer;
		aFrame.bottom = aFrame.top + kMenuHeight;
		
	}
	else {
		fColorSpaceMenu = NULL;
		fColorSpaceSelector = NULL;
	}
	{
		bool first_item = true;
		BMenuItem *m;
		media_format format;
		media_format format_wildcard;
		media_video_display_info *display;
		if(fVideoFormat.type == B_MEDIA_ENCODED_VIDEO) {
			format_wildcard.type = B_MEDIA_ENCODED_VIDEO;
			format_wildcard.u.encoded_video = media_encoded_video_format::wildcard;
			display = &format.u.encoded_video.output.display;
		}
		else {
			format_wildcard.type = B_MEDIA_RAW_VIDEO;
			format_wildcard.u.raw_video = media_raw_video_format::wildcard;
			display = &format.u.raw_video.display;
		}
		fResolutionMenu = new BPopUpMenu("Capture Size");
		for(int i = 0; kResolution[i] != 0; i++) {
			format = format_wildcard;
			display->line_width = kResolutionValues[i].width;
			display->line_count = kResolutionValues[i].height;
			if(BMediaRoster::Roster()->GetFormatFor(*fVideoProducerOutPtr,
			                                        &format) == B_NO_ERROR) {
				BMessage *msg = new BMessage(msg_capturesize);
				msg->AddInt32("width", kResolutionValues[i].width);
				msg->AddInt32("height", kResolutionValues[i].height);
				m = new ColorMenuItem(kResolution[i], msg, 0);
				fResolutionMenu->AddItem(m);
				if(first_item || (kResolutionValues[i].width == 320 &&
				                  kResolutionValues[i].height == 240)) {
					m->SetMarked(true);
					first_item = false;
					printf("SetMarked\n");
				}
			}
		}
		if(fResolutionMenu->CountItems() == 1) {
			m = fResolutionMenu->ItemAt(0);
			char menutext[256];
			sprintf(menutext, "%ldx%ld", display->line_width, display->line_count);
			m->SetLabel(menutext);
			m->SetMarked(true);
			printf("SetMarked one item only\n");
			sprintf(menutext, "%ldx%ld/Other...", display->line_width, display->line_count);
			m->SetLabel(menutext);
			fDisplay.line_width = display->line_width;
			fDisplay.line_count = display->line_count;
		}
		fResolutionMenu->SetTargetForItems(this);
		{
			m = fResolutionMenu->FindItem(fResolutionSetting->Value());
			if(m == NULL) {
				int width, height;
				if(sscanf(fResolutionSetting->Value(), "%dx%d", &width, &height) != 2) {
					printf("no item matching %s\n", fResolutionSetting->Value());
				}
				else {
					format = format_wildcard;
					display->line_width = width;
					display->line_count = height;
					if(BMediaRoster::Roster()->GetFormatFor(*fVideoProducerOutPtr,
	                                        &format) == B_NO_ERROR) {
						m = fResolutionMenu->ItemAt(fResolutionMenu->CountItems()-1);
						char menutext[256];
						sprintf(menutext, "%dx%d", width, height);
						m->SetLabel(menutext);
						m->SetMarked(true);
					printf("SetMarked other\n");
						sprintf(menutext, "%dx%d/Other...", width, height);
						m->SetLabel(menutext);
						fDisplay.line_width = width;
						fDisplay.line_count = height;
					}
				}
			}
			else {
				m->SetMarked(true);
					printf("SetMarked setting\n");
			}
		}
		fResolutionSelector =
			new BMenuField(aFrame, "Capture Size", "Capture Size:", fResolutionMenu);
		//fResolutionSelector->SetDivider(fResolutionSelector->Divider() - 30);
	}
	
	box->AddChild(fResolutionSelector);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	{
		media_format format;
		float input_frame_rate = 1;
		if(fVideoFormat.type == B_MEDIA_ENCODED_VIDEO) {
			format.type = B_MEDIA_ENCODED_VIDEO;
			format.u.encoded_video = media_encoded_video_format::wildcard;
			if(BMediaRoster::Roster()->GetFormatFor(*fVideoProducerOutPtr,
			                                        &format) == B_NO_ERROR) {
				input_frame_rate = format.u.encoded_video.output.field_rate;
			}
		}
		else {
			format.type = B_MEDIA_RAW_VIDEO;
			format.u.raw_video = media_raw_video_format::wildcard;
			if(BMediaRoster::Roster()->GetFormatFor(*fVideoProducerOutPtr,
			                                        &format) == B_NO_ERROR) {
				input_frame_rate = format.u.raw_video.field_rate;
			}
		}

		int selected_interval = fCaptureRateSetting->Value();
		int intervals[] = { 1, 2, 3, 6, 1 };
		intervals[4] = (int)(input_frame_rate+0.5);

		fCaptureRateMenu = new BPopUpMenu("Capture Rate Menu");
		for(size_t i = 0; i < sizeof(intervals) / sizeof(int); i++) {
			BMessage *msg = new BMessage(msg_capture_interval);
			msg->AddInt32("interval", intervals[i]);
			char strbuf[64];
			sprintf(strbuf, "%g fps", input_frame_rate/intervals[i]);
			BMenuItem *m = new BMenuItem(strbuf, msg);
			fCaptureRateMenu->AddItem(m);
			if(i == 0 || intervals[i] == selected_interval) {
				m->SetMarked(true);
			}
		}
		fCaptureRateMenu->SetTargetForItems(this);
		fCaptureRateSelector = new BMenuField(aFrame, "Rate", "Rate:", fCaptureRateMenu);
		//fCaptureRateSelector->SetDivider(fCaptureRateSelector->Divider() - 30);
		box->AddChild(fCaptureRateSelector);
	}

	// Writer controls
	aFrame = theView->Bounds();
	aFrame.top += VIDEO_SIZE_Y + 2*kYBuffer + 40;
	aFrame.left += WINDOW_SIZE_X/2 + 5 - addright;
	aFrame.right -= kXBuffer;
	aFrame.bottom -= kYBuffer;

	float divider = be_plain_font->StringWidth("Video Encoder:") + 5;
		
	box = new BBox( aFrame, "Writer Setup", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Writer Setup");
	fWriterSetupBox = box;
	theView->AddChild(box);
	
	aFrame = box->Bounds();
	aFrame.InsetBy(kXBuffer,kYBuffer);	
	aFrame.top += kYBuffer/2;
	aFrame.bottom = aFrame.top + kMenuHeight;	
	aFrame.right = aFrame.left + 160+addright;
	
	fFileName = new BTextControl(aFrame, "File Name", "File Name:",
	                             fFilenameSetting->Value(),
	                             new BMessage(msg_filename),
	                             B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);

	fFileName->SetTarget(BMessenger(NULL, this));
	fFileName->SetDivider(divider);
	box->AddChild(fFileName);	

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;

	fFileFormatMenu = new BPopUpMenu("Select File Format");
	AddFileformatItems(fFileFormatMenu, fFileFormatSetting->Value(),
						&fWriterInfo.config.fileformat, &fDisplay);
	fFileFormatMenu->SetTargetForItems(this);
	fFileFormatSelector = new BMenuField(aFrame, "File Format", "File Format:",
	                     fFileFormatMenu, B_FOLLOW_TOP | B_FOLLOW_LEFT);
	fFileFormatSelector->SetDivider(divider);
	box->AddChild(fFileFormatSelector);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
// ===================================================
	fVideoEncoderMenu = new BPopUpMenu("Select Encoder");
	strcpy(fWriterInfo.config.videoencoder.pretty_name, fVideoEncoderSetting->Value());
	AddVideoEncoderItems(fVideoEncoderMenu, &fDisplay,
	                &fWriterInfo.config.fileformat,
	                &fWriterInfo.config.videoencoder);
	fVideoEncoderMenu->SetTargetForItems(this);

	fVideoEncoderSelector = new BMenuField(aFrame, "VidEncoder", "Video Encoder:",
	                                  fVideoEncoderMenu, B_FOLLOW_TOP | B_FOLLOW_LEFT);
	fVideoEncoderSelector->SetDivider(divider);
	box->AddChild(fVideoEncoderSelector);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;

// ===================================================

	fAudioEncoderMenu = new BPopUpMenu("Select Encoder");
	strcpy(fWriterInfo.config.audioencoder.pretty_name, fAudioEncoderSetting->Value());
	AddAudioEncoderItems(fAudioEncoderMenu, 
	                &fWriterInfo.config.fileformat,
	                &fWriterInfo.config.audioencoder);
	fAudioEncoderMenu->SetTargetForItems(this);

	fAudioEncoderSelector = new BMenuField(aFrame, "AudEncoder", "Audio Encoder:",
	                                  fAudioEncoderMenu, B_FOLLOW_TOP | B_FOLLOW_LEFT);
	fAudioEncoderSelector->SetDivider(divider);
	box->AddChild(fAudioEncoderSelector);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;

// ===================================================

	aFrame.top += 4;
	aFrame.bottom += 4;
	fQualitySlider = new QualitySlider(aFrame, new BMessage(msg_quality));
	fQualitySlider->SetTarget(this);
	fQualitySlider->SetValue(fQualitySetting->Value());
	box->AddChild(fQualitySlider);

	aFrame.top = aFrame.bottom + kYBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight;
	
	aFrame = theView->Bounds();
	aFrame.top += VIDEO_SIZE_Y + 2*kYBuffer;
	aFrame.left += kXBuffer;
	aFrame.right -= kXBuffer;
	aFrame.bottom = aFrame.top + kMenuHeight + 2*kYBuffer;
		
	fStatusBox = new BBox( aFrame, "Status", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT);
	fStatusBox->SetLabel("Status");
	theView->AddChild(fStatusBox);
	
	aFrame = fStatusBox->Bounds();
	aFrame.InsetBy(kXBuffer,kYBuffer);	
	
	fStatusLine = new BStringView(aFrame,"Status Line","Waiting ...");
	fStatusBox->AddChild(fStatusLine);
	fWriterInfo.config.audio_jitter = fAudioJitterSetting->Value() / 1000000.0;
}

//---------------------------------------------------------------

void
VideoWindow::ApplySelectedMenu(BMenu *menu)
{
	if(menu == NULL)
		return;
	BMenuItem *m;
	m = menu->FindMarked();
	if(m) {
		BMessage *msg = new BMessage(*m->Message());
		if(msg) {
			msg->AddBool("ApplyControls", true);
			PostMessage(msg);
		}
	}
}

void
VideoWindow::ApplyControls()
{
	// apply controls
	fFileName->Invoke();
	ApplySelectedMenu(fFileFormatMenu);
	ApplySelectedMenu(fCaptureRateMenu);
	ApplySelectedMenu(fResolutionMenu);
	ApplySelectedMenu(fColorSpaceMenu);
	ApplySelectedMenu(fVideoEncoderMenu);
	//fEncoderName->Invoke();
	fPreviewCheckBox->Invoke();
	fAudioCheckBox->Invoke();
	fQualitySlider->Invoke();
}

//---------------------------------------------------------------

void
VideoWindow::SetUpSettings(const char *filename, const char *dirname)
{
	fSettings = new Settings(filename, dirname);

	fSettings->Add(fFilenameSetting =
		new StringValueSetting("Filename", "video.avi",
		                       "filename expected", ""));

	fSettings->Add(fCaptureRateSetting =
		new ScalarValueSetting("CaptureInterval", 1, "capture interval value expected", "", 1, 30));

	fSettings->Add(fFileFormatSetting =
		new StringValueSetting("FileFormat", "", "fileformat expected", ""));

	fSettings->Add(fVideoEncoderSetting =
		new StringValueSetting("VideoEncoder", "", "encodername expected", ""));

	fSettings->Add(fAudioEncoderSetting =
		new StringValueSetting("AudioEncoder", "", "encodername expected", ""));

	fSettings->Add(fQualitySetting =
		new ScalarValueSetting("Quality", -1, "quality value expected", "", -1, 100));

	fSettings->Add(fColorSpaceSetting =
		new EnumeratedStringValueSetting("ColorSpace", "RGB32", kColorSpace,
		                                 "color space expected",
		                                 "unrecognized color space selected"));

	fSettings->Add(fResolutionSetting =
		new StringValueSetting("Resolution", "320x240", "resolution expected",
		                                 ""));

	fSettings->Add(fPreviewSetting = new BooleanValueSetting("Preview", true));
	fSettings->Add(fAudioSetting = new BooleanValueSetting("Audio", false));
	fSettings->Add(fAudioJitterSetting =
		new ScalarValueSetting("AudioJitter", 0, "microseconds expected", "", 0, 500000));

	fSettings->TryReadingSettings();
}

//---------------------------------------------------------------

void
VideoWindow::QuitSettings()
{
	BMenuItem *m;
	fFilenameSetting->ValueChanged(fFileName->Text());
	//fEncoderSetting->ValueChanged(fEncoderName->Text());
	fQualitySetting->ValueChanged(fQualitySlider->Value());
	fPreviewSetting->ValueChanged(fPreviewCheckBox->Value());
	fAudioSetting->ValueChanged(fAudioCheckBox->Value());
	//fAudioJitterSetting->ValueChanged(fAudioJitterBox->Value());
	m = fCaptureRateMenu->FindMarked();
	if(m) {
		int32 interval;
		BMessage *message = m->Message();
		if(message && message->FindInt32("interval", &interval) == B_NO_ERROR)
			fCaptureRateSetting->ValueChanged(interval);
	}
	m = fFileFormatMenu->FindMarked();
	if(m)
		fFileFormatSetting->ValueChanged(m->Label());

	m = fVideoEncoderMenu->FindMarked();
	if(m)
		fVideoEncoderSetting->ValueChanged(m->Label());

	m = fAudioEncoderMenu->FindMarked();
	if(m)
		fAudioEncoderSetting->ValueChanged(m->Label());

	m = fResolutionMenu->FindMarked();
	if(m)
		fResolutionSetting->ValueChanged(m->Label());
	
	if(fColorSpaceMenu) {
		m = fColorSpaceMenu->FindMarked();
		if(m)
			fColorSpaceSetting->ValueChanged(m->Label());
	}
	
	fSettings->SaveSettings();
	delete fSettings;
}

//---------------------------------------------------------------

ControlWindow::ControlWindow(const BRect & frame, BView * controls,
                             media_node node, bool audio)
	: BWindow(frame, audio ? "Audio Preferences" : "Video Preferences",
	          B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	, audio(audio)
{
	fView = controls;
	fNode = node;
		
	AddChild(fView);
}

//---------------------------------------------------------------

void
ControlWindow::MessageReceived(BMessage * message) 
{
	BParameterWeb * web = NULL;
	status_t err;
	
	switch (message->what)
	{
		case B_MEDIA_WEB_CHANGED:
		{
			// If this is a tab view, find out which tab 
			// is selected
			BTabView *tabView = dynamic_cast<BTabView*>(fView);
			int32 tabNum = -1;
			if (tabView)
				tabNum = tabView->Selection();

			RemoveChild(fView);
			delete fView;
			
			err = BMediaRoster::Roster()->GetParameterWebFor(fNode, &web);
			
			if ((err >= B_OK) &&
				(web != NULL))
			{
				fView = BMediaTheme::ViewFor(web);
				AddChild(fView);

				// Another tab view?  Restore previous selection
				if (tabNum > 0)
				{
					BTabView *newTabView = dynamic_cast<BTabView*>(fView);	
					if (newTabView)
						newTabView->Select(tabNum);
				}
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}

//---------------------------------------------------------------

bool 
ControlWindow::QuitRequested()
{
	if(audio)
		be_app->PostMessage(msg_audio_control_win);
	else
		be_app->PostMessage(msg_video_control_win);
	return true;
}




