//******************************************************************************
//
//      File:           token.cpp
//
//      Description:    token space class.
//                      Implements token managment in the window server.
//
//      Written by:     Benoit Schillings
//
//      Copyright 1992, Be Incorporated
//
//      Change History:
//	1/20/94		tdl	Support revNumber and portId data members for scripting usage.
//				Added new_token(long a_token, short type, void** data, long portId );
//				Added get_token(long a_token, void** data, long* portId );
//				Added set_token_port(long a_token, long portId );
//      5/22/92         bgs     new today
//
//******************************************************************************
 
#include <support2/TokenSpace.h>

#include <support2/CallStack.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#include <stdio.h>
#include <stdlib.h>

namespace B {
namespace Support2 {

/*-------------------------------------------------------------*/
// replace the following by a gr_malloc later
 
#ifndef x_malloc
#define x_malloc        malloc
#define x_free          free
#endif
 

/*-------------------------------------------------------------*/

BTokenSpace* BTokenSpace::gDefault = NULL;
BLocker BTokenSpace::gDefaultCreationLock;

BTokenSpace *
BTokenSpace::Default()
{
	if (gDefault) return gDefault;
	gDefaultCreationLock.Lock();
	if (!gDefault) gDefault = new BTokenSpace;
	gDefaultCreationLock.Unlock();
	return gDefault;
}

/*-------------------------------------------------------------*/

BDirectMessageTarget::BDirectMessageTarget() { }
BDirectMessageTarget::~BDirectMessageTarget() { }
 
/*-------------------------------------------------------------*/
 
void    BTokenSpace::Dump(ITextOutput::arg io, bool showFree) const
{
	fAccess.Lock();
	
	io << "BTokenSpace " << this << ":" << endl;
	for (int32 i = 0; i < kLevel1Size; i++) {
		if (fLevel1[i]) {
			token_array* tmp_ta = fLevel1[i];
			io << "level 1 [" << i << "]:" << endl;
			for (int32 j=0; j<kLevel2Size; j++) {
				token_entry* tmp_tt = &(tmp_ta->array[j]);
				if (tmp_tt->type >= 0) {
					io << "\tlevel 2 [" << j << "]: type=" << tmp_tt->type;
					switch (tmp_tt->type) {
						case HANDLER_TOKEN_TYPE:
							io << " (BHandler)";
							break;
						case BITMAP_TOKEN_TYPE:
							io << " (BBitmap)";
							break;
						case NEW_HANDLER_TOKEN_TYPE:
							io << " (Be::BHandler)";
							break;
					}
					io << ", revision=" << tmp_tt->revision << endl;
					io << ",\t\tdata=" << tmp_tt->data
						<< ", target=" << tmp_tt->target << endl;
				}
			}
		}
	}
	io << "Current free array = #" << fFreeArray
		<< ", at index " << fFreeIndex << endl;
	io << "Free list: head = " << (void*)fFreeHead
		<< ", tail = " << (void*)fFreeTail;
	if (showFree) {
		int32 freeCount = 0;
		int32 freeToken = fFreeHead;
		while (freeToken != NO_TOKEN) {
			if (freeCount%4) {
				io << endl << "\t #" << freeCount << ": ";
			}
			io << (void*)freeToken << ", ";
			freeCount++;
			token_entry* tt = fast_lookup_token(freeToken);
			freeToken = (int32)(tt->data);
		}
	} else {
		int32 freeCount = 0;
		int32 freeToken = fFreeHead;
		while (freeToken != NO_TOKEN) {
			freeCount++;
			token_entry* tt = fast_lookup_token(freeToken);
			freeToken = (int32)(tt->data);
		}
		io << endl << "Free list contains " << freeCount << " tokens" << endl;
	}
	
	fAccess.Unlock();
}

/*-------------------------------------------------------------*/
 
BTokenSpace::BTokenSpace()
{
	fFreeHead = NO_TOKEN;
	fFreeTail = NO_TOKEN;
	fFreeArray = -1;
	fFreeIndex = 0;
	
	for (int32 i = 0; i < kLevel1Size; i++)
		fLevel1[i] = NULL;
}
 
/*-------------------------------------------------------------*/
 
BTokenSpace::~BTokenSpace()
{
	for (int32 i = 0; i < kLevel1Size; i++) {
		if (fLevel1[i]) {
			free(fLevel1[i]);
			fLevel1[i] = NULL;
		}
	}
}
 
/*-------------------------------------------------------------*/
 
int32 BTokenSpace::NewToken(int16 type, void* data,
						   new_token func)
{
	return NewToken(type, data, NULL, func);
}

/*-------------------------------------------------------------*/
 
int32 BTokenSpace::NewToken(int16 type, void* data,
						   BDirectMessageTarget* target, new_token func)
{
	fAccess.Lock();
	
	int32 token;
	token_entry* tt = find_free_token(&token);
	
	if (tt == NULL) {
		fAccess.Unlock();
		debugger("The token space is full.  I will crash now.");
		return NO_TOKEN;
	}
	
	tt->type = type;
	tt->data = data;
	if (target && target->AcquireTarget())
		tt->target = target;
	else
		tt->target = NULL;
	if (func)
		(*func)(type, data);
	
	fAccess.Unlock();
	
	PRINT(("New token: token=%p, type=%d, data=%p\n",
			(void*)token, type, data));
			
	return token;
}

/*-------------------------------------------------------------*/
 
bool BTokenSpace::CheckToken(int32 token, int16 type) const
{
	token_entry* tmp_tt = lookup_token(token, false);
	if (tmp_tt) {
		if (type != NO_TOKEN_TYPE && type != tmp_tt->type) {
			tmp_tt = NULL;
		}
		fAccess.Unlock();
	}
	
//+	PRINT(("Check %s: token=%p, type=%d, data=%p\n",
//+			(tmp_tt != NULL ? "succeeded" : "failed"), (void*)token, type));
			
	return tmp_tt != NULL;
}
 
/*-------------------------------------------------------------*/

status_t BTokenSpace::GetToken(int32 token, int16 type, void** out_data,
							  get_token func) const
{
	token_entry* tmp_tt = lookup_token(token);
	if (tmp_tt) {
		// Match type, if requested.
		if (type != NO_TOKEN_TYPE && type != tmp_tt->type) {
			tmp_tt = NULL;
		}
		
		// Call acquire func, if requested.
		if (func && tmp_tt) {
			if (!((*func)(tmp_tt->type, tmp_tt->data))) {
				tmp_tt = NULL;
			}
		}
		
		if (tmp_tt) {
			// This token is a-okay, return it.
//+			PRINT(("Get token: token=%p, type=%d, data=%p\n",
//+					(void*)token, tmp_tt->type, tmp_tt->data));
			if (out_data) *out_data = tmp_tt->data;
			fAccess.Unlock();
			return B_OK;
		}
		
		fAccess.Unlock();
	}
	
	PRINT(("Get failed: token=%p, type=%d\n",
			(void*)token, type));
	
	if (out_data) *out_data = NULL;
	return B_ERROR;
}

/*-------------------------------------------------------------*/
// Note that remove_token is not deallocating the ptr but
// only updating the token space.
 
void BTokenSpace::RemoveToken(int32 token, remove_token func)
{
	BDirectMessageTarget* target = NULL;
	
	token_entry* tmp_tt = lookup_token(token);
	if (tmp_tt) {
		PRINT(("Remove token: token=%p, type=%d, data=%p\n",
				(void*)token, tmp_tt->type, tmp_tt->data));
		if (func)
			(*func)(tmp_tt->type, tmp_tt->data);
		tmp_tt->type = -1;
		tmp_tt->revision = bump_revision(tmp_tt->revision);
		tmp_tt->data = (void*)NO_TOKEN;
		target = tmp_tt->target;
		tmp_tt->target = NULL;
		if (fFreeTail != NO_TOKEN) {
			token_entry* tail_tt = fast_lookup_token(fFreeTail);
			tail_tt->data = (void*)token;
		} else {
			fFreeHead = token;
		}
		fFreeTail = token;
		fAccess.Unlock();
	} else {
		PRINT(("*** Remove Failed: token=%p\n",
				(void*)token));
	}
	
	if (target)
		target->ReleaseTarget();
}

/*-------------------------------------------------------------*/

status_t BTokenSpace::SetTokenTarget(uint32 token, BDirectMessageTarget* target)
{
	BDirectMessageTarget* oldTarget = NULL;
	
	token_entry* tmp_tt = lookup_token(token);
	if (tmp_tt) {
		if (tmp_tt && (!target || target->AcquireTarget())) {
			oldTarget = tmp_tt->target;
			tmp_tt->target = target;
			fAccess.Unlock();
	
			if (oldTarget)
				oldTarget->ReleaseTarget();
				
			return B_OK;
		}
		
		fAccess.Unlock();
	}
	
	return B_ERROR;
}

/*-------------------------------------------------------------*/

BDirectMessageTarget* BTokenSpace::TokenTarget(uint32 token, int16 type) const
{
	token_entry* tmp_tt = lookup_token(token);
	if (tmp_tt) {
		// Match type, if requested.
		if (type != NO_TOKEN_TYPE && type != tmp_tt->type) {
			tmp_tt = NULL;
		}
		
		if (tmp_tt && tmp_tt->target && tmp_tt->target->AcquireTarget()) {
			BDirectMessageTarget* target = tmp_tt->target;
			fAccess.Unlock();
			return target;
		}
		
		fAccess.Unlock();
	}
	
	return NULL;
}

/*-------------------------------------------------------------*/

BTokenSpace::token_array* BTokenSpace::new_token_array(int32 level_1_index)
{
	token_array* tmp_ta = (token_array *)x_malloc(sizeof(token_array));
	
	token_entry* tmp_tt = tmp_ta->array;
	for (int32 i = 0; i < kLevel2Size; i++, tmp_tt++) {
		tmp_tt->type = -1;
		tmp_tt->revision = 0;
	}

	fFreeArray = level_1_index;
	fLevel1[level_1_index] = tmp_ta;
	return tmp_ta;
}

/*-------------------------------------------------------------*/
 
BTokenSpace::token_entry* BTokenSpace::find_free_token(int32* token)
{
	if (fFreeHead != NO_TOKEN) {
		// Pull token off the free list.
		int32 t = fFreeHead;
		token_entry* tt = fast_lookup_token(t);
		ASSERT(tt->type < 0);
		fFreeHead = (int32)(tt->data);
		if (fFreeHead == NO_TOKEN) fFreeTail = NO_TOKEN;
		*token = make_token(token_index(t), tt->revision);
		return tt;
	}
	
	token_array* ta = NULL;
	
	if (fFreeArray < 0 || fFreeIndex >= kLevel2Size) {
		// Need to allocate a new token array.
		if (fFreeArray < (kLevel1Size-1)) {
			fFreeArray++;
			fFreeIndex = 0;
			ta = new_token_array(fFreeArray);
		}
		
	} else {
		// Have an existing token array with free space.
		ta = fLevel1[fFreeArray];
	}
	
	if (!ta) {
		return NULL;
	}
	
	// Return token from the new array.
	token_entry* tt = &(ta->array[fFreeIndex]);
	*token = make_token((fFreeArray*kLevel2Size)+fFreeIndex, tt->revision);
	fFreeIndex++;
	return tt;
}
 
/*-------------------------------------------------------------*/

BTokenSpace::token_entry* BTokenSpace::lookup_token(int32 token, bool reportErrors) const
{
	if (token == NO_TOKEN)
		return NULL;
	
	if (token < 0) {
		if (reportErrors) {
			bser << "Negative token: " << (void*)token << endl;
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(bser);
		}
		return NULL;
	}
	
	const int16 revision = token_revision(token);
	const int32 index = token_index(token);
	
	if (index > kMaxTokenSpace) {
		if (reportErrors) {
			bser << "Bad token: " << (void*)token << endl;
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(bser);
		}
		return NULL;
	}
	
// Note that the /, *, and % are replaced by shift and & by the compiler
// due to the choice of kLevel2Size
 
	fAccess.Lock();
	
	token_array* tmp_ta = fLevel1[index / kLevel2Size];
	if (tmp_ta == NULL) {
		fAccess.Unlock();
		if (reportErrors) {
			bser << "Unallocated token: " << (void*)token << endl;
			BCallStack stack;
			stack.Update(1);
			stack.LongPrint(bser);
		}
		return NULL;
	}
	
	token_entry* tmp_tt = &(tmp_ta->array[index % kLevel2Size]);
	
 	if (tmp_tt->revision != revision) {
 		fAccess.Unlock();
 		return NULL;
 	}

	return tmp_tt;
}

} }	// namespace B::Support2

/*-------------------------------------------------------------*/
 
/*
long            test[31000];
 
void    main()
{
        BTokenSpace      *ts;
        long           owner;
        short           type;
        long            size;
        void            *data;
        long            i;
        long            j;
        long            id;
 
        ts = new BTokenSpace;
 
        for (i = 0 ; i < 30000; i++) {
                owner = 55;
                type = i;
                size = 0;
                data = (void *)0;
                id = ts->new_token(owner, type, size, data);
                test[i] = id;
        }
 
        ts->dump();
 
        for (j = 0 ; j < 20; j++) {
                for (i = 0; i < 30000; i += 2) {
                        ts->remove_token(test[i]);
                }
 
                for (i = 0 ; i < 30000; i+= 2) {
                        owner = 55;
                        type = i;
                        size = 0;
                        data = (void *)0;
                        id = ts->new_token(owner, type, size, data);
                        test[i] = id;
                }
        }
 
        ts->dump();
 
        PRINT(("checking\n"));
 
        for (i = 0; i < 30000; i++) {
                ts->get_token(test[i], &owner, &type, &size, &data);
                if (type != i)
                        PRINT(("error\n"));
        }
}
*/
