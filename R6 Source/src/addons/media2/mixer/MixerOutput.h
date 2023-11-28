#ifndef _MIXER2_OUTPUT_H_
#define _MIXER2_OUTPUT_H_

#include <media2/MediaEndpoint.h>
#include <media2/MediaFormats.h>
#include <support2/Vector.h>
#include <storage2/Entry.h>

namespace B {
namespace Media2 {
	class BMediaFile;
	class BMediaTrack;
}
namespace Private {

using namespace Storage2;
using namespace Media2;

class MixerInput;

class MixerOutput : public BMediaOutput
{
public:
	B_STANDARD_ATOM_TYPEDEFS(MixerOutput)

									MixerOutput();
	virtual							~MixerOutput();

	virtual	status_t				Acquired(const void * id);
	virtual	status_t				Released(const void * id);

	// perform connect, kick off mix thread
	virtual	status_t				Connect(
										IMediaInput::ptr * ioInput,
										BMediaFormat * outFormat,
										const BMediaConstraint & constraint);

	// shut down mix thread, clean up, perform disconnect
	virtual	status_t				Disconnect();
	
	// reset newly-connected input's state
			void					ResetInput(const atom_ptr<MixerInput> & input);

			status_t				GetMixFormat(media_multi_audio_format * outFormat);


	// echo output to a file
			status_t				StartLogging(
										const entry_ref & ref,
										const media_file_format & fileFormat);
			status_t				StopLogging();

protected:
	virtual	void					AttachedToNode(const atom_ptr<BMediaNode> & node);
	virtual	void					DetachedFromNode(const atom_ptr<BMediaNode> & node);

private:
			status_t				_Connected(
										IMediaInput::arg connectedInput,
										const BMediaFormat & format);
			void					_Disconnected();

	friend	class MixerInput;
	virtual	status_t				AttachInput(const atom_ptr<MixerInput> & input);
	virtual	status_t				DetachInput(const atom_ptr<MixerInput> & input);
			void					BufferQueued(const atom_ptr<MixerInput> & input);

	static	status_t				MixThreadEntry(void * cookie);
			void					MixThread();

			status_t				PushBuffer(bool & outKeepPushing);
			void					Mix(void * mixBuffer,
										const BVector<atom_ptr<MixerInput> > & inputs);
	
	static	status_t				LogThreadEntry(void * cookie);
			void					LogThread();

	mutable	BLocker						mLock;
	
	// mapped inputs
			BVector<atom_ptr<MixerInput> >	mInputs;
	
	// connection to the outside world
			IMediaInput::ptr			mConnectedInput;
	
	// mix format
			raw_audio_type				mMixType;
			size_t						mMixChannels;
			size_t						mMixFrameSize;

	// mix buffer [only necessary if the mix format != the output format]
			int8 *						mMixBuffer;
	
	// output format
			raw_audio_type				mOutputType;
			size_t						mOutputChannels;
			size_t						mOutputFrameSize;
			size_t						mOutputFrameCount;
			float						mOutputFrameRate;
	
	// mix/buffer-production thread
	enum	state_t {
		RUNNING		= 0x1,
		STOPPING	= 0x2
	};
			thread_id					mThread;
			sem_id						mSem;
			int32						mEmptyCount;
			int32						mState;
			
			uint64						mFramesProduced;
			
			thread_id					mLogThread;
			sem_id						mLogSem;
			int8 *						mLogBuffer;
			size_t						mLogInPage;
			size_t						mLogPageSize;
			size_t						mLogPageCount;
			BMediaFile *				mLogFile;
			BMediaTrack *				mLogTrack;
};

} } // B::Private
#endif //_MIXER2_OUTPUT_H_
