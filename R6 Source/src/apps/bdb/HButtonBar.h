/*	$Id: HButtonBar.h,v 1.1 1998/11/14 14:20:49 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten Hekkelman
	
	Created: 12/30/97 23:01:09
*/

#ifndef HBUTTONBAR_H
#define HBUTTONBAR_H

#include <Rect.h>
#include <View.h>
#include <Window.h>

#include <vector>

class HHelpWindow;

struct BtnTemplate {
	long resID;
	long cmd;
	long flags;
};

enum BtnFlags {
	bfMenu,
	bfToggle,
	bfSpace,
	bfDualIcon
};

enum BtnBarFlags {
	bbDragger,
	bbAcceptFirstClick
};

class HButtonBar;

class HButton {
public:
			HButton(HButtonBar *bar, int resID, int cmd, float x, int flags, const char *help);
virtual	~HButton();
			
virtual	void Draw(bool pushed = false);
virtual	void MouseEnter(bool pushed = false);
virtual	void MouseLeave();

	void SetOn(bool on);
	void SetDown(bool pushed);
	void SetEnabled(bool enabled = true);
	
	BRect Frame() const;
	int Cmd() const;
	bool IsToggle() const;
	bool IsOn() const;
	bool IsDown() const;
	bool IsMenu() const;
	bool IsEnabled() const;
	
	const char* Help() const;

protected:
	HButtonBar *fBar;
	char *fHelp;
	bool fMenu, fToggle, fDown, fEnabled, fOn;
	int fCmd;
	unsigned char *fIcon;
	BRect fFrame;
};

class HDualIconButton : public HButton {
public:
			HDualIconButton(HButtonBar *bar, int resID, int cmd, float x, int flags, const char *help);
			
virtual	void Draw(bool pushed = false);
virtual	void MouseEnter(bool pushed = false);
virtual	void MouseLeave();

private:
			unsigned char *fAltIcon;
};

class HButtonBar : public BView {
public:
			HButtonBar(BRect frame, const char *name, int resID, BHandler *target = NULL);
			~HButtonBar();
		
virtual	void Draw(BRect update);
virtual	void MouseMoved(BPoint where, uint32 code, const BMessage *a_message);
virtual	void MouseDown(BPoint where);

virtual	void WindowActivated(bool active);
			
			void SetTarget(BHandler *target);
			BHandler* Target() const;
			
			void SetDown(int cmd, bool down);
			void SetOn(int cmd, bool on);
			void SetEnabled(int cmd, bool enabled = true);
			
			bool IsActive() 		{ return Window()->IsActive(); }

private:
			int FindButton(BPoint where);
			
			void ShowHelp();
			void HideHelp();
virtual	void Pulse();

			bool fDragger, fAcceptFirstClick;
			BHandler *fTarget;
			std::vector<HButton*> fButtons;
			int fLastButtonOver;
			HHelpWindow *fHelp;
			bigtime_t fLastEnter, fLastDisplay;
};

inline BRect HButton::Frame() const
{
	return fFrame;
} /* HButton::Frame */

inline int HButton::Cmd() const
{
	return fCmd;
} /* HButton::Cmd */

inline bool HButton::IsToggle() const
{
	return fToggle;
} /* HButton::Toggle */

inline bool HButton::IsDown() const
{
	return fDown;
} /* HButton::Down */

inline bool HButton::IsOn() const
{
	return fOn;
} /* HButton::Down */

inline bool HButton::IsMenu() const
{
	return fMenu;
} /* HButton::Menu */

inline bool HButton::IsEnabled() const
{
	return fEnabled;
} /* HButton::IsEnabled */

inline const char* HButton::Help() const
{
	return fHelp;
} /* HButton::Help */

#endif
