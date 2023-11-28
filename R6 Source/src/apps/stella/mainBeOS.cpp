#include <OS.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream.h>
#include <iostream.h>
#include <strstream.h>
#include <string.h>
#include <time.h>

#include <render2/Color.h>
#include <support2/Looper.h>
#include <support2/MessageCodes.h>
#include <support2/StdIO.h>
#include <storage2/Path.h>
#include <render2/Render.h>
#include <render2/Region.h>
#include <render2/Update.h>
#include <interface2/InterfaceEventsDefs.h>
#include <interface2/SurfaceManager.h>
#include <interface2/View.h>

using namespace B::Support2;
using namespace B::Render2;
using namespace B::Interface2;
using namespace B::Storage2;

#include "mainBeOS.h"
#include "DefProps.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"

// ********************************************************************

B::Support2::IBinder::ptr
root(const B::Support2::BValue &/*params*/)
{
	STellaContent *the_game = new STellaContent();	
	return (BContent*)the_game;
}

int main(int , char **)
{
	BValue v;
	BLooper::SetRootObject(root(v));
	return BLooper::Loop();
}

// ********************************************************************
// #pragma mark -


STellaView::STellaView(const STellaContent::ptr& content)
	: 	BView(),
		m_content(content)
{
}

STellaView::STellaView(const STellaView& copyFrom)
	: BView(copyFrom)
{
	m_content = copyFrom.m_content;
	if (m_content != NULL) {
		m_content->SetView(this);
	}
}

void STellaView::DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result)
{
//	bout << msg << endl;
	m_content->DispatchEvent(msg, this);
	if (result)
		*result = B_EVENT_ABSORBED;
}

void STellaView::Draw(const IRender::ptr& child)
{
	static int d = 0;
	B2dTransform tr = 	B2dTransform::MakeTranslate(-320, -240) *
						B2dTransform::MakeScale(1.5,1.5) *
						B2dTransform::MakeRotate((d * 3.14)/180) *
						B2dTransform::MakeTranslate(320, 240);					
	child->Transform(tr);
	d = (d+2) % 360;

	if (m_content->Lock()) {
		STellaContent::rectangle_t *rects;
		uint32 count;
		m_content->GetRectangles(&rects, &count);
		const float f = Shape().Bounds().Width() / (m_content->fWidth*2);	
		for (uint32 i=0 ; i<count ; i++)
		{ // Draw all the rectangles
			const BRect r = rects[i].r * f;
			//child->Clear();
			child->Rect(r);
			child->Color(rects[i].c);
		}
		m_content->Unlock();
	}
}

// ********************************************************************

STellaContent::STellaContent()
	:	fRectIndex(0),
		fConsole(NULL),
		fQuitIndicator(false),
		fPropertiesSet(NULL),
		fCurrentFrameBufferIndex(0),
		fLeftController(JOYSTICK),
		fRightController(JOYSTICK),
		fFrameDuration(1000000/60)
{
	fNbRectangles[0] = 0;
	fNbRectangles[1] = 0;
	fRectangles[0] = NULL;
	fRectangles[1] = NULL;
	fConsummerFrameBuffer[0] = NULL;
	fConsummerFrameBuffer[1] = NULL;
	fWidth = 160;
	fHeight = 100;
}

STellaContent::~STellaContent()
{
	RemoveCartridge();
	delete fPropertiesSet;
	fPropertiesSet = NULL;
}

BValue 
STellaContent::Inspect(const BValue &which, uint32 flags)
{
	return LContent::Inspect(which, flags)
		.Overlay(LValueOutput::Inspect(which, flags));
}

status_t 
STellaContent::Write(const BValue &out)
{
	BValue val;
	if ((val = out.ValueFor("cartridge"))) {
		InsertCartridge(val.AsString());
		ResumeGame();
	}
}

status_t 
STellaContent::End()
{
	return B_OK;
}

IView::ptr STellaContent::CreateView(const BMessage &attr)
{
	(void)attr;
	STellaView::ptr view = new STellaView(this);
	SetView(view);
	return view.ptr();
}

void STellaContent::SetView(const atom_ref<STellaView>& view)
{
	m_view = view;
}

void STellaContent::RemoveCartridge(void)
{
	// If a console was opened, then close it!
	if (fConsole)
	{
		// here, stop the game.
		fSound.reset();
		fSound.mute(true);

		// Tell the thread to quit
		fQuitIndicator = true;
		status_t result;
		wait_for_thread(fComputeThreadID, &result); // This thread is safe to quit

		// delete the console
		delete [] fConsummerFrameBuffer[0]; fConsummerFrameBuffer[0] = NULL;
		delete [] fConsummerFrameBuffer[1]; fConsummerFrameBuffer[1] = NULL;
		delete fConsole; fConsole = NULL;
		delete [] fRectangles[0]; fRectangles[0] = NULL;
		delete [] fRectangles[1]; fRectangles[1] = NULL;
	}
}

status_t STellaContent::InsertCartridge(const char *file)
{
	// Open the cartridge image and read it in
	ifstream in(file);
	if (!in)
		return B_ERROR;

	// Create a properties set for us to use and set it up
	if (fPropertiesSet == NULL)
	{
		fPropertiesSet = new PropertiesSet("Cartridge.Name");
		setupProperties(*fPropertiesSet);	
	}

	// If a console was opened, then close it!
	RemoveCartridge();

	uInt8* image = new uInt8[512 * 1024];
	in.read((char *)image, 512 * 1024);
	uInt32 size = in.gcount();
	in.close();
	
	// Get just the filename of the file containing the ROM image
	BPath p(file);
	const char* filename = p.Leaf();
	
	// Create the 2600 game console
	fConsole = new Console(image, size, filename, fEvent, *fPropertiesSet, fSound);

	// Free the image since we don't need it any longer
	delete [] image;
	image = NULL;

	// The palette
	const uInt32* palette = fConsole->mediaSource().palette();
	for (int i=0 ; i<256 ; i++)
	{
		fPalette[i].red = (palette[i] & 0x00ff0000) >> 16;
		fPalette[i].green = (palette[i] & 0x0000ff00) >> 8;
		fPalette[i].blue = (palette[i] & 0x000000ff);
		fPalette[i].alpha = 255;
	}

	// Switch the console ON !
	fEvent.set(Event::ConsoleOn, 1);
	fEvent.set(Event::ConsoleOff, 0);

	// Get the desired width and height of the display
	fWidth = fConsole->mediaSource().width();
	fHeight = fConsole->mediaSource().height();	

	delete [] fConsummerFrameBuffer[0]; fConsummerFrameBuffer[0] = NULL;
	delete [] fConsummerFrameBuffer[1]; fConsummerFrameBuffer[1] = NULL;

	fCurrentFrameBufferIndex = 0;
	const int frame_buffer_size = fWidth*fHeight;
	fConsummerFrameBuffer[0] = new uint8[frame_buffer_size];
	fConsummerFrameBuffer[1] = new uint8[frame_buffer_size];
	memset(fConsummerFrameBuffer[0], 0, frame_buffer_size);
	memset(fConsummerFrameBuffer[1], 0, frame_buffer_size);

	delete [] fRectangles[0]; fRectangles[0] = NULL;
	delete [] fRectangles[1]; fRectangles[1] = NULL;
	fRectangles[0] = new rectangle_t[frame_buffer_size];
	fRectangles[1] = new rectangle_t[frame_buffer_size];

	// Get informations on the game
	get_game_info();

	// Set the controllers
	BString leftController(fConsole->properties().get("Controller.Left").c_str());
	BString rightController(fConsole->properties().get("Controller.Right").c_str());

	if (leftController == "Booster-Grip")	SetLeftController(BOOSTER_GRIP);
	else if (leftController == "Driving")	SetLeftController(DRIVING);
	else if (leftController == "Keyboard")	SetLeftController(KEYBOARD);
	else if (leftController == "Paddles")	SetLeftController(PADDLE);
	else if (leftController == "Lightgun")	SetLeftController(LIGHTGUN);
	else 									SetLeftController(JOYSTICK);

	if (rightController == "Booster-Grip")	SetRightController(BOOSTER_GRIP);
	else if (rightController == "Driving")	SetRightController(DRIVING);
	else if (rightController == "Keyboard")	SetRightController(KEYBOARD);
	else if (rightController == "Paddles")	SetRightController(PADDLE);
	else 									SetRightController(JOYSTICK);

	// Set frame rate
	fFrameDuration = 1000000/60;
	if ((fGameInfo.format == "PAL") || (fGameInfo.format == "SECAM"))
		fFrameDuration = 1000000/50;
	
	// spawn threads (the draw thread must have a lower priority)
	fQuitIndicator = false;

	// The computing thread must have a priority greater than the display thread
	fComputeThreadID = BLooper::SpawnThread(_ComputeThread, "STella_compute_thread", B_URGENT_DISPLAY_PRIORITY, this);

	// No error
	return B_OK;
}


//  Setup the properties set by loading builtin defaults and then a
//  set of user specific ones from the file "stella.pro"
//  @param set The properties set to setup

void STellaContent::setupProperties(PropertiesSet& set)
{
	// Open the file so use the builtin properties file
	strstream builtin;
	for (const char** p = defaultPropertiesFile(); *p != 0; ++p)
		builtin << *p << endl;		
	set.load(builtin, &Console::defaultProperties());
}


void STellaContent::DispatchEvent(const BMessage& msg, const IView::ptr& view)
{
	//bout << msg << endl;
	BContent::DispatchEvent(msg, view);
}

void STellaContent::KeyDown(const BMessage& msg, const char *bytes, int32 numBytes)
{
	handle_key(msg, bytes, numBytes, 1);
}

void STellaContent::KeyUp(const BMessage& msg, const char *bytes, int32 numBytes)
{
	handle_key(msg, bytes, numBytes, 0);
}


void STellaContent::handle_key(const BMessage& msg, const char *bytes, int32, int32 value)
{
	const bool repeat = msg.Data()["be:key_repeat"].AsBool();
	if (repeat)	return; // We don't want repeated keys

	Event& event = fEvent;
	int32 key = msg.Data()["key"].AsInteger();
	if (bytes[0] != B_FUNCTION_KEY)
	{ 
		// Left Controller
		if ((fLeftController == JOYSTICK) || (fLeftController == BOOSTER_GRIP))
		{
			if ((key == 40) || (bytes[0] == B_UP_ARROW))			event.set(Event::JoystickZeroUp, value);	// Left UP
			else if ((key == 60) || (bytes[0] == B_LEFT_ARROW))		event.set(Event::JoystickZeroLeft, value);	// Left LEFT
			else if ((key == 61) || (bytes[0] == B_DOWN_ARROW))		event.set(Event::JoystickZeroDown, value);	// Left DOWN
			else if ((key == 62) || (bytes[0] == B_RIGHT_ARROW))	event.set(Event::JoystickZeroRight, value);	// Left RIGHT
			else if ((bytes[0] == B_TAB) || (bytes[0] == B_SPACE))	event.set(Event::JoystickZeroFire, value);	// Left FIRE
			else if (fLeftController == BOOSTER_GRIP)
			{	if ((key == 18) || (key == 76))				event.set(Event::BoosterGripZeroTrigger, value);	// Left Trigger
				else if ((key == 19) || (key == 77))		event.set(Event::BoosterGripZeroBooster, value);	// Left Booster
			}
		}
		else if (fLeftController == DRIVING)
		{
			if ((bytes[0] == B_TAB) || (bytes[0] == B_SPACE))
			{	event.set(Event::JoystickZeroFire, value);	// Left FIRE
				event.set(Event::DrivingZeroFire, value);
			}
			else if ((key == 60) || (bytes[0] == B_LEFT_ARROW))
			{	event.set(Event::JoystickZeroLeft, value);	// Left LEFT
			}
			else if ((key == 62) || (bytes[0] == B_RIGHT_ARROW))
			{	event.set(Event::JoystickZeroRight, value);	// Left RIGHT
			}
		}
		else if (fLeftController == PADDLE)
		{
			if ((bytes[0] == B_TAB) || (bytes[0] == B_SPACE))	event.set(Event::PaddleZeroFire, value);	// Left FIRE
		}
		else if (fLeftController == LIGHTGUN)
		{
			if ((bytes[0] == B_TAB) || (bytes[0] == B_SPACE))
				event.set(Event::LightGunZeroFire, value);	// Left FIRE
		}
		else if (fLeftController == KEYBOARD)
		{
			switch (key)
			{
				case 18:	event.set(Event::KeyboardZero1, value);		break;
				case 19:	event.set(Event::KeyboardZero2, value);		break;
				case 20:	event.set(Event::KeyboardZero3, value);		break;
				case 39:	event.set(Event::KeyboardZero4, value);		break;
				case 40:	event.set(Event::KeyboardZero5, value);		break;
				case 41:	event.set(Event::KeyboardZero6, value);		break;
				case 60:	event.set(Event::KeyboardZero7, value);		break;
				case 61:	event.set(Event::KeyboardZero8, value);		break;
				case 62:	event.set(Event::KeyboardZero9, value);		break;
				case 76:	event.set(Event::KeyboardZeroStar, value);	break;
				case 77:	event.set(Event::KeyboardZero0, value);		break;
				case 78:	event.set(Event::KeyboardZeroPound, value);	break;
			}
		}
	
		// Right Controller
		if ((fRightController == JOYSTICK) || (fRightController == BOOSTER_GRIP))
		{
			if (key == 47)			event.set(Event::JoystickOneUp, value);		// Right UP
			else if (key == 67)		event.set(Event::JoystickOneLeft, value);	// Right LEFT
			else if (key == 68)		event.set(Event::JoystickOneDown, value);	// Right DOWN
			else if (key == 69)		event.set(Event::JoystickOneRight, value);	// Right RIGHT
			else if (key == 66)		event.set(Event::JoystickOneFire, value);	// Right FIRE
			else if (fRightController == BOOSTER_GRIP)
			{	if (key == 81)		event.set(Event::BoosterGripOneTrigger, value);	// Left Trigger
				else if (key == 82)	event.set(Event::BoosterGripOneBooster, value);	// Left Booster
			}
		}
		else if (fRightController == DRIVING)
		{
			if (key == 66)
			{	event.set(Event::JoystickOneFire, value);	// Right FIRE
				event.set(Event::DrivingOneFire, value);
			}
			else if (key == 67)
			{	event.set(Event::JoystickOneLeft, value);	// Right LEFT
			}
			else if (key == 69)
			{	event.set(Event::JoystickOneRight, value);	// Right RIGHT
			}
		}
		else if (fRightController == PADDLE)
		{
			if (key == 66)		event.set(Event::PaddleZeroFire, value);	// Right FIRE
		}
		else if (fRightController == KEYBOARD)
		{
			switch (key)
			{
				case 25:	event.set(Event::KeyboardOne1, value);		break;
				case 26:	event.set(Event::KeyboardOne2, value);		break;
				case 27:	event.set(Event::KeyboardOne3, value);		break;
				case 46:	event.set(Event::KeyboardOne4, value);		break;
				case 47:	event.set(Event::KeyboardOne5, value);		break;
				case 48:	event.set(Event::KeyboardOne6, value);		break;
				case 67:	event.set(Event::KeyboardOne7, value);		break;
				case 68:	event.set(Event::KeyboardOne8, value);		break;
				case 69:	event.set(Event::KeyboardOne9, value);		break;
				case 83:	event.set(Event::KeyboardOneStar, value);	break;
				case 84:	event.set(Event::KeyboardOne0, value);		break;
				case 85:	event.set(Event::KeyboardOnePound, value);	break;
			}
		}
	}
	else	// Function Key
	{
		switch (key)
		{
			case B_F1_KEY:	// Select game
				event.set(Event::ConsoleSelect, value);
				break;
			case B_F2_KEY:	// Game Reset
				event.set(Event::ConsoleReset, value);
				break;
		}
		if (value == 1)
		{ // On KeyDown only
			switch (key)
			{
				case B_F3_KEY:	// Color TV
					event.set(Event::ConsoleColor, 		1);
					event.set(Event::ConsoleBlackWhite, 0);
					break;
				case B_F4_KEY:	// Black & White TV
					event.set(Event::ConsoleColor, 		0);
					event.set(Event::ConsoleBlackWhite, 1);
					break;	
				case B_F5_KEY: // Left Player Difficulty B
					event.set(Event::ConsoleLeftDifficultyA, 0);
					event.set(Event::ConsoleLeftDifficultyB, 1);
					break;
				case B_F6_KEY: // Left Player Difficulty A
					event.set(Event::ConsoleLeftDifficultyA, 1);
					event.set(Event::ConsoleLeftDifficultyB, 0);
					break;
				case B_F7_KEY: // Right Player Difficulty B
					event.set(Event::ConsoleRightDifficultyA, 0);
					event.set(Event::ConsoleRightDifficultyB, 1);
					break;
				case B_F8_KEY: // Right Player Difficulty A
					event.set(Event::ConsoleRightDifficultyA, 1);
					event.set(Event::ConsoleRightDifficultyB, 0);
					break;
			}
		}
	}
}

void STellaContent::ResumeGame()
{
	resume_thread(fComputeThreadID);
	fSound.mute(false);
}

void STellaContent::DrawScreen()
{
	STellaView::ptr v = m_view.promote();
	if (v != NULL) {
		BRegion r = v->Shape();
		v->Invalidate(BUpdate(BUpdate::B_OPAQUE,B2dTransform::MakeIdentity(),r,r));
	}
}

//void STellaContent::DrawScreen()
//{
//	STellaView::ptr v = m_view.promote();
//	if (v != NULL) {
//		IViewParent::ptr p = v->Parent();
//		if (p != NULL) {			
//			BRect r(0,150,100,250);
//			B2dTransform tr0 = B2dTransform::MakeTranslate(0,150);
//			B2dTransform tr1 = B2dTransform::MakeTranslate(1,150);
//			BUpdate update(BUpdate::B_OPAQUE, tr0, r, tr1, r);
//			p->InvalidateChild(v, update);
//			
//		}
//	}
//}


void STellaContent::UpdateScreen(bool redraw_entire_frame)
{
	// Get the frame buffer
	uInt8* currentFrame = fConsummerFrameBuffer[fCurrentFrameBufferIndex];
	uInt8* previousFrame = fConsummerFrameBuffer[1-fCurrentFrameBufferIndex];

	// Alias the fNbRectangles variable
	uint32& rindex = fNbRectangles[1-fRectIndex];
	rectangle_t *rects = fRectangles[1-fRectIndex];
	rindex = 0;

	struct Rectangle {
		int color;
		int x, y, width, height;
	} rectangles[2][160];
	
	// This array represents the rectangles that need displaying
	// on the current scanline we're processing
	Rectangle* currentRectangles = rectangles[0];
	
	// This array represents the rectangles that are still active
	// from the previous scanlines we have processed
	Rectangle* activeRectangles = rectangles[1];
	
	// Indicates the number of active rectangles
	int activeCount = 0;

	// This update procedure requires fWidth to be a multiple of four.
	// This is validated when the properties are loaded.
	for (int y=0; y<(int)fHeight ; y++)
	{
		// Indicates the number of current rectangles
		int currentCount = 0;
		
		// Look at four pixels at a time to see if anything has changed
		const uInt32 *current = (const uInt32 *)currentFrame;
		const uInt32 *previous = (const uInt32 *)previousFrame;
		
		for (int x=0 ; x<(int)fWidth ; x+=4, current++, previous++)
		{
			// Has something changed in this set of four pixels?
			uInt32 lc, lp;
			if (((lc=*current) != (lp=*previous)) || redraw_entire_frame)
			{
				// Look at each of the bytes that make up the uInt32
				lp = B_HOST_TO_LENDIAN_INT32(lp);
				lc = B_HOST_TO_LENDIAN_INT32(lc);

				for (int i=0; i<4; i++)
				{
					const int p = lp & 0xFF;
					const int c = lc & 0xFF;
					lp>>=8;
					lc>>=8;
					
					// See if this pixel has changed
					if (redraw_entire_frame || (c != p))
					{
						// Can we extend a rectangle or do we have to create a new one?
						if ((currentCount != 0) &&
							(currentRectangles[currentCount - 1].color == c) &&
							((currentRectangles[currentCount - 1].x + currentRectangles[currentCount - 1].width) == (x + i)))
						{
							currentRectangles[currentCount - 1].width++;
						}
						else
						{
							currentRectangles[currentCount].x = x + i;
							currentRectangles[currentCount].y = y;
							currentRectangles[currentCount].width = 1;
							currentRectangles[currentCount].height = 1;
							currentRectangles[currentCount].color = c;
							currentCount++;
						}
					}
				}
			}
		}
		
		// Merge the active and current rectangles flushing any that are of no use
		int activeIndex = 0;
		for (int t=0; (t<currentCount) && (activeIndex<activeCount) ; )
		{
			Rectangle& current = currentRectangles[t];
			Rectangle& active = activeRectangles[activeIndex];

			if ((current.x == active.x) &&
				(current.width == active.width) &&
				(current.color == active.color))
			{ // Can we merge the current rectangle with an active one?
				current.y = active.y;
				current.height = active.height + 1;
				activeIndex++;
				t++;
			}
			else 
			{ // Can't merge current and active rectangles
				if (current.x+current.width > active.x)
				{ // Is it possible for this active rectangle to be merged?
					// No - Flush the active rectangle
					rects[rindex].r.right = (rects[rindex].r.left = active.x * 2) + active.width * 2;
					rects[rindex].r.bottom = (rects[rindex].r.top = active.y) + active.height;				
					rects[rindex].c = fPalette[active.color];
					rindex++;
					activeIndex++;
				}

				if (current.x+current.width <= active.x+active.width)
				{
					t++;
				}
			}
		}

		// Flush any remaining active rectangles
		for (int s=activeIndex ; s<activeCount ; s++)
		{
			Rectangle& active = activeRectangles[s];	
			rects[rindex].r.right = (rects[rindex].r.left = active.x * 2) + active.width * 2;
			rects[rindex].r.bottom = (rects[rindex].r.top = active.y) + active.height;				
			rects[rindex].c = fPalette[active.color];
			rindex++;
		}
		
		// We can now make the current rectangles into the active rectangles
		Rectangle* tmp = currentRectangles;
		currentRectangles = activeRectangles;
		activeRectangles = tmp;
		activeCount = currentCount;
		
		currentFrame += fWidth;
		previousFrame += fWidth;
	}
	
	// Flush any rectangles that are still active
	for (int t=0 ; t<activeCount ; t++)
	{
		Rectangle& active = activeRectangles[t];
		rects[rindex].r.right = (rects[rindex].r.left = active.x * 2) + active.width * 2;
		rects[rindex].r.bottom = (rects[rindex].r.top = active.y) + active.height;				
		rects[rindex].c = fPalette[active.color];
		rindex++;
	}
}

int32 STellaContent::ComputeThread(void)
{
	MediaSource& mediaSource = fConsole->mediaSource();
	const int size = fWidth*fHeight;	
	bigtime_t wait_until = 0;

	while (fQuitIndicator == false)
	{	
		// Wait 1/60th second
		snooze_until(wait_until, B_SYSTEM_TIMEBASE);
		wait_until = system_time() + (bigtime_t)fFrameDuration;

		// Invalidate screen
		DrawScreen();

		// Draw the frame and handle events
		fConsole->mediaSource().update();
		fCurrentFrameBufferIndex = 1-fCurrentFrameBufferIndex;
		memcpy(fConsummerFrameBuffer[fCurrentFrameBufferIndex], mediaSource.currentFrameBuffer(), size);
		UpdateScreen(true);
		if (Lock()) {
			fRectIndex = 1-fRectIndex;
			Unlock();
		}
	}

	return B_OK;
}

bool STellaContent::Lock()
{
	return (m_lock.Lock() == B_OK);
}

void STellaContent::Unlock()
{
	m_lock.Unlock();
}

void STellaContent::get_game_info()
{
	fGameInfo.name 			= fConsole->properties().get("Cartridge.Name").c_str();
	fGameInfo.manufacturer	= fConsole->properties().get("Cartridge.Manufacturer").c_str();
	fGameInfo.rarity 		= fConsole->properties().get("Cartridge.Rarity").c_str();
	fGameInfo.note			= fConsole->properties().get("Cartridge.Note").c_str();
	fGameInfo.format		= fConsole->properties().get("Display.Format").c_str();
	fGameInfo.width 		= atoi(fConsole->properties().get("Display.Width").c_str());
	fGameInfo.height 		= atoi(fConsole->properties().get("Display.Height").c_str());
	if (fGameInfo.width == 0)	fGameInfo.width = 160;
	if (fGameInfo.height == 0)	fGameInfo.height = 210;
}
