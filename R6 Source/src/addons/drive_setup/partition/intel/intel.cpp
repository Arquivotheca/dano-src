/*--------------------------------------------------------------------*\
  File:      intel.cpp
  Creator:   Robert Polic, Matt Bogosian <mattb@be.com>
  Copyright: (c)1997, 1998, Be, Inc. All rights reserved.
  Description: Source file for the intel DriveSetup add-on. Original
      version by Robert Polic (I just work here).
\*--------------------------------------------------------------------*/

#define DEBUG 1
#include <Debug.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <MenuBar.h>
#include <Screen.h>
#include <PictureButton.h>
#include <interface_misc.h>

#include "intel.h"

/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

static const uint32 k_no_slider(0x00000000);
static const uint32 k_left_slider(0x00000001);
static const uint32 k_right_slider(k_left_slider << 1);
static const int32 k_lock_width(25);
static const off_t k_magnitude(1024 * 1024);
static const char * const k_part_str_100 = "1 100% Partition";
static const char * const k_part_str_50 = "2 50% Partitions";
static const char * const k_part_str_33 = "3 33% Partitions";
static const char * const k_part_str_25 = "4 25% Partitions";


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Structs, Classes =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

class TPartView;

/*--------------------------------------------------------------------*\
  Class name:       LockButton
  Inherits from:    BPictureButton
  New data members: private status_t m_status - the status of the
                        object.
                    private BControl *m_target - .
  Description: Class to implement a . WARNING: this class is not
      stable between the time it is constructed and the time it is
      attached to a window. Do not delete an object of this class
      between those times! (This is a limitation of the BPictureButton
      class.) Okay, so this is a REALLY crappy hack, but oh well...
      this whole module is bad news and will soon be replaced, so WTF?
\*--------------------------------------------------------------------*/

class LockButton : public BPictureButton
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: LockButton
  Member of:     public LockButton
  Arguments:     const BRect a_frame - .
                 const char * const a_name - .
                 BControl * const a_target - .
                 BMessage * const a_msg - Default: NULL.
                 const uint32 a_behavior - Default: B_TWO_STATE_BUTTON.
                 const uint32 a_resizing_mode - Default: B_FOLLOW_LEFT | B_FOLLOW_TOP.
                 const uint32 a_flags - Default: B_WILL_DRAW | B_NAVIGABLE.
                 int * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		LockButton(const BRect a_frame, const char * const a_name, BControl * const a_target, BMessage * const a_msg = NULL, const uint32 a_behavior = B_TWO_STATE_BUTTON, const uint32 a_resizing_mode = B_FOLLOW_LEFT | B_FOLLOW_TOP, const uint32 a_flags = B_WILL_DRAW | B_NAVIGABLE, int * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~LockButton
  Member of:     public LockButton
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~LockButton(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual AttachedToWindow
  Member of:     public LockButton
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void AttachedToWindow(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Invoke
  Member of:     public LockButton
  Arguments:     BMessage * const a_msg - Default: NULL.
  Returns:       status_t - .
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual status_t Invoke(BMessage * const a_msg = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Status const
  Member of:     public LockButton
  Arguments:     none
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Classes who wish to override this
      function, should always call this function first to insure that
      the underlying structure is valid. Possible return values are as
      follows:
      
      B_NO_ERROR - the object was created successfully.
\*--------------------------------------------------------------------*/
		
		virtual status_t Status(void) const;
	
	private:
	
		typedef BPictureButton Inherited;
		
		// Private data members
		status_t m_status;
		BControl *m_target;
		
		// Private prohibitted member functions
		LockButton(const LockButton &a_obj);
		LockButton &operator=(const LockButton &a_obj);
};

/*--------------------------------------------------------------------*\
  Class name:       TSliderView
  Inherits from:    public BControl
  New data members: private status_t m_status - the status of the
                        object.
                    private int32 m_active_slider - .
                    private off_t m_min - .
                    private off_t m_max - .
                    private off_t m_left_bound - .
                    private off_t m_right_bound - .
                    private off_t m_left - .
                    private off_t m_right - .
                    private off_t m_orig_left - .
                    private off_t m_orig_right - .
                    private off_t m_granularity - .
                    private BBitmap *m_off_bitmap - .
                    private BView *m_off_view - .
                    private BControl *m_lock_button - .
                    private TPartView *m_parent - .
  Description: Class to implement a view in which a constant string is
      constantly displayed.
\*--------------------------------------------------------------------*/

class TSliderView : public BControl
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: TSliderView
  Member of:     public TSliderView
  Arguments:     const BRect a_frame - the frame of the view.
                 const off_t a_min - .
                 const off_t a_max - .
                 const off_t a_left_bound - .
                 const off_t a_right_bound - .
                 const off_t a_left - .
                 const off_t a_right - .
                 const off_t a_orig_left - .
                 const off_t a_orig_right - .
                 const bool a_read_only - .
                 const off_t a_granularity - Default: 1.
                 const char * const a_name - the name of the view.
                     Default: NULL.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		TSliderView(const BRect a_frame, const off_t a_min, const off_t a_max, const off_t a_left_bound, const off_t a_right_bound, const off_t a_left, const off_t a_right, const bool a_read_only, off_t a_granularity = 1, const char * const a_name = NULL, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~TSliderView
  Member of:     public TSliderView
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~TSliderView(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual AttachedToWindow
  Member of:     public TSliderView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void AttachedToWindow(void);
		
/*--------------------------------------------------------------------*\
  Function name: inline Clear
  Member of:     public TSliderView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline void Clear(void)
		{
			m_left_bound = m_min;
			m_right_bound = m_left = m_right = m_max;
		}
		
/*--------------------------------------------------------------------*\
  Function name: virtual Draw
  Member of:     public TSliderView
  Arguments:     const BRect a_update_rect - the rect to be updated.
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void Draw(const BRect a_update_rect);
		
/*--------------------------------------------------------------------*\
  Function name: inline IsDirty
  Member of:     public TSliderView
  Arguments:     none
  Returns:       bool - .
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline bool IsDirty(void)
		{
			return (m_left != m_orig_left
				|| m_right != m_orig_right);
		}
		
/*--------------------------------------------------------------------*\
  Function name: virtual KeyDown
  Member of:     public TSliderView
  Arguments:     const char * const a_buf - .
                 const int32 a_buf_len - .
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void KeyDown(const char * const a_buf, const int32 a_buf_len);
		
/*--------------------------------------------------------------------*\
  Function name: virtual MakeFocus
  Member of:     public TSliderView
  Arguments:     const bool a_focused - Default: true.
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void MakeFocus(const bool a_focused = true);
		
/*--------------------------------------------------------------------*\
  Function name: virtual MouseDown
  Member of:     public TSliderView
  Arguments:     const BPoint a_point - .
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void MouseDown(const BPoint a_point);
		
/*--------------------------------------------------------------------*\
  Function name: inline Reset
  Member of:     public TSliderView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline void Reset(void)
		{
			Clear();
			
			m_left = m_orig_left;
			m_right = m_orig_right;
		}
		
/*--------------------------------------------------------------------*\
  Function name: virtual SetEnabled
  Member of:     public TSliderView
  Arguments:     const bool a_enabled - .
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void SetEnabled(const bool a_enabled);
		
/*--------------------------------------------------------------------*\
  Function name: inline SetSliderLock
  Member of:     public TSliderView
  Arguments:     const bool a_lock - .
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline void SetSliderLock(const bool a_lock)
		{
			m_lock_button->SetValue(a_lock ? B_CONTROL_OFF : B_CONTROL_ON);
			m_lock_button->Invoke();
		}
		
/*--------------------------------------------------------------------*\
  Function name: SetTabBounds
  Member of:     public TSliderView
  Arguments:     const off_t a_left_bound - .
                 const off_t a_right_bound - .
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void SetTabBounds(const off_t a_left_bound, const off_t a_right_bound);
		
/*--------------------------------------------------------------------*\
  Function name: SetTabs
  Member of:     public TSliderView
  Arguments:     const off_t a_left - .
                 const off_t a_right - .
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void SetTabs(const off_t a_left, const off_t a_right);
		
/*--------------------------------------------------------------------*\
  Function name: inline Status const
  Member of:     public TSliderView
  Arguments:     none
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Possible return values are as
      follows:
      
      B_NO_ERROR - the object was created successfully.
      B_NO_MEMORY - on construction, there was not enough memory
          available to create the object.
\*--------------------------------------------------------------------*/
		
		inline status_t Status(void) const
		{
			return m_status;
		}
		
/*--------------------------------------------------------------------*\
  Function name: inline TabBoundLeft
  Member of:     public TSliderView
  Arguments:     none
  Returns:       off_t - .
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline off_t TabBoundLeft(void)
		{
			return m_left_bound;
		}
		
/*--------------------------------------------------------------------*\
  Function name: inline TabBoundRight
  Member of:     public TSliderView
  Arguments:     none
  Returns:       off_t - .
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline off_t TabBoundRight(void)
		{
			return m_right_bound;
		}
		
/*--------------------------------------------------------------------*\
  Function name: inline TabLeft
  Member of:     public TSliderView
  Arguments:     none
  Returns:       off_t - .
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline off_t TabLeft(void)
		{
			return m_left;
		}
		
/*--------------------------------------------------------------------*\
  Function name: inline TabRight
  Member of:     public TSliderView
  Arguments:     none
  Returns:       off_t - .
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline off_t TabRight(void)
		{
			return m_right;
		}
	
	private:
	
		typedef BControl Inherited;
		
		// Private data members
		status_t m_status;
		int32 m_active_slider;
		off_t m_min;
		off_t m_max;
		off_t m_left_bound;
		off_t m_right_bound;
		off_t m_left;
		off_t m_right;
		off_t m_orig_left;
		off_t m_orig_right;
		off_t m_granularity;
		BBitmap *m_off_bitmap;
		BView *m_off_view;
		BControl *m_lock_button;
		TPartView *m_parent;
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: AdjustValue
  Member of:     private TSliderView
  Arguments:     const off_t a_change - .
  Returns:       bool - .
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		bool AdjustValue(const off_t a_change);
		
/*--------------------------------------------------------------------*\
  Function name: DrawSlider
  Member of:     private TSliderView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void DrawSlider(void);
		
/*--------------------------------------------------------------------*\
  Function name: SetTabsRaw
  Member of:     private TSliderView
  Arguments:     const off_t a_left - .
                 const off_t a_right - .
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void SetTabsRaw(const off_t a_left, const off_t a_right);
};

/*--------------------------------------------------------------------*\
  Class name:       TPartView
  Inherits from:    public BView
  New data members: private status_t m_status - the status of the
                        object.
                    private off_t m_min - .
                    private off_t m_max - .
                    private BMenuField *m_layout_menu - .
                    private TSliderView *m_partitions[MAX_PARTITION] -.
  Description: Class to implement a view in which .
\*--------------------------------------------------------------------*/

class TPartView : public BView
{
	public:
	
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: TPartView
  Member of:     public TPartView
  Arguments:     const BRect a_frame - the frame of the view.
                 const char * const a_name - the name of the view.
                 const off_t a_min - .
                 const off_t a_max - .
                 const off_t a_lefts[MAX_PARTITION] - .
                 const off_t a_rights[MAX_PARTITION] - .
                 const bool a_read_only - .
                 const off_t a_granularity - Default: 1.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		TPartView(const BRect a_frame, const char * const a_name, const off_t a_min, const off_t a_max, const off_t a_lefts[MAX_PARTITION], const off_t a_rights[MAX_PARTITION], const bool a_read_only, const off_t a_granularity = 1, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~TPartView
  Member of:     public TPartView
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~TPartView(void);
		
/*--------------------------------------------------------------------*\
  Function name: CalculateBounds
  Member of:     public TPartView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void CalculateBounds(void);
		
/*--------------------------------------------------------------------*\
  Function name: CheckLayoutMenu
  Member of:     public TPartView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void CheckLayoutMenu(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual MessageReceived
  Member of:     public TPartView
  Arguments:     BMessage * const a_msg - ..
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void MessageReceived(BMessage * const a_msg);
		
/*--------------------------------------------------------------------*\
  Function name: inline PartitionView
  Member of:     public TPartView
  Arguments:     const int32 a_index - .
  Returns:       TSliderView *
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		inline TSliderView *PartitionView(const int32 a_index)
		{
			if (a_index < 0
				|| a_index >= MAX_PARTITION)
			{
				return NULL;
			}
			
			return m_partitions[a_index];
		}
		
/*--------------------------------------------------------------------*\
  Function name: Reset
  Member of:     public TSliderView
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to .
\*--------------------------------------------------------------------*/
		
		void Reset(void);
		
/*--------------------------------------------------------------------*\
  Function name: inline Status const
  Member of:     public TPartView
  Arguments:     none
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Possible return values are as
      follows:
      
      B_NO_ERROR - the object was created successfully.
      B_NO_MEMORY - on construction, there was not enough memory
          available to create the object.
\*--------------------------------------------------------------------*/
		
		inline status_t Status(void) const
		{
			return m_status;
		}
	
	private:
	
		typedef BView Inherited;
		
		// Private data members
		status_t m_status;
		off_t m_min;
		off_t m_max;
		BMenuField *m_layout_menu;
		TSliderView *m_partitions[MAX_PARTITION];
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Global Variables =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

static BPicture g_off, g_on;


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
LockButton::LockButton(const BRect a_frame, const char * const a_name, BControl * const a_target, BMessage * const a_msg, const uint32 a_behavior, const uint32 a_resizing_mode, const uint32 a_flags, int * const a_err) :
//====================================================================
	Inherited(a_frame, a_name, &g_off, &g_on, a_msg, a_behavior, a_resizing_mode, a_flags),
	m_status(B_NO_ERROR),
	m_target(a_target)
{
	if (m_target != NULL)
	{
		SetValue(m_target->IsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
	}
	
	// Give the error back to the caller
	if (a_err != NULL)
	{
		*a_err = Status();
	}
}

//====================================================================
LockButton::~LockButton(void)
//====================================================================
{
	;
}

//====================================================================
void LockButton::AttachedToWindow(void)
//====================================================================
{
	Inherited::AttachedToWindow();
	
	BPicture *pictures[MAX_PARTITION] =
	{
		NULL,
		NULL,
		NULL,
		NULL
	};
	
	float pen_size(PenSize());
	int32 i;
	
	for (i = 0; i < MAX_PARTITION; i++)
	{
		if ((pictures[i] = new BPicture) == NULL)
		{
			break;
		}
		
		BeginPicture(pictures[i]);
		SetHighColor(ViewColor());
		FillRect(Bounds());
		
		if (i < 2)
		{
			SetHighColor(0x00, 0x00, 0x00);
		}
		else
		{
			SetHighColor(0x80, 0x80, 0x80);
		}
		
		BRect lock(Bounds());
		BPoint center((lock.right + lock.left) / 2, (lock.bottom + lock.top) / 2);
		lock.Set(center.x - 3, center.y, center.x + 3, center.y + 3);
		SetPenSize(1);
		FillRect(lock);
		
		if (i % 2 == 0)
		{
			center.Set(center.x + 4, center.y);
		}
		
		SetPenSize(1);
		StrokeArc(center, 2, 2, 0, 180);
		EndPicture();
	}
	
	SetPenSize(pen_size);
	
	if (i < MAX_PARTITION)
	{
		for (int32 i(0); i < MAX_PARTITION; i++)
		{
			delete pictures[i];
		}
	}
	else
	{
		SetEnabledOn(pictures[0]);
		SetEnabledOff(pictures[1]);
		SetDisabledOn(pictures[2]);
		SetDisabledOff(pictures[3]);
	}
}

//====================================================================
status_t LockButton::Invoke(BMessage * const a_msg)
//====================================================================
{
	if (m_target != NULL)
	{
		m_target->SetEnabled(Value() != B_CONTROL_OFF);
		SetValue(m_target->IsEnabled() ? B_CONTROL_ON : B_CONTROL_OFF);
	}
	
	return Inherited::Invoke(a_msg);
}

//====================================================================
status_t LockButton::Status(void) const
//====================================================================
{
	return m_status;
}

//====================================================================
TSliderView::TSliderView(const BRect a_frame, const off_t a_min, const off_t a_max, const off_t a_left_bound, const off_t a_right_bound, const off_t a_left, const off_t a_right, const bool a_read_only, const off_t a_granularity, const char * const a_name, status_t * const a_err) :
//====================================================================
	Inherited(a_frame, a_name, a_name, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE),
	m_status(B_NO_ERROR),
	m_active_slider(k_no_slider),
	m_min((a_min > a_max) ? a_max : a_min),
	m_max((a_min > a_max) ? a_min : a_max),
	m_left_bound(m_min),
	m_right_bound(m_max),
	m_left(m_left_bound),
	m_right(m_right_bound),
	m_orig_left(m_left),
	m_orig_right(m_right),
	m_granularity(a_granularity),
	m_off_bitmap(NULL),
	m_off_view(NULL),
	m_lock_button(NULL),
	m_parent(NULL)
{
	m_granularity = (m_granularity < 1) ? 1 : m_granularity;

	SetViewColor(0xD8, 0xD8, 0xD8);
	BRect off_bounds(Bounds()), lock_bounds(off_bounds);
	lock_bounds.right = k_lock_width;
	off_bounds.left += lock_bounds.right + 1;
	off_bounds.OffsetTo(0, 0);
	
	if ((m_off_bitmap = new BBitmap(off_bounds, B_CMAP8, true)) == NULL
		|| (m_off_view = new BView(off_bounds, NULL, B_FOLLOW_ALL, B_WILL_DRAW)) == NULL
		|| (m_lock_button = new LockButton(lock_bounds, NULL, this)) == NULL)
	{
		delete m_off_bitmap;
		delete m_off_view;
		m_off_bitmap = NULL;
		m_off_view = NULL;
		m_status = B_NO_MEMORY;
	}
	else
	{
		AddChild(m_lock_button);
		
		if ((m_status = dynamic_cast<LockButton *>(m_lock_button)->Status()) == B_NO_ERROR)
		{
			if (a_read_only)
			{
				SetSliderLock(true);
				m_lock_button->SetEnabled(false);
			}
		}
		
		SetTabsRaw(a_left, a_right);
		SetTabBounds(a_left_bound, a_right_bound);
		m_orig_left = TabLeft();
		m_orig_right = TabRight();
		
		BFont font(*be_plain_font);
		font.SetSize(10.0);
		SetFont(&font);
	}
	
	// Give the error back to the caller
	if (a_err != NULL)
	{
		*a_err = Status();
	}
}

//====================================================================
TSliderView::~TSliderView(void)
//====================================================================
{
	delete m_off_bitmap;
}

//====================================================================
void TSliderView::AttachedToWindow(void)
//====================================================================
{
	Inherited::AttachedToWindow();
	m_off_bitmap->AddChild(m_off_view);
	m_parent = dynamic_cast<TPartView *>(Parent());
}

//====================================================================
void TSliderView::Draw(const BRect a_update_rect)
//====================================================================
{
	Inherited::Draw(a_update_rect);
	DrawSlider();
}

//====================================================================
void TSliderView::KeyDown(const char * const a_buf, const int32 a_buf_len)
//====================================================================
{
	uint32 mods(0x00000000);
//	off_t change(k_magnitude / BLOCK_SIZE);
	off_t change(m_granularity);
	bool changed = false;		
	switch (a_buf[0])
	{
		case B_LEFT_ARROW:
			change *= -1;
			changed = true;
		break;
		
		case B_RIGHT_ARROW:
			change *= 1;
			changed = true;
		break;
		
		case B_DOWN_ARROW:
			change *= -10;
			changed = true;
		break;
		
		case B_UP_ARROW:
			change *= 10;
			changed = true;
		break;
		
		case B_TAB:
			Window()->CurrentMessage()->FindInt32("modifiers", reinterpret_cast<int32 *>(&mods));
			
			if (mods & B_SHIFT_KEY) {
				switch(m_active_slider) {
				case k_left_slider:
					m_active_slider = k_no_slider;
					break;
				case k_right_slider:
					m_active_slider = k_left_slider;
					break;	
				default:
					Inherited::KeyDown(a_buf, a_buf_len);
				}
			} else {
				switch(m_active_slider) {
				case k_no_slider:
					m_active_slider = k_left_slider;
					break;
				case k_left_slider:
					m_active_slider = k_right_slider;
					break;	
				default:
					Inherited::KeyDown(a_buf, a_buf_len);
				}
			}
			
			DrawSlider();
		break;
		
		default:
			Inherited::KeyDown(a_buf, a_buf_len);
		break;
	}
	
	if (changed && AdjustValue(change) && m_parent != NULL) {
		m_parent->CalculateBounds();
	}
}

//====================================================================
void TSliderView::MakeFocus(const bool a_focused)
//====================================================================
{
	uint32 mods(0x00000000);
	bool foundMods(false);
	BMessage *msg(Window()->CurrentMessage());
	
	if (a_focused) {
		if (msg != NULL) {
			foundMods =
				(B_OK == msg->FindInt32("modifiers", reinterpret_cast<int32 *>(&mods)));
		}
		if (foundMods && (mods & B_SHIFT_KEY)) {
			m_active_slider = k_right_slider;
		} else {
			m_active_slider = k_left_slider;
		}
	} else {
		m_active_slider = k_no_slider;
	}
	
	DrawSlider();
	Inherited::MakeFocus(a_focused);
}

//====================================================================
void TSliderView::MouseDown(const BPoint a_point)
//====================================================================
{
	if (!IsEnabled())
	{
		return;
	}
	
	BRect bounds(m_off_bitmap->Bounds());
	bounds.left += 5;
	bounds.right -= 5;
	int32 width(static_cast<int32>(bounds.Width()));
	off_t left(static_cast<off_t>(static_cast<long double>(m_left - m_min) / static_cast<long double>(m_max - m_min) * static_cast<long double>(width)));
	off_t right(static_cast<off_t>(static_cast<long double>(m_right - m_min) / static_cast<long double>(m_max - m_min) * static_cast<long double>(width)));
	off_t left_bound(static_cast<off_t>(static_cast<long double>(m_left_bound - m_min) / static_cast<long double>(m_max - m_min) * static_cast<long double>(width)));
	off_t right_bound(static_cast<off_t>(static_cast<long double>(m_right_bound - m_min) / static_cast<long double>(m_max - m_min) * static_cast<long double>(width)));
	left += k_lock_width + 1 + 6;
	right += k_lock_width + 1 + 6;
	left_bound += k_lock_width + 1 + 6;
	right_bound += k_lock_width + 1 + 6;
	BRect left_track(left - 5, bounds.bottom - 7, left, bounds.bottom - 2);
	BRect right_track(right, bounds.bottom - 7, right + 5, bounds.bottom - 2);
	BRect track;
	uint32 slider(k_no_slider);
	
	if (left_track.Contains(a_point))
	{
		slider = m_active_slider = k_left_slider;
		track = left_track;
	}
	else if (right_track.Contains(a_point))
	{
		slider = m_active_slider = k_right_slider;
		track = right_track;
	}
	
	Inherited::MakeFocus(true);
	DrawSlider();
	
	if (!track.IsValid())
	{
		return;
	}
	
	int32 offset(static_cast<int32>((slider == k_left_slider) ? a_point.x - track.right : a_point.x - track.left));
	long double unit(static_cast<long double>(width) / (static_cast<long double>(m_max - m_min)));
	bool changed(false);
	BPoint where;
	uint32 buttons;
	off_t change;
	
	do
	{
		GetMouse(&where, &buttons);
		where.x -= k_lock_width + 6 + offset;
		change = static_cast<off_t>(where.x / unit - ((slider == k_left_slider) ? TabLeft() : TabRight()));
		
		if (change != 0)
		{
			if (AdjustValue(change)
				&& changed == false)
			{
				changed = true;
			}
		}
		
		snooze(25000);
	}
	while (buttons);
	
	if (m_parent != NULL
		&& changed)
	{
		m_parent->CalculateBounds();
	}
}

//====================================================================
void TSliderView::SetEnabled(const bool a_enabled)
//====================================================================
{
	Inherited::SetEnabled(a_enabled);
	
	if (a_enabled
		&& !IsDirty()
		&& TabLeft() < TabRight())
	{
		BAlert *alert(new BAlert("WARNING", "You are about to edit an existing partition. If you modify the partition, you will lose all data on that partition. If the partition is an extended partition, you will lose all data on all partitions within that extended partition.", "Continue", "Cancel", NULL, B_WIDTH_FROM_WIDEST, B_STOP_ALERT));
		
		if (alert != NULL)
		{
			if (alert->Go() == 1)
			{
				Inherited::SetEnabled(false);
			}
		}
		else
		{
			fprintf(stderr, "WARNING: You are about to edit an existing partition. If you modify the partition, you will lose all data on that partition. If the partition is an extended partition, you will lose all data on all partitions within that extended partition.");
			Inherited::SetEnabled(a_enabled);
		}
	}
	
	if (m_parent != NULL)
	{
		m_parent->CheckLayoutMenu();
	}
}

//====================================================================
void TSliderView::SetTabBounds(const off_t a_left_bound, const off_t a_right_bound)
//====================================================================
{
	m_left_bound = (a_left_bound > a_right_bound) ? a_right_bound : a_left_bound;
	m_right_bound = (a_left_bound > a_right_bound) ? a_left_bound : a_right_bound;
	
	if (m_left_bound < m_min)
	{
		m_left_bound = m_min;
	}
	else if (m_left_bound > m_max)
	{
		m_left_bound = m_max;
	}
	else
	{
		m_left_bound = a_left_bound;
	}
	
	if (m_right_bound < m_min)
	{
		m_right_bound = m_min;
	}
	else if (m_right_bound > m_max)
	{
		m_right_bound = m_max;
	}
	else
	{
		m_right_bound = a_right_bound;
	}
	
	SetTabsRaw(TabLeft(), TabRight());
}

//====================================================================
void TSliderView::SetTabs(const off_t a_left, const off_t a_right)
//====================================================================
{
	SetTabsRaw(a_left - a_left % m_granularity, a_right - a_right % m_granularity);
	
	off_t new_left(TabLeft()), new_right(TabRight());
	
	if (new_left < new_right)
	{
		if (new_left % m_granularity != 0)
		{
			new_left += m_granularity;
			new_left = new_left - new_left % m_granularity;
		}
		
		if (new_right % m_granularity != 0)
		{
			new_right = new_right - new_right % m_granularity;
		}
		
		if (new_left != TabLeft()
			|| new_right != TabRight())
		{
			SetTabsRaw(new_left, new_right);
		}
	}
}

//====================================================================
bool TSliderView::AdjustValue(const off_t a_change)
//====================================================================
{
	if (a_change == 0)
	{
		return false;
	}
	
	off_t left(TabLeft()), right(TabRight());
	
	if (m_active_slider == k_left_slider)
	{
		SetTabs(TabLeft() + a_change, TabRight());
	}
	else if (m_active_slider == k_right_slider)
	{
		SetTabs(TabLeft(), TabRight() + a_change);
	}
	
	//printf("(%Lu, %Lu)\n", TabLeft(), TabRight());
	BWindow *win(Window());
	
	if (win != NULL)
	{
		DrawSlider();
		win->PostMessage(M_SLIDER);
	}
	
	return (left != TabLeft() || right != TabRight());
}

//====================================================================
void TSliderView::DrawSlider(void)
//====================================================================
{
	if (!m_off_bitmap->Lock())
	{
		return;
	}
	
	// Erase everything
	m_off_view->SetDrawingMode(B_OP_COPY);
	m_off_view->SetLowColor(ViewColor());
	BRect bounds(m_off_view->Bounds());
	m_off_view->SetHighColor(ViewColor());
	m_off_view->FillRect(bounds);
	
	// Draw the text
	if (IsEnabled())
	{
		m_off_view->SetHighColor(0x00, 0x00, 0x00);
	}
	else
	{
		m_off_view->SetHighColor(0x80, 0x80, 0x80);
	}
	
	char str[256];
	BFont font;
	GetFont(&font);
	m_off_view->SetLowColor(ViewColor());
	m_off_view->MovePenTo(bounds.left + 4, bounds.top + 10);
	sprintf(str, "%s", Label());
	m_off_view->DrawString(str);
	sprintf(str, "%Lu MB", (m_right - m_left) * BLOCK_SIZE / k_magnitude);
	m_off_view->MovePenTo(bounds.right - font.StringWidth(str) - 4, bounds.top + 10);
	m_off_view->DrawString(str);
	
	// Draw the shadows
	bounds.top += 13;
	bounds.bottom -= 6;
	bounds.left += 4;
	bounds.right -= 4;
	m_off_view->SetHighColor(0xB8, 0xB8, 0xB8);
	m_off_view->FillRect(bounds);
	m_off_view->SetHighColor(0xFF, 0xFF, 0xFF);
	m_off_view->StrokeLine(BPoint(bounds.left + 1, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	m_off_view->StrokeLine(BPoint(bounds.right, bounds.top), BPoint(bounds.right, bounds.bottom));
	m_off_view->SetHighColor(0x50, 0x50, 0x50);
	m_off_view->StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.right - 1, bounds.top));
	m_off_view->StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.left, bounds.bottom));
	
	// Draw the bar
	bounds.InsetBy(1, 1);
	int32 width(static_cast<int32>(bounds.Width()));
	off_t left(static_cast<off_t>(static_cast<long double>(m_left - m_min) / static_cast<long double>(m_max - m_min) * static_cast<long double>(width)));
	off_t right(static_cast<off_t>(static_cast<long double>(m_right - m_min) / static_cast<long double>(m_max - m_min) * static_cast<long double>(width)));
	bounds.right = bounds.left + right;
	bounds.left += left;
	
	if (IsEnabled())
	{
		if (IsDirty())
		{
			m_off_view->SetHighColor(0xCC, 0x20, 0x20);
		}
		else
		{
			m_off_view->SetHighColor(0x20, 0x20, 0xCC);
		}
	}
	else
	{
		if (IsDirty())
		{
			m_off_view->SetHighColor(0xCC, 0x60, 0x60);
		}
		else
		{
			m_off_view->SetHighColor(0x60, 0x60, 0xCC);
		}
	}
	
	m_off_view->FillRect(bounds);
	
	// Draw the tabs
	if (IsEnabled())
	{
		m_off_view->StrokeLine(BPoint(bounds.left, bounds.bottom - 1), BPoint(bounds.left, bounds.bottom - 1));
		m_off_view->StrokeLine(BPoint(bounds.left - 1, bounds.bottom), BPoint(bounds.left, bounds.bottom));
		m_off_view->StrokeLine(BPoint(bounds.left - 2, bounds.bottom + 1), BPoint(bounds.left, bounds.bottom + 1));
		m_off_view->StrokeLine(BPoint(bounds.left - 3, bounds.bottom + 2), BPoint(bounds.left, bounds.bottom + 2));
		
		m_off_view->StrokeLine(BPoint(bounds.right, bounds.bottom - 1), BPoint(bounds.right, bounds.bottom - 1));
		m_off_view->StrokeLine(BPoint(bounds.right, bounds.bottom), BPoint(bounds.right + 1, bounds.bottom));
		m_off_view->StrokeLine(BPoint(bounds.right, bounds.bottom + 1), BPoint(bounds.right + 2, bounds.bottom + 1));
		m_off_view->StrokeLine(BPoint(bounds.right, bounds.bottom + 2), BPoint(bounds.right + 3, bounds.bottom + 2));
		
		if (IsFocus()
			&& m_active_slider != k_no_slider)
		{
			int32 left_x(static_cast<int32>((m_active_slider == k_left_slider) ? bounds.left - 5 : bounds.right));
			m_off_view->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			m_off_view->StrokeLine(BPoint(left_x, bounds.bottom + 5), BPoint(left_x + 5, bounds.bottom + 5));
			m_off_view->SetHighColor(0xFF, 0xFF, 0xFF);
			m_off_view->StrokeLine(BPoint(left_x, bounds.bottom + 6), BPoint(left_x + 5, bounds.bottom + 6));
		}
		
		m_off_view->SetHighColor(0x50, 0x50, 0x50);
		m_off_view->StrokeLine(BPoint(bounds.left, bounds.bottom - 2), BPoint(bounds.left - 5, bounds.bottom + 3));
		m_off_view->StrokeLine(BPoint(bounds.left - 5, bounds.bottom + 3), BPoint(bounds.left, bounds.bottom + 3));
		m_off_view->StrokeLine(BPoint(bounds.left, bounds.bottom + 3), BPoint(bounds.left, bounds.bottom - 2));
		
		m_off_view->StrokeLine(BPoint(bounds.right, bounds.bottom - 2), BPoint(bounds.right, bounds.bottom + 3));
		m_off_view->StrokeLine(BPoint(bounds.right, bounds.bottom + 3), BPoint(bounds.right + 5, bounds.bottom + 3));
		m_off_view->StrokeLine(BPoint(bounds.right + 5, bounds.bottom + 3), BPoint(bounds.right, bounds.bottom - 2));
	}
	
	m_off_view->Sync();
	m_off_bitmap->Unlock();
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(m_off_bitmap, BPoint(k_lock_width + 1, 0));
}

//====================================================================
void TSliderView::SetTabsRaw(const off_t a_left, const off_t a_right)
//====================================================================
{
	off_t old_left(m_left), old_right(m_right);
	m_left = a_left;
	m_right = a_right;
	
	if (m_left < m_left_bound)
	{
		m_left = m_left_bound;
	}
	else if (m_left > m_right_bound)
	{
		m_left = m_right_bound;
	}
	
	if (m_right < m_left_bound)
	{
		m_right = m_left_bound;
	}
	else if (m_right > m_right_bound)
	{
		m_right = m_right_bound;
	}
	
	if (m_left > m_right)
	{
		if (m_left != old_left)
		{
			m_left = m_right;
		}
		else if (m_right != old_right)
		{
			m_right = m_left;
		}
	}
	
//	printf("(%Ld, %Ld), (%Ld, %Ld), (%Ld, %Ld)\n", m_min, m_max, m_left_bound, m_right_bound, m_left, m_right);
}

//====================================================================
TPartView::TPartView(const BRect a_frame, const char * const a_name, const off_t a_min, const off_t a_max, const off_t a_lefts[MAX_PARTITION], const off_t a_rights[MAX_PARTITION], const bool a_read_only, const off_t a_granularity, status_t * const a_err) :
//====================================================================
	Inherited(a_frame, a_name, B_FOLLOW_ALL, B_WILL_DRAW),
	m_status(B_NO_ERROR),
	m_min((a_min > a_max) ? a_max : a_min),
	m_max((a_min > a_max) ? a_min : a_max),
	m_layout_menu(NULL)
{
	SetViewColor(0xD8, 0xD8, 0xD8);
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		m_partitions[i] = NULL;
	}
	
	char str[256];
	BRect bounds(a_frame);
	bounds.OffsetTo(0, 0);
	bounds.bottom = bounds.top + 20;
	BMenu *menu(NULL);
	BMenuItem *item_100(NULL), *item_50(NULL), *item_33(NULL), *item_25(NULL);
	BMessage *msg_100(NULL), *msg_50(NULL), *msg_33(NULL), *msg_25(NULL);
	
	if ((menu = new BPopUpMenu("Layout")) == NULL
		|| (m_layout_menu = new BMenuField(bounds, "", "", menu)) == NULL
		|| (msg_100 = new BMessage(M_LAYOUT)) == NULL
		|| (msg_50 = new BMessage(M_LAYOUT)) == NULL
		|| (msg_33 = new BMessage(M_LAYOUT)) == NULL
		|| (msg_25 = new BMessage(M_LAYOUT)) == NULL)
	{
		delete menu;
		delete msg_100, msg_50, msg_33, msg_25;
		m_status = B_NO_MEMORY;
		
		// Give the error back to the caller
		if (a_err != NULL)
		{
			*a_err = Status();
		}
		
		return;
	}
	
	if ((item_100 = new BMenuItem(k_part_str_100, msg_100)) == NULL
		|| (item_50 = new BMenuItem(k_part_str_50, msg_50)) == NULL
		|| (item_33 = new BMenuItem(k_part_str_33, msg_33)) == NULL
		|| (item_25 = new BMenuItem(k_part_str_25, msg_25)) == NULL)
	{
		delete item_100, item_50, item_33, item_25;
		m_status = B_NO_MEMORY;
		
		// Give the error back to the caller
		if (a_err != NULL)
		{
			*a_err = Status();
		}
		
		return;
	}
	
	BFont font(*be_plain_font);
	font.SetSize(10.0);
	menu->SetFont(&font);
	menu->SetRadioMode(false);
	item_100->SetTarget(this);
	item_50->SetTarget(this);
	item_33->SetTarget(this);
	item_25->SetTarget(this);
	menu->AddItem(item_100);
	menu->AddItem(item_50);
	menu->AddItem(item_33);
	menu->AddItem(item_25);
	m_layout_menu->SetDivider(0);
	AddChild(m_layout_menu);
	
	bounds = a_frame;
	bounds.OffsetTo(0, m_layout_menu->Frame().bottom + 15);
	bounds.bottom = bounds.top + 30;
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		sprintf(str, "Partition %d", i + 1);
		
		if ((m_partitions[i] = new TSliderView(bounds, a_min, a_max, a_min, a_max, a_lefts[i], a_rights[i], a_read_only, a_granularity, str)) == NULL)
		{
			m_status = B_NO_MEMORY;
			break;
		}
		
		AddChild(m_partitions[i]);
		bounds.OffsetBy(0, bounds.Height() + 1);
		
		if ((m_status = m_partitions[i]->Status()) != B_NO_ERROR)
		{
			break;
		}
		
		if (a_lefts[i] != a_rights[i])
		{
			m_partitions[i]->SetSliderLock(true);
		}
	}
	
	if (Status() == B_NO_ERROR)
	{
		CalculateBounds();
		CheckLayoutMenu();
	}
	
	// Give the error back to the caller
	if (a_err != NULL)
	{
		*a_err = Status();
	}
}

//====================================================================
TPartView::~TPartView(void)
//====================================================================
{
	;
}

//====================================================================
void TPartView::CalculateBounds(void)
//====================================================================
{
	off_t left, right;
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		left = m_min;
		right = m_max;
		
		for (int32 j(0); j < MAX_PARTITION; j++)
		{
			if (j == i)
			{
				continue;
			}
			
			if (m_partitions[j]->TabLeft() < m_partitions[i]->TabLeft()
				&& m_partitions[j]->TabRight() > left)
			{
				left = m_partitions[j]->TabRight();
			}
			
			if (m_partitions[j]->TabRight() > m_partitions[i]->TabRight()
				&& m_partitions[j]->TabLeft() < right)
			{
				right = m_partitions[j]->TabLeft();
			}
			
			m_partitions[i]->SetTabBounds(left, right);
		}
	}
}

//====================================================================
void TPartView::CheckLayoutMenu(void)
//====================================================================
{
	bool layout_enabled(true);
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		if (layout_enabled
			&& !m_partitions[i]->IsEnabled())
		{
			layout_enabled = false;
			break;
		}
	}
	
	m_layout_menu->SetEnabled(layout_enabled);
}

//====================================================================
void TPartView::MessageReceived(BMessage * const a_msg)
//====================================================================
{
	if (a_msg->what != M_LAYOUT)
	{
		Inherited::MessageReceived(a_msg);
		return;
	}
	
	BMenuItem *menu_item;
	
	if (a_msg->FindPointer("source", reinterpret_cast<void **>(&menu_item)) != B_NO_ERROR)
	{
		return;
	}
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		m_partitions[i]->Clear();
	}
	
	int32 partitions(MAX_PARTITION);
	
	if (strcmp(menu_item->Label(), k_part_str_100) == 0)
	{
		partitions = 1;
	}
	else if (strcmp(menu_item->Label(), k_part_str_50) == 0)
	{
		partitions = 2;
	}
	else if (strcmp(menu_item->Label(), k_part_str_33) == 0)
	{
		partitions = 3;
	}
	else if (strcmp(menu_item->Label(), k_part_str_25) == 0)
	{
		partitions = 4;
	}
	
	off_t left, right(m_partitions[0]->TabBoundLeft()), width((m_partitions[0]->TabBoundRight() - m_partitions[0]->TabBoundLeft()) / partitions);
	
	for (int32 i(0); i < partitions; i++)
	{
		left = right;
		right = (i == partitions - 1) ? m_partitions[i]->TabBoundRight() : left + width;
		m_partitions[i]->SetTabs(left, right);
	}
	
	CalculateBounds();
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		m_partitions[i]->Invalidate();
	}
}

//====================================================================
void TPartView::Reset(void)
//====================================================================
{
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		m_partitions[i]->Reset();
	}
	
	CalculateBounds();
	
	/*for (int32 i(0); i < MAX_PARTITION; i++)
	{
		if (m_partitions[i]->TabLeft() != m_partitions[i]->TabRight())
		{
			m_partitions[i]->SetSliderLock(true);
		}
	}*/
	
	for (int32 i(0); i < MAX_PARTITION; i++)
	{
		m_partitions[i]->Invalidate();
	}
}


/***********************************************************************\
|*                     \                         /                     *|
|*                   -   E A T   A T   J O E ' S   -                   *|
|*                     /                         \                     *|
\***********************************************************************/

                      /*     In other words,      *\
                      \* here's all the old crap. */


bool ds_partition_id(uchar *sb, int32 size)
{
	return id_intel_partition(sb, size);
}

//--------------------------------------------------------------------

char *ds_partition_name(void)
{
	return "intel";
}

//--------------------------------------------------------------------

static void _set_partition_type_(partition_data *partition, uchar type)
{
	int			i;
	for (i = 0; ; i++) {
		if (types[i].id < 0) {
			sprintf(partition->partition_type, "INTEL 0x%.2x", type);
			partition->hidden = false;
			break;
		} else if (type == types[i].id) {
			sprintf(partition->partition_type, types[i].name);
			partition->hidden = types[i].hidden;
			break;
		}
	}
}

status_t ds_get_nth_map(int32 dev, uchar *sb, uint64 block_num, int32 block_size,
						int32 index, partition_data *partition)
{
	struct _partition_info_ pinfo;

	if (get_nth_intel_partition(dev, sb, block_num, index,
			&pinfo, NULL, NULL, NULL) != B_OK)
		return B_ERROR;

	partition->offset = pinfo.sblock;
	partition->blocks = pinfo.nblocks;
	partition->partition_code = pinfo.type;
	partition->logical_block_size = block_size;

	_set_partition_type_(partition, pinfo.type);
	
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

void ds_partition_flags(drive_setup_partition_flags *flags)
{
	flags->can_partition = true;
	flags->can_repartition = true;
}

//--------------------------------------------------------------------

status_t ds_update_map(int32 dev, int32 index, partition_data *partition)
{
	struct _partition_info_ ptinfo;
	fdisk *pfdisk;
	uint64 nblock_num, num_blocks;
	uchar block[512], oblock[512];
	device_geometry geometry;

	if (ioctl(dev, B_GET_GEOMETRY, &geometry) < 0)
		return B_ERROR;

	if (read_pos(dev, 0LL, block, 512) < 512)
		return B_ERROR;

	if (get_nth_intel_partition(dev, block, 0, index, 
			NULL, &ptinfo, oblock, &pfdisk) != B_OK)
		return B_ERROR;
	
	if (pfdisk->systid != partition->partition_code) {
		pfdisk->systid = partition->partition_code;
		if (write_pos(dev, geometry.bytes_per_sector * ptinfo.sblock, oblock,
				512) < 512)
			return B_ERROR;
		_set_partition_type_(partition, partition->partition_code);
	}

	return 0;
}

//--------------------------------------------------------------------

void ds_partition(BMessage *msg)
{
	BRect		r;
	BWindow		*wind;
	IntelPartWindow	*window;

	msg->FindPointer("window", (void **)&wind);
	r = wind->Frame();
	r.left += ((r.Width() - WIND_WIDTH) / 2);
	r.right = r.left + WIND_WIDTH;
	r.top += ((r.Height() - WIND_HEIGHT) / 2);
	r.bottom = r.top + WIND_HEIGHT;

	BScreen screen( wind );
	BRect screen_frame = screen.Frame();
	screen_frame.InsetBy(6, 6);
	if (r.right > screen_frame.right) {
		r.left -= r.right - screen_frame.right;
		r.right = screen_frame.right;
	}
	if (r.bottom > screen_frame.bottom) {
		r.top -= r.bottom - screen_frame.bottom;
		r.bottom = screen_frame.bottom;
	}
	if (r.left < screen_frame.left) {
		r.right += screen_frame.left - r.left;
		r.left = screen_frame.left;
	}
	if (r.top < screen_frame.top) {
		r.bottom += screen_frame.top - r.top;
		r.top = screen_frame.top;
	}
	window = new IntelPartWindow(r, msg);
}


//====================================================================

IntelPartWindow::IntelPartWindow(BRect rect, BMessage *msg)
				:BWindow(rect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	const char		*str;
	char			type[256];
	int32			result = B_ERROR;
	BBox			*box;
	BButton			*button;
	BFont			font;
	BRect			r, r1, r2, r3;
	BStringView		*string;
	device_geometry	geometry;

	fMessage = msg;
	if (msg->FindInt32("dev", &fDevice) == B_NO_ERROR) {
//		fReadOnly = false;
		msg->FindBool("read_only", &fReadOnly);
		msg->FindInt64("offset", (int64 *)&fOffset);
		ioctl(fDevice, B_GET_GEOMETRY, &geometry);
		fBlockSize = geometry.bytes_per_sector;
		
		if (fBlockSize != 512)
		{
			(new BAlert("", "Error in drive block size passed to Intel partition add-on.", "Sorry"))->Go();
			Run();
			PostMessage(new BMessage(M_CANCEL));
//			Quit();
			return;
		}
		
		msg->FindInt64("blocks", (int64 *)&fSectors);
		fSize = fSectors * fBlockSize;
		if ((result = read_pos(fDevice, fOffset * fBlockSize, &fSB,
				sizeof(bootsector))) == sizeof(bootsector))
		{
			if (ds_partition_id(reinterpret_cast<uchar *>(&fSB), 512) == false)
			{
				result = B_ERROR;
			}
		}
	}
	else {
		(new BAlert("", "Error in message data passed to Intel add-on.", "Sorry"))->Go();
		Run();
		PostMessage(new BMessage(M_CANCEL));
//					Quit();
		return;
	}

	if (result < 0)
		memcpy(&fSB, intel_part_map, sizeof(bootsector));

	r = Frame();
	r.OffsetTo(0, 0);
	r.InsetBy(-1, -1);
	r.top--;
	AddChild(box = new IntelBox(r));

	fMessage->FindString("device", &str);
	r.Set(LABEL_H, LABEL_V, LABEL_H + LABEL_WIDTH, LABEL_V + 16);
	sprintf(type, "%s - %s", str, LABEL_TEXT);
	string = new BStringView(r, "", type);
	box->AddChild(string);
	font = *be_bold_font;
	font.SetSize(12.0);
	string->SetFont(&font);

	r.Set(PART1_H, PART1_V, PART1_H + PART1_WIDTH, PART1_V + 16);
	r1.Set(PART1_TYPE_H, PART1_TYPE_V,
		   PART1_TYPE_H + PART1_TYPE_WIDTH, PART1_TYPE_V + 16);
	r2.Set(PART1_MENU_H, PART1_MENU_V,
		   PART1_MENU_H + PART1_MENU_WIDTH, PART1_MENU_V + 16);
	r3.Set(PART1_RADIO_H, PART1_RADIO_V,
		   PART1_RADIO_H + PART1_RADIO_WIDTH, PART1_RADIO_V + 16);
	AddPartItem(0, box, r, r1, r2, r3);

	r.Set(PART2_H, PART2_V, PART2_H + PART2_WIDTH, PART2_V + 16);
	r1.Set(PART2_TYPE_H, PART2_TYPE_V,
		   PART2_TYPE_H + PART2_TYPE_WIDTH, PART2_TYPE_V + 16);
	r2.Set(PART2_MENU_H, PART2_MENU_V,
		   PART2_MENU_H + PART2_MENU_WIDTH, PART2_MENU_V + 16);
	r3.Set(PART2_RADIO_H, PART2_RADIO_V,
		   PART2_RADIO_H + PART2_RADIO_WIDTH, PART2_RADIO_V + 16);
	AddPartItem(1, box, r, r1, r2, r3);

	r.Set(PART3_H, PART3_V, PART3_H + PART3_WIDTH, PART3_V + 16);
	r1.Set(PART3_TYPE_H, PART3_TYPE_V,
		   PART3_TYPE_H + PART3_TYPE_WIDTH, PART3_TYPE_V + 16);
	r2.Set(PART3_MENU_H, PART3_MENU_V,
		   PART3_MENU_H + PART3_MENU_WIDTH, PART3_MENU_V + 16);
	r3.Set(PART3_RADIO_H, PART3_RADIO_V,
		   PART3_RADIO_H + PART3_RADIO_WIDTH, PART3_RADIO_V + 16);
	AddPartItem(2, box, r, r1, r2, r3);

	r.Set(PART4_H, PART4_V, PART4_H + PART4_WIDTH, PART4_V + 16);
	r1.Set(PART4_TYPE_H, PART4_TYPE_V,
		   PART4_TYPE_H + PART4_TYPE_WIDTH, PART4_TYPE_V + 16);
	r2.Set(PART4_MENU_H, PART4_MENU_V,
		   PART4_MENU_H + PART4_MENU_WIDTH, PART4_MENU_V + 16);
	r3.Set(PART4_RADIO_H, PART4_RADIO_V,
		   PART4_RADIO_H + PART4_RADIO_WIDTH, PART4_RADIO_V + 16);
	AddPartItem(3, box, r, r1, r2, r3);

	r.Set(SLIDER_H, SLIDER_V, SLIDER_H + SLIDER_WIDTH, SLIDER_V + SLIDER_HEIGHT);
	
	device_geometry offset_geometry;
	off_t offset(63);
	
	if (ioctl(fDevice, B_GET_BIOS_GEOMETRY, &offset_geometry) == B_NO_ERROR)
	{
		offset = offset_geometry.sectors_per_track;
	}
	
	off_t lefts[MAX_PARTITION] =
	{
		(fSB.part[0].systid != 0) ? swap32(fSB.part[0].relsect) : fSectors,
		(fSB.part[1].systid != 0) ? swap32(fSB.part[1].relsect) : fSectors,
		(fSB.part[2].systid != 0) ? swap32(fSB.part[2].relsect) : fSectors,
		(fSB.part[3].systid != 0) ? swap32(fSB.part[3].relsect) : fSectors
	};
	
	off_t rights[MAX_PARTITION] =
	{
		(fSB.part[0].systid != 0) ? swap32(fSB.part[0].relsect) + swap32(fSB.part[0].numsect) : fSectors,
		(fSB.part[1].systid != 0) ? swap32(fSB.part[1].relsect) + swap32(fSB.part[1].numsect) : fSectors,
		(fSB.part[2].systid != 0) ? swap32(fSB.part[2].relsect) + swap32(fSB.part[2].numsect) : fSectors,
		(fSB.part[3].systid != 0) ? swap32(fSB.part[3].relsect) + swap32(fSB.part[3].numsect) : fSectors
	};
	
//	for (off_t i(0);
//		i < 4;
//		i++)
//	{
//		printf("lefts[%Ld] = %Ld, rights[%Ld] = %Ld\n", i, lefts[i], i, rights[i]);
//	}
	
	//printf("(fSectors: %Lu)\n", fSectors);
	fPartView = new TPartView(r, NULL, offset, fSectors, lefts, rights, fReadOnly, offset_geometry.head_count * offset_geometry.sectors_per_track);
	box->AddChild(fPartView);

	r.Set(BUTTON_PARTITION_H, BUTTON_PARTITION_V,
		  BUTTON_PARTITION_H + BUTTON_WIDTH, BUTTON_PARTITION_V + BUTTON_HEIGHT);
	button = new BButton(r, "", BUTTON_PARTITION_TEXT, new BMessage(M_OK));
	button->MakeDefault(true);
	//button->SetEnabled(!fReadOnly);
	box->AddChild(button);

	r.Set(BUTTON_CANCEL_H, BUTTON_CANCEL_V,
		  BUTTON_CANCEL_H + BUTTON_WIDTH, BUTTON_CANCEL_V + BUTTON_HEIGHT);
	box->AddChild(new BButton(r, "", BUTTON_CANCEL_TEXT, new BMessage(M_CANCEL)));

	r.Set(BUTTON_REVERT_H, BUTTON_REVERT_V,
		  BUTTON_REVERT_H + BUTTON_WIDTH, BUTTON_REVERT_V + BUTTON_HEIGHT);
	box->AddChild(fRevert = new BButton(r, "", BUTTON_REVERT_TEXT, new BMessage(M_REVERT)));
	fRevert->SetEnabled(false);
	Show();
}

//--------------------------------------------------------------------

void IntelPartWindow::MessageReceived(BMessage *msg)
{
	bool			chs = false;
	bool			check = false;
	bool			enable = false;
	bool			result = false;
	char			label[256];
	int32			cyl;
	int32			sec;
	int32			head;
	int32			side;
	uint32			tmp;
	int32			loop;
	int32			value;
	uint32			pos;
	BWindow			*wind;
	device_geometry	geometry;
	TSliderView *partition(NULL);
	BCheckBox *box(NULL);
	ssize_t box_bytes(sizeof (box));
	
	switch (msg->what) {
		case M_OK:
			for (loop = 0; (partition = fPartView->PartitionView(loop)) != NULL; loop++)
			{
				if (atol(fType[loop]->Text()) != fSB.part[loop].systid
					|| fActive[loop]->Value() != (fSB.part[loop].bootid & ACTIVE_PARTITION)
					|| partition->IsDirty())
				{
					enable = true;
					break;
				}
			}

			if (enable) {
				if ((new BAlert("", "Changing the partition map may "\
"destroy all data on this disk.", "Proceed", "Cancel"))->Go())
					return;
				if (ioctl(fDevice, B_GET_BIOS_GEOMETRY, &geometry) == B_NO_ERROR) {
		//printf("\n  Heads: %d\n", geometry.head_count);
		//printf("  Sectors: %d\n", geometry.sectors_per_track);
		//printf("Cylinders: %d\n", geometry.cylinder_count);
					chs = true;
				}
				
				for (loop = 0; (partition = fPartView->PartitionView(loop)) != NULL; loop++)
				{
					fSB.part[loop].bootid = (fActive[loop]->Value()) ? ACTIVE_PARTITION : 0;
					fSB.part[loop].systid = atol(fType[loop]->Text());
					
					if (partition->IsDirty())
					{
						if (chs) {
							if (partition->TabRight() > partition->TabLeft())
							{
								cyl = partition->TabLeft() / (geometry.head_count * geometry.sectors_per_track);
								tmp = partition->TabLeft() % (geometry.head_count * geometry.sectors_per_track);
								head = tmp / geometry.sectors_per_track;
								sec = tmp % geometry.sectors_per_track + 1;
							}
							else
								cyl = head = sec = 0;
							if (cyl > 1023) {
								fSB.part[loop].beghd = 0xff;
								fSB.part[loop].begsec = 0xff;
								fSB.part[loop].begcyl = 0xff;
							} else {
								fSB.part[loop].beghd = head;
								fSB.part[loop].begsec = ((cyl & 0x300) >> 2) | (sec & 0x3f);
								fSB.part[loop].begcyl = cyl & 0xff;
							}
	
				//printf("Start Head %d, sector %d, cylinder %d\n", head, sec, cyl);
							if (partition->TabRight() > partition->TabLeft()) {
								cyl = (partition->TabRight() - 1) / (geometry.head_count * geometry.sectors_per_track);
								tmp = (partition->TabRight() - 1) % (geometry.head_count * geometry.sectors_per_track);
								head = tmp / geometry.sectors_per_track;
								sec = tmp % geometry.sectors_per_track + 1;
							}
							else
								cyl = head = sec = 0;
							if (cyl > 1023) {
								fSB.part[loop].endhd = 0xff;
								fSB.part[loop].endsec = 0xff;
								fSB.part[loop].endcyl = 0xff;
							} else {
								fSB.part[loop].endhd = head;
								fSB.part[loop].endsec = ((cyl & 0x300) >> 2) | (sec & 0x3f);
								fSB.part[loop].endcyl = cyl & 0xff;
							}
				//printf("End  Head %d, sector %d, cylinder %d\n", head, sec, cyl);
						}
						else {
							fSB.part[loop].beghd = 0;
							fSB.part[loop].begsec = 1;
							fSB.part[loop].begcyl = 0;
							fSB.part[loop].endhd = 0;
							fSB.part[loop].endsec = 1;
							fSB.part[loop].endcyl = 0;
						}
						
						pos = partition->TabLeft();
						pos = swap32(reinterpret_cast<uchar *>(&pos));
						memcpy(&fSB.part[loop].relsect, &pos, sizeof(int32));
						pos = partition->TabRight() - partition->TabLeft();
						pos = swap32(reinterpret_cast<uchar *>(&pos));
						memcpy(&fSB.part[loop].numsect, &pos, sizeof(int32));
						
						if (fSB.part[loop].systid == 0
							|| partition->TabRight() <= partition->TabLeft())
						{
							memset(&fSB.part[loop], 0, sizeof(fdisk));
						}
					}
				}
				// Some BIOS'es require partition maps to have at least one
				// partition marked active or they'll fail to boot, regardless
				// of any installed boot managers. If the user didn't mark any
				// partitions active, we'll mark an appropriate one for them.
				int32 i;
				for (i=0;i<4;i++)
					if (fSB.part[i].bootid & ACTIVE_PARTITION)
						break;
				if (i == 4) {
					for (i=0;i<4;i++) {
						if (	(fSB.part[i].systid == BEOS_PARTITION) &&
								(*(uint32 *)fSB.part[i].numsect)) {
							fSB.part[i].bootid = ACTIVE_PARTITION;
							break;
						}
					}
				}
				if (i == 4) {
					for (i=0;i<4;i++) {
						if (	(fSB.part[i].systid) &&
								(fSB.part[i].systid != EXTENDED_PARTITION1) &&
								(fSB.part[i].systid != EXTENDED_PARTITION2) &&
								(fSB.part[i].systid != EXTENDED_PARTITION3) &&
								(*(uint32 *)fSB.part[i].numsect)) {
							fSB.part[i].bootid = ACTIVE_PARTITION;
							break;
						}
					}
				}
				if (i == 4) {
					for (i=0;i<4;i++) {
						if (	(fSB.part[i].systid) &&
								(*(uint32 *)fSB.part[i].numsect)) {
							fSB.part[i].bootid = ACTIVE_PARTITION;
							break;
						}
					}
				}
				value = write_pos(fDevice, fOffset * fBlockSize, &fSB,
						sizeof(bootsector));
				if (value < 0) {
					(new BAlert("", "Error writing partition map to disk.",
								"Bummer"))->Go();
					return;
				}
				else
					result = true;
			}
			// fall through
		case M_CANCEL:
			fMessage->FindPointer("window", (void **)&wind);
			fMessage->AddPointer("part_window", this);
			fMessage->AddInt32("part_looper_thread", Thread());
			fMessage->AddBool("result", result);
			if (wind != NULL) {
				wind->PostMessage(fMessage);
			}
			delete fMessage;
			Quit();
			break;

		case M_REVERT:
			for (loop = 0; loop < 4; loop++)
			{
				sprintf(label, "%d", fSB.part[loop].systid);
				fType[loop]->SetText(label);
				AdjustMenu(fMenu[loop], fSB.part[loop].systid);
				fActive[loop]->SetValue(fSB.part[loop].bootid & ACTIVE_PARTITION);
			}
			
			fPartView->Reset();
			check = true;
			break;

		case M_TEXT:
			msg->FindInt32("index", &loop);
			AdjustMenu(fMenu[loop], atol(fType[loop]->Text()));
			check = true;
			break;

		case M_RADIO:
			if (msg->FindPointer("source", reinterpret_cast<void **>(&box)) == B_NO_ERROR
				&& box != NULL)
			{
				for (int32 i(0); i < MAX_PARTITION; i++)
				{
					if (fActive[i] != box)
					{
						fActive[i]->SetValue(B_CONTROL_OFF);
					}
				}
			}
			
			check = true;
			break;

		case M_MENU:
			msg->FindInt32("index", &loop);
			msg->FindInt32("value", &value);
			sprintf(label, "%d", value);
			fType[loop]->SetText(label);
			check = true;
			break;

		case M_SLIDER:
			check = true;
			break;

		case M_LAYOUT:
			PostMessage(msg, fPartView);
			check = true;
			break;

		default:
			BWindow::MessageReceived(msg);
	}
	
	if (check)
	{
		TSliderView *partition(NULL);
		
		for (loop = 0; (partition = fPartView->PartitionView(loop)) != NULL; loop++)
		{
			if (atol(fType[loop]->Text()) != fSB.part[loop].systid
				|| fActive[loop]->Value() != (fSB.part[loop].bootid & ACTIVE_PARTITION)
				|| partition->IsDirty())
			{
				enable = true;
				break;
			}
		}
		
		fRevert->SetEnabled(enable);
	}
}

//--------------------------------------------------------------------

void IntelPartWindow::AdjustMenu(BMenu *menu, int32 value)
{
	int32		i;
	BMenuItem	*item;

	for (i = 0; types[i].id >= 0; i++) {
		item = menu->ItemAt(i);
		item->SetMarked(value == types[i].id);
	}
	item = menu->ItemAt(i);
	item->SetMarked(!(menu->FindMarked()));
}

//--------------------------------------------------------------------

void IntelPartWindow::AddPartItem(short index, BView *view, BRect label_rect,
															BRect text_rect,
															BRect menu_rect,
															BRect radio_rect)
{
	char			label[256];
	int32			i;
	BFont			font;
	BMenuField		*menu;
	BMenuItem		*item;
	BMessage		*msg;
	BStringView		*string;

	font = *be_plain_font;

	sprintf(label, "%s %d", PART_TEXT, index + 1);
	string = new BStringView(label_rect, "", label);
	view->AddChild(string);
	font.SetSize(12.0);
	string->SetFont(&font);

	font.SetSize(10.0);
	sprintf(label, "%d", fSB.part[index].systid);
	msg = new BMessage(M_TEXT);
	msg->AddInt32("index", index);
	view->AddChild(fType[index] = new BTextControl(text_rect, "", PART_TYPE_TEXT,
												   label, msg));
	fType[index]->SetFont(&font);
	fType[index]->SetEnabled(!fReadOnly);

	fMenu[index] = new BPopUpMenu("");
	fMenu[index]->SetFont(&font);
	for (i = 0; types[i].id >= 0; i++) {
		msg = new BMessage(M_MENU);
		msg->AddInt32("index", index);
		msg->AddInt32("value", types[i].id);
		fMenu[index]->AddItem(new BMenuItem(types[i].name, msg));
	}
	msg = new BMessage(M_MENU);
	msg->AddInt32("index", index);
	fMenu[index]->AddItem(item = new BMenuItem("Other", msg));
	item->SetEnabled(false);
	menu = new BMenuField(menu_rect, "", "", fMenu[index]);
	menu->SetDivider(0.0);
	menu->SetFont(&font);
	menu->MenuBar()->SetFont(&font);
	view->AddChild(menu);
	AdjustMenu(fMenu[index], fSB.part[index].systid);
	menu->SetEnabled(!fReadOnly);

	view->AddChild(fActive[index] = new BCheckBox(radio_rect, "", PART_RADIO_TEXT,
													 new BMessage(M_RADIO)));
	fActive[index]->SetFont(&font);
	if (fSB.part[index].bootid & ACTIVE_PARTITION)
		fActive[index]->SetValue(1);
	//fActive[index]->SetEnabled(!fReadOnly);
}


//====================================================================

IntelBox::IntelBox(BRect rect)
		 :BBox(rect, "", B_FOLLOW_ALL, B_WILL_DRAW, B_FANCY_BORDER)
{
}

//--------------------------------------------------------------------

void IntelBox::Draw(BRect where)
{
	BBox::Draw(where);

	SetHighColor(VIEW_COLOR - 48, VIEW_COLOR - 48, VIEW_COLOR - 48);
	StrokeLine(BPoint(H_LINE_H, LINE1_V),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE1_V));
	StrokeLine(BPoint(LINE2_H, LINE2_V),
			   BPoint(LINE2_H, LINE2_V + LINE2_HEIGHT));
	StrokeLine(BPoint(H_LINE_H, LINE3_V),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE3_V));
	StrokeLine(BPoint(H_LINE_H, LINE4_V),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE4_V));
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(H_LINE_H, LINE1_V + 1),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE1_V + 1));
	StrokeLine(BPoint(LINE2_H + 1, LINE2_V),
			   BPoint(LINE2_H + 1, LINE2_V + LINE2_HEIGHT));
	StrokeLine(BPoint(H_LINE_H, LINE3_V + 1),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE3_V + 1));
	StrokeLine(BPoint(H_LINE_H, LINE4_V + 1),
			   BPoint(H_LINE_H + H_LINE_WIDTH, LINE4_V + 1));

	SetHighColor(0, 0, 0);
}


//====================================================================

static uint32 swap32(uchar data[4])
{
	return data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
}

static uint32 swap16(uchar data[2])
{
	return data[1] << 8 | data[0];
}

/* support code for libpartition.a */
extern "C" {
	int dprintf(const char *format, ...);
}

int dprintf(const char *format, ...)
{
	int r;
	va_list args;
	va_start(args, format);
	r = vprintf(format, args);
	va_end(args);
	return r;
}

