/*--------------------------------------------------------------------*\
  File:      message_archive.h
  Creator:   Matt Bogosian <mbogosian@usa.net>
  Copyright: (c)1998, Matt Bogosian. All rights reserved.
  Description: Header file describing a simple message archiving
      interface.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include <Resources.h>
#include <TypeConstants.h>


#ifndef _LIBMBOGOSIAN_ARCHIVE_H
#define _LIBMBOGOSIAN_ARCHIVE_H


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
  =-=-=-=-=-=-=-=-=-=-=-=- Function Prototypes =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*\
  Function name: findImagePath
  Defined in:    archive.cpp
  Arguments:     char m_path[B_PATH_NAME_LENGTH] - a placeholder for
                     the path (if found).
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to find the path of this image.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t findImagePath(char m_path[B_PATH_NAME_LENGTH]);

/*--------------------------------------------------------------------*\
  Function name: getMessageFromAttribute
  Defined in:    archive.cpp
  Arguments:     BNode * const a_node - the node from which to read
                     the message.
                 BMessage * const a_msg - a placeholder for the
                     message.
                 const char * const a_name - the name of the attribute
                     to read.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to get a message from an attribute from a
      given node with a given name.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t getMessageFromAttribute(BNode * const a_node, BMessage * const a_msg, const char * const a_name);

/*--------------------------------------------------------------------*\
  Function name: getMessageFromResources
  Defined in:    archive.cpp
  Arguments:     BResources * const a_rsrcs - the resources from which
                     to read the message.
                 BMessage * const a_msg - a placeholder for the
                     message.
                 const int32 a_id - the ID of the resource to read.
                 const char ** const a_name - a placeholder for the
                     name of the resource read.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to get a message with a given ID from a given
      set of resources.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t getMessageFromResources(BResources * const a_rsrcs, BMessage * const a_msg, const int32 a_id, const char ** const a_name = NULL);

/*--------------------------------------------------------------------*\
  Function name: getMessageFromResources
  Defined in:    archive.h
  Arguments:     BResources * const a_rsrcs - the resources from which
                     to read the message.
                 BMessage * const a_msg - a placeholder for the
                     message.
                 const char * const a_name - the name of the resource
                     to read.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to get a message with a given name from a
      given set of resources.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t getMessageFromResources(BResources * const a_rsrcs, BMessage * const a_msg, const char * const a_name);

/*--------------------------------------------------------------------*\
  Function name: setAttributeFromMessage
  Defined in:    archive.cpp
  Arguments:     BNode * const a_node - the node to which to write the
                     message.
                 BMessage * const a_msg - the message.
                 const char * const a_name - the name of the attribute
                     to write.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to set an attribute with a given name in a
      given node to a given message.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t setAttributeFromMessage(BNode * const a_node, BMessage * const a_msg, const char * const a_name);

/*--------------------------------------------------------------------*\
  Function name: setResourceFromMessage
  Defined in:    archive.cpp
  Arguments:     BResources * const a_rsrcs - the resources to which
                     to write the message.
                 BMessage * const a_msg - the message.
                 const int32 a_id - the ID of the resource to write.
                 const char * const a_name - the name of the resource
                     to write.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to set a resource with a given ID and name in
      a given set of resources to a given message.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t setResourceFromMessage(BResources * const a_rsrcs, BMessage * const a_msg, const int32 a_id, const char * const a_name);

/*--------------------------------------------------------------------*\
  Function name: setResourceFromMessage
  Defined in:    archive.h
  Arguments:     BResources * const a_rsrcs - the resources to which
                     to write the message.
                 BMessage * const a_msg - the message.
                 const char * const a_name - the name of the resource
                     to write.
  Returns:       status_t - B_NO_ERROR on success, another error code
                     on failure.
  Throws:        none
  Description: Function to set a resource with a given name in a given
      set of resources to a given message.
\*--------------------------------------------------------------------*/

DECL_SPEC status_t setResourceFromMessage(BResources * const a_rsrcs, BMessage * const a_msg, const char * const a_name);


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
inline status_t getMessageFromResources(BResources * const a_rsrcs, BMessage * const a_msg, const char * const a_name)
//====================================================================
{
	int32 id;
	size_t len;
	
	return (a_rsrcs->GetResourceInfo(B_MESSAGE_TYPE, a_name, &id, &len)) ? getMessageFromResources(a_rsrcs, a_msg, id) : B_ENTRY_NOT_FOUND;
}

//====================================================================
inline status_t setResourceFromMessage(BResources * const a_rsrcs, BMessage * const a_msg, const char * const a_name)
//====================================================================
{
	int32 id;
	size_t len;
	
	if (!a_rsrcs->GetResourceInfo(B_MESSAGE_TYPE, a_name, &id, &len))
	{
		// Find an unused ID
		const char *name;
		id = 0;
		
		while (a_rsrcs->GetResourceInfo(B_MESSAGE_TYPE, id, &name, &len))
		{
			id++;
		}
	}
	
	return setResourceFromMessage(a_rsrcs, a_msg, id, a_name);
}


#endif    // _LIBMBOGOSIAN_ARCHIVE_H
