/* ++++++++++
   
   FILE:  AudioStream.cpp
   REVS:  $Revision: 1.25 $
   NAME:  r
   DATE:  Fri Jun 09 09:30:58 PDT 1995
   
   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
   
   +++++ */

#include <AudioStream.h>
#include <AudioMsgs.h>
#include <Debug.h>
#include <Errors.h>
#include <R3MediaDefs.h>
#include <Roster.h>


BADCStream::BADCStream () : fServer(NULL), fStreamID(NULL)
{
  BMessenger* server = new BMessenger(AUDIO_SERVER_ID);;
  status_t result;

  if (!server->IsValid()) {
	result = be_roster->Launch(AUDIO_SERVER_ID);
	if (result != B_NO_ERROR) {
	  PRINT(("BADCStream: Couldn't launch server.  Error=%x\n", result));
	  return;
	}
	server = new BMessenger(AUDIO_SERVER_ID);
	if (!server->IsValid()) {
	  PRINT(("BADCStream: Couldn't connect to server.  Error=%x\n", result));
	  return;
	}
  }

  fServer = server;
  BMessage msg(GET_STREAM_ID);
  msg.AddInt32("resource", B_ADC_STREAM);

  BMessage reply;
  result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	reply.FindInt32("stream_id", (int32*) &fStreamID);
  else
	PRINT(("BADCStream: GET_STREAM_ID failed.  Error=%x\n", result));
}

BADCStream::~BADCStream()
{
  fStreamID = NULL;
  delete Server();
  fServer = NULL;
}

status_t
BADCStream::ADCInput(int32* device) const
{
  int32 response = 0;
  BMessage msg(GET_ADC_INPUT);
  BMessage reply;
  status_t result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	if (reply.FindInt32("adc_input", &response) == B_NO_ERROR
		&& device)
	  *device = response;
	else
	  result = B_BAD_REPLY;

  return result;
}

status_t
BADCStream::SetADCInput(int32 device)
{
  BMessage msg(SET_ADC_INPUT);
  msg.AddInt32("adc_input", device);
  return SendRPC(&msg);
}

status_t
BADCStream::SamplingRate(float* srate) const
{
  int32 response = 0;
  BMessage msg(GET_SAMPLING_RATE);
  BMessage reply;
  status_t result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	if (reply.FindInt32("sampling_rate", &response) == B_NO_ERROR && srate)
	  *srate = (float) response;
	else
	  result = B_BAD_REPLY;

  return result;
}

status_t
BADCStream::SetSamplingRate(float srate)
{
  BMessage msg(SET_SAMPLING_RATE);
  msg.AddInt32("sampling_rate", (int32) srate);
  return SendRPC(&msg);
}

bool
BADCStream::IsMicBoosted() const
{
  bool response = FALSE;
  BMessage msg(GET_MIC_BOOST);
  BMessage reply;
  status_t result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	reply.FindBool("mic_boost", &response);

  return response;
}

status_t
BADCStream::BoostMic(bool boost)
{
  BMessage msg(SET_MIC_BOOST);
  msg.AddBool("mic_boost",boost);
  return SendRPC(&msg);
}
	
BMessenger*
BADCStream::Server() const
{
  return fServer;
}

stream_id
BADCStream::StreamID() const
{
  return fStreamID;
}

status_t
BADCStream::SetStreamBuffers(size_t bufferSize, int32 bufferCount)
{
  /* enforce multiple of 4 and minimum of 64 */

  int32 newSize = 4 * ((bufferSize + 3) / 4);
  if (newSize < 64)
	newSize = 64;
  return BAbstractBufferStream::SetStreamBuffers(newSize, bufferCount);
}

	
BDACStream::BDACStream () : fServer(NULL), fStreamID(NULL)
{
  BMessenger* server = new BMessenger(AUDIO_SERVER_ID);;
  status_t result;

  if (!server->IsValid()) {
	result = be_roster->Launch(AUDIO_SERVER_ID);
	if (result != B_NO_ERROR) {
	  PRINT(("BDACStream: Couldn't launch server.  Error=%x\n", result));
	  return;
	}
	server = new BMessenger(AUDIO_SERVER_ID);
	if (!server->IsValid()) {
	  PRINT(("BDACStream: Couldn't connect to server.  Error=%x\n", result));
	  return;
	}
  }

  fServer = server;
  BMessage msg(GET_STREAM_ID);
  msg.AddInt32("resource", B_DAC_STREAM);

  BMessage reply;
  result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	reply.FindInt32("stream_id", (int32*) &fStreamID);
  else
	PRINT(("BDACStream: GET_STREAM_ID failed.  Error=%x\n", result));
}

BDACStream::~BDACStream()
{
  fStreamID = NULL;
  delete Server();
  fServer = NULL;
}

status_t
BDACStream::SamplingRate(float* srate) const
{
  int32 response = 0;
  BMessage msg(GET_SAMPLING_RATE);
  BMessage reply;
  status_t result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	if (reply.FindInt32("sampling_rate", &response) == B_NO_ERROR && srate)
	  *srate = (float) response;
	else
	  result = B_BAD_REPLY;

  return result;
}

status_t
BDACStream::SetSamplingRate(float srate)
{
  BMessage msg(SET_SAMPLING_RATE);
  msg.AddInt32("sampling_rate", (int32) srate);
  return SendRPC(&msg);
}

status_t
BDACStream::GetVolume(int32 device,
					  float *l_volume, 
					  float *r_volume, 
					  bool *enabled) const
{
	stereo_facts *sf;
	int32 numBytes;

	if ((device < 0) || (device >= B_SOUND_DEVICE_END))
	  return B_BAD_VALUE;

	BMessage msg(GET_SOUND_HARDWARE_INFO);
	msg.AddInt32("device", device);
	BMessage reply;
	status_t result = SendRPC(&msg, &reply);

	if (result == B_NO_ERROR)
	  if (reply.FindData("stereo_facts", B_STEREO_FACTS_TYPE,
				(const void **)&sf, &numBytes) == B_NO_ERROR) {
		if (l_volume)
		  *l_volume = sf->left;
		if (r_volume)
		  *r_volume = sf->right;
		if (enabled)
		  *enabled = !sf->mute;
	  }
	  else
		result = B_BAD_REPLY;

	return result;
}

status_t
BDACStream::SetVolume(int32 device, float l_volume, float r_volume)
{
	stereo_facts sf;
	stereo_consider se;

	if ((device < 0) || (device >= B_SOUND_DEVICE_END))
	  return B_BAD_VALUE;

	sf.left = l_volume;
	sf.right = r_volume; 
	sf.mute = 0;

	se.left = TRUE;
	se.right = TRUE;
	se.mute = FALSE;
	
	BMessage msg(SET_SOUND_HARDWARE_INFO);
	msg.AddInt32("device", device);
	msg.AddData("stereo_facts", 
				B_STEREO_FACTS_TYPE, 
				(void *)&sf, 
				sizeof(stereo_facts));
	msg.AddData("stereo_consider",
				B_STEREO_CONSIDER_TYPE,
				(void *)&se,
				sizeof(stereo_consider));

	return SendRPC(&msg);
}

bool
BDACStream::IsDeviceEnabled(int32 device) const
{
  stereo_facts *sf;
  int32 numBytes;

  if ((device < 0) || (device >= B_SOUND_DEVICE_END))
	return FALSE;

  BMessage msg(GET_SOUND_HARDWARE_INFO);
  BMessage reply;
  msg.AddInt32("device", device);
  status_t result = SendRPC(&msg, &reply);
  if (result == B_NO_ERROR)
	if (reply.FindData("stereo_facts", B_STEREO_FACTS_TYPE,
				(const void **)&sf, &numBytes) == B_NO_ERROR)
	  return !(sf->mute);

  return FALSE;
}

status_t
BDACStream::EnableDevice(int32 device, bool enable)
{
	stereo_facts sf;
	stereo_consider se;

	if ((device < 0) || (device >= B_SOUND_DEVICE_END))
	  return B_BAD_VALUE;

	sf.left = B_NO_CHANGE;
	sf.right = B_NO_CHANGE;
	sf.mute = !enable;

	se.left = FALSE;
	se.right = FALSE;
	se.mute = TRUE;

	BMessage msg(SET_SOUND_HARDWARE_INFO);
	msg.AddInt32("device", device);
	msg.AddData("stereo_facts", 
				B_STEREO_FACTS_TYPE, 
				(void *)&sf, 
				sizeof(stereo_facts));
	msg.AddData("stereo_consider",
				B_STEREO_CONSIDER_TYPE,
				(void *)&se,
				sizeof(stereo_consider));

	return SendRPC(&msg);
}
	
BMessenger*
BDACStream::Server() const
{
  return fServer;
}

stream_id
BDACStream::StreamID() const
{
  return fStreamID;
}

status_t
BDACStream::SetStreamBuffers(size_t bufferSize, int32 bufferCount)
{
  /* enforce multiple of 4 and minimum of 64 */

  int32 newSize = 4 * ((bufferSize + 3) / 4);
  if (newSize < 64)
	newSize = 64;
  return BAbstractBufferStream::SetStreamBuffers(newSize, bufferCount);
}

void BADCStream::_ReservedADCStream1() {}
void BADCStream::_ReservedADCStream2() {}
void BADCStream::_ReservedADCStream3() {}
void BDACStream::_ReservedDACStream1() {}
void BDACStream::_ReservedDACStream2() {}
void BDACStream::_ReservedDACStream3() {}
