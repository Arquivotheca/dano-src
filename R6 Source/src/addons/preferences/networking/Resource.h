//  Resource.h
//
//	russ 5/21/98
// 
//	(c) 1997 Be, Inc. All rights reserved

#ifndef	_RESOURCE_H
#define	_RESOURCE_H

#include <SupportDefs.h>

enum Message_Map {	MSG_DONE = 1000,
					MSG_CANCEL,
					MSG_RESTARTNETWORKING, 		
					MSG_REVERT,	
					MSG_FILLCONTROLS,
					MSG_EMPTYCONTROLS,					
 					MSG_SAVE,
 					MSG_LISTCHANGE,
 					MSG_LISTDBL_CLICK,
 					MSG_BKLISTCHANGE,
 					MSG_BKLISTDBL_CLICK,
 					MSG_ADD_ETHER_BUTTON,
 					MSG_ADD_BUTTON,
 					MSG_REMOVE,
 					MSG_PROPERTIES_BUTTON,
 					MSG_BACKUP,
 					MSG_PREFERRED,
 					MSG_ENABLE,
 					MSG_CONFIG,
 					MSG_DHCP,
 					MSG_PASSWORD,
 					MSG_PASSWORD_TEXT,
 					MSG_CONFIRM_PSWD_TEXT,
 					MSG_DIRTY};

const int kMaxStrLen = 512;
const int kTabHeight = 22;
const int kButtonHeight	= 20;
const int kFillControls	= 0;
const int kEmptyControls =	1;
const int kMaxNetDevScan = 100; 
const int kAddInterface = 0;
const int kModifyInterface = 1;
const int kSuccess = 1;
const int kFail = -1;
const int kCommandButtonWidth = 75; // UI guidelines for width
const int32 kPanelViewWidth = 380;
const int32 kPanelViewHeight = 300;

#endif //_RESOURCE_H
