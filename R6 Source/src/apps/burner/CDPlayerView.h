//
// CDPlayerView.h
//
//  by Nathan Schrenk (nschrenk@be.com)

#ifndef CDPLAYER_VIEW_H_
#define CDPLAYER_VIEW_H_

#include <View.h>
#include "CDTrack.h"

#define CDPLAYERVIEWNAME "CDPlayerView"

// CD player control messages
const uint32 kPlayButtonMessage 		= 'pLAY';
const uint32 kStopButtonMessage 		= 'sTOP';
const uint32 kSeekBackButtonMessage		= 'sEKB';
const uint32 kSeekForwardButtonMessage	= 'sEKF';
const uint32 kSkipBackButtonMessage		= 'sKPB';
const uint32 kSkipForwardButtonMessage	= 'sKPF';
const uint32 kTrackInvokedMessage 		= 'tINV';

class AudioManager;
class BMessageRunner;
class BStringView;
class BThread;
class BurnerWindow;
class TimeDisplay;
class TransportButton;
class PlayPauseButton;
class TVolumeSlider;

class CDPlayerView : public BView
{
public:
	CDPlayerView(BRect frame, uint32 resizingMode = B_FOLLOW_NONE);
	virtual ~CDPlayerView();
	
	void SetCurrentTrack(int32 track, bool isPlaying);
	void TrackGainChanged(CDTrack *track, float gain);
	
	void	SetControlsEnabled(bool enabled);

protected:
	virtual void MessageReceived(BMessage *message);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void AllAttached();

		
private:
	PlayPauseButton *fPlayButton;
	TransportButton *fStopButton;
	TransportButton *fSeekBackButton, *fSeekForwardButton;
	TransportButton *fSkipBackButton, *fSkipForwardButton;
	TVolumeSlider	*fVolumeSlider;
	BStringView		*fTrackNumber;
	TimeDisplay		*fTrackTime;
	AudioManager	*fManager;
	BThread			*fAudioManagerThread;
	BurnerWindow	*fWindow;
	BMessageRunner	*fTimeRunner;
	int32			fPlayingIndex;
	bool			fConstructed;
	bool			fIsPlaying;
};

#endif // CDPLAYER_VIEW_H_
