/*****************************************************************************
 *	Filename: DeviceStatusProtocol.h
 *  Copyright 2001 by Be Incorporated.
 *  
 *  Description: 
 *****************************************************************************/

#ifndef _INCLUDE_DEVICESTATUSPROTOCOL_H_
#define _INCLUDE_DEVICESTATUSPROTOCOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATUS_GET_INFO			0x3530
#define STATUS_GET_NAME_INFO	0x3531
#define STATUS_GET_NAME_VALUE	0x3532
#define STATUS_SET_NAME_VALUE	0x3533

enum	//	for flags
{
	statusIsWritable = 0x1,
	statusIsReadable = 0x2
};

enum	//	for value_type
{
	typeIsNone = -1,
	typeIsNumber = 0,
	typeIsText = 1
};

typedef struct _status_get_info
{
	size_t		info_size;		//	provided by caller
	char		dev_name[16];	//	short name of device
	int32		dev_version;	//	version of the ioctl() protocol
	int32		name_count;		//	number of shortnames published
} status_get_info;

typedef struct _status_get_name_info
{
	size_t		info_size;		//	provided by caller

	int32		name_index;		//	index ("ID") of this name

	uint32		flags;			//	statusIsWritable, statusIsReadable
	int32		value_type;		//	typeIsNumber, typeIsText
	int32		min_value;		//	minimum legal value/length
	int32		max_value;		//	maximum legal value/length

	char		name[16];		//	[Is 15 characters enough? Should be.]
} status_get_name_info;

typedef struct _status_name_value
{
	size_t		info_size;		//	provided by the caller

	int32		name_index;		//	in: which name
	int32		value_type;		//	out: actual type
	int32		io_value;		//	for numbers: the number
								//	for strings, in: size of buffer
								//	for strings, out: actual size needed
	void *		storage;		//	provided by caller
} status_name_value;

#endif //_INCLUDE_DEVICESTATUSPROTOCOL_H_

