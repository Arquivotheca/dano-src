#include <Be.h>
// GroupsView.cpp

#include "GroupsView.h"
#include "GroupsListView.h"
#include "PackMessages.h"

#include "TCheckBox.h"
#include "STextField.h"

#include "Util.h"
#include "MyDebug.h"

enum {
	M_INFO_MODIFIED =	'InMd'
};

GroupsView::GroupsView(BRect frame,GroupList *theList)
	: BView(frame,"groupsview",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
	fList = theList;
}

void GroupsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	////////////////////////////////////////////
	BRect r = Bounds();
	r.InsetBy(10,10);
	r.bottom = r.top + 110;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	GroupsListView *glist = new GroupsListView(r,fList);
		
	r.right += B_V_SCROLL_BAR_WIDTH;
	BScrollView *scrollV =
		new BScrollView("scroller",glist,B_FOLLOW_ALL,0,FALSE,TRUE);

	AddChild(scrollV);

	glist->SetFont(be_plain_font);
	////////////////////////////////////////////

	r.top = r.bottom + 20;
	r.bottom = r.top + 20;
	
	BMessage *invMessage = new BMessage(M_NAME_GROUP);
	invMessage->AddInt32("item",-1);
	// invMessage->AddObject("object",NULL);
	BTextControl *nControl =
		new BTextControl(r,"groupname","Group Name",B_EMPTY_STRING,
		invMessage,B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	nControl->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	
	BView *li = FindView("grouplisting");
	li->MakeFocus();
	nControl->SetTarget(li);
	nControl->SetDivider(80);
	AddChild(nControl);
	
	r.top = r.bottom + 10;
	r.bottom = r.top + 90;
	
	invMessage = new BMessage(M_SET_DESCRIPTION);
	invMessage->AddInt32("item",-1);
	STextField *desc = new STextField(r,"description","Description",B_EMPTY_STRING,
						invMessage,
						B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
						B_WILL_DRAW | B_NAVIGABLE,
						FALSE);
	desc->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	desc->SetTarget(glist);
	AddChild(desc);
	
	/////////////////// groups help checkbox ///////////////////
	
	r.top = r.bottom + 10;
	r.bottom = r.top + 14;
	
	TCheckBox *doGrpHelp = new TCheckBox(r,"dogrphelp","Display Groups Help",
								new BMessage(M_DO_GROUPSHELP),
								B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	doGrpHelp->SetTarget(glist);
	AddChild(doGrpHelp);
	
	/////////////////// groups help text field ///////////////////
	
	r.top = r.bottom + 6;
	r.left += 18;
	r.bottom = r.top + 90;
	
	STextField *grpsHelp = new STextField(r,"grpshelp","Help Text:",B_EMPTY_STRING,
					new BMessage(M_DO_GROUPSHELP),
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
					B_WILL_DRAW | B_NAVIGABLE,
					TRUE);
	grpsHelp->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	grpsHelp->SetTarget(glist);
	AddChild(grpsHelp);

	doGrpHelp->AddSlave(grpsHelp);
	grpsHelp->SetEnabled(FALSE);
}


GroupsView::~GroupsView()
{

}
