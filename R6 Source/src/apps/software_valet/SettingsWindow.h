#ifndef _SETTINGSWINDOW_H_
#define _SETTINGSWINDOW_H_

#include "ResizeView.h"
#include <Messenger.h>
#include <Window.h>

class SettingsManager;

extern const char		*kListenerSig;
extern SettingsManager	*gSettings;


/*********************************************************/
class SettingsWindow : public BWindow
{
public:
	SettingsWindow(SettingsManager *_set);
	virtual			~SettingsWindow();
	virtual bool	QuitRequested();
	virtual void	WindowActivated(bool state);

	SettingsManager	*settings;
};


class SettingsView : public BView
{
public:
	SettingsView(BRect frame);
	
	virtual void AttachedToWindow();
};


/*********************************************************/
class DownloadSetView : public ResizeView
{
public:
	DownloadSetView(BRect frame);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	
	enum {
		S_DLPATH	= 'SDPa',
		S_RESUME	= 'SRes',
		S_AUTOLAUNCH = 'SALa'
	};
private:
	BMessenger		panelWind;
};

/*********************************************************/
class CommSetView : public ResizeView
{
public:
	CommSetView(BRect frame);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
//	virtual void	Draw(BRect r);
//	virtual void	Pulse();
	
//			void	ListenerStatus(bool running);

	enum {
//		LISTENER_STOP	= 		'LStp',
//		LISTENER_RESTART	= 	'LRst',
//		LISTENER_AUTO	= 		'LAut',
		S_SET_PROXY		=		'SPrx',
		S_SET_PROXYPORT =		'SPrP'
	};
private:
	bool			fDaemonUp;
};

/*********************************************************/
class InstallSetView : public ResizeView
{
public:
	InstallSetView(BRect frame);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	virtual void	Draw(BRect r);
	enum {
		S_AUTOLOG	= 'SALo',
		S_LOGPATH	= 'SLoP',
		S_USEINSTPATH = 'SUiP',
		S_INSTPATH	= 'SInP',
		S_IPREVIEW	= 'SIPr',
		S_LIBTOSYS	= 'SLts',
		S_LIBTOAPP	= 'SLta'
	};
private:
	BMessenger	logPanel;
	BMessenger	instPanel;
};



/*********************************************************/
class RegisterSetView : public ResizeView
{
public:
	RegisterSetView(BRect frame);
	
	virtual void	Draw(BRect r);
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	
	enum {
		S_REGMODE		= 'SRMo',
		S_REGINFO		= 'SRIn'
	};
private:
	void			SetupValues();
	BRect srect;
};

/*********************************************************/
class UpdateSetView : public ResizeView
{
public:
	UpdateSetView(BRect frame);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	
			void	SetHourEnabled(bool state);
			void	SetDayEnabled(bool state);
			void	SetupDefaults();
	enum {
		S_CHECK_FREQ	= 'CFre',
		S_SELECT_DAY	= 'SDay',
		S_SELECT_HOUR	= 'STim',
		S_SELECT_AMPM	= 'SAmP',
		S_SET_SERVER	= 'SSrv'
		//S_AUTODL		= 'SADl',
		//S_AUTOINST		= 'SAIn'
	};
	enum {
		EVERY_HOUR, EVERY_DAY, EVERY_WEEK, CHECK_NEVER
	};
private:
	int				curHr;
	bool			pmHr;
};

/*********************************************************/
class UninstallSetView : public ResizeView
{
public:
	UninstallSetView(BRect frame);
	
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *);
	
	enum {
		S_DELETEMODE	= 'SDMo',
		S_UNINSTARC		= 'SUar'
	};
};


bool GetBoolControlValue(BMessage *m);

#endif
