/*
 * NetNode.cpp
 *
 * NetBinderNode, handles configuring the networking.
 */

#include <OS.h>
#include <Application.h>
#include <String.h>
#include <File.h>
#include <string.h>
#include <Errors.h>
#include <Messenger.h>
#include <stdlib.h>
#include <math.h>
#include <Binder.h>
#include <SupportDefs.h>
#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/time.h>
#include <net/net_control.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <net/route.h>
#include <errno.h>
#include <ByteOrder.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <bone_interface.h>
#include <net/dhcp.h>
#include <Autolock.h>
#include <bone_serial.h>
#include <bone_serial_ppp.h>
#include <bone_observer.h>
#include <net/dhcp.h>
#include <bone_pppoe.h>


//
//
//  Network Binder Node Topology
//
//  service/network          - Net Binder Node
//
//  Net Binder Node properties
//
//		control              - network Control Node (defined in this file)
//		profiles             - backing store of profile data (see end of comment for
//                             format description)
//
//  Control Node properties
//
//		status               - current networking status node (read only)
//		DNS                  - current DNS settings (read only)
//		up()                 - bring current profile up
//		down()               - bring current profile down
//		adopt(name)          - adopt a named profile
//
//  Net Status Node properties (all read only)
//
//		route                - default route node 
//		interfaces           - list of Interface Nodes
//		profile              - string containing current profile name
//		hostname             - string containing current host name
//
//	Default Route Node properties (all read only)
//
//		address              - string containing gateway IP address
//		interface            - string containing interface name to route on
//
//
//  Interface Node properties (all read only)
//		type                 - device type (ethernet, PPP, PPPoE, loopback)				
//		status               - interface status (up/down)
//		address              - ip address of device
//		mask                 - device mask
//		hwaddr               - hardware address
//		bandwidth            - device bandwidth
//		linkstatus           - datalink status (used by PPP, DHCP, etc)
//		linkmsg              - last datalink message (ditto)
//
//-----------------------------------------------------------------------------
//  Backing Store Format
//
//  properties under service/network/profiles:
//
//		currentprofile       - string containing the current profile to use on boot
//		dhcp                 - DHCP profile settings
//		staticip             - static IP profile settings
//		ppp                  - ppp profile settings
//		ppp800               - 800-number registration PPP settings
//		custom               - custom profile, TBD per client
//
//	properties for all profiles
//
//		up_on_boot           - automatically up this profile at boot time (1/0)
//
//  DHCP profile properties
//
//		interfaces           - list of Interface Settings objects
//      
//	Static IP profile properties
//
//		interfaces           - list of Interface Settings objects
//		route                - default route settings object
//		primary_dns          - primary dns server address
//		secondary_dns        - secondary dns server address
//		dns_domain           - default dns domain
//
//  PPP profile properties
//
//      interfaces           - list of Interface Settings objects
//
//  Interface Settings properties
//
//		type                 - device type (ethernet, PPP, PPPoE, loopback)
//		address              - interface address (static ip only)
//		mask                 - interface mask (static IP only)
//		dhcp                 - use DHCP on this interface (1/0)
//		flags                - any additional flags for this interface
//		(the following only apply to PPP interfaces)
//		device               - name of physical device (i.e., modem)
//		phone_number         - ppp phone number
//		dial_prefix          - ppp dial prefix (9 to get out of pbx, etc)
//		modem_init           - ppp modem init string
//		hidden_init          - ppp hidden modem init string
//		user_name            - ppp user name 
//		password             - ppp password 
//
//      (the following are optional)
//       
//		connect_timeout      - ppp connect timeout
//		pulse_dial           - ppp pulse dial (1/0)
//		disable_chat         - ppp login() autonegotiation disable
//		retry_delay          - ppp redial delay
//		retry_count          - number of redial attempts
//		autodial             - ppp autoconnect (1/0)
//
//
//	Default Route settings object
//
//		address              - string containing gateway IP address
//		interface            - string containing interface name to route on

//note that __FUNCTION__ isn't implemented in iad's old toolset...
#define DBP(format...) printf(__FILE__  ": " format)

//--Declarations--------------------------------------------------------------------

//
// PPP script parsing
//

typedef struct {
	char fname[B_PATH_NAME_LENGTH];
	bool pulseDial;
	char *hiddenInit;
	char *modemInit;
	char *dialPrefix;
	char *phoneNum;
} connect_script_params_t;

typedef struct {
	const char *name;
	status_t (*func)(const char *value, connect_script_params_t *scriptParams,
						 bsppp_dial_params_t *dialParams);
} parse_table_entry_t;


//
// Abstract class to monitor BONE for notifications of changed properties
//


class KernelObserver
{
public:
	KernelObserver();
	virtual ~KernelObserver();
	
protected:
	void observe(uint32 propid, void *arg, size_t arglen);
	virtual void notify(uint32 property, uint32 what_happened, uint32 datalen, void *buf) = 0;
	
private:
	int m_fd;
	thread_id m_notifythread;
	static int32 m_threadfunc(void *arg);
};


//
// Simple bindercontainer-like functionality for classes that need to
// manage their own properties
//
class SimpleContainerNode : public BinderNode
{
public:
	SimpleContainerNode();
	virtual ~SimpleContainerNode() {}	
	
protected:

	struct iter_cookie
	{
		long index;
	};
	
	virtual	status_t		OpenProperties(void **cookie, void *copyCookie);
	virtual	status_t		NextProperty(void *cookie, char *nameBuf, int32 *len);
	virtual	status_t		CloseProperties(void *cookie);

	char **m_props;
	int m_num_props;
};

//
// Node to reflect the current in-kernel default route
//
class RouteNode : public SimpleContainerNode
{
public:
	RouteNode();
	virtual ~RouteNode(){};
	void updateRoute();
			
protected:
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
private:
	BString m_iface;
	BString m_address;
};


//
// Node to reflect the current in-kernel state of all interfaces
//
class InterfacesNode : public BinderContainer
{
public:
	void Acquired();
	bool updateInterface(uint32 if_index);
};


//
// The net status node
//
class StatusNode : public SimpleContainerNode, public KernelObserver
{
public:
	StatusNode();
	virtual ~StatusNode(){};
	
	BString CurrentProfile();
	void SetCurrentProfile(const BString& newProfile);

protected:
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
	virtual void notify(uint32 property, uint32 what_happened, uint32 datalen, void *buf);

private:
	atom<InterfacesNode> m_interfaces;  // the current interfaces
	atom<RouteNode>		m_route;

	BString				m_currentProfile;
	BLocker				m_lock;		//protects access to 'm_currentProfile'
};



//
// Node to report the current DNS settings
//
class DNSBinderNode : public SimpleContainerNode
{
public:
	DNSBinderNode();
	void writefile();
	void setPrimary(const char *s) {m_primary = s;}
	void setSecondary(const char *s) {m_secondary = s;}
	void setDomain(const char *s) {m_domain = s;}

protected:
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

private:
	BFile m_resolv_conf;
	struct stat m_stat;
	void parse_resolv_conf();
	BString m_domain;
	BString m_primary;
	BString m_secondary;
};


//
// The net control node
//
class ControlNode : public BinderContainer
{
public:
	ControlNode();
	virtual ~ControlNode(){printf("ERROR!  Net Node should never go away!\n");};
		
protected:
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

private:
	enum ifconf_action {
		ADOPT,
		BRINGUP,
		BRINGDOWN,
		UNADOPT,
	};
	
	atom <StatusNode> m_status;
	atom <DNSBinderNode>	m_dns;
	bool hostNameChanged;
	
	get_status_t up();
	get_status_t down();
	get_status_t adopt(BinderNode::property &prop);
	get_status_t sethostname(const char *name);
	get_status_t ifcontrol(ifconf_action action);
	void config_iface(BString &device, BinderNode::property &iface, ifconf_action action);
	void config_route(BString &addr, BString &iface, ifconf_action action);
	void dhconfig(BString &device, BinderNode::property &iface, ifconf_action action);
	void ifconfig(BString &device, BinderNode::property &iface, ifconf_action action);
	void pppconfig(BString &device, BinderNode::property &iface, ifconf_action action);
	status_t get_iface(const BString &name, ifreq_t *ifr);
};


//--util funcs----------------------------------------------------------------------


static char *
inet_ntoa_r(struct in_addr addr, char *buf)
{
	unsigned long long0;
	
	long0 = ntohl(addr.s_addr);
	sprintf(buf, "%d.%d.%d.%d",
			(int) (long0 >> 24) & 0xff,
			(int) (long0 >> 16) & 0xff,
			(int) (long0 >> 8) & 0xff,
			(int) (long0) & 0xff);
	return (buf);
}

static unsigned countdots(
		  const char *addr,
		  const char **dot1,
		  const char **dot2,
		  const char **dot3
		  )
{
	int ndots;
	const char *dots[3];

	ndots = 0;
	while (*addr) {
		if (*addr == '.') {
			if (ndots < 3) {
				dots[ndots] = addr;
			}
			ndots++;
		}
		addr++;
	}
	*dot1 = dots[0];
	*dot2 = dots[1];
	*dot3 = dots[2];
	return (ndots);
}


static unsigned long inet_addr(const char *addr)
{
	const char *dot1;
	const char *dot2;
	const char *dot3;
	int ndots;
	unsigned long long0;

	if(addr == 0)
	{
		errno = EINVAL;
		return (~0);
	}
	if (!isdigit(*addr)) {
		return (~0);
	}
	ndots = countdots(addr, &dot1, &dot2, &dot3);
	switch (ndots) {
	case 0:
		long0 = atoi(addr);
		break;
	case 1:
		long0 = atoi(addr) * 256 * 256 * 256 + atoi(dot1 + 1);
		break;
	case 2:
		long0 = (atoi(addr) * 256 * 256 * 256 + atoi(dot1 + 1) * 256 * 256 +
				 atoi(dot2 + 1));
		break;
	case 3:
	default:
		long0 = (atoi(addr) * 256 * 256 * 256 + atoi(dot1 + 1) * 256 * 256 +
				 atoi(dot2 + 1) * 256 + atoi(dot3 + 1));
		break;
	}
	return (htonl(long0));
}


int in_ifaddr(struct sockaddr *tar, const char *addr)
{
	in_addr ia;
	struct sockaddr_in *target = (struct sockaddr_in *) tar;
	

	ia.s_addr = inet_addr(addr);
	if ((ia.s_addr == 0xffffffff) && (strcmp(addr,"255.255.255.255") != 0))
		return -1;

	memset(target, 0, sizeof(struct sockaddr_in));
	target->sin_addr.s_addr = ia.s_addr;
	target->sin_len = sizeof(struct sockaddr_in);
	target->sin_family = AF_INET; 
	return 0;
}

//--KernelObserver---------------------------------------------------------------------------


KernelObserver::KernelObserver()
{
	m_fd = socket(AF_NOTIFY, SOCK_DGRAM, 0);
	m_notifythread = spawn_thread(m_threadfunc, "kernelobserver", B_LOW_PRIORITY, (void *) this);
	resume_thread(m_notifythread);
}


KernelObserver::~KernelObserver()
{
	kill_thread(m_notifythread);
	close(m_fd);
}



void KernelObserver::observe(uint32 propid, void *arg, size_t arglen)
{
	struct iovec iov[3];
	int num = 1;
	obsdata_t obs;
	
	obs.event = propid;
	obs.len = arglen;
	
	iov[0].iov_base = (char *) &obs;
	iov[0].iov_len = sizeof(obs);
	
	if(arg != 0)
	{
		num = 2;
		iov[1].iov_base = (char *) arg;
		iov[1].iov_len = arglen;
	}
	
	writev(m_fd, iov, num);
}


int32 KernelObserver::m_threadfunc(void *arg)
{
	KernelObserver *o = (KernelObserver *) arg;
	char buf[BONE_MAX_NOTIFY_DATA + sizeof(notifydata_t)];
	notifydata_t *b = (notifydata_t *) buf;
	int rc;
	
	memset(buf, 0, sizeof(buf));

	while((rc = read(o->m_fd, (void *) buf, sizeof(buf))) > 0)
	{
		if(b->len > 0)
		{
			o->notify(b->event, b->what, b->len, (char *) buf + sizeof(notifydata_t));
		} else {
			o->notify(b->event, b->what, 0, 0);		
		}
		memset(buf, 0, sizeof(buf));
	}
	return 0;
}





//--SimpleContainerNode----------------------------------------------------------------------


SimpleContainerNode::SimpleContainerNode()
{
	m_props = 0;
	m_num_props = 0;
}


status_t SimpleContainerNode::OpenProperties(void **cookie, void *copycookie)
{
	*cookie = new SimpleContainerNode::iter_cookie;
	
	((SimpleContainerNode::iter_cookie *) *cookie)->index = (copycookie ? ((SimpleContainerNode::iter_cookie *) copycookie)->index : 0);

	return B_OK;
}


status_t SimpleContainerNode::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	long index = ((SimpleContainerNode::iter_cookie *) cookie)->index;
	char *prop;
	size_t proplen;
	
	if(index >= m_num_props)
	{
		return ENOENT;
	}
	
	prop = m_props[index];
	proplen = min_c((uint32)*len - 1, strlen(prop));
	strncpy(nameBuf, m_props[index], proplen);
	nameBuf[proplen] = 0;
	*len = proplen;
	index++;
	((SimpleContainerNode::iter_cookie *) cookie)->index = index;
	return B_OK;
	
}


status_t SimpleContainerNode::CloseProperties(void *cookie)
{
	delete cookie;
	return B_OK;
}



//---ControlNode------------------------------------------------------------------


ControlNode::ControlNode():
	hostNameChanged(true)
{
	BinderNode::property prop;
	
	m_dns = new DNSBinderNode();
	m_status = new StatusNode();
	
	this->AddProperty("DNS", (BinderNode *) m_dns);
	this->AddProperty("status", (BinderNode *) m_status);
	
	prop = BinderNode::Root() / "service" / "network" / "profiles" / "currentprofile";
	if (prop.IsUndefined() == true) {
		DBP("ControlNode(): no 'currentprofile' to re-adopt\n");

	} else {
		BString profileName = prop.String();

		DBP("net control node, re-adopting profile %s\n", profileName.String());
		
		//re-adopt the last profile we were using
		adopt(prop);

		//determine if we should "up" this profile, too
		prop = BinderNode::Root() / "service" / "network" / "profiles" / profileName.String();
		if (prop["up_on_boot"] == "1") {
			//yup
			DBP("upping profile %s, too\n", profileName.String());

			ifcontrol(BRINGUP);
		}
	}
}


get_status_t ControlNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	get_status_t rc = ENOENT;
	
	if(strcmp(name, "up") == 0)
	{
		rc = up();
	} else
	if(strcmp(name, "down") == 0)
	{
		rc = down();
	} else 
	if(strcmp(name, "adopt") == 0)
	{
		rc = adopt(args[0]);
	} else
	if(strcmp(name, "sethostname") == 0)
	{
		rc = sethostname(args[0].String().String());
	} else {
		rc = BinderContainer::ReadProperty(name, prop, args);
	}
	
	return rc;
}


get_status_t ControlNode::up()
{
	return ifcontrol(BRINGUP);
}


get_status_t ControlNode::down()
{
	return ifcontrol(BRINGDOWN);
}


get_status_t ControlNode::adopt(BinderNode::property &prop)
{
	BinderNode::property profiles, pr;
	
	DBP("adopt(%s)\n", prop.String().String());

	profiles = BinderNode::Root() / "service" / "network" / "profiles";
	
	profiles->GetProperty(prop.String().String(), pr);
	
	if(!pr.IsUndefined())
	{
		//un-adopt the current profile
		BString current = m_status->CurrentProfile();
		if (current != "")
			ifcontrol(UNADOPT);

		//adopt the new profile
		m_status->SetCurrentProfile(prop.String());
		profiles["currentprofile"] = prop;
		return ifcontrol(ADOPT);
	}
	
	return ENOENT;
}

get_status_t ControlNode::sethostname(const char *name)
{
	int rc;
	
	rc = ::sethostname(name, strlen(name));
	
	if(rc < 0)
	{
		DBP("sethostname failed: %s\n", strerror(errno));
		return B_ERROR;
	}

	//let any future calls to dhconfig() know that the hostname
	//has changed
	hostNameChanged = true;

	return B_OK;
}

get_status_t ControlNode::ifcontrol(ifconf_action action)
{
	BinderNode::property profiles, current, interface, tmp;
	BString profilename, device, rtaddr, rtif;
	
	//
	// go through all interfaces in the current profile.  
	//
	
	profiles = BinderNode::Root() / "service" / "network" / "profiles";
	profilename = m_status->CurrentProfile();
	current = profiles / profilename.String();
	
	BinderNode::iterator interfaces = ((current / "interfaces").Object())->Properties();
	
	while((device = interfaces.Next()) != "")
	{
		interface = current / "interfaces" / device.String();
		config_iface(device, interface, action);
	}
	
	/*
	 * static IP profiles have route and DNS info we need to deal with.
	 */
	bool writedns = false;
	tmp = current / "primary_dns";
	if(tmp.IsUndefined() != true)
	{
		m_dns->setPrimary(tmp.String().String());
		writedns = true;
	}
	tmp = current / "secondary_dns";
	if(tmp.IsUndefined() != true)
	{
		m_dns->setSecondary(tmp.String().String());
		writedns = true;
	}
	tmp = current / "dns_domain";
	if(tmp.IsUndefined() != true)
	{
		m_dns->setDomain(tmp.String().String());
		writedns = true;
	}
	
	if(writedns)
		m_dns->writefile();
		
	tmp = current / "route";
	if(tmp.IsUndefined() != true)
	{
		rtaddr = (tmp / "address").String();
		rtif = (tmp / "interface").String();
		config_route(rtaddr, rtif, action);
	}
	
	return B_OK;
}


void ControlNode::config_iface(BString &device, BinderNode::property &iface, ifconf_action action)
{

	//
	// determine the type of interface (loopback/ethernet, PPP, etc)
	//

	BinderNode::property type = iface["type"];
	if (type.IsUndefined() == true) {
		DBP("config_iface(%s): this interface lacks a 'type' property, so can't be configured\n",
						device.String());
		return;
	}

	BString strType = type.String();

	if (strType == "loopback") {
		//this is the loopback device - it's always configured with
		//ifconfig, since it can't run dhcp
		ifconfig(device, iface, action);

	} else if (strType == "ethernet") {
		//this is an ethernet device - it can be configured automatically
		//through dhcp, or manually through ifconfig

		if (iface["dhcp"] == "1") {
			//dhcp it is
			dhconfig(device, iface, action);
		
		} else {
			//ifconfig it is
			ifconfig(device, iface, action);
		}

	} else if (strType == "ppp" || strType == "pppoe") {
		//this is a ppp interface - pppconfig it
		pppconfig(device, iface, action);

	} else {
		DBP("config_iface(%s): interface type %s not supported\n",
										device.String(), strType.String());
	}
}


void ControlNode::config_route(BString &addr, BString &iface, ifconf_action action)
{
	route_req_t rrt;
	struct sockaddr_in *sin;
	
	//don't mess with routes unless we're switching profiles
	if (action == BRINGUP || action == BRINGDOWN)
		return;

	memset(&rrt, 0, sizeof(rrt));

	rrt.family = AF_INET;
	in_ifaddr(&rrt.mask, "255.255.255.255");
	rrt.flags |= RTF_DEFAULT;		
	in_ifaddr(&rrt.gateway, addr.String());
	rrt.flags |= RTF_GATEWAY;
	strcpy(rrt.iface, iface.String());

	// Now ensure we have enough good info to make a route
	sin = (struct sockaddr_in *)&rrt.gateway;
	if ((sin->sin_addr.s_addr == INADDR_BROADCAST)) {
		DBP("Can't add broadcast address as gateway\n");
		return;
	}

	if ((sin->sin_addr.s_addr == INADDR_ANY) &&
		(strlen(rrt.iface) == 0)) {
		DBP("Profile didn't specify a gateway or device\n");
		return;
	}

	//first, delete the current default route, if any (we can't add
	//a new one until the old one is gone)
	del_route(&rrt);
	
	if (action == ADOPT) {
		//add the new default route
		if (add_route(&rrt) != 0)
			DBP("failed to add default route\n");
	}
}






//
// get the current interface state for the named interface.
// if it does not exist, create it.
//
status_t ControlNode::get_iface(const BString &name, ifreq_t *ifr)
{
	status_t rc = B_ERROR;
	int s;
	bone_interface_params_t params;
	bone_ether_interface_params_t ethparams;
	bone_lognet_params_t logParams;
	char ether_interface[] = "bone_ether";

	memset(ifr, 0, sizeof(ifreq_t));
	memset(&params, 0, sizeof(params));
	memset(&ethparams, 0, sizeof(ethparams));
	memset(&logParams, 0, sizeof(logParams));
	
	if(get_interface_by_name(name.String(), ifr) == B_OK)
	{
		return B_OK;
	}
	
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		DBP("%s\n", strerror(errno));
		return B_ERROR;	
	}
		
	//
	// create interface
	//
	memset(ifr, 0, sizeof(ifreq_t));
	strcpy(ifr->ifr_name, name.String());

	memset(&params, 0, sizeof(params));
	params.interface_leaf = ether_interface;	
	
	ethparams.device = name.String();
	params.buf = &ethparams;

	if (ioctl(s, SIOCADDIFACE, &params) < 0)
	{
		DBP("added physical interface %s\n", name.String());
	}
	
	//now we'll start a logical interface on top of the physical
	//interface we just created
	logParams.name = name.String();
	logParams.driver = name.String();
	logParams.flavor = 0;

	if(ioctl(s, SIOCAIFADDR, &logParams) < 0)
	{
		rc = B_ERROR;
		DBP("%s\n", strerror(errno));	
		goto out;	
	}
	//
	// OK! try and get the interface again (so that we'll have an index)
	//
	if(get_interface_by_name(name.String(), ifr) != B_OK)
	{
		rc = B_ERROR;
		DBP("get_interface_by_name(%s) failed\n", name.String());
		goto out;
	}
	
	rc = B_OK;
out:	
	close(s);
	
	return rc;
}

static int32 start_dhcp(if_index_t index)
{
	int s;
	dhcp_ioctl_data_t data;

	//start with sane values
	data.index = index;
	data.u.onParams.oldAddr = 0;
	data.u.onParams.priority = DHCP_DEFAULT_PRIORITY;

	//
	// do it
	//
	if ((s = socket(AF_INET, SOCK_MISC, 1)) < 0) {
		DBP("can't open dhcp socket: %s\n", strerror(errno));
		goto err;
	}

	if (ioctl(s, DHCP_ON, &data, sizeof(data)) != 0)
	{
		DBP("failed to start dhcp on %d: %s\n", index, strerror(errno));
		goto err;
	}

err:
	close(s);
	return 0;
}

void ControlNode::dhconfig(BString &device, BinderNode::property &, ifconf_action action)
{
	ifreq_t ifreq;
	dhcp_ioctl_data_t data;
	int s;
	thread_id tid;
	
	memset(&data, 0, sizeof(data));

	if((s = socket(AF_INET, SOCK_MISC, 1)) < 0)
	{
		DBP("failed opening dhcp control socket\n");
		return;
	}
	
	if (hostNameChanged == true) {
		char hostname[MAXHOSTNAMELEN];

		//the machine's hostname has changed - use this new hostname
		//in future dhcp requests
		if (gethostname(hostname, sizeof(hostname)) != 0) {
			DBP("unable to retrieve hostname\n");
		
		} else if (hostname[0] != 0x00) {
			data.u.nameValue.name = DHCP_HOSTNAME;
			data.u.nameValue.value.strValue = hostname;

			if (ioctl(s, DHCP_SET_VAR, &data, sizeof(data)) != 0)
				DBP("setting dhcp hostname failed\n");
		}

		hostNameChanged = false;
	}

	//note that get_iface() will create the interface if it doesn't exist
	if(get_iface(device.String(), &ifreq) != B_OK)
	{
		DBP("could not add or find interface\n");
		goto err;
	}

	//we must pass the interface's index to dhcp
	data.index = ifreq.ifr_index;
	
	switch (action) {
	case ADOPT:
		//do nothing (the interface was created implicitly by get_iface()
		//above, and there's no point in turning DHCP on until the
		//interface is brought up)
		break;

	case BRINGUP:
		if(set_iface_flag_by_name(ifreq.ifr_name, IFF_UP) != B_OK)
		{
			DBP("dhconfig(%s): could not set interface to up\n",
						device.String());
			goto err;
		}
		
		//ioctl(DHCP_ON) can block for quite awhile if a dhcp server
		//doesn't respond to our requests - run it in a seperate thread
		if ((tid = spawn_thread((thread_func)start_dhcp, "dhcp",
					B_NORMAL_PRIORITY, (void *)ifreq.ifr_index)) < B_OK ||
			resume_thread(tid) != B_OK)
		{
			DBP("dhconfig(%s): failed to start dhcp thread\n", device.String());
			goto err;
		}

		//don't do this!  (snooze for a bit to allow the thread we just
		//spawned to start dhcp)
		snooze(500000LL);
		break;

//		op = DHCP_ON;
//	
//		//start with sane values
//		data.u.onParams.oldAddr = 0;
//		data.u.onParams.priority = DHCP_DEFAULT_PRIORITY;
//	
//		//
//		// do it
//		//
//		if (ioctl(s, op, &data, sizeof(data)) < 0)
//		{
//			DBP("ControlNode::dhconfig: could not dhcp %s: %s\n",
//										device.String(), strerror(errno));
//			goto err;
//		}
//		break;

	case BRINGDOWN:
	case UNADOPT:
		//turn off dhcp on this interface
		ioctl(s, DHCP_OFF, &data, sizeof(data));

		// bring down and bail
		clr_iface_flag_by_name(ifreq.ifr_name, IFF_UP);
		break;
	}

err:
	close(s);
}


void ControlNode::ifconfig(BString &device, BinderNode::property &iface, ifconf_action action)
{
	ifreq_t ifreq;
	int s;
	char flags[255], *cmd, *save_ptr = 0;
	
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		DBP("ControlNode::ifconfig: could not add or find interface\n");
		return;
	}
	
	//note that get_iface() will create the interface if it doesn't exist
	if(get_iface(device.String(), &ifreq) != B_OK)
	{
		DBP("ControlNode::ifconfig: could not add or find interface\n");
		goto err;
	}

	switch (action) {
	case BRINGDOWN:
		//note that we never down the loopback device
		if (iface["type"] != "loopback") {
			// bring down and bail
			clr_iface_flag_by_name(ifreq.ifr_name, IFF_UP);
		}
		break;

	case ADOPT:
	case BRINGUP:
		{
			BinderNode::property prop;

			//set the interface up
			prop = iface["address"];
			if (prop.IsUndefined() == false) {
				if (in_ifaddr(&ifreq.ifr_addr, prop.String().String()) != 0)
				{
					DBP("ControlNode::ifconfig(%s): bad address in profile\n",
								device.String());	
					goto err;
				}
			
				if(ioctl(s, SIOCSIFADDR, &ifreq, sizeof(ifreq_t)) < 0)
				{
					DBP("ControlNode::ifconfig(%s): unable to set address\n",
								device.String());	
					goto err;
				}
			}
			
			prop = iface["mask"];
			if (prop.IsUndefined() == false) {
				if(in_ifaddr(&ifreq.ifr_mask, prop.String().String()) != 0)
				{
					DBP("ControlNode::ifconfig(%s): bad mask in profile\n",
								device.String());	
					goto err;
				}
			
				if(ioctl(s, SIOCSIFNETMASK,  &ifreq, sizeof(ifreq_t)) < 0)
				{
					DBP("ControlNode::ifconfig(%s): unable to set mask\n",
								device.String());	
					goto err;
				}
			}
			
			prop = iface["flags"];
			if (prop.IsUndefined() == false) {
				strncpy(flags, prop.String().String(), 255);
				flags[254] = 0;
				
				cmd = strtok_r(flags, "\n\r\t ", &save_ptr);

				while(cmd != 0)
				{
					if(strcmp(cmd, "promisc") == 0)
					{
						ifreq.ifr_flags |= IFF_PROMISC;
					} else
					if(strcmp(cmd, "-promisc") == 0)
					{
						ifreq.ifr_flags &= ~IFF_PROMISC;
					} else
					if(strcmp(cmd, "debug") == 0)
					{
						ifreq.ifr_flags |= IFF_DEBUG;
					} else
					if(strcmp(cmd, "-debug") == 0)
					{
						ifreq.ifr_flags &= ~IFF_DEBUG;
					} else
					if(strcmp(cmd, "autoup") == 0)
					{
						ifreq.ifr_flags |= IFF_AUTOUP;
					} else
					if(strcmp(cmd, "-autoup") == 0)
					{
						ifreq.ifr_flags &= ~IFF_AUTOUP;
					}
					cmd = strtok_r(NULL, "\n\r\t ", &save_ptr);
				}
			}
	
			if(action == BRINGUP)
			{
				ifreq.ifr_flags |= IFF_UP;
			}
				
			if(ioctl(s, SIOCSIFFLAGS,  &ifreq, sizeof(struct ifreq)) < 0)
			{
				DBP("ControlNode::ifconfig(%s): unable to set interface flags\n",
							device.String());	
				goto err;
			}
		}
		break;

	case UNADOPT:
		//do nothing (perhaps we should delete the interface here?)
		break;
	}
	
err:
	close(s);	
}



//--PPP script stuff------------------------------------------------------------------------------


static const char *expand(const char *in, connect_script_params_t *sp, bsppp_dial_params_t *dp)
{
	const char *p, *q;
	char *out;
	size_t len;
	int src, dst;

	len = strlen(in) + 1;

	//we'll start by traversing the string to determine how many
	//escape sequences need expanding, and the total size of the
	//expanded string
	p = in;
	q = p;
	while ((p = strchr(q, '~')) != NULL) {
		//we've found a potential escape sequence - verify it
		switch (p[1]) {
		case 'u':
			//usename escape sequence
			len += strlen(dp->username) - 2;
			break;
		
		case 'p':
			//password escape sequence
			len += strlen(dp->password) - 2;
			break;
		
		case 'i':
			//modem init string escape sequence
			len += strlen(sp->hiddenInit) +
							strlen(sp->modemInit) - 2;
			break;
		
		case 'd':
			//modem dial command (also take into account the length
			//of the "ATDT" we'll send before the phone number, but
			//which isn't included in 'phoneNum')
			len += strlen(sp->dialPrefix) +
							strlen(sp->phoneNum) + 4 - 2;
			break;
		
		default:
			//not a valid escape sequence - ignore it
			break;
		}

		//don't find the same tilde twice
		q = p + 1;
	}

	//we now know the total length of the expanded string - allocate it
	if ((out = (char *)malloc(len)) == NULL)
		goto err1;
	
	//now copy 'in' to 'out', expanding escape sequences as we go
	src = dst = 0;
	memset(out, 0, len);

	while (in[src] != 0x00) {
		if (in[src] == '~' && strchr("upid", in[src + 1]) != NULL) {
			//handle escape sequence
			switch (in[src + 1]) {
			case 'u':
				strcpy(&out[dst], dp->username);
				break;
			
			case 'p':
				strcpy(&out[dst], dp->password);
				break;
			
			case 'i':
				strcpy(&out[dst], sp->modemInit);
				strcat(out, sp->hiddenInit);
				break;

			case 'd':
				strcpy(&out[dst], (sp->pulseDial ? "ATDP" : "ATDT"));
				strcat(out, sp->dialPrefix);
				strcat(out,sp->phoneNum);
				break;
			}

			//adjust 'dst' to new length of 'out'
			dst = strlen(out);

			//adjust 'src' to skip over both the tilde and the escaped
			//character
			src += 2;

		} else {
			//just copy non-escape-sequence characters from 'in' to 'out'
			out[dst++] = in[src++];
		}
	}

err1:
	return out;
}

static status_t append_command(int cmd, const char *arg, connect_script_params_t *, bsppp_dial_params_t *dp)
{
	bsppp_script_cmd_t *p;
	size_t cmdCount;

	//resize our array of previous commands
	cmdCount = dp->n_script_commands + 1;
	if ((p = (bsppp_script_cmd_t *)realloc(dp->connect_script,
							sizeof(bsppp_script_cmd_t) * cmdCount)) == NULL)
	{
		return ENOMEM;
	}

	p[dp->n_script_commands].cmd = (bspppe_chat_cmd_t) cmd;
	p[dp->n_script_commands].arg = arg;
	
	dp->connect_script = p;
	dp->n_script_commands++;

	return B_OK;
}

static status_t script_send(const char *value,connect_script_params_t *sp, bsppp_dial_params_t *dp)
{
	const char *arg;

	//the "send" command always requires a value
	if (value == NULL)
		return EFAULT;

	//un-escape the send string
	if ((arg = expand(value, sp, dp)) == NULL)
		return ENOMEM;
	
	//append a new SEND command
	return append_command(BSPPP_CHAT_CMD_SEND, arg, sp, dp);
}

static status_t script_expect(const char *value,connect_script_params_t *sp, bsppp_dial_params_t *dp)
{
	const char *arg;

	//the "expect" command always requires a value
	if (value == NULL)
		return EFAULT;

	//un-escape the expect string
	if ((arg = expand(value, sp, dp)) == NULL)
		return ENOMEM;
	
	//append a new EXPECT command
	return append_command(BSPPP_CHAT_CMD_EXPECT, arg, sp, dp);
}

static status_t script_pause(const char *value, connect_script_params_t *sp, bsppp_dial_params_t *dp)
{
	//the "pause" command never takes a value
	if (value != NULL)
		return EINVAL;

	//append a new PAUSE command
	return append_command(BSPPP_CHAT_CMD_PAUSE, NULL, sp, dp);
}


static bool parseTab(const parse_table_entry_t *parseTable, connect_script_params_t *sp,
						 bsppp_dial_params_t *dp, const char *name, const char *value, const char *, int)
{
	bool errorFree = true;
	int i, ec;
	
	//lookup the function which will process this name/value pair
	for (i = 0; ; i++) {
		if (parseTable[i].name == NULL) {
			errorFree = false;
			break;

		} else if (strcasecmp(parseTable[i].name, name) == 0) {
			//this is the proper function - process the value
			ec = parseTable[i].func(value, sp, dp);

			switch (ec) {
			case B_OK:
				//this line was processed successfully
				break;
				
			case EFAULT:
				errorFree = false;
				break;
			
			case EINVAL:
				errorFree = false;
				break;

			case EALREADY:
				errorFree = false;
				break;
			}

			//a given name will match at most one entry in the
			//parse table - no need to keep looking
			break;
		}
	}
	return errorFree;
}


						
static bool parse_ppp_script(connect_script_params_t *scriptParams,
						 bsppp_dial_params_t *dialParams)
{
	FILE *file;
	char buf[200];
	char *p, *name, *nameEnd, *value;
	int len, lineNo = 0;
	bool ignoreLine = false, errorFree = true;
	const parse_table_entry_t parseTable[] = {
		{"send", script_send},
		{"expect", script_expect},
		{"pause", script_pause},
		{NULL, NULL},
	};

	if (strchr(scriptParams->fname, '/') == NULL) {
		char pathname[B_PATH_NAME_LENGTH];

		//the filename is "simple" - assume the file is located in
		//the default dialer prefs directory
		snprintf(pathname, sizeof(pathname), "/etc/dialer/%s", scriptParams->fname);
		pathname[sizeof(pathname) - 1] = 0x00;

		file = fopen(pathname, "r");

	} else {
		//non-simple pathname - assume the user told us exactly where
		//the file is located
		file = fopen(scriptParams->fname, "r");
	}
	
	//make sure we opened the file successfully
	if (file == NULL)
		goto err1;

	//read the next line from the file
	while (fgets(buf, sizeof(buf), file) != NULL) {

		//keep track of which line we're on, for use in error messages
		//(but don't count each section of a multi-read line)
		if (ignoreLine == false)
			lineNo++;

		//determine if we read a full line
		len = strlen(buf);
		if ((len == sizeof(buf) - 1) && (buf[len - 1] != '\n') &&
			(feof(file) == 0))
		{
			//we didn't read the entire line, which means that this
			//line is unreasonably long 
			errorFree = false;

			//just discard this line
			ignoreLine = true;
			continue;
		}
		
		if (ignoreLine == true) {
			//we finally found the end of a long (multi-read) line -
			//throw it away, but remember to process the next line
			ignoreLine = false;
			continue;
		}

		//strip the line's trailing newline
		if (buf[len - 1] == '\n') {
			buf[len - 1] = 0x00;
			len--;
		}

		//strip any comment from the end of the line
		if ((p = strrchr(buf, '#')) != NULL) {
			*p = 0x00;
			len = (p - buf);
		}

		//we expect lines to consist of a name field, possibly followed
		//by a value field, with some whitespace in-between - find the name
		name = &buf[strspn(buf, " \t")];

		if (*name == 0x00) {
			//just ignore empty lines
			continue;
		}

		//find the end of the name field (note that we don't null-
		//terminate the name field yet, as that would complicate finding
		//the value field below)
		nameEnd = &name[strcspn(name, " \t")];

		//find the value field
		value = &nameEnd[strspn(nameEnd, " \t")];

		//finally, trim any whitespace from the end of the value field
		while (len > 0 && isspace(buf[len - 1])) {
			buf[len - 1] = 0x00;
			len--;
		}

		//now we can null-terminate the name field
		*nameEnd = 0x00;

		//the functions which process the name/value pairs expect a
		//missing value field to be represented as NULL
		if (*value == 0x00)
			value = NULL;

		if(parseTab(parseTable, scriptParams, dialParams, name, value, scriptParams->fname, lineNo) == false)
			errorFree = false;
	}

	if (!feof(file)) {
		//fgets() above returned NULL not because we ran out of file,
		//but because of an error
		goto err2;
	}

	//we're done with the file
	fclose(file);

	//setting 'errno' to B_OK indicates to our caller that, although
	//we may have found syntax errors in the file (our return value will
	//relect that), we finished processing the file successfully
	errno = B_OK;
	return errorFree;

err2:
	fclose(file);
err1:
	return false;
}



void ControlNode::pppconfig(BString &device, BinderNode::property &iface, ifconf_action action)
{
	bs_comm_params_t commParams;
	bsppp_dial_params_t dialParams;
	connect_script_params_t scriptParams;
	BString user, pass, type;
	int sLink = -1, sInet = -1;
	ifreq_t ifreq;
	bone_lognet_params_t lognetParams;
	route_req_t req;

	strncpy(ifreq.ifr_name, device.String(), IFNAMSIZ);
	
	//our caller made sure 'type' has a valid value
	type = iface["type"].String();

	//make sure we can 'goto bail' at any time
	dialParams.connect_script = NULL;

	if ((sInet = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		goto bail;
	}

	switch (action) {
	case BRINGDOWN:
		//
		// just need to bring PPP down
		//
		clr_iface_flag_by_name(device.String(), IFF_UP);
		break;

	case UNADOPT:
		//the profile which created this ppp interface is no longer in
		//use - delete the interface
//XXX hack for geh - don't delete the interface (we'll just re-use
//the current interface next time adopt() is called
//		strncpy(ifreq.ifr_name, device.String(), IFNAMSIZ);
//		ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
		break;

	case ADOPT:
		{
			//
			// adopt() has been called - create a ppp interface of the
			// appropriate type

			//we require that the 'device' parameter be defined
			BinderNode::property dev = iface["device"];
			if (dev.IsUndefined() == true)
				goto bail;
	
			BString physDevice = dev.String();
			if (physDevice == "")
				goto bail;
	
			if ((sLink = socket(AF_LINK, SOCK_DGRAM, 0)) < 0)
			{
				goto bail;
			}
	
			//get the current configuration of the physical interface on
			//which we'll be running
			strncpy(ifreq.ifr_name, physDevice.String(), IFNAMSIZ);
			if (ioctl(sLink, SIOCGIFADDR, &ifreq, sizeof(ifreq)) != 0) {
		
				if (errno != ENODEV) {
					//something bad happened - give up
					perror("SIOCGIFADDR");
					goto bail;
				}
		
				//XXX until interface auto-detection is turned on in the datalink,
				//we'll attempt to load the physical interface here
				bone_interface_params_t ifParams;
				bone_ether_interface_params_t etherParams;
		
				if(type == "ppp")
					ifParams.interface_leaf = "bone_serial";
				else
					ifParams.interface_leaf = "bone_ether";
		
				etherParams.device = physDevice.String();
		
				ifParams.len = sizeof(etherParams);
				ifParams.buf = &etherParams;
		
				if (ioctl(sLink, SIOCADDIFACE, &ifParams, sizeof(ifParams)) != 0) {
					perror("SIOCADDIFACE");
					goto bail;
				}
		
			} else {
				uint8 ifType = ((struct sockaddr_dl *)&ifreq.ifr_addr)->sdl_type;
		
				//the physical interface exists - make sure it is of the proper type
				if(type == "ppp")
				{
					if (ifType != IFT_MODEM) {
						goto bail;
					}
				} else {		
		
					if (ifType != IFT_ETHER) {
						goto bail;
					}
				}
			}
		
			//create the logical interface
			lognetParams.driver = physDevice.String();
			lognetParams.name = device.String();
		
			//the lognet's "flavor", as found in bone.conf, is ppp-type dependent
			if(type == "ppp")
			{
				lognetParams.flavor = 0;
			} else {
				lognetParams.flavor = 1;
			}
		
			if (ioctl(sInet, SIOCAIFADDR, &lognetParams, sizeof(lognetParams)) != 0)
			{
				//that didn't work
				if (errno == B_NAME_IN_USE) {
					DBP("pppconfig(%s): logical interface name collision!\n",
								device.String());
//XXX hack for geh (we don't delete the interface on unadopt, so
//it's ok if we can't re-create it...)
DBP("interface %s already exists - running with it...\n", device.String());
//					goto bail;
		
				} else if (type=="ppp" &&
						   errno == EALREADY)
				{
					//bone_serial_ppp is telling us that there is already a
					//logical interface bound to this device (only one logical
					//interface is allowed on a given serial port at a time) -
					//don't bother continuing (we won't be able to use this
					//serial port until the other logical interface is destroyed)
					DBP("pppconfig(%s): serial port in use!\n", device.String());
					goto bail;
		
				} else {
					//something bad happened - abort
					perror("SIOCAIFADDR");
					goto bail;
				}
			}
		}

		//we're done talking to the physical interface - reset 'ifreq'
		strncpy(ifreq.ifr_name, device.String(), IFNAMSIZ);


		//give the logical interface a zero address (a logical interface
		//must have an address before a route through it can be added)
		memset(&ifreq.ifr_addr, 0, sizeof(ifreq.ifr_addr));
		ifreq.ifr_addr.sa_len = sizeof(ifreq.ifr_addr);
		ifreq.ifr_addr.sa_family = AF_INET;
		if (ioctl(sInet, SIOCSIFADDR, &ifreq, sizeof(ifreq)) != 0) {
			perror("SIOCSIFADDR");
			//delete our ppp logical interface
			ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
			goto bail;
		}
	
		//set the default route to run through the ppp logical interface
		//(note that we don't make it a gateway route - since there's
		//no link-level routing in PPP, this makes no difference)
		memset(&req, 0, sizeof(req));
		req.family = AF_INET;
		req.flags = RTF_DEFAULT;
		memcpy(req.iface, ifreq.ifr_name, IFNAMSIZ);
	
		//start by deleting any current default route
		ioctl(sInet, SIOCDELRT, &req, sizeof(req));
	
		//set new route
		if (ioctl(sInet, SIOCADDRT, &req, sizeof(req)) != 0) {
			perror("failed setting default route");
			//delete our ppp logical interface
			ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
			goto bail;
		}


		//fall through (if the profile specifies that autodial is
		//enabled, we need to fully configure the interface now, too)

	case BRINGUP:
		//the ppp interface has already been created - we'll just
		//(re-)configure it
		memset(&scriptParams, 0, sizeof(scriptParams));
		memset(&dialParams, 0, sizeof(dialParams));
		memset(&commParams, 0, sizeof(commParams));

		if((iface["user_name"]()).IsUndefined()
			|| (iface["password"]()).IsUndefined())
			goto bail;

		user = iface["user_name"].String();
		pass = iface["password"].String();
		if (user == "" || pass == "")
			goto bail;

		//we'll now configure the logical interface
		if(type == "ppp")
		{
			BinderNode::property prop;
			BString modem_init, phone_number, dial_prefix, hidden_init;		
				
			if((iface["modem_init"]()).IsUndefined()
				|| (iface["phone_number"]()).IsUndefined())
				goto bail;
			
			phone_number = iface["phone_number"].String();
			modem_init = iface["modem_init"].String();
			if (phone_number == "" || modem_init == "")
				goto bail;
				
			commParams.port_speed = 115200;
			commParams.data_bits = BS_8_BITS;
			commParams.parity = BS_NO_PARITY;
			commParams.stop_bits = BS_1_STOP_BIT;
			commParams.flow_control = BS_HW_FLOW_CONTROL;
			
			dialParams.username = user.String();
			dialParams.password = pass.String();
			 
			prop = iface["autodial"];
			if(!prop.IsUndefined())
			{
				dialParams.autodial = prop.String() == "1" ? true : false;
			}
	
	
			dialParams.chat_mode = BSPPP_AUTO_CHAT;
			prop = iface["disable_chat"];
			if(!prop.IsUndefined())
			{
				if(prop.String() == "1")
					dialParams.chat_mode = BSPPP_DISABLE_CHAT;	
			}
				
			scriptParams.pulseDial = false;	
			prop = iface["pulse_dial"];
			if(!prop.IsUndefined())
			{
				if(prop.String() == "1")
					scriptParams.pulseDial = true;	
			}
	
			prop = iface["retry_count"];
			if(!prop.IsUndefined())
			{
				dialParams.retry_count = (int) prop.Number();
			}
	
			dialParams.retry_delay = 15000000LL;
			prop = iface["retry_delay"];
			if(!prop.IsUndefined())
			{
				dialParams.retry_delay = (bigtime_t) prop.Number() * 1000000LL;
			}
	
			dialParams.chat_timeout = 3000000LL;
			prop = iface["chat_timeout"];
			if(!prop.IsUndefined())
			{
				dialParams.chat_timeout = (bigtime_t) prop.Number() * 1000000LL;
			}
			
			dialParams.connect_timeout = 60000000LL;
			prop = iface["connect_timeout"];
			if(!prop.IsUndefined())
			{
				dialParams.connect_timeout = (bigtime_t) prop.Number() * 1000000LL;
			}
			
	
			strcpy(scriptParams.fname, "/etc/dialer/defaultscript");
		
			prop = iface["hidden_init"];
			if(!prop.IsUndefined())
			{
				hidden_init = iface["hidden_init"].String();
			}
	
			prop = iface["dial_prefix"];
			if(!prop.IsUndefined())
			{
				dial_prefix = iface["dial_prefix"].String();
			}
		
			scriptParams.hiddenInit = (char *)  hidden_init.String();
			scriptParams.dialPrefix = (char *) dial_prefix.String();
			scriptParams.modemInit = (char *) modem_init.String();
			scriptParams.phoneNum = (char *) phone_number.String();
		
			parse_ppp_script(&scriptParams, &dialParams);

			//set the comm parameters
			memcpy(commParams.if_name, ifreq.ifr_name, IFNAMSIZ);
			if (ioctl(sInet, BONE_SERIAL_SET_COMM_PARAMS, &commParams,
														sizeof(commParams)) != 0)
			{
				//delete our ppp logical interface
				ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
				goto bail;
			}

			//set the dial/chat parameters (note that this will fail with
			//and 'errno' of EALREADY if the interface is currently
			//up - we consider this non-fatal)
			memcpy(dialParams.if_name, ifreq.ifr_name, IFNAMSIZ);
			if (ioctl(sInet, BONE_SERIAL_PPP_SET_PARAMS,
									&dialParams, sizeof(dialParams)) != 0 &&
				errno != EALREADY)
			{
				//delete our ppp logical interface
				ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
				goto bail;
			}
		
		} else {
			bpppoe_user_pass_t pppoeAuthInfo;
	
			//pass username/password to PPPoE (who'll pass it on to PAP/CHAP)
			memcpy(pppoeAuthInfo.if_name, ifreq.ifr_name, IFNAMSIZ);
			pppoeAuthInfo.username = user.String();
			pppoeAuthInfo.password = pass.String();

			if (ioctl(sInet, BONE_PPPOE_SET_USER_PASS,
								&pppoeAuthInfo, sizeof(pppoeAuthInfo)) != 0)
			{
				//delete our ppp logical interface
				DBP("pppconfig(%s): can't set pppoe username/password\n",
							device.String());

				ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
				goto bail;
			}
		}
		
	
		//up the interface (or just reset the autoup flag, if the interface
		//is already up)
		if (ioctl(sInet, SIOCGIFFLAGS, &ifreq, sizeof(ifreq)) != 0) {
			perror("SIOCGIFFLAGS");
			//delete our ppp logical interface
			ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
			goto bail;
		}

		if (dialParams.autodial == true) {
			//our ppp interface is operating in dial-on-demand mode - set
			//IFF_AUTOUP, so the datalink will up it as necessary
			ifreq.ifr_flags |= IFF_AUTOUP;

		} else {
			//don't autodial if we're not supposed to
			ifreq.ifr_flags &= ~IFF_AUTOUP;
		}

		if (action == BRINGUP) {
			//the user want to connect now
			ifreq.ifr_flags |= IFF_UP;
		}
	
		if (ioctl(sInet, SIOCSIFFLAGS, &ifreq, sizeof(ifreq)) != 0) {
			perror("SIOCSIFFLAGS");
			//delete our ppp logical interface
			ioctl(sInet, SIOCDIFADDR, &ifreq, sizeof(ifreq));
			goto bail;
		}
		break;
	}
	
bail:
	if(sLink >= 0)
		close(sLink);
	if(sInet >= 0)
		close(sInet);
	if(dialParams.connect_script)
		free(dialParams.connect_script);
}




//---StatusNode------------------------------------------------------------------
//
// The status node is updated based upon notifications from the kernel on interface changes.
//

StatusNode::StatusNode()
{
	static char *props[] = {
		{"hostname"},
		{"interfaces"},
		{"profile"},
		{"route"},
		0
	};
	
	m_props = props;
	m_num_props = 4;
	
	m_interfaces = new InterfacesNode;
	m_route = new RouteNode;
	observe(BONE_NOTIFY_INTERFACE, 0, 0);
	observe(BONE_NOTIFY_DEFAULTROUTE, 0, 0);
}


BString StatusNode::CurrentProfile()
{
	BAutolock lock(m_lock);
	return m_currentProfile;
}


void StatusNode::SetCurrentProfile(const BString& newProfile)
{
	//don't let anyone read 'm_currentProfile' while we write it
	BAutolock lock(m_lock);

	m_currentProfile = newProfile;
}


get_status_t StatusNode::ReadProperty(const char *name, property &prop, const property_list &)
{
	get_status_t rc = ENOENT;

	if(strcmp(name, "route") == 0)
	{
		prop = (BinderNode *) m_route;
		rc = B_OK;
	} else
	if(strcmp(name, "interfaces") == 0)
	{
		prop = (BinderNode *) m_interfaces;
		rc = B_OK;
	} else
	if(strcmp(name, "profile") == 0)
	{
		//make sure 'm_currentProfile' doesn't change while we read it
		BAutolock lock(m_lock);

		prop = m_currentProfile;
		rc = B_OK;
	} else
	if(strcmp(name, "hostname") == 0)
	{
		char buf[MAXHOSTNAMELEN];
		
		buf[0] = 0;
		
		gethostname(buf, sizeof(buf));
		prop = buf;
		rc = B_OK;
	} 
	
	return rc;
}

void StatusNode::notify(uint32 property, uint32, uint32, void *buf)
{
	uint32 if_index;
	bool refreshRoutes = false;
	
	switch(property)
	{
		case BONE_NOTIFY_INTERFACE:
			memcpy(&if_index, buf, sizeof(if_index));
			refreshRoutes = m_interfaces->updateInterface(if_index);
			NotifyListeners(B_PROPERTY_CHANGED, "interfaces");
		break;
		
		case BONE_NOTIFY_DEFAULTROUTE:
			refreshRoutes = true;
		break;
	}

	if (refreshRoutes == true) {
		m_route->updateRoute();
		NotifyListeners(B_PROPERTY_CHANGED, "route");
	}	
}



//---RouteNode------------------------------------------------------------------

RouteNode::RouteNode()
{
	static char *props[] = {
		{"address"},
		{"interface"},
		0
	};
	
	m_props = props;
	
	m_num_props = 2;
}


get_status_t RouteNode::ReadProperty(const char *name, property &prop, const property_list &)
{
	get_status_t rc = ENOENT;

	if(strcmp(name, "address") == 0)
	{
		prop = m_address.String();
		rc = B_OK;
	} else
	if(strcmp(name, "interface") == 0)
	{
		prop = m_iface.String();
		rc = B_OK;
	} 
	
	return rc;
}

void RouteNode::updateRoute()
{
	route_table_req_t table;
	route_req_t *rrt = 0;
	uint32 i;
	
	m_iface = "none";
	m_address = "none";
	
	/*
	 * get the default route info and store in m_address and m_iface
	 */
	if(get_route_table(&table) == B_OK)
	{
		for(i=0;i<table.cnt; i++)
		{
			rrt = &table.rrtp[i];
			if(rrt->flags & RTF_DEFAULT)
			{
				struct sockaddr_in *sin = NULL;
				ifreq_t ifreq;

				//remember which interface the route passes through
				m_iface = rrt->iface;

				if (rrt->flags & RTF_GATEWAY) {
					//this route has a gateway - use its address
					sin = (struct sockaddr_in *) &rrt->gateway;

				} else {
					//this route doesn't have a gateway - if it's on
					//a point-to-point interface, use the other
					//end of the link's address
					if (get_interface_by_name(rrt->iface, &ifreq) == B_OK &&
						(ifreq.ifr_flags & IFF_PTP) != 0)
					{
						sin = (struct sockaddr_in *)&ifreq.ifr_dstaddr;
					}
				}

				if (sin != NULL) {
					//convert the destination address into text
					char buf[32];
					m_address = inet_ntoa_r(sin->sin_addr, buf);
				}

				//there's only one default route - we're done
				break;
			}
		}
		free(table.rrtp);
	}
	NotifyListeners(B_PROPERTY_CHANGED, "address");
	NotifyListeners(B_PROPERTY_CHANGED, "interface");
}


//---InterfacesNode------------------------------------------------------------------



void InterfacesNode::Acquired()
{
	ifconf_t ifc;
	int i;

	//publish all currently-extant interfaces
	if (get_interface_configuration(&ifc) == 0) {
		for (i = 0; i < ifc.ifc_len / (int)sizeof(ifreq_t); i++)
			updateInterface(ifc.ifc_req[i].ifr_index);

		free_interface_configuration(&ifc);
	}
}


bool InterfacesNode::updateInterface(uint32 if_index)
{
	ifreq_t ifr;
	BinderNode::property prop;
	BString propname;
	char buf[32];
	static const char *pppwords[] = {
		"disconnected",
		"dialing",
		"chatting",
		"negotiating",
		"connected",
		"disconnected",
		"waiting",
	};
	const char *statstr = 0;
	int s;
	bool deleted = false, found = false;
	BinderContainer *iface;
	bool isModem = false;
	
	if(get_interface_by_index(if_index, &ifr) != B_NO_ERROR)
	{
		//this interface is no more
		deleted = true;
	}

	BinderNode::iterator iter = Properties();
	
	while((propname = iter.Next()) != "")
	{
		GetProperty(propname.String(), prop);

		if(prop.IsUndefined())
		{
			DBP("iface name '%s' undefined as property!\n", propname.String());
			continue;
		}
		
		if((prop["index"]()).IsUndefined())
		{
			DBP("%s:index is undefined\n", propname.String());
			continue;		
		}
		if((uint32) (prop["index"]).Number() == if_index)
		{
			found = true;
			break;
		}
	}
	
	if (deleted == true) {
		if (found == true)
			RemoveProperty(propname.String());
		return false;
	}

	if (found == true) {
		iface = dynamic_cast<BinderContainer *> ((BinderNode *)prop.Object());

	} else {
		//this interface isn't in our list - add it
		iface = new BinderContainer;
		iface->AddProperty("index", (int) if_index);
		prop = BinderNode::property(iface);
	}

	//
	// XXX- for PPP, should we be reporting the address or the dest address here?
	//
	iface->AddProperty("address",  inet_ntoa_r(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr, buf));
	iface->AddProperty("mask",  inet_ntoa_r(((struct sockaddr_in *) &ifr.ifr_mask)->sin_addr, buf));

	if ((s = socket(AF_LINK, SOCK_DGRAM, 0)) >= 0)
	{
		ifreq_t ifreq;
		
		memset(&ifreq, 0, sizeof(ifreq));
		ifreq.ifr_index = ifr.ifr_driver_index;

		if (ioctl(s, SIOCGIFNAME, &ifreq, sizeof(ifreq)) != 0 ||
			ioctl(s, SIOCGIFADDR, &ifreq, sizeof(ifreq)) != 0)
		{
			DBP("failed retriving hardware address for interface %s: %s\n", ifr.ifr_name, strerror(errno));

		} else {
			struct sockaddr_dl *addr = (struct sockaddr_dl *)&ifreq.ifr_addr;

			//decode the hardware address
			switch (addr->sdl_type)
			{
				case IFT_LOOP:
					iface->AddProperty("hwaddr",  "zen-like");
					iface->AddProperty("type",  "loopback");
					iface->AddProperty("bandwidth",  10000000);
					break;
				
				case IFT_ETHER:
					{
						uint8 *ab = (uint8 *)LLADDR(addr);
						char buf[20];
						sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
							ab[0],ab[1],ab[2],ab[3],ab[4],ab[5]);
	
						iface->AddProperty("hwaddr",  buf);
						
						if(ifr.ifr_flags & IFF_PTP)
							iface->AddProperty("type",  "pppoe");
						else
							iface->AddProperty("type",  "ethernet");
						iface->AddProperty("bandwidth",  10000000);

					}
					break;
					
				case IFT_PPP:
				case IFT_MODEM:
					iface->AddProperty("type",  "ppp");
					iface->AddProperty("hwaddr",  "unknown");
					isModem = true;
				break;
					
				default:
					DBP("%s: unknown address type %d\n", ifr.ifr_name, addr->sdl_type);
					iface->AddProperty("hwaddr",  "unknown");
					iface->AddProperty("type",  "unknown");
					iface->AddProperty("bandwidth",  0);
					break;
			}
		}

		close(s);
	}
	
	statstr = "unknown";

	if(ifr.ifr_type == LOGT_PPP)
	{
		bsppp_status_t status;

		memset(&status, 0, sizeof(status));
		strcpy(status.if_name, ifr.ifr_name);
		
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
		{
			if(ioctl(s, BONE_SERIAL_PPP_GET_STATUS, &status, sizeof(status)) < 0)
			{
				status.connection_status = BSPPP_DISCONNECTED;
			}
			
			iface->AddProperty("linkstatus",  pppwords[status.connection_status]);
			
			switch (status.connection_status) {
			case BSPPP_DISCONNECTED:
				statstr = "down";
				break;
				
			case BSPPP_CONNECTED:
				statstr = "up";
				break;
			
			default:
				//we're trying to connect
				statstr = "pending";
				break;
			}

			if (isModem == true) {
				//
				// the following code is a hack for working around
				// the sound (ATMx) bug in the trimodem driver
				//
				if(status.connection_status == BSPPP_DIALING) {
					BinderNode::property  audio= BinderNode::Root() / "service" / "audio" ;
					BinderNode::property  modem= BinderNode::Root() / "service" / "audio" / "modem";
					audio->PutProperty("modem", BinderNode::property(fabs(modem.Number())));
				} else {
					BinderNode::property  audio= BinderNode::Root() / "service" / "audio" ;
					BinderNode::property  modem= BinderNode::Root() / "service" / "audio" / "modem";
					audio->PutProperty("modem", BinderNode::property(-fabs(modem.Number())));
				}
	
				//publish the modem connect speed
				iface->AddProperty("bandwidth",  status.connect_speed);
			}
			
			switch (status.last_error) {
				case B_OK:
					iface->AddProperty("linkmsg",  "ok");
					break;
				
				case B_MODEM_NO_CARRIER:
					iface->AddProperty("linkmsg",  "hung up");
					break;
				
				case B_MODEM_BUSY:
					iface->AddProperty("linkmsg",  "busy");
					break;

				case B_MODEM_NO_DIALTONE:
					iface->AddProperty("linkmsg",  "no dialtone");
					break;
				
				case B_MODEM_NO_ANSWER:
					iface->AddProperty("linkmsg",  "no answer");
					break;
				
				case B_MODEM_BAD_LOGIN:
					iface->AddProperty("linkmsg",  "bad login");
					break;
				
				case B_MODEM_TIMEOUT:
					iface->AddProperty("linkmsg",  "timeout");
					break;
				
				default:
					iface->AddProperty("linkmsg",  "error");
					break;
			}

			close(s);
		}

	} else {
		//handle non-ppp interfaces interfaces
		if(ifr.ifr_flags & IFF_UP)
		{
			if(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr == INADDR_ANY ||
				((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr == INADDR_NONE)
				{
					// up but no address, perhaps waiting for DHCP lease.
					statstr = "pending";
				} else {
					statstr = "up";
				}
		} else {
			statstr = "down";
		}
	}
	
	iface->AddProperty("status",  statstr);

	if (found == false) {
		//this is the first time we've seen this interface - now that
		//it's node is completely filled out, let any observers have
		//a crack at it
		AddProperty(ifr.ifr_name, prop);

	} else {
		//let any observers know that this interface has been updated
		NotifyListeners(B_PROPERTY_CHANGED, ifr.ifr_name);
	}

	if (ifr.ifr_flags & IFF_PTP) {
		//routes (including the default route) which go through
		//point-to-point interfaces don't have gateway addresses (there's
		//no need for link-level routing), but the status node makes it
		//appear that they do by replacing the gateway address with the
		//interface's destination address - since this call to
		//updateInterface() may have been the result of our destination
		//address changing, we'll indicate to the status node that it
		//should (perhaps unnecessarily) refresh its list of routes)
		return true;
	}

	//no need to refresh routes
	return false;
}


//---DNSBinderNode------------------------------------------------------------------


DNSBinderNode::DNSBinderNode()
{
	static char *props[] = {
		{"domain"},
		{"primary"},
		{"secondary"},
		0
	};
	
	m_props = props;
	m_num_props = 3;
	m_resolv_conf.SetTo("/etc/resolv.conf", B_READ_WRITE | B_CREATE_FILE);
	parse_resolv_conf();
}

void DNSBinderNode::writefile()
{
	char s[B_PAGE_SIZE + 256];
	
	memset(s, 0, sizeof(s));


	if(m_resolv_conf.InitCheck() != B_NO_ERROR)	
		m_resolv_conf.SetTo("/etc/resolv.conf", B_READ_WRITE | B_CREATE_FILE);

	if(m_resolv_conf.InitCheck() == B_NO_ERROR)
	{		
		if(m_domain != "")
		{
			strcat(s, "domain ");
			strcat(s, m_domain.String());
			strcat(s, "\n");
		}
		
		if(m_primary != "")
		{
			strcat(s, "nameserver ");
			strcat(s, m_primary.String());
			strcat(s, "\n");	
		}
		
		if(m_secondary != "")
		{
			strcat(s, "nameserver ");
			strcat(s, m_secondary.String());
			strcat(s, "\n");
		}
		
		if(s[0] != 0)
		{
			struct stat st;
			size_t slen = strlen(s);
			
			m_resolv_conf.GetStat(&st);
			//
			// IAD hack: bug in cfs puts garbage in shrunken files
			// so we fill with 0
			//
			m_resolv_conf.WriteAt(0, s, max_c(slen, st.st_size));
			m_resolv_conf.GetStat(&m_stat);
		}
	} else {
		DBP("%s\n", strerror(errno));
	}

}


get_status_t DNSBinderNode::ReadProperty(const char *name, property &prop, const property_list &)
{
	struct stat st;
	get_status_t rc = ENFILE;


	if(m_resolv_conf.InitCheck() != B_NO_ERROR)	
		m_resolv_conf.SetTo("/etc/resolv.conf", B_READ_WRITE | B_CREATE_FILE);

	if(m_resolv_conf.InitCheck() == B_NO_ERROR)
	{
		m_resolv_conf.GetStat(&st);
		
		if(st.st_mtime >= m_stat.st_mtime)
		{
			parse_resolv_conf();
		}
		rc = ENOENT;

		if(strcmp(name, "domain") == 0)
		{
			prop = m_domain;
			rc = B_OK;
		} else
		if(strcmp(name, "primary") == 0)
		{
			prop = m_primary;
			rc = B_OK;
		} else
		if(strcmp(name, "secondary") == 0)
		{
			prop = m_secondary;
			rc = B_OK;
		}
	} else {
		DBP("%s\n", strerror(errno));
	}

	return rc;
}

void DNSBinderNode::parse_resolv_conf()
{
	char *buf, *save_ptr, *token;
	bool pri = false, sec = false, dom = false;


	m_primary = "";
	m_domain = "";
	m_secondary = "";
	
	if(m_resolv_conf.InitCheck() == B_NO_ERROR)
	{
		m_resolv_conf.GetStat(&m_stat);
		buf = new char[m_stat.st_size + 1];
		
		memset(buf, 0, sizeof(buf));
		m_resolv_conf.ReadAt(0, buf, m_stat.st_size);
		buf[m_stat.st_size] = 0x00;
		
		token = strtok_r(buf, " \t\r\n", &save_ptr);
		
		while(token != 0)
		{			
			if(strcmp(token, "domain") == 0)
			{
				dom = true;
				token = strtok_r(NULL, " \t\r\n", &save_ptr);
				m_domain = token;
			}
			if(strcmp(token, "nameserver") == 0)
			{
				token = strtok_r(NULL, " \t\r\n", &save_ptr);
				if(!pri)
				{
					pri = true;
					m_primary = token;
				} else {
					if(!sec)
					{
						sec = true;
						m_secondary = token;
					}
				}
			}
			
			token = strtok_r(NULL, " \t\r\n", &save_ptr);
		}
			
		delete buf;
	} else {
		DBP("%s\n", strerror(errno));
	}

}

//--fini-----------------------------------------------------------------

extern "C" _EXPORT BinderNode *return_binder_node()
{
		return new ControlNode();
}


/*
 * testing code
 */
#if BUILD_AS_APP

int main(int , char **)
{
	ControlNode *c;
	BApplication *app;
	
	c = new ControlNode;
	BinderNode::Root() / "service" / "network" / "control" = c;
	app=new BApplication("application/x-vnd.Be-nets");
	app->Run();
	return 0;
	
}

#endif
