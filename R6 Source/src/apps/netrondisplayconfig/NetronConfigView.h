#ifndef _NETRON_CONFIG_VIEW
#define _NETRON_CONFIG_VIEW

#include <View.h>
#include <ScrollBar.h>
#include <experimental/ResourceSet.h>
#include <BitmapButton.h>
#include <Bitmap.h>
#include <MessageRunner.h>
#include "NetronDisplay.h"

static const uint8 kRegisters[11] = {
	0x0A,		// Contrast
	0x0B,		// Brightness
	0x03,		// H Center
	0x01,		// H Size
	0x02,		// V Center
	0x00,		// Size
	0x09,		// Rotation
	0x04,		// V Pincushion
	0x07,		// V Pin Balance
	0x06,		// V Keystone
	0x08		// V Key Balance
};

static const size_t kNumRegisters = (sizeof(kRegisters) / sizeof(kRegisters[0]));

enum register_control {
	contrast_control = 0,
	brightness_control,
	h_center_control,
	h_size_control,
	v_center_control,
	v_size_control,
	rotation_control,
	v_pincushion_control,
	v_pin_balance_control,
	v_keystone_control,
	v_key_balance_control
};

enum netron_display_mode {
	display_basic = 0,
	display_geometry
};

const int32 kSliderCount = 11;

const uint32 M_LOAD_BASIC_DEFAULTS = 'defb';
const uint32 M_LOAD_GEOMETRY_DEFAULTS = 'defg';
const uint32 M_DEGAUSS = 'dega';
const uint32 M_PAGE = 'Page';
const uint32 M_REPEAT_ADJUST_START = 'rads';
const uint32 M_REPEAT_ADJUST = 'radj';
const uint32 M_CANCEL = 'cncl';
const uint32 M_APPLY = 'aply';

class NetronConfigView : public BView
{
	public:
		NetronConfigView(BRect frame);
		~NetronConfigView(void);
		
		virtual void AttachedToWindow(void);
		virtual void Draw(BRect rect);
		virtual void MessageReceived(BMessage *inMessage);
		
		void ToggleDisplayMode(void);
		void RefreshSliders(bool basic, bool geometry, bool factorySetting = false);
		void SaveValues(void);
		void RestoreValues(void);
		
	private:
		void CreateButtons(BPoint degauss, BRect sliderRect);
		
		uint8 fOrigValues[kNumRegisters];
		BResourceSet fResources;
		NetronDisplay *fDisplay;
		BBitmapButton* fDegaussButton;
		BBitmapButton* fDefaultBasicButton;
		BBitmapButton* fDefaultGeometryButton;
		BBitmapButton* fBasicButton;
		BBitmapButton* fOtherButton;
		const BBitmap* fDecrementBitmap;
		const BBitmap* fDecrementOverBitmap;
		const BBitmap* fDecrementDownBitmap;
		const BBitmap* fIncrementBitmap;
		const BBitmap* fIncrementOverBitmap;
		const BBitmap* fIncrementDownBitmap;
		netron_display_mode fDisplayMode;
};

class NetronSlider : public BView
{
	public:

		typedef enum {
			NORMAL_STATE,
			LEFT_OVER_STATE,
			RIGHT_OVER_STATE,
			LEFT_DOWN_STATE,
			RIGHT_DOWN_STATE
		} interaction_state;

		NetronSlider(BRect frame, const char *name, NetronDisplay *display,
						register_control rc, const BBitmap *leftBitmap, const BBitmap *rightBitmap,
						const BBitmap *dec, const BBitmap *decOver, const BBitmap *decDown,
						const BBitmap *inc, const BBitmap *incOver, const BBitmap *incDown);
		~NetronSlider();
		
		virtual void AttachedToWindow();
		virtual void Draw(BRect rect);
		virtual void MouseDown(BPoint pt);
		virtual void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);
		virtual void MouseUp(BPoint pt);
		virtual void MessageReceived(BMessage *inMessage);
		void PerformAdjustment();
		void StopAdjustment();
		
		void Refresh(bool factorySetting = false);
		void SetRange(int32 minRange, int32 maxRange,
		              int32 minRange2 = 0, int32 maxRange2 = 255);
		void SetStepSize(int32 newStepSize);
		void UpdateSettings();
		void UpdatePosition();
		void SetValue(uint8 value);
		void SetState(interaction_state nuState);
		
	private:
	
		register_control fControl;
		NetronDisplay *fDisplay;
		uint8  fMinValue;
		uint8  fMaxValue;
		uint8  fStepSize;
		uint8  fCurrentValue;
		int32  fSavedValue;
		bool   fMouseDown;
		float  fDelta;
		float  fPos;
		int32  fCurrentAdjustment;
		BMessageRunner *fAdjustmentRunner;

		const BBitmap *fLeftBitmap;
		const BBitmap *fRightBitmap;
		const BBitmap *fDecrementBitmap;
		const BBitmap *fDecrementOverBitmap;
		const BBitmap *fDecrementDownBitmap;
		const BBitmap *fIncrementBitmap;
		const BBitmap *fIncrementOverBitmap;
		const BBitmap *fIncrementDownBitmap;

		BRect fLeftRect;
		BRect fRightRect;
		BRect fSliderRect;		
		
		interaction_state fState;
};

#endif
