#include <media2/AudioResampler.h>
#include <media2/MediaFile.h>
#include <media2/MediaTrack.h>
#include "MixerCoreNode.h"
#include "MixerOutput.h"
#include "MixerInput.h"

#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#include <scheduler.h>

#include <cstring>

#define checkpoint \
//berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Private {

MixerOutput::MixerOutput() :
	BMediaOutput("mix.out"),
	mLock("MixerOutput::mLock")
{
}

MixerOutput::~MixerOutput()
{
}

status_t 
MixerOutput::Acquired(const void *id)
{
	BMediaOutput::Acquired(id);

	mMixBuffer = 0;

	mThread = -1;
	mSem = -1;
	
	mLogThread = -1;
	mLogSem = -1;
	mLogBuffer = 0;
	mLogFile = 0;
	mLogTrack = 0;
	
	mFramesProduced = 0LL;

	return B_OK;
}

status_t 
MixerOutput::Released(const void *id)
{
//	mLock.Lock();
	_Disconnected();
//	mLock.Unlock();
	
	StopLogging();
	
	mInputs.MakeEmpty();
	return BMediaOutput::Released(id);
}

status_t 
MixerOutput::Connect(IMediaInput::ptr *ioInput, BMediaFormat *outFormat, const BMediaConstraint &constraint)
{
	status_t err = BMediaOutput::Connect(ioInput, outFormat, constraint);
	if (err < B_OK) return err;
	
	// we're now connected; prepare to send buffers
	mLock.Lock();
	err = _Connected(*ioInput, *outFormat);
	mLock.Unlock();

	if (err < B_OK)
	{
berr << "_Connected(): " << strerror(err) << endl;		
		Disconnect();
	}

	return err;
}

status_t 
MixerOutput::Disconnect()
{
	mLock.Lock();
	_Disconnected();
	mLock.Unlock();

	return BMediaOutput::Disconnect();
}

void 
MixerOutput::ResetInput(const atom_ptr<MixerInput> &input)
{
	input->SetBufferOffset(0);
}

status_t 
MixerOutput::GetMixFormat(media_multi_audio_format *outFormat)
{
	if (mConnectedInput == 0) return B_NOT_ALLOWED;
	outFormat->format = mMixType;
	outFormat->channel_count = mMixChannels;
	outFormat->byte_order = B_MEDIA_HOST_ENDIAN;
	outFormat->buffer_size = mMixFrameSize * mOutputFrameCount;
	outFormat->frame_rate = mOutputFrameRate;
	return B_OK;
}

status_t 
MixerOutput::StartLogging(
	const entry_ref &ref, const media_file_format &fileFormat)
{
	status_t err;
	BAutolock _l(mLock.Lock());
	if (mConnectedInput == 0) return B_NOT_ALLOWED;
	if (mLogThread >= B_OK) return B_NOT_ALLOWED;

	media_format f;
	f.type = B_MEDIA_RAW_AUDIO;
	media_multi_audio_format & af = f.u.raw_audio;
	af.format = mOutputType;
	af.frame_rate = mOutputFrameRate;
	af.channel_count = mOutputChannels;
	af.byte_order = B_MEDIA_HOST_ENDIAN;
	af.buffer_size = mOutputFrameSize * mOutputFrameCount;

//berr << "logging " << mOutputFrameCount << " frames of " << mOutputFrameSize << endl;	
	mLogInPage = 0;
	mLogPageSize = af.buffer_size;
	mLogPageCount = 4;
	mLogBuffer = new int8[mLogPageSize * mLogPageCount];

	mLogFile = new BMediaFile(&ref, &fileFormat);
	err = mLogFile->InitCheck();
	if (err < B_OK)
	{
berr << "MixerOutput::StartLogging(): mLogFile->InitCheck: " << strerror(err) << endl;
		goto err0;
	}

	mLogTrack = mLogFile->CreateTrack(&f);
	err = mLogTrack ? mLogTrack->InitCheck() : B_ERROR;
	if (err < B_OK)
	{
berr << "MixerOutput::StartLogging(): CreateTrack: " << strerror(err) << " " << mLogTrack << endl;
		goto err0;
	}

	mLogFile->CommitHeader();

	mLogSem = create_sem(0, "mixerlog.throttle");
	if (mLogSem < B_OK) goto err0;
	
	mLogThread = spawn_thread(&LogThreadEntry, "mixerlog", B_NORMAL_PRIORITY, this);
	if (mLogThread < B_OK) goto err1;
	
	resume_thread(mLogThread);
	return B_OK;

err1:
	delete_sem(mLogSem);
	mLogSem = -1;
err0:
	delete mLogFile;
	mLogFile = 0;
	mLogTrack = 0;
	return err;
}

status_t 
MixerOutput::StopLogging()
{
	thread_id tid;
	
	{
		BAutolock _l(mLock.Lock());
		if (mLogThread < B_OK) return mLogThread;
		if (mLogSem < B_OK) return mLogSem;
		delete_sem(mLogSem);
		mLogSem = -1;
		tid = mLogThread;
	}

	status_t err;
	while (wait_for_thread(tid, &err) == B_INTERRUPTED) {}
	 
	BAutolock _l(mLock.Lock());
	mLogThread = -1;
	if (mLogFile)
	{
		if (mLogTrack) mLogTrack->Flush();
		delete mLogFile;
		mLogFile = 0;
		mLogTrack = 0;
	}
	delete [] mLogBuffer;
	mLogBuffer = 0;

	return B_OK;
}

void 
MixerOutput::AttachedToNode(const atom_ptr<BMediaNode> &node)
{
	MixerCoreNode::ptr mixer = dynamic_cast<MixerCoreNode*>(node.ptr());
	ASSERT(node != 0);

	media_multi_audio_format mixFormat;
	mixer->GetMixFormat(&mixFormat);

	mMixType = (raw_audio_type)mixFormat.format;
	mMixChannels = mixFormat.channel_count;
	mMixFrameSize = (mMixType & B_AUDIO_SIZE_MASK) * mMixChannels;

	BMediaConstraint constraint;

	constraint.And(B_FORMATKEY_RAW_AUDIO_TYPE, BValue::Int32(B_AUDIO_FLOAT));
	constraint.Or(B_FORMATKEY_RAW_AUDIO_TYPE, BValue::Int32(B_AUDIO_INT16));
	constraint.Or(B_FORMATKEY_RAW_AUDIO_TYPE, BValue::Int32(B_AUDIO_UINT8));

	constraint.
		And(B_FORMATKEY_MEDIA_TYPE, BValue::Int32(B_MEDIA_RAW_AUDIO)).
		And(B_FORMATKEY_CHANNEL_COUNT, BValue::Int32(mMixChannels)).
		And(B_FORMATKEY_BUFFER_FRAMES, BMediaConstraintItem::B_GE, BValue::Int32(1)).
		And(B_FORMATKEY_BYTE_ORDER, BValue::Int32(B_MEDIA_HOST_ENDIAN));

	SetConstraint(constraint);
	
	BMediaPreference preference;
	preference.AddItem(B_FORMATKEY_FRAME_RATE, BValue::Float(44100.0f));
	SetPreference(preference);
}

void 
MixerOutput::DetachedFromNode(const atom_ptr<BMediaNode> &)
{
}

status_t 
MixerOutput::AttachInput(MixerInput::arg input)
{
	if (input == 0) return B_BAD_VALUE;
	BAutolock _l(mLock.Lock());
	mInputs.AddItem(input);
	return B_OK;
}

status_t 
MixerOutput::DetachInput(MixerInput::arg input)
{
	if (input == 0) return B_BAD_VALUE;
	BAutolock _l(mLock.Lock());
	for (int32 n = mInputs.CountItems()-1; n >= 0; n--)
	{
		if (mInputs[n] == input)
		{
			mInputs.RemoveItemsAt(n,1);
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

void 
MixerOutput::BufferQueued(MixerInput::arg input)
{
	BAutolock _l(mLock.Lock());
	if (!(mState & RUNNING))
	{
		// start the output when enough buffers have accumulated at a given input
		// to produce a full output buffer.
		size_t minFrames = (size_t)
			((float)mOutputFrameCount * (input->FrameRate() / mOutputFrameRate) + 0.5f);
		if (input->FramesQueued() >= minFrames)
		{
			release_sem(mSem);
			mState |= RUNNING;
		}
	}
}

status_t 
MixerOutput::_Connected(IMediaInput::arg connectedInput, const BMediaFormat &format)
{
	if (mConnectedInput != 0) return B_NOT_ALLOWED;
	
	status_t err = B_OK;

	// set output format
	mOutputType = (raw_audio_type)format[B_FORMATKEY_RAW_AUDIO_TYPE].AsInteger();
	mOutputChannels = format[B_FORMATKEY_CHANNEL_COUNT].AsInteger();
	mOutputFrameSize = (mOutputType & B_AUDIO_SIZE_MASK) * mOutputChannels;
	ASSERT(mOutputFrameSize);
	mOutputFrameCount = format[B_FORMATKEY_BUFFER_FRAMES].AsInteger();
	ASSERT(mOutputFrameCount);
	err = format[B_FORMATKEY_FRAME_RATE].GetFloat(&mOutputFrameRate);
	ASSERT(err >= B_OK);

	// allocate mix buffer if needed
	if (mOutputType != mMixType)
	{
		size_t mixBufferSize = mOutputFrameCount * mMixFrameSize;
		mMixBuffer = new int8[mixBufferSize];
	}
	
	// +++ broadcast buffer requirements

	mSem = create_sem(0, "mix.run");
	if (mSem < B_OK)
	{
		err = mSem;
		goto err0;
	}

	mThread = spawn_thread(
		&MixThreadEntry,
		"mix.thread",
		120,
		(void*)this);
	if (mThread < B_OK)
	{
		err = mThread;
		goto err1;
	}

	mConnectedInput = connectedInput;
	mEmptyCount = 0;
	mState = 0;

	resume_thread(mThread);	
	return B_OK;

err1:
	delete_sem(mSem);
	mSem = -1;
err0:
berr << "MixerOutput::_Connected(): FAILED: " << strerror(err) << endl;
	return err;
}

void 
MixerOutput::_Disconnected()
{
	if (mConnectedInput == 0) return;
	if (atomic_or(&mState, STOPPING) & STOPPING) return;
	delete_sem(mSem);
	mSem = -1;
	status_t err;
	thread_id tid = mThread;
	mLock.Unlock();
berr << "_D\n";
checkpoint
	while (wait_for_thread(tid, &err) == B_INTERRUPTED) {}
berr << "_D ok\n";
checkpoint
	mLock.Lock();
	mThread = -1;
	if (mMixBuffer)
	{
		delete [] mMixBuffer;
		mMixBuffer = 0;
	}
	mConnectedInput = 0;
	atomic_and(&mState, ~STOPPING);
}

status_t 
MixerOutput::MixThreadEntry(void *cookie)
{
	((MixerOutput*)cookie)->MixThread();
	return B_OK;
}

void 
MixerOutput::MixThread()
{
checkpoint
	status_t err = B_OK;
	while (!(mState & STOPPING) && err == B_OK)
	{
checkpoint
		err = acquire_sem(mSem);
		if (err < B_OK) break;
checkpoint
		
		bool keepPushing;
		do
		{
			if (mState & STOPPING) break;
			err = PushBuffer(keepPushing);
			if (err < B_OK)
			{
berr << "MixerOutput::PushBuffer(): " << strerror(err) << endl;
				break;
			}
		}
		while (keepPushing);
	}
checkpoint
}

status_t 
MixerOutput::PushBuffer(bool & outKeepPushing)
{
	outKeepPushing = true;
	
	// fetch the output buffer
	BBuffer buffer;
checkpoint
	status_t err = AcquireBuffer(&buffer);
checkpoint
	if (err < B_OK)
	{
berr << "MixerOutput::PushBuffer(): AcquireBuffer(): " << strerror(err) << endl;
		return err;
	}

	void * mixBuffer = mMixBuffer ? mMixBuffer : buffer.Data();
	ASSERT(mixBuffer);

checkpoint
	// gather active inputs
	mLock.Lock();
	BVector<MixerInput::ptr> liveInputs;
	const size_t inputCount = mInputs.CountItems();
	for (size_t n = 0; n < inputCount; n++)
	{
		if (mInputs.EditItemAt(n)->TopBuffer()) liveInputs.AddItem(mInputs[n]);
	}
	if (!liveInputs.CountItems())
	{
		if (++mEmptyCount == 3)
		{
			// stop producing empty buffers; on exit, block until a new input
			// buffer has been received.
			mState &= ~RUNNING;
			outKeepPushing = false;
		}
	}
	mLock.Unlock();

checkpoint
	Mix(mixBuffer, liveInputs);
	mFramesProduced += mOutputFrameCount;

	if (mixBuffer == mMixBuffer)
	{
		// convert to output format
		switch (mOutputType)
		{
			case B_AUDIO_UINT8:
				BAudioResampler::ConvertFloatToUInt8(
					(const float*)mixBuffer, (uint8*)buffer.Data(),
					mOutputFrameCount * mOutputChannels);
				break;
			case B_AUDIO_INT16:
				BAudioResampler::ConvertFloatToInt16(
					(const float*)mixBuffer, (int16*)buffer.Data(),
					mOutputFrameCount * mOutputChannels);
				break;
			default:
				TRESPASS();
				break;
		}
	}
	
	sem_id sem = mLogSem;
	if (sem >= B_OK)
	{
		BAutolock _l(mLock.Lock());
		memcpy(mLogBuffer + mLogInPage * mLogPageSize, buffer.Data(), mLogPageSize);
		if (++mLogInPage == mLogPageCount) mLogInPage = 0;
		release_sem(sem);
	}
	
	err = SendBuffer(&buffer);
	if (err < B_OK)
	{
		buffer.ReleaseBuffer();
	}
	return err;
}

void 
MixerOutput::Mix(void *mixBuffer, const BVector<MixerInput::ptr> &inputs)
{
	ASSERT(mixBuffer);
	const size_t count = inputs.CountItems();
	memset(mixBuffer, 0, mOutputFrameCount * mMixFrameSize);
	float gains[8];
	for (size_t n = 0; n < count; n++)
	{
		MixerInput::ptr input = inputs[n];
		size_t inOffset = input->BufferOffset();
		size_t outOffset = 0;
		const size_t inFrameSize = input->FrameSize();
		for (size_t toWrite = mOutputFrameCount; toWrite;)
		{
			const BBuffer * inBuffer = input->TopBuffer();
			if (!inBuffer)
			{
berr << "MixerOutput::Mix(): no buffer for input " << n << " at "
	 <<	mFramesProduced << " frames." << endl;
				break;
			}
			else
			{
//berr << "MIX: buffer " << inBuffer->ID() << " inOffset " << inOffset << ", outOffset " << outOffset << endl;
			}
			size_t inSize = inBuffer->Size();
			if (inOffset >= inSize)
			{
berr << "MixerOutput::Mix(): popping empty buffer for input " << input.ptr() << endl;
				input->PopBuffer();
				input->SetBufferOffset(0);
				continue;
			}
			size_t inFrames = (inSize - inOffset) / inFrameSize;
//berr << "MIX: inFrames " << inFrames << ", toWrite " << toWrite << endl;
			size_t inPrevious = inFrames;
			size_t outPrevious = toWrite;
			input->GetGain(gains);
			input->Resampler()->Resample(
				(int8*)inBuffer->Data() + inOffset,
				&inFrames,
				(int8*)mixBuffer + outOffset,
				&toWrite,
				gains,
				true);
			outOffset += ((outPrevious - toWrite) * mMixFrameSize);
			if (!inFrames)
			{
				// done with this buffer
				input->PopBuffer();
				inOffset = 0;
			}
			else
			{
				ASSERT(!toWrite);
				inOffset += (inPrevious - inFrames) * inFrameSize;
			}
//berr << "     inOffset now " << inOffset << endl;
			input->SetBufferOffset(inOffset);
		}
	}
}

status_t 
MixerOutput::LogThreadEntry(void *cookie)
{
	((MixerOutput*)cookie)->LogThread();
	return B_OK;
}

void 
MixerOutput::LogThread()
{
	status_t err;
	size_t page = 0;
	while ((err = acquire_sem(mLogSem)) == B_OK)
	{
		mLogTrack->WriteChunk(mLogBuffer + (page * mLogPageSize), mLogPageSize);
		if (++page == mLogPageCount) page = 0;
	}
}

} } // B::Private
