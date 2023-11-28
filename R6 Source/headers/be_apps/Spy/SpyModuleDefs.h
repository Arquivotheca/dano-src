// --------------------------------------------------------------------------- 
/* 
	SpyModuleDefs.h 
	 
	Copyright (c) 2000 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			November 21 
 
	Public SpyModule definitions 
*/ 
// --------------------------------------------------------------------------- 
 
#ifndef SPYMODULEDEFS_H
#define SPYMODULEDEFS_H

#define B_SPY_DISPLAY_LISTEN_PORT 	4139
#define B_SPY_MONITOR_LISTEN_PORT 	4138

//! Message constants that you can subscribe to. They should be pretty self eplanatory.
#define B_SPY_ALL_MODULES_LOADED		'_AML'
#define B_SPY_NO_TARGET					'_SNT'
#define B_SPY_NOTIFICATION				'_SNT'
#define B_SPY_PING_REMOTE				'_SPR'
#define B_SPY_REMOTE_TARGET_INVALID		'_RTI'
#define B_SPY_REMOTE_TARGET_GOOD		'_RTG'
#define B_SPY_SUBSCRIBE					'_SSS'

#define B_SPY_ERROR						"be:Spy-O-Matic.ErrorCode"
#define B_SPY_MESSENGER_ERROR			"be:Spy-O-Matic.MessengerErr"
#define B_SPY_MODULE_NAME				"be:Spy-O-Matic.ModuleName"
#define B_SPY_ORIGINAL_WHAT				"be:Spy-O-Matic.OriginalWhat"
#define B_SPY_REMOTE_TARGET_NAME		"be:Spy-O-Matic.RemoteTargetName"
#define B_SPY_SUBSCRIBER_MESSENGER		"be:Spy-O-Matic.SubscriberMessenger"
#define B_SPY_SUBSCRIBE_TO				"be;Spy-O-Matic.SubscribeTo"

// todo: Move into more appropriate place
#define B_SPY_TOTAL_TIME				"be:Spy-O-Matic.TotalTime"
#define B_SPY_TOTAL_VALUE				"be:Spy-O-Matic.TotalValue"

#endif

