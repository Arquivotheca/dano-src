// ===========================================================================
//	MIDECommandList.h
//	
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//
// ===========================================================================

#ifndef _MIDECOMMANDLIST_H
#define _MIDECOMMANDLIST_H

#include "IDEConstants.h"

const int32		kEditorGroupID = 6;
const int32		kPrefixGroupID = 7;

struct CommandInfo
{
	CommandT		command;
	char			name[32];
	char			description[64];
};

struct CommandGroupInfo
{
	char			name[32];
	int32			count;
	CommandInfo*	commands;
};

class MIDECommandList
{
public:
	static bool							GetCommandInfo(
											CommandT		inCommand,
											CommandInfo&	outInfo);					
	static bool							GetNthCommandGroup(
											int32				inIndex,
											CommandGroupInfo&	outInfo);
	static int32						Count();
};

#endif
