// ===========================================================================
//	PluginSupport.h
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#ifndef PLUGINSUPPORT_H
#define PLUGINSUPPORT_H

#include <image.h>
#include <Message.h>
#include "NetPositivePlugins.h"
#include "URL.h"

void InitPlugins();
void KillPlugins();

class BMessenger;
class UResourceImp;
class BView;

BView *InstantiatePlugin(const char *mimeType, BMessage *parameters, const char *tagText, const char* pageURL, BMessenger *viewMessenger, float width, float height, BMessage *pluginData);
UResourceImp *PluginResource(URLParser& parser, long docRef, const char *downloadPath, BMessenger *listener, bool useStreamIO);

class PluginImageInfo {
public:
	image_id				mImageID;
	BMessage				mPluginInfo;
	int32					mAPIVersion;
	netpositive_plugin_init_func		mInitLocation;
	netpositive_plugin_term_func		mTermLocation;
};

#endif
