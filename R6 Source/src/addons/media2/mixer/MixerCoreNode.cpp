#include "MixerCoreNode.h"

#include "MixerInput.h"
#include "MixerOutput.h"

#include <support2/Autolock.h>
#include <support2/StdIO.h>

#include <cstring>

#define checkpoint \
//berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

namespace B {
namespace Private {

MixerCoreNode::MixerCoreNode() :
	BMediaNode("audio.mixer"),
	mLock("MixerCoreNode::mLock")
{
}


MixerCoreNode::~MixerCoreNode()
{
}

status_t 
MixerCoreNode::Acquired(const void *id)
{
	BMediaNode::Acquired(id);

	mMixFormat = media_multi_audio_format::wildcard;
	mMixFormat.format = B_AUDIO_FLOAT;
	mMixFormat.channel_count = 2;
	mMixFormat.byte_order = B_MEDIA_HOST_ENDIAN;
	
	mDefaultInput = new BMediaInput("free.in");
	berr << "MixerCoreNode: mDefaultInput " << mDefaultInput.ptr() << endl;
	mDefaultInput->SetConstraint(MixerInput::ConstraintFor(mMixFormat));
	AddEndpoint(mDefaultInput);

	return B_OK;
}

status_t 
MixerCoreNode::Released(const void *id)
{
	mDefaultInput = 0;
	mDefaultOutput = 0;
	mOutputs.MakeEmpty();
	return BMediaNode::Released(id);
}

BMediaInput::ptr 
MixerCoreNode::DefaultInput() const
{
	berr << "MixerCoreNode: DefaultInput() returning " << mDefaultInput.ptr() << " (I) " <<
		(IMediaInput*)mDefaultInput.ptr() << endl;
	return mDefaultInput;
}

status_t 
MixerCoreNode::AddMixerOutput(MixerOutput::arg output)
{
	if (output == 0) return B_BAD_VALUE;
	BAutolock _l(mLock.Lock());
	AddEndpoint(output);
	mOutputs.AddItem(output);
	if (mDefaultOutput == 0) mDefaultOutput = output;
	return B_OK;
}

status_t 
MixerCoreNode::RemoveMixerOutput(MixerOutput::arg output)
{
	if (output == 0) return B_BAD_VALUE;
	BAutolock _l(mLock.Lock());
	RemoveEndpoint(output);
	for (int32 n = mOutputs.CountItems()-1; n >= 0; n--)
	{
		if (mOutputs[n] == output)
		{
			// +++ detach (reassign?) all inputs
			bool was_default = (mDefaultOutput == mOutputs[n]);
			mOutputs.RemoveItemsAt(n,1);
			if (was_default)
			{
				mDefaultOutput = (mOutputs.CountItems() > 0) ? mOutputs[0] : 0;
			}
			return B_OK;
		}
	}
	return B_BAD_VALUE;
}

size_t 
MixerCoreNode::CountMixerOutputs() const
{
	BAutolock _l(mLock.Lock());
	return mOutputs.CountItems();
}

MixerOutput::ptr 
MixerCoreNode::MixerOutputAt(size_t index) const
{
	BAutolock _l(mLock.Lock());
	return (index < mOutputs.CountItems()) ? mOutputs[index] : 0;
}

void 
MixerCoreNode::GetMixFormat(media_multi_audio_format *outFormat)
{
	*outFormat = mMixFormat;
}

status_t 
MixerCoreNode::ReserveInputConnection(
	IMediaOutput::arg, BMediaInput::ptr * ioInput,
	BMediaConstraint *, media_endpoint_state)
{
checkpoint
berr << "MixerCoreNode::ReserveInputConnection(): got input " << (IMediaInput*)ioInput->ptr() <<
	", defaultinput " << (IMediaInput*)mDefaultInput.ptr() << " aka " << (BMediaInput*)mDefaultInput.ptr() << endl;

	if (ioInput->ptr() == mDefaultInput.ptr())
	{
checkpoint
		MixerInput::ptr newInput = new MixerInput();
		newInput->SetOutput(mDefaultOutput);
		status_t err = AddEndpoint(newInput);
		if (err < B_OK) return err;
		*ioInput = newInput;
	}
	return B_OK;
}

void 
MixerCoreNode::AbortInputConnection(
	IMediaOutput::arg, BMediaInput::arg input, media_endpoint_state)
{
	MixerInput::ptr mixerInput = dynamic_cast<MixerInput*>(input.ptr());
	if (mixerInput != 0)
	{
		status_t err = RemoveEndpoint(input);
		if (err < B_OK)
		{
berr << "MixerCoreNode::AbortInputConnection(): RemoveEndpoint(): " << strerror(err) << endl;
		}
	}
}

void 
MixerCoreNode::Disconnected(
	BMediaEndpoint::arg localEndpoint, IMediaEndpoint::arg)
{
checkpoint
	MixerInput::ptr mixerInput = dynamic_cast<MixerInput*>(localEndpoint.ptr());
	if (mixerInput != 0)
	{
		status_t err = RemoveEndpoint(mixerInput);
		if (err < B_OK)
		{
berr << "MixerCoreNode::Disconnected(): RemoveEndpoint(): " << strerror(err) << endl;
		}
	}
}

} } // B::Private
