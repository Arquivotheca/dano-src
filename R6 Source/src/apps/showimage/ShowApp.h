//	ShowApp.h

#if !defined(SHOW_APP_H)
#define SHOW_APP_H

#include <Application.h>


class BFilePanel;

class ShowApp :
	public BApplication
{
public:
								ShowApp(
									const char *	signature);
								~ShowApp();

		void					RefsReceived(
									BMessage *		message);
		virtual void			ReadyToRun();
		virtual void			MessageReceived(BMessage *message);

		virtual void			AboutRequested();

private:
	friend class ShowAboutWindow;
		static int32			sWindowCount;
		static ShowAboutWindow * sAboutWindow;

		BFilePanel *			fOpen;
};

#endif /* SHOW_APP_H */

