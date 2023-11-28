//******************************************************************************
//
//	File:		token.h
//
//	Description:	token space class.
//			Implements token managment in the window server.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//	1/20/94		tdl	Added revNumber and portId token data members to support scripting.
//				Added new_token(long a_token, short type, void** data, long portId );
//				Added get_token(long a_token, void** data, long* portId );
//				Added set_token_port(long a_token, long portId );
//	5/22/92		bgs	new today
//
//******************************************************************************

#ifndef	_TOKEN_SPACE_H
#define _TOKEN_SPACE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif
#ifndef _LOCKER_H
#include <Locker.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

class BDataIO;
class BMessage;

extern "C" status_t	_init_tokens_();
extern "C" status_t	_delete_tokens_();

namespace BPrivate {

/*-------------------------------------------------------*/

class BDirectMessageTarget
{
public:
// Add and remove references to the target.  Note that
// AcquireTarget() may be called with the token space lock
// held, so you should not acquire any other locks in it.
// ReleaseTarget() is guaranteed not to be called with the
// token lock held.
virtual	bool		AcquireTarget()					= 0;
virtual	void		ReleaseTarget()					= 0;

// Add the given message to the target.  Return B_OK on
// success, an error on failure.  This error code will
// be propagated up to SendMessage().
virtual	status_t	EnqueueMessage(BMessage* msg)	= 0;

protected:
					BDirectMessageTarget();
virtual				~BDirectMessageTarget();
};

/*-------------------------------------------------------*/

static inline int32 make_token(int32 index, int16 revision) {
	return (index&0xFFFF) | (((int32)revision) << 16);
}
static inline int32 token_index(int32 token) {
	return (token&0xFFFF);
}
static inline int16 token_revision(int32 token) {
	return (int16)(token>>16);
}
static inline int16 bump_revision(int16 revision) {
	return (revision+1)&0x7FFF;
}

// This must be a #define to not conflict with the app_server's
// definition.
#ifndef NO_TOKEN
#define NO_TOKEN		(-1)
#endif

// System token types
const int16 FREE_TOKEN_TYPE = -1;
const int16 NO_TOKEN_TYPE = 0;

// Standard token types
const int16 HANDLER_TOKEN_TYPE = 1;
const int16 BITMAP_TOKEN_TYPE = 2;
const int32 REPLY_TOKEN_TYPE = 3;		// target for a synchronous reply

// IK2 token types
const int16 NEW_HANDLER_TOKEN_TYPE = 10;

/*-------------------------------------------------------*/

typedef void (*new_token)(int16 type, void* data);
typedef bool (*get_token)(int16 type, void* data);
typedef void (*remove_token)(int16 type, void* data);

/*-------------------------------------------------------*/

class BTokenSpace {
public:
								BTokenSpace();	
								~BTokenSpace();
		
		int32					NewToken(	int16 type, void* data,
											new_token func = NULL);
		bool					CheckToken(	int32 token,
											int16 type = NO_TOKEN_TYPE) const;
		status_t				GetToken(	int32 token, int16 type,
											void** out_data,
											get_token func = NULL) const;
		void					RemoveToken(	int32 token,
												remove_token func = NULL);
		void					Dump(BDataIO& io, bool showFree=false) const;

		int32					NewToken(	int16 type, void* data,
											BDirectMessageTarget* target,
											new_token func);
		status_t				SetTokenTarget(	uint32 token,
												BDirectMessageTarget* target);
		BDirectMessageTarget*	TokenTarget(	uint32 token,
												int16 type = NO_TOKEN_TYPE) const;
		
		enum {
			kLevel1Size = 256,
			kLevel2Size = 256,
			
			kMaxTokenSpace = ((kLevel1Size * kLevel2Size) - 1)
		};

		/* one token takes 16 bytes */
		typedef	struct {
				int16					type;
				int16					revision;
				void*					data;
				BDirectMessageTarget*	target;
				int32					reserved;
			} token_entry;

		/* one token array takes 4096 bytes */
		typedef	struct	{
				token_entry	array[kLevel2Size];
			} token_array;
		
private:
		token_array*	new_token_array(int32 level_1_index);
		token_entry*	find_free_token(int32* token);
		
		// fAccess is locked upon non-NULL return.
		token_entry*	lookup_token(int32 token, bool reportErrors=true) const;
		
inline	token_entry*	fast_lookup_token(int32 token) const {
			token = token_index(token);
			return &(fLevel1[token/kLevel2Size]->array[token%kLevel2Size]);
		}
		
mutable	BLocker			fAccess;
		int32			fFreeHead;				// A linked list of free tokens
		int32			fFreeTail;
		int32			fFreeArray;				// Next array of new tokens
		int32			fFreeIndex;				// Index into new token array
		token_array*	fLevel1[kLevel1Size];
		uint32			_reserved0[12];
};

extern BTokenSpace* gDefaultTokens;

}	// namespace BPrivate
using namespace BPrivate;

#endif
