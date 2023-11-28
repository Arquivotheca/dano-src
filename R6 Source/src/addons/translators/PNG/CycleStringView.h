/*--------------------------------------------------------------------*\
  File:      CycleStringView.h
  Creator:   Matt Bogosian <mbogosian@usa.net>
  Copyright: (c)1998, Matt Bogosian. All rights reserved.
  Description: Header file describing the CycleStringView class.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "data_array.h"

#include <StringView.h>


#ifndef SWAP_STRING_VIEW_H
#define SWAP_STRING_VIEW_H


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#ifndef DECL_SPEC
#if defined __BEOS__ && defined EXPORT_SYMBOLS
#define DECL_SPEC _EXPORT
#elif defined __BEOS__ && defined IMPORT_SYMBOLS
#define DECL_SPEC _IMPORT
#else
#define DECL_SPEC
#endif
#endif


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Structs, Classes =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Class name:       CycleStringView
  Inherits from:    public BStringView
  New data members: private status_t m_status - the status of the
                        object.
                    private size_t m_which_str - the current string
                        which is being displayed.
                    private const char * const *m_strs - the array of
                        strings through which to cycle.
  Description: Class to implement a string view that cycles through a
      list of strings when a mouse-down message is received.
\*--------------------------------------------------------------------*/

class DECL_SPEC CycleStringView : public BStringView
{
	public:
	
		// Public static member functions
		
/*--------------------------------------------------------------------*\
  Function name: static Instantiate
  Member of:     public CycleStringView
  Defined in:    CycleStringView.h
  Arguments:     BMessage * const a_archv - the archived view.
  Returns:       CycleStringView * - the new configuration view on
                     success, NULL on failure.
  Throws:        none
  Description: Function to construct a configuration view from an
      archive. Note: this view has been allocated with new and should
      be deleted when no longer needed.
\*--------------------------------------------------------------------*/
		
		static CycleStringView *Instantiate(BMessage * const a_archv);
		
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: CycleStringView
  Member of:     public CycleStringView
  Defined in:    CycleStringView.h
  Arguments:     const BRect a_frame - the view's frame.
                 const char * const a_name - the view's name.
                 const char * const * const a_text - a null-terminated
                     array of strings through which the view will
                     cycle (these strings are copied).
                 const uint32 a_resize_mode - the view's resizing
                     mode. Default: B_FOLLOW_LEFT | B_FOLLOW_TOP.
                 const uint32 a_flags - the view's flags. Default:
                     B_WILL_DRAW.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		CycleStringView(const BRect a_frame, const char * const a_name, const char * const * const a_text, const uint32 a_resize_mode = B_FOLLOW_LEFT | B_FOLLOW_TOP, const uint32 a_flags = B_WILL_DRAW, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: CycleStringView
  Member of:     public CycleStringView
  Defined in:    CycleStringView.h
  Arguments:     const BRect a_frame - the view's frame.
                 const char * const a_name - the view's name.
                 const char * const a_text1 - the first of up to three
                     strings through which the view will cycle.
                 const char * const a_text2 - the second of up to
                     three strings through which the view will cycle.
                     Default: NULL.
                 const char * const a_text3 - the third of up to three
                     strings through which the view will cycle.
                     Default: NULL.
                 const uint32 a_resize_mode - the view's resizing
                     mode. Default: B_FOLLOW_LEFT | B_FOLLOW_TOP.
                 const uint32 a_flags - the view's flags. Default:
                     B_WILL_DRAW.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Default class constructor. You must check the value of
      a_err or the return value of Status() after invoking the
      constructor to see if the object is usable. See the Status()
      function (below) for possible error values.
\*--------------------------------------------------------------------*/
		
		CycleStringView(const BRect a_frame, const char * const a_name, const char * const a_text1, const char * const a_text2 = NULL, const char * const a_text3 = NULL, const uint32 a_resize_mode = B_FOLLOW_LEFT | B_FOLLOW_TOP, const uint32 a_flags = B_WILL_DRAW, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: CycleStringView
  Member of:     public CycleStringView
  Defined in:    CycleStringView.h
  Arguments:     BMessage * const a_archv - the archived view.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		CycleStringView(BMessage * const a_archv, status_t * const a_err);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~CycleStringView
  Member of:     public CycleStringView
  Defined in:    CycleStringView.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~CycleStringView(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Clear
  Member of:     public CycleStringView
  Defined in:    CycleStringView.cpp
  Arguments:     BMessage * const a_archv - the message into which to
                     archive the object.
                 const bool a_deep - false if the archiving should
                     be superficial (i.e., only include this object's
                     face-level items), true if the archive should
                     contain "owned" objects, etc. Default: true.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to archive the object into a given message.
\*--------------------------------------------------------------------*/
		
		virtual status_t Archive(BMessage * const a_archv, const bool a_deep = true) const;
		
/*--------------------------------------------------------------------*\
  Function name: virtual Clear
  Member of:     public CycleStringView
  Defined in:    CycleStringView.cpp
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to reset the object.
\*--------------------------------------------------------------------*/
		
		virtual void Clear(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual MouseDown
  Member of:     public CycleStringView
  Defined in:    CycleStringView.cpp
  Arguments:     const BPoint a_point - the point at which the mouse-
                     down occurred.
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void MouseDown(const BPoint a_point);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Status const
  Member of:     public CycleStringView
  Defined in:    CycleStringView.cpp
  Arguments:     status_t * const a_err - a placeholder for the error
                     code (this will be the same value that is
                     returned). Default: NULL.
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
      B_NO_MEMORY - on construction, there was not enough memory
          available to create the object.
      B_BAD_VALUE - on construction, there was an error in copying the
          data (e.g., from an object whose Status() was not
          B_NO_ERROR).
\*--------------------------------------------------------------------*/
		
		virtual status_t Status(status_t * const a_err = NULL) const;
	
	private:
	
		typedef BStringView Inherited;
		
		// Private data members
		status_t m_status;
		size_t m_which_str;
		const char * const *m_strs;
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: Init
  Member of:     private CycleStringView
  Defined in:    CycleStringView.h
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to initialize a new object.
\*--------------------------------------------------------------------*/
		
		void Init(void);
		
		// Private prohibitted member functions
		CycleStringView(const CycleStringView &a_obj);
		CycleStringView &operator=(const CycleStringView &a_obj);
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline CycleStringView *CycleStringView::Instantiate(BMessage * const a_archv)
//====================================================================
{
	status_t err;
	CycleStringView *view;
	
	if ((view = new CycleStringView(a_archv, &err)) == NULL
		|| err != B_NO_ERROR)
	{
		delete view;
		view = NULL;
	}
	
	return view;
}

//====================================================================
inline CycleStringView::CycleStringView(const BRect a_frame, const char * const a_name, const char * const * const a_text, const uint32 a_resize_mode, const uint32 a_flags, status_t * const a_err) :
//====================================================================
	Inherited(a_frame, a_name, a_text[0], a_resize_mode, a_flags)
{
	Init();
	
	if (a_text == NULL
		|| a_text[0] == NULL)
	{
		m_status = B_BAD_VALUE;
	}
	else if ((m_strs = replicateArguments(arrayLength(reinterpret_cast<const void * const *>(a_text)), a_text)) == NULL)
	{
		m_status = B_NO_MEMORY;
	}
	
	Status(a_err);
}

//====================================================================
inline CycleStringView::CycleStringView(const BRect a_frame, const char * const a_name, const char * const a_text1, const char * const a_text2, const char * const a_text3, const uint32 a_resize_mode, const uint32 a_flags, status_t * const a_err) :
//====================================================================
	Inherited(a_frame, a_name, a_text1, a_resize_mode, a_flags)
{
	Init();
	const char *strs[4];
	strs[0] = a_text1;
	strs[1] = a_text2;
	strs[2] = a_text3;
	strs[3] = NULL;
	
	if (strs[0] == NULL)
	{
		m_status = B_BAD_VALUE;
	}
	else if ((m_strs = replicateArguments(arrayLength(reinterpret_cast<const void * const *>(strs)), strs)) == NULL)
	{
		m_status = B_NO_MEMORY;
	}
	
	Status(a_err);
}

//====================================================================
inline CycleStringView::CycleStringView(BMessage * const a_archv, status_t * const a_err) :
//====================================================================
	Inherited(a_archv)
{
	Init();
	Status(a_err);
}

//====================================================================
inline void CycleStringView::Init(void)
//====================================================================
{
	m_status = B_NO_ERROR;
	m_which_str = 0;
	m_strs = NULL;
}


#endif    // SWAP_STRING_VIEW_H
