
#if !defined(WriterNode_h)
#define WriterNode_h

#include <MediaEventLooper.h>
#include <BufferConsumer.h>
#include <FileInterface.h>
#include <MediaAddOn.h>

class WriterAddOn;

class WriterNode : public BMediaEventLooper, public BBufferConsumer, public BFileInterface
	//	, public BControllable
{
public:
		WriterNode(WriterAddOn * addon, const flavor_info * info, const char * name, status_t *out_error);
		virtual ~WriterNode();

protected:	//	MediaNode

		virtual BMediaAddOn * AddOn(int32 * internal_id) const;
		virtual void NodeRegistered();
		virtual status_t HandleMessage(int32 message, const void * data, size_t size);
		virtual	void Preroll();

protected:	//	MediaEventLooper

		virtual	void HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent = false);

protected:	//	BufferConsumer

		virtual	status_t AcceptFormat(const media_destination & dest, media_format * format);
		virtual	status_t GetNextInput(int32 * cookie, media_input * out_input);
		virtual	void DisposeInputCookie(int32 cookie);
		virtual	void BufferReceived(BBuffer * buffer);
		virtual	void ProducerDataStatus(const media_destination & for_whom, int32 status, bigtime_t at_performance_time);
		virtual	status_t GetLatencyFor(const media_destination & for_whom, bigtime_t * out_latency, media_node_id * out_timesource);
		virtual	status_t Connected(const media_source & producer, const media_destination & where, const media_format & with_format, media_input * out_input);
		virtual	void Disconnected(const media_source & producer, const media_destination & where);
		virtual	status_t FormatChanged(const media_source & producer, const media_destination & consumer,  int32 change_tag, const media_format & format);
		virtual	status_t SeekTagRequested(const media_destination & destination, bigtime_t in_target_time, uint32 in_flags,  media_seek_tag * out_seek_tag, bigtime_t * out_tagged_time, uint32 * out_flags);

protected:	//	FileInterface

		virtual	status_t GetNextFileFormat(int32 * cookie, media_file_format * out_format);
		virtual	void DisposeFileFormatCookie(int32 cookie);
		virtual	status_t GetDuration(bigtime_t * out_time);
		virtual	status_t SniffRef(const entry_ref & file, char * out_mime_type, float * out_quality);
		virtual	status_t SetRef(const entry_ref & file, bool create, bigtime_t * out_time);
		virtual	status_t GetRef(entry_ref * out_ref, char * out_mime_type);

protected:

		enum { MAX_TRACKS = 2 };

		BEntry m_entry;
		BMediaFile * m_outputFile;
		const media_file_format * m_fileFormat;
		BMediaTrack * m_tracks[MAX_TRACKS];
		media_input m_inputs[MAX_TRACKS];
		WriterAddOn * m_addon;
		const flavor_info * m_flavor;
		int32 m_producerStatus[MAX_TRACKS];
		bool m_needHeaderCommit;
		bool m_headerCommitted;
		bool m_encoderChosen[MAX_TRACKS];
		media_codec_info m_encoder[MAX_TRACKS];
		bigtime_t m_latency;

		void recalc_time();
};


#endif	//	WriterNode_h
