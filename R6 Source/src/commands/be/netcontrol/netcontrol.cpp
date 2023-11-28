#include <stdio.h>
#include <stdlib.h>
#include <OS.h>
#include <SupportKit.h>
#include <StorageKit.h>
#include <AppKit.h>
#include <InterfaceKit.h>

#include <netconfig.h>
#include <nsmessages.h>

// Look down for examples of the control message stuff.

// g++ notification_example.cpp -I/boot/iad/src/inc/net_p -lbe

static const char *kNetControlSig = "application/x-vnd.Be-NETCNTRL";
static const char *kNetServer = "application/x-vnd.Be-NETS";

typedef struct {
	const char *var;
	char *val;
} setting;

class simpleapp : public BApplication {
public:
	simpleapp();
	virtual void MessageReceived(BMessage *msg);
	virtual ~simpleapp() {};

	void RegisterWithNS(void);

	void DialPPP(setting *settings);
	void HangupPPP(void);
	void PrintNetworkInfo(BMessage &reply);
	status_t GetNetworkState(BMessage &reply);
	void PrintCurrentState(void);
	void SwitchToDHCP(void);
	void SwitchToPPP(void);
	void CopySettingsToMessage(BMessage *msg, setting *settings);
	void SavePPPSettings(setting *settings);
	void SavePPPUsername(const char *username);
	void SavePPPPassword(const char *password);
	void SavePPPPhonenumber(const char *phonenumber);
	void SetDefaultConnectionType(const char *type);
	void PrintBandwidthInfo(void);
	void SendSmartcardMessage(setting *settings);
};

simpleapp::simpleapp() : BApplication(kNetControlSig)
{
}

void simpleapp::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case NS_CONNECTION_INFO:
			PrintNetworkInfo(*msg);
		break;
	
		default:
			printf("not nsci->\n");
			msg->PrintToStream();
			printf("\n");
			BApplication::MessageReceived(msg);
		break;
	}
	return;
}

void simpleapp::RegisterWithNS(void)
{

	BMessenger ns(kNetServer);
	BMessage msg(NS_CONNECTION_INFO);
	BMessage reply;

	msg.AddString("appsig", kNetControlSig);
	if (ns.SendMessage(&msg) != B_OK) {
		fprintf(stderr, "Couldnt register with net_server\n");
		exit(-1);
	}
}

// Dial the ppp connection
void simpleapp::DialPPP(setting *settings)
{
	BMessenger ns(kNetServer);
	BMessage msg(PPP_UP);
	
	CopySettingsToMessage(&msg, settings);
	
	ns.SendMessage(&msg);
}

// TearDown the ppp connection
void simpleapp::HangupPPP(void)
{
	BMessenger ns(kNetServer);
	BMessage msg(PPP_DOWN);
	
	ns.SendMessage(&msg);
}

void simpleapp::CopySettingsToMessage(BMessage *msg, setting *settings)
{
	for (int i=0;settings[i].var;i++) {
		if (! settings[i].val)
			continue;
		
		//printf("%s %s\n",settings[i].var,settings[i].val);
		// The execptions
		if (strcmp(settings[i].var,"maxidle") == 0) {
			msg->AddInt32("maxidle", abs(strtol(settings[i].val, NULL, 0)) );
		} else {
			msg->AddString(settings[i].var, settings[i].val);
		}
	}
}

// Save PPP Settings
void simpleapp::SavePPPSettings(setting *settings)
{
	BMessenger ns("application/x-vnd.Be-NETS");
	BMessage msg(NS_CONNECTION_INFO);

	int32 dummy = 0;
	msg.AddInt32("store", dummy);

	CopySettingsToMessage(&msg,settings);
	ns.SendMessage(&msg);

}

void simpleapp::SendSmartcardMessage(setting *settings)
{
	BMessenger ns("application/x-vnd.Be-NETS");
	BMessage msg(PPP_SMARTCARD_INFO);

	CopySettingsToMessage(&msg,settings);
	ns.SendMessage(&msg);
}

// Get current network info
void simpleapp::PrintCurrentState(void)
{
	BMessage reply;
	
	if (GetNetworkState(reply) == B_OK)
		PrintNetworkInfo(reply);

}



status_t simpleapp::GetNetworkState(BMessage &reply)
{
	BMessenger ns("application/x-vnd.Be-NETS");
	BMessage msg(NS_CONNECTION_INFO);

	if (ns.SendMessage(&msg,&reply) != B_OK) {
		printf("Couldn't talk to the net_server - is it runing?\n");
		return B_ERROR;
	}
	return B_OK;
}


void simpleapp::PrintNetworkInfo(BMessage &reply)
{
	const char *p = NULL;

	// PPP or DHCP ?
	if (reply.FindString("connectiontype", &p) != B_OK) {
		printf("You appear to be using a non-BeIA netserver.  Sorry.\n");
		reply.PrintToStream();
		return;
	}

	if (strcmp(p, "dhcp") == 0) {
		printf("Using DHCP\n");
	} else if (strcmp(p, "ppp") == 0) {
		printf("Using PPP\n");
	}
	
	if (reply.FindString("defaultconnectiontype", &p) == B_OK) {
		printf("Using %s mode at startup\n",p);
	}
	
	// DHCP Stuff
	int32 dhcpstate;
	reply.FindInt32("dhcpstate", &dhcpstate);
	if (dhcpstate == NS_ETHER_UP) {
		reply.FindString("dhcpipaddress", &p);
		printf("DHCP IP %s\n",p);
		reply.FindString("dhcpgwaddress", &p);
		printf("DHCP GW %s\n",p);
		if (reply.FindString("dhcpprimarydns", &p) == B_OK) {
			printf("DHCP Primary DNS %s\n",p);
		}
		if (reply.FindString("dhcpsecondarydns", &p) == B_OK) {
			printf("DHCP Secondary DNS %s\n",p);
		}		
	}


	// PPP Stuff
	int32 pppstate;
	reply.FindInt32("pppstate", &pppstate);
	switch (pppstate) {
		case NS_PPP_UP:
			printf("PPP is dialed in\n");
			reply.FindString("pppipaddress", &p);
			printf("PPP IP %s\n",p);
			reply.FindString("pppgwaddress", &p);
			printf("PPP GW %s\n",p);
			if (reply.FindString("pppprimarydns", &p) == B_OK) {
				printf("PPP Primary DNS %s\n",p);
			}
			if (reply.FindString("pppsecondarydns", &p) == B_OK) {
				printf("PPP Secondary DNS %s\n",p);
			}		
		break;
		
		case NS_PPP_DOWN:
			printf("PPP is not dialed\n");
		break;
		
		case NS_PPP_DIALING:
			printf("PPP is dialing\n");
		break;
		
		case NS_PPP_CONNECTING:
			printf("PPP is connecting\n");
		break;
		
		case NS_PPP_DISCONNECTING:
			printf("PPP is disconnecting\n");
		break;
	};
	
	reply.FindString("username", &p);
	printf("PPP username is %s\n",p);
	
	reply.FindString("password", &p);
	printf("PPP password is %s\n",p);
	
	reply.FindString("phonenumber", &p);
	printf("PPP phonenumber is %s\n",p);
	
	if (reply.FindString("modeminit", &p) == B_OK) {
		printf("PPP modeminit is %s\n",p);
	}
	
	if (reply.FindString("serialport", &p) == B_OK) {
		printf("PPP using %s\n",p);
	}
	
	int32 idle_time;
	if (reply.FindInt32("maxidle", &idle_time) == B_OK) {
		if (idle_time != 0)
			printf("PPP Max idle %d seconds\n",idle_time);
		else
			printf("PPP has no timeout set\n");
	}

}



void simpleapp::SwitchToDHCP(void)
{
	BMessenger ns(NET_SERVER);
	BMessage msg(NS_CONNECTION_INFO);

	int32 dummy = 0;
	msg.AddInt32("store", dummy);
	msg.AddString("connectiontype", "dhcp");
	
	ns.SendMessage(&msg);
}

void simpleapp::SwitchToPPP(void)
{
	BMessenger ns(NET_SERVER);
	BMessage msg(NS_CONNECTION_INFO);

	int32 dummy = 0;
	msg.AddInt32("store", dummy);
	msg.AddString("connectiontype", "ppp");
	
	ns.SendMessage(&msg);
}

void simpleapp::SavePPPUsername(const char *username)
{
	BMessenger ns(NET_SERVER);
	BMessage msg(NS_CONNECTION_INFO);
	int32 dummy = 0;
	msg.AddInt32("store", dummy);
	msg.AddString("username",username);
	ns.SendMessage(&msg);
}

void simpleapp::SavePPPPassword(const char *password)
{
	BMessenger ns(NET_SERVER);
	BMessage msg(NS_CONNECTION_INFO);
	int32 dummy = 0;
	msg.AddInt32("store", dummy);
	msg.AddString("password",password);
	ns.SendMessage(&msg);
}

void simpleapp::SavePPPPhonenumber(const char *phonenumber)
{
	BMessenger ns(NET_SERVER);
	BMessage msg(NS_CONNECTION_INFO);
	int32 dummy = 0;
	msg.AddInt32("store", dummy);
	msg.AddString("phonenumber",phonenumber);
	ns.SendMessage(&msg);
}

void simpleapp::SetDefaultConnectionType(const char *type)
{
	BMessenger ns(NET_SERVER);
	BMessage msg(NS_CONNECTION_INFO);
	int32 dummy = 0;
	msg.AddInt32("store", dummy);
	msg.AddString("defaultconnectiontype",type);
	ns.SendMessage(&msg);
}

void simpleapp::PrintBandwidthInfo(void)
{
	BMessenger ns(kNetServer);
	BMessage msg(NS_BANDWIDTH_INFO);
	BMessage reply;

	if (ns.SendMessage(&msg,&reply) == B_OK) {
		float bps,hiwater;
		if (reply.FindFloat("be:bps",&bps) == B_OK)
			printf("BPS : %f\n",bps);
		if (reply.FindFloat("be:hiwater",&hiwater) == B_OK)
			printf("Hiwater : %f\n",hiwater);
	}
}


// Gratefully stolen from ush
typedef struct 
{
	status_t (*func)(simpleapp *app, int argc, char **argv);
	int minargs;
	const char *name;
	const char *help;
} command;

status_t do_help(simpleapp *app, int argc, char **argv);


status_t do_show(simpleapp *app, int argc, char **argv)
{
	app->PrintCurrentState();
	return 0;
}

status_t do_ppp(simpleapp *app, int argc, char **argv)
{
	app->SwitchToPPP();
	return 0;
}

status_t do_dhcp(simpleapp *app, int argc, char **argv)
{
	app->SwitchToDHCP();
	return 0;
}

void get_cli_settings(setting *settings, int argc, char **argv)
{
	for (int j=0;j<argc;j++) {
		const char *str = argv[j];
		char *eqs,*val;
		const char *var;
		if (eqs = (char *) strchr(str, '=')) {
			var = str;
			val = eqs + 1;
			*eqs = '\0';
			if (val == NULL) continue;	
			if (var == NULL) continue;
			
			for (int i=0;settings[i].var;i++) {
				if (strcmp(settings[i].var, var) == 0)
					settings[i].val = val;
			}
		}
	}
}

static setting gSettings[] = {
	{ "username", NULL },
	{ "password", NULL },
	{ "phonenumber", NULL },
	{ "modeminit", NULL },
	{ "standard", NULL },
	{ "maxidle", NULL },
	{ "serialport", NULL },
	{ NULL, NULL}
};
	

status_t do_set(simpleapp *app, int argc, char **argv)
{
	if (strcmp(*argv,"help") == 0) {
		printf("Settable parameters are:\n");
		for (int i=0;gSettings[i].var;i++) {
			printf("%s\n", gSettings[i].var);
		}
	}

	get_cli_settings(gSettings, argc, argv);
	
	app->SavePPPSettings(gSettings);

	return 0;
}

status_t do_dial(simpleapp *app, int argc, char **argv)
{
	get_cli_settings(gSettings, argc, argv);

	app->DialPPP(gSettings);

	return 0;
}

status_t do_smartcard(simpleapp *app, int argc, char **argv)
{
	get_cli_settings(gSettings, argc, argv);
	
	app->SendSmartcardMessage(gSettings);
	
	return 0;
}

status_t do_hangup(simpleapp *app, int argc, char **argv)
{
	app->HangupPPP();
	return 0;
}

status_t do_default_connection_type(simpleapp *app, int argc, char **argv)
{
	if ((strcmp(argv[0],"dhcp") != 0) && (strcmp(argv[0],"ppp") != 0)) {
		printf("Must use dhcp or ppp.\n");
		do_help(app, 0, NULL);
	}
	
	app->SetDefaultConnectionType(argv[0]);
	return 0;
}

status_t do_listen(simpleapp *app, int argc, char **argv)
{
	app->RegisterWithNS();
	app->Run();
	return 0;
}

status_t do_bps(simpleapp *app, int argc, char **argv)
{
	app->PrintBandwidthInfo();
	return 0;
}

command commands[] = {
	{ do_set, 1, "set", "param1=value1 param2=value2 ... Use set help to list params"},
	{ do_dhcp, 0, "dhcp", "Use DHCP to connect to the world."},
	{ do_ppp, 0, "ppp", "Use PPP to connect to the world."},
	{ do_dial, 0, "dial", "Dial your ISP."},
	{ do_hangup, 0, "hangup", "Hangup from your ISP."},
	{ do_default_connection_type, 1, "default", "[ppp | dhcp]  Set default connection type.\n"},
	{ do_listen, 0, "listen", "Listen for net_server notifications"},
	{ do_bps, 0, "bps", "Show bandwidth info"},
	{ do_smartcard, 0, "smartcard", "Send a fake smartcard message"},
	{ do_help, 0, "help", "Show commands."},
	{ do_help, 0, "--help", "Show commands."},
	{ NULL, 0, NULL, NULL}
};

status_t do_help(simpleapp *app, int argc, char **argv)
{
	int i;
	
	printf("Commands are:\n");
	for (i=0;commands[i].func;i++) {
		printf("%s : %s\n",commands[i].name, commands[i].help);
	}
	return 0;
}

status_t
parse_and_go(simpleapp *app, int argc, char **argv)
{
	int i;
		
	for(i=0;commands[i].func;i++){
		if(!strcmp(argv[0],commands[i].name)) {
			argc--; argv++;
			if(argc < commands[i].minargs){
				fprintf(stderr,"%s : requires at least %d argument%s\n",
						commands[i].name,commands[i].minargs,commands[i].minargs>1?"s":"");
				fprintf(stderr,"%s : %s\n",commands[i].name,commands[i].help);
				return 1;
			} else {
				return commands[i].func(app,argc,argv);
			}
		}
	}

	// fallback
	return do_show(app, argc, argv);
}


int
main(int argc, char **argv)
{
	simpleapp tapp;
	if (argc == 1) {
		do_show(&tapp, argc, argv);
	} else {
		argc--;
		argv++;
		return parse_and_go(&tapp, argc, argv);
	}
	return 0;
}


