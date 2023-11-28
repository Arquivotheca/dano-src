#if !defined(_TIMEDALERT_H_)
#define _TIMEDALERT_H_

#include <Alert.h>

class TTimedAlert : public BAlert
{
public:
						TTimedAlert(
								bigtime_t	interval,
								int32		button,
								const char *title,
								const char *text,
								const char *button1,
								const char *button2 = NULL,
								const char *button3 = NULL,
								button_width width = B_WIDTH_AS_USUAL,
								alert_type type = B_INFO_ALERT);
						TTimedAlert(
								bigtime_t	interval,
								int32		button,
								const char *title,
								const char *text,
								const char *button1,
								const char *button2,
								const char *button3,
								button_width width,
								button_spacing spacing,
								alert_type type = B_INFO_ALERT);
	virtual				~TTimedAlert();
	virtual	void		Show();
	virtual	void		MessageReceived(BMessage *msg);
	void				Stop(int32 stopbutton);

private:

	bigtime_t		mInterval;
	int32			mTimeOutButton;
	char			*txt;
	int32			curr;
};

#endif
