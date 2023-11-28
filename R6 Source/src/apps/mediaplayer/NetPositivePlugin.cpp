#include <View.h>
#include <Window.h>
#include <Looper.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <String.h>
#include <MediaTrack.h>
#include <NetPositivePlugins.h>
#include "MediaPlayerApp.h"
#include "PluginView.h"
#include "debug.h"

extern "C" {
	_EXPORT status_t InitBrowserPlugin(const BMessage *browserInfo, BMessage *pluginInfo);
	_EXPORT void TerminateBrowserPlugin();
};

class NetPositivePlugin : public PluginView {
public:

	NetPositivePlugin(BMessage*);
	virtual void MessageReceived(BMessage *message);
	static NetPositivePlugin *Instantiate(BMessage *data);
};


NetPositivePlugin::NetPositivePlugin(BMessage *)
	:	PluginView(BRect(0, 0, 0, 0))
{
}

void NetPositivePlugin::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_NETPOSITIVE_INIT_INSTANCE: {
			// Parse out the tag attributes.
			BMessage params;
			message->FindMessage("Parameters", &params);
			int32 i = 0;
			const char *attrName;
			const char *attrValue;

			while ((attrName = params.FindString("Attribute", i)) != NULL) {
				attrValue = params.FindString("Value", i);
				if (attrValue)
					SetProperty(attrName, attrValue);

				i++;
			}

			break;
		}

		default:
			PluginView::MessageReceived(message);
	}
}

NetPositivePlugin* NetPositivePlugin::Instantiate(BMessage *data)
{
	return new NetPositivePlugin(data);
}

status_t InitBrowserPlugin(const BMessage*, BMessage *pluginInfo)
{
	pluginInfo->AddString("PluginName", "MediaPlayer");
	pluginInfo->AddString("PluginVersion", "1.0");
	pluginInfo->AddInt32("PluginAPISupported", 1);
	pluginInfo->AddString("PluginAboutPage",
		"<html><head><title>MediaPlayer Plug-in</title></head><body>"
		"<h1>BeOS MediaPlayer</h1></body></html>");
	
	BMessage dataTypePlugin(B_NETPOSITIVE_DATATYPE_PLUGIN);
	dataTypePlugin.AddString("MIMEType", "application/x-vnd.Be.MediaPlayer");
	dataTypePlugin.AddString("MIMEDescription", "MediaPlayer plug-in");
	
	BMessage dataTypeArchive;
	dataTypeArchive.AddString("add_on", kAppSignature);
	dataTypeArchive.AddString("class", "NetPositivePlugin");
	dataTypePlugin.AddMessage("ViewArchive", &dataTypeArchive);
	pluginInfo->AddMessage("DataTypePlugins", &dataTypePlugin);
	
	return B_OK;
}

void TerminateBrowserPlugin()
{
}

