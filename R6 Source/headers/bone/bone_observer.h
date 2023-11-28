/*
	bone_observer.h
	
	BONE AF_NOTIFY observation constants
	
	Copyright 2001, Be Incorporated, All Rights Reserved.
*/

#ifndef H_BONE_OBSERVER
#define H_BONE_OBSERVER

#ifdef __cplusplus
extern "C" {
#endif


/*
 * "who" changed state
 */
#define BONE_NULL_EVENT								0
#define BONE_NOTIFY_INTERFACE 						1
#	define BONE_NOTIFY_INTERFACE_DATALEN			sizeof(uint32)   /* if_index send on interface notifies */
#define BONE_NOTIFY_DEFAULTROUTE					2 /* no data sent on notify */


/*
 * "what" happened
 */

#define BONE_ITEM_ADDED				0
#define BONE_ITEM_DELETED			1
#define BONE_ITEM_CHANGED			2

/*
 * this structure is written to an AF_NOTIFY socket to start observing
 * for event (and using additional data up to len as a filtering key)
 */
typedef struct obsdata
{
	uint32 event;
	size_t len;
	/* data follows, up to min of len and BONE_MAX_NOTIFY_DATA */
} obsdata_t;

/*
 * this structure is read from an AF_NOTIFY socket when the event
 * being watched for occurs.  event is the event that triggered
 * the notify; what is the "what" code above; len is the len
 * of additional key data, and the data itself follows.
 */

typedef struct notifydata
{
	uint32 event;
	uint32 what;
	size_t len;
	/* data follows, min of len and BONE_MAX_NOTIFY_DATA */
} notifydata_t;

#define BONE_MAX_NOTIFY_DATA 1024 /* heinously large, no one should ever send this much */
#define BONE_NOTIFY_MOD_NAME "network/bone_notify"

#ifdef __cplusplus
}
#endif

#endif
