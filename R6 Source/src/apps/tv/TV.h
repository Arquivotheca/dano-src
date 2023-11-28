
#if !defined(TV_H)
#define TV_H

#include <Application.h>
#include <InterfaceKit.h>
#include <MediaDefs.h>
#include <MediaNode.h>

class BMediaRoster;
class ControlWindow;

class TVApp : public BApplication {

public:

		TVApp();

virtual	bool QuitRequested();

private:

		BMediaRoster * m_roster;
		ControlWindow * m_window;

		media_node timesourceNode;

		int32			m_num_nodes;
		struct {
			media_node		node;
			media_output	output;
			media_input		input;
		}				m_nodes[3];
};

#include <Window.h>

class ControlWindow : public BWindow {

public:

		ControlWindow(const BRect & frame, BView * controls, media_node node);
		ControlWindow(const BRect & r);
		~ControlWindow();
		void MessageReceived(BMessage * message);
virtual	bool QuitRequested();

private:

		BView * m_view;
		media_node m_node;

};

#endif /* TV_H */

