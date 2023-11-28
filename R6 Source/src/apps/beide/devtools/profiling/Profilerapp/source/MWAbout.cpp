#include "MWAbout.h"

const BRect kAboutFrame(120,120, 361,361);
const rgb_color Shadow =	{194,194,194,255};
const rgb_color hite ={255,255,255,		255};

/* Sets up the about box, i.e. background image and text */
MWAbout::MWAbout():
	BWindow(kAboutFrame, "About Profiler", B_TITLED_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{

	ImageView=new MWAboutView(Bounds(), "AboutView", B_FOLLOW_NONE, B_WILL_DRAW);
	//ImageView->SetViewColor(Black);
	AddChild(ImageView);
	cout <<"Added the BView\n";
	BStringView *AutherStr=new BStringView(BRect(10,90,140,105), "AboutText", "Author:    Zeid Derhally",B_FOLLOW_NONE,B_WILL_DRAW);
   	BStringView *ThankStr=new BStringView(BRect(10,115,140,130),"ThankText","Special Thanks To:",B_FOLLOW_NONE,B_WILL_DRAW);
	BStringView *BrianStr=new BStringView(BRect(15,140,155,155),"Brian","Brian Stern",B_FOLLOW_NONE,B_WILL_DRAW);
	BStringView *BurtonStr=new BStringView(BRect(15,158,155,170),"Burton","Burton Miller",B_FOLLOW_NONE,B_WILL_DRAW);
	BStringView *BernieStr=new BStringView(BRect(15,173,155,185),"Bernie","Bernie Estavillo",B_FOLLOW_NONE,B_WILL_DRAW);
	BStringView *RichardStr=new BStringView(BRect(15,188,155,201),"Richard","Richard Atwell",B_FOLLOW_NONE,B_WILL_DRAW);
	BStringView *AryStr=new BStringView(BRect(15,204,155,216),"Ary","Ary Djajadi",B_FOLLOW_NONE,B_WILL_DRAW);
	
	AutherStr->SetFont(be_bold_font);
	ThankStr->SetFont(be_bold_font);
	BrianStr->SetFont(be_bold_font);
	BurtonStr->SetFont(be_bold_font);
	BernieStr->SetFont(be_bold_font);
	RichardStr->SetFont(be_bold_font);
	AryStr->SetFont(be_bold_font);

	AutherStr->SetFontSize(11);
	ThankStr->SetFontSize(11);
	BrianStr->SetFontSize(11);
	BurtonStr->SetFontSize(11);
	BernieStr->SetFontSize(11);
	RichardStr->SetFontSize(11);
	AryStr->SetFontSize(11);

	AutherStr->SetDrawingMode(B_OP_OVER);
	ThankStr->SetDrawingMode(B_OP_OVER);
	BrianStr->SetDrawingMode(B_OP_OVER);
	BurtonStr->SetDrawingMode(B_OP_OVER);
	BernieStr->SetDrawingMode(B_OP_OVER);
	RichardStr->SetDrawingMode(B_OP_OVER);
	AryStr->SetDrawingMode(B_OP_OVER);

	AutherStr->SetHighColor(hite);
	ThankStr->SetHighColor(hite);
	BrianStr->SetHighColor(hite);
	BurtonStr->SetHighColor(hite);
	BernieStr->SetHighColor(hite);
	RichardStr->SetHighColor(hite);
	AryStr->SetHighColor(hite);
	
  // MyString->SetViewColor(Shadow);
	AutherStr->SetLowColor(B_TRANSPARENT_32_BIT);
    ThankStr->SetLowColor(B_TRANSPARENT_32_BIT);
    BrianStr->SetLowColor(B_TRANSPARENT_32_BIT);
    BernieStr->SetLowColor(B_TRANSPARENT_32_BIT);
    BurtonStr->SetLowColor(B_TRANSPARENT_32_BIT);
    RichardStr->SetLowColor(B_TRANSPARENT_32_BIT);
    AryStr->SetLowColor(B_TRANSPARENT_32_BIT);


    AddChild(AutherStr); 
	AddChild(ThankStr);
	AddChild(BrianStr);
    AddChild(BurtonStr); 
    AddChild(BernieStr); 
    AddChild(RichardStr);
    AddChild(AryStr); 
}

MWAbout::~MWAbout()
{
	AboutBox=false;
}

bool
MWAbout::QuitRequested()
{
	int i=be_app->CountWindows();
	if (i==1)
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		this->Hide();
		this->Quit();		
	}
	else
	{	
		this->Hide();
		this->Quit();		
	}
}