//******************************************************************************
//
//	File:		DirectDriver.cpp
//
//	Description:	Private class to encapsulate init/dispose of client-side
//					graphic add-on clone.
//
//	Written by:	Pierre Raynaud-Richard
//
//	Revision history
//
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <Debug.h>

#include <messages.h>

#ifndef _DIRECT_DRIVER_H
#include <DirectDriver.h>
#endif

/*-------------------------------------------------------------*/

BDirectDriver::BDirectDriver(uint32 token)
{
	dd_token = token;
}

/*-------------------------------------------------------------*/

BDirectDriver::~BDirectDriver()
{
}

/*-------------------------------------------------------------*/

status_t BDirectDriver::GetHook(driver_hook_token token, graphics_card_hook *hook)
{
	return B_ERROR;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver::GetCookie(void **cookie)
{
	return B_ERROR;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver::Sync(bool fullscreen)
{
	return B_ERROR;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver::GetDriverInfo(void *info) {
	uint32				size;
	status_t			retval;
	_BAppServerLink_	link;

	link.session->swrite_l(GR_GET_DRIVER_INFO);
	link.session->swrite_l(dd_token);
	link.session->flush();
	link.session->sread(4, &size);
	link.session->sread(size, info);
	link.session->sread(4, &retval);
	return retval;
}

/*-------------------------------------------------------------*/

status_t BDirectDriver::GetSyncInfo(void **info) {
	uint32				size;
	status_t			retval;
	_BAppServerLink_ 	link;

	link.session->swrite_l(GR_GET_SYNC_INFO);
	link.session->swrite_l(dd_token);
	link.session->flush();
	link.session->sread(4, &size);
	if (size == 0)
		*info = NULL;
	else {
		*info = (void*)malloc(size);
		link.session->sread(size, *info);
	}
	link.session->sread(4, &retval);
	return retval;
}


