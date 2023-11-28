/*	BMixer.cpp	*/

//comments that start with "dug -" are todo items.

#include "mixer.h"

#include <mixer_i586.h>
#include <assert.h>
#include <resampler_sinc.h>				// need this...
#include <resampler_sinc_lowpass.h>		// and this for AcceptFormat
#include <TimeSource.h>
#include <ParameterWeb.h>
#include <MediaTheme.h>
#include <Buffer.h>
#include <MediaRoster.h>
#include <MediaAddOn.h>

#include <ByteOrder.h>
#include <scheduler.h>
#include <Autolock.h>
#include <OS.h>
#include <image.h>
#include <SupportDefs.h>
#include <Roster.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <Path.h>
#include <FindDirectory.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>



// NDEBUG is defined (or not) in the makefile
#if !NDEBUG
	#define PRINTF printf
	#define FPRINTF fprintf
	#define DEBUG 1
	#define DECL(x) x
#else
// don't printf anything at all
	#define PRINTF(x...)
	#define DECL(x)
#endif

// these are just printfs for debugging different stuff
#define TERROR(string)			PRINTF##string
#define LATE(string)			PRINTF##string
#define TEMP(string)			PRINTF##string
#define FUNCTION(string) 		//PRINTF##string
#define LATENCY(string)			//PRINTF##string
#define MIXRUN(string) 			//PRINTF##string
#define SETUP(string)			//PRINTF##string
#define HANDLE(string)			//PRINTF##string
#define QUEUE(string)			//PRINTF##string
#define DATASTATUS(string)		//PRINTF##string
#define OFFLINE(string)			//PRINTF##string
#define CONVERSION(string)		//PRINTF##string
#define CONNECTION(string)		//PRINTF##string
#define FORMATS(string)			//PRINTF##string
#define SYNCHRO(string)			//PRINTF##string
#define OFFBYONE(string)		//PRINTF##string
#define RESAMPLEGLITCH(string)	//PRINTF##string
#define RESAMPLEGLITCHX(string)	//PRINTF##string
#define MIXA(string)			//PRINTF##string
#define MIXINFO(string)			//PRINTF##string
#define SENDINFO(string)		//PRINTF##string
#define BUFFERS(string)			//PRINTF##string
#define BUFID(string)			//PRINTF##string
#define BUFFERREC(string)		//PRINTF##string
#define MESSAGEREAD(string)		//PRINTF##string
#define RECYCLES(string)		//PRINTF##string
#define PARAMS(string)	 		//PRINTF##string
#define CONSTRUCTOR(string)		//PRINTF##string
#define MEMSETS(string)			//PRINTF##string

// these turn on/off features/debugging thingys
#define RESAMPLE_DEBUG 0
#define RECYCLEINPUT 0 //recycle all buffers in buffer received
#define ASM_OPTIMIZED 1 //enable assembly optimizations

// defaults and other things
#define DEFAULT_TIMEOUT 6000000L
#define DEFAULT_CHUNK_SIZE 2048
#define DEFAULT_SAMPLE_RATE 44100.0

#define MAX_BUFFER_SIZE 16384
#define CACHELINE_FIX 64

#define B_ABSTAIN_YOU_DUMMY 0x60000000	//quit message

// were 200 200 2000
#define MIX_LATENCY_PER_CHANNEL 200
#define MIX_SENDING_LATENCY 200
#define FUDGE_FACTORY_LATENCY 2000

// parameters
#define CHANNEL_MASK 		0xffff0000
#define M_PAN 				1
#define M_MUTE 				2
#define M_GAIN 				3
#define M_NULL 				100
#define M_RESAMPLER_TYPE	200
#define GAIN_BOTTOM 		-60
#define GAIN_TOP 			18

//endians!
#if B_HOST_IS_LENDIAN
#define ENDIAN B_MEDIA_LITTLE_ENDIAN
#else
#define ENDIAN B_MEDIA_BIG_ENDIAN
#endif

// added to adjust for flushing events from a matching connection
BTimedEventQueue::queue_action
_remove_connection_buffers(media_timed_event *event, void *context);

BTimedEventQueue::queue_action
_find_channel_event(media_timed_event *event, void *context);

// Mixer
///////////

BMixer::BMixer(char *name, int32 id, uint32 nBuffers, uint32 nChannels, BMediaAddOn *addon) :
	BMediaNode(name),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	mEventQueue(),
	mBufferQueue(),
	mLock("MixerLock", true),
	mAddon(addon),
	mFlavorID(id),
	mSaveLock("MixerSaveLock")
{
	BPath 	path;

	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Media");
	mkdir(path.Path(), 0775);
	path.Append("MixerSettings");
	
	AddNodeKind(B_SYSTEM_MIXER);
	mMixFormat.type = B_MEDIA_RAW_AUDIO;
	mMixFormat.u.raw_audio.frame_rate = DEFAULT_SAMPLE_RATE;
	mMixFormat.u.raw_audio.channel_count = nChannels;
	mMixFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	mMixFormat.u.raw_audio.byte_order = ENDIAN;
	mMixFormat.u.raw_audio.buffer_size = DEFAULT_CHUNK_SIZE;
	mMixFrameSize = (mMixFormat.u.raw_audio.format & 0xf) * mMixFormat.u.raw_audio.channel_count;
	mMixFrameCount = mMixFormat.u.raw_audio.buffer_size / mMixFrameSize;
	mMixFrameDuration = 1000000.0 / (double)mMixFormat.u.raw_audio.frame_rate;
	mMixBufferDuration = (bigtime_t) (mMixFrameCount * 1000000.0 / mMixFormat.u.raw_audio.frame_rate);

	mOutputFormat.type = B_MEDIA_RAW_AUDIO;
	mOutputFormat.u.raw_audio.frame_rate = DEFAULT_SAMPLE_RATE;
	mOutputFormat.u.raw_audio.channel_count = nChannels;
	mOutputFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	mOutputFormat.u.raw_audio.byte_order = ENDIAN;
	mOutputFormat.u.raw_audio.buffer_size = DEFAULT_CHUNK_SIZE;
	
	mActive = true;
	mRunning = false;
	mOffline = false;
	mHookedUp = false;
	mOutputEnabled = false;
	
	mStartTime = 0;	
	mMixStartTime = 0;
	mNextMixTime = 0;
	mTimeToSend = 0;

	mSchedulingLatency = 1000;					//a nice round number to start with
	mSendLatency = MIX_SENDING_LATENCY;			//a couple hundred micros for good measure
	mMixLatency = MIX_LATENCY_PER_CHANNEL;		//a nice number per channel
	mDownStreamLatency = 0;
	
	mResamplerType = RESAMPLER_LINEAR;

	// Set up save thread
	mSaveMap.clear();
	time(&mSaveTimestamp);
	mSavePortID = create_port(20,"MixerSaveCmd");

	mSettingsLoadedSem=create_sem(0,"MixerSettingsLoadedSem");
	
	mSaveThreadID = spawn_thread(save_thread,"MixerDeferredSave",B_LOW_PRIORITY,this);
	resume_thread(mSaveThreadID);
	
	// Control port
	mControlPort = create_port(20, "BMixer service port");

	// Mix thread
	mMixThread = spawn_thread(BMixer::MixThreadEntry, "BMixer::MixThread", B_REAL_TIME_PRIORITY, this);

	// Connections
	mOutputDestination = media_destination::null;
	mOutputSource.port = mControlPort;
	mOutputSource.id = 0;

	mConnectedCount = 0;
	// Buffer group and MixBuffer
	mOurGroup = new BBufferGroup(MAX_BUFFER_SIZE, nBuffers);
	if (mOurGroup->InitCheck() != B_OK)
	{
		TERROR(("BMixer:: error in output buffer group creation!!!!!\n"));
	}
	mOutputGroup = mOurGroup;
	mOutputBuffer = NULL;
	mMixGroup = NULL;
	mMixBuffer = NULL;
	mMixBufFresh = false;
	mMixFrameTotal = 0;
	
	mChannelDataStatus = 0;
	mDataStatus = B_DATA_NOT_AVAILABLE;
	
	while (acquire_sem(mSettingsLoadedSem)==B_INTERRUPTED)
		;
	
	delete_sem(mSettingsLoadedSem);	// we know that the settings are valid now
	mSettingsLoadedSem=B_ERROR;		// no need for this semaphore to stick around
	
	// Set up master
	mMaster = new mixchannel(0, mControlPort,this);
	
	// Set up channels	
	mChannelArray = (mixchannel **)malloc(MAX_CHANNEL_STRIPS * sizeof(mixchannel*));
	memset((char *)mChannelArray, 0, MAX_CHANNEL_STRIPS * sizeof(mixchannel*));
	MEMSETS("memset constructor\n");
	MakeChannels(&mChannelOne, MIN_CHANNEL_STRIPS);
	CONSTRUCTOR(("BMixer constructor - mChannelOne: 0x%x\n", mChannelOne));
	
	// Lock Memory
	media_realtime_init_image(addon->ImageID(), B_MEDIA_REALTIME_AUDIO);		
	media_realtime_init_thread(mMixThread, 0x5000, B_MEDIA_REALTIME_AUDIO);
}


BMixer::~BMixer()
{
	status_t status;
		
	// Quit the mix thread
	mActive = false;
	write_port(mControlPort, B_ABSTAIN_YOU_DUMMY, NULL, 0);
	wait_for_thread(mMixThread, &status);
	delete_port(mControlPort);
	
	// Quit the save thread
	close_port(mSavePortID);
	wait_for_thread(mSaveThreadID,&status);
	delete_port(mSavePortID);
	
	//clear the queues
	if (mEventQueue.HasEvents())
	{
		mEventQueue.FlushEvents(0, BTimedEventQueue::B_ALWAYS);
	}
	if (mBufferQueue.HasEvents())
	{
		mBufferQueue.FlushEvents(0, BTimedEventQueue::B_ALWAYS);
	}
	//delete buffer groups
	if (mMixBuffer && mMixBuffer != mOutputBuffer)
	{
		mMixBuffer->Recycle();
		mMixBuffer = NULL;
	}
	if (mMixGroup)
	{
		delete mMixGroup;
		mMixGroup = NULL;
	}
	if (mOutputBuffer)
	{
		mOutputBuffer->Recycle();
		mOutputBuffer = NULL;
	}
	if(mOutputGroup != mOurGroup)
	{
		delete mOutputGroup;
		mOutputGroup = NULL;
	}
	delete mOurGroup;
	mOurGroup = NULL;
	
	mixchannel **ch = &mChannelOne;
	while(*ch)
	{
		mixchannel * del = *ch;
		*ch = del->mNext;
		delete(del);
	}
}

int32
BMixer::FindNextEmptyChannel(int32 index)
{
	FUNCTION(("BMixer::FindNextEmptyChannel %d\n"));
	if (index == 0)
	{
		index = 1;
	}
	for(; index < MAX_CHANNEL_STRIPS; index++)
	{
		if (GetChannel(index) == NULL)
		{
			break;
		}
	}
	return index;
}

void
BMixer::MakeChannels(mixchannel **next, int32 count)
{
	FUNCTION(("BMixer::MakeChannels %d\n", count));
	mixchannel **ch = next;
	int32 index = 1;
	//int32 bufcount;
	for(int i=0; i < count; i++)
	{
		index = FindNextEmptyChannel(index);
		*ch = new mixchannel(index, mControlPort,this);
		mChannelArray[index] = *ch;
		ch = &((*ch)->mNext);
	}
}

void
BMixer::WebRun()
{
	char outname[B_MEDIA_NAME_LENGTH];
	BParameter *top = NULL;
	// Parameter Web
	BParameterWeb *web = new BParameterWeb();
	BParameterGroup *main = web->MakeGroup(Name());
	BParameterGroup *outermaster = main->MakeGroup("Master");
	BParameterGroup *master = outermaster->MakeGroup("Master");
	BParameterGroup *channels = main->MakeGroup("Channels");
	//Other selector		
	//BDiscreteParameter * advanced = main->MakeDiscreteParameter(M_ADVANCED, B_MEDIA_RAW_AUDIO, "More Info", B_ENABLE);
	
	BDiscreteParameter *resampler_type = main->MakeDiscreteParameter(M_RESAMPLER_TYPE, B_MEDIA_RAW_AUDIO, "Sample Rate Conversion", 0);
	resampler_type->AddItem(RESAMPLER_ZERO,"Drop-sample");
	resampler_type->AddItem(RESAMPLER_LINEAR,"Linear interpolator");
	resampler_type->AddItem(RESAMPLER_TRILINEAR,"Tri-linear interpolator");
	resampler_type->AddItem(RESAMPLER_CUBIC,"Cubic interpolator");	
	resampler_type->AddItem(RESAMPLER_SINC,"Sinc interpolator");
	resampler_type->AddItem(RESAMPLER_SINC_LOWPASS,"Sinc interpolator with lowpass filter");	
	
	// Master Group
	BNullParameter *in = master->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Master Gain", B_WEB_BUFFER_OUTPUT);
	top = in;
	char bit[8];
	if (mOutputFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT)
	{
		sprintf(bit, "float");
	}
	else
	{
		sprintf(bit, "%ldbit", (mOutputFormat.u.raw_audio.format & 0xf) * 8);
	}
	sprintf(outname, "%.4gkHz %s", mOutputFormat.u.raw_audio.frame_rate/1000, bit);
	BNullParameter *bits = master->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, outname, B_GENERIC);
	bits->AddInput(top);
	top = bits;
	BDiscreteParameter * mute = master->MakeDiscreteParameter(M_MUTE, B_MEDIA_RAW_AUDIO, "Mute", B_MUTE);
	BContinuousParameter * gain = master->MakeContinuousParameter(M_GAIN, B_MEDIA_RAW_AUDIO, "Gain", 
																  B_MASTER_GAIN, "dB", GAIN_BOTTOM, GAIN_TOP, 1.0);
	gain->SetChannelCount(mMixFormat.u.raw_audio.channel_count);
	gain->AddInput(mute);
	if (mMixFormat.u.raw_audio.channel_count <= 1)
	{
		BContinuousParameter * pan = master->MakeContinuousParameter(M_PAN, B_MEDIA_RAW_AUDIO, "Balance",
														  			B_BALANCE, "L R", -1.0, 1.0, 0.1);
		pan->AddInput(top);
		top = pan;
	}
	mute->AddInput(top);
	BNullParameter *out = master->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "To Output", B_WEB_BUFFER_OUTPUT);
	out->AddInput(gain);

	// lock the object for the rest of the function so we can make
	// and set the various things we need to make and set.
	BAutolock lock(&mLock);

	// Channel Group
	if (1)
	{
		mixchannel *ch = mChannelOne;
		while(ch != NULL)
		{
			ch->MakeWebControls(channels, true);
			ch = ch->mNext;
		}
	}

	// Set Web
	SetParameterWeb(web);
}

status_t
BMixer::WebThreadEntry(void * data)
{
	((BMixer *)data)->WebRun();
	return B_OK;
}

void
BMixer::MakeParameterWeb()
{
	FUNCTION(("BMixer::MakeParameterWeb()\n"));
	mWebThread = spawn_thread(BMixer::WebThreadEntry, "BMixer::WebThread", B_DISPLAY_PRIORITY, this);
	resume_thread(mWebThread);
}


uint32 BMixer::ResamplerType()
{
	return mResamplerType;
}

port_id BMixer::SavePortID()
{
	return mSavePortID;
}

time_t BMixer::SaveTimestamp()
{
	return mSaveTimestamp;
}

save_map *BMixer::SaveMap()
{
	return &mSaveMap;
}

BLocker *BMixer::SaveLock()
{
	return &mSaveLock;
}

status_t BMixer::save_thread(void *m)
{
	BMixer 		*mixer = (BMixer *) m;
	status_t 	err;
	int32 		cmd;
	save_info 	info;
	bool 		dirty = false;
	BPath 		path;

	// point to the save file
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Media");
	mkdir(path.Path(), 0775);
	path.Append("MixerVolumes");

	//-------------------------------------------------------------------------------------------------------	
	//	slurp in saved items here
	//-------------------------------------------------------------------------------------------------------	
	
	PARAMS(("fetching settings\n"));
	
	// lock down the save set
	mixer->mSaveLock.Lock();
	
	FILE * f = fopen(path.Path(), "r");
	
	if (f) {
		char line[1024];
		uint32	resampler_type;
		
		// get the version line
		fgets(line, 1024, f);
		
		// only read the settings if the version is OK
		if (!strcmp(line, "# version 2\n")) {
		
			fgets(line, 1024, f);	// skip first explanation line
			
			fgets(line, 1024, f);	// get resampler type line
			sscanf(line,"%ld",&resampler_type);
			mixer->mResamplerType = resampler_type;

			fgets(line, 1024, f);	//	skip second explanation line

			// read settings and add them to the set
			while (5 == fscanf(f, "%ld %f %f %f %64[^\n]", &info.time, &info.gain[0],
					&info.gain[1], &info.pan, info.name)) {
				info.name[63] = 0;
				if (info.time > mixer->mSaveTimestamp-(24*60*60*31)) {	//	one month expiration
					mixer->mSaveMap.insert(info);
		
				}
				else {
					PARAMS(("%ld < %ld, not inserting %s\n", 
						info.time, 
						mixer->mSaveTimestamp-(24*60*60*31), 
						info.name));
				}
			}
		}
		else {
			PARAMS(("MixerVolumes: Bad file version: %s", line));
		}
		fclose(f);
	}
	
	// unlock the save set
	mixer->mSaveLock.Unlock();

	release_sem(mixer->mSettingsLoadedSem);	// loaded settings are now valid
	
	//-------------------------------------------------------------------------------------------------------	
	//	main thread loop to write settings to disk
	//-------------------------------------------------------------------------------------------------------	

	// loop until the port is closed	
	while (1) {
	
		// wait for data to show up in the port; time out after 15 seconds
		err = read_port_etc(mixer->mSavePortID, &cmd, &info, sizeof(info), B_TIMEOUT, 15000000LL);
	
		if (err < B_OK) {
	
			// 15 seconds have gone by; write settings to disk if dirty
			if (dirty) {
			
				mixer->mSaveLock.Lock();
				
				FILE * f = fopen(path.Path(), "w");
				if (f != 0) {
					fprintf(f,"# version 2\n");
					fprintf(f,"# Resampler quality: 0 is worst, %u is best\n",RESAMPLER_MAX);
					fprintf(f,"%lu\n",mixer->mResamplerType);
					fprintf(f, "# timestamp leftgain rightgain pan name\n");
					for (save_map::iterator ptr(mixer->mSaveMap.begin()); ptr != mixer->mSaveMap.end(); ptr++) {
						fprintf(f, "%ld %g %g %g %s\n", (*ptr).time, (*ptr).gain[0], (*ptr).gain[1], (*ptr).pan, (*ptr).name);
					}
					fclose(f);
				}
				else {
					PARAMS(("MixerVolumes: Cannot create: %s\n", path.Path()));
				}
				mixer->mSaveLock.Unlock();
				dirty = false;
			}
	
			// punt if the port has been closed
			if ((err != B_WOULD_BLOCK) && (err != B_TIMED_OUT)) {
				break;
			}
		}
		else 
		{
			// if the command is SAVE_LEVELS, put the levels into the save map
			// if the command is SAVE_RESAMPLER, do nothing; it's already stored in BMixer
			if (SAVE_LEVELS == cmd)
			{
				// add the information to the save set
				save_map::iterator ptr(mixer->mSaveMap.find(info));
				if (ptr == mixer->mSaveMap.end()) {
					mixer->mSaveMap.insert(info);
					ptr = mixer->mSaveMap.find(info);
					assert(ptr != mixer->mSaveMap.end());
				}
				info.time = mixer->mSaveTimestamp;
				const_cast<save_info &>(*ptr) = info;
			}
			dirty = true;
		}
	}
	mixer->mSaveLock.Lock();
	mixer->mSaveMap.clear();
	mixer->mSaveLock.Unlock();
	
	return B_OK;
	
} // save_thread


// MediaNode
/////////////
//#pragma mark ----MediaNode----

BMediaAddOn*
BMixer::AddOn(int32 *internal_id) const
{
	*internal_id = mFlavorID;
	return mAddon;
}


port_id
BMixer::ControlPort() const
{
	return mControlPort;
}


status_t 
BMixer::HandleMessage(int32 code, const void *data, size_t size)
{
	if (BMediaNode::HandleMessage(code, data, size))
		if (BBufferProducer::HandleMessage(code, data, size))
			if (BBufferConsumer::HandleMessage(code, data, size)) {
				BAutolock lock(&mLock);
				if (BControllable::HandleMessage(code, data, size))
					HandleBadMessage(code, data, size);
			}
	return B_OK;
}

void
BMixer::Start(bigtime_t performance_time)
{
	FUNCTION(("BMixer::Start\n"));
	media_timed_event event(performance_time, BTimedEventQueue::B_START);
	mEventQueue.AddEvent(event);
}

void
BMixer::Stop(bigtime_t performance_time, bool immediate)
{	
	FUNCTION(("BMixer::Stop\n"));
	if (immediate)
	{
		performance_time = mEventQueue.FirstEventTime() - 1;
	}
	media_timed_event event(performance_time, BTimedEventQueue::B_STOP);
	mEventQueue.AddEvent(event);
}

void
BMixer::Seek(bigtime_t performance_time, bigtime_t media_time)
{
	FUNCTION(("BMixer::Seek\n"));
	media_timed_event event(performance_time, BTimedEventQueue::B_SEEK);
	event.bigdata = media_time;
	mEventQueue.AddEvent(event);
}

void
BMixer::TimeWarp(bigtime_t at_real_time, bigtime_t /*performance_time*/)
{
	FUNCTION(("BMixer::TimeWarp\n"));
	media_timed_event event(TimeSource()->PerformanceTimeFor(at_real_time),
						BTimedEventQueue::B_WARP);
	mEventQueue.AddEvent(event);	
}

void 
BMixer::NodeRegistered()
{
	FUNCTION(("BMixer::NodeRegistered\n"));
	mMaster->SetName("BeOS Mixer Master");
	MakeParameterWeb();
	resume_thread(mMixThread);
}


// Controllable
///////////////
///////////////
//#pragma mark ----Controllable----

status_t 
BMixer::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	BAutolock lock(&mLock);
	PARAMS(("BMixer::GetParameterValue - id: %x  Size: %d\n", id, *ioSize));
	
	*last_change = mLastParameterChangeTime;
	
	PARAMS(("id & 0xffff0000 hex: %x dec: %d\n", id & 0xffff0000, id & 0xffff0000));
	if ((id & CHANNEL_MASK) != 0)
	{
		//It's a channel
		mixchannel *ch = GetChannel(id >> 16);
		if (ch == NULL)
		{
			PARAMS(("ERROR couldn't find channel for id: %x\n", id));
			return B_ERROR;
		}
		return ch->GetParameterValue(id, last_change, value, ioSize);
	}
	else
	{
		//It's the master
		switch(id)
		{
			case M_PAN:
			{
				*ioSize = sizeof(float);
				*((float *)value) = mMaster->Pan();
				PARAMS(("pan %f\n", *((float *)value)));
				break;
			}
			case M_MUTE:
			{
				bool *aValue = ((bool *)value);
				*ioSize = sizeof(bool);
				*aValue = mMaster->Mute();
				PARAMS(("channel mute %d\n", *aValue));
				return B_OK;
			}
			case M_GAIN:
			{
				float *aValue;
				int32 count = mMixFormat.u.raw_audio.channel_count;
				PARAMS(("parameter channel count: %d\n", count)); 
				*ioSize = sizeof(float) * count;
				for(int i=0; i<count; i++)
				{
					aValue = ((float *)value) + i;
					float x = mMaster->Gain(i);
					if (x > 1.0)
					{
						x = sqrt((x-1)/7*GAIN_TOP*GAIN_TOP);
					}
					else
					{
						x = (GAIN_BOTTOM - 1) * (1 - 2 * x + x * x);
					}
					*aValue = x;
					PARAMS(("gain%d : %f\n", i, *aValue));
				}
				break;
			}
			case M_RESAMPLER_TYPE :
			{
				*((uint32 *) value) = mResamplerType;
				*ioSize = sizeof(uint32);
				break;
			}
			default:
				PARAMS(("ERROR couldn't find paramter for id: %x\n", id));
				return B_ERROR;
			break;
		}
		return B_OK;
	}
}

void 
BMixer::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	BAutolock lock(&mLock);
	PARAMS(("BMixer::SetParameterValue - id: %x  size: %d\n", id, size));

	mLastParameterChangeTime = when;
	PARAMS(("id & 0xffff0000 hex: %x dec: %d\n", id & 0xffff0000, id & 0xffff0000));
	if ((id & CHANNEL_MASK) != 0){
		//It's a channel
		mixchannel *ch = GetChannel(id >> 16);
		if (ch == NULL)
		{
			PARAMS(("ERROR couldn't find channel for id: %x\n", id));
			return;
		}
		ch->SetParameterValue(id, when, value, size);
	}
	else
	{
		//It's the master
		switch(id)
		{
			case M_PAN:
				mMaster->SetPan(*((float *)value));
				PARAMS(("set balance %f\n", mMaster->Pan()));
			break;
			case M_MUTE:
				mMaster->SetMute(*((bool *)value));
				PARAMS(("set mute %d\n", mMaster->Mute()));
			break;
			case M_GAIN:
			{
				int32 count = mMixFormat.u.raw_audio.channel_count;
				float *f;
				if (size != (sizeof(f[0]) * count)){
					PARAMS(("not enough for all the channels\n"));
					PARAMS(("setting them all to the first value\n"));
					PARAMS(("size should be %d\n", sizeof(f[0]) * count));
					f = ((float *)value);
					for(int i=0; i<count; i++)
					{
						//nice curves for the fader above and below 0dB
						if (*f < 0)
						{
							mMaster->SetGain(i, 1.0 - sqrt(-*f)/sqrt((float)1-GAIN_BOTTOM));
						}
						else
						{
							mMaster->SetGain(i, 1.0 + 7 * *f * *f / (GAIN_TOP * GAIN_TOP));
						}
						PARAMS(("set gain[%d] %f\n", i, mMaster->Gain(i)));
					}
				}
				else
				{
					for(int i=0; i<count; i++)
					{
						f = ((float *)value)+ i;
						//nice curves for the fader above and below 0dB
						if (*f < 0)
						{
							mMaster->SetGain(i, 1.0 - sqrt(-*f)/sqrt((float)1-GAIN_BOTTOM));
						}
						else
						{
							mMaster->SetGain(i, 1.0 + 7 * *f * *f / (GAIN_TOP * GAIN_TOP));
						}
						PARAMS(("set gain[%d] %f\n", i, mMaster->Gain(i)));
					}					
				}
				break;
			}
			
			case M_RESAMPLER_TYPE :
			{
				//
				// Store the new value
				//
				mResamplerType = *((uint32 *) value);
				
				//
				// Tell the save thread to do it's stuff
				//
				if (write_port_etc(	mSavePortID, 
									SAVE_RESAMPLER_TYPE, 
									NULL, 
									0, 
									B_TIMEOUT, 
									10000LL)) 
				{
					PARAMS(("BMixer::SetParameterValue failed\n"));
				}

				break;
			}
			
			default:
				PARAMS(("ERROR couldn't find parameter for id: %x\n", id));
			break;
		}
	}
	(void)BroadcastNewParameterValue(when, id, const_cast<void *>(value), size);
}

status_t
BMixer::StartControlPanel(BMessenger * out_messenger)
{
	FUNCTION(("BMixer::StartControlPanel\n"));
	int32 id;
	BMediaAddOn * addon = AddOn(&id);
	if (addon == NULL)
	{
		TERROR(("BMixer::StartControlPanel - can't find addon id?!\n"));
		return B_ERROR;
	}
	image_id im = addon->ImageID();
	if (im < 0)
	{
		TERROR(("BMixer::StartControlPanel - can't find image id?!\n"));
		return im;
	}
	image_info imin;
	status_t err = get_image_info(im, &imin);
	if (err < B_OK)
	{
		TERROR(("BMixer::StartControlPanel - can't get image info?!\n"));
		return err;
	}
	entry_ref ref;
	err = get_ref_for_path(imin.name, &ref);
	if (err < B_OK)
	{
		TERROR(("BMixer::StartControlPanel - can't get_ref_for_path?!\n"));
		return err;
	}
	team_id team = -1;
	BMessage msg(B_LAUNCH_MIXER_CONTROL);
	msg.AddInt32("id", ID());
	err = be_roster->Launch(&ref, &msg, &team);
	if ((err >= B_OK) || (err == B_ALREADY_RUNNING))
	{
		FUNCTION(("BMixer::StartControlPanel - success\n"));
		*out_messenger = BMessenger(NULL, team);
		err = B_OK;
	}
	return err;
}


// BufferConsumer
/////////////////
/////////////////
//#pragma mark ----BufferConsumer----

status_t
BMixer::AcceptFormat(const media_destination & /*dest*/, media_format * format)
{
	FUNCTION(("BMixer::AcceptFormat\n"));

#if DEBUG
	char str[256];
	string_for_format(*format, str, 256);
	FORMATS(("BMixer: accept format: %s  format code is 0x%lx\n", str,format->u.raw_audio.format));
#endif

	FORMATS(("format->u.raw_audio.buffer_size %ld\n",format->u.raw_audio.buffer_size));
	
	//----------------------------------------------------------------------------------------------
	// 
	// Validate the proposed format & fill out any wildcards
	//
	//----------------------------------------------------------------------------------------------

	if (format->type != B_MEDIA_NO_TYPE && format->type != B_MEDIA_UNKNOWN_TYPE)
	{
		if (format->type != B_MEDIA_RAW_AUDIO)
			return B_MEDIA_BAD_FORMAT;

		format->type = B_MEDIA_RAW_AUDIO;

		if (format->u.raw_audio.channel_count > mMixFormat.u.raw_audio.channel_count)
		{
			return B_MEDIA_BAD_FORMAT;
		}
		else if (format->u.raw_audio.channel_count == media_raw_audio_format::wildcard.channel_count)
		{
			format->u.raw_audio.channel_count = mMixFormat.u.raw_audio.channel_count;
		}

		if (format->u.raw_audio.format == media_raw_audio_format::wildcard.format)
		{
			format->u.raw_audio.format = mMixFormat.u.raw_audio.format;
		}

		if (format->u.raw_audio.frame_rate == media_raw_audio_format::wildcard.frame_rate)
		{
			format->u.raw_audio.frame_rate = mMixFormat.u.raw_audio.frame_rate;
		}

		if (format->u.raw_audio.byte_order == media_raw_audio_format::wildcard.byte_order)
		{
			format->u.raw_audio.byte_order = mMixFormat.u.raw_audio.byte_order;
		}

		if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
		{
			FORMATS(("\tUsing wildcard buffer size: "));
		
			uint32 size = 64;
			uint32 min_size = (uint32)((format->u.raw_audio.frame_rate * format->u.raw_audio.channel_count *
							(format->u.raw_audio.format & 0xf) * mMixFormat.u.raw_audio.buffer_size) /
							(mMixFormat.u.raw_audio.frame_rate * mMixFormat.u.raw_audio.channel_count *
							(mMixFormat.u.raw_audio.format & 0xf)));
			//choose a size that is a power of two in the case of unevenly divisible frame_rates
			while(size < min_size)
			{
				size <<= 1;
			}
			format->u.raw_audio.buffer_size = size;		
			
			FORMATS(("%ld\n",size));
		}
	}
	else
	{
		*format = mMixFormat;
	}
	
	//----------------------------------------------------------------------------------------------
	// 
	// Since the lookup table for the sinc resampler can become quite large, the sinc resampler
	// may round the input sample rate up or down to limit the table size.  Find out if the
	// sinc resampler will do this and adjust the format as necessary.
	//
	// The routine to determine the table size also does the rounding, so call it and throw away
	// the result.
	//
	//----------------------------------------------------------------------------------------------

	switch (mResamplerType)
	{
		uint32 freq_in,freq_out;
		
		case RESAMPLER_SINC :

			freq_in = (uint32) format->u.raw_audio.frame_rate;
			freq_out = (uint32) mMixFormat.u.raw_audio.frame_rate;
			
			get_sinc_table_size(
				&freq_in, 
				&freq_out, 
				resampler_sinc_const::SINC_TAPS, 
				resampler_sinc_const::MAX_SINC_TABLE_SIZE);
			
			format->u.raw_audio.frame_rate = (float) freq_in;
		
			break;
			
		case RESAMPLER_SINC_LOWPASS :
		
			freq_in = (uint32) format->u.raw_audio.frame_rate;
			freq_out = (uint32) mMixFormat.u.raw_audio.frame_rate;
			
			// sinc+lowpass oversamples, so compute the table size
			// based on the oversampled rate
			freq_out <<= sinc_lowpass_const::OVERSAMPLE_SHIFT_FACTOR;
			
			get_sinc_table_size(
				&freq_in, 
				&freq_out, 
				resampler_sinc_const::SINC_TAPS, 
				resampler_sinc_const::MAX_SINC_TABLE_SIZE);
			
			format->u.raw_audio.frame_rate = (float) freq_in;
		
			break;
	}
	
	return B_OK;
}
 
status_t
BMixer::FormatChanged(const media_source & /*producer*/, const media_destination & consumer, 
						int32 /*from_change_tag*/, const media_format & format)
{

#if DEBUG
	char str[256];
	string_for_format(format, str, 256);
	FORMATS(("BMixer::FormatChanged %s\n", str));
#endif
	mixchannel *ch = GetChannel(consumer.id);
	if (ch == NULL)
	{
		return B_ERROR;
	}
	ch->SubmitChange(format, mMixFormat);
	return B_OK;
}


status_t
BMixer::GetNextInput(int32 * cookie, media_input * out_input)
{
	FUNCTION(("BMixer::GetNextInput 0x%x\n", *cookie));
	
	if (*cookie == 0)
	{
		out_input->node = Node();
		out_input->destination.id = 0;
		out_input->destination.port = mControlPort;
		out_input->source = media_source::null;
		out_input->format = mMixFormat;
		strcpy(out_input->name, "Free Input");
		
		*cookie = (int32)mChannelOne;
		return B_OK;
	}
	else if (*cookie == -1)
	{
		return B_ERROR;
	}
	else
	{
		mixchannel *ch = (mixchannel *)(*cookie);
		media_format format;
		out_input->node = Node();
		out_input->destination = ch->Destination();
		out_input->source = ch->Source();
		out_input->format = ch->Format();
		
		strcpy(out_input->name, ch->Name());
		
		*cookie = (int32)ch->mNext;
		if (*cookie == 0)
		{
			*cookie = -1;
		}
	}
	return B_OK;
}


void
BMixer::DisposeInputCookie(int32 /*cookie*/)
{
	// Don't do anything special since we didn't allocate
	// anything on the cookie's behalf.
}


status_t
BMixer::GetLatencyFor(const media_destination &for_whom,
	bigtime_t * out_latency, media_node_id * out_timesource)
{
	FUNCTION(("BMixer::GetLatencyFor\n"));
	media_node_id downstream_tsid = 0;
	bigtime_t downstream_latency = 0;
	bigtime_t latency = 0;
	if (mOutputDestination != media_destination::null){
		status_t error = FindLatencyFor(mOutputDestination, &downstream_latency, &downstream_tsid);
		if (error < B_OK)
		{
			TERROR(("BMixer::GetLatencyFor - something's wrong 0x%x\n", error));
		}
		else if (downstream_tsid == TimeSource()->ID())
		{
			mDownStreamLatency = downstream_latency;
			latency = mDownStreamLatency;
			LATENCY(("BMixer::GetLatencyFor - downstream latency: %Ld\n", latency));
		}
	}
	mixchannel *ch = GetChannel(for_whom.id);
	if (ch == NULL)
	{
		return B_BAD_VALUE;
	}
	if (ch->IsSynchronous())
	{
		latency += (mSendLatency + mMixLatency + FUDGE_FACTORY_LATENCY);
	}
	else
	{
		latency += (mSendLatency + mMixLatency + mMixBufferDuration + FUDGE_FACTORY_LATENCY);
	}
	*out_latency = latency;

	*out_timesource = TimeSource()->ID();
	
	LATENCY(("BMixer::GetLatencyFor - total latency: %Ld\n", *out_latency));
	return B_OK;
}


void
BMixer::BufferReceived(BBuffer * buffer)
{
	BUFFERREC(("buf: %Ld\n",  buffer->Header()->start_time));
	
//	BUFFERREC(("buf dest: %d\n", buffer->Header()->destination));
	if (!buffer)
	{
		TERROR(("BMixer::BufferReceived - what buffer?\n"));
		return;
	}
#if RECYCLEINPUT
	TERROR(("Mixer: RECYCLEINPUT on\n"));
	buffer->Recycle();
	return;
#endif

	if (mRunning && buffer->Header()->start_time < mNextMixTime)
	{
		MixBuffer(buffer);
	}
	else if (mRunning)
	{
		media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
			buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
		event.bigdata = buffer->Header()->start_time;
		status_t err = mBufferQueue.AddEvent(event);
		if (err < B_OK)
		{
			TERROR(("Mixer - error pushing buffer into queue!!!!!!!!!!\n"));
			buffer->Recycle();
		}
	}
	else
	{
		buffer->Recycle();
	}
}

void
BMixer::ProducerDataStatus(const media_destination & for_whom, int32 status, bigtime_t at_performance_time)
{
	DATASTATUS(("BMixer::ProducerDataStatus channel: %d status: %x total: %d\n", for_whom.id, status, mChannelDataStatus));
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS);
	event.pointer = (void *)for_whom.id;
	event.bigdata = status;
	mEventQueue.AddEvent(event);
}


status_t 
BMixer::Connected(		
	const media_source & producer,
	const media_destination & where,
	const media_format & with_format,
	media_input * out_input)
{	
	BAutolock lock(&mLock);
	CONNECTION(("BMixer::Connected\n"));
	
#if DEBUG
	char fmt[256];
	string_for_format(with_format, fmt, 256);
	FORMATS(("Mixer: suggested format %s\n", fmt));
#endif

	mixchannel *ch = mChannelOne;
	while(ch)
	{
		CONNECTION(("BMixer::Connected - next channel\n"));
		if (ch->Source() != media_source::null)
		{
			if (ch->mNext == NULL)
			{
				MakeChannels(&(ch->mNext), 1);
			}
			ch = ch->mNext;
		}
		else
		{
			CONNECTION(("source id:%d\n", producer.id));
			CONNECTION(("destination id: %d\n", out_input->destination.id));
			//do channel stuff
			ch->Connected(producer, where, with_format, out_input, mMixFormat);
			mConnectedCount++;
			mMixLatency = MIX_LATENCY_PER_CHANNEL * mConnectedCount;
			out_input->node = Node();
			out_input->format = with_format;
			//make a new parameter web so channels get updated
			MakeParameterWeb();
			return B_OK;
		}
	}
	TERROR(("BMixer::Connected - something went wrong\n"));
	return B_ERROR;
}


void
BMixer::Disconnected(const media_source & /*producer*/, const media_destination & where)
{
	BAutolock lock(&mLock);
	CONNECTION(("BMixer::Disconnected\n"));
	mixchannel **ch = &mChannelOne;
	while(*ch)
	{
		if ((*ch)->ID() == (uint32) where.id)
		{
			(*ch)->Disconnected();
			mConnectedCount--;
			mMixLatency = MIX_LATENCY_PER_CHANNEL * mConnectedCount;
			if (where.id > MIN_CHANNEL_STRIPS)
			{
				mixchannel *del = *ch;
				*ch = del->mNext;
				mChannelArray[where.id] = NULL;
				if (del->GetDataStatus() == B_DATA_AVAILABLE)
				{
					atomic_add(&mChannelDataStatus, -1);
				}
				delete(del);
			}
			MakeParameterWeb();
			break;
		}
		else
		{
			ch = &(*ch)->mNext;
		}
	}
}


// BufferProducer
/////////////////
/////////////////
//#pragma mark ----BufferProducer----

//dug - figure out...
status_t
BMixer::FormatChangeRequested(const media_source & /*source*/, const media_destination & /*destination*/,
							media_format * DECL(io_format), int32 * /*out_change_count*/)
{
	FUNCTION(("BMixer::FormatChangeRequested\n"));
#if DEBUG
	char str[256];
	string_for_format(*io_format, str, 256);
	FORMATS(("BMixer::FormatChangeRequested %s\n", str));
#endif

	return B_ERROR;
}


status_t 
BMixer::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format *format)
{
	FUNCTION(("BMixer::FormatSuggestionRequested\n"));
#if DEBUG
	char str[256];
	string_for_format(*format, str, 256);
	FORMATS(("BMixer::FormatSuggestionRequested %s\n", str));
#endif

	if (type == B_MEDIA_RAW_AUDIO || type == B_MEDIA_NO_TYPE || type == B_MEDIA_UNKNOWN_TYPE)
	{
		*format = mMixFormat;
		format->u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;
		format->u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
		format->u.raw_audio.format = media_raw_audio_format::wildcard.format;
		
		return B_OK;
	}
	else
	{
		return B_ERROR;
	}
}


status_t 
BMixer::FormatProposal(const media_source & output, media_format *format)
{
	FUNCTION(("BMixer::FormatProposal\n"));
	
	/* check the output */
	if (output != mOutputSource)
	{
		FORMATS(("BMixer::FormatProposal wrong output\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	
#if DEBUG
	char str[256];
	string_for_format(*format, str, 256);
	FORMATS(("BMixer::FormatProposal proposed: %s\n", str));
#endif
	
	status_t err = B_OK;
	media_format f2(mMixFormat);
	f2.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;
	f2.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
	f2.u.raw_audio.format = media_raw_audio_format::wildcard.format;

	if (!format_is_compatible(*format, f2)) {
		FORMATS(("BMixer::FormatProposal format is not compatible\n"));
		*format = f2;
		
		return B_MEDIA_BAD_FORMAT;
	}

	/* specialize the format if it's a wildcard, fail if it's not short or float */
	if (format->u.raw_audio.format == media_raw_audio_format::wildcard.format) {
		format->u.raw_audio.format = mMixFormat.u.raw_audio.format;
	}else if (format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT &&
			format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT) {
		FORMATS(("BMixer::FormatProposal bad format\n"));
		format->u.raw_audio.format = f2.u.raw_audio.format;
		err = B_MEDIA_BAD_FORMAT;
	}

#if DEBUG
	string_for_format(*format, str, 256);
	FORMATS(("BMixer::FormatProposal prespecialized: %s\n", str));
#endif
	/* specialize any leftover wildcards to the mix format */
	format->SpecializeTo(&f2);
	
#if DEBUG
	string_for_format(*format, str, 256);
	FORMATS(("BMixer::FormatProposal returning: %s\n", str));
#endif

	return B_OK;
}


status_t 
BMixer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	FUNCTION(("BMixer::GetNextOutput - %d\n", *cookie));

	if (*cookie != 0)
	{
		*cookie = -1;
		return B_ERROR;
	}
	media_format f2(mOutputFormat);
	
	if (mOutputDestination == media_destination::null)
	{
		f2 = mMixFormat;
		f2.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;
		f2.u.raw_audio.frame_rate = media_raw_audio_format::wildcard.frame_rate;
		f2.u.raw_audio.format = media_raw_audio_format::wildcard.format;
		f2.u.raw_audio.format = media_raw_audio_format::wildcard.channel_count;
	}
	out_output->node = Node();
	out_output->source = mOutputSource;
	out_output->destination = mOutputDestination;
	out_output->format = f2;
	char name[] = "Audio Mixer";
	strcpy(out_output->name, name);
	
	*cookie = 1;
	
	return B_OK;
}


status_t 
BMixer::DisposeOutputCookie(int32 /*cookie*/)
{
	// Don't do anything because we didn't allocate
	// anything on behalf of the cookie.
	return B_OK;
}


status_t 
BMixer::SetBufferGroup(const media_source & for_source, BBufferGroup *group)
{	
	FUNCTION(("BMixer::SetBufferGroup - %d\n"));
	if (for_source != mOutputSource || group == NULL)
	{
		return B_MEDIA_BAD_SOURCE;
	}
	BBufferGroup *g = mOutputGroup;
	mOutputGroup = group;
	if (g != mOurGroup)
	{
		delete g;
	}
	return B_OK;
}


void
BMixer::LateNoticeReceived(const media_source & /*what*/,
						bigtime_t /*how_much*/,
						bigtime_t /*performance_time*/)
{
	FUNCTION(("BMixer::LateNoticeReceived - doh!\n"));
	
}


status_t
BMixer::PrepareToConnect(const media_source & /*what*/,
						const media_destination & /*where*/,
						media_format * format,
						media_source * out_source,
						char * out_name)
{
	
	FUNCTION(("BMixer::PrepareToConnect\n"));

#if DEBUG
	char str[256];
	string_for_format(*format, str, 256);
	FORMATS(("BMixer PrepareToConnect: incoming: %s\n", str));
	string_for_format(mMixFormat, str, 256);
	FORMATS(("BMixer PrepareToConnect: mMixerFormat: %s\n", str));
#endif

	status_t err = B_OK;
	
	/* if we're already connected fail */
	if (mHookedUp)
		return B_ERROR;
	
	if (format->type != B_MEDIA_RAW_AUDIO)
	{
		FORMATS(("bad format\n"));
		*format = mMixFormat;
		err = B_MEDIA_BAD_FORMAT;
	}	

	// frame rate can be anything
	if (format->u.raw_audio.frame_rate == media_raw_audio_format::wildcard.frame_rate)
		format->u.raw_audio.frame_rate = mMixFormat.u.raw_audio.frame_rate;
	
	// channel count can't be greater than mMixFormat
	if (format->u.raw_audio.channel_count == media_raw_audio_format::wildcard.channel_count)
		format->u.raw_audio.channel_count = mMixFormat.u.raw_audio.channel_count;
	
	if (format->u.raw_audio.channel_count > media_raw_audio_format::wildcard.channel_count && 
		format->u.raw_audio.channel_count > mMixFormat.u.raw_audio.channel_count)
	{
		FORMATS(("BMixer: bad channel count\n"));
		format->u.raw_audio.channel_count = mMixFormat.u.raw_audio.channel_count;
		err = B_MEDIA_BAD_FORMAT;
	}
	
	// format can either be SHORT or FLOAT
	if (format->u.raw_audio.format == media_raw_audio_format::wildcard.format)
		format->u.raw_audio.format = mMixFormat.u.raw_audio.format;
	
	if (format->u.raw_audio.format > media_raw_audio_format::wildcard.format && 
		format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT &&
		format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT)
	{
		FORMATS(("BMixer: bad format\n"));
		format->u.raw_audio.format = mMixFormat.u.raw_audio.format;
		err = B_MEDIA_BAD_FORMAT;
	}
	
	// byte order must be host endian
	if (format->u.raw_audio.byte_order == media_raw_audio_format::wildcard.byte_order)
		format->u.raw_audio.byte_order = mMixFormat.u.raw_audio.byte_order;
	
	if (format->u.raw_audio.byte_order > media_raw_audio_format::wildcard.byte_order && 
		format->u.raw_audio.byte_order != mMixFormat.u.raw_audio.byte_order)
	{
		FORMATS(("BMixer: bad byte order\n"));
		format->u.raw_audio.byte_order = mMixFormat.u.raw_audio.byte_order;
		err = B_MEDIA_BAD_FORMAT;
	}
	
	// buffer size can be anything
	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
		format->u.raw_audio.buffer_size = mMixFormat.u.raw_audio.buffer_size;
	
#if DEBUG
	string_for_format(*format, str, 256);
	FORMATS(("PrepareToConnect: returning: %s\n", str));
#endif

	/* reserve the output */
	mHookedUp = true;
	sprintf(out_name, "Main Mix");
	*out_source = mOutputSource;

	return err;

}


void
BMixer::Connect(
		status_t err,
		const media_source & /*what*/,
		const media_destination & where,
		const media_format & format,
		char * /*io_name*/)
{
	FUNCTION(("BMixer::Connect\n"));
	
	if (err == B_OK){
		mOutputDestination = where;
	
		media_node_id  timesource;
		bigtime_t lat;

		FindLatencyFor(where, &lat, &timesource);

		if (timesource == TimeSource()->ID())
		{
			mDownStreamLatency = lat;
		}
		else
		{
			mDownStreamLatency = 0;
		}
		LATENCY(("BMixer::Connect - DownStreamLatency: %Ld\n", mDownStreamLatency));
		mOutputFormat.u.raw_audio.buffer_size = format.u.raw_audio.buffer_size;
		mOutputFormat.u.raw_audio.format = format.u.raw_audio.format;
		mOutputFormat.u.raw_audio.channel_count = format.u.raw_audio.channel_count;
		mOutputFormat.u.raw_audio.frame_rate = format.u.raw_audio.frame_rate;

		mMixFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
		mMixFormat.u.raw_audio.channel_count = format.u.raw_audio.channel_count;
		mMixFormat.u.raw_audio.frame_rate = format.u.raw_audio.frame_rate;
		mMixFormat.u.raw_audio.buffer_size = (size_t)(format.u.raw_audio.buffer_size * ((float)(mMixFormat.u.raw_audio.format & 0xf)/
																						(mOutputFormat.u.raw_audio.format & 0xf)));
		mMixFrameSize = (mMixFormat.u.raw_audio.format & 0xf) * mMixFormat.u.raw_audio.channel_count;
		mMixFrameCount = mMixFormat.u.raw_audio.buffer_size / mMixFrameSize;
		mMixFrameDuration = 1000000.0 / (double)mMixFormat.u.raw_audio.frame_rate;
		mMixBufferDuration = (bigtime_t) (mMixFrameCount * 1000000.0 / mMixFormat.u.raw_audio.frame_rate);

		LATENCY(("BMixer::Connect - mMixBufferDuration: %Ld\n", mMixBufferDuration));
		
		if(mMixBuffer && mMixBuffer != mOutputBuffer)
		{
			mMixBuffer->Recycle();
			mMixBuffer = NULL;
		}	
		delete(mMixGroup);
		mMixGroup = NULL;
		if (mMixFormat.u.raw_audio.format != mOutputFormat.u.raw_audio.format)
		{
			CONNECTION(("BMixer::Connect - mix and output formats do not match\n"));
			CONNECTION(("BMixer::Connect - creating mMixGroup\n"));
			mMixGroup = new BBufferGroup(mMixFormat.u.raw_audio.buffer_size, 1);
			if (mMixGroup->InitCheck() != B_OK)
			{
				TERROR(("BMixer:: error in mix buffer group creation!!!!!\n"));
			}
		}

		if(mOutputBuffer)
		{
			mOutputBuffer->Recycle();
			mOutputBuffer = NULL;
		}
		delete(mOurGroup);
		mOurGroup = NULL;
		//calculate minimum buffers for downstream latency
		int32 numbufs = MAX(3, (int32)ceil((float)mDownStreamLatency / mMixBufferDuration));
		CONNECTION(("BMixer::Connect - creating mOurGroup with %d buffers\n", numbufs));
		mOurGroup = new BBufferGroup(mOutputFormat.u.raw_audio.buffer_size, numbufs);
		if (mOurGroup->InitCheck() != B_OK)
		{
			TERROR(("BMixer:: error in output buffer group creation!!!!!\n"));
		}
		mOutputGroup = mOurGroup;
		mOutputEnabled = true;
	}
	else
	{
		mHookedUp = false;
		mOutputEnabled = false;
	}
}


void
BMixer::Disconnect(const media_source & what, const media_destination & /*where*/)
{	
	FUNCTION(("BMixer::Disconnect\n"));
	if (what.port != mControlPort || what.id != 0)
	{
		return;
	}

	mHookedUp = false;
	mOutputEnabled = false;
	mOutputDestination = media_destination::null;
	
	if (mOutputGroup != mOurGroup)
	{
		BBufferGroup *g = mOutputGroup;
		mOutputGroup = mOurGroup;
		delete(g);
	}
	return;
}

void
BMixer::EnableOutput(const media_source & what, bool enabled, int32 * /*deprecated*/)
{
	FUNCTION(("BMixer::EnableOutput\n"));
	//dont send buffers from output
	if (mOutputSource == what)
	{
		mOutputEnabled = enabled;
	}
}


// Assembly Mix
///////////////
///////////////
//#pragma mark ----Assembly----

static inline void
_mix_uchar_to_float(bool mix,
						uchar *in,
						float *out,
						float *gain,
						uint32 in_channel_count,
						uint32 out_channel_count,
						uint32 frame_count)
{
#if ASM_OPTIMIZED
	//assemby routines require multiples of 4
	uint32 f = frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4){
		if (mix){
			if (in_channel_count == 2 && out_channel_count == 2){
				unsignedByteToFloatAccum2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				unsignedByteToFloatAccum1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				unsignedByteToFloatAccum1to2(out, in, gain, f);
			}else{
				unsignedByteToFloatAccumN(out, in, gain, out_channel_count, f);
			}
		}else{
			if (in_channel_count == 2 && out_channel_count == 2){
				unsignedByteToFloat2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				unsignedByteToFloat1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				unsignedByteToFloat1to2(out, in, gain, f);
			}else{
				unsignedByteToFloatN(out, in, gain, out_channel_count, f);
			}
		}
		frame_count -= f;
		in += f * in_channel_count;
		out += f * out_channel_count;
	}
#endif
	int x = (in_channel_count > 1 && in_channel_count == out_channel_count) ? 1 : 0;
	double max = 1.0/127.0;
	if (mix){	
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * (float)(((double)(*in)-128) * max);
				*(out+1) += gain[1] * (float)(((double)(*in+x)-128) * max);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * (float)(((double)(*in)-128) * max);
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out += gain[x*y] * (float)(((double)(*(in+(x*y)))-128) * max);
					++out;
				}
				in += in_channel_count;
			}									
		}
	}else{
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * (float)(((double)(*in+x)-128) * max);
				*(out+1) = gain[1] * (float)((double)(*(in+x)-128) * max);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * (float)(((double)(*in)-128) * max);
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out = gain[x*y] * (float)(((double)(*(in+(x*y)))-128) * max);
					++out;
				}
				in += in_channel_count;
			}									
		}
	}
}


static inline void
_mix_short_to_float(bool mix,
					short *in,
					float *out,
					float *gain,
					uint32 in_channel_count,
					uint32 out_channel_count,
					uint32 frame_count)
{
#if ASM_OPTIMIZED
	//assemby routines require multiples of 4
	uint32 f = frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4){
		if (mix){
			if (in_channel_count == 2 && out_channel_count == 2){
				wordToFloatAccum2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				wordToFloatAccum1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				wordToFloatAccum1to2(out, in, gain, f);
			}else{
				wordToFloatAccumN(out, in, gain, out_channel_count, f);
			}
		}else{
			if (in_channel_count == 2 && out_channel_count == 2){
				wordToFloat2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				wordToFloat1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				wordToFloat1to2(out, in, gain, f);
			}else{
				wordToFloatN(out, in, gain, out_channel_count, f);
			}
		}
		frame_count -= f;
		in += f * in_channel_count;
		out += f * out_channel_count;
	}
#endif
	int x = (in_channel_count > 1 && in_channel_count == out_channel_count) ? 1 : 0;
	double max = GAIN_SCALE_INT16_TO_FLOAT;
	if (mix){	
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * (float)((double)(*in) * max);
				*(out+1) += gain[1] * (float)((double)(*(in+x)) * max);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * (float)((double)(*in) * max);
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out += gain[x*y] * (float)((double)(*(in+(x*y))) * max);
					++out;
				}
				in += in_channel_count;
			}									
		}
	}else{
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * (float)((double)(*in) * max);
				*(out+1) = gain[1] * (float)((double)(*(in+x)) * max);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * (float)((double)(*in) * max);
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out = gain[x*y] * (float)((double)(*(in+(x*y))) * max);
					++out;
				}
				in += in_channel_count;
			}									
		}
	}
}


static inline void
_mix_int32_to_float(bool mix,
					int32 *in,
					float *out,
					float *gain,
					uint32 in_channel_count,
					uint32 out_channel_count,
					uint32 frame_count)
{

#if ASM_OPTIMIZED
	//assemby routines require multiples of 4
	uint32 f = frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4){
		if (mix){
			if (in_channel_count == 2 && out_channel_count == 2){
				intToFloatAccum2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				intToFloatAccum1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				intToFloatAccum1to2(out, in, gain, f);
			}else{
				intToFloatAccumN(out, in, gain, out_channel_count, f);
			}
		}else{
			if (in_channel_count == 2 && out_channel_count == 2){
				intToFloat2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				intToFloat1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				intToFloat1to2(out, in, gain, f);
			}else{
				intToFloatN(out, in, gain, out_channel_count, f);
			}
		}
		frame_count -= f;
		in += f * in_channel_count;
		out += f * out_channel_count;
	}
#endif
	int x = (in_channel_count > 1 && in_channel_count == out_channel_count) ? 1 : 0;
	double max = GAIN_SCALE_INT32_TO_FLOAT;	// mattg
	if (mix){	
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * (float)((double)(*in) * max);
				*(out+1) += gain[1] * (float)((double)(*(in+x)) * max);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * (float)((double)(*in) * max);
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out += gain[x*y] * (float)((double)(*(in+(x*y))) * max);
					++out;
				}
				in += in_channel_count;
			}									
		}
	}else{
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * (float)((double)(*in) * max);
				*(out+1) = gain[1] * (float)((double)(*(in+x)) * max);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * (float)((double)(*in) * max);
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out = gain[x*y] * (float)((double)(*(in+(x*y))) * max);
					++out;
				}
				in += in_channel_count;
			}									
		}
	}
}


static inline void
_mix_float_to_float(bool mix,
					float *in,
					float *out,
					float *gain,
					uint32 in_channel_count,
					uint32 out_channel_count,
					uint32 frame_count)
{
#if ASM_OPTIMIZED
	//assemby routines require multiples of 4
	uint32 f = frame_count & 0xfffffffc;	// round down to nearest multiple of 4
	if (f >= 4){
		if (mix){
			if (in_channel_count == 2 && out_channel_count == 2){
				floatToFloatAccum2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				floatToFloatAccum1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				floatToFloatAccum1to2(out, in, gain, f);
			}else{
				floatToFloatAccumN(out, in, gain, out_channel_count, f);
			}
		}else{
			if (in_channel_count == 2 && out_channel_count == 2){
				floatToFloat2(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 1){
				floatToFloat1(out, in, gain, f);
			}else if (in_channel_count == 1 && out_channel_count == 2){
				floatToFloat1to2(out, in, gain, f);
			}else{
				floatToFloatN(out, in, gain, out_channel_count, f);
			}
		}
		frame_count -= f;
		in += f * in_channel_count;
		out += f * out_channel_count;
	}
#endif
	int x = (in_channel_count > 1 && in_channel_count == out_channel_count) ? 1 : 0;

	if (mix){	
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * *in;
				*(out+1) += gain[1] * *(in+x);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out += gain[0] * *in;
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out += gain[x*y] * (*(in+(x*y)));
					++out;
				}
				in += in_channel_count;
			}									
		}
	}else{
		if (out_channel_count == 2){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * *in;
				*(out+1) = gain[1] * *(in+x);
				out += out_channel_count;
				in += in_channel_count;
			}	
		}else if (out_channel_count == 1){
			for(uint32 z=0; z < frame_count; z++){
				*out = gain[0] * *in;
				out += out_channel_count;
				in += in_channel_count;
			}										
		}else{
			int y;
			for(uint32 z=0; z < frame_count; z++){
				for(y=0; y < (int) out_channel_count; y++){
					*out = gain[x*y] * (*(in+(x*y)));
					++out;
				}
				in += in_channel_count;
			}									
		}
	}
}

// Mixer thread
///////////////
///////////////
//#pragma mark ----Engine----

#define TIME_TO_MIX 0x7171
#define TIME_TO_SEND 0x7172

#if RESAMPLE_DEBUG
	static uint32 antiglitch;
#endif


void
BMixer::MixBuffer(BBuffer *buffer)
{
	if (buffer == NULL)
	{
		TERROR(("Mixer ERROR missing popped buffer!!!\n"));
		return;
	}
	MIXINFO("BMixer::MixBuffer()\n");
	
	//prep stuff represents the actual MIX buffer here
	BBuffer *prep = mMixBuffer;
	uint32 prep_in = 0, prep_out = 0;	//frames!
	
	//buffer stuff represents the incoming buffer
	media_header *buffer_header = NULL;
	uint32 buffer_in = 0, buffer_out = 0;	//frames!
	
	//used with resampling
	size_t inEnd, outEnd = 0;	
	
	//channels per channel (yuck!)
	float gain[MAX_CHANNEL_COUNT];

	buffer_header = buffer->Header();
	BUFID(("buffer id: %d\n", buffer->ID()));
	//use array to get channel
	mixchannel *ch = GetChannel(buffer_header->destination);
	if (ch == NULL)
	{
		buffer->Recycle();
		buffer = NULL;
		return;
	}
	const media_format & channel_format = ch->Format();
	//determine buffer info
	uint32 buffer_channel_count = channel_format.u.raw_audio.channel_count;
	uint32 buffer_frame_count = buffer_header->size_used / ((channel_format.u.raw_audio.format & 0xf) * buffer_channel_count);
	float buffer_frame_rate = channel_format.u.raw_audio.frame_rate;
	double buffer_frame_time = 1000000.0 / (double)(channel_format.u.raw_audio.frame_rate);
	bigtime_t buffer_start = buffer_header->start_time;
	bigtime_t buffer_end = buffer_start + (bigtime_t)floor((buffer_frame_time * (double)buffer_frame_count) + 0.5);

	MIXINFO(("buffer_start: %Ld  buffer_end %Ld\n", buffer_start, buffer_end));
	MIXINFO(("mMixStartTime: %Ld  mNextMixTime %Ld\n", mMixStartTime, mNextMixTime));
	MIXINFO(("buffer_frame_time: %f   buffer_frame_count: %d\n",
			buffer_frame_time, buffer_frame_count));
	MIXINFO(("mMixFrameDuration: %f   mMixFrameCount: %d\n",
			mMixFrameDuration, mMixFrameCount));

	float scale = channel_format.u.raw_audio.frame_rate /
					mMixFormat.u.raw_audio.frame_rate;

#if RESAMPLE_DEBUG
	if (antiglitch)
	{
		RESAMPLEGLITCH(("ch %p buf_st %Ld mix_st %Ld buf_end %Ld\n",ch, buffer_start, mMixStartTime, buffer_end));
	}
#endif

	//if the buffer doesn't contain any data for this mix buffer
	if ( buffer_end < mMixStartTime )
	{
	
		RECYCLES(("recycle late buffer: %x : %Ld : %Ld\n", buffer, buffer_start, mNextMixTime));
		LATE(("mixer id:%d late buffer! Mix %Ld  Next mix %Ld  buf_start %Ld  buf_end is %Ld\n",ID(),mMixStartTime,mNextMixTime,buffer_start,buffer_end));
		LATE(("delta is %Ld\n",mMixStartTime - buffer_end));
		RESAMPLEGLITCH(("(b)mixer id:%d found late buffer! prep_in is %d buf_in is %d \n(b)Mix %Ld NextMix %Ld buf_start %Ld buf_end is %Ld\n", ID(), ch->mPrepIn, ch->mBuffIn, mMixStartTime,mNextMixTime, buffer_start,buffer_end));
		
		buffer->Recycle();
		buffer = NULL;
		
		return;
	}
	else if ( 	(buffer_start >= mNextMixTime) && 
				(buffer_start - (int64) floor(mMixFrameDuration)  < mNextMixTime) && 
				ch->mPrepIn >= (mMixFrameCount - 2)) {
		RESAMPLEGLITCH(("ANTIGLITCH buf_start %Ld mix_time %Ld next_mix %Ld prep_in is %d buff_in is %d\n",buffer_start, mMixStartTime, mNextMixTime, ch->mPrepIn, ch->mBuffIn));
		if (scale != 1.0) {
			//scale not 1.0, resampler will calculate actual prep_out, buffer_out
			if (ch->mLastMixTime + mMixBufferDuration + (int64) floor(mMixFrameDuration) < mMixStartTime) {

				RESAMPLEGLITCH(("reset based on lost buffer (inside antiglitch)\n"));
				//large gap means we broke the chain
				if (buffer_start >= mMixStartTime)
				{
					ch->mBuffIn = 0;
					ch->mPrepIn = (uint32)floor((double)(buffer_start - mMixStartTime) * mMixFormat.u.raw_audio.frame_rate / 1000000.0);
				}
				else
				{
					ch->mPrepIn = 0;
					ch->mBuffIn = (uint32)ceil((double)(mMixStartTime - buffer_start) * buffer_frame_rate / 1000000.0);
				}
			}
			ch->mLastMixTime = mMixStartTime;				
			buffer_in = ch->mBuffIn;
			buffer_out= buffer_frame_count - buffer_in;
			prep_in   = ch->mPrepIn;
			prep_out  = mMixFrameCount - prep_in;
		}
		else {
			TERROR(("early buffer with no scale (you should not be here)\n"));
			buffer->Recycle();
			buffer = NULL;
			return;		
		}
	}
	//if the buffer is early
	else if ( buffer_start >= mNextMixTime || ((mNextMixTime - buffer_start < (int64) floor(mMixFrameDuration)) && ch->mPrepIn == 0) )
	{
		RESAMPLEGLITCH(("early buffer start %Ld nextMixTime %Ld\n",buffer_start,mNextMixTime));
		MIXINFO(("early\n"));
		media_timed_event event(mNextMixTime, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
		event.bigdata = buffer->Header()->start_time;	//hplus
		mBufferQueue.AddEvent(event);
		buffer = NULL;
		return;		
	}
	//if the channel's format is conducive to synchronous operations
	else if (ch->IsSynchronous())
	{
		if (buffer_start <= mMixStartTime + mMixFrameDuration)
		{
			SYNCHRO(("playing synchronous buffer\n"));
			buffer_in = 0;
			buffer_out = buffer_frame_count;
			prep_in = 0;
			prep_out = mMixFrameCount;
		}
		else
		{
			SYNCHRO(("holding synchronous buffer\n"));
			media_timed_event event(mNextMixTime, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
			event.bigdata = buffer->Header()->start_time;	//hplus
			mBufferQueue.AddEvent(event);
			buffer = NULL;
			return;
		}
	}
	else
	{
		SYNCHRO(("NOT synchronous buffer\n"));
		//handles unchained and lost buffers
		if ((!ch->mChain) || ((scale != 1.0) && (ch->mLastMixTime + mMixBufferDuration + (int64) floor(mMixFrameDuration) < mMixStartTime))) {
			RESAMPLEGLITCH(("\nInitialize chain\n"));			

			ch->mLastMixTime = mMixStartTime;				
			ch->mChain  = TRUE;

			if (buffer_start >= mMixStartTime)
			{
				ch->mBuffIn = 0;
				ch->mPrepIn = (uint32)floor((double)(buffer_start - mMixStartTime) * mMixFormat.u.raw_audio.frame_rate / 1000000.0);
			}
			else
			{
				ch->mPrepIn = 0;
				ch->mBuffIn = (uint32)ceil((double)(mMixStartTime - buffer_start) * buffer_frame_rate / 1000000.0);
			}

			RESAMPLEGLITCHX(("bufin %d buf start %Ld mix start %Ld\n",ch->mBuffIn,buffer_start, mMixStartTime));

		}

		if (scale == 1.0) {
			//calculate for _mix_ routines
			ch->mLastMixTime = mMixStartTime;				
			buffer_in = ch->mBuffIn;
			buffer_out= buffer_frame_count - buffer_in;
			prep_in   = ch->mPrepIn;
			prep_out  = mMixFrameCount - prep_in;

			if (prep_out >  buffer_out) {
				prep_out =  buffer_out;
			}
			else {
				buffer_out = prep_out;
			}

		}
		else {
			//scale not 1.0, resampler will calculate actual prep_out, buffer_out
			//lost buffer case handled above...
			ch->mLastMixTime = mMixStartTime;				
			buffer_in = ch->mBuffIn;
			buffer_out= buffer_frame_count - buffer_in;
			prep_in   = ch->mPrepIn;
			prep_out  = mMixFrameCount - prep_in;

		}	

		//check for out of range
		if (/*prep_in < 0 ||*/ prep_in >= mMixFrameCount ||
			/*buffer_in < 0 ||*/ buffer_in >= buffer_frame_count ||
			buffer_in + buffer_out > buffer_frame_count || buffer_out <= 0 ||
			prep_in + prep_out > mMixFrameCount || prep_out <= 0)
		{
			TERROR(("BMixer::MixBuffer  prep out of range!!!!!!!!!!!!!!!!!!!!\n"));
			TERROR(("prep_in: %d\nprepout: %d\nprepmax: %d\nbuff_in: %d\nbuffout: %d\nbuffmax: %d\n", prep_in, prep_out, mMixFrameCount, buffer_in, buffer_out, buffer_frame_count));
			buffer->Recycle();
			buffer = NULL;
			return;
		}
	}

	//if !mute reformat, pan and gain
	if (!ch->Mute() && !mMaster->Mute() && mOutputEnabled && mHookedUp)
	{
		BUFFERS("prep is mixbuffer\n");
		prep = mMixBuffer;
		if (prep == NULL || prep->Data() == NULL)
		{
			TERROR(("Mixer - prep buffer not initialized!!!!!!!!!!\n"));
		}
		
		//if we aren't filling in from the start and the mix buffer is fresh
		if (prep_in > 0 && mMixBufFresh)
		{
			memset((char *)prep->Data(), 0, (prep_in * mMixFrameSize));
			MEMSETS(("1 memset prep_in %d (pi*fr/so(flt))%d \n",prep_in, prep_in*mMixFrameSize/sizeof(float)));
			RESAMPLEGLITCH(("1 bufstart %Ld mixstart %Ld\n",buffer_start, mMixStartTime));
		}
		//calculate pan and gain info including the master gain
		for(int y=0; y < (int) mMixFormat.u.raw_audio.channel_count; y++)
		{
			gain[y] = ch->PanAndGain(y) * mMaster->Gain(y);
		}							
		inEnd = buffer_out;
		outEnd = prep_out;
		
		_resampler_base *r = ch->Resampler();
		MIXINFO(("BMixer::SetupMix - scale : %f\n", scale));
		//if necessary swap, then resample and mix, or reformat and mix
		//here's a COPY(RW)
		switch(channel_format.u.raw_audio.format)
		{
			case media_raw_audio_format::B_AUDIO_UCHAR:
				MIXINFO("BMixer::SetupMix - process w/B_AUDIO_UCHAR\n");
				if (scale != 1.0)
				{
					r->resample_gain(((uchar *)buffer->Data())+(buffer_in*buffer_channel_count),inEnd,
							((float *)prep->Data())+(prep_in*mMixFormat.u.raw_audio.channel_count), outEnd, gain, !mMixBufFresh);
				}
				else
				{
					uchar *chars = (uchar *)buffer->Data() + (buffer_in * buffer_channel_count);
					float *flots = (float *)prep->Data() + (prep_in * mMixFormat.u.raw_audio.channel_count);
					BUFFERS(("buffer_in chars: %x prep_in flots: %x\n", chars, flots));
					_mix_uchar_to_float(!mMixBufFresh, chars, flots, gain, buffer_channel_count, mMixFormat.u.raw_audio.channel_count, prep_out);
				}
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				MIXINFO(("BMixer::SetupMix - process w/B_AUDIO_SHORT\n"));
				if (channel_format.u.raw_audio.byte_order != mMixFormat.u.raw_audio.byte_order)
				{
					swap_data(B_INT16_TYPE, (void *)(((int16 *)buffer->Data())+(buffer_in*buffer_channel_count)), (buffer_out*(sizeof(int16)*buffer_channel_count)), B_SWAP_ALWAYS);
				}
				if (scale != 1.0)
				{
					r->resample_gain(((short *)buffer->Data())+(buffer_in*buffer_channel_count),inEnd,
									((float *)prep->Data())+(prep_in*mMixFormat.u.raw_audio.channel_count), outEnd, gain, !mMixBufFresh);
				}
				else
				{
					short *shrts = (short *)buffer->Data() + (buffer_in * buffer_channel_count);
					float *flots = (float *)prep->Data() + (prep_in * mMixFormat.u.raw_audio.channel_count);
					BUFFERS(("buffer_in shrts: %x prep_in flots: %x\n", shrts, flots));
					_mix_short_to_float(!mMixBufFresh, shrts, flots, gain, buffer_channel_count, mMixFormat.u.raw_audio.channel_count, prep_out);					
				}
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				MIXINFO("BMixer::SetupMix - process w/B_AUDIO_INT\n");
				if (channel_format.u.raw_audio.byte_order != mMixFormat.u.raw_audio.byte_order)
				{
					swap_data(B_INT32_TYPE, (void *)(((int32 *)buffer->Data())+(buffer_in*buffer_channel_count)), (buffer_out*(sizeof(int32)*buffer_channel_count)), B_SWAP_ALWAYS);
				}
				if (scale != 1.0)
				{
					r->resample_gain(((int32 *)buffer->Data())+(buffer_in*buffer_channel_count),inEnd,
							((float *)prep->Data())+(prep_in*mMixFormat.u.raw_audio.channel_count), outEnd, gain, !mMixBufFresh);
				}
				else
				{
					int32 *ints = ((int32 *)buffer->Data()) + (buffer_in * buffer_channel_count);
					float *flots = ((float *)prep->Data()) + (prep_in * mMixFormat.u.raw_audio.channel_count);
					BUFFERS(("buffer_in ints: %x prep_in flots: %x\n", ints, flots));
					
					_mix_int32_to_float(!mMixBufFresh, ints, flots, gain, buffer_channel_count, mMixFormat.u.raw_audio.channel_count, prep_out);
				}
				break;
			case media_raw_audio_format::B_AUDIO_FLOAT:
			default:
				MIXINFO("BMixer::SetupMix - process w/B_AUDIO_FLOAT\n");
				if (channel_format.u.raw_audio.byte_order != mMixFormat.u.raw_audio.byte_order)
				{
					swap_data(B_FLOAT_TYPE, (void *)(((float *)buffer->Data())+(buffer_in*buffer_channel_count)), (buffer_out*(sizeof(float)*buffer_channel_count)), B_SWAP_ALWAYS);
				}
				if (scale != 1.0)
				{
#if RESAMPLE_DEBUG
					{
						int32 count = 0;
						float *in = ((float *)buffer->Data())+(buffer_in*buffer_channel_count);
						for (int i=0; i<inEnd; i++) {
							if (in[i] == 0.0) {
								RESAMPLEGLITCHX(("\t %d", i));
								count++;
							}
						}
						if (count > 0)
							RESAMPLEGLITCHX("%ld zero input samples found **\n", count);
					}
#endif
					r->resample_gain(((float *)buffer->Data())+(buffer_in*buffer_channel_count),inEnd,
							((float *)prep->Data())+(prep_in*mMixFormat.u.raw_audio.channel_count), outEnd, gain, !mMixBufFresh);

				}
				else
				{
					float *ins = ((float *)buffer->Data()) + (buffer_in * buffer_channel_count);
					float *outs = ((float *)prep->Data()) + (prep_in * mMixFormat.u.raw_audio.channel_count);
					BUFFERS(("buffer_in floats: %x prep_in floats: %x\n", ins, outs));
					_mix_float_to_float(!mMixBufFresh, ins, outs, gain, buffer_channel_count, mMixFormat.u.raw_audio.channel_count, prep_out);
				}
				break;
		}

		//get the actual values....(unchanged if scale == 1)
		if (scale != 1.0) {
			buffer_out -= inEnd;
			prep_out -= outEnd;
		}
		//if we didn't fillin the whole mix buffer and it's fresh, zero the rest
		if (prep_in+prep_out < mMixFrameCount && mMixBufFresh)
		{
			memset(((char *)prep->Data()) + ((prep_in+prep_out) * mMixFrameSize), 0,
						(mMixFrameCount - (prep_in+prep_out)) * mMixFrameSize);
			MEMSETS(("2 memset prep_out zeroed last %d mix start %Ld ch %p\n",(mMixFrameCount - (prep_in+prep_out)) , mMixStartTime,ch));


			//there are 3 cases: normal end of buffer, next buffer just after next mixtime, and next buffer WAY after nmt
			//should this be scaling only??
			bigtime_t cst = ChannelStartTime(ch, buffer_end - (int64)ceil(mMixFrameDuration));
			if ( cst < mNextMixTime ) {
				RESAMPLEGLITCHX(("No Problem\n"));
			}
			else if (cst < mNextMixTime + mMixBufferDuration + (int64) floor(mMixFrameDuration)  && scale != 1.0 ){
#if RESAMPLE_DEBUG
				antiglitch = (mMixFrameCount - (prep_in+prep_out));
#endif
				RESAMPLEGLITCH(("SEARCH for %d mix start %Ld  1st event %Ld current buf end %Ld\n",(mMixFrameCount - (prep_in+prep_out)),mMixStartTime, mBufferQueue.FirstEventTime(),buffer_end));
				AntiGlitch(ch);
			}
			else{
				RESAMPLEGLITCH(("Data too late @ %Ld\n",cst));
			}
		}
		prep = NULL;
		mMixBufFresh = false;
	}
	else {
		//just calculate the values to see if we can keep the channel frame count close....
		ch->mChain = false;
		if (scale != 1.0) {
			prep_out = (uint32)floor(((double)buffer_out * scale) +.5);
		}
	}
	
	//Update the values
	ch->mBuffIn += buffer_out;
	ch->mBuffIn %= buffer_frame_count;
	ch->mPrepIn += prep_out;
	ch->mPrepIn %= mMixFrameCount;

	ch->IncrementFrameCount(prep_out);

	//if there's stuff left in the popped buffer
	if (buffer_in+buffer_out < buffer_frame_count )
	{
		MIXINFO(("left %d\n", (buffer_frame_count-(buffer_in+buffer_out))));
		media_timed_event event(mNextMixTime, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
		event.bigdata = buffer->Header()->start_time;	//hplus
		mBufferQueue.AddEvent(event);
		buffer = NULL;
		return;
	}
	else
	{
		//recycle popped buffer
		RECYCLES(("done: %x\n", buffer));
		buffer->Recycle();
		buffer = NULL;
		return;
	}
} // BMixer::MixBuffer


bigtime_t
BMixer::ChannelStartTime(mixchannel * ch, bigtime_t the_time)
{
	antiglitch_struct ags;
	ags.mixer = this;
	ags.mchannel = ch;
	ags.event = NULL;
	// find the next event associated with the current ch
	// note: if the producer is sending discontiguous buffers
	// with a difference less than a mixbuffer, we will glitch.
	// THIS IS A PRODUCER PROBLEM!!!
	mBufferQueue.DoForEach(_find_channel_event, (void *)&ags, the_time, BTimedEventQueue::B_AFTER_TIME, false, BTimedEventQueue::B_HANDLE_BUFFER);
	if (ags.event != NULL) {
		BBuffer *buffer = (BBuffer *)ags.event->pointer;
		media_header * mh = buffer->Header();

		return mh->start_time;
	}
	else return B_INFINITE_TIMEOUT;
}

void
BMixer::AntiGlitch(mixchannel * ch)
{
	antiglitch_struct ags;
	ags.mixer = this;
	ags.mchannel = ch;
	ags.event = NULL;
	// find the next event associated with the current ch
	// note: if the producer is sending discontiguous buffers
	// with a difference less than a mixbuffer, we will glitch.
	// THIS IS A PRODUCER PROBLEM!!!

	mBufferQueue.DoForEach(_find_channel_event, (void *)&ags, mNextMixTime, BTimedEventQueue::B_AFTER_TIME, true, BTimedEventQueue::B_HANDLE_BUFFER);

	if (ags.event != NULL)
	{
		BBuffer *buffer = (BBuffer *)ags.event->pointer;

		RESAMPLEGLITCH(("RESCUE (start time is %Ld) \n",buffer->Header()->start_time));

		status_t err = mBufferQueue.RemoveEvent(ags.event);
		if (err >= B_OK)
		{
			media_timed_event event(mNextMixTime-1LL, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
			event.bigdata = buffer->Header()->start_time;	//hplus
			mBufferQueue.AddEvent(event);
			buffer = NULL;
		}
		else
		{
			TERROR(("Houston, we have a problem--negative removal of that queue event\n"));
		}
	}

	return;
}

BTimedEventQueue::queue_action
BMixer::_find_channel_event(media_timed_event *event, void *context)
{

	antiglitch_struct *ags = (antiglitch_struct *) context; 
	mixchannel *ch  = ags->mchannel;

	BBuffer *buffer = (BBuffer *)event->pointer;
	media_header * mh = buffer->Header();
	mixchannel *che  = ags->mixer->GetChannel( mh->destination );

	if (che == ch) {
			ags->event = event;
			RESAMPLEGLITCHX(("Found start %Ld mix %Ld \n",mh->start_time,event->event_time));
			return BTimedEventQueue::B_DONE;
	}
	else {
		RESAMPLEGLITCHX("Lost\n");
		return BTimedEventQueue::B_NO_ACTION;
	}
}

void
BMixer::OfflineCheck()
{
	OFFLINE(("BMixer::OfflineCheck\n"));
	mixchannel *ch = mChannelOne;
	while(ch){
		//look for channels that haven't received a buffer yet
		if(ch->WaitingForData(mNextMixTime, mMixFrameCount))
		{
			//found one, keep waiting for buffers
			return;
		}
		ch = ch->mNext;
	}
	//if all the channels are ready to go
	ch = mChannelOne;
	//zero the counters
	while (ch)
	{
		ch->ZeroFrameCount();
		ch = ch->mNext;
	}
	//if nothing's been mixed yet
	if (mMixBufFresh)
	{
		//send a buffer full of zeros
		memset((char *)mOutputBuffer->Data(), 0, mOutputFormat.u.raw_audio.buffer_size);
		MEMSETS("memset offline\n");
		mMixBufFresh = false;		
	}
	//send mix buffer
	if(mOutputEnabled && mHookedUp)
		SendMixBuffer();
	//increment frame count
	mMixFrameTotal += mMixFrameCount;
	OFFLINE(("mMixFrameTotal out: %Ld\n", mMixFrameTotal));
	//setup mix buffer
	mMixing = false;
	SetupMixBuffer();	
} // BMixer::OfflineCheck

status_t
BMixer::SendMixBuffer()
{
	mixchannel *ch = mChannelOne;

	SENDINFO("BMixer::SendMixBuffer\n");
	//if we've mixed into the mix buffer
	if (!mMixBufFresh)
	{
#if RESAMPLE_DEBUG
		{
			RESAMPLEGLITCHX("Stale strt %Ld\n",mMixStartTime);
			int32 count = 0;
			float *out = (float *)mMixBuffer->Data();
			for (int i=0; i<mMixFrameCount*2; i++) {

				if (out[i] == 0.0) {
					RESAMPLEGLITCHX("\t %d", i);
					count++;
				}
			}

			if ( /*count > 2  ||*/ ((count == (antiglitch<<1)) && antiglitch  ) ){
				RESAMPLEGLITCH("SendMixBuffer GLITCH %ld @ %Ld\n", count,mMixStartTime);
				RESAMPLEGLITCHX("Q depth %d (1st event %Ld us)\n", mBufferQueue.EventCount(), mBufferQueue.FirstEventTime());
				if (mBufferQueue.EventCount() > 0) {
					RESAMPLEGLITCHX("time delta %Ld\n", mBufferQueue.FirstEvent()->bigdata-
						mBufferQueue.FirstEventTime());
				}
			}
			antiglitch = 0;
		}
#endif

		//zero the state
		while (ch)
		{
			ch->mPrepIn = 0;
			ch = ch->mNext;
		}

		if (mDataStatus == B_DATA_NOT_AVAILABLE)
		{
			mDataStatus = B_DATA_AVAILABLE;
			DATASTATUS(("BMixer  ********  calling SendDataStatus status: %d\n", mDataStatus));
			SendDataStatus(mDataStatus, mOutputDestination, mNextMixTime);
		}
		if (mOutputBuffer != mMixBuffer)
		{
			if (mOutputFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_SHORT)
			{
				CONVERSION(("SendMixBuffer: converting float to int\n"));
				convertBufferFloatToShort( (int16 *)(mOutputBuffer->Data()),
											(float *)(mMixBuffer->Data()),
											mMixFrameCount * mMixFormat.u.raw_audio.channel_count,
											32767.0);
			}
			//dug - these cases don't exist yet on PPC and we don't accept these connections
			else if (mOutputFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_INT)
			{
#if 0
				convertBufferFloatToInt( (int32 *)(mOutputBuffer->Data()),
											(float *)(mMixBuffer->Data()),
											1.0,
											mMixFrameCount * mMixFormat.u.raw_audio.channel_count );
#endif
				TERROR(("Mixer: Ooops - someone is mixing to ints?"));
			}
			else
			{
#if 0
				convertBufferFloatToUByte( (int32 *)(mOutputBuffer->Data()),
											(float *)(mMixBuffer->Data()),
											1.0,
											mMixFrameCount * mMixFormat.u.raw_audio.channel_count );
#endif
				TERROR(("Mixer: Ooops - someone is mixing to uchars?"));
			}
		}
	}
	//if not, but we haven't notified the destination yet
	else if (mDataStatus == B_DATA_AVAILABLE)
	{
		//send one more buffer full of zeros before stopping
		memset((char *)mOutputBuffer->Data(), 0, mOutputFormat.u.raw_audio.buffer_size);
		MEMSETS("memset last buffer\n");
		RESAMPLEGLITCH(("clear last buffer\n"));
		mMixBufFresh = false;
		mDataStatus = B_DATA_NOT_AVAILABLE;
		DATASTATUS(("BMixer  calling SendDataStatus status: %d *********** \n", mDataStatus));
		SendDataStatus(mDataStatus, mOutputDestination, mNextMixTime);

		//unchain the melody
		ch = mChannelOne;
		//zero the state
		while (ch)
		{
			ch->mChain = false;
			ch = ch->mNext;
		}
	}
	else
	{
		//reset buff state to 0 here?
		return B_OK;
	}

	//send the mix buffer
	mOutputBuffer->Header()->start_time = mMixBuffer->Header()->start_time;
	mOutputBuffer->Header()->size_used = mOutputFormat.u.raw_audio.buffer_size;
	mOutputBuffer->Header()->time_source = TimeSource()->ID();
	mOutputBuffer->Header()->type = B_MEDIA_RAW_AUDIO;
	status_t err = B_ERROR;	
	SENDINFO("BMixer::SendMixBuffer\n");	
	err = SendBuffer(mOutputBuffer, mOutputDestination);
	if (err < B_OK)
	{
		TERROR(("BMixer sendbuffer error %x\n", err));
		mOutputBuffer->Recycle();
	}
	if(mMixBuffer == mOutputBuffer)
	{
		mMixBuffer = NULL;
	}
	mOutputBuffer = NULL;
	
	return err;
} // BMixer::SendMixBuffer


void
BMixer::SetupMixBuffer()
{
	SETUP(("BMixer::SetupMixBuffer\n"));
	if(!mOutputBuffer)
	{
		SETUP(("SetupMixBuffer: Requesting mOutputBuffer\n"));
		mOutputBuffer = mOutputGroup->RequestBuffer(mOutputFormat.u.raw_audio.buffer_size);
	}
	if (!mMixBuffer)
	{
		if (mMixFormat.u.raw_audio.format != mOutputFormat.u.raw_audio.format)
		{
			SETUP(("SetupMixBuffer: Requesting mMixBuffer\n"));
			mMixBuffer = mMixGroup->RequestBuffer(mMixFormat.u.raw_audio.buffer_size);
		}
		else
		{
			mMixBuffer = mOutputBuffer;
		}
	}
	
	
	//figure out when next buffer is due to play
	mMixStartTime = mNextMixTime;
	mNextMixTime = mStartTime + (bigtime_t)floor((mMixFrameTotal + mMixFrameCount) * 1000000.0
												/ (double)mMixFormat.u.raw_audio.frame_rate);
	mMixBuffer->Header()->start_time = mMixStartTime;
	mMixBufFresh = true;
	
	//figure out our deadlines
	if (RunMode() == B_OFFLINE)
	{
		SETUP(("Offline mode\n"));
		mOffline = true;
		mMixing = true;
	}
	else
	{
		SETUP(("mix: %Ld\n", mMixStartTime));
		media_timed_event event(mMixStartTime, TIME_TO_MIX);
		mEventQueue.AddEvent(event);
		mTimeToSend = TimeSource()->RealTimeFor(mMixStartTime, -mMixLatency);
		
		event.event_time = TimeSource()->PerformanceTimeFor(mTimeToSend);
		event.type = TIME_TO_SEND;
		event.bigdata = mTimeToSend;
		status_t err = mEventQueue.AddEvent(event);
		if (err < B_OK)
			TERROR(("Holy Cow Batman! We failed to add the send event!\n"));
		SETUP(("snd: %Ld\n", TimeSource()->PerformanceTimeFor(mTimeToSend)));
	}
	while (mBufferQueue.FirstEventTime() < mNextMixTime)
	{
		media_timed_event event;
		status_t err = mBufferQueue.RemoveFirstEvent(&event);
		if (err < B_OK)
		{
			return;
		}
		
		if (event.type == BTimedEventQueue::B_HANDLE_BUFFER)
		{
			MixBuffer((BBuffer *)event.pointer);
		}
	}
}

void
BMixer::HandleEvent()
{
	HANDLE(("BMixer::HandleEvent\n"));
	if (!mEventQueue.HasEvents())
	{
		return;
	}

	media_timed_event event;
	status_t err = mEventQueue.RemoveFirstEvent(&event);

	if (err < B_OK)
	{
		return;
	}
	switch(event.type)
	{
		case BTimedEventQueue::B_START:
			HANDLE(("BMixer::B_START %Ld\n", event.event_time));
			if (!mRunning)
			{
				mRunning = true;
				mStartTime = event.event_time;
				mNextMixTime = mStartTime;
				mMixFrameTotal = 0;
				SetupMixBuffer();
			}
			break;
		case BTimedEventQueue::B_STOP:
			HANDLE(("BMixer::B_STOP %Ld\n", event.event_time));
			if (mRunning)
			{
				mRunning = false;
				mEventQueue.FlushEvents(event.event_time, BTimedEventQueue::B_AFTER_TIME);
				if (mMixBuffer)
					mMixBuffer->Recycle();	
				if (mOutputBuffer && mOutputBuffer != mMixBuffer)
					mOutputBuffer->Recycle();
				mMixBuffer = NULL;
				mOutputBuffer = NULL;
				mDataStatus = B_DATA_NOT_AVAILABLE;
				SendDataStatus(mDataStatus, mOutputDestination, event.event_time);
			}
			break;
		case BTimedEventQueue::B_SEEK:
			HANDLE(("BMixer::B_SEEK %Ld\n", event.event_time));
			//nothing
			break;
		case BTimedEventQueue::B_WARP:
			HANDLE(("BMixer::B_WARP %Ld\n", event.event_time));
			if	(mRunning)
			{
				mEventQueue.FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, TIME_TO_MIX);
				mEventQueue.FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, TIME_TO_SEND);
				mNextMixTime = event.bigdata;
				//adjust start time back from new next time
				mStartTime = mNextMixTime - (bigtime_t)floor((mMixFrameTotal + mMixFrameCount) * 1000000.0
															/ (double)mMixFormat.u.raw_audio.frame_rate);
				SetupMixBuffer();
			}
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			HANDLE(("BMixer::B_HANDLE_BUFFER %Ld\n", event.event_time));
			if (!mRunning)
			{
				((BBuffer *)event.pointer)->Recycle();
			}
			else if (mMixing)
			{
				MixBuffer((BBuffer *)event.pointer);
				if(mOffline)
					OfflineCheck();
			}
			else
			{
				HANDLE(("Push buffer back %Ld\n", mMixStartTime));
				event.event_time = mMixStartTime;
				mBufferQueue.AddEvent(event);
			}
			break;
		case BTimedEventQueue::B_DATA_STATUS:
		{
			HANDLE(("BMixer::B_DATA_STATUS %Ld\n", event.event_time));

			int32 channel = (int32)event.pointer;
			if (event.data == B_DATA_NOT_AVAILABLE) {
				mBufferQueue.DoForEach(_remove_connection_buffers, (void *)channel, event.event_time, BTimedEventQueue::B_AFTER_TIME, false, BTimedEventQueue::B_HANDLE_BUFFER);
			}
			mixchannel *ch = GetChannel(channel);
			if (ch == NULL)
			{
				break;
			}
			ch->SetDataStatus((int32)event.bigdata, event.event_time);
			break;
		}
		case TIME_TO_MIX:
			HANDLE(("BMixer::TIME_TO_MIX %Ld\n", event.event_time));
			//set mixing flag
			mMixing = true;
			break;
		case TIME_TO_SEND:
			HANDLE(("BMixer::TIME_TO_SEND %Ld\n", event.event_time));
			//unset mixing flag
			mMixing = false;
			//send mix buffer
			if(mOutputEnabled && mHookedUp)
				SendMixBuffer();
			//increment frame count
			mMixFrameTotal += mMixFrameCount;
			SENDINFO(("mMixFrameTotal out: %Ld\n", mMixFrameTotal));
			RESAMPLEGLITCHX(("mMixFrameTotal out: %Ld\n", mMixFrameTotal));
			//setup new mix buffer
			SetupMixBuffer();		
			break;
		case BTimedEventQueue::B_NO_EVENT:
			HANDLE(("BMixer::B_NO_EVENT %Ld\n", event.event_time));
		default:
			return;
	}
}

#define ENOUGH_TIME_FOR_SOMEONE_ELSE_TO_DO_SOMETHING_USEFUL 100

void
BMixer::MixRun()
{
	mSchedulingLatency = estimate_max_scheduling_latency();
	bigtime_t latency = mMixLatency + mSendLatency + mDownStreamLatency + mSchedulingLatency;
	bigtime_t wait_until = B_INFINITE_TIMEOUT;
	status_t err = B_OK;
	int32 code = 0;

	while(mActive)
	{
		latency = mMixLatency + mSendLatency + mDownStreamLatency + mSchedulingLatency;
		//setup time
		if (TimeSource()->IsRunning() && mEventQueue.HasEvents())
		{
			MIXRUN(("MixRun - NextEventTime: %Ld\n", mEventQueue.FirstEventTime()));
			wait_until = TimeSource()->RealTimeFor(mEventQueue.FirstEventTime(), latency);
			MIXRUN(("MixRun - wait for %Ld micros\n", wait_until-system_time()));
		}
		else
		{
			wait_until = B_INFINITE_TIMEOUT;
			MIXRUN(("MixRun - wait forever\n"));
		}
		//read message
		if (system_time() < wait_until - ENOUGH_TIME_FOR_SOMEONE_ELSE_TO_DO_SOMETHING_USEFUL)
		{
			MIXRUN(("MixRun - read_port_etc\n"));
			err = read_port_etc(mControlPort, &code, &mMessage, B_MEDIA_MESSAGE_SIZE, B_ABSOLUTE_TIMEOUT, wait_until);
		}
		else
		{
			MIXRUN(("MixRun - not enough time\n"));
			err = B_TIMED_OUT;
		}
		//handle something
		if (err == B_TIMED_OUT)
		{
			MIXRUN(("MixRun - handle event\n"));
			HandleEvent();
		}
		else if(code == B_ABSTAIN_YOU_DUMMY)
		{
			MIXRUN(("MixRun - B_ABSTAIN_YOU_DUMMY\n"));
			return;
		}
		else if (err >= 0)
		{
			MIXRUN(("MixRun - handle message\n"));
			BMixer::HandleMessage(code, &mMessage, err);
		}
		else if (err != B_INTERRUPTED)
		{
			MIXRUN(("MixRun - error: %s\n", strerror(err)));
			TERROR(("BMixer::MixRun() - Unexpected error:\n $s\n", strerror(err)));
			return;
		}
	}
}

status_t
BMixer::MixThreadEntry(void * data)
{
	((BMixer *)data)->MixRun();
	return B_OK;
}


BTimedEventQueue::queue_action
_remove_connection_buffers(media_timed_event *event, void *context)
{
	int32 channel = (int32)context;
	BBuffer *buffer = (BBuffer *)event->pointer;
	if (buffer->Header()->destination == channel)
		return BTimedEventQueue::B_REMOVE_EVENT;
	else return BTimedEventQueue::B_NO_ACTION;
}
