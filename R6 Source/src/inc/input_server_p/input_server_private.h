/******************************************************************************
/
/	File:			input_server_private.h
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
******************************************************************************/

#ifndef _INPUT_SERVER_PRIVATE_H
#define _INPUT_SERVER_PRIVATE_H

#include <BeBuild.h>


// BInputDevice message constants
const uint32 IS_START_DEVICES		= 'Istd';
const uint32 IS_STOP_DEVICES		= 'Ispd';
const uint32 IS_CONTROL_DEVICES		= 'Icnd';
const uint32 IS_DEVICE_RUNNING		= 'Idvr';


// global function message constants
const uint32 IS_FIND_DEVICES			= 'Ifdv';
const uint32 IS_WATCH_DEVICES			= 'Iwdv';
const uint32 IS_GET_KEYBOARD_ID			= 'Igid';
const uint32 IS_SET_KEYBOARD_LOCKS		= 'Iskl';
const uint32 IS_GET_KEY_INFO			= 'Igki';
const uint32 IS_GET_MODIFIERS			= 'Igmd';
const uint32 IS_SET_MODIFIER_KEY		= 'Ismk';
const uint32 IS_GET_KEY_MAP				= 'Igkm';
const uint32 IS_SET_KEY_MAP				= 'Iskm';
const uint32 IS_GET_KEY_REPEAT_DELAY	= 'Igrd';
const uint32 IS_SET_KEY_REPEAT_DELAY	= 'Isrd';
const uint32 IS_GET_KEY_REPEAT_RATE		= 'Igrr';
const uint32 IS_SET_KEY_REPEAT_RATE 	= 'Isrr';
const uint32 IS_GET_MOUSE_MAP			= 'Igmm';
const uint32 IS_SET_MOUSE_MAP			= 'Ismm';
const uint32 IS_GET_MOUSE_TYPE			= 'Igmt';
const uint32 IS_SET_MOUSE_TYPE			= 'Ismt';
const uint32 IS_GET_MOUSE_SPEED			= 'Igms';
const uint32 IS_SET_MOUSE_SPEED			= 'Isms';
const uint32 IS_GET_MOUSE_ACCELERATION	= 'Igma';
const uint32 IS_SET_MOUSE_ACCELERATION	= 'Isma';
const uint32 IS_GET_CLICK_SPEED			= 'Igcs';
const uint32 IS_SET_CLICK_SPEED			= 'Iscs';
const uint32 IS_SET_MOUSE_POSITION		= 'Ismp';
const uint32 IS_FOCUS_IM_AWARE_VIEW		= 'Ifim';
const uint32 IS_UNFOCUS_IM_AWARE_VIEW	= 'Iuim';

const uint32 IS_REDIRECT_MOUSE_EVENTS	= 'Irme';
const uint32 IS_MOUSE_MOVED				= 'Imme';


// well-defined names
#define IS_STATUS			"status"
#define IS_DEVICE_NAME		"device"
#define IS_DEVICE_TYPE		"type"
#define IS_CONTROL_CODE 	"code"
#define IS_CONTROL_MESSAGE	"message"
#define IS_WATCH_TARGET		"target"
#define IS_WATCH_START		"start"
#define IS_KEYBOARD_ID		"id"
#define IS_KEY_LOCKS		"locks"
#define IS_KEY_INFO			"key_info"
#define IS_MODIFIERS		"modifiers"
#define IS_KEY_MAP			"keymap"
#define IS_KEY_BUFFER		"key_buffer"
#define IS_DELAY			"delay"
#define IS_RATE				"rate"
#define IS_MOUSE_MAP		"mousemap"
#define IS_MOUSE_TYPE		"mouse_type"
#define IS_SPEED			"speed"
#define IS_MODIFIER			"modifier"
#define IS_KEY				"key"
#define IS_WHERE			"where"
#define IS_VIEW				"view"


// function prototypes
status_t _control_input_server_(BMessage *command, BMessage *reply);


#endif
