#if ! defined ADVANCEDVIEW_INCLUDED
#define ADVANCEDVIEW_INCLUDED 1

#include <View.h>

class CMonitorControl;
class BButton;
class BTextControl;
class BCheckBox;
class BTextView;
class BMessage;
class BSlider;
class TimeSlider;
class BBox;
class BListView;
class BScreenSaver;
class ModuleListView;
class ModuleListItem;
class BFilePanel;
class ModuleRoster;
class BMessageFilter;

class AdvancedView : public BView
{
	bool			masterenable;
	BTextView		*cap1;
	BTextView		*cap2;
	CMonitorControl	*fadecorner;
	CMonitorControl *nofadecorner;
	BCheckBox		*dodpms;
	BStringView		*timedpms;
	TimeSlider		*dpms;
	BCheckBox		*dolock;
	BStringView		*timelock;
	TimeSlider		*nolock;
	BButton			*passbutton;

	// ouch, that password panel stuff
	char			m_Password[256];

	BButton*		m_okbut;
	BButton*		m_cancelbut;
	BRadioButton	*netradio;
	BRadioButton	*customradio;

	BTextControl*	m_pass1;
	BTextControl*	m_pass2;
	BTextView*		m_real1;
	BTextView*		m_real2;	

	BMessageFilter	*filter;
	bool			passchanged;

	int		check_password(void);
	char	ranchar(void);
	void	SetPassword(const char *pass);

public:
			AdvancedView(BRect frame, const char *name);
	void	MessageReceived(BMessage *msg);
	void	AllAttached();
	void	CheckDependencies();
	void	SaveState();
	void	AttachedToWindow();
	void	DetachedFromWindow();
};

#endif
