// --------------------------------------------------------------------------- 
/* 
	Ethernet test	
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	orig: Jon Watte
			now ownz0red by: Alan Ellis 
			March 27, 2001 
 
	An ethernet tester.
*/ 
// --------------------------------------------------------------------------- 
#ifndef ETHERNET_H
#define ETHERNET_H

#include "Test.h"

#if !defined(TESTDATASIZE)
	#define TESTDATASIZE 100000
#endif

// ---------------------------------------------------------------------------
//	Static IPs can be used for this test if the following undocumented settings
//	are defined...

//	The following four settings must be all present or all absent
//	ethernet.use_dhcp=0
//	ethernet.address=192.168.100.5
//	ethernet.mask=255.255.255.0
//	ethernet.gateway=192.168.100.3
// ---------------------------------------------------------------------------

extern Test* make_ethernet();

class EthernetWindow : public TestWindow
{
	public:		
				EthernetWindow(bigtime_t pulseRate);
				~EthernetWindow();
	
		bool	QuitRequested();
		void	MessageReceived(BMessage * msg);
		void	Show();

		void	SetStringPropertyWrapper(BinderNode::property& PrevNode, const char* NextPropertyName, BString& Value);	
		bool	SetupNetwork();
		BString	DetermineEtherDHCPInterface();
		void 	RestoreNetwork();
		
	
	
		void	StartTest();
		void	StopTest();
	
	
		static void		action(void * win, const char * txt, ...);
		static void		sig_foo(int);
		static status_t	dial_thread(void * win);
		static int		xran();
		static int		generate_testdata();	
		status_t		connect_stuff();

	public:
		BinderNode::property	m_network;
		BinderNode::property	m_dialer;
		BinderNode::property	m_interface;
		BinderNode::property	m_route;
		BString					m_ppp;
		BString					m_dhcp;
		BString					m_address;
		BString					m_mask;
		BString					m_gateway;
		BString					m_PreviousInterface;

		bigtime_t				m_startTime;
		thread_id				m_thread;
	
		bool					m_useDHCP;
		int 					m_failCount;
		char					m_recvdata[TESTDATASIZE];
		
		static int				generate_testdata_once;
		static char				testdata[TESTDATASIZE];
		static volatile int		testok;
		static int				xseed;
		static int				xmul;
		static int				xadd;
		static volatile bool	m_quitting;
};


#endif

