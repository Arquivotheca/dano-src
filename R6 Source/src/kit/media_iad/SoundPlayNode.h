
#if !defined(_SOUND_PLAY_NODE_H)
#define _SOUND_PLAY_NODE_H

#include <BufferProducer.h>
#include <MediaEventLooper.h>

class BSoundPlayer;

class _SoundPlayNode : public BBufferProducer, public BMediaEventLooper {
public:
		_SoundPlayNode(
				const char * name,
				BSoundPlayer * player,
				const media_raw_audio_format * format);

		bigtime_t Now();

	/* MediaNode overrides */
virtual		BMediaAddOn* AddOn(
				int32 * internal_id) const;
protected:
virtual	void Preroll();
virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);
virtual void		NodeRegistered();

	/* MediaEventLooper overrides */
protected:
		virtual void		HandleEvent(	const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent);

	/* BufferProducer overrides */
virtual	status_t FormatSuggestionRequested(
				media_type type,
				int32 quality,
				media_format * format);
virtual	status_t FormatProposal(
				const media_source & output,
				media_format * format);
virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * out_change_count);
virtual	status_t GetNextOutput(	/* cookie starts as 0 */
				int32 * cookie,
				media_output * out_output);
virtual	status_t DisposeOutputCookie(
				int32 cookie);
virtual	status_t SetBufferGroup(
				const media_source & for_source,
				BBufferGroup * group);
virtual	status_t VideoClippingChanged(
				const media_source & for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * out_from_change_count);
virtual	status_t GetLatency(
				bigtime_t * out_lantency);
virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name);
virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * io_name);
virtual	void Disconnect(
				const media_source & what,
				const media_destination & where);
virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * change_tag);


private:

		int64 m_frames_played;
		BSoundPlayer * m_player;
		BBufferGroup * m_buffers;
		media_raw_audio_format m_raw_format;
		media_output m_output;
		bigtime_t m_downstream_latency;
		bigtime_t m_private_latency;
		BBuffer *m_prerolled;
		bigtime_t m_stop_time;
		bigtime_t m_seek_time;
		bigtime_t m_media_time;
		bigtime_t m_delta;
		bool m_haveData;
		bool m_connected;
		bool m_bufferReady;
		bool m_muted;

virtual		~_SoundPlayNode();

		BBuffer * ReadyBuffer(bigtime_t bufferTime);
		bigtime_t frames_duration(int64 played);
		size_t frame_size(const media_raw_audio_format & format);
		void alloc_buffers();
};

#endif /* _SOUND_PLAY_NODE_H */
