
#include <support2/Atom.h>
#include <support2/Locker.h>
#include <support2/Team.h>
#include <render2/Color.h>
#include <support2/String.h>
#include <support2/ValueStream.h>
#include <interface2/View.h>
#include <content2/Content.h>

#include "SoundBeOS.h"
#include "Event.hxx"
#include "Console.hxx"

using namespace B::Content2;

// ********************************************************************
class STellaView;

class STellaContent : public BContent, public LValueOutput
{
friend class STellaView;

	enum controller_t
	{ // The value is the index in the menu
		JOYSTICK=0,
		BOOSTER_GRIP=1,
		DRIVING=2,
		PADDLE=3,
		KEYBOARD=4,
		LIGHTGUN=5,
		MINDLINK=6
	};

	struct game_info {
		BString name;			// "Cartridge.Name"
		BString manufacturer;	// "Cartridge.Manufacturer"
		BString rarity;			// "Cartridge.Rarity"
		BString note;			// "Cartridge.Note"
		BString format;			// "Display.Format"
		int		width;			// "Display.Width"
		int		height;			// "Display.Height"
	};

	struct rectangle_t {
		BRect r;
		BColor32 c;	
	};

public:
			STellaContent();
		virtual	~STellaContent();
		IView::ptr CreateView(const BMessage &attr);

		void SetView(const atom_ref<STellaView>& view);
	
		virtual	void 		DispatchEvent(const BMessage& msg, const IView::ptr& view);
		virtual	void 		KeyDown(const BMessage& msg, const char *bytes, int32 numBytes);
		virtual	void 		KeyUp(const BMessage& msg, const char *bytes, int32 numBytes);

		virtual	BValue		Inspect(const BValue &which, uint32 flags = 0);
		virtual	status_t	Write(const BValue &out);
		virtual	status_t	End();

			status_t InsertCartridge(const char *file);
			void RemoveCartridge();
			void ResumeGame();
			void UpdateScreen(bool redraw_entire_frame);
			void DrawScreen();
			void GetRectangles(rectangle_t** r, uint32 *n) {
				*r = fRectangles[fRectIndex];
				*n = fNbRectangles[fRectIndex];
			}
			void SetLeftController(controller_t controller)	{fLeftController = controller; }
			void SetRightController(controller_t controller) {fRightController = controller; }
			int32 GetLeftController() const {return fLeftController; }
			int32 GetRightController() const {return fRightController; } 

			bool Lock();
			void Unlock();

		B_STANDARD_ATOM_TYPEDEFS(STellaContent)

private:
	static int32 _ComputeThread(void *user)	{return ((STellaContent *)user)->ComputeThread();}
			int32 ComputeThread();

	void handle_key(const BMessage& msg, const char *bytes, int32 numBytes, int32 value);
	void setupProperties(PropertiesSet& set);
	void get_game_info();

private:
	int			fRectIndex;
	uint32		fNbRectangles[2];
	rectangle_t *fRectangles[2];
	
	Console 	*fConsole;			// Pointer to the console object or the null pointer
	SoundBeOS 	fSound;				// Create a sound object for use with the console
	Event 		fEvent;				// Event object to use
	bool 		fQuitIndicator;		// Indicates if the user wants to quit
	PropertiesSet *fPropertiesSet;
	uint32		fHeight;
	uint32		fWidth;
	thread_id	fComputeThreadID;
	BColor32	fPalette[256];
	int			fCurrentFrameBufferIndex;
	uint8		*fConsummerFrameBuffer[2];
	sem_id		fDrawSemID;
	int32		fLeftController;
	int32		fRightController;
	float		fFrameDuration;
	game_info	fGameInfo;
		
	atom_ref<STellaView>	m_view;
	BNestedLocker			m_lock;
};


class STellaView : public BView
{
public:
			STellaView(const STellaContent::ptr& content);
			STellaView(const STellaView &copyFrom);
	virtual	BView *Copy() { return new STellaView(*this); }	
	virtual void Draw(const IRender::ptr& into);
	virtual	void DispatchEvent(const BMessage &msg, const BPoint& where, event_dispatch_result *result);

		B_STANDARD_ATOM_TYPEDEFS(STellaView)

private:
	STellaContent::ptr		m_content;
};

