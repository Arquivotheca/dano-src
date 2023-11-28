#include <AutoLock.h>
#include <Entry.h>
#include <Node.h>
#include <Directory.h>
#include "Attributes.h"
#include "AttributeStream.h"
#include "MediaPlayerApp.h"
#include "PlayerWindow.h"
#include "Preferences.h"
#include "URL.h"
#include "MediaTrackController.h"
#include "HTTPStream.h"
#include "FileHandler.h"
#include "PlayListHandler.h"
#include "debug.h"

bool debug_enabled = false;

//
//	For plugins
//
class _GlobalInitializer {
public:
	_GlobalInitializer();
} _initializer;

_GlobalInitializer::_GlobalInitializer()
{
	StreamHandler::RegisterStreamHandler("http", HTTPStream::InstantiateHTTPStream);
	StreamHandler::RegisterStreamHandler("file", FileHandler::InstantiateFileStream);
	RegisterPlayListHandlers();
}

MediaPlayerApp::MediaPlayerApp()
	:	BApplication(kAppSignature),
		fSuppressWindow(false),
		fLocalAutoPlay(false),
		fLocalSuppressAutoPlay(false),
		fLocalAutoQuit(false),
		fSuppressAutoQuit(false),
		fWindowCount(0),
		fNextWin(50),
		fGotRefs(false),
		fHideWindow(false)
{
}

PlayerWindow* MediaPlayerApp::OpenRef(const entry_ref &ref, AttributeStreamNode *attributes)
{
	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK || !entry.Exists()) 
		return 0;

	PlayerWindow *result = new PlayerWindow(BPoint(fNextWin, fNextWin));
	if (result->OpenURL(URL(ref), "") != B_OK) {
		result->Run();
		result->PostMessage(B_QUIT_REQUESTED);
		return 0;
	}

	if (attributes)
		result->RestoreState(attributes);

	return result;
}

PlayerWindow* MediaPlayerApp::OpenRef(const entry_ref &ref)
{
	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK || !entry.Exists())
		return 0;

	BNode node(&ref);
	AttributeStreamFileNode attributeReader(&node);
	return OpenRef(ref, &attributeReader);
}

PlayerWindow* MediaPlayerApp::OpenURL(const URL &url)
{
	PlayerWindow *result = new PlayerWindow(BPoint(fNextWin, fNextWin));
	if (result->OpenURL(url, "") != B_OK) {
		writelog("MediaPlayerApp::OpenURL() failed, posting quit message to window\n");
		result->Run();
		result->PostMessage(B_QUIT_REQUESTED);
		return 0;
	}

	return result;
}


bool MediaPlayerApp::AutoPlay() const
{
	return fLocalAutoPlay
		|| (!fLocalSuppressAutoPlay && settings.AutoPlay()->Value());
}

bool MediaPlayerApp::AutoQuitSounds() const
{
	return fLocalAutoQuit
		|| (!fSuppressAutoQuit && settings.QuitWhenDoneWithSounds()->Value());
}

bool MediaPlayerApp::AutoQuitMovies() const
{
	return fLocalAutoQuit
		|| (!fSuppressAutoQuit && settings.QuitWhenDoneWithMovies()->Value());
}

void MediaPlayerApp::SuppressAutoQuit()
{
	fSuppressAutoQuit = true;
}


void MediaPlayerApp::RunPrefsPanel(BPoint pos)
{
	settings.RunSettingsDialog(pos);
}


void MediaPlayerApp::RefsReceived(BMessage *message)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	message->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
		return;
	
	for (int32 i = --count; i >= 0; i--) 
		if (message->FindRef("refs", i, &ref) == B_OK) {
			PlayerWindow *window = OpenRef(ref);
			if (!window)
				continue;

			fGotRefs = true;
			window->Show();
			fNextWin += 10;
			if (fNextWin > 200)
				fNextWin = 50;
		}
}

void MediaPlayerApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kWindowOpened:
			fWindowCount++;
			break;

		case kWindowClosed:
			if (--fWindowCount <= 0)
				Quit();
			
			break;

		case kPrefsChanged: {
			int32 count = CountWindows();
			for (int32 index = 0; index < count; index++) {
				PlayerWindow *window = dynamic_cast<PlayerWindow *>(WindowAt(index));
				if (window) {
					AutoLock<BLooper> lock(window);
					window->PrefsChanged();
				}
			}
		}
			
		default:
			_inherited::MessageReceived(message);
			return;
	}
}

void MediaPlayerApp::Usage()
{
	printf("MediaPlayer command line syntax:\n"
		"MediaPlayer [-loop (on|off)] [-closeWhenDone (on|off)] [-autoPlay (on|off)] \n"
		"	[-pos <microseconds>] [-inpoint <microseconds>] [-outpoint <microseconds>] \n"
		"	[-mini | -large | -full] [-hideWindow] [-volume <0-1>] [-h | --help] file ....\n");
	fSuppressWindow = true;
	PostMessage(B_QUIT_REQUESTED);
}

void MediaPlayerApp::ArgvReceived(int32 , char **argv)
{
	BMessage *message = CurrentMessage();
	const char *currentWorkingDirectoryPath = NULL;
	BDirectory currentWorkingDir;

	if (message->FindString("cwd", &currentWorkingDirectoryPath) == B_OK)
		currentWorkingDir.SetTo(currentWorkingDirectoryPath);

	fHideWindow = false;
	// TODO: window and controller should handle their own override command-line args?
	bool nonDefaultControllerAttributes = false;
	bool nonDefaultWindowAttributes = false;
	bool dontSaveSettings = false;
	AttributeStreamMemoryNode attributes;
	for (++argv; *argv; argv++) {
	
		if (strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0) {
			Usage();
			break;
		} else if (strcmp(*argv, "-loop") == 0) {
			if (*++argv) {
				bool value = (strcmp(*argv, "on") == 0 || strcmp(*argv, "1") == 0);
				AttributeStreamBoolValue tmp(kAutoLoopAttribute, value);
				attributes << tmp;
				nonDefaultControllerAttributes = true;
			}
		} else if (strcmp(*argv, "-closeWhenDone") == 0) {
			if (*++argv) {
				bool value = (strcmp(*argv, "on") == 0 || strcmp(*argv, "1") == 0);
				if (value)
					fLocalAutoQuit = true;
				else
					fSuppressAutoQuit = true;
			}
		} else if (strcmp(*argv, "-large") == 0) {
			AttributeStreamInt32Value tmp(kMiniModeAttribute, PlayerWindow::kLarge);
			attributes << tmp;
			nonDefaultWindowAttributes = true;

		}  else if (strcmp(*argv, "-mini") == 0) {
			AttributeStreamInt32Value tmp(kMiniModeAttribute, PlayerWindow::kMini);
			attributes << tmp;
			nonDefaultWindowAttributes = true;

		}  else if (strcmp(*argv, "-full") == 0) {
			AttributeStreamInt32Value tmp(kMiniModeAttribute, PlayerWindow::kFullScreen);
			attributes << tmp;
			nonDefaultWindowAttributes = true;

		}  else if (strcmp(*argv, "-inpoint") == 0) {
			if (*++argv) {
				int64 value = 0;
				if (sscanf(*argv, "%Ld", &value) > 0) {
					AttributeStreamInt64Value tmp(kInPointAttribute, value);
					attributes << tmp;
					nonDefaultControllerAttributes = true;
				}
			}
			
		}  else if (strcmp(*argv, "-pos") == 0) {
			if (*++argv) {
				int64 value = 0;
				if (sscanf(*argv, "%Ld", &value) > 0) {
					AttributeStreamInt64Value tmp(kPosAttribute, value);
					attributes << tmp;
					nonDefaultControllerAttributes = true;
				}
			}
			
		}  else if (strcmp(*argv, "-outpoint") == 0) {
			if (*++argv) {
				int64 value = 0;
				if (sscanf(*argv, "%Ld", &value) > 0) {
					AttributeStreamInt64Value tmp(kOutPointAttribute, value);
					attributes << tmp;
					nonDefaultControllerAttributes = true;
				}
			}
			
		}  else if (strcmp(*argv, "-volume") == 0) {
			if (*++argv) {
				float value = 0;
				if (sscanf(*argv, "%f", &value) > 0) {
					AttributeStreamFloatValue tmp(kVolumeAttribute, value);
					attributes << tmp;
					nonDefaultControllerAttributes = true;
				}
			}

		}  else if (strcmp(*argv, "-autoPlay") == 0) {
			if (*++argv) {
				if (strcmp(*argv, "on") == 0 || strcmp(*argv, "1") == 0)
					fLocalAutoPlay = true;
				if (strcmp(*argv, "off") == 0 || strcmp(*argv, "0") == 0)
					fLocalSuppressAutoPlay = true;
			}
			dontSaveSettings = true;

		} else if (strcmp(*argv, "-hideWindow") == 0) {
			fHideWindow = true;
		} else if (strcmp(*argv, "-debug") == 0) {
			debug_enabled = true;
		} else if (**argv == '-') {
			printf("MediaPlayer: unknown option %s\n", *argv);
			Usage();
			break;
		} else {
			PlayerWindow *window = 0;

			{
				BEntry entry(&currentWorkingDir, *argv);
				entry_ref ref;
				if (entry.InitCheck() == B_OK && entry.Exists()
					&& entry.GetRef(&ref) == B_OK) {
					dontSaveSettings |= nonDefaultWindowAttributes | nonDefaultControllerAttributes;
	
					if (nonDefaultWindowAttributes)
					 	window = OpenRef(ref, &attributes);
					else
					 	window = OpenRef(ref);
	
					if (dontSaveSettings)
						window->DontSaveState(nonDefaultControllerAttributes);
				} else {
					URL url(*argv);
					if (url.IsValid()) {
						window = OpenURL(url);			
						fLocalAutoPlay = true;
					}
				}
			}

			if (window != 0) {
				fGotRefs = true;
				if (fHideWindow)
					window->Run();
				else
					window->Show();

				fNextWin += 10;
				if (fNextWin > 200)
					fNextWin = 50;
			} else {
				printf("MediaPlayer: error opening %s\n", *argv);
				continue;
			}
		}
	}
}

void MediaPlayerApp::ReadyToRun()
{
	if (!fGotRefs && !fSuppressWindow) {
		fSuppressAutoQuit = true;
			// we are only going to play movies that get dropped
			// on us, don't quit after that
		(new PlayerWindow(BPoint(fNextWin, fNextWin)))->Show();
	}
}

bool MediaPlayerApp::HideWindow() const
{
	return fHideWindow;
}

bool MediaPlayerApp::BackgroundHalfVolume() const
{
	return settings.BackgroundVolume()->EnumValue() == BackgroundVolumeSettingItem::kHalfVolume;
}

bool MediaPlayerApp::BackgroundMutedVolume() const
{
	return settings.BackgroundVolume()->EnumValue() == BackgroundVolumeSettingItem::kMuted;
}

bool MediaPlayerApp::LoopMoviesByDefault() const
{
	return settings.LoopMoviesByDefault()->Value();
}

bool MediaPlayerApp::LoopSoundsByDefault() const
{
	return settings.LoopSoundsByDefault()->Value();
}

bool MediaPlayerApp::MediaNodeDefault() const
{
	return settings.MediaNodeDefault()->Value();
}

bool MediaPlayerApp::EnableHardwareOverlays() const
{
	return settings.EnableHardwareOverlays()->Value();
}



