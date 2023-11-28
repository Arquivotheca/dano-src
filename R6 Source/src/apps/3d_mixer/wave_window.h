#ifndef WAVE_WINDOW_H
#define WAVE_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif
#include "channel.h"
#include "sound_view.h"
#include "DirFilePanel.h"
#include "track_view.h"
#include "status_view.h"
#include <MediaKit.h>

#ifndef	WAVE_WINDOW
#define	WAVE_WINDOW

class	TrackView;
class	xBView;
class	BMenu;

class TWaveWindow : public BWindow {

public:
				TWaveWindow(BRect, const char*);
				~TWaveWindow();
		void	Switch();
		void	Switch_H();
virtual	void	MessageReceived(BMessage *b);
		void	UpdateMenus();
		void	ChooseDestination(void);
		void	TakeRef(entry_ref ref);
		void	SaveAs();
		void	MixMaster();
		void	DoSave(entry_ref *dir, const char	*name);
		void	handle_special_char(char c);

		BView			*track_view;
		BView			*mix_view;	
		xBView			*master_view;	
		TCtrlView		*ctrl_view;
		TrackView		*tmain_view;
		TimeView		*time_view;
		int				mode;
		int				ctr;
		BFilePanel		*mOpenPanel;
		BFilePanel		*mSavePanel;
		BEntry			*mDirEntry;
		BMenuItem		*t_i[3];
		BMenuItem		*h_c[4];					//has channels
		BMenuItem		*os[4];
		BMenuItem		*h_cl[3];					//has clip
		BMenuItem		*fast_item;
		BMenuItem		*undo_item;
		BMenuItem		*pause_item;
		BMenu			*channel_menu;
		TDirFilePanel	*fDestFilePanel;
		entry_ref		fDestRef;
		TDirFilter		fDirFilter;
		TSoundView		*sound_view; 
		StatusView		*sv;
		char			wave_save;
		char			is_paused;
};

#endif

#endif
