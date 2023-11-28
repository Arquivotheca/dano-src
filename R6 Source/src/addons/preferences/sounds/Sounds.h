#if ! defined SOUNDS_INCLUDED
#define SOUNDS_INCLUDED 1

#include <PlaySound.h>
#include <View.h>

class PPAddOn;

class SoundsView : public BView
{
public:
	
				SoundsView(PPAddOn *adn);
	virtual		~SoundsView();
	virtual		void MessageReceived(BMessage * message);
	void		AttachedToWindow();

private:

	PPAddOn			*addon;
	thread_id 		m_thread;
	BMediaFiles 	*m_media;
	BListView 		*eventList;
	BMenu 			*soundFileMenu;
	BPictureButton 	*m_play;
	BPictureButton 	*m_stop;
	BButton 		*m_other;
	BFilePanel 		*m_file_panel;
	sound_handle	m_sound_sem;
	bool 			m_sound_started;
	bool			m_selected_is_valid;
	entry_ref 		currentSelectedFile;
	BMenuField		*menuField;

	BButton			*stopButton;
	BButton			*playButton;

	static	status_t file_getter(void *);

	void ReadingDone(BMessage * message);
	void OtherPressed(BMessage * message);
	void PlayPressed();
	void StopPressed();
	void EventSelected();
	void SoundFileSelected(BMessage *message);
	BMenuItem *SelectSoundFile(const entry_ref & ref);
};

#endif
