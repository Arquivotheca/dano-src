// IStatusWindow.h
#ifndef _ISTATUSWINDOW_H_
#define _ISTATUSWINDOW_H_

#include <Window.h>
#include <View.h>
#include <StringView.h>
#include <StatusBar.h>
#include <Rect.h>
#include <Message.h>

class InstallPack;

class StatusWindow : public BWindow
{

public:
				StatusWindow(BRect frame, const char *);

InstallPack		*packFile;
virtual void	Show();
virtual void	Hide();
};

class StatusView : public BView
{

public:
				StatusView(BRect,const char *);
virtual void	MessageReceived(BMessage *);
virtual void	AttachedToWindow();

BStatusBar		*sbar;
bool			canceled;
long	iCount;
long	curCount;

char status[40];

};

class GrayStringView : public BStringView
{
public:

	GrayStringView(BRect bounds,
				const char *name, 
				const char *text,
				ulong resizeFlags =
					B_FOLLOW_LEFT | B_FOLLOW_TOP,
				ulong flags = B_WILL_DRAW)
		: BStringView(bounds, name, text, resizeFlags, flags) {};
virtual void Draw(BRect up);
virtual void AttachedToWindow();
};

#endif
