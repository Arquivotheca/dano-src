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
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <Alert.h>
#include <Binder.h>
#include <Button.h>
#include <OS.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>

#include "ValidApp.h"
#include "settings.h"

#include "ethernet.h"

#if !defined(NETTEST_PORT)
	#define NETTEST_PORT 6909
#endif

#if !defined(NETTEST_SERVER)
	//	10.113.215.130	--	some random server address on a local-only net
	//	The idea being that the factory has one server which gives out DHCP leases,
	//	and which also runs the server program (testsrv.c). Because that net is
	//	factory local and test specific, we can safely just hard-code one specific
	//	IP address, and document that as how it's gotta be done.
	#define NETTEST_SERVER 0x0a71d782
#endif


#if !defined(BLOCKSIZESTART)
	#define BLOCKSIZESTART 1
#endif

#if !defined(BLOCKSIZEINCREMENT)
	#define BLOCKSIZEINCREMENT 17
#endif


//	Just a stupid incremental accumulator pseudo-random-number-generator.
//	I ran some minimal tests on the coefficients to see that they generate
//	a reasonably even spread. Don't need anything fancy anyway; this is
//	just to have a pattern that's the same on Linux and BeOS.
volatile bool EthernetWindow::m_quitting;
volatile int EthernetWindow::testok = 0;
char EthernetWindow::testdata[TESTDATASIZE];
int EthernetWindow::xseed;
int EthernetWindow::xmul = 97;
int EthernetWindow::xadd = 11147;
int generate_testdata_once = EthernetWindow::generate_testdata();

// --------------------------------------------------------------------------------------------

Test* make_ethernet()
{
	EthernetWindow * kkw = new EthernetWindow(1000000);
	return kkw->GetTest();
}

// --------------------------------------------------------------------------------------------
static const void * mycmp(const void * a, const void * b, size_t s)
{
	const uchar * aa = (const uchar *)  a;
	const uchar * bb = (const uchar *)  b;
	size_t o = 0;

	while (o < s)
	{
		if (aa[o]  != bb[o])
		{
			fprintf(stderr, "difference at %ld: 0x%02x != 0x%02x\n", o, aa[o], bb[o]);
			fprintf(stderr, "%.10s vs %.10s\n", aa + o, bb + o);
			return aa + o;
		}
		o++;
	}
	return NULL;
}
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------

EthernetWindow::EthernetWindow(bigtime_t pulseRate)
               :TestWindow(BRect(100, 100, 400, 350), "Ethernet Test",
                           B_TITLED_WINDOW, B_NOT_RESIZABLE |
                           B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
                           B_NOT_CLOSABLE, pulseRate),
               m_useDHCP(true),            
               m_failCount(0)
{
	// default is to use DHCP
	m_useDHCP = get_setting_value("ethernet.use_dhcp", 1)  == 1;

	BTextView * tv = new BTextView(BRect(0, 0, 300, 200), "text", BRect(10, 10, 290, 90), B_FOLLOW_ALL);
	AddChild(tv);
	m_failCount = 0;
	BButton * fail = new BButton(BRect(10, 202, 90, 230), "fail", "Fail", new BMessage('fail'));
	AddChild(fail);
	m_thread = -1;
	m_quitting = false;

	tv->SetFont(be_fixed_font);
	tv->SetFontSize(12);
}
// --------------------------------------------------------------------------------------------
EthernetWindow::~EthernetWindow()
{
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::SetStringPropertyWrapper(BinderNode:: property &PrevNode, const char *NextPropertyName, BString &Value)
{
	BinderNode::property returnVal;
	BinderNode::property arg(Value);
	BinderNode::property_list argList;
	argList.AddItem(&arg);
	PrevNode->GetProperty(NextPropertyName, returnVal, argList);
}
// --------------------------------------------------------------------------------------------
bool EthernetWindow::SetupNetwork()
{
	BString testInterface("staticip");

	if (1 == get_setting_value("ethernet.use_dhcp", 1))
	{
		testInterface = "dhcp";
	}

	EthernetWindow::action(this, "Setting up network for %s.\n", testInterface.String());

	BinderNode::property controlNode = BinderNode::Root()  / "service" / "network" / "control";

	m_PreviousInterface = (controlNode / "status" / "profile");

	printf("Prev = %s\n", m_PreviousInterface.String());

	controlNode["down"]  ();
	SetStringPropertyWrapper(controlNode, "adopt", testInterface);
	controlNode["up"]  ();

	if (true == (controlNode / "status" / "profile")().IsUndefined())
	{
		action(this, "Error: service/network/control/status/profile is '<undefined>', possible bad card/conection.\n");
		return false;
	}

	printf("service/network/control set to %s\n", (controlNode / "status" / "profile").String().String());

	if (m_useDHCP)
	{
		//Binder now handles dhcp requests asynchronously, since
		//they can take quite a while to complete if the server is
		//busy or not available - as such, we'll poll the network
		//status for awhile, ensuring that the network is up when
		//we return

		// Figure out which interface is using ehternet and dhcp
		BString interface(DetermineEtherDHCPInterface());

		printf("Interface = %s\n", interface.String());

		if (interface.Length()  == 0)
		{
			action(this, "Unable to determine dhcp interface, possible bad connection/card.\n");
			return false;
		}

		const int timeout = 10;
		int i = 0;
		for (; i < timeout; i++)
		{
			if ( (BinderNode::Root() / "service" / "network" /
			      "control" / "status" / "interfaces" /
			      interface.String())["status"].String() == "up"
			   )
			{
				//we successfully obtained an ip address - we can
				//continue with our test
				break;
			}

			//wait a bit, then check again
			snooze(500000LL);

			// Some sort of progress indicator is nice.
			EthernetWindow::action(this, ".");
		}

		printf("\nThis took %.1f seconds.\n", (float)i / 2.0);

		if (timeout == i)
		{
			EthernetWindow::action(this, "\nA Timeout occured.");
			return false;
		}

		EthernetWindow::action(this, "\n");
	}
	
	return true;
}
// --------------------------------------------------------------------------------------------
BString EthernetWindow::DetermineEtherDHCPInterface()
{
	{
		BString result;
		BinderNode::property interfaces = BinderNode::Root()  / "service" /
		        "network" / "profiles" / "dhcp" / "interfaces";

		BinderNode::iterator iFaceIter = interfaces->Properties();
		BString iFaceName;

		while ((iFaceName = iFaceIter.Next()).Length()  != 0)
		{
			BinderNode::property settings = interfaces[iFaceName.String()];

			if (settings["dhcp"]  == "1")
			{
				if (settings["type"]  == "ethernet")
				{
					result = iFaceName;
					break;
				}
			}
		}

		return result;
	}
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::RestoreNetwork()
{
	// restore prior network setup
	EthernetWindow::action(this, "Restoring prior network settings.\n");

	BinderNode::property control(BinderNode::Root() / "service" / "network" / "control");

	control["down"]();
	SetStringPropertyWrapper(control, "adopt", m_PreviousInterface);

	printf("service/network/control set to %s\n", (control / "status" / "profile").String().String());
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::StartTest()
{
	if (m_thread < 0 && m_quitting == false)
	{
		m_thread = spawn_thread(dial_thread, "dial_thread", 10, this);
		resume_thread(m_thread);
		m_startTime = system_time();
	}
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::StopTest()
{
	m_quitting = true;
	while (m_thread > 0)
	{
		fprintf(stderr, "signalling threads to quit\n");
		send_signal(m_thread, 1);
		snooze(500000);
	}
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::Show()
{
	TestWindow::Show();

	BTextView * tv = dynamic_cast < BTextView * > (FindView("text"));

	if (tv != NULL)
		tv->SetFontSize(12);

	StartTest();
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::action(void *win, const char *txt, ...)
{
	char str[600];
	va_list vl;
	va_start(vl, txt);
	vsprintf(str, txt, vl);
	va_end(vl);
	fprintf(stderr, "%s", str);
	BMessage msg('text');
	msg.AddString("text", str);
	((BWindow *)  win)  ->PostMessage(&msg);
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::sig_foo(int)
{
	m_quitting = true;
	testok = 0;
}
// --------------------------------------------------------------------------------------------
status_t EthernetWindow::dial_thread(void *win)
{
	signal(1, sig_foo);
	EthernetWindow * mw = (EthernetWindow *)  win;

	status_t err = B_OK;
	if( true == mw->SetupNetwork() )
	{
	
		err = mw->connect_stuff();
	
		if (err < 0)
		{
			fprintf(stderr, "connect_stuff returned and errno = %s (%x/%lx)\n", strerror(errno), errno, err);
			action(win, "connect_stuff(): %s\n", strerror(err));
		}
	}
	else
	{
		err = B_ERROR;
	}
	
	mw->RestoreNetwork();

	mw->m_thread = -1;
	BMessage msg('done');
	msg.AddInt32("error", err);
	mw->PostMessage(&msg);
	return err;
}
// --------------------------------------------------------------------------------------------
int EthernetWindow::xran()
{
	xseed = xseed * xmul + xadd;
	return xseed & 0x7fff;
}
// --------------------------------------------------------------------------------------------
int EthernetWindow::generate_testdata()
{
	int ix;
	for (ix = 0; ix < TESTDATASIZE; ix++)
	{
		testdata[ix]  = xran()  >> 7;
	}
	return 0;
}
// --------------------------------------------------------------------------------------------
status_t EthernetWindow::connect_stuff()
{
	status_t err = B_OK;
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NETTEST_PORT);
	addr.sin_addr.s_addr = htonl(NETTEST_SERVER);
	char address[30] = "10.113.215.130";
	if (get_setting("ethernet.server_address", address, sizeof(address)) != NULL)
	{
		int a, b, c, d;
		if (4 == sscanf(address, "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			addr.sin_addr.s_addr = htonl(((a << 24)  | (b << 16)  | (c << 8)  | (d)));
		}
		else
		{
			fprintf(stderr, "address '%s' must be in x.y.z.w numeric format\n", address);
			action(this, "Bad ethernet.address found, using default.\n");
		}
		fprintf(stderr, "address: %ld.%ld.%ld.%ld\n",
		        (ntohl(addr.sin_addr.s_addr)  >> 24)  & 0xff,
		        (ntohl(addr.sin_addr.s_addr)  >> 16)  & 0xff,
		        (ntohl(addr.sin_addr.s_addr)  >> 8)  & 0xff,
		        (ntohl(addr.sin_addr.s_addr)  >> 0)  & 0xff);
	}

	action(this, "Connecting via %s to %s.\n", m_useDHCP ? "DHCP" : "static IP", address);

	if (connect(sock, (struct sockaddr *)  & addr, sizeof(addr))  < 0)
	{
		err = errno;
		fprintf(stderr, "connect(): %s\n", strerror(err));
	}

	if (!err)
	{
		char * ptr = m_recvdata;
		char * end = ptr + TESTDATASIZE;
		int blocksize;

		signal(SIGALRM, sig_foo);
		memset(m_recvdata, 0, TESTDATASIZE);
		fprintf(stderr, "m_recvdata = %lx, and testdata = %lx\n", (uint32)  m_recvdata, (uint32)  testdata);
		testok = 1;
		action(this, "Talking to server.\n");
		//	try with varying block sizes to make sure they all work
		//	the other side needs to use the same algorithm because we're synchronous
		for (blocksize = BLOCKSIZESTART; (ptr < end)  && testok; blocksize += BLOCKSIZEINCREMENT)
		{
			int ptrbase = 0;
			if (ptr + blocksize > end)
			{
				blocksize = end - ptr;
			}
			alarm(3); 	//	three second timeout should be plenty

			int s;
again:
			s = recv(sock, ptr + ptrbase, blocksize - ptrbase, 0);
			if (s < 0)
			{
				err = errno;
				fprintf(stderr, "recv: %s\n", strerror(errno));
				fail(strerror(errno));
				perror("recv()");
			}
			else if (s == 0)
			{
				err = errno ? errno : B_IO_ERROR;
				fail(strerror(errno));
				fprintf(stderr, "0-size recv()\n");
			}
			else if (s < blocksize - ptrbase)
			{
				ptrbase += s;
				goto again;
			}
			else
			{
				s += ptrbase;
				int r = send(sock, &testdata[ptr - m_recvdata], blocksize, 0);
				if (s > blocksize)
				{
					err = B_ERROR;
					fail("long recv()");
					fprintf(stderr, "recv(): long receive (!?) %d\n", s);
				}
				else if (s == 0)
				{
					err = B_ERROR;
					fail("zero-size recv()");
					fprintf(stderr, "recv(): zero bytes\n");
				}
				else if (s < blocksize)
				{
					err = B_ERROR;
					fail("short recv()\n");
					fprintf(stderr, "short recv(): %d\n", s);
				}
				else
				{
					ptr += blocksize;
				}
				if (r != s)
				{
					fprintf(stderr, "bad send(): %d (%s)\n", r, strerror(err = errno));
					if (err >= 0)  err = B_ERROR;
					fail("bad send()\n");
				}
			}
			alarm(0);
		}
		const void * v;
		if (testok && (v = mycmp(testdata, m_recvdata, TESTDATASIZE)))
		{
			fprintf(stderr, "data corruption, diff @ %ld, inputs %p, %p\n", (int32)  v, testdata, m_recvdata);
			fail("data corruption");
			err = B_ERROR;
		}
	}
	alarm(2);

	//	wait for status from server
	char status[10];
	status[0]  = 0;
	recv(sock, status, 1, 0);
	alarm(0);

	if (status[0]  == '2')
	{
		//	declare success
	}

	else
	{
		fprintf(stderr, "server error status: %c (%d)\n", status[0], status[0]);
		fail("server returned error status (check server log file)");
		err = B_ERROR;
	}

	action(this, "Done (error %s).\n", strerror(err));
	close(sock);
	return err;
}
// --------------------------------------------------------------------------------------------
bool EthernetWindow::QuitRequested()
{
	// make sure the test is stopped when we abort
	StopTest();
	return TestWindow::QuitRequested();
}
// --------------------------------------------------------------------------------------------
void EthernetWindow::MessageReceived(BMessage *msg)
{
	int32 error = B_ERROR;
	const char * text;

	switch (msg->what)
	{
	case 'fail':
		fail("Connection cancelled by operator\n");
		StopTest();
		this->TestDone(false);
		break;

	case 'done':
		// Well, it should not be possible to get a message which has no 'error'
		//  field, but the code was like this in the first place, so I don't want
		//  to change the logic just yet.
		if (B_OK != msg->FindInt32("error", &error)  || (B_OK != error))
		{
			// only show the alert on a true fail
			if ((m_quitting == false) && (m_failCount < 4))
			{
				m_failCount++;
				BString str("Ethernet connection failed (Try #");
				str << m_failCount;
				str += ") :";
				str += strerror(error);

				BAlert * alert = new BAlert("", str.String(), m_failCount < 3 ? "Again" : "Fail");
				SetLargeAlertFont(alert);
				alert->Go();

				if (m_failCount > 2)
				{
					fail(str.String());
					TestDone(false);
				}
				else
				{
					reset_failure();
					StartTest();
				}
			}
		}
		else
		{
			TestDone(true);
		}
		break;

	case 'text':
		if (!msg->FindString("text", &text))
		{
			BTextView * tv = static_cast < BTextView * > (FindView("text"));
			tv->Insert(tv->TextLength(), text, strlen(text));
			int line = tv->LineAt(tv->TextLength());
			float h = tv->TextHeight(0, line);
			if (h > tv->Bounds().Height())
				tv->ScrollTo(BPoint(0, h - tv->Bounds().Height()));
			tv->Draw(tv->Bounds());
			tv->Flush();
		}
		break;

	case 'puls':
		//	50 seconds for whole test, maximum
		if (system_time()  - m_startTime > 50000000)
		{
			m_startTime = B_INFINITE_TIMEOUT;
			BMessage msg('text');
			msg.AddString("text", "Ethernet timed out\n");
			PostMessage(&msg);
			StopTest();
			msg.what = 'done';
			msg.AddInt32("error", B_TIMED_OUT);
			PostMessage(&msg);
		}
		break;

	default:
		TestWindow::MessageReceived(msg);
		break;
	}
}
// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------


