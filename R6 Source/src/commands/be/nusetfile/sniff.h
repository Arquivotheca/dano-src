// ===========================================================================
//	MimeType.h
// 	©1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

// ===========================================================================
// A mime type object

#include <Debug.h>

#include <string.h>

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif
#ifndef _LIST_H
#include <List.h>
#endif

class TSniffer {
public:

static	TSniffer*	MatchMime(const char *filename, void *data, int dataCount);

const	char*	GetMimeType();
const	char*	GetDescription();
		
protected:

static	void	InitMimes();

				TSniffer(const char* mimeType,
						const char* desc,
						const char* suffixes,
						const char* sig = NULL,
						int sigOffset = 0);
				TSniffer(const char* mimeType,
						const char* desc,
						const char* suffixes,
						bool (*LooksLike)(const char* data, int dataCount));
virtual			~TSniffer();

virtual	bool	MatchSuffix(const char *filename);
virtual	bool	MatchSignature(void* data, int dataCount);

static	BList		mMimeList;
static	bool		mMimesInited;

static	TSniffer*	mTextType;		// Text type if no match and it looks like text
static	TSniffer*	mDefaultType;	// Default type if all else fails

		const char	*mMimeType;
		const char	*mDescription;
		const char	*mSuffixes;
	
		const char	*mSig;		// Byte string that identifies this as a file of this type
		int		mSigOffset;		// Starting offset of signature
};
