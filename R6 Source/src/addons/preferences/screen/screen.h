#ifndef NuScreen_H
#define NuScreen_H

#include <Application.h>
#include <Screen.h>
#include <Box.h>
#include <ColorControl.h>
#include <ListItem.h>
#include <ListView.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <Slider.h>
#include <Window.h>
#include <Alert.h>

#include "ModeModeler.h"

class TRefreshSlider : public BSlider
{
public:
			TRefreshSlider(BRect, BMessage*);
	void	KeyDown(const char *bytes, int32 numBytes);
	void	SetValue(int32);
	void	SetApplyNow(bool);		
	void	SetRate(float rate);
	float	Rate() const;
	void	SetFreqs(float bottom, float top);
	char	*UpdateText() const;			

private:
		float				fTop;
		float 				fBottom;
		char				str[32];
		
		//	type ahead variables
		char				fKeyBuffer[10];
		bigtime_t			fLastTime;
};

const int32 kMaxWorkspaceCount = 32;

//	the main background that grabs key events
//		to configure the monitor
class TBox : public BView
{
public:
								TBox();
								~TBox();

		void					GetPrefs();
		void					SetPrefs();

		void					AttachedToWindow();
		void					MessageReceived(BMessage*);

		void					WorkspaceActivated(int32 ws, bool state);

		void 					AddParts();

		void 					UpdateControls();
		void					UpdateAdvancedControls();
		void					Revert();
		void					Defaults();
		void					CheckChanges();

		void 					SaveSettings();
		display_mode			*TweakAdvanced();
		void					TweakEasy();
		bool					advancedmode;
		display_mode			*advancedentry;

private:
		bool					fCanRevert;
		bool					fAllWorkspaces;
		
		BView					*miniscreen;

		BPopUpMenu*				fTargetMenu;
		
		BButton*				fApplyBtn;
		BButton*				fRevertBtn;
		BButton*				fDefaultsBtn;
		TRefreshSlider			*fRefreshRateSlider;
		BCheckBox				*hsync;
		BCheckBox				*vsync;
		BCheckBox				*green;
		BButton					*up;
		BButton					*left;
		BButton					*right;
		BButton					*down;

		ModeModeler				mm;
		BPopUpMenu				*col[MM_COLUMN_COUNT];
		BMenuField				*menu[MM_COLUMN_COUNT];
		display_mode			current;
		display_mode			initial[32];
};

#endif
