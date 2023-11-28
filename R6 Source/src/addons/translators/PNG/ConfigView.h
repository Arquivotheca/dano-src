/*--------------------------------------------------------------------*\
  File:      ConfigView.h
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Header file the configuration classes for the PNG
      image translator add-on.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H

#include <Message.h>
#include <View.h>
#include <stdio.h>

/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Structs, Classes =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

class BMenuField;

/*--------------------------------------------------------------------*\
  Class name:       ConfigView
  Inherits from:    public BView
  New data members: private static const uint32 mk_msg_intrlcng - the
                        interlacing message type.
                    private static const KeyValue mk_vals_intrlcng[] -
                        the interlacing interface key values.
                    private static const float mk_min_height - the
                        minimum height of a view in the interface.
                    private static const float mk_spacer - a spacer
                        for the views in the interface.
                    private static const char * const mk_name - the
                        name of the configuration view.
                    private static const char * const mk_long_vers -
                        the name of the view which contains the
                        version information.
                    private static const char * const mk_name_views[]
                        - the names of the views in the interface.
                    private static const char *
                        const mk_name_menu_fields[] - the names of the
                        menu fields in the interface.
                    private status_t m_status - the status of the
                        object.
                    private BMessage m_config_msg - .
  Description: Class to implement a view in which a constant string is
      constantly displayed.
\*--------------------------------------------------------------------*/

class ConfigView : public BView
{
	public:
	
		// Public static const data members
		static const char * const mk_name_config_view;
		
		// Public static member functions
		
/*--------------------------------------------------------------------*\
  Function name: static Instantiate
  Member of:     public ConfigView
  Defined in:    ConfigView.h
  Arguments:     BMessage * const a_archv - the archived view.
  Returns:       ConfigView * - the new configuration view on
                     success, NULL on failure.
  Throws:        none
  Description: Function to construct a configuration view from an
      archive. Note: this view has been allocated with new and should
      be deleted when no longer needed.
\*--------------------------------------------------------------------*/
		
		static ConfigView *Instantiate(BMessage * const a_archv);
		
		// Public member functions
		
/*--------------------------------------------------------------------*\
  Function name: ConfigView
  Member of:     public ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     const BRect a_frame - the frame of the view.
                 status_t * const a_err - a placeholder for the error
                     encountered during construction (see below).
                     Default: NULL.
  Throws:        none
  Description: Class constructor. You must check the value of a_err or
      the return value of Status() after invoking the constructor to
      see if the object is usable. See the Status() function (below)
      for possible error values.
\*--------------------------------------------------------------------*/
		
		ConfigView(const BRect a_frame, status_t * const a_err = NULL);
		
/*--------------------------------------------------------------------*\
  Function name: ConfigView
  Member of:     public ConfigView
  Defined in:    ConfigView.h
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
		
		ConfigView(BMessage * const a_archv, status_t * const a_err);
		
/*--------------------------------------------------------------------*\
  Function name: virtual ~ConfigView
  Member of:     public ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     none
  Throws:        none
  Description: Class destructor.
\*--------------------------------------------------------------------*/
		
		virtual ~ConfigView(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual AttachedToWindow
  Member of:     public ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void AttachedToWindow(void);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Draw
  Member of:     public ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     BRect a_update_rect - the rect to be updated.
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void Draw(BRect a_update_rect);
		
/*--------------------------------------------------------------------*\
  Function name: virtual MessageReceived
  Member of:     public ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     BMessage * const a_msg - the message received.
  Returns:       none
  Throws:        none
  Description: Derived hook function.
\*--------------------------------------------------------------------*/
		
		virtual void MessageReceived(BMessage * const a_msg);
		
/*--------------------------------------------------------------------*\
  Function name: virtual Status const
  Member of:     public ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     status_t * const a_err - a placeholder for the error
                     code (this will be the same value that is
                     returned). Default: NULL.
  Returns:       status_t - B_NO_ERROR if the object is valid, another
                     error code if not (see below).
  Throws:        none
  Description: Function to check if the object is valid. This function
      must be called after creating the object to insure that its
      instantiation was successful. Possible return values are as
      follows:
      
      B_NO_ERROR - the object was created successfully.
\*--------------------------------------------------------------------*/
		
		virtual status_t Status(status_t * const a_err = NULL) const;
	
	private:
	
		typedef BView Inherited;
		
		struct KeyValue
		{
			char *m_key;
			int32 m_val;
		};
		
		// Private static const data members
		static const uint32 mk_msg_intrlcng;
		static const KeyValue mk_vals_intrlcng[];
		static const float mk_min_height;
		static const float mk_spacer;
		static const char * const mk_name;
		static const char * const mk_long_vers;
		static const char * const mk_name_views[];
		static const char * const mk_name_menu_fields[];
		
		// Private data members
		status_t m_status;
		BMessage m_config_msg;
		
		// Private member functions
		
/*--------------------------------------------------------------------*\
  Function name: Init
  Member of:     private ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     none
  Returns:       none
  Throws:        none
  Description: Function to initialize a new object.
\*--------------------------------------------------------------------*/
		
		void Init(void);
		
/*--------------------------------------------------------------------*\
  Function name: CreateMenuField
  Member of:     private ConfigView
  Defined in:    ConfigView.cpp
  Arguments:     const char * const a_name - the name of the menu
                     field.
                 const uint32 a_msg_type - the type of message which
                     should be sent when a menu item is triggered.
                 const KeyValue * const a_vals - the names and values
                     of the menu item.
                 const int32 a_dflt_val - the value of the menu item
                     which should be set initially.
  Returns:       BMenuField * - the menu field on success, NULL on
                     failure.
  Throws:        none
  Description: Function to create a radio-mode menu field from a set
      of given parameters.
\*--------------------------------------------------------------------*/
		
		BMenuField *CreateMenuField(const char * const a_name, const uint32 a_msg_type, const KeyValue * const a_vals, const int32 a_dflt_val, int top);
		
		// Private prohibitted member functions
		ConfigView(const ConfigView &a_obj);
		ConfigView &operator=(const ConfigView &a_obj);
};


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline ConfigView *ConfigView::Instantiate(BMessage * const a_archive)
//====================================================================
{
	status_t err;
	ConfigView *config_view;
	
	if ((config_view = new ConfigView(a_archive, &err)) == NULL
		|| err != B_NO_ERROR)
	{
		delete config_view;
		config_view = NULL;
	}
	
	return config_view;
}

//====================================================================
inline ConfigView::ConfigView(BMessage * const a_archive, status_t * const a_err) :
//====================================================================
	Inherited(a_archive)
{
	Init();
	Status(a_err);
}


#endif    // CONFIG_VIEW_H
