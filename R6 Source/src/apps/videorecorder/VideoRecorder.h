/* VideoRecorder.h */

#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

#include <Box.h>
#include <Menu.h>
#include <string.h>
#include <Window.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <StringView.h>
#include <Application.h>
#include <TextControl.h>

#include <Slider.h>

#include "Settings.h"
#include "VideoConsumer.h"

enum {
	msg_capture_interval = 'capi',

	msg_format		= 'form',
	msg_videncoder	= 'venc',
	msg_audencoder	= 'aenc',
	msg_colorspace  = 'cspa',
	msg_capturesize = 'csiz',
	msg_quality		= 'qual',
	msg_preview		= 'prev',
	msg_audio		= 'audi',

	msg_filename	= 'file',
	
	msg_record		= 'rec ',
	msg_finish		= 'fin ',
	msg_recording	= 'rec_',
	msg_finished	= 'fin_',

	msg_about		= 'abut',
	msg_setup		= 'setp',
	msg_setup_done  = 'setd',
	msg_video		= 'vdeo',
	msg_audiopref	= 'audp',

	msg_restart		= 'rest',

	msg_video_control_win = 'vctl',
	msg_audio_control_win = 'actl'
};

const char * const kResolution[] = {
	"120x90",
	"160x120",
	"240x180",
	"320x240",
	"640x480",
	"720x480",
	"Other...",
	0
};

const struct {
	int width, height;
} kResolutionValues[] = {
	{120, 90},
	{160, 120},
	{240, 180},
	{320, 240},
	{640, 480},
	{720, 480},
	{0, 0},
	{0, 0}
};

class VideoWindow;

class VideoRecorder : public BApplication {
	public:
							VideoRecorder();
		virtual				~VideoRecorder();
		
		void				ReadyToRun();
		virtual bool		QuitRequested();
		virtual void		MessageReceived(
								BMessage *message);

		status_t			FindVideoOutput();
		status_t			ChangeVideoConnection();

		status_t			StartRecording();
		status_t			StopRecording();
	
	private:
		void				UpdateStatus(const char *text);

		status_t			SetUpNodes();
		void				TearDownNodes();
		
		BMediaRoster *		fMediaRoster; 

		media_node			fTimeSourceNode;
		media_node			fVideoProducerNode;

		VideoConsumer *		fVideoConsumer;
				
		media_output		fVideoProducerOut;
		media_input			fVideoConsumerIn;

		bool				have_audio;
		bool				audio_ready;
		bool				is_recording;
		media_node			fAudioProducerNode;
		media_output		fAudioProducerOut;
		media_input			fAudioConsumerIn;

		VideoWindow *		fWindow;
		
		port_id				fPort;
			
		BWindow 			*mVideoControlWindow;
		BWindow 			*mAudioControlWindow;
		bool				video_settings_changed;
};

class VideoWindow : public BWindow
{
public:
						VideoWindow (
							BRect frame,
							const char * title,
							window_type type,
							uint32 flags,
							port_id * consumerport,
							media_output *);
						~VideoWindow();

	virtual	bool		QuitRequested();
	virtual void		MessageReceived(
							BMessage *message);

	void 				ApplyControls();
							
	BView *				VideoView();
	BStringView *		StatusLine();
	
	void				UpdateStatus(const char *text);
	
	writer_msg_info		fWriterInfo;
	bool				fRecordaudio;
	media_video_display_info	fDisplay;
	media_format				fVideoFormat;

	//media_node			fConsumer;

private:
	void				BuildCaptureControls(
							BView *theView);
	void				UpdateMenus();

	void				SetUpSettings(
							const char *filename,
							const char *dirname);
	void				QuitSettings();

	void ApplySelectedMenu(BMenu *menu);
	
private:
		//media_node *		fProducer;
		port_id	*			fPortPtr;
		media_output *		fVideoProducerOutPtr;
		
		BView *				fView;
		BView *				fVideoView;
		
		BBox *				fCaptureSetupBox;
		BMenu *				fCaptureRateMenu;
		BMenuField *		fCaptureRateSelector;

		BMenu *				fColorSpaceMenu;
		BMenuField *		fColorSpaceSelector;

		BMenu *				fResolutionMenu;
		BMenuField *		fResolutionSelector;

		BBox *				fWriterSetupBox;
		BTextControl *		fFileName;
		BMenu *				fFileFormatMenu;
		BMenuField *		fFileFormatSelector;

		BMenu *				fVideoEncoderMenu;
		BMenuField *		fVideoEncoderSelector;

		BMenu *				fAudioEncoderMenu;
		BMenuField *		fAudioEncoderSelector;

		//BTextControl *		fEncoderName;

		BCheckBox *			fPreviewCheckBox;
		BCheckBox *			fAudioCheckBox;
		BSlider *			fQualitySlider;
		
		BButton *			fStartRecord;
		BButton *			fStopRecord;

		BBox *				fStatusBox;
		BStringView	*		fStatusLine;
		
		//ftp_msg_info		fFtpInfo;
		
		Settings *						fSettings;
		StringValueSetting				*fFilenameSetting;
		StringValueSetting				*fFileFormatSetting;
		StringValueSetting				*fVideoEncoderSetting;
		StringValueSetting				*fAudioEncoderSetting;
		ScalarValueSetting				*fCaptureRateSetting;
		EnumeratedStringValueSetting 	*fColorSpaceSetting;
		StringValueSetting				*fResolutionSetting;
		ScalarValueSetting				*fQualitySetting;
		BooleanValueSetting				*fPreviewSetting;
		BooleanValueSetting				*fAudioSetting;
		ScalarValueSetting				*fAudioJitterSetting;
};

class ControlWindow : public BWindow {

public:
		ControlWindow(const BRect & frame, BView * controls, media_node node, bool audio);
		void MessageReceived(BMessage * message);
		bool QuitRequested();

private:
		BView *				fView;
		media_node			fNode;
		bool audio;
};

#endif

