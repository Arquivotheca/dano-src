#ifndef __MEDIA_NODE_CONTROLLER__
#define __MEDIA_NODE_CONTROLLER__

#include <String.h>
#include "MediaController.h"
#include "ObjectList.h"

class NodeWrapper;
class BParameter;
class BTimeSource;
struct dormant_node_info;
class VideoConsumerNode;


class MediaNodeController : public MediaController {
	// MediaNodeController
	//			- opens a file and sets it up for playback
	//			- holds/controls the play state of the file
	//
	//			- sends back messages to target
public:
	static MediaController *Open(const entry_ref *, BLooper *owner,
		status_t *result = 0, bool debug = false);

	virtual void Delete(BLooper *owner);

	virtual void MessageReceived(BMessage*);
	
	virtual status_t ConnectVideoOutput(VideoView *);
	
	
	virtual bigtime_t Position() const;
	
	virtual void SetInPoint(bigtime_t, bool tracking = false);
	virtual void SetOutPoint(bigtime_t, bool tracking = false);

	virtual void Play();
	virtual void Stop();
	virtual void Pause();

	virtual void BumpInPointAndRewind();
	virtual void BumpOutPointAndGoToEnd();

	virtual float Volume();
	virtual void SetVolume(float);

protected:
	MediaNodeController(const entry_ref *ref, bool debugOutput = false);
	~MediaNodeController();
	virtual void Quit();

	static bool CanHandle(const entry_ref *);

	virtual status_t SetTo(const entry_ref *);

	virtual void PauseAfterThumbNudge();
		// if the position thumb gets hit by a tracked in/out point thumb during
		// in/out point tracking, we pause for a bit
	void SetOutPosWakeup();
		// schedule a wakeup to handle hitting the end point

	enum ScheduledCommand {
		kIdle,
		kQuit,
		kScheduledToPlay,
		kScheduledToCheckOutPoint,
		kScheduledToScrub
	};

	void ScheduleCommand(ScheduledCommand what, bigtime_t when);
		// schedules the run loop to wake up to service an event like
		// out-point hit
	void CheckTimeUp(float optionalScrubTargetPosition);
		// call to see if it is time to execute scheduled command

	virtual float RawVolume() const;
	virtual void SetRawVolume(float);

	virtual status_t ReadyToLoop(loop_state* outState);
	bool ExecuteLoop(loop_state* outState);
	
private:

	status_t HookUpMixer(NodeWrapper *node);

	void _SetTo(const entry_ref *ref, const dormant_node_info *);
	void _SetPosition(bigtime_t pos, bigtime_t when);
	void _ScrubTo(bigtime_t pos);
	void _SetScrubbing(bool);
	void _SetInPoint(bigtime_t, bool tracking = false);
	void _SetOutPoint(bigtime_t, bool tracking = false);
	void _Stop();
	void _Play();
	void _Pause();

	void _BumpInPointAndRewind();
	void _BumpOutPointAndGoToEnd();

	void PausedSetPositionScrubToCommon(bigtime_t pos, bigtime_t now);
	void ScrubIfNeeded(float);

	NodeWrapper	*fCurrentSourceNode;
	NodeWrapper	*fFileInterface;
	BObjectList<NodeWrapper> fNodeChain;
	BTimeSource *fTimeSource;
	VideoConsumerNode *fVideoConsumerNode;

	bigtime_t fRelativeStart;
	bigtime_t fCurrentPosition;

	bigtime_t wakeUpTime;
	ScheduledCommand scheduledCommand;
	
	BParameterWeb *fMixerWeb;
	BContinuousParameter *fVolume;

	// Looper state 
	bool fLooping; 
	BMessenger fLocalScrubTarget;
	bool fLocalScrubbing;
	bigtime_t fLocalLength;
	
	friend class CheckInOutPointsThread;
};

#endif

