#include <InterfaceKit.h>
#include <Application.h>
#include <StorageKit.h>
#include <Debug.h>
#include <string.h>
#include <String.h>
#include <stdlib.h>
#include <roster_private.h>

const char *explanationText = 
"This program will configure whether your system uses the new or old media "
"services when it boots.  Changes made here will be applied to your "
"bootscripts, and will take effect the next time you boot your system.\n\n"
"In general, it is best to use the new media services.  However, if you are "
"having problems some older programs, reverting to the old media services may "
"fix them.  Note that you can always switch back to the previous configuration "
"by re-running this program.";

const char *kConfigFileName = "use_old_audio";
const int32 kRebootMessage = 'REBT';

class MediaServicesWindow : public BWindow {
public:

	MediaServicesWindow(BRect rect) 
		:	BWindow(rect, "Media Kit Setup", B_TITLED_WINDOW, B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE),
			settingsSaved(false)
	{

		//
		//	Background BBox
		//
		BRect backgroundRect(Bounds());
		backgroundRect.right += 1;
		backgroundRect.bottom += 1;
		BBox *backgroundView = new BBox(backgroundRect, "background view",
			B_FOLLOW_ALL);
		AddChild(backgroundView);	

		//
		//	Explanation Text View
		//
		BRect explanationRect(Bounds());
		explanationRect.InsetBy(10,10);	
		explanationRect.bottom = backgroundRect.bottom - 115;
		BRect textRect(explanationRect);
		textRect.OffsetTo(0,0);
		textRect.InsetBy(4,4);
		explanationView = new BTextView(explanationRect, "explanation", 
			textRect, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS);

		explanationView->SetWordWrap(true);
		explanationView->MakeEditable(false);
		explanationView->SetViewColor(backgroundView->ViewColor());
		explanationView->SetText(explanationText);
		backgroundView->AddChild(explanationView);

		//
		//	Radio button BBox
		//
		BRect chooserRect(Bounds());
		chooserRect.InsetBy(10,10);
		chooserRect.top = explanationRect.bottom + 10;
		chooserRect.bottom = chooserRect.top + 60;
		BBox *chooserBox = new BBox(chooserRect, "chooser box",	B_FOLLOW_BOTTOM
			| B_FOLLOW_LEFT_RIGHT);
		backgroundView->AddChild(chooserBox);

		BRect buttonRect(chooserRect);
		buttonRect.OffsetTo(0, 0);
		buttonRect.InsetBy(6, 6);
		buttonRect.bottom = buttonRect.top + 25;

		//
		//	New Media Services Radio Button
		//
		newMediaButton = new BRadioButton(buttonRect, 
			"new media services button", "Use New Media Services", 0,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
		chooserBox->AddChild(newMediaButton);

		//
		//	Old Media Services Radio Button
		//
		buttonRect.OffsetBy(0, 25);
		oldMediaButton = new BRadioButton(buttonRect,
			"old media services button", "Use Old Media Services", 0,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
		chooserBox->AddChild(oldMediaButton);
	
		//
		//	Reboot button
		//
		BRect rebootRect(backgroundRect);
		rebootRect.InsetBy(10, 10);
		rebootRect.top = chooserRect.bottom + 10;
		rebootRect.right = rebootRect.left + 95;
		rebootButton = new BButton(rebootRect, "reboot button", 
			"Reboot Now", new BMessage(kRebootMessage), B_FOLLOW_LEFT | 
			B_FOLLOW_BOTTOM);
		backgroundView->AddChild(rebootButton);	


		//
		//	Determine what the previous setting was
		//
		BPath configFilePath;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &configFilePath) 	
			== B_OK) {
			BDirectory configDir(configFilePath.Path());
			BFile file(&configDir, kConfigFileName, B_READ_ONLY);
			if (file.InitCheck() != B_OK) {
				originalValue = 0;
				newMediaButton->SetValue(1);
			} else {
				originalValue = 1;
				oldMediaButton->SetValue(1);	
			}
		}
	}

	void RemovePreferenceLink(const char *name)
	{
		BPath userConfig;
		if (find_directory(B_USER_CONFIG_DIRECTORY, &userConfig) == B_OK) {
			BString userPrefsDirPath(userConfig.Path());
			userPrefsDirPath += "/be/Preferences";
			BDirectory userPrefsDir(userPrefsDirPath.String());
			if (userPrefsDir.InitCheck() == B_OK) {
				BEntry entry(&userPrefsDir, name);
				if (entry.InitCheck() == B_OK) {

#if DEBUG
					BPath entryPath;
					entry.GetPath(&entryPath);
					PRINT(("About to delete link '%s'\n", entryPath.Path()));
#endif

					entry.Remove();
				}
			}
		}

		BPath systemPrefs;
		if (find_directory(B_PREFERENCES_DIRECTORY, &systemPrefs) 
			== B_OK) {
			BDirectory systemPrefsDir(systemPrefs.Path());
			if (systemPrefsDir.InitCheck() == B_OK) {
				BEntry entry(&systemPrefsDir, name);
				if (entry.InitCheck() == B_OK) {

#if DEBUG
					BPath entryPath;
					entry.GetPath(&entryPath);
					PRINT(("About to delete link '%s'\n", entryPath.Path()));
#endif

					entry.Remove();
				}
			}
		}
	}


	void AddPreferenceLink(const char *name, const char *path)
	{
		BSymLink dummy;
		BPath userConfig;
		if (find_directory(B_USER_CONFIG_DIRECTORY, &userConfig) == B_OK) {
			BString userPrefsDirPath(userConfig.Path());
			userPrefsDirPath += "/be/Preferences";
			BDirectory userPrefsDir(userPrefsDirPath.String());
			if (userPrefsDir.InitCheck() == B_OK) {

#if DEBUG
				BPath entryPath;
				BEntry entry(&userPrefsDir, name);
				entry.GetPath(&entryPath);
				PRINT(("About to create symlink '%s->%s'\n", entryPath.Path(),
					path));
#endif

				userPrefsDir.CreateSymLink(name, path, &dummy);
			}
		}

		BPath systemPrefs;
		if (find_directory(B_PREFERENCES_DIRECTORY, &systemPrefs) 
			== B_OK) {
			BDirectory systemPrefsDir(systemPrefs.Path());
			if (systemPrefsDir.InitCheck() == B_OK) {
#if DEBUG
				BPath entryPath;
				BEntry entry(&systemPrefsDir, name);
				entry.GetPath(&entryPath);
				PRINT(("About to create symlink '%s->%s'\n", entryPath.Path(),
					path));
#endif

				systemPrefsDir.CreateSymLink(name, path, &dummy);
			}
		}
	}

	virtual void MessageReceived(BMessage *message)
	{
		switch (message->what) {
			case kRebootMessage: {
				SaveSettings();
				BMessenger(ROSTER_SIG).SendMessage(CMD_REBOOT_SYSTEM);
			}
	
			default:
				BWindow::MessageReceived(message);	
		}
	}

	void SaveSettings()
	{
		BDirectory *configDir = 0;
		
		BPath configFilePath;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &configFilePath) == B_OK)
			configDir = new BDirectory(configFilePath.Path());

		if (oldMediaButton->Value() == true) {
			// 	
			// 	Use old media kit
			//
			RemovePreferenceLink("Sounds");
			RemovePreferenceLink("Audio");
			RemovePreferenceLink("Video");
			AddPreferenceLink("Sound", "/boot/beos/preferences/Sound");
	
			if (configDir)
				BFile file(configDir, kConfigFileName, B_READ_WRITE | 
					B_CREATE_FILE);
		} else {

			//
			// User wants to use new media kit
			//
			RemovePreferenceLink("Sound");
			AddPreferenceLink("Sounds", "/boot/beos/preferences/Sounds");
			AddPreferenceLink("Video", "/boot/beos/preferences/Video");
			AddPreferenceLink("Audio", "/boot/beos/preferences/Audio");

			if (configDir) {
				BEntry entry(configDir, kConfigFileName);
				entry.Remove();
			}
		}	

		delete configDir;
		settingsSaved = true;
	}


	virtual bool QuitRequested()
	{
		if (!settingsSaved)
			SaveSettings();

		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}

	virtual void FrameResized(float width, float height) 
	{
		BRect textRect(explanationView->Frame());
		textRect.OffsetTo(0,0);
		textRect.InsetBy(4,4);
		explanationView->SetTextRect(textRect);
	}

private:

	BTextView 		*explanationView;
	BButton 		*rebootButton;
	BRadioButton 	*newMediaButton;
	BRadioButton 	*oldMediaButton;
	bool			originalValue;	// 0 = new, 1 = old
	bool 			settingsSaved;
};

main()
{
	BApplication app("application/x-vnd.Be.MediaServicesPrefs");
	MediaServicesWindow *window = new MediaServicesWindow(
		BRect( 100, 100, 400, 450));
	window->Show();

	app.Run();
}
