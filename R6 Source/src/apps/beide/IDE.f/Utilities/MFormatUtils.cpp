//========================================================================
//	MFormatUtils.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "IDEConstants.h"
#include "MFormatUtils.h"

// ---------------------------------------------------------------------------
//		FindFileFormat
// ---------------------------------------------------------------------------
//	Determine if this file contains text in Unix format, Mac format, or DOS 
//	format.  The default is Unix if no eol chars are found.  The buffer
//	must be null terminated.

TextFormatType
MFormatUtils::FindFileFormat( 
	const char * 	inBuffer)
{
	const unsigned char *		ptr = (const unsigned char *) inBuffer;
	unsigned char				c;

	while ((c = *ptr) != '\0')
	{
		switch(c)
		{
			case NEWLINE:
				return kNewLineFormat;
				break;

			case MAC_RETURN:
				if (*(ptr + 1) == NEWLINE)
					return kCRLFFormat;
				else
					return kMacFormat;
				break;
			
			default:
				ptr++;
				break;
		}
	}

	return kNewLineFormat;
}

// ---------------------------------------------------------------------------
//		ConvertToNewLineFormat
// ---------------------------------------------------------------------------
//	Convert a buffer from Mac or Dos format to NewLine format.  In the case
//	of Dos format the length will be changed.  The buffer must be null terminated.

void
MFormatUtils::ConvertToNewLineFormat(
	char * 			inBuffer, 
	off_t&			inLength,
	TextFormatType	inExistingFormat)
{
	unsigned char *		ptr = (unsigned char *) inBuffer;
	unsigned char		c;

	switch (inExistingFormat)
	{
		// Convert all carriage returns to line feeds
		case kMacFormat:
			while ((c = *ptr) != '\0')
			{
				switch(c)
				{
					case MAC_RETURN:
						*ptr = NEWLINE;		// replace the cr with nl
						break;
					
					default:
						ptr++;
						break;
				}
			}
			break;
		
		// Remove all carriage returns and shorten the block
		case kCRLFFormat:
			unsigned char * 	target = ptr;
			int32				newLength = inLength;

			while ((c = *ptr) != '\0')
			{
				switch(c)
				{
					case MAC_RETURN:
						if (*(ptr + 1) == NEWLINE)	// skip the CR this time
						{							// the nl will be copied next iteration
							newLength--;
						}
						else
						{
							*target++ = NEWLINE;	// CR without a LF (?)
						}
						ptr++;
						break;
					
					default:
						*target++ = *ptr++;
						break;
				}
			}

			*target = 0;				// null terminate the block
			inLength = newLength;		// update the length
			break;
	}
}

// ---------------------------------------------------------------------------
//		ConvertToNativeFormat
// ---------------------------------------------------------------------------
//	Convert a buffer from NewLine format to Mac or Dos format.  In the case
//	of Dos format the length will be changed.  The input buffer must be null 
//	terminated.

void
MFormatUtils::ConvertToNativeFormat(
	char *&			inBuffer,
	int32&			inLength,
	int32			inLines,
	TextFormatType	inNativeFormat)
{
	unsigned char *		ptr = (unsigned char *) inBuffer;
	unsigned char * 	target;
	unsigned char		c;

	switch (inNativeFormat)
	{
		// Convert all line feeds to carriage returns
		case kMacFormat:
			target = new unsigned char[inLength];
			inBuffer = (char*) target;	// let the caller know where the target block is

			while ((c = *ptr) != '\0')
			{
				switch(c)
				{
					case NEWLINE:
						*target++ = MAC_RETURN;		// replace the cr with nl
						ptr++;
						break;
					
					default:
						*target++ = *ptr++;
						break;
				}
			}
			break;
		
		// Convert all line feeds to carriage return/Line feeds
		case kCRLFFormat:
			int32				newLength = inLength + inLines;
			int32				returns = 0;

			target = new unsigned char[newLength];
			inBuffer = (char*) target;	// let the caller know where the target block is

			while ((c = *ptr) != '\0')
			{
				switch(c)
				{
					case NEWLINE:
						*target++ = MAC_RETURN;
						*target++ = *ptr++;
						returns++;
						break;
					
					default:
						*target++ = *ptr++;
						break;
				}
			}

			inLength += returns;		// update the length
			break;
	}
}
