// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _NODE_WATCHER_H_
#define _NODE_WATCHER_H_

#include <SupportDefs.h>
#include <Node.h>
#include <Entry.h>
#include <Looper.h>
#include <NodeMonitor.h>

class NodeWatcher : public BHandler
{
public:
			NodeWatcher(BLooper *looper);
	virtual ~NodeWatcher();

	virtual status_t StartWatching(const node_ref& nref, uint32 flags);
	virtual status_t StopWatching(const node_ref& nref);

protected:
	virtual void MessageReceived(BMessage *message);

private:
	virtual void EntryCreated(entry_ref& eref, node_ref& nref);
	virtual void EntryRemoved(node_ref& parent_eref, node_ref& nref);
	virtual void EntryMoved(entry_ref& src_eref, entry_ref& dst_eref, node_ref& nref);
	virtual void StatChanged(node_ref& nref);
	virtual void AttributeChanged(node_ref& nref);
	virtual void DeviceMounted(dev_t ndevice, node_ref& mntpnt_nref);
	virtual void DeviceUnmounted(dev_t device);
};

#endif
