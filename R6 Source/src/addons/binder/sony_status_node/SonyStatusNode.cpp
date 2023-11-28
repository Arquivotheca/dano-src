/*****************************************************************************
 *	Filename: SonyStatusNode.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *
 *  Description: 
 *****************************************************************************/
 
#include "SonyStatusNode.h"
#if DEBUG
#include <string.h>
#endif /* #if DEBUG */
#include <Debug.h>

#define BYLINE "SonyStatusNode"

SonyStatusNode::SonyStatusNode()
	: BinderNode(), m_powerLedState(NetronDisplay::POWER_ON),
		mContainer(0), mNetronDisplay(NULL)
{
	mContainer = new BinderContainer();
	mNetronDisplay = new NetronDisplay(-1);
	
	mLedState_Mail = 0;

	if (mNetronDisplay->InitCheck() == B_OK) {
		mNetronDisplay->SetRegister((uint8)0x0c, LEDNODE_LED_OFF);
	}	
	else
	{
		delete mNetronDisplay;
		mNetronDisplay = NULL;
		SERIAL_PRINT(("SonyStatusNode: failed to initialize!\n"));
	}
}

SonyStatusNode::~SonyStatusNode()
{
	delete mNetronDisplay;
}

status_t SonyStatusNode::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) {
		*c = *((store_cookie*)copyCookie);
	} else {
		c->index = 0;
	}
	*cookie = c;
	return B_OK;
}

status_t SonyStatusNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;
	
	int numProperties = LEDNODE_NUM_LEDS;

	if (c->index < numProperties) {	
		const char *name = properties[c->index];
		strncpy(nameBuf, name, *len);
		*len = strlen(name);
		c->index++;
		err = B_OK;
	}
	
	return err;
}

status_t SonyStatusNode::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

put_status_t SonyStatusNode::WriteProperty(const char *name, const property &prop)
{
	status_t rtnvalue = ENOENT;
	
	int propvalue = (int)prop.Number();
	if (strcmp(name, "email") == 0) 
	{
		bool turnon = false;
		if(propvalue != 0) turnon = true;
		
		uint8 setvalue = (turnon ? LEDNODE_LED_ON : LEDNODE_LED_OFF);
		
		//_sPrintf("SonyStatusNode: attempting to turn %s the email LED\n", turnon ? "on" : "off");
		if(mNetronDisplay && mNetronDisplay->SetRegister((uint8)0x0c, setvalue) == B_NO_ERROR)
		{
			//_sPrintf("\tsuccess!\n");
			mLedState_Mail = turnon ? 1 : 0;
			rtnvalue = B_OK;
		} else {
			//_sPrintf("\tfailed!\n");
			rtnvalue = B_ERROR;
		}
	}
	
	return rtnvalue;
}

get_status_t SonyStatusNode::ReadProperty(const char *name, property &prop, const property_list &)
{
	status_t rtnvalue = ENOENT;
	
	prop.Undefine();
	
	if (strcmp(name, "email") == 0) 
	{
		prop = property(mLedState_Mail);	
		rtnvalue = B_OK;
	}
	else if (strcmp(name, "power") == 0) {
		if (mNetronDisplay != NULL) {
			rtnvalue = mNetronDisplay->GetPowerState(&m_powerLedState);
			if (rtnvalue == B_OK) {
				switch (m_powerLedState) {
					case NetronDisplay::POWER_ON:
						prop = "on";
						break;
					case NetronDisplay::POWER_OFF:
						prop = "off";
						break;
					case NetronDisplay::POWER_STANDBY:
						prop = "standby";
						break;
				}
			} else {
				SERIAL_PRINT(("NetronDisplay::GetPowerState() returned %s\n",
					strerror(rtnvalue)));
			}
		}
	}	
	
	return rtnvalue;
}

