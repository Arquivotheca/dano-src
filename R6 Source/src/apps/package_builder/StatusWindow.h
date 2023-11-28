// StatusWindow.h

#include "GStatusWind.h"

#ifndef _STATUSWINDOW_H
#define _STATUSWINDOW_H

class FileLooper;

class StatusWindow : public GStatusWindow
{
public:
				StatusWindow(const char *, bool *);
virtual void	DoCancel(bool c = TRUE);		

private:
	bool	*cancelVar;
};

class StatusView : public GStatusView
{
public:
				StatusView(BRect,const char *);
virtual void	MessageReceived(BMessage *);


private:
		void	SetupStatus(BMessage *msg);
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


class ProgressBarView : public BView
{
public:

	ProgressBarView(BRect bounds,
				const char *name,
				ulong resizeFlags =
					B_FOLLOW_LEFT | B_FOLLOW_TOP,
				ulong flags	= B_WILL_DRAW)
		: BView(bounds, name, resizeFlags, flags) {
			SetMinValue(0);
			SetMaxValue(0);
			SetValue(0);
		};
inline void		SetValue(float p) { fVal = p; };
inline void		SetMinValue(float min) { fMin = min; };
inline void		SetMaxValue(float max) { fMax = max; };
virtual	void 	Draw(BRect update);
virtual void	AttachedToWindow();

private:
	float	fVal;
	float	fMin,fMax;
};


//=======================================================

#endif
