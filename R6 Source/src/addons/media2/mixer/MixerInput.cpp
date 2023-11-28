#include <media2/AudioResampler.h>
#include "MixerInput.h"
#include "MixerOutput.h"

#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#include <cstring>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Private {

const BValue MixerInput::key_gain("gain");

MixerInput::MixerInput() :
	BMediaInput("mix.in"),
	mLock("MixerInput::mLock")
{
}


MixerInput::~MixerInput()
{
}

BMediaConstraint 
MixerInput::ConstraintFor(const media_multi_audio_format &)
{
	return BMediaConstraint().
		And(B_FORMATKEY_RAW_AUDIO_TYPE,
			BValue().
			Overlay(BValue::Int32(B_AUDIO_FLOAT)).
			Overlay(BValue::Int32(B_AUDIO_INT32)).
			Overlay(BValue::Int32(B_AUDIO_INT16)).
			Overlay(BValue::Int32(B_AUDIO_UINT8))).
		And(B_FORMATKEY_CHANNEL_COUNT,
			BValue().
			Overlay(BValue::Int32(1)).
			Overlay(BValue::Int32(2))).
		And(B_FORMATKEY_MEDIA_TYPE, BValue::Int32(B_MEDIA_RAW_AUDIO)).
		And(B_FORMATKEY_BUFFER_FRAMES, BMediaConstraintItem::B_GE, BValue::Int32(1));
}

BValue 
MixerInput::Inspect(const BValue & v)
{
	return BMediaInput::Inspect(v) + BMediaControllable::Inspect(v);
}

status_t 
MixerInput::Acquired(const void *id)
{
	BMediaInput::Acquired(id);
	
	mResampler = 0;

	mQueueSize = 3;
	mQueue = new BBuffer[mQueueSize];
	mQueueHead = 0;
	mQueueTail = 0;
	mBufferOffset = 0;
	mFramesQueued = 0;
	mGain = 1.0;
	
	SetConstraint(ConstraintFor(mMixFormat));
	
	AddControl(key_gain, BValue::Double(mGain));
	
	return B_OK;
}

status_t 
MixerInput::Released(const void *id)
{
berr << "MixerInput::Released()" << endl;
	delete mResampler;
	mResampler = 0;
	mOutput = 0;
	mConnectedOutput = 0;
	delete [] mQueue;
	mQueue = 0;
//	FlushBuffers();
	return BMediaInput::Released(id);
}

status_t 
MixerInput::SetOutput(const atom_ptr<MixerOutput> &output)
{
	status_t err;
	BAutolock _l(mLock.Lock());
	if (output == mOutput) return B_OK;
	if (mOutput != 0)
	{
		delete mResampler;
		mResampler = 0;
		err = mOutput->DetachInput(this);
		if (err < B_OK)
		{
berr << "MixerInput::SetOutput(): mOutput->DetachInput: " << strerror(err) << endl;
			return err;
		}
	}
	mOutput = output;
	if (mOutput != 0)
	{
		err = mOutput->GetMixFormat(&mMixFormat);
		if (err < B_OK)
		{
berr << "MixerInput::SetOutput(): mOutput->GetMixFormat: " << strerror(err) << endl;
			return err;
		}
		if (mConnectedOutput != 0)
		{
			err = BAudioResampler::Instantiate(
				BAudioResampler::B_LINEAR,
				mInputFormat,
				mMixFormat,
				&mResampler);
			if (err < B_OK)
			{
berr << "MixerInput::SetOutput(): BAudioResampler::Instantiate(): " << strerror(err) << endl;
				return err;
			}
		}

		mLock.Unlock();
		err = mOutput->AttachInput(this);
		mLock.Lock();
		if (err < B_OK)
		{
berr << "MixerInput::SetOutput(): mOutput->AttachInput: " << strerror(err) << endl;
			delete mResampler;
			mResampler = 0;
			return err;
		}
	}
	return B_OK;
}

status_t 
MixerInput::AcquireBuffer(BBuffer *)
{
	return B_ERROR;
}

status_t 
MixerInput::HandleBuffer(BBuffer *buffer)
{
	if (buffer->Size() != mInputFrameSize * mInputFrameCount) return B_BAD_VALUE;
	MixerOutput::ptr output;
	{
		BAutolock _l(mLock.Lock());
//berr << "MixerInput::HandleBuffer()  IN: head " << mQueueHead << ", tail " << mQueueTail <<
//		", size " << mQueueSize << endl;
	
		if (mOutput == 0)
		{
			buffer->ReleaseBuffer();
			return B_OK;
		}
		output = mOutput;
	
		if (mNeedSwap)
		{
			swap_data(
				mSwapType, buffer->Data(), mInputFrameSize * mInputFrameCount,
				B_SWAP_ALWAYS);
		}
		
		size_t headPrev = mQueueHead ? mQueueHead-1 : mQueueSize-1;
		if (mQueueTail == headPrev)
		{
			// queue full; compact/expand vector
			size_t newSize = mQueueSize << 1;
			BBuffer * newQueue = new BBuffer[newSize];
			size_t n = 0;
			for (size_t h = mQueueHead;
				 h != mQueueTail;
				 n++, h = (h+1 == mQueueSize) ? 0 : h+1)
			{
				newQueue[n] = mQueue[h];
			}
			for (; n < newSize; n++)
			{
				newQueue[n] = BBuffer();
			}
			
			mQueueTail = (mQueueTail > mQueueHead) ?
				mQueueTail - mQueueHead :
				mQueueTail + mQueueSize - mQueueHead;			
			mQueueHead = 0;
			mQueueSize = newSize;
			delete [] mQueue;
			mQueue = newQueue;
		}
		mQueue[mQueueTail++] = *buffer;
		if (mQueueTail == mQueueSize) mQueueTail = 0;
		
		mFramesQueued += mInputFrameCount;
		
//berr << "MixerInput::HandleBuffer() OUT: head " << mQueueHead << ", tail " << mQueueTail << endl;
	}
	if (output != 0) output->BufferQueued(this);
	return B_OK;
}

status_t 
MixerInput::AcceptConnect(
	IMediaEndpoint::arg sourceEndpoint,
	IMediaOutput::arg sourceOutput,
	const atom_ptr<IBufferOutlet> & transport,
	const BMediaConstraint & constraint,
	const BMediaPreference & sourcePreference,
	IMediaEndpoint::ptr * outConnectedEndpoint,
	IMediaInput::ptr * outConnectedInput,
	BMediaFormat * outFormat)
{
	status_t err = BMediaInput::AcceptConnect(
		sourceEndpoint, sourceOutput, transport, constraint, sourcePreference,
		outConnectedEndpoint, outConnectedInput, outFormat);
	if (err < B_OK) return err;
	
	const BMediaFormat & format = *outFormat;
	
	// enable byteswapping if necessary
	mNeedSwap = (format[B_FORMATKEY_BYTE_ORDER].AsInteger() != B_MEDIA_HOST_ENDIAN);
	if (mNeedSwap) switch (format[B_FORMATKEY_MEDIA_TYPE].AsInteger())
	{
		case B_AUDIO_INT16:
			mSwapType = B_INT16_TYPE;
			break;
		case B_AUDIO_FLOAT:
			mSwapType = B_FLOAT_TYPE;
			break;
		case B_AUDIO_INT32:
			mSwapType = B_INT32_TYPE;
			break;
		default:
			mNeedSwap = false;
			break;
	}
	
	const raw_audio_type audioType = (raw_audio_type)format[B_FORMATKEY_RAW_AUDIO_TYPE].AsInteger();
	const size_t channelCount = format[B_FORMATKEY_CHANNEL_COUNT].AsInteger();
	mInputFrameSize = (audioType & B_AUDIO_SIZE_MASK) * channelCount;
	mInputFrameCount = format[B_FORMATKEY_BUFFER_FRAMES].AsInteger();

	media_format f;
	format.as_media_format(&f);
	mInputFormat = f.u.raw_audio;
	
	mConnectedOutput = sourceOutput;
	if (mOutput != 0)
	{
		ASSERT(!mResampler);
		err = BAudioResampler::Instantiate(
			BAudioResampler::B_LINEAR,
			mInputFormat,
			mMixFormat,
			&mResampler);
		if (err < B_OK)
		{
berr << "BAudioResampler::Instantiate(): " << strerror(err) << endl;
		}
	}
	return err;
}

status_t 
MixerInput::AcceptDisconnect()
{
checkpoint
	status_t err = BMediaInput::AcceptDisconnect();
	if (err < B_OK) return err;
	FlushBuffers();
	delete mResampler;
	mResampler = 0;
	mConnectedOutput = 0;
	return B_OK;
}

status_t 
MixerInput::SetControl(const BValue &key, const BValue &value)
{
	status_t err = BMediaControllable::SetControl(key, value);
	if (err < B_OK) return err;
	
	BAutolock _l(mLock.Lock());
	if (key == key_gain)
	{
		mGain = (float)value.AsDouble();
	}
	
	return B_OK;
}



BAudioResampler *
MixerInput::Resampler() const
{
	BAutolock _l(mLock.Lock());
	return mResampler;
}

size_t 
MixerInput::FrameSize() const
{
	return mInputFrameSize;
}

float 
MixerInput::FrameRate() const
{
	return mInputFormat.frame_rate;
}

size_t 
MixerInput::FramesQueued() const
{
	return mFramesQueued;
}

void 
MixerInput::GetGain(float *outGain) const
{
	for (size_t n = 0; n < mInputFormat.channel_count; n++) outGain[n] = mGain;
}


const BBuffer *
MixerInput::TopBuffer() const
{
	BAutolock _l(mLock.Lock());
	return (mQueueHead == mQueueTail) ? 0 : &mQueue[mQueueHead];
}

size_t 
MixerInput::BufferOffset() const
{
	return mBufferOffset;
}

void 
MixerInput::SetBufferOffset(size_t offset)
{
	mBufferOffset = offset;
}


status_t 
MixerInput::PopBuffer()
{
	BAutolock _l(mLock.Lock());	
	if (mQueueHead == mQueueTail) return B_NOT_ALLOWED;
	mQueue[mQueueHead].ReleaseBuffer();
	if (++mQueueHead == mQueueSize) mQueueHead = 0;
	mFramesQueued -= mInputFrameCount;
	return B_OK;
}

void 
MixerInput::FlushBuffers()
{
berr << "MixerInput::FlushBuffers()" << endl;
checkpoint
	BAutolock _l(mLock.Lock());
checkpoint
	while (mQueueHead != mQueueTail)
	{
checkpoint
		mQueue[mQueueHead].ReleaseBuffer();
		if (++mQueueHead == mQueueSize) mQueueHead = 0;
	}
	mFramesQueued = 0;
checkpoint
}

} } // B::Private
