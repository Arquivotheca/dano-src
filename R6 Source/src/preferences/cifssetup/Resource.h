//  Resource.h
//
//	russ 1/21/99
//
//	(c) 1997-99 Be, Inc. All rights reserved

#ifndef	_RESOURCE_H
#define	_RESOURCE_H

enum Message_Map {	MSG_DONE = 1000,
					MSG_CANCEL,
 					MSG_PASSWORD,
 					MSG_PASSWORD_TEXT,
 					MSG_CONFIRM_PSWD_TEXT,
 					MSG_CIFSENABLE,
					MSG_HIDESHARES,
					MSG_UPDATE_CONTROLS};

const int kMaxStrLen = 512;
const int kTabHeight = 22;
const int kButtonHeight	= 20;
const int kFillControls	= 0;
const int kEmptyControls =	1;
const int kMaxAddOns = 100; 
const int kSuccess = 1;
const int kFail = -1;
const int kCommandButtonWidth = 75; // UI guidelines for width


#endif //_RESOURCE_H


