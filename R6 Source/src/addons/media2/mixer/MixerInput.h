#ifndef _MIXER2_INPUT_H_
#define _MIXER2_INPUT_H_

#include <media2/MediaControllable.h>
#include <media2/MediaEndpoint.h>

namespace B {
namespace Media2 {
	class BAudioResampler;
}
namespace Private {

using namespace Media2;

class IBufferOutlet;
class MixerOutput;
class MixerCoreNode;

class MixerInput : public BMediaInput, public BMediaControllable
{
public:
	B_STANDARD_ATOM_TYPEDEFS(MixerInput);
	static	const BValue key_gain;

									MixerInput();
	virtual							~MixerInput();

	static	BMediaConstraint		ConstraintFor(const media_multi_audio_format & mixFormat);

	virtual	BValue					Inspect(const BValue &);

	virtual	status_t				Acquired(const void * id);
	virtual	status_t				Released(const void * id);

	// +++ HERE pick a resampler
	virtual	status_t				SetOutput(const atom_ptr<MixerOutput> & output);

	virtual	status_t				AcquireBuffer(BBuffer * outBuffer);

	// queue buffer if connected to an output
	virtual	status_t				HandleBuffer(BBuffer * buffer);

	virtual	status_t				AcceptConnect(
										IMediaEndpoint::arg sourceEndpoint,
										IMediaOutput::arg sourceOutput,
										const atom_ptr<IBufferOutlet> & transport,
										const BMediaConstraint & constraint,
										const BMediaPreference & sourcePreference,
										IMediaEndpoint::ptr * outConnectedEndpoint,
										IMediaInput::ptr * outConnectedInput,
										BMediaFormat * outFormat);

	virtual	status_t				AcceptDisconnect();	

	// ** IMediaControllable
	
	virtual	status_t				SetControl(const BValue & key, const BValue & value);

	// MixerOutput API
			BAudioResampler *		Resampler() const;
			size_t					FrameSize() const;
			float					FrameRate() const;
			size_t					FramesQueued() const;
			void					GetGain(float * outGain) const;
			
			const BBuffer *			TopBuffer() const;
			size_t					BufferOffset() const;
			void					SetBufferOffset(size_t offset);
			status_t				PopBuffer();
			void					FlushBuffers();

private:
	mutable	BLocker					mLock;
	
	// output bus for this channel
			atom_ptr<MixerOutput>	mOutput;
			
	// remote connection
			IMediaOutput::ptr		mConnectedOutput;

			bool					mNeedSwap;
			type_code				mSwapType;
			size_t					mInputFrameSize;
			size_t					mInputFrameCount;

	// resampler
			media_multi_audio_format	mMixFormat;
			media_multi_audio_format	mInputFormat;
			BAudioResampler *		mResampler;
			
			float					mGain;
	
	// queued buffers
			BBuffer *				mQueue;
			size_t					mQueueHead;
			size_t					mQueueTail;
			size_t					mQueueSize;
			
			size_t					mBufferOffset;
			size_t					mFramesQueued;
};

} } // B::Private
#endif //_MIXER2_INPUT_H_
