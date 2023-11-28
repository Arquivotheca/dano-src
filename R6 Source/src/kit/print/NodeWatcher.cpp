#include <stdio.h>

#include "NodeWatcher.h"

#if 0
#	define D(_x)	_x
#else
#	define D(_x)
#endif

#define bug		printf

NodeWatcher::NodeWatcher(BLooper *looper)
	:	BHandler("NodeMonitor")
{
	if (looper->Lock())
	{
		looper->AddHandler(this);		
		looper->Unlock();
	}
}

NodeWatcher::~NodeWatcher()
{
	BLooper *looper = Looper();
	if (looper->Lock())
	{ // We are still attached to a looper - detach us
		looper->RemoveHandler(this);	
		looper->Unlock();
	}
}

status_t NodeWatcher::StartWatching(const node_ref& nref, uint32 flags)
{
	status_t err = B_ERROR;
	if (LockLooper())
	{
		D(bug("StartWatching [%lX:%LX]\n", nref.device, nref.node));
		err = watch_node(&nref, flags, this, Looper());
		UnlockLooper();
	}
	return err;
}

status_t NodeWatcher::StopWatching(const node_ref& nref)
{
	status_t err = B_ERROR;
	if (LockLooper())
	{
		D(bug("StopWatching [%lX:%LX]\n", nref.device, nref.node));
		err = watch_node(&nref, B_STOP_WATCHING, this, Looper());
		UnlockLooper();
	}
	return err;
}

void NodeWatcher::MessageReceived(BMessage *m)
{
	if (m->what != B_NODE_MONITOR)
	{
		BHandler::MessageReceived(m);
		return;
	}
	
	int32 opcode;
	if (m->FindInt32("opcode", &opcode) != B_OK)
		return;

	dev_t dev = m->FindInt32("device");
	
	if (opcode == B_ENTRY_CREATED)
	{
		ino_t dir = m->FindInt64("directory");
		ino_t node = m->FindInt64("node");
		const char *name = m->FindString("name");		
		entry_ref eref(dev, dir, name);
		node_ref nref;
		nref.device = dev;
		nref.node = node;
		EntryCreated(eref, nref);
	}
	else if (opcode == B_ENTRY_REMOVED)
	{
		ino_t dir = m->FindInt64("directory");
		ino_t node = m->FindInt64("node");
		node_ref parent_nref;
		parent_nref.device = dev;
		parent_nref.node = dir;
		node_ref nref;
		nref.device = dev;
		nref.node = node;
		EntryRemoved(parent_nref, nref);
	}
	else if (opcode == B_ENTRY_MOVED)
	{
		const char *name = m->FindString("name");		
		ino_t sdir = m->FindInt64("from directory");
		ino_t ddir = m->FindInt64("to directory");
		ino_t node = m->FindInt64("node");
		entry_ref src_eref(dev, sdir, name);
		entry_ref dst_eref(dev, ddir, name);
		node_ref nref;
		nref.device = dev;
		nref.node = node;
		EntryMoved(src_eref, dst_eref, nref);
	}
	else if (opcode == B_STAT_CHANGED)
	{
		ino_t node = m->FindInt64("node");
		node_ref nref;
		nref.device = dev;
		nref.node = node;
		StatChanged(nref);
	}
	else if (opcode == B_ATTR_CHANGED)
	{
		ino_t node = m->FindInt64("node");
		node_ref nref;
		nref.device = dev;
		nref.node = node;
		AttributeChanged(nref);
	}
	else if (opcode == B_DEVICE_MOUNTED)
	{
		dev_t ndev = m->FindInt32("new device");
		ino_t dir = m->FindInt64("directory");
		node_ref mntpnt_nref;
		mntpnt_nref.device = dev;
		mntpnt_nref.node = dir;
		DeviceMounted(ndev, mntpnt_nref);
	}
	else if (opcode == B_DEVICE_UNMOUNTED)
	{
		DeviceUnmounted(dev);
	}
}

void NodeWatcher::EntryCreated(entry_ref& eref, node_ref& nref)
{
	D(bug("ENTRY_CREATED\n"));
}

void NodeWatcher::EntryRemoved(node_ref& parent_nref, node_ref& nref)
{
	D(bug("ENTRY_REMOVED\n"));
}

void NodeWatcher::EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref)
{
	D(bug("ENTRY_MOVED\n"));
}

void NodeWatcher::StatChanged(node_ref& nref)
{
	D(bug("STAT_CHANGED\n"));
}

void NodeWatcher::AttributeChanged(node_ref& nref)
{
	D(bug("ATTRIBUTE_CHANGED\n"));
}

void NodeWatcher::DeviceMounted(dev_t ndevice, node_ref& mntpnt_nref)
{
	D(bug("DEVICE_MOUNTED\n"));
}

void NodeWatcher::DeviceUnmounted(dev_t device)
{
	D(bug("DEVICE_UNMOUNTED\n"));
}
