#ifndef TRANSPORT_VIEW_H
#define TRANSPORT_VIEW_H

#include <View.h>

class TransportButton;
class PlayPauseButton;
class RecordButton;
class TMultiThumbMediaSlider;
class TVolumeSlider;
class BitmapButton;

enum MediaThumb
{
	kStartThumb = 1,
	kStopThumb = 2,
	kNowThumb = 0
};

enum ControlState
{
	kStopped,
	kRecording,
	kPlaying,
	kPaused
};
		
class TransportView : public BView
{

	public:
		TransportView( BRect bounds, uint32 resizeMask = B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT );
		virtual ~TransportView( void );
		
		virtual void AttachedToWindow( void );
		virtual void Pulse( void );
		
		// Buttons
		void SetButtonState( ControlState state );
		ControlState GetButtonState( void );
		void EnablePlay( bool enabled );
		void EnableRecord( bool enabled );
		
		// Volume Slider
		float GetVolume( void );
		void SetVolume( float level );
		
		// Media Slider
		void UpdatePosition( float pos, MediaThumb thumb );
		void UpdateTime( bigtime_t time, MediaThumb thumb );
		float GetPosition( MediaThumb thumb );
		BHandler *GetMediaSlider( void );
		
	private:
		status_t InitChildren( void );
		void UpdateCommon( void );
		void UpdateButtons( void );
	
	private:
		
		enum {
			kUpdateStartThumb = 1,
			kUpdateStopThumb = 2,
			kUpdateNowThumb = 4
		};
		PlayPauseButton			*playPause;
		RecordButton			*record;
		TransportButton			*fastForward, *rewind, *stop;
		TMultiThumbMediaSlider	*mediaSlider;
		TVolumeSlider			*volumeSlider;
		BitmapButton			*saveButton;
		ControlState			state, lastState;
		
		int32					updateMask;
		
		float					startThumbP;
		float					stopThumbP;
		float					nowThumbP;
};

enum {
kBeginning = 'begn',
kRewind = 'rwnd',
kStop = 'stop',
kRecord = 'rcrd',
kPlay = 'play',
kPlayPause = 'plap',
kFastForward = 'ffwd',
kEnd = 'endd',
kScrub = 'scrb',
kStartScrub = 'stsc',
kStopScrub = 'spsc',
kDoneScrubbing = 'dscr',
kInPointChanged = 'kinp',
kOutPointChanged = 'koup',
kNudgeForward = 'ndgf',
kNudgeBackward = 'ndgb',
kVolumeChanged = 'volc',
kBumpAndRewind = 'bprw',
kBumpAndGoToEnd = 'bpge',
kSave = 'sAve',
MSG_TRANSPORT_UPDATE = 'trup'
};

#endif
