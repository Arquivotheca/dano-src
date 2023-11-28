// ===========================================================================
//	PluginSupport.cpp
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#ifdef PLUGINS

#include "PluginSupport.h"
#include "Strings.h"
#include "NetPositivePlugins.h"
#include "Store.h"

#include <List.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <stdio.h>
#include <Archivable.h>
#include <View.h>
#include <Resources.h>
#include <malloc.h>

void 	pprint(const char *format, ...);

static BList sPluginList;
const int32 kCurrentAPIVersion = 1;

void *GetNamedResource(const char *name, size_t& size);
void *GetNamedResourceFromFile(BResources& resources,const char *name, size_t& size);

void InitPlugins()
{
	BPath pluginPath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &pluginPath, true);
	pluginPath.Append(kApplicationName);
	pluginPath.Append(kPluginDirName);
	
	BDirectory dir(pluginPath.Path());
	
	BEntry entry;
	
	while (dir.GetNextEntry(&entry, true) == B_OK) {
		BPath path;
		entry.GetPath(&path);
		
		image_id id = load_add_on(path.Path());
		if (id != B_ERROR) {
			netpositive_plugin_init_func initLocation;
			netpositive_plugin_term_func termLocation;

			if (get_image_symbol(id, "InitBrowserPlugin", B_SYMBOL_TYPE_TEXT, (void **)&initLocation) != B_OK ||
				get_image_symbol(id, "TerminateBrowserPlugin", B_SYMBOL_TYPE_TEXT, (void **)&termLocation) != B_OK)
			{
				pprint("Loading of add_on %s failed.", path.Path());
				unload_add_on(id);
			} else {
				pprint("Loading of add_on %s succeessful.", path.Path());
				BMessage browserInfo, pluginInfo;
				browserInfo.AddString("BrowserVersionString", kDefaultBrowserString);
				browserInfo.AddInt32("PluginAPIVersion", 1);
				browserInfo.AddInt32("PluginImageID", id);
				status_t status = initLocation(&browserInfo, &pluginInfo);
				int32 pluginAPIVersion = pluginInfo.FindInt32("PluginAPISupported");
				if (pluginAPIVersion > kCurrentAPIVersion)
					continue;
				if (status == B_OK) {
					PluginImageInfo *info = new PluginImageInfo;
					info->mImageID = id;
					info->mInitLocation = initLocation;
					info->mTermLocation = termLocation;
					info->mPluginInfo = pluginInfo;
					info->mAPIVersion = MIN(pluginAPIVersion, 1);
					sPluginList.AddItem(info);
				}
			}
			
		}
	}
}

static bool MatchType(const char *pluginType, const char *dataType)
{
	if (strcasecmp(pluginType, dataType) == 0)
		return true;
	const char *pluginSlashPos = strstr(pluginType, "/*");
	if (!pluginSlashPos)
		return false;
	const char *dataSlashPos = strstr(dataType, "/");
	if (!dataSlashPos)
		return false;
	if ((pluginSlashPos - pluginType) != (dataSlashPos - dataType))
		return false;
	if (strncasecmp(pluginType, dataType, pluginSlashPos - pluginType) == 0)
		return true;
	else
		return false;
}

BView *InstantiatePlugin(const char *mimeType, BMessage *parameters, const char *tagString, const char *pageURL, BMessenger *viewMessenger, float width, float height, BMessage *pluginData)
{
	for (int i = 0; i < sPluginList.CountItems(); i++) {
		PluginImageInfo *info = (PluginImageInfo *)sPluginList.ItemAt(i);
		BMessage dataType;
		info->mPluginInfo.FindMessage("DataTypePlugins", &dataType);
		const char *type;
		for (int j = 0; dataType.FindString("MIMEType", j, &type) == B_OK; j++)
			if (type && *type && MatchType(type, mimeType)) {
				BMessage viewArchive;
				dataType.FindMessage("ViewArchive", &viewArchive);
				BArchivable *archivable = instantiate_object(&viewArchive);
				BView *view = dynamic_cast<BView *>(archivable);
				if (view) {
					if (view->Frame().Width() != width || view->Frame().Height() != height)
						view->ResizeTo(width, height);
					BMessage initMessage(B_NETPOSITIVE_INIT_INSTANCE);
					initMessage.AddMessage("Parameters", parameters);
					if (tagString)
						initMessage.AddString("ParameterString", tagString);
					initMessage.AddString("URL", pageURL);
					if (pluginData)
						initMessage.AddMessage("InstanceData", pluginData);
					initMessage.AddMessenger("BrowserMessenger", *viewMessenger);
					view->MessageReceived(&initMessage);
					return view;
				}
				else if (archivable)
					delete archivable;
			}
	}
	return NULL;
}

void KillPlugins()
{
	while (sPluginList.CountItems()) {
		PluginImageInfo *info = (PluginImageInfo *)sPluginList.ItemAt(0);
		info->mTermLocation();
		pprint("Terminating plug-in");
		unload_add_on(info->mImageID);
		delete info;
		sPluginList.RemoveItem((int32)0);
	}
}

UResourceImp *PluginResource(URLParser& parser, long docRef, const char *downloadPath, BMessenger *listener, bool useStreamIO)
{
	BString url;
	parser.WriteURL(url);
	BString data;
#warning Not thread-safe
	if (strcmp(parser.Path(), "Plug-ins/About.html") == 0) {
		BString pluginList;
		
		for (int i = 0; i < sPluginList.CountItems(); i++) {
			PluginImageInfo *info = (PluginImageInfo *)sPluginList.ItemAt(i);
			BString name = info->mPluginInfo.FindString("PluginName");

			pluginList += "<A HREF=\"";
			pluginList += name;
			pluginList += "/About.html\">";
			pluginList += name;
			pluginList += " ";
			pluginList += info->mPluginInfo.FindString("PluginVersion");
			pluginList += "</A><BR>\n";
		}
		
		size_t size;
		char *origData = (char *)GetNamedResource("AboutPlugins.html", size);
		data = origData;
		free(origData);
		
		data.ReplaceFirst("<!-- PLUG-IN LIST GOES HERE -->", pluginList.String());
	} else {
		const char *slashPos = strchr(parser.Path() + 9, '/');
		if (!slashPos)
			return 0;
		BString name = parser.Path() + 9;
		name.Truncate(slashPos - parser.Path() - 9);
		for (int i = 0; i < sPluginList.CountItems(); i++) {
			PluginImageInfo *info = (PluginImageInfo *)sPluginList.ItemAt(i);
			if (name == info->mPluginInfo.FindString("PluginName")) {
				image_info img_info;
				if (get_image_info(info->mImageID, &img_info) != B_OK)
					return 0;
				BEntry entry(img_info.name);
				if (entry.InitCheck() != B_OK)
					return 0;
				BFile file(&entry, O_RDONLY);
				BResources resources(&file);
				size_t size;
				data = (char *)GetNamedResourceFromFile(resources, slashPos + 1, size);
				break;
			}
		}
	}

	
	if (downloadPath) {
		BEntry entry(downloadPath);
		BFileStore target(&entry);
		target.Open(false);
		target.Write(data.String(), data.Length());
		target.Close();
	}

	if (!data.Length())
		return 0;
		
	char typeStr[256];
	DetermineFileType(url.String(),0,data.String(),data.Length(),typeStr);		// Find out what kind of data this is
	
	BString typeCStr(typeStr);
	UResourceImp* r = NewResourceFromData(data.String(),data.Length(),url.String(),typeCStr, listener, useStreamIO);
	return r;
}

#endif
