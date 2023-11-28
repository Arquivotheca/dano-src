#include <support/Binder.h>
#include "Dialer.h"

//-------------- AutodialListener --------------------

class AutodialListener: public BinderListener
{
public:
							AutodialListener();

	bool					CanAutodial() const {return m_canAutodial;}

private:
	virtual status_t		Overheard(binder_node node,
									  uint32 observed,
									  BString propertyName);

	BinderNode::property	m_profilesNode;
	binder_node				m_routeNode;
	BString					m_ifName;
	bool					m_canAutodial;
};

AutodialListener::AutodialListener():
	m_canAutodial(false)
{
	//locate useful nodes
	BinderNode::property netNode = BinderNode::Root() / "service" / "network";
	m_profilesNode = netNode / "profiles";
	m_routeNode = netNode / "control" / "status" / "route";

	//we'll watch the 'default route' node
	StartListening(m_routeNode, B_PROPERTY_CHANGED, "interface");

	//get in sync with the current system state
	Overheard(m_routeNode, B_PROPERTY_CHANGED, "interface");
}

status_t AutodialListener::Overheard(binder_node node,
									 uint32 observed,
									 BString propertyName)
{
	//the default route has switched interfaces - remember the new one
	m_ifName = BinderNode::property(m_routeNode)["interface"];

	//look in the current profile to deteremine whether autodial is
	//enabled for the default route's interface
	BString currentProfile = m_profilesNode["currentprofile"];
	BString autodial = (m_profilesNode / currentProfile.String() / "interfaces" / m_ifName.String() / "autodial").String();

	//note that 'autodial' will be "undefined" if there's no default route,
	//the current profile doesn't exist, the default route's interface isn't
	//specified by the current profile, etc.
	m_canAutodial = (autodial == "1");
}


//-------------- Dialer --------------------

Dialer::Dialer()
{
	//create the AutodialListener
	m_autodialListener = new AutodialListener();
}

bool Dialer::CanAutodial()
{
	BinderListener *p = (BinderListener *)m_autodialListener;
	return ((AutodialListener *)p)->CanAutodial();
}
