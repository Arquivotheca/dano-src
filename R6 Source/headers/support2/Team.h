
#ifndef _SUPPORT2_TEAM_H
#define _SUPPORT2_TEAM_H

#include <kernel/OS.h>
#include <support2/SupportDefs.h>
#include <support2/Atom.h>
#include <support2/IBinder.h>
#include <support2/Locker.h>
#include <support2/Vector.h>
#include <support2/Root.h>

namespace B {
namespace Support2 {

class BHandler;

typedef IBinder::ptr (*root_object_func)(BValue &params);

extern int32 this_team();
extern atom_ptr<IBinder> load_object_remote(const char *pathname,
											const BValue& params = BValue::undefined);
extern atom_ptr<IBinder> load_object(	const char *pathname,
										const BValue& params = BValue::undefined);

/*----------------------------------------------------------------*/

class BTeam : virtual public BAtom
{
public:
	B_STANDARD_ATOM_TYPEDEFS(BTeam)

								BTeam(team_id tid);

			team_id				ID() const;
			
			void				DispatchMessage();

			IBinder::ptr		GetProxyForHandle(int32 handle);
			IBinder::ref		GetProxyRefForHandle(int32 handle);
			void				ExpungeHandle(int32 handle);
			
protected:
	virtual						~BTeam();
	virtual	status_t			Acquired(const void* id);
	virtual	status_t			Released(const void* id);

private:

	friend	class BHandler;
	
	static	bool					ResumingScheduling();
	static	void					ClearSchedulingResumed();
	static	bool					InsertHandler(BHandler **handlerP, BHandler *handler);
			void					UnscheduleHandler(BHandler* h, bool lock=true);
			void					ScheduleNextEvent();
			void					ScheduleHandler(BHandler* h);
			IBinder* & 				BinderForHandle(int32 handle);
			
			const team_id			m_id;
			BLocker					m_lock;
			BHandler*				m_pendingHandlers;
			bigtime_t				m_nextEventTime;
			BVector<IBinder*>		m_handleRefs;
			BNestedLocker			m_handleRefLock;
};

} } // namespace B::Support2

extern "C" B::Support2::IBinder::ptr root(const B::Support2::BValue &params);

#endif /* _SUPPORT2_TEAM_H */
