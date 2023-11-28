#ifndef _MIXER2_MIXERCORENODE_H_
#define _MIXER2_MIXERCORENODE_H_

#include <media2/MediaNode.h>
#include <media2/MediaEndpoint.h>

#include <support2/Locker.h>
#include <support2/Vector.h>

#include "MixerInput.h"
#include "MixerOutput.h"

namespace B {
namespace Private {

using namespace Support2;

class MixerCoreNode : public BMediaNode
{
public:
	B_STANDARD_ATOM_TYPEDEFS(MixerCoreNode);

									MixerCoreNode();
	virtual							~MixerCoreNode();

	virtual	status_t				Acquired(const void * id);
	virtual	status_t				Released(const void * id);

	// the free-connection input: connections may only be made to this input.
	// a new MixerInput is substituted when the connection is reserved, leaving
	// this input continuously free for new connections.	
			BMediaInput::ptr		DefaultInput() const;

	// output bus access
	virtual	status_t				AddMixerOutput(MixerOutput::arg output);
	virtual	status_t				RemoveMixerOutput(MixerOutput::arg output);
			size_t					CountMixerOutputs() const;
			MixerOutput::ptr		MixerOutputAt(size_t index) const;

			void					GetMixFormat(media_multi_audio_format * outFormat);

	// BMediaNode implementation

	// if the requested input is the default (free-connection) input then
	// create a new MixInput and defer connection to it.  otherwise return
	// an error.
	virtual	status_t				ReserveInputConnection(
										IMediaOutput::arg remoteOutput,
										BMediaInput::ptr * ioInput,
										BMediaConstraint * constraint,
										media_endpoint_state requestedState);

	// if the input was a MixInput, remove it.
	virtual	void					AbortInputConnection(
										IMediaOutput::arg remoteOutput,
										BMediaInput::arg input,
										media_endpoint_state deniedState);

	// if a MixInput was disconnected, remove it.
	virtual	void					Disconnected(
										BMediaEndpoint::arg localEndpoint,
										IMediaEndpoint::arg remoteEndpoint);

private:
	mutable	BLocker						mLock;

			media_multi_audio_format	mMixFormat;

			BMediaInput::ptr			mDefaultInput;
			MixerOutput::ptr			mDefaultOutput;
			BVector<MixerOutput::ptr>	mOutputs;
};

} } // B::Private
#endif //_MIXER2_MIXERCORENODE_H_
