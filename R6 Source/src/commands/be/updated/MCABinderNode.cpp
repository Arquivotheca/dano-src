
#include <Window.h>
#include <View.h>
#include <Resource.h>
#include <ResourceCache.h>
#include <ContentView.h>
#include "MCABinderNode.h"

using namespace Wagner;

class MCAConnection;

class MCAConnectionObserver : public BinderObserver
{
	private:
				bool					m_connectionActive;
				MCAConnection *			m_connection;

	public:
										MCAConnectionObserver(const binder_node &node, MCAConnection *conn);

				void					ReviseConnectionActivity(bool force);
				bool					ConnectionActive();
		virtual	status_t				ObservedChange(uint32 observed, const char *name);
};

class MCAConnection : public BinderContainer
{
	public:
											MCAConnection(const char *name, const atomref<MCABinderNode> &mca);
		virtual								~MCAConnection();

				status_t					LoadURL(const Wagner::URL &url, bool sync, bool updateStatus, Wagner::GroupID groupID);
		virtual	status_t					HandleMessage(BMessage *msg);
				void						RemoveSelf();
				void						SetConnectionActive(bool active);

		virtual	get_status_t				ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	void						Cleanup();

	private:
		MCAConnectionObserver *				m_observer;
		BString								m_name;
		BWindow *							m_window;
		BView *								m_view;
		atom<Wagner::ContentInstance>		m_script;
		int32								m_id;
		bool								m_active;
		atomref<MCABinderNode>				m_MCA;
};

MCAConnectionObserver::MCAConnectionObserver(const binder_node &node, MCAConnection *conn)
	: BinderObserver(node,B_SOMETHING_CHANGED)
{
	m_connection = conn;
	ReviseConnectionActivity(true);
}

bool 
MCAConnectionObserver::ConnectionActive()
{
	return m_connectionActive;
}

void 
MCAConnectionObserver::ReviseConnectionActivity(bool force)
{
	bool active = false;
	BinderNode::property obj = Object();
	if ((obj["observationCount"].Number() > 0) ||
		(obj["threadCount"].Number() > 0) ||
		(obj["pendingLoadCount"].Number() > 0) ||
		(obj["status"].String() != "loaded"))
		active = true;

	if (force || (m_connectionActive != active)) {
		m_connectionActive = active;
		m_connection->SetConnectionActive(active);
	}
}

status_t 
MCAConnectionObserver::ObservedChange(uint32 observed, const char *name)
{
	ReviseConnectionActivity(false);
	return B_OK;
}

MCAConnection::MCAConnection(const char *name, const atomref<MCABinderNode> &mca)
{
	m_view = new BView(BRect(0,0,1,1), "MCA dummy view", B_FOLLOW_ALL_SIDES, 0);
	m_window = new BWindow(
		BRect(0,0,100,100),
		"MCA dummy window",
		B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS);
	m_window->AddChild(m_view);
	m_window->Run();
	m_id = 0;
	m_name = name;
	m_MCA = mca;
	m_active = true;
	m_observer = NULL;

	SetPermissions(permsRead|permsWrite|permsCreate|permsDelete);
	AddProperty("status","connecting",permsRead);
	AddProperty("content",property::undefined,permsRead);
	AddProperty("stepName",property::undefined,permsRead|permsWrite);
	AddProperty("stepFractionCompleted",property::undefined,permsRead|permsWrite);
}

MCAConnection::~MCAConnection()
{
	printf("MCAConnection::DESTRUCTOR\n");
}

void 
MCAConnection::Cleanup()
{
	printf("MCAConnection::Cleanup()\n");
	if (m_script) {
		m_script->DetachedFromView();
		m_script = NULL;
	}
	if (m_observer) {
		delete m_observer;
		m_observer = NULL;
	}
	m_MCA = NULL;
	BinderContainer::Cleanup();
	m_window->Lock();
	m_window->Close();
}

get_status_t 
MCAConnection::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp(name,"content")) {
		BinderNode *node = dynamic_cast<BinderNode*>((Wagner::ContentInstance*)m_script);
		if (node) prop = node;
		else prop.Undefine();
	} else if (!strcmp(name,"shutdown")) {
		RemoveSelf();
		Cleanup();
		prop = "ok";
	} else
		return BinderContainer::ReadProperty(name,prop,args);

	return B_OK;
}

void 
MCAConnection::SetConnectionActive(bool active)
{
	printf("MCAConnection::SetConnectionActive(%s)\n",active?"true":"false");
	if ((active == false) && m_observer) {
		RemoveSelf();
	}
}

void 
MCAConnection::RemoveSelf()
{
	printf("MCAConnection::RemoveSelf()\n");
	AddProperty("status","done");
	m_MCA->RemoveConnection(m_name.String());
	if (m_script) {
		m_script->DetachedFromView();
		m_script = NULL;
	}
	if (m_observer) {
		delete m_observer;
		m_observer = NULL;
	}
}

status_t 
MCAConnection::LoadURL(const Wagner::URL &url, bool sync, bool updateStatus, GroupID groupID)
{
	status_t err = B_OK;
	thread_id from;
	BMessage msg(sync?'exec':'exe2');
	msg.AddInt32("thid",find_thread(NULL));
	Wagner::resourceCache.InvalidateAll();
	if (updateStatus) AddProperty("status","loading");
	
	if (m_observer) {
		delete m_observer;
		m_observer = NULL;
	}
	resourceCache.NewContentInstance(url,m_id++,this,Wagner::FORCE_RELOAD, msg, groupID);
	return B_OK;
}

status_t 
MCAConnection::HandleMessage(BMessage *msg)
{
	if ((msg->what == NEW_CONTENT_INSTANCE) || (msg->what == CONTENT_INSTANCE_ERROR)) {
		int32 thid,id;
		BMessage userData;
		atom<Wagner::ContentInstance> content;
		msg->PrintToStream();
		if (msg->FindMessage("user_data", &userData) != B_OK) return B_OK;
		if (msg->FindInt32("id", &id) != B_OK) return B_OK;
		if (userData.FindInt32("thid", &thid) != B_OK) return B_OK;
		if ((msg->what == NEW_CONTENT_INSTANCE) &&
			(msg->FindAtom("instance", content) != B_OK)) return B_OK;
		uint32 flags;
		status_t err = -1;
		if (msg->what == NEW_CONTENT_INSTANCE) {
			AddProperty("status","running");
			if (m_script) m_script->DetachedFromView();
			m_script = content;
			m_script->AttachedToView(m_view, &flags);
			m_script->FrameChanged(BRect(0,0,1,1), 1, 1);
			BinderNode *node = dynamic_cast<BinderNode*>((Wagner::ContentInstance*)m_script);
			if (!node) {
				printf("got content of non-HTML type\n");
				RemoveSelf();
			} else {
				m_observer = new MCAConnectionObserver(node,this);
				printf("got content\n");
			}
			err = 0;
		} else {
			BString s = "error: ";
			const char *errstr = msg->FindString("error");
			s << errstr;
			AddProperty("result",s);
			RemoveSelf();
			printf("got ERROR\n");
		}
//		if (userData.what == 'exec') send_data(thid,err,NULL,0);
	} else {
		switch (msg->what) {
			case bmsgLinkTo: {
				Wagner::GroupID groupID;
				Wagner::URL url;
				url.ExtractFromMessage("url",msg);
				if (msg->FindInt32("requestor_groupid", (int32*) &groupID) != B_OK) {
					printf("WARNING: Add requestor_groupid to bmsgLinkTo message\n");
					groupID = securityManager.GetGroupID(url);
				}
				LoadURL(url,false,false,groupID);
			} break;
			case bmsgReload: {
				Wagner::URL url = m_script->GetContent()->GetResource()->GetURL();
				LoadURL(url,false,false,securityManager.GetGroupID(url));
			} break;
		}
		return BinderNode::HandleMessage(msg);
	}

	return B_OK;
}

/***********************************************************************/

MCABinderNode::MCABinderNode()
{
	InitWebSettings();
	
	atom<BinderContainer> conn = new BinderContainer();

	SetPermissions(permsRead|permsWrite);
	AddProperty("DMS_Connect",property::undefined,permsRead);
	AddProperty("DMS_UpdatePoll",property::undefined,permsRead);
	AddProperty("Visit",property::undefined,permsRead);
	AddProperty("connections",(BinderNode*)conn,permsRead);
	
	m_connections = conn;

	StackOnto(BinderNode::Root()["service"]["mca"]);
}

MCABinderNode::~MCABinderNode()
{
}

void augment_url(Wagner::URL &url)
{
	BinderNode::property sys = BinderNode::Root()["service"]["system"];
	url.AddQueryParameter("deviceid",sys["device_id"].String().String());
	url.AddQueryParameter("deviceclassid",sys["device_class"].String().String());
	url.AddQueryParameter("swversionid",sys["version_id"].String().String());
	url.AddQueryParameter("userid",BinderNode::Root()["user"]["~"]["name"].String().String());
}

void 
MCABinderNode::RemoveConnection(const char *name)
{
	atom<BinderContainer> conn = m_connections;
	if (conn) conn->RemoveProperty(name);
}

void 
MCABinderNode::Cleanup()
{
	m_connections = NULL;
	BinderContainer::Cleanup();
}

BinderNode::property MCABinderNode::Connect(const char *name, Wagner::URL &url)
{
	char buf[1024];
	url.GetString(buf,1024);
	printf("MCABinderNode::Connect('%s','%s')\n",name,buf);

	if (!m_connections.Ptr()->Property(name).IsUndefined())
		return "connection already open";

	atom<MCAConnection> conn = new MCAConnection(name,this);
	conn->LoadURL(url,true,true,securityManager.RegisterGroup("custom_content"));
	property prop = (BinderNode*)conn;
	m_connections.Ptr()->AddProperty(name,prop);
	return prop;
}

get_status_t 
MCABinderNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp(name,"DMS_Connect")) {
		if (!args.Count()) {
			prop = "missing connection id";
		} else {
			Wagner::URL url(BinderNode::Root()["service"]["vendor"]["url"]["dms_slave"].String().String());
			url.AddQueryParameter("cookie",args[0].String().String());
			augment_url(url);
			prop = Connect("dms",url);
		}
	} else if (!strcmp(name,"DMS_UpdatePoll")) {
		Wagner::URL url(BinderNode::Root()["service"]["vendor"]["url"]["updatepoll"].String().String());
		augment_url(url);
		prop = Connect("update_poll",url);
	} else if (!strcmp(name,"Visit")) {
		if (args.Count() != 2) {
			prop = "calling convention is: Visit(name,url)";
		} else {
			Wagner::URL url(args[1].String().String());
			augment_url(url);
			prop = Connect(args[0].String().String(),url);
		}
	} else
		return BinderContainer::ReadProperty(name,prop,args);
		
	return B_OK;
}

