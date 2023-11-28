#ifndef __PLAYER_APP__
#define __PLAYER_APP__

#include "Preferences.h"
#include <Application.h>

#define kAppSignature "application/x-vnd.Be.MediaPlayer"

class PlayerWindow;

namespace BPrivate {

class AttributeStreamNode;

}
using namespace BPrivate;

class URL;

const uint32 kWindowOpened = 'kwop';
const uint32 kWindowClosed = 'kwcl';
const uint32 kPrefsChanged = 'prfc';

class MediaPlayerApp : public BApplication {
public:

	MediaPlayerApp();
	bool HideWindow() const;
	bool AutoPlay() const;
	bool AutoQuitSounds() const;
	bool AutoQuitMovies() const;
	bool BackgroundHalfVolume() const;
	bool BackgroundMutedVolume() const;
	bool LoopMoviesByDefault() const;
	bool LoopSoundsByDefault() const;
	bool MediaNodeDefault() const;
	bool EnableHardwareOverlays() const;


	bool VerboseMode() const;
	void SuppressAutoQuit();
	void RunPrefsPanel(BPoint pos);
	
protected:
	virtual void RefsReceived(BMessage *message);
	virtual void MessageReceived(BMessage *message);
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void ReadyToRun();

	void Usage();

private:

	PlayerWindow* OpenRef(const entry_ref&);
	PlayerWindow* OpenRef(const entry_ref&, AttributeStreamNode *);
	PlayerWindow* OpenURL(const URL &url);
	
	bool fSuppressWindow;
	bool fLocalAutoPlay;
	bool fLocalSuppressAutoPlay;
	bool fLocalAutoQuit;
	bool fSuppressAutoQuit;
	MediaPlayerSettings settings;
	int32 fWindowCount;
	float fNextWin;
	bool fGotRefs;
	bool fHideWindow;
	
	typedef BApplication _inherited;
};

#endif
