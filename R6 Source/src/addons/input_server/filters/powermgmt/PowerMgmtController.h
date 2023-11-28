#if ! defined POWERMGMTCONTROLLER_INCLUDED
#define POWERMGMTCONTROLLER_INCLUDED

#define PM_DELAY 100		/*delay before PM is triggered */

/*packages*/
#define DT300	1

#include <Node.h>
#include <Path.h>
#include <Message.h>
#include <Looper.h>
#include <OS.h>

class BMessageRunner;
class MonitorView;
class BWindow;

class PowerMgmtController : public BLooper
{
	volatile bigtime_t	last;
	volatile bigtime_t	lastwake;
	volatile bool		saving;
	int32				fadetime;
	int32				pkg;
	bigtime_t			start;
	BMessageRunner		*idlecheck;
	sem_id				saving_sem;
	
public:
						PowerMgmtController();

	virtual	void		MessageReceived(BMessage *msg);
	virtual bool		QuitRequested();

	bool				ProcessInput(BMessage *message);
	bool				Power_Off_Devs (int32 pkg);
	void				Power_On_Devs (int32 pkg);
	

};

#endif
