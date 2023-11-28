/* ++++++++++

   FILE:  StreamController.cpp
   REVS:  $Revision: 1.28 $
   NAME:  r
   DATE:  Thu Jun 22 16:33:18 PDT 1995

   Copyright (c) 1995-1997 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <BufferStreamManager.h>
#include <Debug.h>
#include <R3MediaDefs.h>
#include <OS.h>
#include <string.h>


BBufferStreamManager::BBufferStreamManager(char* name) : fLock("BBufferStreamManager lock",true,true)
{
  fState = B_IDLE;
  fName = (name ? strdup(name) : NULL);

  fBufferCount = B_DEFAULT_BSTREAM_COUNT;
  fBufferSize = B_DEFAULT_BSTREAM_SIZE;
  fBufferDelay = B_DEFAULT_BSTREAM_DELAY;
  fTimeout = B_DEFAULT_BSTREAM_TIMEOUT;
  fManagerID = B_NO_SUBSCRIBER_ID;
  fSem = create_sem(0, (fName ? fName : "waiting for buffer"));
  fStream = NULL;

  fNotifyPort = B_BAD_PORT_ID;
  fProcessingThread = B_BAD_THREAD_ID;
}

BBufferStreamManager::~BBufferStreamManager()
{
  Abort();
  Lock();
  delete_sem(fSem);
  if (fName)
	free(fName);
}

char *BBufferStreamManager::Name() const
{
  return fName;
}

BBufferStream *BBufferStreamManager::Stream() const
{
  return fStream;
}

int32 BBufferStreamManager::BufferCount() const
{
  return fBufferCount;
}

void BBufferStreamManager::SetBufferCount(int32 count)
{
  if (count < 1) count = 1;
  fBufferCount = count;
}

int32 BBufferStreamManager::BufferSize() const {
  return fBufferSize;
}

/* NB: BufferSize only takes effect as new buffers are released.
 */
void BBufferStreamManager::SetBufferSize(int32 Size) {
  if (Size < 1) Size = 1;
  fBufferSize = Size;
}

bigtime_t BBufferStreamManager::BufferDelay() const {
  return fBufferDelay;
}

void BBufferStreamManager::SetBufferDelay(bigtime_t usecs) {
  fBufferDelay = usecs;
}

bigtime_t BBufferStreamManager::Timeout() const {
  return fTimeout;
}

void BBufferStreamManager::SetTimeout(bigtime_t usecs) {
  fTimeout = usecs;
}

stream_state BBufferStreamManager::Start()
{
  Lock();
  if (fState == B_IDLE) {
	StartProcessing();
  } else if (fState == B_STOPPING) {
	SetState(B_RUNNING);
  }
  Unlock();
  return fState;
}

stream_state BBufferStreamManager::Stop()
{
  Lock();
  if (fState != B_IDLE) {
	SetState(B_STOPPING);
  }
  Unlock();
  return fState;
}

stream_state BBufferStreamManager::Abort()
{
  Stop();
  StopProcessing();
  return fState;
}

stream_state BBufferStreamManager::State() const
{
  return fState;
}

port_id BBufferStreamManager::NotificationPort() const
{
  return fNotifyPort;
}

void BBufferStreamManager::SetNotificationPort(port_id port)
{
  if (port <= 0) port = B_BAD_PORT_ID;
  fNotifyPort = port;
}

bool BBufferStreamManager::LockWithTimeout(bigtime_t timeout)
{
  return (fLock.LockWithTimeout(timeout) == B_OK);
}

bool BBufferStreamManager::Lock()
{
  return fLock.Lock();
}

void BBufferStreamManager::Unlock()
{
  fLock.Unlock();
}

status_t BBufferStreamManager::Subscribe(BBufferStream* stream)
{
  status_t result = stream->Subscribe(fName, &fManagerID, fSem);
  if (result == B_NO_ERROR)
	fStream = stream;

  return result;
}

status_t BBufferStreamManager::Unsubscribe()
{
  if (fManagerID == B_NO_SUBSCRIBER_ID)
	return B_BAD_SUBSCRIBER;
  if (fStream == NULL)
	return B_STREAM_NOT_FOUND;

  status_t result = fStream->Unsubscribe(fManagerID);
  if (result == B_NO_ERROR)
	fManagerID = B_NO_SUBSCRIBER_ID;

  return result;
}

subscriber_id BBufferStreamManager::ID() const
{
  return fManagerID;
}


/****************************************************************
 * 
 * Processing Thread
 *
 */

void BBufferStreamManager::StartProcessing()
{
  while (1) {
	  fProcessingThread = spawn_thread(BBufferStreamManager::_ProcessingThread,
									   (fName ? fName : "Stream Manager"),
									   B_MEDIA_LEVEL, this);
	  if (fProcessingThread != B_INTERRUPTED)
		break;
  }
  SetState(B_RUNNING);
  //printf("BBufferStreamManager(%x)->StartProcessing()\n",this);
  resume_thread(fProcessingThread);
}

void BBufferStreamManager::StopProcessing()
{
  //printf("BBufferStreamManager(%x)->StopProcessing()\n",this);

  /* If the processing thread was blocked in a call to
   * AcquireBuffer(), UnblockSubscriber() will unblock it
   * with an error return.  This will cause the processing
   * thread to go idle soon thereafter.
   */
  fStream->UnblockSubscriber(fManagerID);

  /* wait for ProcessingThread to die */
  while (fState != B_IDLE) {
	PRINT(("BBufferStreamManager::StopProcessing() - waiting for thread to die\n"));
	snooze(1000000);
  }
}

status_t BBufferStreamManager::_ProcessingThread(void *arg)
{
  BBufferStreamManager *self = (BBufferStreamManager *)arg;
  self->ProcessingThread();
  self->StopProcessing();
  return B_NO_ERROR;
}

/* ================
 *
 * ProcessingThread() - manage the flow of buffers in the stream.
 *
 */

void BBufferStreamManager::ProcessingThread()
{
  status_t result;
  bigtime_t lastSowTime = system_time();
  bool sentLast = FALSE;
  buffer_id acqID;

  Lock();
  while (sentLast == FALSE) {

	/* =======
	 * Sow enough buffers to meet the quota.  If fState != B_RUNNING,
	 * send one last buffer with the isFinalBuffer flag set, then
	 * don't sent any more.
	 */
	while ((sentLast == FALSE) && (fStream->CountBuffers() < fBufferCount)) {
	  sentLast = (fState != B_RUNNING);
	  //if (sentLast) printf("Sowing last buffer...\n");
	  result=fStream->AddBuffer(fStream->CreateBuffer(fBufferSize, sentLast));
	  if (result != B_NO_ERROR) 
		goto egress;
	  if (fBufferDelay > 0) {
		bigtime_t now = system_time();
		if (now >= lastSowTime + fBufferDelay)
		  lastSowTime = now;
		else {
		  lastSowTime += fBufferDelay;
		  Unlock();
		  snooze(lastSowTime - now);
		  Lock();
		}
	  }
	  //printf("<%d,%d,%d> ",fState,fStream->CountBuffers(),sentLast);fflush(stdout);
	}


	/* ========
	 * Reap what we've sown.  Reap at least one buffer.  If we sowed the
	 * last buffer (above), don't stop until we've reaped all the buffers.
	 */
	while (fStream->CountBuffers() > 0) {
	  Unlock();
	  result = fStream->AcquireBuffer(fManagerID, 
									  &acqID, 
									  fTimeout);
	  Lock();
	  if (result != B_NO_ERROR) goto egress;

	  /* Release the buffer.  Since fManagerID is the last subscriber in the
	   * itinerary, this has the effect of releasing the buffer "off the end of
	   * the world" so that no other subscriber can or will acquire it.  Then
	   * it's safe to remove the buffer from the stream and free it.
	   */
	  result = fStream->ReleaseBuffer(fManagerID);
	  if (result != B_NO_ERROR) break;
	  acqID = fStream->RemoveBuffer(FALSE);
	  fStream->DestroyBuffer(acqID);

	  /* If the user has called Start() and fState has reverted to
	   * B_RUNNING, clear the sentLast flag - we're back in business...
	   */
	  if (sentLast && fState == B_RUNNING)
		sentLast = FALSE;

	  /* Quit now if if we've reduced the number of the buffers in the
	   * stream to at least one less than fBufferCount.  If we've sent
	   * the last buffer, keep reaping until there are zero in the stream.
	   */
	  // printf("[%d,%d,%d] ",fState,fStream->CountBuffers(),sentLast);fflush(stdout);
	  if ((sentLast == FALSE) && (fStream->CountBuffers() < fBufferCount))
		break;
	}

  } /* while (sentLast == FALSE) */

egress:
  //printf("Leaving ProcessingThread() with status %x %s\n", result, fName);
  //if (result == B_TIMED_OUT) {printf ("TIMED OUT!\n"); fStream->PrintStream();}
  SetState(B_IDLE);
  Unlock();
}

void BBufferStreamManager::SetState(stream_state newState)
{
  if (newState != fState) {
	fState = newState;

	if (fNotifyPort != B_BAD_PORT_ID) {
	  while (write_port(fNotifyPort, fState, NULL, 0) == B_INTERRUPTED)
		;
	}
  }
}

/* OBSOLETE */

bigtime_t BBufferStreamManager::SnoozeUntil(bigtime_t time)
{
  bigtime_t now = system_time();
  if (now >= time)
	return now;
  snooze(time - now);
  return time;
}

void BBufferStreamManager::_ReservedBufferStreamManager1() {}
void BBufferStreamManager::_ReservedBufferStreamManager2() {}
void BBufferStreamManager::_ReservedBufferStreamManager3() {}
