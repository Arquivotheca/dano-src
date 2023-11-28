/***************************************************************************
//
//	File:			NodeMonitor.h
//
//	Description:	The Node Monitor
//
//	Copyright 1992-98, Be Incorporated, All Rights Reserved.
//
***************************************************************************/


#ifndef _STORAGE2_NODEMONITOR_H
#define _STORAGE2_NODEMONITOR_H

#include <storage2/StorageDefs.h>
#include <storage2/Node.h>

namespace B {
namespace Storage2 {

/*
status_t watch_node(const node_ref *node, 
					   uint32 flags, 
					   BMessenger target);

status_t watch_node(const node_ref *node, 
					   uint32 flags, 
					   const BHandler *handler,
					   const BLooper *looper = NULL);

status_t stop_watching(BMessenger target);

status_t stop_watching(const BHandler *handler, 
					  const BLooper *looper=NULL);
*/
/* Flags for the watch_node() call.
 * Note that B_WATCH_MOUNT is NOT included in 
 * B_WATCH_ALL -- the latter is a convenience
 * for watching all watchable state changes for 
 * a specific node.  Watching for volumes
 * being mounted and unmounted (B_WATCH_MOUNT)
 * is in a category by itself. 
 *
 * BVolumeRoster provides a handy cover for
 * volume watching.
 */
enum {
    B_STOP_WATCHING = 0x0000,
	B_WATCH_NAME =	0x0001,
	B_WATCH_STAT =	0x0002,
	B_WATCH_ATTR =	0x0004,
	B_WATCH_DIRECTORY =	0x0008,
	B_WATCH_ALL = 0x000f,
	B_WATCH_MOUNT = 0x0010
};

/* When a node monitor notification
 * returns to the target, the "opcode" field 
 * will hold one of these values.  The other
 * fields in the message tell you which
 * node (or device) changed.  See the documentation
 * for the names of these other fields.
 */
#define		B_ENTRY_CREATED		1
#define		B_ENTRY_REMOVED		2
#define		B_ENTRY_MOVED		3
#define		B_STAT_CHANGED		4
#define		B_ATTR_CHANGED		5
#define		B_DEVICE_MOUNTED	6
#define		B_DEVICE_UNMOUNTED	7

} } // namespace B::Storage2

#endif	// _STORAGE2_NODEMONITOR_H
