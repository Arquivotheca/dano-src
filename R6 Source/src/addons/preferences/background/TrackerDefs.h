//*****************************************************************************
//
//	File:		 TrackerDefs.h
//
//	Description: Tracker constants header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined TRACKERDEFS_INCLUDED
#define TRACKERDEFS_INCLUDED

// attribute name
extern const char *kBackgroundImageInfo;

// BMessage entries
extern const char *kBackgroundImageInfoPath;		// string path
extern const char *kBackgroundImageInfoMode;		// int32
extern const char *kBackgroundImageInfoOffset;		// BPoint
extern const char *kBackgroundImageInfoEraseText;	// bool
extern const char *kBackgroundImageInfoWorkspaces;	// uint32

enum Mode {
	kAtOffset,
	kCentered,			// only works on Desktop (maybe I'll make it work for normal windows too)
	kScaledToFit,		// only works on Desktop
	kTiled
};

#endif
