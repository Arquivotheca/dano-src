#ifndef _CHILDWINDOW_H
#define _CHILDWINDOW_H


enum {
	M_DO_ACTIVATE =		'DoAc',
	M_QUIT_FULFILLED =	'QFul',
	M_NEW_TITLE =		'NTil',
	M_INFO_MODIFIED =	'InMd'
};


class ChildWindow : public BWindow
{
public:
				ChildWindow(BRect frame,
							const char *title,
							window_type type,
							ulong flags, BWindow* parentWindow,
							ulong workspace = B_CURRENT_WORKSPACE);
		// frees parent title string
	
virtual			~ChildWindow();
		
		// adds in parent title
virtual void	SetTitle(const char *newTitle);
		// sends a reply so the parent knows when its done
virtual bool	QuitRequested();
	
		// receives messages for activate, parent title
virtual void 	DispatchMessage(BMessage *,BHandler *);
	
inline	bool	Dirty();					
virtual void	SetDirty(bool state = TRUE);

protected:
		bool			fWindowDirty;
		void			UpdateParTitle(const char *);
		char			*parentTitle;
		char			*realTitle;
		BWindow			*parentWin;
};


inline bool ChildWindow::Dirty()
{
	return fWindowDirty;
}
#endif
