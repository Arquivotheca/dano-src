/* ++++++++++

   FILE:  Stream.cpp
   REVS:  $Revision: 1.45 $
   NAME:  r
   DATE:  Thu Jun 22 16:33:16 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <Debug.h>
#include <BufferStream.h>
#include <BufferStreamManager.h>
#include <Subscriber.h>
#include <R3MediaDefs.h>
#include <string.h>
#include <OS.h>

#include <stdio.h>
//#include <system_priv.h>
//#define _debugPrintf _kdprintf_

#define NOT_BLOCKED B_BAD_SEM_ID
#define FORCE_UNBLOCK B_ERROR

/* Debugging hack to check consistancy of Subscriber IDs */
#ifdef DEBUG
#define DEBUG_STREAM
#else
#undef DEBUG_STREAM
#endif

#ifdef DEBUG_STREAM
#define SUBIDCHECK(SUB) { if(SubIDCheck(SUB,__FILE__,__LINE__)) \
							 { PrintSubscribers(); PrintBuffers();}}
#else
#define SUBIDCHECK(SUB)
#endif

#ifdef DEBUG_STREAM
static bool SubIDCheck(subscriber_id subID, char *file, int32 line)
{
#if 0
  char *errstr;

  errstr = NULL;

  if (subID == NULL) {
	errstr = "null subscriber to SubIDCheck";
	goto errout;
  }

  if (subID->fRel) {
	int32 count;
	buffer_id buf;
	for (buf = subID->fRel, count = 0; buf != NULL; buf = buf->fNext) {
	  if (buf->fHeldBy == subID) {
		count += 1;
	  } else {
		break;
	  }
	}
	if (count != subID->fHeld) {
	  errstr = "number of held buffers doesn\'t match fHeld";
	  goto errout;
	}
  }

  if (subID->fRel) {
	buffer_id buf;

	for (buf=subID->fRel; buf && (buf->fHeldBy == subID); buf = buf->fNext)
	  ;
	/* buf now is the oldest buffer not held by subID -- it should be
	 * the next one to be acquired.
	 */
	if (buf && (buf != subID->fAcq)) {
	  errstr = "fAcq != fHeldBy + 1";
	  goto errout;
	}
  }

  if (subID->fRel && subID->fAcq) {
	int32 i;
	buffer_id buf;
	/* if fHeld == 0, then assert subID->fRel == subID->fAcq.
	 * if fHeld == 1, then assert (subID->fRel)->fNext == subID->fAcq.
	 * etc.
	 */
	for (i=0, buf = subID->fRel; i<subID->fHeld; i++, buf = buf->fNext)
	  ;

	if (buf != subID->fAcq) {
	  errstr = "held count doesn't match rel / acq";
	  goto errout;
	}
  }

  if ((subID->fRel == NULL) && 
	  (subID->fAcq == NULL) &&
	  (subID->fHeld != 0)) {
	errstr = "null fRel / fAcq but non-0 held\n";
	goto errout;
  }

  if ((subID->fHeld > 0) && ((subID->fRel)->fHeldBy != subID)) {
	errstr = "owns buffer, buffer doesn't know it";
	goto errout;
  }

 errout:
  if (errstr) {
	_debugPrintf("%s, line %d: SubID %x %s:\n", file, line, subID, errstr);
	return TRUE;
  } else {
	return FALSE;
  }
#else
  return FALSE;
#endif
}
#endif			// #ifdef DEBUG_STREAM



BBufferStream::BBufferStream(size_t headerSize,
							 BBufferStreamManager* controller,
							 BSubscriber* headFeeder,
							 BSubscriber* tailFeeder) : fLock("BBufferStream lock",true,true)
{
  fSubCount = 0;
  fEnteredSubCount = 0;
  fFirstSem = create_sem(0, "first subscriber");
  fStreamManager = NULL;
  fHeadFeeder = NULL;
  fTailFeeder = NULL;
  fHeaderSize = headerSize;
  InitSubscribers();
  InitBuffers();

  if (controller)
	if (controller->Subscribe(this) == B_NO_ERROR)
	  if (EnterStream(controller->ID(), NULL, FALSE) == B_NO_ERROR)
		fStreamManager = controller;
  if (headFeeder)
	if (headFeeder->Subscribe(this) == B_NO_ERROR)
	  if (headFeeder->EnterStream(NULL, TRUE, NULL, NULL, NULL, TRUE)
		  == B_NO_ERROR)
		fHeadFeeder = headFeeder;
  if (tailFeeder)
	if (tailFeeder->Subscribe(this) == B_NO_ERROR)
	  if (tailFeeder->EnterStream(NULL, FALSE, NULL, NULL, NULL, TRUE)
		  == B_NO_ERROR)
		fTailFeeder = tailFeeder;
  // printf("\n%x  %x  %x\n",fStreamManager,fHeadFeeder,fTailFeeder);
}

BBufferStream::~BBufferStream()
{
  Lock();

  FreeAllSubscribers();
  FreeAllBuffers();
  delete_sem(fFirstSem);
  //Unlock();
}

void *BBufferStream::operator new(size_t size)
{
  BBufferStream *bs;
  area_id id;
  int32 rem;

  /* Round size up to B_PAGE_SIZE */
  rem = size % B_PAGE_SIZE;
  if (rem > 0) size += B_PAGE_SIZE-rem;

  id = create_area("Buffer Stream",
				   (void **)(&bs),
				   B_ANY_KERNEL_ADDRESS,
				   size,
				   B_LAZY_LOCK,	/* ?? too heavy handed ?? */
				   B_READ_AREA + B_WRITE_AREA);


  if (id < 0) {
	return NULL;
  }

  bs->fAreaID = id;
  return bs;
}

void BBufferStream::operator delete(void *stream, size_t size)
{
  if (stream) delete_area(((BBufferStream *)stream)->fAreaID);
}

size_t BBufferStream::HeaderSize() const
{
  return fHeaderSize;
}

status_t BBufferStream::GetStreamParameters(size_t *bufferSize,
											int32 *bufferCount,
											bool *isRunning,
											int32 *subscriberCount) const
{
  if (fStreamManager == NULL)
	return B_ERROR;
  if (bufferSize)
	*bufferSize = fStreamManager->BufferSize();
  if (bufferCount)
	*bufferCount = fStreamManager->BufferCount();
  if (isRunning)
	*isRunning = (fStreamManager->State() == B_RUNNING);
  if (subscriberCount)
	*subscriberCount = CountSubscribers();
  return B_NO_ERROR;
}

status_t BBufferStream::SetStreamBuffers(size_t bufferSize, int32 bufferCount)
{
  if (fStreamManager == NULL)
	return B_ERROR;
  fStreamManager->SetBufferSize(bufferSize);
  fStreamManager->SetBufferCount(bufferCount);
  return B_NO_ERROR;
}

status_t BBufferStream::StartStreaming()
{
  if (fStreamManager == NULL)
	return B_ERROR;
  fStreamManager->Start();
  return B_NO_ERROR;
}

status_t BBufferStream::StopStreaming()
{
  if (fStreamManager == NULL)
	return B_ERROR;
  fStreamManager->Stop();
  return B_NO_ERROR;
}

BBufferStreamManager* BBufferStream::StreamManager() const
{
  return fStreamManager;
}

int32 BBufferStream::CountBuffers() const
{
  return fCountBuffers;
}


/* Subscribe() finds a free subscriber record in the freelist of
 * subscribers and removes it from the freelist.  It's the
 * responsibility of the caller to remember the subscriber_id
 * for subsequent calls to EnterStream(), etc.
 */
status_t BBufferStream::Subscribe(char *name, 
								  subscriber_id *subID,
								  sem_id semID)
{
  status_t result = B_NO_ERROR;
  subscriber_id s;
  Lock();

  if (fFreeSubs == NULL)
	/* no subscriber slots left.  Unsubscribe dead subscribers. */
	for (int i=0; i<B_MAX_SUBSCRIBER_COUNT; i+=1) {
	  int32 temp;
	  s = &fSubscribers[i];
	  sem_id sem = s->fSem;
	  if (sem >= B_NO_ERROR)
		if (get_sem_count (sem, &temp) == B_BAD_SEM_ID)
		  Unsubscribe (s);
	}

  if ((s = fFreeSubs) != NULL) {
	/* unlink s from doubly linked list */
	if (s->fNext == s) {		/* that was the last one */
	  fFreeSubs = NULL;
	} else {					/* unlink, point fFreeSubs at next */
	  (s->fPrev)->fNext = s->fNext;
	  (s->fNext)->fPrev = s->fPrev;
	  fFreeSubs = s->fNext;
	}
	s->fNext = s->fPrev = NULL;
	s->fRel = s->fAcq = NULL;
	s->fSem = semID;
	s->fBlockedOn = NOT_BLOCKED;
	s->fHeld = 0;
	s->fTotalTime = 0;
	fSubCount += 1;
	*subID = s;

  } else {
	/* no subscriber slots left.  What's the best return value? */
	result = B_BAD_SUBSCRIBER;
  }
  SUBIDCHECK(s);

  Unlock();
  return result;
}

status_t BBufferStream::Unsubscribe(subscriber_id subID)
{
  status_t result;
  Lock();

  if (IsSubscribedSafe(subID)) {
	ExitStream(subID);				/* remove from entered list */

	/* Insert subID in the (circular) free list previous to fFreeSubs. 
	 * This makes it so the most recently freed subscriber_id won't
	 * be re-used until all the others have been taken.
	 */
	SUBIDCHECK(subID);

	if (fFreeSubs) {
	  subID->fNext = fFreeSubs;
	  subID->fPrev = fFreeSubs->fPrev;
	  (fFreeSubs->fPrev)->fNext = subID;
	  fFreeSubs->fPrev = subID;
	} else {
	  fFreeSubs = subID;
	  subID->fPrev = subID;
	  subID->fNext = subID;
	}
	
	subID->fRel = subID->fAcq = NULL;
	subID->fHeld = 0;
	subID->fSem = B_BAD_SUBSCRIBER;

	fSubCount -= 1;

	result = B_NO_ERROR;
  } else {
	result = B_BAD_SUBSCRIBER;
  }

  SUBIDCHECK(subID);
  Unlock();
  return result;
}

status_t BBufferStream::EnterStream(subscriber_id subID, 
									subscriber_id neighbor,
									bool before)
{
  status_t result = B_NO_ERROR;

  Lock();

  if (neighbor == NULL)
	if (before &&  fHeadFeeder) {
	  before = FALSE;
	  neighbor = fHeadFeeder->ID();
	}
	else if (!before && fTailFeeder) {
	  before = TRUE;
	  neighbor = fTailFeeder->ID();
	}
	else if (!before && fStreamManager) {
	  before = TRUE;
	  neighbor = fStreamManager->ID();
	}

  if (!IsSubscribedSafe(subID) || IsEntered(subID)) {
	result = B_BAD_SUBSCRIBER;
	goto errout;
  } else if ((neighbor != NULL) && 
			 (!IsSubscribedSafe(neighbor) || !IsEnteredSafe(neighbor))) {
	result = B_SUBSCRIBER_NOT_ENTERED;
	goto errout;
  }

  SUBIDCHECK(subID);

  //  ExitStream(subID);		/* don't activate twice */
  //  SUBIDCHECK(subID);

  if (before) {

	if (neighbor == NULL) {		/* subscribe first in the itinerary */
	  subID->fNext = fFirstSub;
	  subID->fPrev = NULL;
	  fFirstSub = subID;
	} else {					/* subscribe before neighbor */
	  subID->fNext = neighbor;
	  subID->fPrev = neighbor->fPrev;
	  neighbor->fPrev = subID;
	}

  } else /* if (before == FALSE) */ {

	if (neighbor == NULL) {		/* subscribe last in the itinerary */
	  subID->fPrev = fLastSub;
	  subID->fNext = NULL;
	  fLastSub = subID;
	} else {					/* subscribe after neighbor */
	  subID->fPrev = neighbor;
	  subID->fNext = neighbor->fNext;
	  neighbor->fNext = subID;
	}
  }

  if (subID->fPrev)			/* fixup predessor's next pointer */
	(subID->fPrev)->fNext = subID;
  else
	fFirstSub = subID;

  if (subID->fNext)			/* fixup successor's prev pointer */
	(subID->fNext)->fPrev = subID;
  else
	fLastSub = subID;

  fEnteredSubCount += 1;
  subID->fTotalTime = 0;

  /* Prepare the subscriber for its first buffers */
  InheritBuffers(subID);
  /* Adjust next subscriber's semaphore */
  WakeSubscriber(subID->fNext);

errout:
  SUBIDCHECK(subID);
  Unlock();
  return result;
}

status_t BBufferStream::ExitStream(subscriber_id subID)
{
  status_t result = B_NO_ERROR;

  Lock();

  if (IsEnteredSafe(subID) == FALSE) {
	result = B_SUBSCRIBER_NOT_ENTERED;
	goto errout;
  }

  SUBIDCHECK(subID);

  /* hand all available buffers over to the next in line */
  BequeathBuffers(subID);
  /* Adjust next subscriber's semaphore */
  WakeSubscriber(subID->fNext);

  /* remove subscriber from itinerary */
  if (subID->fNext) {
	(subID->fNext)->fPrev = subID->fPrev;
  } else if (fLastSub == subID) {
	fLastSub = subID->fPrev;
  }
  if (subID->fPrev) {
	(subID->fPrev)->fNext = subID->fNext;
  } else if (fFirstSub == subID) {
	fFirstSub = subID->fNext;
  }
  fEnteredSubCount -= 1;
  subID->fNext = subID->fPrev = NULL;

  /* If subID was waiting for a buffer, break the sad news.  In the call to
   * AcquireBuffer, IsEnteredSafe() will return FALSE and AcquireBuffer
   * will give an error return.
   */
  WakeSubscriber(subID);

errout:
  SUBIDCHECK(subID);
  Unlock();
  return result;
}

bool BBufferStream::IsSubscribed(subscriber_id subID)
{
  bool result;

  Lock();
  result = IsSubscribedSafe(subID);
  SUBIDCHECK(subID);
  Unlock();
  return result;
}

bool BBufferStream::IsEntered(subscriber_id subID)
{
  bool result;

  Lock();
  result = IsEnteredSafe(subID);
  SUBIDCHECK(subID);
  Unlock();
  return result;
}

status_t BBufferStream::SubscriberInfo(subscriber_id subID,
									   char** name,
									   stream_id* streamID,
									   int32* position)
{
	sem_info si;
	status_t result;

	Lock();
	SUBIDCHECK(subID);
	if (!IsSubscribedSafe(subID))
	  result = B_BAD_SUBSCRIBER;
	else {
	  if (name)
		if (subID == B_SHARED_SUBSCRIBER_ID)
		  *name = B_SHARED_SUBSCRIBER_NAME;
		else if (subID == B_NO_SUBSCRIBER_ID)
		  *name = B_NO_SUBSCRIBER_NAME;
		else if ((result = get_sem_info(subID->fSem, &si)) == B_NO_ERROR) 
		  *name = si.name;
	  if (streamID)
		*streamID = (stream_id)this;
	  if (position)
		*position = 0;  // fix this
	}
	Unlock();
	return result;
}


/* ================
 * UnblockSubscriber() - wake a blocked subscriber with error return
 */

status_t BBufferStream::UnblockSubscriber(subscriber_id subID)
{
  status_t result = B_NO_ERROR;

  Lock();

  if (IsEnteredSafe(subID) == FALSE) {
	result = B_SUBSCRIBER_NOT_ENTERED;
	goto errout;
  }

  /* Setting fBlockedOn to FORCE_UNBLOCK notifies AcquireBuffer to abort. */
  /* This is hokey but no worse than what I'm replacing. */
  if (subID->fBlockedOn >= 0) {
	sem_id blockedOn = subID->fBlockedOn;
	subID->fBlockedOn = FORCE_UNBLOCK;
	result = release_sem_etc(blockedOn, 1, B_DO_NOT_RESCHEDULE);
  }

errout:
  SUBIDCHECK(subID);
  Unlock();
  return result;
}

status_t BBufferStream::AcquireBuffer(subscriber_id subID, 
									  buffer_id *bufID,
									  bigtime_t timeout)
{
  buffer_id buf = NULL;
  status_t result;

  Lock();

  while (TRUE) {			/* loop until we get our buffer... */
	result = B_NO_ERROR;

	if (IsSubscribedSafe(subID) == FALSE) {
	  result = B_BAD_SUBSCRIBER;
	  break;
	} else if (IsEnteredSafe(subID) == FALSE) {
	  result = B_SUBSCRIBER_NOT_ENTERED;
	  break;
	}
	
	SUBIDCHECK(subID);

	if (subID->fAcq && subID->fAcq->fAvailTo == subID) {
	  /* we got our buffer */
	  buf = subID->fAcq;	  
	  buf->fHeldBy = subID;
	  buf->fAcqTime = system_time();
	  if (subID->fRel == NULL) subID->fRel = buf;
	  subID->fHeld += 1;

	  *bufID = buf;
	  subID->fAcq = buf->fNext;
	  SUBIDCHECK(subID);
	  break;
	}
	else {
	  /* The next buffer doesn't exist yet (== NULL) or isn't available
	   * to us (->fAvailTo != subID).  Block until something happens.
	   */
	  subscriber_id prevID = subID->fPrev;
	  sem_id blockOnThis = (prevID ? prevID->fSem : fFirstSem);
	  subID->fBlockedOn = blockOnThis;
	  ASSERT((subID->fAcq == subID->fRel) == (subID->fHeld == 0));
	  Unlock();
	  do {
		if (timeout > 0)
		  result = acquire_sem_etc(blockOnThis, 1, B_TIMEOUT, timeout);
		else
		  result = acquire_sem(blockOnThis);
	  } while (result == B_INTERRUPTED);
	  Lock();
	  sem_id wasBlockedOn = subID->fBlockedOn;
	  subID->fBlockedOn = NOT_BLOCKED;
	  /* Check for UnblockSubscriber telling us to return an error. */
	  if (wasBlockedOn == FORCE_UNBLOCK) {
		result = B_ERROR;
		break;
	  }
	  if (result == B_BAD_SEM_ID) {
		if (prevID != subID->fPrev)
		  continue;					// prev changed, try again
		if (prevID && prevID->fSem != blockOnThis)
		  continue;					// prev recycled?, try again
		if (prevID) {
		  /* Dead subscriber, bury it and try again */
		  result = Unsubscribe(prevID);
		  if (result == B_NO_ERROR)
			continue;
		}
	  }
	  if (result != B_NO_ERROR) {
		/* Miscellaneous error, give up */
		SUBIDCHECK(subID);
		break;
	  }
	}
  }

  SUBIDCHECK(subID);
  Unlock();
  return result;

}

status_t BBufferStream::ReleaseBuffer(subscriber_id subID)
{
  buffer_id buf;

  status_t result = B_NO_ERROR;
  Lock();

  if (IsSubscribedSafe(subID) == FALSE) {
	result = B_BAD_SUBSCRIBER;
	goto errout;
  } else if (IsEnteredSafe(subID) == FALSE) {
	result = B_SUBSCRIBER_NOT_ENTERED;
	goto errout;
  }

  SUBIDCHECK(subID);

  /* Ensure that subID has a buffer to release */
  if (subID->fHeld == 0) {
	result = B_BUFFER_NOT_AVAILABLE;
	goto errout;
  }

  result = ReleaseBufferSafe(subID);

errout:
  SUBIDCHECK(subID);
  Unlock();
  return result;

}

char *BBufferStream::BufferData(buffer_id bufID) const
{
  return bufID->fAddress;
}

size_t BBufferStream::BufferSize(buffer_id bufID) const
{
  return bufID->fSize;
}

bool BBufferStream::IsFinalBuffer(buffer_id bufID) const
{
  return bufID->fIsFinal;
}

int32 BBufferStream::CountBuffersHeld(subscriber_id subID)
{
  int32 result;

  Lock();

  if (IsSubscribedSafe(subID) == FALSE) {
	result = B_BAD_SUBSCRIBER;
	goto errout;
  } else if (IsEnteredSafe(subID) == FALSE) {
	result = B_SUBSCRIBER_NOT_ENTERED;
	goto errout;
  }

  result = subID->fHeld;

errout:
  SUBIDCHECK(subID);
  Unlock();
  return result;

}

int32 BBufferStream::CountSubscribers() const
{
  return fSubCount;
}

int32 BBufferStream::CountEnteredSubscribers() const
{
  return fEnteredSubCount;
}

subscriber_id BBufferStream::FirstSubscriber() const
{
  return fFirstSub;
}

subscriber_id BBufferStream::LastSubscriber() const
{
  return fLastSub;
}

subscriber_id BBufferStream::NextSubscriber(subscriber_id subID)
{
  subscriber_id next;

  Lock();
  if (IsSubscribedSafe(subID) && IsEnteredSafe(subID)) {
	next = subID->fNext;
  } else {
	next = NULL;
  }
  SUBIDCHECK(subID);
  Unlock();
  return next;
}

subscriber_id BBufferStream::PrevSubscriber(subscriber_id subID)
{
  subscriber_id prev;

  Lock();
  if (IsSubscribedSafe(subID) && IsEnteredSafe(subID)) {
	prev = subID->fPrev;
  } else {
	prev = NULL;
  }
  SUBIDCHECK(subID);
  Unlock();
  return prev;
}



void BBufferStream::PrintStream()
{
  Lock();
  PrintSubscribers();
  PrintBuffers();
  Unlock();
}

void BBufferStream::PrintBuffers()
{
  buffer_id bufID;

  Lock();

  _debugPrintf("Buffers: Oldest=%7x, Newest=%7x\n",fOldestBuffer,fNewestBuffer);
  for (bufID = fOldestBuffer; bufID != NULL; bufID = bufID->fNext) {
	_debugPrintf("  bufID %7x: availTo %7x, heldBy %7x, addr %7x, size %4x\n",
		   bufID, bufID->fAvailTo, bufID->fHeldBy, 
		   bufID->fAddress, bufID->fSize);
  }
  Unlock();
}

void BBufferStream::PrintSubscribers()
{
  subscriber_id subID;

  Lock();

  _debugPrintf("Entered Subscribers:   FirstSem %d\n", fFirstSem);
  for (subID = fFirstSub; subID != NULL; subID = subID->fNext) {
	_debugPrintf(" subID %7x: rel %7x, acq %7x, hld %2d sem %6d, blocked %6d\n",
				 subID, subID->fRel, subID->fAcq, subID->fHeld,
				 (subID->fSem > 0) * subID->fSem,
				 (subID->fBlockedOn >= 0) * subID->fBlockedOn);
  }
  Unlock();
}

/* ================
   Private member functions that require locking
   ================ */

/* Link bufID into the chain of buffers as the "newest" buffer.  Notify
 * the first subscriber that there's now a buffer available.
 */
status_t BBufferStream::AddBuffer(buffer_id bufID)
{
  if (bufID == NULL)
	return B_BUFFER_NOT_AVAILABLE;

  Lock();

  if (fNewestBuffer)
	fNewestBuffer->fNext = bufID;
  fNewestBuffer = bufID;

  if (fOldestBuffer == NULL)
	fOldestBuffer = bufID;

  fCountBuffers += 1;

  status_t result = ReleaseBufferTo(bufID, fFirstSub);

  SUBIDCHECK(fFirstSub);
  Unlock();
  return result;
}

/* Remove the oldest buffer from the chain.  If force is false, the oldest
 * buffer in the chain is removed only if it isn't held by anyone. If true,
 * the oldest unheld buffer is is removed.
 */
buffer_id BBufferStream::RemoveBuffer(bool force)
{
  buffer_id bufID = NULL;

  Lock();

  if (force && fOldestBuffer && fOldestBuffer->fHeldBy) {
	/* Remove oldest unheld buffer */
	buffer_id prev;
	for (prev = fOldestBuffer; prev && prev->fNext; prev = prev->fNext)
	  if (!prev->fNext->fHeldBy) {
		bufID = prev->fNext;
		prev->fNext = bufID->fNext;
		break;
	  }
  }
  else if (fOldestBuffer) {
	/* Remove oldest buffer (usual case) */
	bufID = fOldestBuffer;
	fOldestBuffer = fOldestBuffer->fNext;
  }

  if (bufID) {
	if (bufID == fNewestBuffer)	/* removing last buffer */
	  fNewestBuffer = NULL;

	/* Adjust subscriber fAcq and fRel slots (not normally necessary) */
	subscriber_id subID = bufID->fAvailTo;
	while (subID && subID->fAcq == bufID) {
	  subID->fAcq = bufID->fNext;
	  if (subID->fRel == bufID)
		subID->fRel = bufID->fNext;
	  subID = subID->fNext;
	}

	bufID->fNext = NULL;
	fCountBuffers -= 1;
  }

  Unlock();
  return bufID;
}

/* allocate a buffer from shared memory.  Returns a buffer_id that
 * is NOT yet linked into the entered buffer chain.
 */
buffer_id BBufferStream::CreateBuffer(size_t size, bool isFinal)
{
  area_id id;
  buffer_id bufID = NULL;
  char *data;
  int32 alloc_size, rem;

  if (size <= 0)
	return NULL;

  /* find a free bufID for us to use */
  Lock();
  bufID = fFreeBuffers;
  if (bufID)
	fFreeBuffers = bufID->fNext;
  Unlock();

  if (bufID == NULL)
	return NULL;

  /* Round size up to B_PAGE_SIZE */
  alloc_size = size;
  rem = alloc_size % B_PAGE_SIZE;
  if (rem > 0) 
	alloc_size += B_PAGE_SIZE - rem;

  if (bufID->fAreaSize != alloc_size) {
	if (bufID->fAreaSize > 0)
	  delete_area(bufID->fAreaID);
	bufID->fAreaID = create_area("Stream Buffer",
								 (void **)(&data),
								 B_ANY_KERNEL_ADDRESS,
								 alloc_size,
								 B_FULL_LOCK,
								 B_READ_AREA + B_WRITE_AREA);
	if (bufID->fAreaID < 0) {
	  /* If create_area returns an error, free bufID before returning. */
	  bufID->fAreaSize = 0;
	  DestroyBuffer(bufID);
	  return NULL;
	}
	bufID->fAreaSize = alloc_size;
	bufID->fAddress = data;
	memset(bufID->fAddress, 0, bufID->fAreaSize);
  }

  /* finish up */
  bufID->fSize = size;
  bufID->fAvailTo = bufID->fHeldBy = NULL;
  bufID->fAcqTime = 0;
  bufID->fIsFinal = isFinal;
  bufID->fNext = NULL;

  return bufID;
}

/* Free up the storage used by the buffer.  bufID is assumed to already
 * be removed from the entered buffer chain.
 */
void BBufferStream::DestroyBuffer(buffer_id bufID)
{
  if (bufID == NULL)
	return;

  /* clean up fields */
  bufID->fAvailTo = NULL;
  bufID->fHeldBy = NULL;

//  delete_area(bufID->fAreaID);
//  bufID->fAreaID = NULL;

  /* link into free list */
  Lock();
  bufID->fNext = fFreeBuffers;
  fFreeBuffers = bufID;
  Unlock();
}

void BBufferStream::RescindBuffers()
{
  subscriber_id subID;
  buffer_id bufID;

  Lock();

  if ((subID = fFirstSub) == NULL) {
	bufID = fOldestBuffer;		/* no subscribers at all */
  } else {
	bufID = subID->fAcq;		/* next buffer to be acquired */
  }

  if (bufID) {

	/* at this point, bufID is the "newest" buffer that can be acquired
	 * by any subscriber.  Find its predecessor (the hard way, by starting
	 * at the oldest) and break the link to bufID.
	 */
	buffer_id prevBuf = fOldestBuffer;
	while (prevBuf && (prevBuf->fNext != bufID))
	  prevBuf = prevBuf->fNext;

	fNewestBuffer = prevBuf;

	if (prevBuf) {
	  prevBuf->fNext = NULL;
	}

	/* Notify any subscribers that bufID is gone. */
	subID = fFirstSub;
	while (subID) {
	  if (subID->fAcq = bufID) subID->fAcq = NULL;
	  if (subID->fRel = bufID) subID->fRel = NULL;
	  subID = subID->fNext;
	}

	/* Free bufID and all of its successors */
	while (bufID) {
	  buffer_id nextBuf = bufID->fNext;
	  DestroyBuffer(bufID);
	  fCountBuffers -= 1;
	  bufID = nextBuf;
	}

  }
  SUBIDCHECK(subID);
  Unlock();
}


/* ================
   Private member functions that assume locking already done.
   ================ */

bool BBufferStream::LockWithTimeout(bigtime_t timeout)
{
  return (fLock.LockWithTimeout(timeout) == B_OK);
}

bool BBufferStream::Lock()
{
  return fLock.Lock();
}

void BBufferStream::Unlock()
{
  fLock.Unlock();
}

/* Set up the subscribers data structures and link them into the
 * freelist of subscribers.
 */
void BBufferStream::InitSubscribers()
{
  int32 i;
  subscriber_id subID, next;

  fFreeSubs = fFirstSub = fLastSub = NULL;

  for (i=0; i<B_MAX_SUBSCRIBER_COUNT; i+=1) {
	subscriber_id subID = &fSubscribers[i];

	/* build a circular, doubly linked list of subscribers on fFreeSubs */
	if (fFreeSubs) {			/* insert subID before fFreeSubs */
	  subID->fNext = fFreeSubs;
	  subID->fPrev = fFreeSubs->fPrev;
	  (fFreeSubs->fPrev)->fNext = subID;
	  fFreeSubs->fPrev = subID;
	} else {					/* first one */
	  fFreeSubs = subID;
	  subID->fPrev = subID;
	  subID->fNext = subID;
	}
	subID->fSem = B_BAD_SUBSCRIBER;
	subID->fTotalTime = 0;
	subID->fBlockedOn = NOT_BLOCKED;
	subID->fHeld = 0;
	fFreeSubs = subID;
  }
}

bool BBufferStream::IsSubscribedSafe(subscriber_id subID) const
{
  /* Is subID a valid pointer? */
  if ((subID < &fSubscribers[0]) ||
	  (subID >= &fSubscribers[B_MAX_SUBSCRIBER_COUNT]))
	return FALSE;

  return (subID->fSem != B_BAD_SUBSCRIBER);
}

	  
bool BBufferStream::IsEnteredSafe(subscriber_id subID) const
{
  if ((subID->fNext == NULL) && (fLastSub != subID)) return FALSE;
  if ((subID->fPrev == NULL) && (fFirstSub != subID)) return FALSE;
  return TRUE;
}

void BBufferStream::InitBuffers()
{
  int32 i;

  fFreeBuffers = fOldestBuffer = fNewestBuffer = NULL;
  fCountBuffers = 0;

  for (i=B_MAX_BUFFER_COUNT-1; i>=0; i--) {
	buffer_id bufID = &fBuffers[i];
	bufID->fNext = fFreeBuffers;
	bufID->fAvailTo = NULL;
	bufID->fHeldBy = NULL;
	bufID->fAcqTime = 0;
	bufID->fSize = 0;
	bufID->fIsFinal = FALSE;
	bufID->fAreaID = 0;
	bufID->fAreaSize = 0;
	bufID->fAddress = NULL;
	fFreeBuffers = bufID;
  }
}

/* WakeSubscriber() - wake a blocked subscriber
 *   AcquireBuffer wakes and looks for next buffer.
 */

status_t BBufferStream::WakeSubscriber(subscriber_id subID)
{
  if (subID && subID->fBlockedOn >= 0) {
	sem_id blockedOn = subID->fBlockedOn;
	subID->fBlockedOn = NOT_BLOCKED;
	return release_sem_etc(blockedOn, 1, B_DO_NOT_RESCHEDULE);
  }
  return B_NO_ERROR;
}

/* subID has just entered the stream.  As a bare minimum, this subID
 * can wait on the same buffer as its successor in the itinerary.
 * But it can also "steal" any buffers that are available to the
 * next subscriber in line.
 */
void BBufferStream::InheritBuffers(subscriber_id subID)
{
  subscriber_id nextSub = subID->fNext;
  buffer_id buf;

  if (nextSub) {
	subID->fAcq = nextSub->fAcq;
  } else {
	/* subID is the last subscriber.  Inherit all the "orphaned" buffers */
	subID->fAcq = fOldestBuffer;
  }

  /* buf is the oldest buffer subID can claim.  Re-direct any newer buffers
   * to subID.  NB: nextSub may be NULL in the event of orphaned buffers.
   */
  buf = subID->fAcq;
  while (buf && (buf->fAvailTo == nextSub)) {
	buf->fAvailTo = subID;
	buf = buf->fNext;
  }
  subID->fRel = subID->fAcq;	/* indicates no buffers acquired */
  subID->fHeld = 0;
}

/* subID is about to become inactive.  Any buffers it holds and
 * any buffers that are marked as "available" to subID must be
 * handed off to the next in line.
 */
void BBufferStream::BequeathBuffers(subscriber_id subID)
{
  status_t result;
  buffer_id buf;

  /* Release any buffers this subscriber has aquired */
  while (subID->fHeld > 0) {
	if ((result = ReleaseBufferSafe(subID)) != B_NO_ERROR) break;
  }

  /* subID->fAcq is the next buffer this subscriber could have
   * acquired (if it was still in the business).  Find all such
   * buffers and point them at the next subscriber.
   */
  buf = subID->fAcq;
  while (buf && (buf->fAvailTo == subID)) {
	buf->fAvailTo = subID->fNext;
	buf = buf->fNext;
  }
  subID->fAcq = subID->fRel = NULL;	/* indicate no buffers available */
  subID->fHeld = 0;
}

/* Release buffer, safe version.  Assumes subID is valid
 * and that the stream is locked.
 */
status_t BBufferStream::ReleaseBufferSafe(subscriber_id subID)
{
  buffer_id buf = subID->fRel;

  SUBIDCHECK(subID);

  subID->fTotalTime += system_time() - buf->fAcqTime;
  subID->fRel = buf->fNext;
  subID->fHeld -= 1;

  SUBIDCHECK(subID);

  return ReleaseBufferTo(buf, subID->fNext);
}

/* Notify subID that bufID may now be acquired by subID.  subID may
 * be NULL, which simply means that bufID has been released by the
 * last subscriber in the itinerary.  bufID is assumed non-NULL.
 */
status_t BBufferStream::ReleaseBufferTo(buffer_id bufID, subscriber_id subID)
{
  status_t result = B_NO_ERROR;

  /* tell this subscriber that there's now a buffer available to it */
  WakeSubscriber(subID);

  /* Tell this buffer it's available to the subscriber */
  bufID->fAvailTo = subID;
  bufID->fHeldBy = NULL;

  /* The buffer is not only available to the first subscriber, but to
   * all subscribers that have been waiting on a null buffer.  Thus a
   * while loop to iterate through the itinerary.
   */
  
  while (subID && (subID->fAcq == NULL)) {
	/* subID was waiting on a null buffer.  Make bufID be the next
	 * buffer subID will acquire.
	 */
	subID->fAcq = bufID;
	if (subID->fRel == NULL)
	  subID->fRel = bufID;
	SUBIDCHECK(subID);
	subID = subID->fNext;
  }

  return result;
}

void BBufferStream::FreeAllSubscribers()
{
  int32 i;
  /* A subscriber can be subscribed (assigned a sem_id), but not yet
   * entered into the stream.  So we iterate through the entire list
   * (rather than linking through subID->fNext) to make sure they
   * get unsubscribed.
   */
  for (i=0;i<B_MAX_SUBSCRIBER_COUNT;i++) {
	subscriber_id subID = &fSubscribers[i];
	Unsubscribe(subID);			/* let Unsubscribe() handle validity check */
  }
}

void BBufferStream::FreeAllBuffers()
{
  buffer_id bufID;
  subscriber_id subID;

  while ((bufID = RemoveBuffer(TRUE)) != NULL) {
	DestroyBuffer(bufID);
  }

  /* clean up all entered subscribers */
  for (subID = fFirstSub; subID != NULL; subID = subID->fNext) {
	subID->fRel = subID->fAcq = NULL;
	subID->fHeld = 0;
  }
}

void BBufferStream::_ReservedBufferStream1() {}
void BBufferStream::_ReservedBufferStream2() {}
void BBufferStream::_ReservedBufferStream3() {}
void BBufferStream::_ReservedBufferStream4() {}
