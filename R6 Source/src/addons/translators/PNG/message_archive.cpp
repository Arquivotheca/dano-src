/*--------------------------------------------------------------------*\
  File:      message_archive.cpp
  Creator:   Matt Bogosian <mbogosian@usa.net>
  Copyright: (c)1998, Matt Bogosian. All rights reserved.
  Description: Source file containing a simple message archiving
      interface.
\*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-=-=- Included Files -=-=-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

#include "message_archive.h"

#include <Node.h>
#include <Message.h>
#include <fs_attr.h>
#include <image.h>

#include <string.h>


/*--------------------------------------------------------------------*\
  =-=-=-=-=-=-=-=-=-=-=-= Function Definitions =-=-=-=-=-=-=-=-=-=-=-=
\*--------------------------------------------------------------------*/

//====================================================================
status_t findImagePath(char m_path[B_PATH_NAME_LENGTH])
//====================================================================
{
	status_t err;
	image_info info;
	int32 cookie(0);
	m_path[0] = '\0';
	int32 func_addr(reinterpret_cast<int32>(findImagePath)), text_addr;
	
	while ((err = get_next_image_info(0, &cookie, &info)) == B_NO_ERROR)
	{
		text_addr = reinterpret_cast<int32>(info.text);
		
		if (func_addr >= text_addr
			&& func_addr < text_addr + info.text_size)
		{
			strncpy(m_path, info.name, B_PATH_NAME_LENGTH);
			break;
		}
	}
	
	return err;
}

//====================================================================
status_t getMessageFromAttribute(BNode * const a_node, BMessage * const a_msg, const char * const a_name)
//====================================================================
{
	status_t err;
	char *buf(NULL);
	attr_info info;
	
	// Try to allocate space for the attribute
	if ((err = a_node->GetAttrInfo(a_name, &info)) == B_NO_ERROR
		&& (buf = new char[info.size]) == NULL)
	{
		err = B_NO_MEMORY;
	}
	
	// Try to read the attribute
	ssize_t bytes_read;
	
	if (err == B_NO_ERROR
		&& (bytes_read = a_node->ReadAttr(a_name, B_MESSAGE_TYPE, 0, buf, info.size)) != info.size)
	{
		err = B_IO_ERROR;
	}
	
	// Try to instantiate the message from the attribute
	if (err == B_NO_ERROR)
	{
		err = a_msg->Unflatten(buf);
	}
	
	delete buf;
	
	return err;
}

//====================================================================
status_t getMessageFromResources(BResources * const a_rsrcs, BMessage * const a_msg, const int32 a_id, const char ** const a_name)
//====================================================================
{
	status_t err(B_NO_ERROR);
	char *buf(NULL);
	size_t buf_len;
	const char *name;
	const char **name_holder((a_name == NULL) ? &name : a_name);
	
	// Try to find the resource
	if (!a_rsrcs->GetResourceInfo(B_MESSAGE_TYPE, a_id, name_holder, &buf_len))
	{
		err = B_ENTRY_NOT_FOUND;
	}
	
	// Try to allocate space for the resource
	if (err == B_NO_ERROR
		&& (buf = new char[buf_len]) == NULL)
	{
		err = B_NO_MEMORY;
	}
	
	// Try to read the resource and instantiate the message
	if (err == B_NO_ERROR
		&& (err = a_rsrcs->ReadResource(B_MESSAGE_TYPE, a_id, buf, 0, buf_len)) == B_NO_ERROR)
	{
		err = a_msg->Unflatten(buf);
	}
	
	delete buf;
	
	return err;
}

//====================================================================
status_t setAttributeFromMessage(BNode * const a_node, BMessage * const a_msg, const char * const a_name)
//====================================================================
{
	status_t err(B_NO_ERROR);
	char *buf(NULL);
	ssize_t buf_len(a_msg->FlattenedSize());
	
	// Try to allocate space for the attribute
	if ((buf = new char[buf_len]) == NULL)
	{
		err = B_NO_MEMORY;
	}
	
	// Try to write the attribute
	ssize_t bytes_written;
	
	if (err == B_NO_ERROR
		&& (err = a_msg->Flatten(buf, buf_len)) == B_NO_ERROR
		&& (bytes_written = a_node->WriteAttr(a_name, B_MESSAGE_TYPE, 0, buf, buf_len)) != buf_len)
	{
		err = B_IO_ERROR;
	}
	
	delete buf;
	
	return err;
}

//====================================================================
status_t setResourceFromMessage(BResources * const a_rsrcs, BMessage * const a_msg, const int32 a_id, const char * const a_name)
//====================================================================
{
	status_t err(B_NO_ERROR);
	char *buf(NULL);
	size_t buf_len(a_msg->FlattenedSize());
	
	// Try to find the resource
	if (a_rsrcs->HasResource(B_MESSAGE_TYPE, a_id))
	{
		err = a_rsrcs->RemoveResource(B_MESSAGE_TYPE, a_id);
	}
	
	// Try to allocate space for the resource
	if (err == B_NO_ERROR
		&& (buf = new char[buf_len]) == NULL)
	{
		err = B_NO_MEMORY;
	}
	
	// Try to write the attribute
	if (err == B_NO_ERROR
		&& (err = a_msg->Flatten(buf, buf_len)) == B_NO_ERROR
		&& (err = a_rsrcs->AddResource(B_MESSAGE_TYPE, a_id, buf, buf_len, a_name)) == B_NO_ERROR)
	{
		err = a_rsrcs->WriteResource(B_MESSAGE_TYPE, a_id, buf, 0, buf_len);
	}
	
	delete buf;
	
	return err;
}
