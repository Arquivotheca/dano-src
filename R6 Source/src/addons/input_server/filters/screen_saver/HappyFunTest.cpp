#include <string.h>

#include <Application.h>
#include <Binder.h>
#include <StreamIO.h>

#define BYLINE "HappyFunTest"

class Listener : public BinderListener {
public:
	Listener(void);
	virtual ~Listener(void);
	
	virtual status_t Overheard(binder_node node, uint32 observed,
		BString propertyName);
};

class HappyFunTestApp : public BApplication {
public:
	HappyFunTestApp(void);
	virtual ~HappyFunTestApp(void);
	
	virtual void ReadyToRun(void);
	virtual bool QuitRequested(void);
	
private:
	atom<Listener> m_listener;
	binder_node m_controlNode;
	binder_node m_settingsNode;
};

Listener::Listener(void)
{
}

Listener::~Listener(void)
{
}

status_t Listener::Overheard(binder_node node, uint32 observed,
	BString propertyName)
{
	if (observed & B_PROPERTY_CHANGED) {
		BinderNode::property prop = BinderNode::property::property(node)
			[propertyName.String()];
		BOut << BYLINE << ": property " << propertyName.String() << " is now \""
			<< prop.String() << "\"\n";
	} else {
		BOut << BYLINE << ": event was not B_PROPERTY_CHANGED\n";
	}
}

HappyFunTestApp::HappyFunTestApp(void)
	: BApplication("application/x-vnd.Be-HappyFunTestApp"),
		m_listener(new Listener())
{
	m_controlNode = BinderNode::Root()["service"]["screensaver"]["control"];
	if (!m_controlNode->IsValid()) {
		BOut << BYLINE << ": Durn!\n";
	}
	
	m_settingsNode = BinderNode::Root()["service"]["screensaver"]["settings"];
	if (!m_settingsNode->IsValid()) {
		BOut << BYLINE << ": Double durn!\n";
	}
}

HappyFunTestApp::~HappyFunTestApp(void)
{
}

void HappyFunTestApp::ReadyToRun(void)
{
	status_t err;

	err = m_listener->StartListening(m_controlNode, B_PROPERTY_CHANGED,
		"is_blanking");
	BOut << BYLINE << ": BinderListener::StartListening() returned " << strerror(err)
		<< "\n";
		
	err = m_listener->StartListening(m_settingsNode, B_PROPERTY_CHANGED,
		"fade_time");
	BOut << BYLINE << ": BinderListener::StartListening() returned " << strerror(err)
		<< "\n";
	
	err = m_listener->StartListening(m_settingsNode, B_PROPERTY_CHANGED,
		"enabled");
	BOut << BYLINE << ": BinderListener::StartListening() returned " << strerror(err)
		<< "\n";
}

bool HappyFunTestApp::QuitRequested(void)
{
	status_t err = m_listener->StopListening(m_controlNode);
	BOut << BYLINE << ": BinderListener::StopListening() returned "
		<< strerror(err) << "\n";
		
	err = m_listener->StopListening(m_settingsNode);
	BOut << BYLINE << ": BinderListener::StopListening() returned "
		<< strerror(err) << "\n";

	return true;
}

int main(void)
{
	HappyFunTestApp app;
	
	return app.Run();
}
