/* ================

   FILE:  AbstractStream.cpp
   REVS:  $Revision: 1.5 $
   NAME:  marc
   DATE:  Mar 03 1997

   Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.

================ */

#include <BufferStream.h>
#include <BufferMsgs.h>

//#include <stdio.h>
//#define X(N) {printf("<%d> ",(N));fflush(stdout);}

BMessenger* BAbstractBufferStream::Server() const
{
  return NULL;
}

stream_id BAbstractBufferStream::StreamID() const
{
  return NULL;
}


status_t BAbstractBufferStream::Subscribe(char* name,
										  subscriber_id* subID,
										  sem_id semID)
{
  status_t result;

  BMessage msg(SUBSCRIBE);
  msg.AddInt32("stream_id", (int32) StreamID());
  msg.AddString("name", name);
  msg.AddInt32("sem", semID);
  msg.AddBool("will_wait", FALSE);	// do we want to support this?

  BMessage reply;
  result = SendRPC(&msg, &reply);

  if (result == B_NO_ERROR)
	reply.FindInt32("subscriber_id", (int32*) subID);

  return result;
}

status_t BAbstractBufferStream::Unsubscribe(subscriber_id subID)
{
  BMessage msg(UNSUBSCRIBE);
  msg.AddInt32("stream_id", (int32) StreamID());
  msg.AddInt32("subscriber_id", (int32) subID);
  return SendRPC(&msg);
}

status_t BAbstractBufferStream::GetStreamParameters(size_t *bufferSize,
													int32 *bufferCount,
													bool *isRunning,
													int32 *subscriberCount) const
{
  status_t result;

  BMessage msg(GET_STREAM_PARAMETERS);
  msg.AddInt32("stream_id", (int32) StreamID());

  BMessage reply;
  result = SendRPC(&msg, &reply);

  if (result == B_NO_ERROR) {
	if (bufferSize)
	  reply.FindInt32("buffer_size", (int32 *)bufferSize);
	if (bufferCount)
	  reply.FindInt32("buffer_count", bufferCount);
	if (isRunning)
	  reply.FindBool("is_running", isRunning);
	if (subscriberCount)
	  reply.FindInt32("subscriber_count", subscriberCount);
  }
	  
  return result;
}


status_t BAbstractBufferStream::SetStreamBuffers(size_t bufferSize,
												 int32 bufferCount)
{
  BMessage msg(SET_STREAM_PARAMETERS);
  msg.AddInt32("stream_id", (int32) StreamID());
  msg.AddInt32("buffer_size", bufferSize);
  msg.AddInt32("buffer_count", bufferCount);
  return SendRPC(&msg);
}

status_t BAbstractBufferStream::StartStreaming()
{
  BMessage msg(SET_STREAM_PARAMETERS);
  msg.AddInt32("stream_id", (int32) StreamID());
  msg.AddBool("is_running", TRUE);
  return SendRPC(&msg);
}

status_t BAbstractBufferStream::StopStreaming()
{
  BMessage msg(SET_STREAM_PARAMETERS);
  msg.AddInt32("stream_id", (int32) StreamID());
  msg.AddBool("is_running", FALSE);
  return SendRPC(&msg);
}


/* ================
   Streaming functions.
   ================ */

status_t BAbstractBufferStream::EnterStream(subscriber_id subID,
											subscriber_id neighbor,
											bool before)
{
  BMessage msg(ENTER_STREAM);
  msg.AddInt32("subscriber_id", (int32) subID);
  msg.AddInt32("neighbor", (int32) neighbor);
  msg.AddBool("before", before);
  return SendRPC(&msg);
}

status_t BAbstractBufferStream::ExitStream(subscriber_id subID)
{
  BMessage msg(EXIT_STREAM);
  msg.AddInt32("subscriber_id", (int32) subID);
  return SendRPC(&msg);
}


/* Standard function to send msg, await reply of no arguments.  If
 * the message sends cleanly, get a reply.  If the reply->what field
 * indicates ERROR_RETURN, fetch the error code from the server's
 * reply and return that.
 */

status_t BAbstractBufferStream::SendRPC(BMessage* msg, BMessage* reply) const
{
  BMessenger* server = Server();
  if (server == NULL)
	return B_SERVER_NOT_FOUND;

  BMessage* local_reply = (reply ? reply : new BMessage());

  status_t result = server->SendMessage(msg, local_reply);
  if (result == B_NO_ERROR)
	if (local_reply->what != msg->what) {
	  result = B_BAD_REPLY;
	  if (local_reply->what == ERROR_RETURN)
		local_reply->FindInt32("error", &result);
	}

  if (!reply)
	delete local_reply;
  return result;
}

void BAbstractBufferStream::_ReservedAbstractBufferStream1() {}
void BAbstractBufferStream::_ReservedAbstractBufferStream2() {}
void BAbstractBufferStream::_ReservedAbstractBufferStream3() {}
void BAbstractBufferStream::_ReservedAbstractBufferStream4() {}
