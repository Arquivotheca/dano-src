/* ================

   FILE:  Subscriber.cpp
   REVS:  $Revision: 1.37 $
   NAME:  r
   DATE:  Mon Jun 05 20:30:00 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

================ */

#include <R3MediaDefs.h>
#include <Subscriber.h>
#include <BufferMsgs.h>
#include <string.h>

//#include <stdio.h>
//#define X(N) {printf("<%d> ",(N));fflush(stdout);}

BSubscriber::BSubscriber(const char *name)
{
  if (name && *name)
	fName = strdup(name);
  else
	fName = "";

  fStream = NULL;
  fSubID = B_NO_SUBSCRIBER_ID;
  fSem = create_sem(0, (*fName ? fName : "waiting for buffer"));

  fUserData = NULL;
  fStreamFn = (enter_stream_hook)NULL;
  fCompletionFn = (exit_stream_hook)NULL;
  fCallStreamFn = FALSE;
  fTimeout = 0;
  fSynchLock = create_sem(1, "subscriber synch lock");
  fBackThread = B_BAD_THREAD_ID;
  fFileID = -1;
}

BSubscriber::~BSubscriber()
{
	if (IsInStream())
	  ExitStream(TRUE);

	Unsubscribe(); 

	delete_sem(fSem);
	delete_sem(fSynchLock);

	if (*fName)
	  free(fName);
}

subscriber_id BSubscriber::ID() const
{
	return fSubID;
}

const char *BSubscriber::Name(void) const
{
	return fName;
}

status_t BSubscriber::Subscribe(BAbstractBufferStream* stream)
{
  status_t result = stream->Subscribe(fName, &fSubID, fSem);
  if (result == B_NO_ERROR)
	fStream = stream;

  return result;
}

status_t BSubscriber::Unsubscribe()
{
  ExitStream(TRUE);

  if (fSubID == B_NO_SUBSCRIBER_ID)
	return B_BAD_SUBSCRIBER;
  if (fStream == NULL)
	return B_STREAM_NOT_FOUND;

  status_t result = fStream->Unsubscribe(fSubID);
  if (result == B_NO_ERROR) {
	fSubID = B_NO_SUBSCRIBER_ID;
	fStream = NULL;
  }

  return result;
}

bigtime_t BSubscriber::Timeout(void) const
{
  return (fTimeout);
}

void BSubscriber::SetTimeout(bigtime_t microseconds)
{
  fTimeout = microseconds;
}


/* ================
   Streaming functions.
   ================ */

status_t BSubscriber::EnterStream(subscriber_id neighbor,
								  bool before,
								  void *userData,
								  enter_stream_hook streamFunction,
								  exit_stream_hook completionFunction,
								  bool background)
{
  if (fSubID == B_NO_SUBSCRIBER_ID)
	return B_BAD_SUBSCRIBER;
  if (fStream == NULL)
	return B_STREAM_NOT_FOUND;

  /* ask the server to plunge us into the flowing torrent of buffers */
  status_t result = fStream->EnterStream(fSubID, neighbor, before);
  if (result != B_NO_ERROR)
	return result;

  fCallStreamFn = TRUE;
  fUserData = userData;
  fStreamFn = streamFunction;
  fCompletionFn = completionFunction;
  
  if (background == FALSE) {	/* run ProcessLoop in this thread */
	while (acquire_sem(fSynchLock) == B_INTERRUPTED)
		;
	result = ProcessLoop();
	release_sem(fSynchLock);
  }
  else {						/* run ProcessLoop in separate thead */
	while (1) {
		fBackThread = spawn_thread(BSubscriber::_ProcessLoop,
								   (*fName ? fName : "Stream Subscriber"),
								   B_MEDIA_LEVEL, this);
		if (fBackThread != B_INTERRUPTED)
			break;
	}
	result = resume_thread(fBackThread);
  }
  return result;
}

/* ExitStream() arranges to stop calling the user function and
 * informs the server that we wish to get out of the stream.
 * Note, however, that we must continue to run the processing
 * loop (below) until AcquireBuffer() tells us that there are
 * no more buffers available to us.
 */
status_t BSubscriber::ExitStream(bool synch)
{
  fCallStreamFn = FALSE;

  if (fSubID == B_NO_SUBSCRIBER_ID)
	return B_BAD_SUBSCRIBER;
  if (fStream == NULL)
	return B_STREAM_NOT_FOUND;

  status_t result = fStream->ExitStream(fSubID);

  if (synch) {
	if (fBackThread != B_BAD_THREAD_ID) {
	  status_t retval;
	  while (wait_for_thread(fBackThread, &retval) == B_INTERRUPTED)
			;
	  fBackThread = B_BAD_THREAD_ID;
	}
	else {
	  // Rare--but ExitStream could be called from a separate
	  // thread.
	  while (acquire_sem(fSynchLock) == B_INTERRUPTED)
		;
	  release_sem(fSynchLock);
	}
  }
  return result;
}

bool BSubscriber::IsInStream(void) const
{
  if (fSubID == B_NO_SUBSCRIBER_ID)
	return B_BAD_SUBSCRIBER;
  if (fStream == NULL)
	return B_STREAM_NOT_FOUND;

  /* It's not worth locking the stream.  The only 
   * way we can get stung, here, is if the subscriber is deleted
   * (and so its id becomes invalid) between the fSubID check above
   * and the IsEntered call below.  But in that case, we're 
   * pretty much done for anyway.
   */
  stream_id stream = fStream->StreamID();
  return (stream && stream->IsEntered(fSubID));
}


/* ================================================================
   The Processing Loop
   
   The processing loop is responsible for receiving buffers from the
   buffer stream and calling the user-supplied StreamFunction.  It
   may be called from the callers thread or spawned in a separate
   thread.
   ================================================================ */

/* trampoline function called from spawn_thread() */
status_t BSubscriber::_ProcessLoop(void *arg)
{
  BSubscriber *self = (BSubscriber *) arg;
  self->ProcessLoop();
  return B_NO_ERROR;
}

/* Arrive here with everything ready to go, entered into the stream.
 * Once we're receiving buffers, we are obliged to stay in the stream,
 * acquring and releasing buffers, until a call to AcquireBuffer()
 * gives us an error return, indicating that it's okay to quit the
 * stream.
 */
status_t BSubscriber::ProcessLoop()
{
  buffer_id bufID;
  status_t result;
  bool exitCalled = FALSE;
  stream_id stream = fStream->StreamID();
  uint32 headerSize = stream->HeaderSize();

  //buffer_request br;
  //br.fSubID = fSubID;
  //br.fTimeout = fTimeout;
  //br.fBufID = &bufID;

  while (1) {
	result = stream->AcquireBuffer(fSubID,&bufID,fTimeout);
	if (result != B_NO_ERROR)
	  break;

	if (!fStreamFn)
	  fCallStreamFn = FALSE;

	if (fCallStreamFn) {
	  char* data = stream->BufferData(bufID);
	  uint32 size = stream->BufferSize(bufID);
	  if (!fStreamFn(fUserData, data + headerSize, size - headerSize, data))
		fCallStreamFn = FALSE;
	}

	/* forward the buffer to the next subscriber */
	result = stream->ReleaseBuffer(fSubID);

	if (result != B_NO_ERROR)
	  fCallStreamFn = FALSE;

	/* Call ExitStream() as needed to tell the server to get us
	 * out of the stream, but continue to acquire and release
	 * buffers until AcquireBuffer() indicates that there are no
	 * more buffers.
	 */
	if (fCallStreamFn == FALSE)
	  if (exitCalled == FALSE) {
		ExitStream();
		exitCalled = TRUE;
	  }
  }
  
  /* If we got out of the loop without getting out of the stream,
   * we're probably hosed, but we should at least try.
   */
  if (!exitCalled)
	ExitStream();

  /* Call the user's completion function if there is one.  
   */

  if (fCompletionFn) 
	return (*fCompletionFn)(fUserData, result);
  else
	return result;
}

BAbstractBufferStream* BSubscriber::Stream() const
{
  return fStream;
}

void BSubscriber::_ReservedSubscriber1() {}
void BSubscriber::_ReservedSubscriber2() {}
void BSubscriber::_ReservedSubscriber3() {}
