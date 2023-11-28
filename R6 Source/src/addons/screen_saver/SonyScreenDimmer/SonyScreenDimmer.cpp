#include "NetronDisplay.h"
#include "SonyScreenDimmer.h"
#include <StringView.h>
#include <stdio.h>

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id img)
{
	return new SonyScreenDimmer(msg, img);
}

//---------- SonyScreenDimmer -------------

// power states (see section 2.3.12 of the ECS spec)
const int8 kPowerStateOn		= 0x02;
const int8 kPowerStateOff		= 0x03;
const int8 kPowerStateStandby	= 0x06;

SonyScreenDimmer::SonyScreenDimmer(BMessage *message, image_id image)
	: BScreenSaver(message, image),
	  fNetronDisplay(NULL),
	  fPowerOnScreen(false)
{
}

SonyScreenDimmer::~SonyScreenDimmer()
{
	delete fNetronDisplay;
}

void SonyScreenDimmer::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING,
		"eVilla Screen Blanker"));
}

status_t SonyScreenDimmer::StartSaver(BView *v, bool preview)
{
	fPowerOnScreen = false;
	SetTickSize(10000000LL); // 10 sec.
	v->SetLowColor(0, 0, 0);

	if (preview) {
		return B_OK;
	}

	fNetronDisplay = new NetronDisplay();
	if (fNetronDisplay->InitCheck() != B_OK) {
		fprintf(stderr, "Error creating NetronDisplay!\n");		
		delete fNetronDisplay;
		fNetronDisplay = NULL;
		return B_ERROR;
	}

	// set the monitor power state to "standby" if it is "on"
	NetronDisplay::power_state curr_state = NetronDisplay::POWER_STANDBY;
	if ((fNetronDisplay->GetPowerState(&curr_state) == B_OK) &&
		(curr_state == NetronDisplay::POWER_ON))
	{
		fNetronDisplay->SetIgnorePowerKey(true);
		fNetronDisplay->SetPowerState(NetronDisplay::POWER_STANDBY);
		fPowerOnScreen = true;
	}
	
	return B_OK;
}

void SonyScreenDimmer::StopSaver()
{
	if (fPowerOnScreen && (fNetronDisplay != NULL)) {
		// set the monitor power state to "on"
		fNetronDisplay->SetPowerState(NetronDisplay::POWER_ON);
		fNetronDisplay->SetIgnorePowerKey(false);
		fPowerOnScreen = false;
	}
}

void SonyScreenDimmer::Draw(BView *v, int32 frame)
{
	if(frame == 0)
		v->FillRect(v->Bounds(), B_SOLID_LOW);
}
