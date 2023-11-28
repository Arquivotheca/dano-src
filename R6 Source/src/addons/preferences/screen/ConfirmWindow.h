#if ! defined CONFIRMWINDOW_INCLUDED
#define CONFIRMWINDOW_INCLUDED 1

#include <View.h>
#include <Window.h>
#include <OS.h>
#include <Alert.h>

class TAlertView : public BView
{
public:
				TAlertView(BRect b, alert_type type=B_WARNING_ALERT);
				~TAlertView();
	void		AttachedToWindow();
	void		Draw(BRect);
	void		GetIcon();
private:
	alert_type	fMsgType;
	BBitmap*	fIconBits;				
};

class TPoofAlert : public BWindow
{
public:
				TPoofAlert(alert_type type=B_WARNING_ALERT);
				~TPoofAlert();
				
	void		MessageReceived(BMessage* m);	
	int32		Go();
	
	bool		Skip() { return fSkip; }
	
private:
	alert_type	fAlertType;
	TAlertView*	fBG;
		
	BTextView*	fMsgFld;
	
	BButton*	fPoofBtn;
	BButton*	fCancelBtn;
	
	BCheckBox*	fBypassWarning;
	
	sem_id		fAlertSem;
	int32		fAlertVal;
	
	bool		fSkip;
};

#endif
