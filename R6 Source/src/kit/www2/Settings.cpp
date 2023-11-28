#include <support2/Binder.h>
#include <support2/Message.h>
#include <www2/ResourceCache.h>

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#include "beia_settings.h"

using namespace B::WWW2;


void set_settings(BMessage *msg)
{
#warning "Fix me set_settings"
//	int32 n;
//	char *name;
//	const char *str, *sname;
//	type_code type;
//	
//	if(msg->FindString("be:settings", &sname) != B_OK) {
//		printf("set_settingS : bogus msg\n");
//		return;
//	}
//	
//	n = 0;
//	while(msg->GetInfo(B_STRING_TYPE, n, &name, &type) == B_OK){
//		n++;
//		if(!strcmp(name,"be:settings")) continue;
//		
//		msg->FindString(name, &str);
//		set_setting(sname, name, str);
//	}
}

void get_settings(BMessage *msg, BMessage *resp)
{
#warning "Fix me get_settings"	
//	const char *str;
//	int i;
//	char name[255];
//	char value[255];
//	
//	if(msg->FindString("be:settings", &str) != B_OK) {
//		return;
//	}
//		
//	for(i=0;get_nth_setting(str, i, name, 255, value, 255) == B_NO_ERROR;i++)
//	{
//		resp->AddString(name, value);
//	}
//		
}


//BinderNode::property proxySettings;
//BinderNode::property navigatorSettings;
//BinderNode::property configSettings;

void InitWebSettings()
{
#warning "Fix me InitWebSettings"
//	if (!proxySettings || !navigatorSettings || !configSettings) {
//		BinderNode::property root = BinderNode::Root() / "service" / "web";
//		if (!proxySettings) {
//			proxySettings = root["proxy"];
//			// Make sure it's completely initialized.  This avoids any
//			// potential race condition with multiple threads accessing
//			// the object for the first time.
//			proxySettings.Object();
//		}
//		if (!navigatorSettings) {
//			navigatorSettings = root["navigator"];
//			proxySettings.Object();
//		}
//		if (!configSettings) {
//			configSettings = root["macros"];
//			proxySettings.Object();
//		}
//	}
}

void get_proxy_server(char *out_server, int *out_port)
{
#warning "Fix me get_proxy_server"
//	strcpy(out_server,proxySettings["http_server"].String().String());
//	*out_port = (int32)proxySettings["http_port"].Number();
}

void set_proxy_server(const char *server, int port)
{
#warning "Fix me set_proxy_server"
//	proxySettings["http_port"] = (double)port;
//	proxySettings["http_server"] = server;
}

bool get_proxy_password(char *out_user, char *out_password)
{
#warning "Fix me get_proxy_password"
//	strcpy(out_user, proxySettings["http_user"].String().String());
//	if (out_user[0] == '\0' || strcmp(out_user, "<undefined>") == 0)	// HACK
//		return false;
//		
//	strcpy(out_password, proxySettings["http_password"].String().String());
//	printf("proxy password %s:%s\n", out_user, out_password);
//	return true;
}

void get_navigator_setting(const char *name, char *out_value, size_t value_len, const char *def_value)
{
#warning "Fix me get_navigator_setting"
//	BinderNode::property prop = navigatorSettings[name];
//	if (prop.IsUndefined()) prop = def_value;
//	strncpy(out_value,prop.String().String(),value_len);
}
