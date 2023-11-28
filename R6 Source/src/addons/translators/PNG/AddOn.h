/*--------------------------------------------------------------------*\
  File:      AddOn.h
  Creator:   Matt Bogosian <mattb@be.com>
  Copyright: (c)1998, Be, Inc. All rights reserved.
  Description: Header file for the PNG image translator.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include <SupportDefs.h>


#ifndef ADD_ON_H
#define ADD_ON_H


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-= Definitions, Enums, Typedefs, Consts =-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

// Configuration message entry names
extern const char * const k_config_intrlcng;
extern const char * const k_config_res_x;
extern const char * const k_config_res_y;
extern const char * const k_config_res_units;
extern const char * const k_config_offset_x;
extern const char * const k_config_offset_y;
extern const char * const k_config_offset_units;

// Configuration defaults
extern const int32 k_default_intrlcng;
extern const size_t k_default_res_x;
extern const size_t k_default_res_y;
extern const int32 k_default_res_units;
extern const size_t k_default_offset_x;
extern const size_t k_default_offset_y;
extern const int32 k_default_offset_units;


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-= Structs, Classes =-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

class BMessage;


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=- Function Prototypes =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Function name: saveConfiguration
  Defined in:    AddOn.cpp
  Arguments:     const BMessage * const a_config_msg - the message to
                     save.
  Returns:       none
  Throws:        none
  Description: Function to save a given message into the configuration
      file.
\*--------------------------------------------------------------------*/

status_t saveConfiguration(const BMessage * const a_config_msg);


#endif    // ADD_ON_H
