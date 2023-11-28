#include <stdio.h>
#include <Window.h>

#include "Bitmaps.h"
#include "NuTransportView.h"
#include "TransportButton.h"
#include "VolumeSlider.h"
#include "MultiThumbMediaSlider.h"
#include "BitmapButton.h"
#include "DiskBM.h"

static const float kViewW = 300, kViewH = 25;

static const float kButtonSpacing = 3;
static const bigtime_t kVolumeUpdatePeriod = 500000;

static const int32 kSkipBitmapWidth = 40;
static const int32 kSkiptBitmapHeight = 22;
static const BPoint kSkipButtonSize(kSkipBitmapWidth - 1, kSkiptBitmapHeight - 1);
const bigtime_t kUpdateInterval = 100000;

TransportView::TransportView( BRect bounds, uint32 resizeMask )
	: BView( bounds, "Transport View",
		resizeMask, B_PULSE_NEEDED )
{
	state = lastState = kStopped;
	updateMask = 0;
}

TransportView::~TransportView( void )
{
	
}

void TransportView::AttachedToWindow( void )
{
	SetViewColor( Parent()->ViewColor() );
	InitChildren();
	Window()->SetPulseRate(kUpdateInterval);
}

void TransportView::TransportView::Pulse( void )
{
	Looper()->PostMessage( MSG_TRANSPORT_UPDATE );
	UpdateCommon();
}

void TransportView::UpdateCommon( void )
{
	UpdateButtons();
}

status_t TransportView::InitChildren( void )
{
	const unsigned char *kPressedPlayButtonBitmapBits = kPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
	const unsigned char *kPlayingPlayButtonBitmapBits = kPressedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
	const unsigned char *kPressedPlayingPlayButtonBitmapBits = kPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
	const unsigned char *kPausedPlayButtonBitmapBits = kPressedPlayingPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
	const unsigned char *kPressedPausedPlayButtonBitmapBits = kPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
	const unsigned char *kDisabledPlayButtonBitmapBits = kPressedPausedPlayButtonBitmapBits + kPlayPauseBitmapWidth * kPlayPauseBitmapHeight;
	
	const unsigned char *kPressedStopButtonBitmapBits = kStopButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
	const unsigned char *kDisabledStopButtonBitmapBits = kPressedStopButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
	
	const unsigned char *kPressedSkipBackBitmapBits = kSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	const unsigned char *kSkippingSkipBackBitmapBits = kPressedSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	const unsigned char *kPressedSkippingSkipBackBitmapBits = kSkippingSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	const unsigned char *kDisabledSkipBackBitmapBits = kPressedSkippingSkipBackBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	
	const unsigned char *kPressedSkipForwardBitmapBits = kSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	const unsigned char *kSkippingSkipForwardBitmapBits = kPressedSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	const unsigned char *kPressedSkippingSkipForwardBitmapBits = kSkippingSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	const unsigned char *kDisabledSkipForwardBitmapBits = kPressedSkippingSkipForwardBitmapBits + kSkipBitmapWidth * kSkiptBitmapHeight;
	
	const unsigned char *kPressedRecordButtonBitmapBits = kRecordBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
	const unsigned char *kRecordingRecordButtonBitmapBits = kPressedRecordButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
	const unsigned char *kPressedRecordingRecordButtonBitmapBits = kRecordingRecordButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;
	const unsigned char *kDisabledRecordButtonBitmapBits = kPressedRecordingRecordButtonBitmapBits + kStopBitmapWidth * kStopBitmapHeight;

	TransportButton		*tbutton;
	
	BRect buttonRect(Bounds());
	buttonRect.InsetBy(4, 2);
	
	// Rewind
	buttonRect.SetRightTop(buttonRect.LeftBottom() + BPoint(kSkipButtonSize.x, -kSkipButtonSize.y));
	
	tbutton = new TransportButton(buttonRect, "",
		kSkipBackBitmapBits, kPressedSkippingSkipBackBitmapBits, kDisabledSkipBackBitmapBits,
		new BMessage(kNudgeBackward), 0, new BMessage(kRewind), 0, 100000, B_LEFT_ARROW, 0,
		B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(tbutton);
	rewind = tbutton;
	
	// Stop
	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kStopButtonSize);
	tbutton = new TransportButton(buttonRect, "",
		kStopButtonBitmapBits, kPressedStopButtonBitmapBits, kDisabledStopButtonBitmapBits,
		new BMessage(kStop), 0, 0, 0, 0, '.' , 0, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(tbutton);
	stop = tbutton;
	stop->SetEnabled( false );
	
	// Play
	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kPlayButtonSize);
	tbutton = new PlayPauseButton(buttonRect, "",
		kPlayButtonBitmapBits, kPressedPlayButtonBitmapBits, kDisabledPlayButtonBitmapBits,
		kPlayingPlayButtonBitmapBits, kPressedPlayingPlayButtonBitmapBits,
		kPausedPlayButtonBitmapBits, kPressedPausedPlayButtonBitmapBits,
		new BMessage(kPlayPause), ' ', 0, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(tbutton);
	playPause = (PlayPauseButton *)tbutton;
	
	// Fast-forward
	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kSkipButtonSize);
	tbutton = new TransportButton(buttonRect, "",
		kSkipForwardBitmapBits, kPressedSkippingSkipForwardBitmapBits, kDisabledSkipForwardBitmapBits,
		new BMessage(kNudgeForward), 0, new BMessage(kFastForward), 0, 100000,
		B_RIGHT_ARROW, 0, B_FOLLOW_LEFT | B_FOLLOW_TOP );
		AddChild(tbutton);
	fastForward = tbutton;
	
	// Record
	buttonRect.OffsetTo(buttonRect.right + kButtonSpacing, buttonRect.top);
	buttonRect.SetRightBottom(buttonRect.LeftTop() + kStopButtonSize);
	tbutton = new RecordButton(buttonRect, "",
		kRecordBitmapBits, kPressedRecordButtonBitmapBits, kDisabledRecordButtonBitmapBits,
		kRecordingRecordButtonBitmapBits, kPressedRecordingRecordButtonBitmapBits,
		new BMessage(kRecord), 0, 0, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(tbutton);
	record = (RecordButton *)tbutton;
	
	// Save Button
  	AddChild( saveButton = new BitmapButton( BPoint( 249, Bounds().bottom-(kDiskBMHeight/3) ), "", 
  	kDiskBM, kDiskBMWidth, kDiskBMHeight/3, new BMessage( kSave ), B_FOLLOW_TOP | B_FOLLOW_RIGHT ) );
  	
	// Volume Slider
	BRect		frame;
	frame.Set( Bounds().right -95, Bounds().bottom -17, Bounds().right - 4, Bounds().bottom+5 );
	AddChild( volumeSlider = new TVolumeSlider( frame, NULL, 0, 120, 
		TVolumeSlider::NewVolumeWidget(), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM ) );
	volumeSlider->SetValue( 0 );
	volumeSlider->SetModificationMessage( new BMessage( kVolumeChanged ) );
	
	// Media Slider
	// Thumb 0=center, 1=left, 2=right
	mediaSlider =  new TMultiThumbMediaSlider( BRect( 5, 10, Bounds().right -5, 50 ), "", "Slider", 0, 1000000, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT );
	AddChild( mediaSlider );
	mediaSlider->AddThumbs( 0, 0, 1000000,
		new BMessage(kStartScrub), new BMessage(kStopScrub), new BMessage(kScrub),
		NULL, NULL, new BMessage(kInPointChanged),
		NULL, NULL, new BMessage(kOutPointChanged));
	
	
	mediaSlider->SetLabelPlacement( kLabelTop );
	mediaSlider->SetThumbCollisionDetection( true );
	mediaSlider->UpdatePosition( stopThumbP, 1.0 );
	mediaSlider->UpdatePosition( startThumbP, 0 );
	mediaSlider->UpdatePosition( nowThumbP, 0 );
		
	return B_OK;
}

void TransportView::UpdateButtons( void )
{
	// If state changed
	if( state != lastState )
	{
		switch( state )
		{
			case kStopped:
				playPause->SetStopped();
				record->SetStopped();
				break;
			case kRecording:
				record->SetRecording();
				playPause->SetStopped();
				break;
			case kPlaying:
				playPause->SetPlaying();
				record->SetStopped();
				break;
			case kPaused:
				playPause->SetPaused();
				break;
		}
		
		if( state == kStopped )
			stop->SetEnabled( false );
		else
			stop->SetEnabled( true );
		
		if( (state == kPlaying) || (state == kPaused) )
			record->SetEnabled( false );
		else
			record->SetEnabled( true );
	}
	else if( state == kRecording )
		record->SetRecording(); // Flash Record LED
	else if( state == kPaused )
		playPause->SetPaused(); // Flash Pause LED
		
	if( updateMask )
	{
		if( updateMask & kUpdateStartThumb )
			mediaSlider->UpdatePosition( startThumbP, kStartThumb );
		if( updateMask & kUpdateStopThumb )
			mediaSlider->UpdatePosition( stopThumbP, kStopThumb );
		if( updateMask & kUpdateNowThumb )
			mediaSlider->UpdatePosition( nowThumbP, kNowThumb );
		updateMask = 0;
	}
	
	lastState = state;
}

void TransportView::SetButtonState( ControlState state )
{
	this->state = state;
	UpdateButtons();
}

ControlState TransportView::GetButtonState( void )
{
	return state;
}

float TransportView::GetVolume( void )
{
	return float(volumeSlider->Value())/100;
}

void TransportView::SetVolume( float level )
{
	printf( "Set Volume: %ld\n", int32(level*100.0) );
	volumeSlider->SetValue( level*100.0 );
}

void TransportView::UpdatePosition( float pos, MediaThumb thumb )
{
	switch( thumb )
	{
		case kStartThumb:
			updateMask |= kUpdateStartThumb;
			startThumbP = pos;
			break;
		case kStopThumb:
			updateMask |= kUpdateStopThumb;
			stopThumbP = pos;
			break;
		case kNowThumb:
			updateMask |= kUpdateNowThumb;
			nowThumbP = pos;
			break;
	}
}

void TransportView::UpdateTime( bigtime_t time, MediaThumb thumb )
{
	mediaSlider->UpdateTime( time, thumb );
}

float TransportView::GetPosition( MediaThumb thumb )
{
	return mediaSlider->Position( thumb );
}

void TransportView::EnablePlay( bool enabled )
{
	playPause->SetEnabled( enabled );
	fastForward->SetEnabled( enabled );
	rewind->SetEnabled( enabled );
	volumeSlider->SetEnabled( enabled );
	saveButton->SetEnabled( enabled );
	// mediaSlider->SetEnabled( enabled );
	
}

void TransportView::EnableRecord( bool enabled )
{
	record->SetEnabled( enabled );
}

BHandler *TransportView::GetMediaSlider( void )
{
	return mediaSlider;
}
