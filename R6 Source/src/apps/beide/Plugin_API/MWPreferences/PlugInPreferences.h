//========================================================================
//	PlugInPreferences.h
//	Copyright 1996 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _PLUGINPREFERENCES_H
#define _PLUGINPREFERENCES_H

#include <Rect.h>
#include <GraphicsDefs.h>

enum UpdateType {
	kNoUpdate = 0,
	kCompileUpdate = 1,
	kLinkUpdate = 2
};


// Targets of the plugin panel
// usually this will be only kMWDefaults + kCurrentProject
typedef ulong TargetT;
enum {
	kNoTarget			= 0,
	kMWDefaults			= 1,
	kCurrentProject		= 2,
	kMessageWindow		= 4,
	kTextWindows		= 8
};

const TargetT kInvalidTarget = 0xffffffff;

// The stages of Make at which the tool can act 
typedef ulong MakeStageT;
enum {
	kIgnoreStage		= 0,
	kPrecompileStage	= 1,
	kCompileStage		= 2,
	kLinkStage			= 4,
	kPostLinkStage		= 8
};

const MakeStageT kInvalidStage = 0xffffffff;

// Actions that can be taken during Make
typedef ulong MakeActionT;
enum {
	kPrecompile			= 1,
	kCompile			= 2,
	kLink				= 4,
	kPostLinkExecute	= 8,
	kPreprocess			= 16,
	kCheckSyntax		= 32,
	kDisassemble		= 64
};

const MakeActionT kInvalidAction = 0xffffffff;

// Flags 
typedef ulong PlugInFlagsT;
enum {
	kNotIDEAware		= 0,
	kIDEAware			= 1,
	kUsesFileCache		= 2
};

class MPlugInPrefsView;
class MPlugInBuilder;
class MPlugInLinker;

// These types are used by the MW compiler and linker
// prefs panels, so shouldn't be used by other prefs panels
// Type of the settings data to be added to BMessages
// in GetData, SetData, and ValidateSettings
const ulong kMWCCType = 'mwcc';
const ulong kMWLDType = 'mwld';

// Messages the Settings window will understand when sent
// from a plug-in view
const ulong msgPluginViewModified = 20000;

// Prototype of the functions that must be exported by the add-on
// parameters are index, bounds rect, and view or builder
typedef status_t (*makeaddonView)(int32, BRect, MPlugInPrefsView*&);
typedef status_t (*makeaddonBuilder)(int32, MPlugInBuilder*&);
typedef status_t (*makeaddonLinker)(MPlugInLinker*&);

// This is the grey used in the Settings window
const rgb_color kPrefsGray = { 235, 235, 235, 255 };

#endif
