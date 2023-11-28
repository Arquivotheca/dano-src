/*	$Id: DRuntimeSupport.h,v 1.1 1998/11/17 12:16:39 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 11/16/98 21:42:59
*/

#ifndef DRUNTIMESUPPORT_H
#define DRUNTIMESUPPORT_H

#include "DTypes.h"

#include <vector>

class DThread;
class DTeam;

struct DRuntimeInfo
{
	ptr_t		addr;
	int			action;
	int			info;
};

typedef std::vector<DRuntimeInfo> DRuntimeSupportList;

class DStateMachine
{
  public:
	DStateMachine() : fState(0), fDone(false) {}
	virtual ~DStateMachine();
	
	virtual EDebugAction NextAction (DThread& thread) = 0;
	
	bool IsDone () const	{ return fDone; }

  protected:
	int fState;
	bool fDone;
};

void CreateRuntimeList(DTeam& team, DRuntimeSupportList& lst);
DStateMachine* CheckRuntimeList(DThread& thread, DRuntimeSupportList& lst);

#endif
