#if ! defined ACTIVITY_INCLUDED
#define ACTIVITY_INCLUDED

#include <SupportDefs.h>
#include <OS.h>

class BWindow;

class Activity
{
public:
	virtual			~Activity() {}
	virtual void	Start() = 0;
	virtual void	Next() = 0;
	virtual void	Stop() = 0;
};

class DPMSActivity : public Activity
{
	static BWindow *black;
	uint32	st;

public:
			DPMSActivity(uint32 state);
	void	Start();
	void	Next();
	void	Stop();
};

class BLooper;

class ModuleActivity : public Activity
{
	team_id	team;
	BLooper	*dd;

public:
			ModuleActivity();
	void	Start();
	void	Next() { Stop(); }
	void	Stop();
};

class BlankWindow;

class ModuleRunActivity : public Activity
{
	BlankWindow	*win;

public:
			ModuleRunActivity();
	void	Start();
	void	Next() { Stop(); }
	void	Stop();
};

#endif
