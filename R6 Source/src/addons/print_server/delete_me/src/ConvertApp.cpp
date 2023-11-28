//******************************************************************************
//
//	File:		ConvertApp.h
//
//	Description:	ConvertApp object for PostScript driver
//
//	Copyright 1996, International Lorienne Inc.
//
//	11/23/96 : Test purpose. pre prototype
//
//			
//		Marc Verstaen
//
//******************************************************************************

#include "ConvertApp.h"
#include "Convert.h"

//==============================================================================
//
//	Constructor for ConvertApp
//
//
//==============================================================================

ConvertApp::ConvertApp(ulong signature) : BApplication (signature)
{
}

//==============================================================================
//
//	Destructor.
//
//			Do nothing
//
//==============================================================================

ConvertApp::~ConvertApp()
{
}

//==============================================================================
//
//	ReadyToRun.
//
//			Do nothing
//
//==============================================================================

void ConvertApp::ReadyToRun()
{
//	if (currentConvert == NULL) currentConvert = new Convert();
}

//==============================================================================
//
//	RefsReceived (
//					BMessage *a_message
//				)
//		
//		
//==============================================================================

void ConvertApp::RefsReceived(BMessage *message)
{
	ulong	type;
	long	count;

	message->GetInfo("refs",&type,&count);
	
	for (long i = -- count; i >= 0 ; i--) {
		record_ref item = message->FindRef("refs",i);
		if (does_ref_conform(item,"File")) {
			if ((currentConvert != NULL) && (!currentConvert->HasRef())) {
				currentConvert->SetRef(item);
			}
			else currentConvert = new Convert(item);
		}
	}
}

bool ConvertApp::IsPortrait()
{
	bool ret;

	portrait->Window()->Lock();
	if (portrait->Value()) ret = true;
	else ret = false;
	portrait->Window()->Unlock();
	return ret;
}

float ConvertApp::Scale()
{
	float scaleValue;


	scale->Window()->Lock();
	const char *string = scale->Text();
	sscanf(string,"%f",&scaleValue);
	scale->Window()->Unlock();

	if ((scaleValue < 0.00001) || (scaleValue > 1000)) scaleValue = 1.;
	return scaleValue;
}

BPoint ConvertApp::Origin()
{
	float x,y;
	xOrig->Window()->Lock();
	const char *string = xOrig->Text();
	sscanf(string,"%f",&x);
	string = yOrig->Text();
	xOrig->Window()->Unlock();
	sscanf(string,"%f",&y);
	BPoint p(x,y);
	
	return p;
}


