#include "MediaNodeController.h"

#include <Application.h>
#include <AutoLock.h>
#include <Debug.h>
#include <MediaKit.h>
#include <Screen.h>

#include <stdio.h>
#include <malloc.h>

#include "DrawingTidbits.h"
#include "MediaPlayerApp.h"
#include "MediaController.h"
#include "NodeWrapper.h"
#include "VideoView.h"
#include "VideoConsumerNode.h"
#include "MultiThumbMediaSlider.h"
#include "debug.h"


class LoopUnroller {
public:
	LoopUnroller(BLooper *looper);
	~LoopUnroller();

private:
	BLooper *looper;
	int32 count;
};

LoopUnroller::LoopUnroller(BLooper *looper)
	:	looper(looper),
		count(0)
{	
	count = looper->CountLocks();
	for (int32 tmp = count ; tmp--; ) {
		if (!looper->IsLocked()) {
			TRESPASS();
			break;
		}
		looper->Unlock();
	}
}

LoopUnroller::~LoopUnroller()
{
	 while (count--)
		if (!looper->Lock()) {
			TRESPASS();
			return;
		}

	ASSERT(looper->IsLocked());
}


// ToDo:
// make file opening more asynchronous
// send back a message to PlayerWindow once file gets opened


const bigtime_t kScrubCheckPeriod = 50000;
	// during scrubbing MediaNodeController will check the slider this much
	// for changes

MediaNodeController::MediaNodeController()
	:	fFileInterface(0),
		fNodeChain(10, false),
		fVideoConsumerNode(0),
		fCurrentPosition(0),
		scheduledCommand(kIdle),
		fMixerWeb(0),
		fVolume(0),
		fLooping(false)
{
}

MediaNodeController::~MediaNodeController()
{
	writelog("Closing media file\n");
	AutoLock<BLooper> lock(this);
	
	delete fMixerWeb;
	fMixerWeb = 0;
	fVolume = 0;

	// Stop and disconnect all nodes first...
	writelog("Stopping nodes\n");
	int32 count = fNodeChain.CountItems();
	for (int index = 0; index < count; index++)
		if ((index != 0 || IsPlaying()) && fNodeChain.ItemAt(index)->CanStop()) {
			writelog("stopping node %d\n", index);
			fNodeChain.ItemAt(index)->SyncStopNow();
			writelog("stopped node %d\n", index);
		}

	writelog("Disconnecting nodes\n");
	for (int index = 0; index < count; index++) 
		fNodeChain.ItemAt(index)->Disconnect();
	
	// Then release them.
	writelog("Releasing nodes\n");
	NodeWrapper *node = fNodeChain.RemoveItemAt(0);
	while (node) {
		delete node;
		node = fNodeChain.RemoveItemAt(0);
	}
	
	if (fVideoConsumerNode) {
		BMediaRoster::Roster()->ReleaseNode(fVideoConsumerNode->Node());
		// Jeff - Why can't I delete the node here?
		// delete fVideoConsumerNode;
	}

	writelog("Close finished\n");
}

bool 
MediaNodeController::CanHandle(const entry_ref&)
{
	return true;
}


MediaController *
MediaNodeController::Open(const entry_ref &ref, BLooper *owner,
	status_t *error, BString *failingFormatDescription)
{
	if (!CanHandle(ref)) {
		if (error)
			*error = B_MEDIA_NO_HANDLER;
		return 0;
	}

	MediaNodeController *result = new MediaNodeController();
	result->Run();
	result->SetTo(ref, failingFormatDescription);
	result->SetTarget(owner, owner);

	if (error) 
		*error = result->InitCheck();

	return result;
}



void 
MediaNodeController::Delete(BLooper *owner)
{
	SetVolume(0);
	// avoid crackle while quitting sound
	
	
	// This *must* unlock the owner so that the output node can
	// flush all of its buffers and the nodes can be disconnected
	// properly.
	LoopUnroller unroller(owner);
	thread_id thread = Thread();
	Lock();
	Quit();
	// carefull, no this after here
	
	// gotta wait here for MediaController to completely go away
	// before we can relock the window
	wait_for_thread(thread, &thread);
}

void 
MediaNodeController::Quit()
{
	Unlock();
	scheduledCommand = kQuit;
	Lock();
	MediaController::Quit(); 
}

status_t
MediaNodeController::ConnectVideoOutput(VideoView *view)
{
	media_node timeSource;
	status_t err = BMediaRoster::Roster()->GetTimeSource(&timeSource);
	if (err != B_OK) 
		return err;

	NodeWrapper *node;

	try {
		ASSERT(fVideoConsumerNode == 0);
		fVideoConsumerNode = new VideoConsumerNode(view->Name(), view, &timeSource);
		node = new NodeWrapper(fVideoConsumerNode->Node(), false, true);

		writelog("Connecting video output\n");
		ASSERT(ReadyToPlay());
		AutoLock<BLooper> lock(this);
		
		status_t error;
		if (fFileInterface->HasOutputType(B_MEDIA_RAW_VIDEO)) {
			writelog("File interface has raw video output, connect directly\n");
			error = fFileInterface->ConnectOutput(node, B_MEDIA_RAW_VIDEO);
			if (error != B_OK) {
				writelog("MediaNodeController: connecting file interface to video output failed: %s\n",
					strerror(error));
				return error;
			}
		} else if (fFileInterface->HasOutputType(B_MEDIA_ENCODED_VIDEO)) {
			writelog("File interface has a encoded output.  Search for decoder node\n");
			media_format videoIn;
			fFileInterface->GetOutputFormat(B_MEDIA_ENCODED_VIDEO, &videoIn);
		
			// Find a codec that can handle this format.
			media_format videoOut;
			videoOut.type = B_MEDIA_RAW_VIDEO;
			videoOut.u.raw_video = media_raw_video_format::wildcard;
			NodeWrapper *videoCodec = NodeWrapper::InstantiateNode(videoIn, videoOut);	
			if (videoCodec == 0) {
				writelog("Couldn't find a codec that handles this format.\n");
				return B_MEDIA_NO_HANDLER;
			}
	
			fNodeChain.AddItem(videoCodec);
	
			writelog("Connect output of video codec to video output\n");	
			error = fFileInterface->ConnectOutput(videoCodec, B_MEDIA_ENCODED_VIDEO, &videoIn);
			if (error != B_OK) {
				writelog("Error %s occured connecting file reader to codec\n",
					strerror(error));
				return error;
			}
			
			error = videoCodec->Start();	
			if (error != B_OK) {
				writelog("Error %s occured starting video codec\n", strerror(error));
				return error;
			}
			
			media_format viewFormat;
			viewFormat.type = B_MEDIA_RAW_VIDEO;
	
			writelog("Connecting output of video codec to video view\n");
	
			// Ask for the same color format as the display		
			viewFormat.u.raw_video.display.format = BScreen().ColorSpace();
			error = videoCodec->ConnectOutput(node, B_MEDIA_RAW_VIDEO, &viewFormat);
			if (error != B_OK) {
				// Fine, you pick it then.
				viewFormat.u.raw_video.display.format = media_raw_video_format::wildcard.display.format;
				error = videoCodec->ConnectOutput(node, B_MEDIA_RAW_VIDEO, &viewFormat);
				if (error != B_OK) {
					writelog("Error %s occured connecting codec output to video output, give up\n",
						strerror(error));
	
					return error;
				}
			}
	
			fDescriptionString << "Using video codec: " << videoCodec->Name() << "\n";
		}
	
		fNodeChain.AddItem(node);
	
		writelog("Starting video output\n"); 
		error = node->Start();
		if (error != B_OK) {
			writelog("Error %s occured starting video output node\n", strerror(error));
			return error;
		}
	
	}
	catch (status_t error) {
		return error;
	}
	
	catch (...) {
		TRESPASS();
		return B_ERROR;
	}

	writelog("Connected video output ok\n");
	return B_OK;
}

status_t 
MediaNodeController::SetTo(const entry_ref &ref, BString *)
{
	writelog("MediaNodeController: SetTo %s\n", ref.name);
	
	// sniff the file and check if we have a handler
	dormant_node_info fileNodeInfo;		
	status_t error = B_OK;
	error = BMediaRoster::Roster()->SniffRef(ref, 0, &fileNodeInfo);
	if (error != B_OK) {
		// no handler, bail out quick
		writelog("Couldn't find a handler for this file, %s\n", strerror(error));
		fInitCheckVal = error;
		return error;
	}

	// pass file to looper thread to open
	BMessage message(B_REFS_RECEIVED);
	message.AddRef("refs", &ref);
	message.AddData("medaPlayer:dormantNodeInfo", B_RAW_TYPE, &fileNodeInfo,
		sizeof(dormant_node_info));
	PostMessage(&message);
	return B_OK;
}

void
MediaNodeController::_SetTo(const entry_ref &ref, const dormant_node_info *nodeInfo)
{
	BMediaRoster *roster = BMediaRoster::Roster();
	
	writelog("MediaNodeController: _SetTo %s\n", ref.name);
	ASSERT(IsLocked());

	dormant_node_info fileNodeInfo;
	memcpy(&fileNodeInfo, nodeInfo, sizeof(fileNodeInfo));
	
	fDescriptionString = "";
	fDescriptionString << ref.name << "\n"
		<< "File Handler is " << fileNodeInfo.name  << "\n";
	
	writelog("Instantiating file handler %s\n", fileNodeInfo.name);
	media_node fileNode;
	fInitCheckVal = roster->InstantiateDormantNode(fileNodeInfo, &fileNode);
	if (fInitCheckVal != B_OK) {
		writelog("Couldn't instantiate a node to handle the file: %s\n",
			strerror(fInitCheckVal));
		return;
	}
	
	fInitCheckVal = BMediaRoster::Roster()->SetRefFor(fileNode, ref, false, &fLength);
	if (fInitCheckVal != B_OK) {
		writelog("Couldn't set file ref: %s\n", strerror(fInitCheckVal));
		return;
	}
	
	fInPoint = 0;
	fOutPoint = fLength;
	
	try {
		fFileInterface = new NodeWrapper(fileNode, true, true);
	} catch (status_t error) {
		fInitCheckVal = error;
		return;
	}

	fNodeChain.AddItem(fFileInterface);

	// This may not continue to be true if this is some multistream
	// encoded format.
	fCurrentSourceNode = fFileInterface;	

	// Find a time source for the file reader
	fTimeSource = roster->MakeTimeSourceFor(fileNode);
	if (fTimeSource == 0) {
		writelog("MakeTimeSourceFor() failed\n");
		fInitCheckVal = B_MEDIA_SYSTEM_FAILURE;
		return;
	}

	// start it if needed
	if (!fTimeSource->IsRunning()) {
		writelog("Hmmmm... Timesource isn't running.  Try to start it.\n");
		status_t err = roster->StartNode(fTimeSource->Node(), BTimeSource::RealTime());
			if (err < B_OK)
				writelog("can't start timesource: %lx\n", err);
		writelog("prerolling timesource.\n");
		roster->PrerollNode(fTimeSource->Node());
	}
	
	//
	// Check for an encoded stream...
	//
	if (fFileInterface->HasOutputType(B_MEDIA_MULTISTREAM)) {
		writelog("This is a multistream format\n");
		fDescriptionString << "Multistream file.\n";
		
		media_format format;
		fInitCheckVal = fFileInterface->GetOutputFormat(B_MEDIA_MULTISTREAM, &format);
		if (fInitCheckVal != B_OK)
			return;
			
		NodeWrapper *streamDecoder = NodeWrapper::InstantiateNode(format);
		if (streamDecoder == 0) {
			fInitCheckVal = B_MEDIA_NO_HANDLER;
			return;
		}
			
		fNodeChain.AddItem(streamDecoder);

		fInitCheckVal = fFileInterface->ConnectOutput(streamDecoder, B_MEDIA_MULTISTREAM);
		if (fInitCheckVal != B_OK) 
			return;

		fCurrentSourceNode = streamDecoder;	
		fDescriptionString << "Encoded Videos\n";
	}

	//
	//	Try to massage out some audio output
	//
	fHasAudio = true;
	if (fCurrentSourceNode->HasOutputType(B_MEDIA_ENCODED_AUDIO)) {
		writelog("Looking for audio output\n");
		media_format audioIn;
		fCurrentSourceNode->GetOutputFormat(B_MEDIA_ENCODED_AUDIO, &audioIn);

		fDescriptionString << "Encoded Audio\n";
		
		media_format audioOut;
		audioOut.type = B_MEDIA_RAW_AUDIO;
		audioOut.u.raw_audio = media_raw_audio_format::wildcard;
		NodeWrapper *audioCodec = NodeWrapper::InstantiateNode(audioIn, audioOut);	
		if (audioCodec == 0) {
			fInitCheckVal = B_MEDIA_NO_HANDLER;
			return;
		}

		fDescriptionString << "Using audio codec: " << audioCodec->Name() << "\n"; 

		fNodeChain.AddItem(audioCodec);		

		writelog("Connect file reader to audio codec\n");
		fInitCheckVal = fCurrentSourceNode->ConnectOutput(audioCodec, B_MEDIA_ENCODED_AUDIO);
		if (fInitCheckVal != B_OK) {
			writelog("Couldn't connect audio codec to source node: %s\n", strerror(fInitCheckVal));
			return;
		} 

		audioCodec->Start();

		writelog("Hook up mixer\n");
		fInitCheckVal = HookUpMixer(audioCodec);
		if (fInitCheckVal != B_OK)
			return;
	} else if (fCurrentSourceNode->HasOutputType(B_MEDIA_RAW_AUDIO)) {
		writelog("File interface has raw audio output\n");
		fInitCheckVal = HookUpMixer(fCurrentSourceNode);
		if (fInitCheckVal != B_OK)
			return;
	} else
		fHasAudio = false;

	// Just kinda figure out what video we have	
	if (fFileInterface->HasOutputType(B_MEDIA_RAW_VIDEO)) {
		media_format videoFormat;
		fFileInterface->GetOutputFormat(B_MEDIA_RAW_VIDEO, &videoFormat);
		fVideoWidth = videoFormat.u.raw_video.display.line_width;
		fVideoHeight = videoFormat.u.raw_video.display.line_count;
		fVideoFramePeriod = (bigtime_t)(1000000 /
			videoFormat.u.raw_video.field_rate);
		fHasVideo = true;
		fDescriptionString << (int32) fVideoWidth << " x " << (int32) fVideoHeight << ",  "
			<< (int32) videoFormat.u.raw_video.field_rate << " fps\n";
	} else if (fFileInterface->HasOutputType(B_MEDIA_ENCODED_VIDEO)) {
		media_format videoFormat;
		fFileInterface->GetOutputFormat(B_MEDIA_ENCODED_VIDEO, &videoFormat);
		fVideoWidth = videoFormat.u.encoded_video.output.display.line_width;
		fVideoHeight = videoFormat.u.encoded_video.output.display.line_count;
		fVideoFramePeriod = (bigtime_t)(1000000.0 /
			videoFormat.u.encoded_video.output.field_rate);
		fHasVideo = true;
		fDescriptionString << (int32) fVideoWidth << " x " << (int32) fVideoHeight << ",  "
			<< (int32) videoFormat.u.encoded_video.output.field_rate << " fps\n";
	} else {
		fVideoWidth = 0;
		fVideoHeight = 0;
		fVideoFramePeriod = 0;
		fHasVideo = false;
	}

	writelog("opened using MediaNodeController\n");
	fReadyToPlay = true;
	writelog("_SetTo finished ok\n");
	return;
}


status_t 
MediaNodeController::HookUpMixer(NodeWrapper *node)
{
	writelog("Hooking up mixer to output of file\n");
	ASSERT(IsLocked());

	BMediaRoster *roster = BMediaRoster::Roster();	
	media_node mixerNode;
	status_t err = roster->GetAudioMixer(&mixerNode);
	if (err != B_OK) {
		writelog("Couldn't find audio mixer\n");
		return err;
	}

	NodeWrapper *mixer = 0;
	
	try {
		mixer = new NodeWrapper(mixerNode, false, false);
	} catch (status_t error) {
		return error;
	}
	
	fNodeChain.AddItem(mixer);

	media_format outputFormat;
	node->GetOutputFormat(B_MEDIA_RAW_AUDIO, &outputFormat);	
	switch (outputFormat.u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR:
		fDescriptionString << "8 Bit ";
		break;
	
	case media_raw_audio_format::B_AUDIO_SHORT:
		fDescriptionString << "16 Bit ";
		break;
	case media_raw_audio_format::B_AUDIO_FLOAT:
		fDescriptionString << "32 Bit (float) ";
		break;
	case media_raw_audio_format::B_AUDIO_INT:
		fDescriptionString << "32 Bit (int) ";
		break;

	default:
		fDescriptionString << "??? Bit ";
	}

	if (outputFormat.u.raw_audio.channel_count == 0)
		fDescriptionString << "Mono\n";
	else
		fDescriptionString << "Stereo\n";

	status_t error = mixer->ConnectInput(node, B_MEDIA_RAW_AUDIO);
	if (error != B_OK)
		return error;

	// Find out how to control the volume of the connection
	writelog("Get control for mixer volume\n");
	error = roster->GetParameterWebFor(mixerNode, &fMixerWeb);
	if (error != B_OK) {
		writelog("Couldn't get parameter web for mixer: %s", strerror(error));
	} else {
		int32 destConnectionID = mixer->GetDestConnectionID(0);
		BParameterGroup *group = fMixerWeb->GroupAt(0)->GroupAt(1)->
			GroupAt(destConnectionID - 1);
		if (group) {
			int32 volParamID = (destConnectionID << 16) + 4;
			int32 numParams = group->CountParameters();
			for (int32 param = 0; param < numParams; param++) {
				BParameter *tryParam = group->ParameterAt(param);
				if (tryParam->ID() == volParamID) {
					writelog("Found gain parameter on mixer\n");
					fVolume = dynamic_cast<BContinuousParameter *>(tryParam);
					break;
				}
			}
		}
	}

	writelog("Hooked up mixer ok\n");
	return B_OK;
}

bigtime_t 
MediaNodeController::Position() const
{
	AutoLock<BLooper> lock(const_cast<MediaNodeController *>(this));

	if (CurrentState() != STATE_PLAYING)
		return fCurrentPosition;
	
	bigtime_t playTime = fTimeSource->Now() - fRelativeStart;			
	if (playTime > fOutPoint) 
		playTime = fOutPoint;

	return playTime;
}

void 
MediaNodeController::PausedSetPositionScrubToCommon(bigtime_t pos, bigtime_t now)
{
	ASSERT(IsLocked());
	ASSERT(CurrentState() != STATE_PLAYING || CurrentState() == STATE_SCRUBBING);
	fFileInterface->RollOnce(pos, now, 1);
	fFileInterface->Sync(now + 1);
	fRelativeStart = now - pos;	
	fCurrentPosition = pos;
}


void 
MediaNodeController::_SetPosition(bigtime_t pos, bigtime_t when)
{
	ASSERT(IsLocked());

	if (pos > fOutPoint)
		pos = fOutPoint;
	if (pos < fInPoint)
		pos = fInPoint;

	bigtime_t now = fTimeSource->Now();

	if (CurrentState() == STATE_PLAYING) {
		fFileInterface->Seek(pos, when);
		fRelativeStart = now - pos;	
		writelog("Setting position to %Li.  Relative start is now %Li\n",
			pos, fRelativeStart);
		SetOutPosWakeup();
		return;
	}
	
	PausedSetPositionScrubToCommon(pos, now);
}

void 
MediaNodeController::_SetScrubbing(bool scrub)
{
	// set up/tear down scrub state
	ASSERT(IsLocked());
	
	if (scrub) {
		if (CurrentState() == STATE_PLAYING)
			fFileInterface->Stop(0);
		ScheduleCommand(kScheduledToScrub, system_time());
			// get scheduled right away to start scrubbing
		PushState(CurrentState());
		SetState(STATE_SCRUBBING);
	} else {
		bigtime_t pos = Position();

		if (pos < fInPoint)
			_SetPosition(fInPoint, 0);
		else if (pos > fOutPoint)
			_SetPosition(fOutPoint, 0);

		if (CurrentState() == STATE_PLAYING) 
			// we were playing when we started scrubbing, restart play again
			_Play();
		SetState(PopState());
	}
}

void 
MediaNodeController::_ScrubTo(bigtime_t pos)
{
	ASSERT(IsLocked());

	if (pos == fCurrentPosition)
		// we are done, bail
		return;

	if (pos > fOutPoint) {
		if (pos > fLength)
			pos = fLength;
		_SetOutPoint(pos, false);
	} else if (pos < fInPoint) {
		if (pos < 0)
			pos = 0;
		_SetInPoint(pos, false);
	}

	PausedSetPositionScrubToCommon(pos, fTimeSource->Now());
}

void 
MediaNodeController::ScrubIfNeeded(float scrubTargetPosition)
{
	// drive a synchronous scrub
	if (CurrentState() != STATE_SCRUBBING)
		return;

	bigtime_t nextScrubCheck = system_time() + kScrubCheckPeriod;
	
	// do the scrub (lazily, OK to call without a change)
	_ScrubTo(scrubTargetPosition * Length());
	
	// come back in a bit to see if we need to scrub again
	ScheduleCommand(kScheduledToScrub, nextScrubCheck);
}

void
MediaNodeController::SetOutPosWakeup()
{
	ASSERT(CurrentState() == STATE_PLAYING);

	ScheduleCommand(kScheduledToCheckOutPoint,
		fOutPoint - Position() + system_time());
}

void 
MediaNodeController::SetInPoint(bigtime_t pos, bool tracking)
{
	BMessage message(kSetInPoint);
	message.AddInt64("be:pos", pos);
	message.AddBool("be:tracking", tracking);
	PostMessage(&message);
}

void 
MediaNodeController::SetOutPoint(bigtime_t pos, bool tracking)
{
	BMessage message(kSetOutPoint);
	message.AddInt64("be:pos", pos);
	message.AddBool("be:tracking", tracking);
	PostMessage(&message);
}

void 
MediaNodeController::Play()
{
	PostMessage(kPlay);
}

void 
MediaNodeController::Stop()
{
	PostMessage(kStop);
}

void 
MediaNodeController::Pause()
{
	PostMessage(kPause);
}

void 
MediaNodeController::_SetInPoint(bigtime_t pos, bool tracking)
{
	ASSERT(IsLocked());

	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	if (Position() < pos) {
		if (CurrentState() != STATE_SCRUBBING) {
			_SetPosition(pos, 0);
			if (IsPlaying() && tracking)
				PauseAfterThumbNudge();
		} else {
			fInPoint = pos;
			if (fOutPoint < pos)
				fOutPoint = pos;
			_ScrubTo(pos);
		}
	}

	fInPoint = pos;
	if (fOutPoint < pos)
		fOutPoint = pos;
}

void 
MediaNodeController::_SetOutPoint(bigtime_t pos, bool tracking)
{
	ASSERT(IsLocked());

	writelog("setOutPoint - new %Lx, Position %Lx\n", fOutPoint, Position());

	if (pos <= 0)
		pos = 0;
	else if (pos > fLength)
		pos = fLength;

	if (Position() > pos) {
		if (!CurrentState() == STATE_SCRUBBING) {
			_SetPosition(pos, 0);
			if (IsPlaying() && tracking)
				PauseAfterThumbNudge();
			fOutPoint = pos;
		} else {
			fOutPoint = pos;
			if (fInPoint > pos)
				fInPoint = pos;		
			_ScrubTo(pos);
		}
	} else {
		fOutPoint = pos;	
		if (CurrentState() == STATE_PLAYING)
			SetOutPosWakeup();
	}
	if (fInPoint > pos)
		fInPoint = pos;
}

void 
MediaNodeController::_Stop()
{
	ASSERT(IsLocked());

	if (CurrentState() == STATE_STOPPED) {
		writelog("Attempt to stop node that isn't playing!\n");
		return;
	}

	if (CurrentState() == STATE_PLAYING) {
		fCurrentPosition = Position();
		fFileInterface->Stop();
		ScheduleCommand(kIdle, 0);
	}

	SetState(STATE_STOPPED);
	Rewind();
}

void 
MediaNodeController::_Play()
{
	ASSERT(IsLocked());

	if (CurrentState() == STATE_SCRUBBING) {
		SetState(STATE_PLAYING);
	} else {
		fFileInterface->Preroll();
		bigtime_t startTime = fTimeSource->Now() + 1000;
		fFileInterface->Start(startTime);
		fRelativeStart = startTime - fCurrentPosition;
		SetState(STATE_PLAYING);
		SetOutPosWakeup();
	}
}

void 
MediaNodeController::_Pause()
{
	ASSERT(IsLocked());

	if (CurrentState() == STATE_PAUSED) {
		writelog("Attempt to pause a node that is already paused\n");
		return;
	}

	if (CurrentState() == STATE_PLAYING) {
		fCurrentPosition = Position();
		fFileInterface->Stop();
		ScheduleCommand(kIdle, 0);
	}

	SetState(STATE_PAUSED);
}

void 
MediaNodeController::BumpInPointAndRewind()
{
	PostMessage(kBumpAndRewind);
}

void 
MediaNodeController::BumpOutPointAndGoToEnd()
{
	PostMessage(kBumpAndGoToEnd);
}

void 
MediaNodeController::_BumpInPointAndRewind()
{
	_SetInPoint(0, false);
	_SetPosition(0, 0);
}

void 
MediaNodeController::_BumpOutPointAndGoToEnd()
{
	_SetOutPoint(fLength, false);
	_SetPosition(fLength, 0);
}

bigtime_t kPauseAfterThumbNudge = 700000;

void
MediaNodeController::PauseAfterThumbNudge()
{
	if (CurrentState() == STATE_PLAYING) {
		fCurrentPosition = Position();
		fFileInterface->Stop();
		ScheduleCommand(kScheduledToPlay,
			system_time() + kPauseAfterThumbNudge);
			// start playing again if half a second
	} else if (CurrentState() == STATE_PAUSE_AFTER_END_POINT_NUDGE) {
		ScheduleCommand(kScheduledToPlay,
			system_time() + kPauseAfterThumbNudge);
	}

	SetState(STATE_PAUSE_AFTER_END_POINT_NUDGE);
}

float 
MediaNodeController::RawVolume() const
{
	if (!fVolume)
		return 0;

	float val;
	bigtime_t lastChange;
	size_t valSize = sizeof(val);

	fVolume->GetValue(&val, &valSize, &lastChange);
	return val;
}

void 
MediaNodeController::SetRawVolume(float mixerVolume)
{
	if (fVolume) {
		if (mixerVolume > fVolume->MaxValue())
			mixerVolume = fVolume->MaxValue();
		else if (mixerVolume < fVolume->MinValue())
			mixerVolume = fVolume->MinValue();

		fVolume->SetValue(&mixerVolume, sizeof(mixerVolume), fTimeSource->Now());
	}
}

float 
MediaNodeController::Volume()
{
	if (!fVolume)
		return 0;

	float val;
	if (fMuted || fHalfVolume)
		val = fOriginalVolume;
	else
		val = RawVolume();

	val = (val - fVolume->MinValue()) / (fVolume->MaxValue() - fVolume->MinValue());
	return val;
}

void 
MediaNodeController::SetVolume(float volume)
{
	if (fVolume) {
		float mixerVolume = (volume * (fVolume->MaxValue() - fVolume->MinValue()) +
			fVolume->MinValue());
	
		SetRawVolume(mixerVolume);
	}
}

status_t
MediaNodeController::ReadyToLoop(loop_state* outState)
{
	if (!fLooping) {
		fLooping = true;
		fLocalScrubbing = false;
		fLocalLength = Length();
		fLocalScrubTarget;
	}
	
	while (!ExecuteLoop(outState)) {
		outState->next_loop_time = B_INFINITE_TIMEOUT;
	}
	
	return B_OK;
}

bool MediaNodeController::ExecuteLoop(loop_state* outState)
{
	bigtime_t timeout;
	bigtime_t now = system_time();
	
	if (scheduledCommand == kIdle)
		timeout = B_INFINITE_TIMEOUT;
	else 
		timeout = wakeUpTime - now;

	if (timeout < 0)
		// don't pass negative timeout
		// is this needed?
		timeout = 0;

	outState->next_loop_time = timeout;
	
	float localScrubTargetPosition = 0;
	if (fLocalScrubbing) 
		// we are scrubbing, read the scrub slider value for later
		localScrubTargetPosition = GetScrubTargetPosition(fLocalScrubTarget,
			fLocalLength);
	
	if (scheduledCommand == kQuit) {
		PostMessage(B_QUIT_REQUESTED);
		return true;
	}

	CheckTimeUp(localScrubTargetPosition);
	fLocalScrubbing = fScrubbing;
	fLocalLength = Length();
	
	return true;
}

void
MediaNodeController::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kRescheduleWakeup:
			break;

		case B_REFS_RECEIVED:
			{
				entry_ref ref;
				if (message->FindRef("refs", &ref) != B_OK){
					writelog("RefsReceived - bad params\n");
					break;
				}
				const dormant_node_info *fileNodeInfo;
				ssize_t size;	
				if (message->FindData("medaPlayer:dormantNodeInfo", B_RAW_TYPE,
					(const void **)&fileNodeInfo, &size) != B_OK){
					writelog("RefsReceived - bad params\n");
					break;
				}
				_SetTo(ref, fileNodeInfo);
				break;
			}
			
		case kPlay:
			_Play();
			break;
		
		case kPause:
			_Pause();
			break;
		
		case kStop:
			_Stop();
			break;

		case kBumpAndRewind:
			_BumpInPointAndRewind();
			break;
		
		case kBumpAndGoToEnd:
			_BumpOutPointAndGoToEnd();
			break;

		case kSetInPoint:
			{
				bigtime_t pos;
				if (message->FindInt64("be:pos", (int64 *)&pos) != B_OK) {
					writelog("SetInPoint - bad params\n");
					break;
				}
				_SetInPoint(pos);
				break;
			}
			break;
			
		case kSetOutPoint:
			{
				bigtime_t pos;
				if (message->FindInt64("be:pos", (int64 *)&pos) != B_OK) {
					writelog("SetOutPoint - bad params\n");
					break;
				}
				_SetOutPoint(pos);
				break;
			}
			break;
		
		case kSetPosition:
			{
				bigtime_t pos;
				bigtime_t when;

				message->FindInt64("be:pos", (int64 *)&pos);
				message->FindInt64("be:when", (int64 *)&when);

				_SetPosition(pos, when);
				break;
			}

		case kSetScrubbing:
			{
				bool scrub = false;
				status_t result = message->FindBool("be:scrub", &scrub);
				ASSERT(result == B_OK);
				if (scrub == (CurrentState() == STATE_SCRUBBING))
					break;
				
				if (scrub) {
					// if scrub on, record the messenger of the control driving us
					result = message->FindMessenger("be:scrubDriver", &fLocalScrubTarget);
					ASSERT(result == B_OK);
				}

				fLocalScrubbing = scrub;
				_SetScrubbing(scrub);
				continue;
			}
			break;
}

void
MediaNodeController::CheckTimeUp(float optionalScrubTargetPosition)
{
	if (scheduledCommand == kIdle)
		return;
	
	bigtime_t now = system_time();
	if (wakeUpTime - now <= 10) {
		ScheduledCommand currentCommand = scheduledCommand;
		scheduledCommand = kIdle;
		switch (currentCommand) {
			case kScheduledToPlay:
				PlayAfterThumbNudge();
				break;

			case kScheduledToCheckOutPoint:
				OutPointHit();
				break;

			case kScheduledToScrub:
				ScrubIfNeeded(optionalScrubTargetPosition);
				break;

			default:
				break;
		}
	}
}

void
MediaNodeController::ScheduleCommand(ScheduledCommand what, bigtime_t when)
{
	bool needToReschedule = what != kIdle && when < wakeUpTime;
	
	wakeUpTime = when;
	scheduledCommand = what;
	
	if (needToReschedule) 
		// re-scheduling to an earlier time than we are set to wake up,
		// force thread to wake up and re-schedule itself
		PostMessage(kRescheduleWakeup);
}
