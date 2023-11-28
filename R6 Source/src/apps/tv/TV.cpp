#include "TV.h"
#include "prefs.h"

#include <MediaRoster.h>
#include <MediaTheme.h>
#include <TimeSource.h>
#include <Alert.h>
#include <TextView.h>
#include <MediaAddOn.h>

#include <scheduler.h>
#include <string.h>
#include <stdio.h>

static void
list_nodes()
{
	int nnodes = 50;
	live_node_info * lni = new live_node_info[nnodes];
	while (1) {
		int32 count = nnodes;
		status_t err = BMediaRoster::Roster()->GetLiveNodes(
			lni, &count, NULL, NULL, NULL, 0);
		if (err < B_OK) {
//			fprintf(stderr, "can't get list of nodes: %s [%x]\n", strerror(err), err);
			delete[] lni;
			return;
		}
		if (count > nnodes) {
			count += 50;
			delete[] lni;
			nnodes = count;
			lni = new live_node_info[nnodes];
			continue;
		}
		nnodes = count;
		break;
	}
	fprintf(stderr, "%d nodes:\n", nnodes);
	for (int ix=0; ix < nnodes; ix++) {
		fprintf(stderr, "%5d %30s%s%s%s%s\n", lni[ix].node.node, lni[ix].name,
			(lni[ix].node.kind & B_BUFFER_CONSUMER) ? " CONSUMER" : "",
			(lni[ix].node.kind & B_BUFFER_PRODUCER) ? " PRODUCER" : "",
			(lni[ix].node.kind & B_TIME_SOURCE) ? " TIME_SOURCE" : "",
			(lni[ix].node.kind & ~(B_BUFFER_CONSUMER|B_BUFFER_PRODUCER|B_TIME_SOURCE)) ?
				" {other}" : "");
	}
}

static void
error(const char * message, status_t err)
{
	char msg[256];
	sprintf(msg, "%s\n%s [%lx]", message, strerror(err), err);
	(new BAlert("", msg, "Quit"))->Go();
	list_nodes();
	be_app->PostMessage(B_QUIT_REQUESTED);
}

class ErrorView : public BTextView {
public:
	ErrorView(BRect r, const char * t) : BTextView(r, "error", r.InsetByCopy(3, 3), 
		B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
		{
			SetText(t);
			SetFontSize(18.0);
			SetAlignment(B_ALIGN_CENTER);
		}
	void FrameResized(float w, float h)
		{
			BTextView::FrameResized(w, h);
			BRect r(3,3,w-3,h-3);
			SetTextRect(r);
			FillRect(Bounds(), B_SOLID_LOW);
			Draw(Bounds());
			Flush();
		}
};


TVApp::TVApp() : BApplication("application/x-vnd.be.mk-tv-app")
{
	status_t err;
	int32 count;
	dormant_node_info dormantNode;
	media_node newConsumer;
	media_format format;
	bigtime_t latency, initLatency, perf, real;
	BTimeSource *source;
	bool running;
	BParameterWeb *web;

	m_window = NULL;
	m_num_nodes = 0;

	/* connect to the media server */
	m_roster = BMediaRoster::Roster();
	if (!m_roster) {
		(new BAlert("", "Cannot connect to the media server!", "Quit"))->Go();
		PostMessage(B_QUIT_REQUESTED);
		goto err1;
	}
	
	/* find TimeSource */
	err = m_roster->GetTimeSource(&timesourceNode);
	if (err < B_OK) {
		error("Can't get TimeSource!", err);
		goto err2;
	}

	/* find video source */
	err = m_roster->GetVideoInput(&m_nodes[m_num_nodes].node);
	if (err < B_OK) {
		error("Can't find any video input!", err);
		goto err2;
	}

	/* find free output from video source */
	err = m_roster->GetFreeOutputsFor(m_nodes[m_num_nodes].node,
			&m_nodes[m_num_nodes].output, 1, &count, B_MEDIA_RAW_VIDEO);
	if ((err < 0) || (count < 1)) {
		dormant_node_info dni[1];
		media_format input, output;

		err = m_roster->GetFreeOutputsFor(m_nodes[m_num_nodes].node,
				&m_nodes[m_num_nodes].output, 1, &count,
				B_MEDIA_ENCODED_VIDEO);
		if ((err < 0) || (count < 1)) {
			error("The video input is busy!", err);
			goto err3;
		}
		m_num_nodes++;

		count = sizeof(dni) / sizeof(dni[0]);
		input.type = B_MEDIA_ENCODED_VIDEO;
		input.u.encoded_video = media_encoded_video_format::wildcard;
		output.type = B_MEDIA_RAW_VIDEO;
		output.u.raw_video = media_raw_video_format::wildcard;
		err = m_roster->GetDormantNodes(dni, &count, &input, &output);
		if (err < B_OK) {
			error("Error getting dormant nodes", err);
			goto err3;
		}
		if (count == 0) {
			error("Unable to find decoder node", err);
			goto err3;
		}

		err = m_roster->InstantiateDormantNode(
				dni[0], &m_nodes[m_num_nodes].node, B_FLAVOR_IS_LOCAL);
		if (err < B_OK) {
			error("Unable to instantiate decoder node", err);
			goto err3;
		}

		err = m_roster->GetFreeInputsFor(m_nodes[m_num_nodes].node,
				&m_nodes[m_num_nodes].input, 1, &count,
				B_MEDIA_ENCODED_VIDEO);
		if ((err < 0) || (count < 1)) {
			error("The decoder input is busy!", err);
			goto err3;
		}

		err = m_roster->GetFreeOutputsFor(m_nodes[m_num_nodes].node,
				&m_nodes[m_num_nodes].output, 1, &count,
				B_MEDIA_RAW_VIDEO);
		if ((err < 0) || (count < 1)) {
			error("The decoder output is busy!", err);
			goto err3;
		}
	}
	m_num_nodes++;

	/* find video output to make sure we can display */
	err = m_roster->GetVideoOutput(&m_nodes[m_num_nodes].node);
	if (err < B_OK) {
		error("Can't display video!", err);
		goto err3;
	}

	err = m_roster->GetDormantNodeFor(m_nodes[m_num_nodes].node, &dormantNode);
	if (err == B_OK)
		err = m_roster->InstantiateDormantNode(
				dormantNode, &newConsumer, B_FLAVOR_IS_LOCAL);
	if (err == B_OK) {
		m_roster->ReleaseNode(m_nodes[m_num_nodes].node);
		m_nodes[m_num_nodes].node = newConsumer;
	}

	/* find free input to video window */
	err = m_roster->GetFreeInputsFor(m_nodes[m_num_nodes].node,
			&m_nodes[m_num_nodes].input, 1, &count,
			B_MEDIA_RAW_VIDEO);
	if ((err < 0) || (count < 1)) {
		error("The video output is busy!", err);
		goto err3;
	}
	m_num_nodes++;

	/* connect them */
	for (int32 i=0;i<m_num_nodes-1;i++) {
		if (i == m_num_nodes - 2) {
			format.type = B_MEDIA_RAW_VIDEO;
			format.u.raw_video = media_raw_video_format::wildcard;
		} else {
			format.type = B_MEDIA_ENCODED_VIDEO;
			format.u.encoded_video = media_encoded_video_format::wildcard;
		}
		err = m_roster->Connect(
				m_nodes[i].output.source,
				m_nodes[i+1].input.destination,
				&format,
				&m_nodes[i].output,
				&m_nodes[i+1].input);
		if (err < B_OK) {
			error("Couldn't connect nodes", err);
			goto err4;
		}
	}

	/* Set time source for each node */
	for (int32 i=0;i<m_num_nodes;i++) {
		err = m_roster->SetTimeSourceFor(
				m_nodes[i].node.node, timesourceNode.node);
		if (err < B_OK) {
			error("Couldn't set time source", err);
			goto err4;
		}
	}

	// XXX: do more?
	err = m_roster->GetLatencyFor(m_nodes[m_num_nodes-1].node, &latency);
//	err = m_roster->SetProducerRunModeDelay(m_nodes[0].node, latency,
//			BMediaNode::B_RECORDING);
	if(m_num_nodes == 3) {
		m_roster->SetRunModeNode(m_nodes[1].node, BMediaNode::B_OFFLINE);
	}


	/* start them */
	err = BMediaRoster::Roster()->GetInitialLatencyFor(
			m_nodes[0].node, &initLatency);
	if (err < B_OK) {
		error("error getting initial latency for fCaptureNode", err);	
		goto err4;
	}
	initLatency += estimate_max_scheduling_latency();

	source = BMediaRoster::Roster()->MakeTimeSourceFor(
			m_nodes[m_num_nodes-1].node);
	running = source->IsRunning();
//	printf("running: %s\n", running ? "true" : "false");
	
	// workaround for folks without a sound card!
	// system time source isn't running for those poor souls
	// so we need to start it
	real = BTimeSource::RealTime();	
	if (!running) {
		err = m_roster->StartTimeSource(timesourceNode, real);
		if (err < B_OK) {
			error("Couldn't start TimeSource!", err);
			goto err4;
		}
		err = m_roster->SeekTimeSource(timesourceNode, 0, real);
		if (err < B_OK) {
			error("Couldn't seek TimeSource!", err);
			goto err4;
		}
	}
	perf = source->PerformanceTimeFor(real + latency + initLatency);
	source->Release();

//	printf("perf = %.4f real = %.4f\n", (double)perf/1000000., (double)real/1000000.);
	
	for (int32 i=0;i<m_num_nodes;i++) {
		err = m_roster->StartNode(m_nodes[i].node, perf);
		if (err < B_OK) {
			error("Couldn't start node", err);
			goto err5;
		}
	}

	/* create control panel */
	err = m_roster->GetParameterWebFor(m_nodes[0].node, &web);
	get_prefs();
	if ((err == B_OK) && (web != NULL)) {
		BView *view = BMediaTheme::ViewFor(web);
		/* create control window */
		m_window = new ControlWindow(BRect(prefs.x, prefs.y,
				view->Bounds().right+prefs.x, view->Bounds().bottom+prefs.y),
				view, m_nodes[0].node);
		BMediaRoster::Roster()->StartWatching(BMessenger(NULL, m_window),
				m_nodes[0].node, B_MEDIA_WEB_CHANGED);
	} else {
		/* create an empty window */
		m_window = new ControlWindow(
				BRect(prefs.x,prefs.y,300+prefs.x,100+prefs.y));
		ErrorView *sv = new ErrorView(m_window->Bounds(),
				"This device has no controls.");
		m_window->AddChild(sv);
	}
	m_window->Show();

	return;

err5:
	for (int32 i=0;i<m_num_nodes;i++)
		if (i) m_roster->StopNode(m_nodes[i].node, 0, true);
err4:
	for (int32 i=0;i<m_num_nodes-1;i++)
		m_roster->Disconnect(
				m_nodes[i].node.node,
				m_nodes[i].output.source,
				m_nodes[i+1].node.node,
				m_nodes[i+1].input.destination);
err3:
	for (int32 i=0;i<m_num_nodes;i++)
		m_roster->ReleaseNode(m_nodes[i].node);
err2:
	m_roster = NULL;
err1:
	return;
}

bool TVApp::QuitRequested()
{
	if (m_window && m_window->Lock())
		m_window->Quit();
	m_window = NULL;

	if (m_roster) {
		for (int32 i=0;i<m_num_nodes-1;i++)
			m_roster->Disconnect(
					m_nodes[i].node.node,
					m_nodes[i].output.source,
					m_nodes[i+1].node.node,
					m_nodes[i+1].input.destination);

		for (int32 i=0;i<m_num_nodes;i++) {
			/* don't stop producer node (bt848 doesn't like it) */
			if (i) m_roster->StopNode(m_nodes[i].node, 0, true);
			m_roster->ReleaseNode(m_nodes[i].node);
		}
	}

	return true;
}

static BRect offset_rect(BRect r, float x, float y)
{
	r.OffsetBy(x, y);
	return r;
}

ControlWindow::ControlWindow(const BRect & frame, BView * controls, media_node node) :
	BWindow(frame, "TV Controls",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	m_view = controls;
	m_node = node;
		
	AddChild(m_view);
}

ControlWindow::ControlWindow(const BRect & frame) :
	BWindow(frame, "TV Controls",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
}


ControlWindow::~ControlWindow()
{
	prefs.x = Frame().left;
	prefs.y = Frame().top;
	put_prefs();
}



void
ControlWindow::MessageReceived(BMessage * message) 
{
	BParameterWeb * web = NULL;
	status_t err;
	int32 count;
	
	switch (message->what) {
		case B_MEDIA_WEB_CHANGED: {
//			printf("Number children: %d\n", CountChildren());
			count = CountChildren();
//			for (int i = 0; i < count; i++)
//				printf("   Child view: %08x\n", ChildAt(i));

			// If this is a tab view, find out which tab 
			// is selected
			BTabView *tabView = dynamic_cast<BTabView*>(m_view);
			int32 tabNum = -1;
			if (tabView)
				tabNum = tabView->Selection();

//			printf("REMOVING VIEW \n");
			RemoveChild(m_view);
//			printf("DELETING VIEW \n");
			delete m_view;
//			printf("GETTING PARAMETER WEB\n");
			err = BMediaRoster::Roster()->GetParameterWebFor(m_node, &web);
			if (err >= B_OK && web != NULL) {
//				printf("GETTING NEW VIEW\n");
				m_view = BMediaTheme::ViewFor(web);
				AddChild(m_view);

				// Another tab view?  Restore previous selection
				if (tabNum > 0) {
					BTabView *newTabView = dynamic_cast<BTabView*>(m_view);	
					if (newTabView)
						newTabView->Select(tabNum);
				}
			}

			break;
		}

	default:
		BWindow::MessageReceived(message);
	}
}

bool
ControlWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

int
main()
{
	TVApp app;
	app.Run();
	return 0;
}
