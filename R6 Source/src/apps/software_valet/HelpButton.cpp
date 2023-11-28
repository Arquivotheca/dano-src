// HelpButton.cpp
#include <AppFileInfo.h>
#include <Application.h>
#include <Resources.h>
#include <Roster.h>
#include <Bitmap.h>
#include "HelpButton.h"
#include "HelpWindow.h"

#include "Util.h"
#include "MyDebug.h"

BPicture	*HelpButton::HelpButtonPicture = NULL;
BPicture	*HelpButton::HelpButtonOnPicture = NULL;
bool		HelpButton::picsInited = FALSE;

HelpButton::HelpButton( BRect frame,
						ulong resizeMask,
						const char *_helpTitle,
						const char *_helpText)
	/**					
	:	BButton(frame,"helpbutton","?",
				new BMessage(M_HELP_REQUESTED),
				resizeMask),
	**/
	
	: 	BPictureButton(frame,"helpbutton",
				HelpButtonPicture,
				HelpButtonOnPicture,
				new BMessage(M_HELP_REQUESTED),
				B_ONE_STATE_BUTTON,
				resizeMask,
				B_WILL_DRAW),
				
		helpTitle(_helpTitle),
		helpText(_helpText)
{
}

HelpButton::~HelpButton()
{
	helpWindowMessenger.SendMessage(B_QUIT_REQUESTED);
}

void HelpButton::AttachedToWindow()
{
	BPictureButton::AttachedToWindow();	
	SetTarget(this);
}

void HelpButton::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_HELP_REQUESTED: {
			if (helpWindowMessenger.IsValid()) {
				helpWindowMessenger.SendMessage(M_DO_ACTIVATE);
			}
			else {
				BPoint loc = Bounds().RightBottom();
				ConvertToScreen(&loc);
				BWindow *hw = new HelpWindow(helpTitle,
											 loc,
											 helpText);
				helpWindowMessenger = BMessenger(hw);
			}
			break;
		}
		default:
			BPictureButton::MessageReceived(msg);
			break;
	}
}

void HelpButton::SetHelpText(const char *text)
{
	helpText = text;
	BMessage sTxt(M_SET_HTEXT);
	sTxt.AddData("helptext",B_POINTER_TYPE,&text,sizeof(text));
	helpWindowMessenger.SendMessage(&sTxt);
}

void HelpButton::Show()
{
	if (IsHidden())
		BPictureButton::Show();
}

void HelpButton::Hide()
{
	if (!IsHidden()) {
		BPictureButton::Hide();
		helpWindowMessenger.SendMessage(B_QUIT_REQUESTED);
	}
}

void HelpButton::InitPicture(BView *context)
{
	if (picsInited) {
		PRINT(("\thelp pics inited\n"));
		if (!HelpButtonPicture)
			PRINT(("picture is null!!!\n"));
		return;
	}	
	PRINT(("\thelp button init picture\n"));

	app_info info;
	be_app->GetAppInfo(&info);
	BFile		 appFile(&info.ref,O_RDONLY);
	BResources appRes(&appFile);

	uint32 dataSize;	// size_t dataSize
	//appFile.Open(B_READ_ONLY);
	void *data = appRes.FindResource('BMAP',"question",&dataSize);
	//appFile.Close();
	if (!data)
		data = malloc(16*16);

	BBitmap *bmap = new BBitmap(BRect(0,0,15,15),B_RGB_32_BIT);

	if (data) {
		bmap->SetBits(data, dataSize, 0,B_RGB_32_BIT);
		free(data);
	}
	BRect r(0,0,17,17);
	
	rgb_color saveColor = context->HighColor();
	
	context->BeginPicture(new BPicture);
		context->SetPenSize(1.0);
		context->StrokeRect(r);
		r.InsetBy(1,1);
		context->SetHighColor(128,128,128);
		context->FillRect(r);
		context->SetDrawingMode(B_OP_OVER);
		r.InsetBy(1,1);
		context->DrawBitmap(bmap,r);
		
	HelpButtonPicture = context->EndPicture();
	
	r = BRect(0,0,16,16);
	context->BeginPicture(new BPicture);
		context->DrawPicture(HelpButtonPicture);
		context->InvertRect(r);
	HelpButtonOnPicture = context->EndPicture();
	
	context->SetHighColor(saveColor);
	context->SetDrawingMode(B_OP_COPY);
	delete bmap;
	
	picsInited = TRUE;
}

// could have race conditions here
void HelpButton::Free()
{
	if (!picsInited)
		return;
	
	PRINT(("\thelp button free\n"));
	
	if (HelpButtonOnPicture) {
		delete HelpButtonOnPicture;
		HelpButtonOnPicture = NULL;	
	}
	if (HelpButtonPicture) {
		delete HelpButtonPicture;
		HelpButtonPicture = NULL;	
	}
	picsInited = FALSE;
}
