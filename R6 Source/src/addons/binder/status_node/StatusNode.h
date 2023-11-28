/*****************************************************************************
 *	Filename: StatusNode.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Description: 
 *****************************************************************************/

#ifndef _STATUSNODE_H_INCLUDED_
#define _STATUSNODE_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <String.h>
#include <Binder.h>
#include <List.h>
#include <dirent.h>

#include "DeviceStatusProtocol.h"
#include "StatusProperty.h"

#define STATUSNODE_LED_ON			0x00000001
#define STATUSNODE_LED_OFF			0x00000000
#define STATUSNODE_NUM_LEDS		3

enum LEDS{
	LED_MAIL,
	LED_NETWORK,
	LED_POWER
};

class StatusNode : public BinderNode
{
public:
								StatusNode();
	virtual						~StatusNode();

protected:

	struct store_cookie {
		int32 index;
	};

	virtual	status_t			OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t			NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t			CloseProperties(void *cookie);
	
	virtual	put_status_t		WriteProperty(const char *name, const property &prop);
	virtual	get_status_t		ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

private:
	binder_node mContainer;
	
	BList mProperties;
		
	void FindDevices(BString path);
	void RegisterDevice(BString devpath);
};

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new StatusNode();
}

#endif //_STATUSNODE_H_INCLUDED_

