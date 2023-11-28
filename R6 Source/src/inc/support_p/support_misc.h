/******************************************************************************
/
/	File:			support_misc.h
/
/	Copyright 1998, Be Incorporated
/
******************************************************************************/

#ifndef _SUPPORT_MISC_H
#define _SUPPORT_MISC_H

#include <BeBuild.h>
#include <SupportDefs.h>

#if !_SUPPORTS_MEDIA_NODES
//	Secret hooks for people who link with libmedia.so in IAD
namespace BPrivate {
status_t set_beep_hooks(status_t (*beep_f)(const char *), status_t (*add_f)(const char *));
}
using namespace BPrivate;
#endif

// Deprecated calls (starting with R4) from <UTF8.h>
status_t convert_to_utf8(uint32		srcEncoding, 
						 			const char	*src, 
						 			int32		*srcLen, 
									char		*dst, 
						 			int32		*dstLen);

status_t convert_from_utf8(uint32		dstEncoding,
									  const char	*src, 
									  int32			*srcLen, 
						   			  char			*dst, 
						   			  int32			*dstLen);


#endif
