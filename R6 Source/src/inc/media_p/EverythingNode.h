/* EverythingNode.h */
/* basic header file with declarations of all the functions */
/* for the various subclasses of BMediaNode */

#include <MediaNode.h>
#include <MediaEventLooper.h>
#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <FileInterface.h>
#include <TimeSource.h>


class EverythingNode :
	public BMediaEventLooper,
	public BBufferConsumer,
	public BBufferProducer,
	public BControllable,
	public BFileInterface,
	public BTimeSource
{
	/* BMediaNode */
	public:
		virtual BMediaAddOn *		AddOn(int32 *internal_id) const;
	protected:
		virtual	void				Start(bigtime_t performance_time);
		virtual void				Stop(bigtime_t performance_time, bool immediate);
		virtual void				Seek(bigtime_t media_time, bigtime_t performance_time);
		virtual void				SetRunMode(run_mode mode);
		virtual void				TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);
		virtual void				Preroll();
		virtual void				SetTimeSource(BTimeSource *time_source);
		virtual status_t			HandleMessage(int32 code, const void *msg, size_t size);
		virtual	status_t 			RequestCompleted(const media_request_info & info);
		virtual status_t			DeleteHook(BMediaNode *node);
		virtual void				NodeRegistered();
		virtual	status_t 			GetNodeAttributes(media_node_attribute * outAttributes,
										size_t inMaxCount);
		virtual status_t			AddTimer(bigtime_t at_performance_time, int32 cookie);

	/* BMediaEventLooper */
	protected:
		virtual void				HandleEvent(const media_timed_event *event, bigtime_t lateness,
											bool realTimeEvent = false);
		virtual void				CleanUpEvent(const media_timed_event *event);
		virtual	bigtime_t			OfflineTime();
		virtual void				ControlLoop();

	/* BBufferConsumer */
	protected:
		virtual status_t			AcceptFormat(const media_destination &dest, media_format *format);
		virtual status_t			GetNextInput(int32 *cookie, media_input *out_input);
		virtual void				DisposeInputCookie(int32 cookie);
		virtual void				BufferReceived(BBuffer *buffer);
		virtual void				ProducerDataStatus(const media_destination &for_whom, int32 status,
										bigtime_t at_media_time);
		virtual status_t			GetLatencyFor(const media_destination &for_whom, bigtime_t *out_latency,
										media_node_id *out_timesource);
		virtual status_t			Connected(const media_source & producer, const media_destination &where,
										const media_format &with_format, media_input *out_input);
		virtual void				Disconnected(const media_source &producer, const media_destination &where);
		virtual status_t			FormatChanged(const media_source &producer, const media_destination &consumer,
										int32 from_change_format, const media_format &format);
		virtual	status_t 			SeekTagRequested(const media_destination & destination, bigtime_t in_target_time,
										uint32 in_flags, media_seek_tag * out_seek_tag, bigtime_t * out_tagged_time,
										uint32 * out_flags);

	/* BBufferProducer */
	protected:
		virtual status_t			FormatSuggestionRequested(media_type type, int32 quality, media_format *format);
		virtual status_t			FormatProposal(const media_source &output, media_format *format);
		virtual status_t			FormatChangeRequested(const media_source &source, const media_destination &dest,
										media_format *io_format, int32 *out_change_count);
		virtual status_t			GetNextOutput(int32 *cookie, media_output *out_output);
		virtual status_t			DisposeOutputCookie(int32 cookie);
		virtual status_t			SetBufferGroup(const media_source &for_source, BBufferGroup *group);
		virtual status_t			VideoClippingChanged(const media_source &for_source, int16 num_shorts,
										int16 *clip_data, const media_video_display_info &display,
										int32 * out_from_change_count);
		virtual status_t			GetLatency(bigtime_t *out_latency);
		virtual status_t			PrepareToConnect(const media_source &what, const media_destination &where,
										media_format *format, media_source *out_source, char *out_name);
		virtual void				Connect(status_t error, const media_source &source,
										const media_destination &dest, const media_format &format,
										char * io_name);
		virtual void				Disconnect(const media_source &what, const media_destination &where);
		virtual void				LateNoticeReceived(const media_source &what, bigtime_t how_much,
										bigtime_t performance_time);
		virtual void				EnableOutput(const media_source &what, bool enabled, int32 *change_tag);
		virtual status_t			SetPlayRate(int32 numer, int32 denom);
		virtual	void 				AdditionalBufferRequested(const media_source & source, media_buffer_id prev_buffer,
										bigtime_t prev_time, const media_seek_tag * prev_tag);
		virtual	void 				LatencyChanged(const media_source & source, const media_destination & destination,
										bigtime_t new_latency, uint32 flags);
										
	/* BControllable */
	protected:
		virtual	status_t 			GetParameterValue(int32 id, bigtime_t * last_change, void * value, size_t * ioSize);
		virtual	void 				SetParameterValue(int32 id, bigtime_t when, const void * value, size_t size);
		virtual	status_t 			StartControlPanel(BMessenger * out_messenger);

	/* BFileInterface */
	protected:
		virtual	status_t 			GetNextFileFormat(int32 * cookie, media_file_format * out_format);
		virtual	void 				DisposeFileFormatCookie(int32 cookie);
		virtual	status_t 			GetDuration(bigtime_t * out_time);
		virtual	status_t 			SniffRef(const entry_ref & file, char * out_mime_type,float * out_quality);
		virtual	status_t 			SetRef(const entry_ref & file, bool create, bigtime_t * out_time);
		virtual	status_t 			GetRef(entry_ref * out_ref, char * out_mime_type);

	/* BTimeSource */
	public:
		virtual	status_t 			SnoozeUntil(bigtime_t performance_time, bigtime_t with_latency = 0,
										bool retry_signals = false);		
	protected:
		virtual	status_t 			TimeSourceOp(const time_source_op_info & op, void * _reserved);
		
	private:
		virtual	status_t 			RemoveMe(BMediaNode * node);
		virtual	status_t 			AddMe(BMediaNode * node);
	
};
