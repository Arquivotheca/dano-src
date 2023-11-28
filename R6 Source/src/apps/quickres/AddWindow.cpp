#include "AddWindow.h"

#include <Button.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <Autolock.h>
#include <Debug.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

enum {
	kAdd = 'add ',
	kSelection = 'sele'
};

static float GetMenuFieldSize(BMenuField* field, float* width, float* height,
							  bool set_divider)
{
	BMenu* popup = field->Menu();
	
	font_height fhs;
	field->GetFontHeight(&fhs);
	const float fh = fhs.ascent+fhs.descent+fhs.leading;
	float fw = field->StringWidth("WWWW");
	
	float pref_w=0;
	if( popup ) {
		int32 num = popup->CountItems();
		for( int32 i=0; i<num; i++ ) {
			BMenuItem* item = popup->ItemAt(i);
			if( item ) {
				const float w=field->StringWidth(item->Label());
				if( w > pref_w ) pref_w = w;
			}
		}
	}
	
	float lw = (field->Label() && *field->Label())
		? field->StringWidth(field->Label()) + field->StringWidth(" ") + 5
		: 0;
	if( set_divider ) field->SetDivider(lw);
	*width = floor((fw>pref_w?fw:pref_w) + 20 + lw + .5);
	*height = floor(fh + 8 + .5);
	return lw;
}

// ---------------------------- AddWindow ----------------------------

AddWindow::AddWindow(BList* items, add_types types, BMessenger target,
			   BPoint center, const char* doc_name,
			   window_look look, window_feel feel,
			   uint32 flags, uint32 workspace)
	: BWindow(BRect(center.x-100, center.y-100, center.x+100, center.y+100),
			  doc_name, look, feel, flags, workspace),
	  fDocName(doc_name), fTypes(types), fTarget(target),
	  fList(0), fID(0), fType(0), fAddButton(0), fCloseButton(0)
{
	BString title(fDocName);
	title << ": New Resources";
	SetTitle(title.String());
	
	BRect frame(Bounds());
	
	// Add root view
	BView* root = new BView(frame, "root", B_FOLLOW_ALL, 0);
	root->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(root);

	fCloseButton = new BButton(frame, "close", "Close", new BMessage(B_QUIT_REQUESTED),
							   B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	root->AddChild(fCloseButton);
	fAddButton = new BButton(frame, "add", "Add", new BMessage(kAdd),
							 B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	root->AddChild(fAddButton);
	
	fID = new BTextControl(frame, "id", "ID: ", "1", 0,
						   B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	root->AddChild(fID);
	
	BPopUpMenu* popup = new BPopUpMenu("type");
	BMenuItem* addattr = new BMenuItem("Attribute", new BMessage(kSelection));
	popup->AddItem(addattr);
	BMenuItem* addres = new BMenuItem("Resource", new BMessage(kSelection));
	popup->AddItem(addres);
	switch( fTypes ) {
		case ADD_ATTRIBUTES_ONLY:
			addattr->SetMarked(true);
			addres->SetEnabled(false);
			break;
		case ADD_RESOURCES_ONLY:
			addres->SetMarked(true);
			addattr->SetEnabled(false);
			break;
		default:
			addres->SetMarked(true);
	}
	
	fName = new BTextControl(frame, "name", "Name: ", "", 0,
						   B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	root->AddChild(fName);
	
	fType = new BMenuField(frame, "type", "Type: ", popup,
						   B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	root->AddChild(fType);
	
	const int SPACER = 6;
	
	float pw, ph;
	float butw, buth;
	float nmw, nmh;
	float typw, typh;
	
	butw = buth = 0;
	fAddButton->GetPreferredSize(&butw, &buth);
	fCloseButton->GetPreferredSize(&pw, &ph);
	if( ph > buth ) buth = ph;
	butw = (pw > butw ? pw : butw) * 2;
	
	fName->ResizeToPreferred();
	fName->GetPreferredSize(&nmw, &nmh);
	
	fID->ResizeToPreferred();
	fID->GetPreferredSize(&typw, &typh);
	GetMenuFieldSize(fType, &pw, &ph, true);
	if( ph > typh ) typh = ph;
	typw += pw + fID->StringWidth("0000");
	
	font_height fh;
	root->GetFontHeight(&fh);
	
	frame.right = (butw > typw ? butw : typw) + SPACER * 3;
	frame.bottom = SPACER + (fh.ascent+fh.descent+fh.leading)*20 + SPACER
				 + typh + SPACER + nmh + SPACER + buth + SPACER;
	
	ResizeTo(frame.Width(), frame.Height());
	root->ResizeTo(frame.Width(), frame.Height());
	
	frame.bottom -= SPACER + buth;
	
	fCloseButton->MoveTo(SPACER, frame.bottom);
	fCloseButton->ResizeTo(floor(butw/2), buth);
	fAddButton->MoveTo(frame.right - SPACER - floor(butw/2), frame.bottom);
	fAddButton->ResizeTo(floor(butw/2), buth);
	
	frame.bottom -= SPACER + nmh;
	
	fName->MoveTo(SPACER, frame.bottom);
	fName->ResizeTo(typw, nmh);
	
	frame.bottom -= SPACER + typh;
	
	fType->MoveTo(frame.right - SPACER - pw, frame.bottom);
	fType->ResizeTo(pw, typh);
	fID->MoveTo(SPACER, frame.bottom);
	fID->ResizeTo(typw - pw, typh);
	
	frame.top = frame.left = SPACER;
	frame.right -= SPACER;
	frame.bottom -= SPACER;
	
	BRect listframe(frame);
	listframe.right -= B_V_SCROLL_BAR_WIDTH + 4;
	listframe.bottom -= 4;
	
	fList = new BListView(listframe, "res_list");
	fList->AddList(items);
	fList->SetInvocationMessage(new BMessage(kAdd));
	fList->SetSelectionMessage(new BMessage(kSelection));
	fList->SetResizingMode(B_FOLLOW_ALL);
	BScrollView* scroller = new BScrollView("res_scroll", fList, B_FOLLOW_ALL,
											0, false, true, B_FANCY_BORDER);
	root->AddChild(scroller);
	
	BRect bnd(Bounds());
	float x = center.x - (bnd.Width()/2);
	float y = center.y - (bnd.Height()/2);
	BRect frm(BScreen(this).Frame());
	if( (x+bnd.Width()) > frm.right ) x = frm.right-bnd.Width();
	if( (y+bnd.Height()) > frm.bottom ) y = frm.bottom-bnd.Height();
	if( x < frm.left ) x = frm.left;
	if( y-20 < frm.top ) y = frm.top+20;
	
	MoveTo(x, y);
	
	UpdateControls();
	
	Lock();
	AddCommonFilter(this);
	Unlock();
}

bool AddWindow::QuitRequested()
{
	return true;
}

void AddWindow::Quit()
{
	RemoveCommonFilter(this);
	BWindow::Quit();
}

void AddWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case ADDWIN_ACTIVATE:
		{
			Activate();
		} break;
		
		case kAdd: {
			int32 idx;
			for( int32 i=0; (idx=fList->CurrentSelection(i)) >= 0; i++ ) {
				BGenerateItem* it = dynamic_cast<BGenerateItem*>(fList->ItemAt(idx));
				if( it && it->Info() ) {
					BMessage msg(*(it->Info()));
					
					BMenuItem* type = (fType && fType->Menu())
									? fType->Menu()->FindMarked()
									: 0;
					bool attr = false;
					if( type && fType->Menu()->IndexOf(type) == 0 ) attr = true;
					if( attr ) msg.AddBool("be:attribute", true);
					else if( fID ) {
						msg.AddInt32("be:id", atol(fID->Text()));
					}
					if( fName ) {
						msg.AddString("be:item_name", fName->Text());
					}
					fTarget.SendMessage(&msg);
				}
			}
		} break;
		
		case kSelection: {
			UpdateControls();
		} break;
		
		default :
			BWindow::MessageReceived(msg);
			break;
	}
}

void AddWindow::UpdateControls()
{
	BMenuItem* type = (fType && fType->Menu())
					? fType->Menu()->FindMarked()
					: 0;
	bool attr = false;
	if( type && fType->Menu()->IndexOf(type) == 0 ) attr = true;
	if( fID ) fID->SetEnabled(!attr);
	bool sel = fList ? (fList->CurrentSelection() >= 0) : false;
	if( fAddButton ) fAddButton->SetEnabled(sel);
}
