#include "Thread.h"
#include "FunctionObjectMsg.h"

SimpleThread::SimpleThread(int32 priority, const char *name)
	:	scanThread(-1),
		priority(priority),
		name(name)
{
}


SimpleThread::~SimpleThread()
{
	if (scanThread > 0 && scanThread != find_thread(NULL)) 
		// kill the thread if it is not the one we are running in
		kill_thread(scanThread);
}

void 
SimpleThread::Go()
{
	scanThread = spawn_thread(SimpleThread::RunBinder, name ? name : "TrackerTaskLoop",
		priority, this);
	resume_thread(scanThread);
}

status_t 
SimpleThread::RunBinder(void *castToThis)
{
	SimpleThread *self = (SimpleThread *)castToThis;
	self->Run();
	return B_NO_ERROR;
}

void 
Thread::Launch(FunctionObject *functor, int32 priority, const char *name)
{
	new Thread(functor, priority, name);
}


Thread::Thread(FunctionObject *functor, int32 priority, const char *name)
	:	SimpleThread(priority, name),
		functor(functor)
{
	Go();
}

Thread::~Thread()
{
	delete functor;
}


void 
Thread::Run()
{
	(*functor)();
	delete this;
		// commit suicide
}

void 
ThreadSequence::Launch(BObjectList<FunctionObject> *list, bool async, int32 priority)
{
	if (!async)
		// if not async, don't even create a thread, just do it right away
		Run(list);
	else
		new ThreadSequence(list, priority);
}


ThreadSequence::ThreadSequence(BObjectList<FunctionObject> *list, int32 priority)
	:	SimpleThread(priority),
 		functorList(list)
{
	Go();
}


ThreadSequence::~ThreadSequence()
{
	delete functorList;
}

void 
ThreadSequence::Run(BObjectList<FunctionObject> *list)
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(*list->ItemAt(index))();
}

void 
ThreadSequence::Run()
{
	Run(functorList);
	delete this;
		// commit suicide
}
