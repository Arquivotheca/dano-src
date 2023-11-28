#include <media2/SoundPlayer.h>
#include <support2/Autolock.h>

#include <kernel/scheduler.h>

#include <SoundPlayerNode.h>		// from media2_p

namespace B {
namespace Media2 {

#if 0

BSoundPlayer::BSoundPlayer (const char *name,
							BufferPlayerFunc PlayBuffer,
							void *cookie)
	: mInitCheck(B_NO_INIT),
	  mPlayBuffer(PlayBuffer),
	  mCookie(cookie),
	  mRunning(false)	  
{
	mImpl=new B::Private::BSoundPlayerNode(name);
	
	Init(BMediaConstraint(true),BMediaRoster::Roster()->AudioMixer());
}

BSoundPlayer::BSoundPlayer (const BMediaConstraint &c,
							const char *name, BufferPlayerFunc PlayBuffer,
							void *cookie)
	: mInitCheck(B_NO_INIT),
	  mPlayBuffer(PlayBuffer),
	  mCookie(cookie),
	  mRunning(false)	  
{
	mImpl=new B::Private::BSoundPlayerNode(name);

	Init(c,BMediaRoster::Roster()->AudioMixer());
}

#endif

BSoundPlayer::BSoundPlayer (IMediaInput::ptr *toInput,
							const char *name,
							BufferPlayerFunc PlayBuffer,
							void *cookie)
	: mInitCheck(B_NO_INIT),
	  mPlayBuffer(PlayBuffer),
	  mCookie(cookie),
	  mRunning(false)
{
	mImpl=new B::Private::BSoundPlayerNode(name);

	Init(BMediaConstraint(true),toInput);
}

BSoundPlayer::BSoundPlayer (const BMediaConstraint &c,
							IMediaInput::ptr *toInput,
							const char *name,
							BufferPlayerFunc PlayBuffer,
							void *cookie)
	: mInitCheck(B_NO_INIT),
	  mPlayBuffer(PlayBuffer),
	  mCookie(cookie),
	  mRunning(false)
{
	mImpl=new B::Private::BSoundPlayerNode(name);

	Init(c,toInput);
}

BSoundPlayer::~BSoundPlayer()
{
	if (mInitCheck>=B_OK)
	{
		if (mRunning)
			Stop();
		
		dynamic_cast<B::Private::BSoundPlayerNode *>(mImpl.ptr())->DisconnectSelf();
	}
}

status_t 
BSoundPlayer::InitCheck() const
{
	return mInitCheck;
}

BMediaFormat
BSoundPlayer::Format() const
{
	return dynamic_cast<B::Private::BSoundPlayerNode *>(mImpl.ptr())->Format();
}

status_t 
BSoundPlayer::Start()
{
	if (mRunning)
		return B_ERROR;
	
	mDone=false;
	
	mTID=spawn_thread(SoundPlayerThread,"SoundPlayer",
						B_NORMAL_PRIORITY,//suggest_thread_priority(B_AUDIO_PLAYBACK),
						this);

	if (mTID<B_OK)
		return mTID;
	
	status_t result=resume_thread(mTID);
	
	if (result<B_OK)
	{
		kill_thread(mTID);
		mTID=B_ERROR;
		
		return result;
	}
	
	mRunning=true;
	
	return B_OK;
}

status_t 
BSoundPlayer::Stop()
{
	if (!mRunning)
		return B_ERROR;
	
	mDone=true;
	status_t result;
	while (wait_for_thread(mTID,&result)==B_INTERRUPTED)
		;
	
	mTID=B_ERROR;

	mRunning=false;
	
	return result;
}

void 
BSoundPlayer::SetBufferPlayer (BufferPlayerFunc PlayBuffer, void *cookie)
{
	BAutolock autolock(mLock.Lock());
	
	mPlayBuffer=PlayBuffer;
	mCookie=cookie;
}

BSoundPlayer::BufferPlayerFunc 
BSoundPlayer::BufferPlayer() const
{
	BAutolock autolock(mLock.Lock());
	
	return mPlayBuffer;
}

void *
BSoundPlayer::Cookie() const
{
	BAutolock autolock(mLock.Lock());
	
	return mCookie;
}

void
BSoundPlayer::Init (const BMediaConstraint &c, IMediaInput::ptr *toInput)
{
	BMediaEndpointVector outputs;
	mImpl->ListEndpoints(&outputs,B_OUTPUT_ENDPOINT,B_FREE_ENDPOINT);
	
	IMediaOutput::ptr output=IMediaOutput::AsInterface(outputs[0]->AsBinder());

	BMediaFormat format;
	mInitCheck=output->Connect(toInput,&format,c);
}

int32 
BSoundPlayer::SoundPlayerThread(void *param)
{
	return static_cast<BSoundPlayer *>(param)->SoundPlayerThread();
}

int32 
BSoundPlayer::SoundPlayerThread()
{
	BMediaOutput::ptr output=dynamic_cast<B::Private::BSoundPlayerNode *>(mImpl.ptr())->Output();
	BMediaFormat format=dynamic_cast<B::Private::BSoundPlayerNode *>(mImpl.ptr())->Format();
		
	while (!mDone)
	{
		BBuffer buffer;
		if (output->AcquireBuffer(&buffer)>=B_OK)
		{
			BAutolock autolock(mLock.Lock());
			
			if (mPlayBuffer!=NULL)
			{
				mPlayBuffer(mCookie,buffer.Data(),buffer.Size(),format);
				
				if (output->SendBuffer(&buffer)<B_OK)
					buffer.ReleaseBuffer();
			}
			else
				buffer.ReleaseBuffer();
		}
	}
	
	return B_OK;
}

} } // B::Media2
