#pragma once
#include "ProfilerApp.h"


#include "MWAbout.h"
#define Sort_By_Method 'METH'
#define Sort_By_Call 'CALL'
#define Sort_By_Usec 'USEC'
#define Sort_By_User 'USER'
#define Sort_By_Kernel 'KERN'
#define Sort_By_Clock 'CLOK'
#define B_SELECT_REQUESTED 'SELE'
#define B_OPEN_REQUESTED 'OPN'
#define B_CLOSE_WINDOW	'CLW'
#define B_QUIT_DAMNIT	'QUIT'
#define B_ABOUT_REQUESTED 'ABTP'

class MWWindow : public BWindow
{
	public:
						MWWindow();
						~MWWindow();
		virtual bool	QuitRequested();
		virtual void	MessageReceived(BMessage* message);
		void 			ReadFile(char * filename); 
		void			SetUpMenuBar(void);
	private:
		BView		*MainView;
		
		BRect 		depthStringRect, createdStringRect, scrollViewBounds;
		BStringView *depthString, *createdString;
		BButton		*methodButton,*calledButton,* usecButton,*userButton,
					*kernelButton,*clockButton;
		BListView 	*ProfListView;
		BScrollView	*ListScrollView;
		BMenu		*FileMenu,*EditMenu, *SortMenu;
		BMenuBar	*MenuBar;
		BMenuItem	*NameFuncMenuItem, *CallsFunMenuItem, *UsecFunMenuItem,
					*UserFunMenuItem, *KernelFunMenuItem, *WallclockFunMenuItem;
		
};