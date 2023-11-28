/*****************************************************************************
 *	Filename: SonyStatusNode.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Author: Myron W. Walker  >> others... see change log at end of file...
 *
 *  Description: 
 *****************************************************************************/

#ifndef _INCLUDED_SONYSTATUSNODE_H_
#define _INCLUDED_SONYSTATUSNODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <String.h>
#include <Binder.h>

#include "NetronDisplay.h"

#define LEDNODE_LED_ON			(uint8)0x01
#define LEDNODE_LED_OFF			(uint8)0x07
#define LEDNODE_NUM_LEDS		2

// NOTE: these properties need to stay in alphabetical order.  If a new property
// is inserted in this list, code to handle it should be added to ReadProperty().
static const char*	properties[] = {
						"email",
						"power"
};

//if(display_control->SetRegister(0xc, down ? 7 : 1) != B_NO_ERROR)

class SonyStatusNode : public BinderNode
{
public:
								SonyStatusNode();
	virtual						~SonyStatusNode();

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
	int mLedState_Mail;
	NetronDisplay::power_state m_powerLedState;
	
	binder_node mContainer;
	NetronDisplay *mNetronDisplay;	
};

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new SonyStatusNode();
};

#endif //_INCLUDED_SONYSTATUSNODE_H_
