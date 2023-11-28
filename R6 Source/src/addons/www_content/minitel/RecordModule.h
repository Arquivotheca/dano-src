#ifndef RECORD_MOD
#define RECORD_MOD

#include <Window.h>


class	BMenuItem;
class	BCheckBox;
class	BTextControl;
class	Protocole;
class	MinitelView;


#define kMAX_PAGE	32765
#define kPAS_ALLOC	32765
#define kSTOP			0
#define kPLAY			1
#define kENREG			2


//========================================================================

class  RecordModule : public BWindow
{
	public:
							RecordModule		(BRect frame,
												 const char* title,
												 window_type type,
												 ulong flags,
												 bool free_when_closed=TRUE);
		void				ReInit				();
		void				ChangePageStop		();
		void				ChangeAutoDraw		();
		void				ChangeVitesse		();
		void				Enreg				();
		void				DoEnreg				(unsigned char c);
		void				StopEnreg			();
		void				StopPlay			();
		void				Pulse				();
		void				DoReplay			();
		void				Play				();
		void				Pause				();
		void				GoTo				(int32 num);
		void				SkipBwd				();
		void				SkipGo				();
		void				SkipFwd				();
		void				Open				();
		void				DoOpen				(entry_ref *item);
		void				SetBinary			();
		void				SetAscii			();
		void				SetScopeAll			();
		void				SetScopePart		();
		void				EnterPage			();
		void				DoSave				();
		void				DoSaveAscii			();
		void				Save				();
		void				SaveUnder			();
		void				SaveRequested		(entry_ref directory,
												 const char* filename);
		void				Close				();

	    BTextControl*		item_page;
	    Protocole*			protocole;
	    BWindow*			loritel_win;
	    BWindow*			compose_win;
	    MinitelView*		ecran;
	    BCheckBox*			stop_item;
		BCheckBox*			autoDraw_item;
		//LRObject*			savepanel;
		//LRObject*			saveView;
		//LRObject*			beg_item;
		//LRObject*			end_item;
		//LRObject*			scope_item;
		//LRObject*			a_item;
		BMenuItem*			asciiSaveMI;
		BMenuItem*			binSaveMI;

	protected:
		char				fic_save[255];
		int16				mode;
		int16			 	modif_flag;
		int16				page_stop;
		int32				affiche_time_entry;
		int32				autoDraw;
		int32				cur_data;
		int32				cur_page;
		int32				max_alloc;
		int32				max_page;
		int32				nbr_data;
		int32				nv;
		int32				page[kMAX_PAGE];
		int32				saveBeg;
		int32				saveEnd;
		int32				saveMode;
		int32				savePart;
		int32				vit_nbr;
		unsigned char*		buffer;
		entry_ref			dir;
};
#endif

