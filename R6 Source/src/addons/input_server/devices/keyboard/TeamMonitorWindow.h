#ifndef _TEAMMONITORWINDOW_H
#define _TEAMMONITORWINDOW_H

#include <Window.h>


class _TeamListView_;


class TeamMonitorWindow : public BWindow {
public:
						TeamMonitorWindow(uint8		*threadMonitor,
										  uint32	threadMonitorSize);

	void				Enable(int driver);
	void				Disable();

	bool				IsEnabled() const;

	virtual void		TeamMonitorWindow::DispatchMessage(BMessage *message, BHandler *handler);
	virtual void		MessageReceived(BMessage *message);

	virtual bool		QuitRequested(void);

private:
	int					fDriver;
	_TeamListView_*		fTeamListView;

public:
	static bool			sEnabled;
	static thread_id*	sThreadID;
	static uint8*		sThreadMonitor;
	static uint32		sThreadMonitorSize;
	static int32*		sCrazyThreadsCount;
	static int32*		sCrazyThreads;
};


#endif
