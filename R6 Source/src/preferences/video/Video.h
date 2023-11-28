
#if !defined(AUDIO_H)
#define AUDIO_H

#include <Window.h>
#include <MediaNode.h>

class BScrollBar;
class BParameterWeb;
class BMediaRoster;
class BScrollView;
class BMenuControl;

class VideoWindow : public BWindow {
public:
					VideoWindow(const BRect area, const char * name);
					~VideoWindow();

	virtual	void 	MessageReceived(BMessage * message);
	virtual	bool 	QuitRequested();
	void 			SetInputView(bool addTab);
	void 			SetOutputView(bool addTab);	
	void			ReplaceTab(int tab, BView *newView);
	void			RestartServer();

	void			FrameMoved(BPoint location);
	void			FrameResized(float width, float height);

private:

		BView * m_bg;
		BMediaRoster * m_roster;
		BTabView * m_tabs;
		media_node m_output_node;
		BParameterWeb * m_output_web;
		BView * m_output_view;
		media_node m_input_node;
		BParameterWeb * m_input_web;
		BView * m_input_view;
		BView * m_hardware_view;
		BMenuControl * m_input_list;
		BMenuControl * m_output_list;

		void UpdateScrollbars();

};

#endif
