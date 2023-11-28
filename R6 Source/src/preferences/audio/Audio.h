
#if !defined(AUDIO_H)
#define AUDIO_H

#include <Window.h>
#include <MediaNode.h>

class BScrollBar;
class BParameterWeb;
class BMediaRoster;
class BScrollView;
class BMenuControl;

class AudioWindow : public BWindow {
public:
		AudioWindow(
				const BRect area,
				const char * name);
		~AudioWindow();

virtual		void MessageReceived(
				BMessage * message);
virtual		void FrameResized(float new_width, float new_height);
virtual		void FrameMoved(BPoint position);
virtual		bool QuitRequested();
			void SetInputView(bool addTab);
			void SetOutputView(bool addTab);	
			void SetMixerView(bool addTab);	
		void RestartServer();
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

		BView * m_mixer_view;
		media_node m_mixer_node;
		BParameterWeb *m_mixer_web;
		BRect m_mixer_rect;

		BMenuControl * m_input_list;
		BMenuControl * m_output_list;

		void UpdateScrollBars();

};

#endif
