#include <Application.h>
#include <AutoLock.h>
#include <Debug.h>
#include <FilePanel.h>
#include <InterfaceKit.h>
#include <MediaKit.h>
#include <MessageFilter.h>
#include <Roster.h>
#include <stdio.h>
#include "URL.h"
#include "Attributes.h"
#include "AttributeStream.h"
#include "PlayerWindow.h"
#include "VideoView.h"
#include "MediaTrackController.h"
#include "LargeTransportView.h"
#include "MediaPlayerApp.h"
#include "MiniTransportView.h"
#include "NodeWrapper.h"
#include "VolumeMenuControl.h"
#include "URLPanel.h"
#include "InfoWindow.h"
#include "PlayListHandler.h"
#include "debug.h"

const float kDefaultWindowWidth = 320;
const float kMinWindowWidth = kMinViewWidth;
const float kMinWindowHeight = kMinViewHeight;
const char *kDefaultTitle = "MediaPlayer";


const char *kDownloadURL = "http://www.be.com/support/updates/codecs.html";

class ShortcutDispatchFilter : public BMessageFilter {
	// this is to work around not being able to use AddShortcut to add
	// shortcuts without modifiers
public:
	ShortcutDispatchFilter(PlayerWindow *target);

protected:
	filter_result Filter(BMessage *message, BHandler **handler);

private:
	PlayerWindow *target;
};

ShortcutDispatchFilter::ShortcutDispatchFilter(PlayerWindow *target)
	:	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		target(target)
{
}

filter_result ShortcutDispatchFilter::Filter(BMessage *message, BHandler**)
{
	if (target->Mode() == PlayerWindow::kFullScreen) {
		// if in full screen mode, any input will switch us back to one of the
		// other two modes
		if (message->what == B_MOUSE_DOWN) {
			target->RestoreFromFullScreen();
			return B_SKIP_MESSAGE;
		}
		
		if (message->what == B_KEY_DOWN) {
			// let a few keyboard shortcuts still be recognized even in
			// full screen mode
			uint32 modifiers;
			uint32 rawKeyChar = 0;
			uint8 byte = 0;
			int32 key = 0;
			if (message->FindInt32("modifiers", (int32 *)&modifiers) == B_OK
				&& message->FindInt32("raw_char", (int32 *)&rawKeyChar) == B_OK
				&& message->FindInt8("byte", (int8 *)&byte) == B_OK
				&& message->FindInt32("key", &key) == B_OK) {

				modifiers &= B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY
					| B_OPTION_KEY | B_MENU_KEY;
					// strip caps lock, etc.
				
				if (modifiers == 0 && rawKeyChar == ' ') 
					target->Looper()->PostMessage(kPlayPause, target);
				else if (modifiers == 0 && rawKeyChar == B_LEFT_ARROW)
					target->Looper()->PostMessage(kNudgeBackward, target);
				else if (modifiers == 0 && rawKeyChar == '.')
					target->Looper()->PostMessage(kStop, target);
				else if (modifiers == 0 && rawKeyChar == B_RIGHT_ARROW)
					target->Looper()->PostMessage(kNudgeForward, target);
				else					
					target->RestoreFromFullScreen();

				return B_SKIP_MESSAGE;
			}
		}
	}

	if (target->fTransportView
		&& dynamic_cast<MiniTransportView *>(target->fTransportView)
			// don't filter if large transport visible
		&& (message->what == B_KEY_DOWN || message->what == B_KEY_UP)) {
		uint32 modifiers;
		uint32 rawKeyChar = 0;
		uint8 byte = 0;
		int32 key = 0;
		
		if (message->FindInt32("modifiers", (int32 *)&modifiers) != B_OK
			|| message->FindInt32("raw_char", (int32 *)&rawKeyChar) != B_OK
			|| message->FindInt8("byte", (int8 *)&byte) != B_OK
			|| message->FindInt32("key", &key) != B_OK)
			return B_DISPATCH_MESSAGE;

		modifiers &= B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY
			| B_OPTION_KEY | B_MENU_KEY;
			// strip caps lock, etc.


		if (modifiers == 0 && rawKeyChar == B_LEFT_ARROW)
			target->Looper()->PostMessage(kNudgeBackward, target);
		else if (modifiers == 0 && rawKeyChar == '.')
			target->Looper()->PostMessage(kStop, target);
		else if (modifiers == 0 && rawKeyChar == B_RIGHT_ARROW)
			target->Looper()->PostMessage(kNudgeForward, target);
		else
			return B_DISPATCH_MESSAGE;

		return B_SKIP_MESSAGE;
	}

	// let others deal with this
	return B_DISPATCH_MESSAGE;
}


float PlayerWindow::TransportHeightForMode(PlayMode mode)
{
	if (mode == kMini)
		return kMinTransportHeight;
	else if (mode == kLarge)
		return kMaxTransportHeight;
	else
		return 0;
}

float PlayerWindow::MinHeightForMode(PlayMode mode) const
{
	if (mode == kFullScreen) {
		BRect frame(BScreen().Frame());
		return frame.Height();
	}

	float result = TransportHeightForMode(mode);
	if (fMainMenu)
		result += fMainMenu->Bounds().Height() + 1;

	return result;
}

float PlayerWindow::MaxHeightForMode(PlayMode mode, bool noVideo) const
{
	if (mode == kFullScreen) {
		BRect frame(BScreen().Frame());
		return frame.Height();
	}

	if (!noVideo)
		return 1200;

	return MinHeightForMode(mode);
}

float PlayerWindow::MinWidthForMode(PlayMode mode) const
{
	if (mode == kFullScreen) {
		BRect frame(BScreen().Frame());
		return frame.Width();
	}

	if (mode == kMini)
		return kMinViewWidth;

	return 270;
}

float PlayerWindow::MaxWidthForMode(PlayMode mode, bool noFile) const
{
	if (mode == kFullScreen) {
		BRect frame(BScreen().Frame());
		return frame.Width();
	}

	if (noFile)
		return kDefaultWindowWidth;
	
	return 2048;
}


const window_look kDefaultWindowLook = B_TITLED_WINDOW_LOOK;

PlayerWindow::PlayerWindow(BPoint upperLeft)
	:	BWindow(BRect(upperLeft.x, upperLeft.y, upperLeft.x + kDefaultWindowWidth,
			upperLeft.y + kMinWindowHeight), kDefaultTitle, kDefaultWindowLook, 
			B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS),
		fVideoView(0),
		fController(0),
		fTransportView(0),
		fMode(kLarge),
		fFallBackMode(kMini),
		fStateChanged(false),
		fDontSaveWindowState(false),
		fDontSaveControllerState(false),
		fKeepNonproportionalResize(false),
		fInfoWindow(0)
{
	SetSizeLimits(0, 2048, 0, 1200);
	fBackground = new BView(Bounds(), "background", B_FOLLOW_ALL, B_WILL_DRAW);
	fBackground->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fBackground->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fBackground);
	
	fMainMenu = 0;
	if (fMode == kLarge)
		SetUpMenu();
	
	BRect transportRect(Bounds());
	transportRect.top = transportRect.bottom - TransportHeightForMode(fMode);
	
	BRect newBounds(transportRect);
	newBounds.top = 0;

	ResizeTo(newBounds.Width(), newBounds.Height());

	if (fMode == kMini)
		fTransportView = new MiniTransportView(transportRect, B_FOLLOW_LEFT_RIGHT |
			B_FOLLOW_BOTTOM);
	else if (fMode == kLarge)
		fTransportView = new LargeTransportView(transportRect, B_FOLLOW_LEFT_RIGHT |
			B_FOLLOW_BOTTOM);
	
	if (fTransportView)
		fBackground->AddChild(fTransportView);


	SetSizeLimits(MinWidthForMode(fMode), MaxWidthForMode(fMode, true),
		MinHeightForMode(fMode), MaxHeightForMode(fMode, true));
	FrameResized(Bounds().Width(),Bounds().Height());

	fFilePanel = NULL;
	
	AddCommonFilter(new ShortcutDispatchFilter(this));
	SetUpShortcuts();

	if (fMode == kFullScreen) 
		SetLook(B_NO_BORDER_WINDOW_LOOK);

	be_app->PostMessage(kWindowOpened);
}


void PlayerWindow::SetUpMenu()
{
	BRect menuRect(Bounds());
	menuRect.bottom = menuRect.top + 25;
	fMainMenu = new BMenuBar(menuRect, "main menu");
	BMenu *menu = new BMenu("File");
	fMainMenu->AddItem(menu); 
	fBackground->AddChild(fMainMenu);
	menu->AddItem(new BMenuItem("Open File"B_UTF8_ELLIPSIS, new BMessage(M_OPEN), 'O'));
	menu->AddItem(new BMenuItem("Open URL"B_UTF8_ELLIPSIS, new BMessage(M_OPEN_URL_PANEL), 'U'));
	menu->AddItem(new BMenuItem("File Info"B_UTF8_ELLIPSIS, new BMessage(M_FILE_INFO), 'I'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Download new media addons"B_UTF8_ELLIPSIS,
		new BMessage(M_DOWNLOAD_ADDONS)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
	BMenuItem *item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);

	menu = new BMenu("View");
	fMainMenu->AddItem(menu); 
	menu->AddItem(new BMenuItem("Mini mode", new BMessage(M_TOGGLE_MINI_MODE), 'T'));
	menu->AddItem(new BMenuItem("50% scale", new BMessage(M_RESIZE_TO_1_BY_2_SCALE), '0'));
	menu->AddItem(new BMenuItem("100% scale", new BMessage(M_RESIZE_TO_1_BY_1_SCALE), '1'));
	menu->AddItem(new BMenuItem("200% scale", new BMessage(M_RESIZE_TO_2_BY_1_SCALE), '2'));
	menu->AddItem(new BMenuItem("Full screen", new BMessage(M_RESIZE_TO_FULL_SCREEN), 'F'));
	
	menu = new BMenu("Settings");
	fMainMenu->AddItem(menu); 
	menu->AddItem(new BMenuItem("Application Preferences"B_UTF8_ELLIPSIS,
		new BMessage(M_RUN_APP_SETTINGS)));	
	menu->AddItem(new BMenuItem("Loop", new BMessage(M_TOGGLE_LOOP_MODE), 'L'));
	menu->AddItem(new BMenuItem("Preserve Video Timing", new BMessage(M_TOGGLE_DROP_FRAMES)));
}

void PlayerWindow::SetUpShortcuts()
{
	// make sure menu item shortcuts also work in mini mode without a menu
	AddShortcut('0', B_COMMAND_KEY, new BMessage(M_RESIZE_TO_1_BY_2_SCALE), this);
	AddShortcut('1', B_COMMAND_KEY, new BMessage(M_RESIZE_TO_1_BY_1_SCALE), this);
	AddShortcut('2', B_COMMAND_KEY, new BMessage(M_RESIZE_TO_2_BY_1_SCALE), this);
	AddShortcut('F', B_COMMAND_KEY, new BMessage(M_RESIZE_TO_FULL_SCREEN), this);
	AddShortcut('+', B_COMMAND_KEY, new BMessage(M_NUDGE_VOLUME_UP), this);
	AddShortcut('-', B_COMMAND_KEY, new BMessage(M_NUDGE_VOLUME_DOWN), this);
	AddShortcut('I', B_COMMAND_KEY, new BMessage(M_FILE_INFO), this);
	AddShortcut('T', B_COMMAND_KEY, new BMessage(M_TOGGLE_MINI_MODE), this);

	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED), this);
	AddShortcut('Q', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED), be_app);
	AddShortcut('O', B_COMMAND_KEY, new BMessage(M_OPEN), this);
	AddShortcut('U', B_COMMAND_KEY, new BMessage(M_OPEN_URL_PANEL), this);
	AddShortcut('L', B_COMMAND_KEY, new BMessage(M_TOGGLE_LOOP_MODE), this);
}

void PlayerWindow::MenusBeginning()
{
	if (!fMainMenu)
		return;
	
	BMenu *fileMenu = fMainMenu->SubmenuAt(0);
	ASSERT(fileMenu && strcmp(fileMenu->Name(), "File") == 0);
	UpdateFileMenu(fileMenu);

	BMenu *viewMenu = fMainMenu->SubmenuAt(1);
	ASSERT(viewMenu && strcmp(viewMenu->Name(), "View") == 0);
	UpdateViewMenu(viewMenu);

	BMenu *settingsMenu = fMainMenu->SubmenuAt(2);
	ASSERT(settingsMenu && strcmp(settingsMenu->Name(), "Settings") == 0);
	UpdateSettingsMenu(settingsMenu);
}

void PlayerWindow::UpdateFileMenu(BMenu *menu)
{
	BMenuItem *item = menu->FindItem(M_FILE_INFO);
	if (item)
		item->SetEnabled(fController != 0);
}

void PlayerWindow::UpdateViewMenu(BMenu *menu)
{
	BMenuItem *item = menu->FindItem(M_RESIZE_TO_1_BY_2_SCALE);
	if (item)
		item->SetEnabled(fController != 0 && fController->HasVideo());
	item = menu->FindItem(M_RESIZE_TO_1_BY_1_SCALE);
	if (item)
		item->SetEnabled(fController != 0 && fController->HasVideo());
	item = menu->FindItem(M_RESIZE_TO_2_BY_1_SCALE);
	if (item)
		item->SetEnabled(fController != 0 && fController->HasVideo());

	item = menu->FindItem(M_RESIZE_TO_FULL_SCREEN);
	if (item)
		item->SetEnabled(fController != 0 && fController->HasVideo());
}

void PlayerWindow::UpdateSettingsMenu(BMenu *menu)
{
	BMenuItem *item = menu->FindItem(M_TOGGLE_LOOP_MODE);
	if (item) {
		item->SetEnabled(fController && !fController->IsContinuous());
		item->SetMarked(fController && fController->AutoLoop()
			&& !fController->IsContinuous());
	}

	item = menu->FindItem(M_TOGGLE_DROP_FRAMES);
	if (item) {
		item->SetEnabled(fController && fController->HasVideo());
		item->SetMarked(fController != 0 && fController->HasVideo()
			&& fController->DropFrames());
	}
}

void PlayerWindow::BuildContextMenu(BMenu *menu)
{
	menu->AddItem(new BMenuItem("Open File"B_UTF8_ELLIPSIS, new BMessage(M_OPEN), 'O'));
	menu->AddItem(new BMenuItem("File Info"B_UTF8_ELLIPSIS, new BMessage(M_FILE_INFO), 'I'));
	menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));

	//	only show this menu item if the current mode is mini
	if (Mode() == kMini) {
		menu->AddSeparatorItem();
		TSliderMenuItem* slider = new TSliderMenuItem(0, (2 * (60 + 18)),
			new BMessage(kVolumeMenuChange), new BMessage(kVolumeMenuChange));
		// TVolumeMenu AddItems slider
		TVolumeMenu* volumeMenu = new TVolumeMenu("Volume", slider, Controller());
		menu->AddItem(volumeMenu);
		//	enable the control only if the controller thinks it has audio
		volumeMenu->SetEnabled(Controller() && Controller()->HasAudio());
		slider->SetTarget(this);
	}
	
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(Mode() == kMini ? "Large mode" : "Mini mode",
		new BMessage(M_TOGGLE_MINI_MODE), 'T'));
	menu->AddItem(new BMenuItem("50% scale", new BMessage(M_RESIZE_TO_1_BY_2_SCALE), '0'));
	menu->AddItem(new BMenuItem("100% scale", new BMessage(M_RESIZE_TO_1_BY_1_SCALE), '1'));
	menu->AddItem(new BMenuItem("200% scale", new BMessage(M_RESIZE_TO_2_BY_1_SCALE), '2'));
	menu->AddItem(new BMenuItem("Full screen", new BMessage(M_RESIZE_TO_FULL_SCREEN), 'F'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Loop", new BMessage(M_TOGGLE_LOOP_MODE)));
	menu->AddItem(new BMenuItem("Preserve Video Timing", new BMessage(M_TOGGLE_DROP_FRAMES)));
	menu->AddItem(new BMenuItem("Application Preferences"B_UTF8_ELLIPSIS,
		new BMessage(M_RUN_APP_SETTINGS)));

	menu->SetTargetForItems(this);
	
	UpdateFileMenu(menu);
	UpdateViewMenu(menu);
	UpdateSettingsMenu(menu);
}

bool PlayerWindow::ShouldSaveState()
{
	return fStateChanged || (fController && fController->StateChanged());
}

bool PlayerWindow::QuitRequested()
{
	delete fFilePanel;

	if (ShouldSaveState()) {
		writelog("Save file attributes\n");
		BFile node(&fRef, O_RDWR);
		if (node.InitCheck() == B_OK) {
			AttributeStreamFileNode attributeWriter(&node);
			SaveState(&attributeWriter);
		}
	}

	writelog("Disconnect transport view from controller\n");
	if(fTransportView)
		fTransportView->AttachToController(0);

	writelog("Quit controller\n");
	MediaController *tmp = fController;
	fController = 0;
	if (tmp)
		// quit/delete the controller
		tmp->Close();

	if (fMode == kFullScreen) {
		writelog("Show cursor\n");
		AutoLock<BLooper> lock(be_app);
		if (lock.IsLocked()) 
			be_app->ShowCursor();
	}
	
	if (fInfoWindow) {
		writelog("Close info window\n");
		fInfoWindow->Lock();
		fInfoWindow->Close();
	}

	writelog("Inform app that window has closed\n");
	be_app->PostMessage(kWindowClosed);
	return true;
}

void PlayerWindow::MessageReceived(BMessage *message)
{

   if (message->WasDropped() || message->what == B_REFS_RECEIVED) {
        entry_ref fileRef;
		int32 index = 0;

		const char *urlString;
		if (message->FindString("be:url", &urlString) == B_OK) {
			const char *cookies;
			if (message->FindString("be:cookie", &cookies) != B_OK)
				cookies = "";

			URL url(urlString);
			if (url.IsValid()) {
				OpenURL(url, cookies);
				MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
				app->SuppressAutoQuit();
				Play();
			}
		} else if (message->FindRef("refs", index, &fileRef) == B_OK) {
			// the first window gets opened by 
			BEntry entry(&fileRef, true);
			if (entry.InitCheck() != B_OK || !entry.Exists())
				return;

			OpenURL(URL(fileRef), "");
			MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
			if (app)
				app->SuppressAutoQuit();
		}
		
		for (++index; ; index++) {
			if (message->FindRef("refs", index, &fileRef) != B_OK)
				break;	
			BMessage refsReceived(B_REFS_RECEIVED);
			refsReceived.AddRef("refs", &fileRef);
			be_app->PostMessage(&refsReceived);
		}

        return;
    }

	switch (message->what) {
		case M_OPEN:{
			BMessenger messenger(0, this);
			fFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger);
			fFilePanel->Show();
			break;
		}	
		case M_TOGGLE_MINI_MODE:
			ToggleMiniMode();
			break;
	
		case M_FILE_INFO: 
			if (fController) {
				BString info;
				fController->GetFileInfo(&info);
				info << "Location: " << fCurrentURL.GetURLString() << "\n"; 

				BString title;
				if (strlen(fCurrentURL.GetFileName()) == 0)
					title = fCurrentURL.GetHostName();
				else
					title = fCurrentURL.GetFileName();	

				if (fInfoWindow && fInfoWindow->Lock()) {
					fInfoWindow->SetTo(title.String(), info.String(),
						fIsLocalFile ? &fRef : 0, Controller());

					if (fInfoWindow->IsHidden()) {
						BPoint upperLeft;
						BRect frame = Frame();
						upperLeft.x = (frame.left + frame.right) / 2 - (kInfoWindowWidth / 2);
						upperLeft.y = frame.top + 20;
						if (upperLeft.x < 25)
							upperLeft.x = 25;
							
						if (upperLeft.y < 25)
							upperLeft.y = 25;
						
						fInfoWindow->MoveTo(upperLeft.x, upperLeft.y);

						fInfoWindow->Show();
					} else
						fInfoWindow->Activate(true);

					fInfoWindow->Unlock();
				} else {
					BPoint upperLeft;
					BRect frame = Frame();
					upperLeft.x = (frame.left + frame.right) / 2 - (290 / 2);
					upperLeft.y = frame.top + 20;
					if (upperLeft.x < 25)
						upperLeft.x = 25;
						
					if (upperLeft.y < 25)
						upperLeft.y = 25;
					
					fInfoWindow = new InfoWindow(upperLeft, title.String(),
						info.String(), fIsLocalFile ? &fRef : 0, Controller());
					fInfoWindow->Show();
				}
			} else 
				(new BAlert("Error", "No File Opened", "OK"))->Go(0);
			
			break;
	
		case M_RESIZE_TO_1_BY_1_SCALE:
			Set1By1Scale();
			break;
		
		case M_RESIZE_TO_2_BY_1_SCALE:
			Set1By2Scale();
			break;

		case M_RESIZE_TO_1_BY_2_SCALE:
			Set2By1Scale();
			break;

		case M_RESIZE_TO_FULL_SCREEN:
			SetFullScreen();
			break;

		case kVolumeMenuChange:
			//	if the controller has audio then update it
			if (Controller() && Controller()->HasAudio()) {
				float position;
				//	stashed in message is the position (0-1)
				message->FindFloat("be:position", &position);
				Controller()->SetVolume(position);
			}
			break;

		case kPlayPause:
		case kStop:
		case M_NUDGE_VOLUME_UP:
		case M_NUDGE_VOLUME_DOWN:
		case kBeginning:
		case kPlay:
		case kEnd:
		case kScrub:
		case kFastForward:
		case kRewind:
		case kNudgeForward:
		case kNudgeBackward:
		case kVolumeChanged:
			// forward these - we need to set up window level shortcuts to
			// target the window rather that the transport view - that way we
			// don't need to re-target each time the transport switches between
			// mini and large
			if (fTransportView)
				PostMessage(message, fTransportView);
			else {
				// full screen mode - only dispatch a few shortcuts
				switch (message->what) {
					case kPlayPause:
						if (Controller()) {
							if (Controller()->IsPlaying())
								Controller()->Pause();
							else
								Controller()->Play();
						}
						break;

					case kStop:
						if (Controller()) 
							Controller()->Stop();
						break;

					case kNudgeForward:
						if (Controller()) 
							Controller()->NudgeForward();
						break;

					case kNudgeBackward:
						if (Controller()) 
							Controller()->NudgeBackward();
						break;

					default:
						break;
				}
			}
			break;

		case M_TOGGLE_LOOP_MODE:
			if (fController) {
				fController->SetAutoLoop(!fController->AutoLoop());
				fController->SetDefaultAutoLoop(false);
			}
			break;
			
		case M_TOGGLE_DROP_FRAMES:
			if (fController)
				fController->SetDropFrames(!fController->DropFrames());

			break;
			
		case M_RUN_APP_SETTINGS:
			{
				BRect myFrame(Frame());
			
				MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
				app->SuppressAutoQuit();
				app->RunPrefsPanel(BPoint(myFrame.left + 10, myFrame.top + 10));
			}
			break;

		case M_DONE_PLAYING:
			{
				writelog("received M_DONE_PLAYING message\n");
				MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
				ASSERT(app);
				if (app && ((fController && fController->HasVideo() && app->AutoQuitMovies())
					|| (fController && !fController->HasVideo() && app->AutoQuitSounds())))
					PostMessage(B_QUIT_REQUESTED);
			}
			break;

		case M_DOWNLOAD_ADDONS:
			DownloadCodec();
			break;

		case M_OPEN_URL_PANEL:
			{
				BMessenger messenger(0, this);
				BRect myFrame(Frame());
				(new URLPanel(BPoint(myFrame.left + 10, myFrame.top + 10),
					&messenger, fCurrentURL.GetURLString()))->Show();
				break;
			}

		case kOpenURL:
			{
				char *urlString;
				if (message->FindString("be:url", (const char**) &urlString) == B_OK) {
					URL url(urlString);
					if (url.IsValid()) {
						fCurrentURL = url;
						OpenURL(url, "");
					}
				}
			
				break;
			}

		case kFileReady:
			ControllerReady();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}


void PlayerWindow::WindowActivated(bool state)
{
	_inherited::WindowActivated(state);
	if (fController && fController->HasVideo()) {
		if (state)
			fController->RestoreFullVolume();
		else {
			MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
			if (app->BackgroundHalfVolume())
				fController->SetHalfVolume();
			else if (app->BackgroundMutedVolume())
				fController->SetMuted();
		}
	}
}

void PlayerWindow::WorkspaceActivated(int32 workspace, bool on)
{	
	_inherited::WorkspaceActivated(workspace, on);
	
	// deal with hidden cursor in full screen mode
	if (fMode == kFullScreen) {

		uint32 workspaceMask = 1;
		for ( ; workspace; workspace--)
			workspaceMask *= 2;

		if (Workspaces() & workspaceMask) {
			AutoLock<BLooper> lock(be_app);
			if (lock.IsLocked()) {
				if (on)
					be_app->HideCursor();
				else
					be_app->ShowCursor();
			}
		}
	}
}

MediaController* PlayerWindow::Controller() const
{
	return fController;
}

void 
PlayerWindow::DownloadCodec(const char *)
{
	be_roster->Launch(B_URL_HTTP, 1, (char **)&kDownloadURL);
}

void PlayerWindow::CloseFile()
{
	if (fInfoWindow && fInfoWindow->Lock()) {
		if (!fInfoWindow->IsHidden())
			fInfoWindow->Hide();
		fInfoWindow->Unlock();
	}

	if (fTransportView)	
		fTransportView->SetEnabled(false);

	if (fController) {
		// save state of the file we have open
		if (ShouldSaveState()) {
			BFile node(&fRef, O_RDWR);
			if (node.InitCheck() == B_OK) {
				AttributeStreamFileNode attributeWriter(&node);
				SaveState(&attributeWriter);
			}
		}

		// In case a new file was thrown onto the window,
		// reset the controller-save override.
		// The window-save override should NOT be reset here!
		fDontSaveControllerState = false;

		fTransportView->AttachToController(0);

		MediaController *tmp = fController;
		fController = 0;
		if (tmp)
			// quit/delete the controller
			tmp->Close();

		ASSERT(this->IsLocked());
	}
	
	if (fVideoView) {
		fVideoView->SetBitmap(0);
		UpdateIfNeeded();
	}

	BeginViewTransaction();
	if (fVideoView)
		RemoveVideoView();

	ResizeTo(MinWidthForMode(fMode), MinHeightForMode(fMode));
	EndViewTransaction();
}

//
//	Initiate the process of opening a URL.  When this process completes,
// 	the controller will send a 'kFileReady' message, at which time we will
//	complete the initialization in ControllerReady().
//
status_t PlayerWindow::OpenURL(const URL &url, const char *cookies)
{
	status_t err = B_OK;
	BString title;
	
	if (!url.IsValid()) {
		err = B_ERROR;
		goto error1;
	}

	CloseFile();

	if (strlen(url.GetHostName()) != 0)
		title << "Connecting to " << url.GetHostName() << B_UTF8_ELLIPSIS;
	else
		title << "Opening " << url.GetFileName() << B_UTF8_ELLIPSIS;

	SetTitle(title.String());
	fController = MediaTrackController::Open(url, cookies, BMessenger(0, this),
		&err);
	if (!fController)
		goto error2;

	fFileName = url.GetFileName();
	fIsLocalFile = (strcmp(url.GetScheme(), "file") == 0);
	if (fIsLocalFile)
		url.GetRef(fRef);

	fCurrentURL = url;

	fFallBackRect = Frame();
	return B_OK;

error2:
	SetTitle(kDefaultTitle);
error1:
	return err;
}

//
//	This is the second half of file opening, and occurs when the controller has
// 	sent a message that it has opened the file and is ready to play (or has an error)
//
void PlayerWindow::ControllerReady()
{
	ASSERT(fController);
	status_t err = B_OK;
	BRect videoRect;
	BRect newBounds;
	MediaPlayerApp *app = 0;
	float minWidth = 0, minHeight = 0, maxWidth = 1000000.0, maxHeight = 1000000.0;
	
	err = fController->InitCheck();
	if (err != B_OK)
		goto error1;
		 
	// set up new resizing limits
	if (fController->HasVideo()) {
		fController->GetOutputSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
		minHeight += MinHeightForMode(fMode) + 1;	// Take transport/menu size into account
		maxHeight += MinHeightForMode(fMode) + 1;
	}

	minWidth = MAX(minWidth, MinWidthForMode(fMode));
	minHeight = MAX(minHeight, MinHeightForMode(fMode));
	maxWidth = MIN(maxWidth, MaxWidthForMode(fMode, !fController));
	maxHeight = MIN(maxHeight, MaxHeightForMode(fMode, !fController || !fController->HasVideo()));
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// Set up video area
	if (fController->HasVideo()) {
		videoRect = VideoView::ViewRect(fController->VideoSize());
		if (videoRect.Width() < MinWidthForMode(fMode))
			videoRect.right = videoRect.left + MinWidthForMode(fMode);
		if (videoRect.Height() < MinHeightForMode(fMode))
			videoRect.bottom = videoRect.top + MinHeightForMode(fMode);
		newBounds = SizeForVideoRect(videoRect);

		BeginViewTransaction();
		ResizeTo(newBounds.Width(), newBounds.Height());
		if (fVideoView) {
			fVideoView->ResizeTo(videoRect.Width(), videoRect.Height());
			fVideoView->SetVideoSize(BPoint(fController->VideoSize().Width(),
				fController->VideoSize().Height()));
		} else {
			if (fMode == kLarge)
				videoRect.OffsetTo(0, fMainMenu->Frame().bottom + 1);
			fVideoView = new VideoView(videoRect, BPoint(fController->VideoSize().Width(),
				fController->VideoSize().Height()), "video", B_FOLLOW_ALL_SIDES);
			fBackground->AddChild(fVideoView);
		}
		EndViewTransaction();

		err = fController->ConnectVideoOutput(fVideoView);
		if (err < 0)
			goto error2;
		
		fVideoView->SetBitmap(0);
	} else if (fVideoView) 
		RemoveVideoView();

	if(fTransportView)
	{
		fTransportView->AttachToController(fController);
		fTransportView->SetEnabled(true);
	}

	if (strlen(fController->GetConnectionName()) > 0)
		SetTitle(fController->GetConnectionName());
	else if (strlen(fCurrentURL.GetFileName()) == 0)
		SetTitle(fCurrentURL.GetHostName());
	else
		SetTitle(fCurrentURL.GetFileName());	

	if (fIsLocalFile) {
		//
		//	Read attributes if this is a local file
		//
		BNode node(&fRef);
		AttributeStreamFileNode attributeReader(&node);
		RestoreState(&attributeReader, true);
	}

	app = dynamic_cast<MediaPlayerApp *>(be_app);
	if ((app && app->AutoPlay()) || fController->IsContinuous())
		Play();

	return;

error2:
	RemoveVideoView();
error1:
	fController->Close();
	fController = 0;
	SetTitle(kDefaultTitle);

	//
	//	Display feedback to the user why we couldn't play
	//
	const char *desc = "";
	CurrentMessage()->FindString("be:error_description", &desc);
	BString failureDescription("Unable to play");
	if (strlen(desc) > 0)
		failureDescription << ": " << desc;

	if (err == B_MEDIA_NO_HANDLER) {
		failureDescription << "Would you like to check the "
			"Be website for an addon that can handle this file?";

		if ((new BAlert("Error", failureDescription.String(), "Visit Site", "Cancel"))->Go() == 0)
			DownloadCodec();
	} else
		(new BAlert("Error", failureDescription.String(), "Darn"))->Go();
}

void PlayerWindow::RemoveVideoView()
{
	BeginViewTransaction();
	fVideoView->RemoveSelf();
	ResizeTo(Bounds().Width(), MinHeightForMode(fMode));
	EndViewTransaction();

	delete fVideoView;
	fVideoView = 0;
}

void PlayerWindow::Play()
{
	if (fController)
		fController->Play();
}

void PlayerWindow::DontSaveState(bool dontsavecontrollerstate)
{
	fDontSaveWindowState = true;
	fDontSaveControllerState = dontsavecontrollerstate;
}

void PlayerWindow::Set1By1Scale()
{
	if (!fController || !fController->HasVideo())
		return;

	BRect frame(SizeForVideoRect(fController->VideoSize()));
	ResizeTo(frame.Width(), frame.Height());
}

void PlayerWindow::Set1By2Scale()
{
	if (!fController || !fController->HasVideo())
		return;

	BRect videoFrame(fController->VideoSize());
	videoFrame.OffsetTo(0, 0);
	videoFrame.right = (videoFrame.right + 1) * 2 - 1;
	videoFrame.bottom = (videoFrame.bottom + 1) * 2 - 1;
	BRect frame(SizeForVideoRect(videoFrame));
	ResizeTo(frame.Width(), frame.Height());
}

void PlayerWindow::Set2By1Scale()
{
	if (!fController || !fController->HasVideo())
		return;

	BRect videoFrame(fController->VideoSize());
	videoFrame.OffsetTo(0, 0);
	videoFrame.right = (videoFrame.right + 1) / 2 - 1;
	videoFrame.bottom = (videoFrame.bottom + 1) / 2 - 1;
	BRect frame(SizeForVideoRect(videoFrame));
	ResizeTo(frame.Width(), frame.Height());
}

void PlayerWindow::SetFullScreen()
{
	SetMode(kFullScreen);
}

void PlayerWindow::RestoreFromFullScreen()
{
	SetMode(fFallBackMode);
}


BRect PlayerWindow::SizeForVideoRect(BRect rect)
{
	BRect result(VideoView::ViewRect(rect));
	
	if (result.Width() < kMinWindowWidth)
		result.right = result.left + kMinWindowWidth;

	if (fMode != kFullScreen)
		result.bottom += TransportHeightForMode(fMode) + 1;

	if (fMode == kLarge) 
		result.bottom += fMainMenu->Frame().Height() + 1;

	return result;
}

void PlayerWindow::SetMode(PlayMode mode)
{
	if (mode == fMode)
		return;

	fStateChanged = true;
	
	if (fVideoView)
		// when switching mode, remember that you were in a non-proportional
		// mode and don't have the window resize switch back to proportional
		fKeepNonproportionalResize = !fVideoView->ProportionalResize();

	if (mode == kFullScreen) {
		if (!fController || !fController->HasVideo())
			// can't do that
			return;

		// save current mode to restore from full screen
		fFallBackMode = fMode;
		fFallBackRect = Frame();
	}
	
	PlayMode oldMode = fMode;
	fMode = mode;

	BPoint videoViewSize(0, 0);
	if (fVideoView) {
		videoViewSize.x = fVideoView->Bounds().Width();
		videoViewSize.y = fVideoView->Bounds().Height();
	}

	BeginViewTransaction();
	
	if (oldMode == kFullScreen) {
		MoveTo(fFallBackRect.LeftTop());
		// restore the cursor
		AutoLock<BLooper> lock(be_app);
		if (lock.IsLocked()) 
			be_app->ShowCursor();
	} else if (fMode == kFullScreen)
		MoveTo(0, 0);
	
	// rip out view items we will no longer need
	if (fTransportView) {
		fTransportView->RemoveSelf();
		delete fTransportView;
		fTransportView = 0;
	}

	if (fMainMenu) {
		fMainMenu->RemoveSelf();
		delete fMainMenu;
		fMainMenu = 0;
	}

	BRect newBounds(Bounds());
	newBounds.bottom = videoViewSize.y;

	// add menu if needed and move video view to accommodate for menubar
	if (fMode == kLarge) {
		SetUpMenu();
		newBounds.bottom += fMainMenu->Frame().Height() + 1;
		if (fVideoView)
			fVideoView->MoveTo(0, fMainMenu->Frame().Height() + 1);
	} else if (fVideoView)
		fVideoView->MoveTo(0, 0);
	
	if (fMode == kFullScreen) {
		newBounds = BScreen().Frame();
		videoViewSize.x = newBounds.Width();
		videoViewSize.y = newBounds.Height();
	} else if (oldMode == kFullScreen) {
		newBounds = fFallBackRect;
		videoViewSize.x = newBounds.Width();
		videoViewSize.y = newBounds.Height();
		videoViewSize.y -= TransportHeightForMode(fMode) + 1;
		if (fMode == kLarge)
			videoViewSize.y -= fMainMenu->Frame().Height() + 1;
	} else
		// account for transport
		newBounds.bottom += TransportHeightForMode(fMode) + 1;

	// make sure size is right for new mode
	float minWidth = 0, minHeight = 0, maxWidth = 1000000.0, maxHeight = 1000000.0;
	if (fController && fController->HasVideo()) {
		fController->GetOutputSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
		if(fMode != kFullScreen)
		{
			minHeight += MinHeightForMode(fMode) + 1;	// Take transport/menu size into account
			maxHeight += MinHeightForMode(fMode) + 1;
		}
	}

	minWidth = MAX(minWidth, MinWidthForMode(fMode));
	minHeight = MAX(minHeight, MinHeightForMode(fMode));
	maxWidth = MIN(maxWidth, MaxWidthForMode(fMode, !fController));
	maxHeight = MIN(maxHeight, MaxHeightForMode(fMode, !fController || !fController->HasVideo()));

	if (newBounds.Width() < minWidth) 
		newBounds.right = minWidth;
	else if (newBounds.Width() > maxWidth) 
		newBounds.right = maxWidth;

	if (newBounds.Height() < minHeight)
		newBounds.bottom = minHeight;

	// set up new resizing limits
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// new window size
	ResizeTo(newBounds.Width(), newBounds.Height());

	// view got resized by the window resize but we really want it to stay the same
	// size in this case
	if (fVideoView) {
		if (videoViewSize.x < newBounds.Width())
			videoViewSize.x = newBounds.Width();
		fVideoView->ResizeTo(videoViewSize.x, videoViewSize.y);
	}

	// add the transport view if appropriate
	BRect transportRect(Bounds());
	transportRect.top = transportRect.bottom - TransportHeightForMode(fMode);

	if (fMode == kMini)
		fTransportView = new MiniTransportView(transportRect, B_FOLLOW_LEFT_RIGHT |
			B_FOLLOW_BOTTOM);
	else if (fMode == kLarge)
		fTransportView = new LargeTransportView(transportRect, B_FOLLOW_LEFT_RIGHT |
			B_FOLLOW_BOTTOM);
	
	if (fTransportView) {
		fBackground->AddChild(fTransportView);
		fTransportView->AttachToController(fController);
		if (fController)
			fTransportView->SetEnabled(true);
	}
	
	if (fMode == kFullScreen) {
		SetLook(B_NO_BORDER_WINDOW_LOOK);
		Activate();

		BRect newFrame(BScreen().Frame());
		MoveTo(newFrame.LeftTop());
			// already resized right, finish the move
		AutoLock<BLooper> lock(be_app);
		if (lock.IsLocked()) 
			be_app->HideCursor();
	} else 
		SetLook(kDefaultWindowLook);

	EndViewTransaction();

	SetUpShortcuts();
	UpdateIfNeeded();
}

void PlayerWindow::FixupViewLocations()
{
	BRect videoView(Bounds());

	BeginViewTransaction();
	if (fTransportView) {
		videoView.bottom -= fTransportView->Bounds().Height() + 1;
		fTransportView->MoveTo(0, videoView.bottom + 1);
		fTransportView->ResizeTo(videoView.Width(), fTransportView->Bounds().Height());
	}
	if (fVideoView) {
		if (fMode == kLarge) 
			videoView.top += fMainMenu->Frame().Height() + 1;
		fVideoView->MoveTo(videoView.LeftTop());
		fVideoView->ResizeTo(videoView.Width(), videoView.Height());
	}
	EndViewTransaction();
}

void PlayerWindow::FrameResized(float newWidth, float newHeight)
{
	float minwidth, maxwidth, minheight, maxheight;
	
	fStateChanged = true;
	
	GetSizeLimits(&minwidth, &maxwidth, &minheight, &maxheight);
	if(newWidth < minwidth)
		return ResizeTo(minwidth,newHeight);
	if(newWidth > maxwidth)
		return ResizeTo(maxwidth,newHeight);
	if(newHeight < minheight)
		return ResizeTo(newWidth,minheight);
	if(newHeight > maxheight)
		return ResizeTo(newWidth,maxheight);

	if (fVideoView) {
		bool oldProportionalResize = fVideoView->ProportionalResize();
		bool shouldResizeProportional = (modifiers() & B_SHIFT_KEY) == 0
			&& !fKeepNonproportionalResize;
		
		fVideoView->SetProportionalResize(shouldResizeProportional);
		if (oldProportionalResize != shouldResizeProportional)
			fVideoView->Invalidate();
	}

	_inherited::FrameResized(newWidth, newHeight);
#if DEBUG
	if (modifiers() & B_CAPS_LOCK)
#endif
	FixupViewLocations();

	fKeepNonproportionalResize = false;
}


void PlayerWindow::ToggleMiniMode()
{
	if (fMode == kMini)
		SetMode(kLarge);
	else if (fMode == kLarge)
		SetMode(kMini);
	else
		SetMode(fFallBackMode);
}

void PlayerWindow::Zoom(BPoint, float, float)
{
	SetFullScreen();
}


PlayerWindow::PlayMode PlayerWindow::Mode() const
{
	return fMode;
}

void PlayerWindow::RestoreState(AttributeStreamNode *stream, bool keepWindowPos)
{
	int32 tmpInt;
	if (stream->Read(kMiniModeAttribute, 0, B_INT32_TYPE, sizeof(tmpInt), &tmpInt)
		== sizeof(tmpInt)) {	
		SetMode((PlayMode)tmpInt);
	}

	BRect frame;
	if (stream->Read(kWindowFrameAttribute, 0, B_RECT_TYPE, sizeof(frame), &frame)
		== sizeof(frame)) {

		ResizeTo(frame.Width(), frame.Height());

		// make sure we stay onscreen
		BScreen screen;
		BRect screenRect(screen.Frame());
		screenRect.InsetBy(30, 30);
		if (!keepWindowPos && screenRect.Contains(frame.LeftTop()))
			MoveTo(frame.LeftTop());
	}

	if (fController) 
		fController->RestoreState(stream);
		
	fStateChanged = false;
}

void PlayerWindow::SaveState(AttributeStreamNode *stream)
{
	if (!fDontSaveWindowState)
	{
		PlayMode mode = fMode;
		stream->Write(kMiniModeAttribute, 0, B_INT32_TYPE, sizeof(mode), &mode);
	
		BRect frame(Frame());
		stream->Write(kWindowFrameAttribute, 0, B_RECT_TYPE, sizeof(frame), &frame);
	}
	
	if (!fDontSaveControllerState)
	{
		if (fController)
			fController->SaveState(stream);
	}
	
	fStateChanged = false;
}

void PlayerWindow::PrefsChanged()
{
	if (fController) {
		fController->PrefsChanged();
		if (!IsActive()) {
			MediaPlayerApp *app = dynamic_cast<MediaPlayerApp *>(be_app);
			if (app->BackgroundHalfVolume())
				fController->SetHalfVolume();
			else if (app->BackgroundMutedVolume())
				fController->SetMuted();
			else
				fController->RestoreFullVolume();
		}
	}	
}

TransportView* PlayerWindow::GetTransportView() const
{
	return fTransportView;
}
