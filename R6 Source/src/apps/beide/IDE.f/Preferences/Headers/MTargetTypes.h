//========================================================================
//	MTargetTypes.h
//	Copyright 1996 - 98 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MTARGETTYPES_H
#define _MTARGETTYPES_H

#include "MList.h"

const ulong kDR8TargetVersion = 0;			// DR8
const ulong kDR9TargetVersion = 5;			// DR9
const ulong kCurrentTargetVersion = 7;		// R3

enum {
	ignoreStage,
	precompileStage,
	compileStage,
	linkStage,
	postLinkStage	
};

enum {
	noResources		= 0,
	hasResources	= 1,
	ideAware		= 2
};


struct TargetRec
{
	uchar		unused1;
	uchar		Stage;
//	short		Stage;
	ushort		Flags;
	char		Extension[8];
	char		ToolName[64];
	char		MimeType[64];
};

struct TargetRecOld
{
	ulong		FileType;
	short		Stage;
	ushort		Flags;
	char		Extension[8];
	char		ToolName[64];
};
const ulong noFileType = 0;

struct TargetPrefs;

// Prototypes
void SwapTargetsBigToHost(TargetPrefs& inPrefs);
void SwapTargetsHostToBig(TargetPrefs& inPrefs);


class MPlugInBuilder;

struct BuilderRec
{
	MPlugInBuilder*		builder;
	BuilderRec*			next;
	bool				isLinker;
};

struct ProjectTargetRec
{
	BuilderRec*		Builder;
	TargetRec		Target;
};


typedef MList<ProjectTargetRec*> ProjectTargetList;

struct TargetHeader
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	ulong		version;
	int32		count;
};

inline bool
TargetHasResources(
	ushort inFlags)
{
	return (inFlags & hasResources) != 0;
}

#endif
