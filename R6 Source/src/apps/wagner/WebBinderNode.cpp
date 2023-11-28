
#include "Wotan.h"
#include "WebBinderNode.h"
#include <SecurityManager.h>
#include <ResourceCache.h>
#include <Content.h>
#include <DNSCache.h>

static const char *prop_list[] = {
	"TopGoto",
	"topContent",
	NULL
};

WebBinderNode::WebBinderNode()
{
	status_t err = StackOnto(BinderNode::Root()["service"]["web"]);
	printf("************************* WebBinderNode %ld %s\n",err,strerror(err));
}

WebBinderNode::~WebBinderNode()
{
}

status_t 
WebBinderNode::OpenProperties(void **cookie, void *copyCookie)
{
	int32 *i = new int32;
	*i = 0;
	if (copyCookie) *i = *((int32*)copyCookie);
	*cookie = i;
	return B_OK;
}

status_t 
WebBinderNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	int32 *i = (int32*)cookie;
	if (prop_list[*i]) {
		strncpy(nameBuf,prop_list[*i],*len);
		*len = strlen(prop_list[*i]);
		(*i)++;
	} else
		return ENOENT;

	return B_OK;
}

status_t 
WebBinderNode::CloseProperties(void *cookie)
{
	int32 *i = (int32*)cookie;
	delete i;
	return B_OK;
}

put_status_t 
WebBinderNode::WriteProperty(const char *name, const property &prop)
{
	return BinderNode::WriteProperty(name,prop);
}

get_status_t 
WebBinderNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp(name,"TopGoto")) {
		prop = (int)B_BAD_VALUE;
		if (args.Count()) {
			thread_id from;
			int32 error;
			BMessage userData('goto');
			URL url(args[0].String().String());
			GroupID groupID = securityManager.RegisterGroup("custom_content");
			resourceCache.NewContentInstance(url,find_thread(NULL),this,0,userData,groupID,NULL,(const char*)NULL);
			error = receive_data(&from,NULL,0);
			prop = (int)error;
		}
	} else if (!strcmp(name,"topContent")) {
		prop.Undefine();
		atom<ContentInstance> c = ((Wotan*)be_app)->GetTopContentInstance();
		if (c) {
			BinderNode *n = dynamic_cast<BinderNode*>((ContentInstance*)c);
			if (n) prop = n;
		}
	} else if (!strcmp(name, "ClearHistory")) {
		prop.Undefine();
		ContentView *view = ((Wotan*)be_app)->GetContentView();
		if (view)
			view->ClearHistory();
	} else if (!strcmp(name, "StartLoadBlackout")) {
		dnsCache.Blackout(true);
	} else if (!strcmp(name, "StopLoadBlackout")) {
		dnsCache.Blackout(false);
	}
		return BinderNode::ReadProperty(name,prop,args);
	
	return B_OK;
}

status_t 
WebBinderNode::HandleMessage(BMessage *msg)
{
	if ((msg->what == NEW_CONTENT_INSTANCE) || (msg->what == CONTENT_INSTANCE_ERROR)) {
		int32 id;
		BMessage userData;
		atom<Wagner::ContentInstance> content;
		if (msg->FindMessage("user_data", &userData) != B_OK) return B_OK;
		if (msg->FindInt32("id", &id) != B_OK) return B_OK;
		if ((msg->what == NEW_CONTENT_INSTANCE) &&
			(msg->FindAtom("instance", content) != B_OK)) return B_OK;
		if (userData.what == 'goto') {
			status_t err = -1;
			if (msg->what == NEW_CONTENT_INSTANCE)
				err = ((Wotan*)be_app)->GoTo(content);
			send_data(id,err,NULL,0);
		}
	} else
		return BinderNode::HandleMessage(msg);

	return B_OK;
}
