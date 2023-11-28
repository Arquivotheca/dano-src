// ===========================================================================
//	MimeType.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __MIMETYPE__
#define __MIMETYPE__

#include "Utils.h"
#include <SupportDefs.h>
#include <String.h>
class BMessage;

// ===========================================================================
// A mime type object

class MimeType : public CLinkable {
public:

static	MimeType*	MatchMime(const char *filename, const void *data, int dataCount);
static BString	RepairMimeType(const BString& type);

const	char*	GetMimeType();
const	char*	GetDescription();

		
protected:
static	void	InitMimes();

static bool InstallMimeIfNeeded(const char *type, const uchar *largeIconBits,
	const uchar *miniIconBits, const char *shortDescription, const char *
	longDescription, const char *preferredAppSignature, BMessage *attrInfo,
	uint32 forceMask);

				MimeType(const char* mimeType, const char* desc, const char* suffixes, const char* sig = NULL, int sigOffset = 0);
				MimeType(const char* mimeType, const char* desc, const char* suffixes, bool (*LooksLike)(const char* data, int dataCount));
virtual			~MimeType();

virtual	bool	MatchSuffix(const char *filename);
virtual	bool	MatchSignature(const void* data, int dataCount);


static	CLinkedList	mMimeList;
static	bool		mMimesInited;

static	MimeType*	mTextType;		// Text type if no match and it looks like text
static	MimeType*	mDefaultType;	// Default type if all else fails
static  MimeType*	mHTMLType;

		BString mMimeType;
		BString	mDescription;
		BString mSuffixes;
	
		BString	mSig;			// Byte string that identifies this as a file of this type
		int		mSigOffset;		// Starting offset of signature
};

#endif
