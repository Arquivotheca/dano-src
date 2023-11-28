#ifndef _GSTATUSWIND_H
#define _GSTATUSWIND_H


// GStatusWind.h

class GStatusView;

class GStatusWindow : public BWindow
{
public:
	GStatusWindow(BRect frame, const char *,bool needView = TRUE);
	BMessenger		StatusMessenger();
	virtual void	DoCancel(bool c = TRUE);
	virtual void	Show();
	virtual void	Hide();
protected:
	GStatusView *sview;	
};

class GStatusView : public BView
{
public:
				GStatusView(BRect,const char *);
virtual void	MessageReceived(BMessage *);
virtual void	AttachedToWindow();

protected:
	long	iCount;
	long	curCount;

	char status[80];
};


enum {
	M_SETUP_STATUS = 'SeSt'
};

#endif
