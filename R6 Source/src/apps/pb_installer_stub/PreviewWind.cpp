#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include "PreviewWind.h"
#include "LabelView.h"

#include "FileTree.h"
#include "InstallerType.h"
#if (!SEA_INSTALLER)

#include "Util.h"

#include "FSIcons.h"
#include "MyDebug.h"



/*****************************************************/

PreviewWind::PreviewWind(FileTree *prevTree, bool *cancelInst)
	:	BWindow(BRect(0,0,300,380),"Preview Install",
					B_TITLED_WINDOW,
					0/*NULL*/,0/*NULL*/),
		cancel(cancelInst)
{
	Lock();
	*cancel = TRUE;
	PositionWindow(this,0.4,0.2);
		
	AddChild(new PreviewView(Bounds(),prevTree));

	AddShortcut('A',B_COMMAND_KEY,new BMessage(T_SELECT_ALL),prevTree);
	
	Unlock();
	// Show();
}

void PreviewWind::Go()
{
	long exitVal;
	
	Show();
	wait_for_thread(Thread(),&exitVal);
}


bool PreviewWind::QuitRequested()
{
	return TRUE;
}


/*****************************************************/
#pragma mark -----file view-----


PreviewView::PreviewView(BRect frame,FileTree *_treeView)
	:	BView(frame,"fileview",B_FOLLOW_ALL,B_WILL_DRAW),
		treeView(_treeView)
{
}

void PreviewView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(light_gray_background);
	
	BRect r = Bounds();
	r.InsetBy(8,8);
	
	r.left += 40;
	r.bottom = r.top + 40;
	
	AddChild(new LabelView(r,"The following items will be installed. \
Clicking the \"Continue\" button will install items in the indicated locations."));

	r.left -= 40;
	
	r.top = r.bottom + 12;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom = Bounds().bottom - 68;
	
	treeView->MoveTo(r.LeftTop());
	treeView->ResizeTo(r.Width(),r.Height());
	
	BScrollView *sv = new BScrollView("scroller",treeView,B_FOLLOW_ALL,
				0/*NULL*/, TRUE, TRUE);
	AddChild(sv);
	// Let the TreeView know about the scrollbars
	// do this automatically
	treeView->SetScrollBars(sv->ScrollBar(B_HORIZONTAL), sv->ScrollBar(B_VERTICAL));
	treeView->MakeFocus(TRUE);
	
					
	r.bottom += B_H_SCROLL_BAR_HEIGHT;
	r.right += B_V_SCROLL_BAR_WIDTH;
	
	r.top = r.bottom + 16;
	
	r.right -= 6;
	r.bottom = r.top + 20;
	r.left = r.right - 80;
	
	BButton *btn;
	btn = new BButton(r,"continuebtn","Continue",new BMessage((ulong)B_QUIT_REQUESTED),
				B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	btn->MakeDefault(TRUE);
	btn->SetTarget(this);

	//btn->SetFontName("Erich");
	
	r.right = r.left - 36;
	r.left = r.right - 80;
	btn = new BButton(r,"cancelbtn","Cancel",new BMessage((ulong)B_QUIT_REQUESTED),
				B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	//btn->SetFontName("Erich");
	
	Window()->SetSizeLimits(Window()->Frame().Width(),8192,
						Window()->Frame().Height()/2.0,8192);
}

void PreviewView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_QUIT_REQUESTED: {
			// the continue button was pressed
			*(((PreviewWind *)Window())->cancel) = FALSE;
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void PreviewView::Draw(BRect up)
{
	BView::Draw(up);
	
	SetDrawingMode(B_OP_OVER);
	DrawBitmapAsync(gYellowWarnIcon,BPoint(8,8));
	SetDrawingMode(B_OP_COPY);
}


#endif // !SEA_INSTALLER
