//*****************************************************************************
//
//	File:			TheMightyNodeMonitor.cpp
//
//	Written by:		Sam and Janet Evening
//
//	Copyright 1997, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************
#pragma once

#include <NodeMonitor.h>
#include <Messenger.h>
#include <OS.h>
#include <fs_info.h>

#include <system_calls.h>
#include <token.h>
#include <message_util.h>

#include <Handler.h>
#include <Looper.h>

#include "Storage.h"

#define     PREFERRED_TOKEN (0xFFFFFFFE)

status_t BNodeMonitor::watch_cover(const node_ref *node, 
				   node_monitor_flags flags, 
				   bool vol,
				   bool start)
{
	status_t err1, err2;

	if ((flags == 0) && !vol) 
	  return B_BAD_VALUE;

	if (!fMsngr->IsValid()) {
	  return B_ERROR;
	}

	const port_id port = _get_messenger_port_(*fMsngr);
	const int32 token = _get_messenger_preferred_(*fMsngr)
						? PREFERRED_TOKEN : _get_messenger_token_(*fMsngr);
	if (port < B_OK)
		return port;
	
	if (start) {
	  if (vol)
		err1 = _kstart_watching_vnode_(-1, -1, 0, port, token);

	  if (flags)
		err2 = _kstart_watching_vnode_(node->device, node->node, 
									   flags, port, token);
	}
	else {
	  if (vol)
		err1 = _kstop_watching_vnode_(-1, -1, port, token);
	  if (flags)
		err2 = _kstop_watching_vnode_(node->device, node->node, port, token);
	}
	
	return (err2)?err2:err1;
}


BNodeMonitor::BNodeMonitor()
{
}

BNodeMonitor::~BNodeMonitor()
{
  delete fMsngr;
}

void BNodeMonitor::SetMessenger(const BMessenger msngr)
{
  BMessenger *ptr = fMsngr;
  fMsngr = new BMessenger(msngr);
  delete ptr;
}

BMessenger BNodeMonitor::Messenger() const
{
  return *fMsngr;
}

status_t BNodeMonitor::StartWatchingNode(const node_ref *node, 
										 node_monitor_flags flags)
{
  return watch_cover(node, flags, false, true);
}

status_t BNodeMonitor::StopWatchingNode(const node_ref * node)
{
  return watch_cover(node, B_WATCH_ALL, false, false);
}

status_t BNodeMonitor::StartWatchingVolumes()
{

  return watch_cover((node_ref *)NULL, 0, true, true);
}

status_t BNodeMonitor::StopWatchingVolumes()
{

  return watch_cover((node_ref *)NULL, 0, true, false);
}
