
#if !defined(_TRINITY_P_H)
#define _TRINITY_P_H

#include <Entry.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <MediaNode.h>
#include <Locker.h>
#include <Messenger.h>
//#if defined(__GNUC__) && defined(__INTEL__)
//#include <realtime_allocator.h>
//#endif

#include <rt_map.h>
#include <map>
#include <rt_alloc.h>
#include <list>


extern rtm_pool * _rtm_pool;
extern "C" void rtm_dump_block(rtm_pool * pool, void * block);

namespace BPrivate {
	extern bool media_debug;
}


class _BMediaRosterP;
class BLocker;

#define DEFAULT_TIMEOUT 6000000L

/* "magic" node ids, private */
#define DEFAULT_AUDIO_INPUT ((media_node_id)-1)
#define DEFAULT_AUDIO_OUTPUT ((media_node_id)-2)
#define DEFAULT_AUDIO_MIXER ((media_node_id)-3)
#define DEFAULT_VIDEO_INPUT ((media_node_id)-17)
#define DEFAULT_VIDEO_OUTPUT ((media_node_id)-18)

/* Add specially-registered types here */
#define B_MEDIA_INTELLIGENT_PARADIGM ((media_type)B_MEDIA_PRIVATE)

enum {
	MEDIA_REGISTER_APP = '_TR0',
	MEDIA_UNREGISTER_APP,
	MEDIA_REGISTER_NODE,
	MEDIA_UNREGISTER_NODE,
	MEDIA_REGISTER_BUFFER,
	MEDIA_SET_DEFAULT,
	MEDIA_ACQUIRE_NODE_REFERENCE,		// inc global ref count for node
	MEDIA_REQUEST_NOTIFICATIONS,
	MEDIA_CANCEL_NOTIFICATIONS,
	MEDIA_MAKE_CONNECTION,
	MEDIA_BREAK_CONNECTION = '_TRA',
	MEDIA_SET_OUTPUT_BUFFERS,
	MEDIA_RECLAIM_OUTPUT_BUFFERS,
	MEDIA_ORPHAN_RECLAIMABLE_BUFFERS,
	MEDIA_SET_TIMESOURCE,
	MEDIA_QUERY_NODES,
	MEDIA_QUERY_INPUTS,
	MEDIA_QUERY_OUTPUTS,
	MEDIA_QUERY_LATENTS,
	MEDIA_SNIFF_FILE,
	MEDIA_FORMAT_CHANGED,
	MEDIA_INSTANTIATE_PERSISTENT_NODE,
	MEDIA_GET_DEFAULT_INFO,
	MEDIA_SET_RUNNING_DEFAULT,
	MEDIA_RELEASE_NODE_REFERENCE,		/* O */	// dec global ref count for node
	MEDIA_BROADCAST_MESSAGE,
	MEDIA_TYPE_ITEM_OP,
	MEDIA_FORMAT_OP,
	MEDIA_ERROR_MESSAGE,
	MEDIA_GET_DORMANT_NODE,
	MEDIA_BEEP,					// addon-host only
	MEDIA_DEPRECATED_0,
	MEDIA_GET_DORMANT_FLAVOR,
	MEDIA_GET_DORMANT_FILE_FORMATS,
	MEDIA_GET_REALTIME_FLAGS,
	MEDIA_SET_REALTIME_FLAGS,
	MEDIA_GET_LATENT_INFO = '_TSA',
	MEDIA_REFERENCE_RELEASED,
	MEDIA_GET_RUNNING_DEFAULT,
	MEDIA_BUFFER_GROUP_REG,
	MEDIA_FIND_RUNNING_INSTANCES,
	MEDIA_GET_NODE_ID,
	MEDIA_ACK_FLAT_WEB,			//	to release the area flattened into for big webs
	MEDIA_SOUND_EVENT_CHANGED	// addon-host only
};

enum {	//	private notifications
	MEDIA_SET_NODE_WATCH_LIST = '_TS0', 
	ROSTER_RESCAN_BUFFER_GROUPS
};

enum {	/* FORMAT_OP */
	MF_SET_FORMAT = 1,
	MF_GET_FORMATS = 2,
	MF_BIND_FORMATS = 3
};

enum _media_capability
{
	B_MEDIA_CAP_IS_SERVER = B_MEDIA_FLAGS_PRIVATE,
	B_MEDIA_CAP_SET_SERVER,
	B_MEDIA_CAP_SET_DEBUG	//	bool *
};

	enum {
		kNoIAmNotTheAddonServer,
		kYesIAmTheAddonServer
	};

/* messages for BMediaNode remote control */
/* Codes are as follows:	*/
/* 0x0- Buffer passing */
/* 0x40000000- BMediaNode	*/
/* 0x40000100- BBufferProducer */
/* 0x40000200- BBufferConsumer */
/* 0x40000300- BTimeSource */
/* 0x40000400- BControllable */
/* 0x40000500- BFileInterface */
/* 0x40000600- BEntityInterface */
/* 0x40000700- Mixer private api */
/* 0x60000000- User-defined */

enum {
	M_START = 0x40000000,
	M_STOP,
	M_SEEK,
	M_SET_TIMESOURCE,
	M_SET_RUN_MODE,
	M_TIMEWARP,
	M_PREROLL,
	M_GET_TIMESOURCE,
	M_REQUEST_COMPLETED,
	M_NODE_DIED,
	M_ROLL,
	M_GET_ATTRIBUTES,
	M_SYNC,
	M_GET_MESSENGER,
	M_RECOVER_NODE,
	M_BUFFER = 0x40000200
};
enum {
	M_PREROLL_REPLY = 0x50000000L,
	M_STOP_REPLY,
	M_GET_TIMESOURCE_REPLY,
	M_GET_ATTRIBUTES_REPLY,
	M_SYNC_REPLY,
	M_GET_MESSENGER_REPLY,
	M_RECOVER_NODE_REPLY
};


#if 0
//	synchronous cmd header
struct sync_cmd_q {
	port_id reply;
	int32 cookie;
};
struct sync_cmd_a {
	status_t error;
	int32 cookie;
};
#endif

struct start_q {
	bigtime_t performance_time;
};

struct stop_q {
	enum {
		STOP_SYNC = 0x1
	};
	bigtime_t performance_time;
	uint32 flags;
	port_id reply;
	int32 cookie;
};
struct stop_a {
	status_t error;
	int32 cookie;
};

struct seek_q {
	bigtime_t media_time;
	bigtime_t performance_time;
};

struct set_time_source_q {
	media_node_id time_source;
};

struct set_run_mode_q {
	BMediaNode::run_mode mode;
	bigtime_t delay;
};

struct timewarp_q {
	bigtime_t real_time;
	bigtime_t performance_time;
};

struct preroll_q {
	port_id reply;
	int32 cookie;
};
struct preroll_a {
	int32 cookie;
};

struct get_timesource_q {
	port_id reply;
	int32 cookie;
};
struct get_timesource_a {
	int32 cookie;
	media_node_id time_source;
};

#define MAX_NODE_DIED_Q_COUNT ((B_MEDIA_MESSAGE_SIZE-sizeof(int32))/sizeof(media_node_id))
struct node_died_q {
	int32 nodeCount;
	media_node_id nodes[1];
};

struct roll_q {
	bigtime_t start;
	bigtime_t stop;
	bigtime_t media;
};

struct get_attributes_q {
	port_id reply;
	int32 cookie;
};
struct get_attributes_a {
	status_t error;
	int32 cookie;
};
#define MAX_NODE_ATTRIBUTE_COUNT ((B_MEDIA_MESSAGE_SIZE-sizeof(get_attributes_a))/sizeof(media_node_attribute))

struct sync_q {
	port_id reply;
	int32 cookie;
	bigtime_t performance_time;
};
struct sync_a {
	status_t error;
	int32 cookie;
	bigtime_t time;
	media_node_id node;
};

struct get_messenger_q {
	port_id reply;
	int32 cookie;
};
struct get_messenger_a {
	status_t error;
	int32 cookie;
	BMessenger messenger;
};


struct recover_node_q {
	port_id reply;
	int32 cookie;
};
struct recover_node_a {
	status_t error;
	int32 cookie;
	media_node_id id;
};


enum {	/* buffer producer messages */
	BP_SUGGEST_FORMAT = 0x40000100L,
	BP_PROPOSE_FORMAT,
	BP_ITERATE_OUTPUTS,
	BP_DISPOSE_COOKIE,
	BP_HOOKUP,
	BP_BREAK,
	BP_SET_BUFFERS,
	BP_YOURE_LATE,
	BP_CALC_TOTAL_LATENCY,
	BP_REQUEST_FORMAT_CHANGE,
	BP_MUTE_OUTPUT,
	BP_PREPARE_CONNECTION,
	BP_SET_RATE,
	BP_REQUEST_BUFFER,
	BP_LATENCY_CHANGED,
	BP_GET_INITIAL_LATENCY,
/* should have a re-set format function here */

/* special request for raw_video producers */
	BP_VIDEO_CLIP = 0x40000150L
};
enum {
	BP_SUGGEST_FORMAT_REPLY = 0x50000100L,
	BP_PROPOSE_FORMAT_REPLY,
	BP_ITERATE_OUTPUTS_REPLY,
	BP_HOOKUP_REPLY,
	BP_CALC_TOTAL_LATENCY_REPLY,
	BP_REQUEST_FORMAT_CHANGE_REPLY,
	BP_BREAK_REPLY,
	BP_MUTE_OUTPUT_REPLY,
	BP_PREPARE_CONNECTION_REPLY,
	BP_SET_RATE_REPLY,
	BP_GET_INITIAL_LATENCY_REPLY,
//	BP_SET_BUFFERS_REPLY,
	BP_VIDEO_CLIP_REPLY = 0x50000150L
};


#define MAX_CLIP_SIZE 16100
#define MAX_BUFFER_COUNT 128


struct suggest_format_q {
	port_id reply;
	int32 cookie;
	media_format format;
	media_type type;
	int32 quality;
};
struct suggest_format_a {
	status_t error;
	int32 cookie;
	media_format format;
};

struct propose_format_q {
	port_id reply;
	int32 cookie;
	media_format format;
	media_source output;
};
struct propose_format_a {
	status_t error;
	int32 cookie;
	media_format format;
};

struct iterate_outputs_q {
	port_id reply;
	int32 cookie;
};
struct iterate_outputs_a {
	status_t error;
	int32 cookie;
	media_output output;
};

struct dispose_cookie_q {
	int32 cookie;
};

struct hookup_q {
	port_id reply;
	int32 cookie;
	status_t status;
	media_output output;
	uint32 flags;
};
struct hookup_a {
	int32 cookie;
	media_output output;
};

struct prepare_connection_q {
	port_id reply;
	int32 cookie;
	media_source from;
	media_destination where;
	media_format format;
};
struct prepare_connection_a {
	status_t error;
	int32 cookie;
	media_output output;
};

struct break_q {
	port_id reply;
	int32 cookie;
	media_source from;
	media_destination where;
};
struct break_a {
	status_t error;
	int32 cookie;
};

struct youre_late_q {
	media_source source;
	bigtime_t how_much;
	bigtime_t performance_time;
};

struct calc_total_latency_q {
	port_id reply;
	int32 cookie;
};
struct calc_total_latency_a {
	status_t error;
	int32 cookie;
	bigtime_t latency;
};

struct request_format_change_q {
	void * user_data;
	int32 cookie;
	media_source source;
	media_destination destination;
	media_format format;
};
struct request_format_change_a {
	status_t error;
	int32 cookie;
	int32 change_tag;
	media_format format;
};

struct mute_output_q {
	void * user_data;
	int32 cookie;
	media_source source;
	media_destination destination;
	bool muted;
	bool _reserved_[3];
};
//struct mute_output_a {
//	status_t error;
//	int32 cookie;
//	int32 change_tag;
//};

struct set_buffers_q {
	void * user_data;
	int32 cookie;
	int32 change_tag;
	media_source where;
	media_destination destination;
	bool will_reclaim;
	bool _unused_[3];
	int32 count;
	media_buffer_id buffers[MAX_BUFFER_COUNT];
};
//this call cannot be synchronous
//struct set_buffers_a {
//	status_t error;
//	int32 cookie;
//};

struct set_rate_q {
	port_id reply;
	int32 cookie;
	int32 numer;
	int32 denom;
};
struct set_rate_a {
	status_t error;
	int32 cookie;
};

struct request_buffer_q {
	media_source source;
	bigtime_t prev_time;
	int32 prev_buffer;
	media_seek_tag prev_tag;
	uint32 flags;
	enum {
		B_TAG_VALID = 1
	};
};

struct latency_changed_q {
	media_source source;
	media_destination destination;
	bigtime_t new_latency;
	uint32 flags;
};

struct get_initial_latency_q {
	port_id reply;
	int32 cookie;
};
struct get_initial_latency_a {
	status_t error;
	int32 cookie;
	bigtime_t latency;
	uint32 flags;
};


struct video_clip_q {
	void * user_data;
	int32 cookie;	/* spit back in answer */
	media_source source;
	media_destination destination;
	media_video_display_info display;
	int32 format;	/* what format of "data" */
	size_t size;
	char data[MAX_CLIP_SIZE];
};
//struct video_clip_a {
//	status_t error;
//	int32 cookie;	/* from q */
//	int32 from_change_count;
//};


enum {	/* buffer consumer messages */
	BC_BUFFER = M_BUFFER,
	BC_ACCEPT_FORMAT = 0x40000201L,
	BC_ITERATE_INPUTS,
	BC_DISPOSE_COOKIE,
	BC_HOOKED_UP,
	BC_BROKEN,
	BC_DATA_STATUS,
	BC_GET_LATENCY,
	BC_CHANGE_FORMAT,
	BC_FIND_SEEK_TAG
};
enum {
	BC_ACCEPT_FORMAT_REPLY = 0x50000201L,
	BC_ITERATE_INPUTS_REPLY,
	BC_HOOKED_UP_REPLY,
	BC_GET_LATENCY_REPLY,
	BC_CHANGE_FORMAT_REPLY,
	BC_FIND_SEEK_TAG_REPLY
};

struct accept_format_q {
	port_id reply;
	media_destination input;
	media_format format;
};
struct accept_format_a {
	status_t error;
	media_format format;
};

struct iterate_inputs_q {
	port_id reply;
	int32 cookie;
};
struct iterate_inputs_a {
	status_t error;
	int32 cookie;
	media_input input;
};

struct hooked_up_q {
	port_id reply;
	int32 cookie;
	media_input input;
};
struct hooked_up_a {
	status_t error;
	int32 cookie;
	media_input input;
};

struct broken_q {
	media_source producer;
	media_destination where;
};

struct data_status_q {
	media_destination whom;
	bigtime_t at_time;
	int32 status;
};

struct get_latency_q {
	port_id reply;
	int32 cookie;
	media_destination destination;
};
struct get_latency_a {
	status_t error;
	int32 cookie;
	bigtime_t latency;
	media_node_id timesource;
};

struct change_format_q {
	port_id reply;
	int32 change_tag;
	media_source output;
	media_destination input;
	media_format format;
};
struct change_format_a {
	status_t error;
	int32 change_tag;
};

struct find_seek_tag_q {
	port_id reply;
	int32 cookie;
	media_destination destination;
	bigtime_t in_time;
	uint32 in_flags;
};

struct find_seek_tag_a {
	status_t error;
	int32 cookie;
	media_seek_tag tag;
	bigtime_t out_time;
	uint32 out_flags;
};



enum { /* time source messages */
	TS_ADD_NODE = 0x40000300,
	TS_REMOVE_NODE,
	TS_GET_START_LATENCY,
	TS_OP
};
enum {
	TS_GET_START_LATENCY_REPLY = 0x50000300,
	TS_OP_REPLY
};

struct add_node_q {
	media_node_id node;
};

struct remove_node_q {
	media_node_id node;
};

struct get_start_latency_q {
	port_id reply;
	int32 cookie;
};
struct get_start_latency_a {
	status_t error;
	int32 cookie;
	bigtime_t latency;
};

struct ts_op_q {
	BTimeSource::time_source_op op;
	port_id reply;
	bigtime_t real_time;
	bigtime_t performance_time;
	int32 _reserved2[6];
};

struct ts_op_a {
	status_t error;
};

enum { /* controllable messages */
	CT_GET_WEB = 0x40000400,
	CT_GET_VALUES, 
	CT_SET_VALUES,
	CT_START_CONTROL_PANEL
};
enum {
	CT_GET_WEB_REPLY = 0x50000400,
	CT_GET_VALUES_REPLY,
	CT_START_CONTROL_PANEL_REPLY
};

struct get_web_q {
	port_id reply;
};
struct get_web_a {
	status_t error;
	media_node node;
	int32 size;
#define _BIG_FLAT_WEB 0x1
	uint32 flags;
	struct {
		size_t big_size;
		area_id area;
		BMessenger owner;
	} big;
	char raw_data[16200];
};

struct get_values_q {
	port_id reply;
	int32 count_ids;
	int32 ids[2000];
};
struct get_values_a {
	status_t error;
	media_node node;
	int32 num_values;
	char raw_data[16200];
};

struct set_values_q {
	media_node node;	/* redundant, but part of the format... */
	int32 num_values;
	char raw_data[16200];
};

struct start_control_panel_q {
	port_id reply;
	int32 cookie;
};
struct start_control_panel_a {
	status_t error;
	int32 cookie;
	BMessenger messenger;
};



/* FileInterface allows you to specify a file to read data from */
/* or write data to. */

enum { /* file interface messages */
	FI_GET_LENGTH = 0x40000500,
	FI_SNIFF_FILE,
	FI_SPECIFY_FILE,
	FI_GET_CUR_FILE,
	FI_ITERATE_FILE_FORMATS,
	FI_DISPOSE_FILE_FORMAT_COOKIE
};
enum {
	FI_GET_LENGTH_REPLY = 0x50000500,
	FI_SNIFF_FILE_REPLY,
	FI_SPECIFY_FILE_REPLY,
	FI_GET_CUR_FILE_REPLY,
	FI_ITERATE_FILE_FORMATS_REPLY
};

struct get_length_q {
	port_id reply;
	int32 cookie;
};
struct get_length_a {
	status_t error;
	int32 cookie;
	bigtime_t length;
};

struct sniff_file_q {
	port_id reply;
	int32 cookie;
	int32 _reserved_[3];
	dev_t device;
	ino_t directory;
	char name[256];	/* could be extended for path on networks */
};
struct sniff_file_a {
	int32 cookie;
	status_t error;
	float quality;
	int32 _reserved_[3];
	char mime_type[256];
};

struct specify_file_q {
	port_id reply;
	int32 cookie;
	bool create;
	bool _reserved_b[3];
	int32 _reserved_i[2];
	dev_t device;
	ino_t directory;
	char name[256];	/* could be extended for path on networks */
};
struct specify_file_a {
	int32 cookie;
	status_t error;
	bigtime_t length;
};

struct get_cur_file_q {
	port_id reply;
	int32 cookie;
};
struct get_cur_file_a {
	int32 cookie;
	status_t error;
	char mime_type[256];
	int32 _reserved_[1];
	dev_t device;
	ino_t directory;
	char name[256];	/* could be extended for path on networks */
};

struct iterate_file_formats_q {
	port_id reply;
	int32 cookie;
};
struct iterate_file_formats_a {
	status_t error;
	int32 cookie;
	media_file_format format;
};

struct dispose_file_format_cookie_q {
	int32 cookie;
};

struct media_message {
	char whatever[B_MEDIA_MESSAGE_SIZE];
};


class _BMediaRosterP :
	public BMediaRoster
{
public:

		_BMediaRosterP(
				status_t * out_error = NULL);
		~_BMediaRosterP();

		void MessageReceived(
				BMessage * message);

		status_t SaveDefaultNode(
				media_node_id id,
				const media_node & node,
				const media_destination * dest = 0,
				const char * name = 0);
		status_t SaveDefaultDormant(
				media_node_id id,
				const dormant_node_info & node);
		status_t GetDormantFlavorInfoForP(
				const dormant_node_info & dormant,
				dormant_flavor_info * out_flavor,
				char * outPath,
				ssize_t inBufSize);

		status_t RegisterBuffer(
				BBuffer * buffer);
		BBuffer * FindBuffer(
				media_buffer_id buffer);

		status_t ReclaimOutputBuffers(
				BBufferGroup * into);
		void OrphanReclaimableBuffers(
				BBufferGroup * group);
		void AddBufferGroupToBeRegistered(
				area_id groupArea);
		void AddBufferGroupToBeUnregistered(
				area_id groupArea);
	enum {
		BUF_GRP_TABLE_SIZE = 64	//	power of two!
	};
static	std::list<area_id> s_bufferGroupsToRegister;
static	std::list<area_id> s_bufferGroupsToUnregister;
static	BLocker s_bufferGroupLock;

		area_id NewAreaUser(
				area_id to_clone);
		status_t RemoveAreaUser(
				area_id orig_area);
		status_t RegisterDedicatedArea(
				area_id no_clone);

		status_t MediaFormatChanged(
				const media_source & source, 
				const media_destination & destination, 
				const media_format & format);

		BTimeSource * ReturnNULLTimeSource();

		status_t Broadcast(
				BMessage & msg,
				uint32 what);
		status_t Broadcast(
				BMediaNode * node,
				BMessage & msg,
				uint32 what);

		enum {
			B_MEDIA_TYPE_NAME_LENGTH = 64,
			B_MEDIA_ITEM_NAME_LENGTH = 64
		};
		struct media_item {
			char name[B_MEDIA_ITEM_NAME_LENGTH];
			entry_ref ref;
			float audio_gain;
		};
		enum {
			B_OP_GET_TYPES = 1,
			B_OP_ADD_TYPE,
			B_OP_GET_ITEMS,
			B_OP_SET_ITEM,
			B_OP_CLEAR_ITEM,
			B_OP_REMOVE_ITEM
		};
		status_t GetMediaTypes(
				BList * out_types);
		status_t AddMediaType(
				const char * type);
		status_t GetTypeItems(
				const char * type,
				BList * out_items);
		status_t SetTypeItem(
				const char * type,
				const char * item,
				const entry_ref & ref);
		status_t SetTypeItemGain(
				const char * type,
				const char * item,
				float audio_gain);
		status_t RemoveTypeItemRef(
				const char * type,
				const char * item,
				const entry_ref & ref);	/* need to remove it if it is this -- could have changed meanwhile */
		status_t RemoveTypeItem(
				const char * type,
				const char * item);	/* need to remove it if it is this -- could have changed meanwhile */
		void SetDefaultHook(
				void (*func)(int32, void *),
				void * cookie);

static	status_t MediaErrorMessage(
				const char * message);

		status_t _SetOutputBuffersImp_(
				const media_source & source,
				const media_destination * destination,
				void * user_data,
				int32 change_tag,
				BBufferGroup * group,
				bool will_reclaim);

		status_t FlattenHugeWeb(
				BParameterWeb * web,
				size_t size,
				area_id * out_area,
				BMessenger * out_owner);
		status_t UnflattenHugeWeb(
				BParameterWeb * target,
				size_t size,
				area_id in_area,
				BMessenger & owner);

		status_t InstantiateDormantNode(
				const dormant_node_info & info,
				media_node * out_node,
				uint32 flags);

		void AddLocalInstance(BMediaNode*);
	
		// Registers a node but doesn't acquire reference.
		status_t RegisterUnownedNode(BMediaNode * node);

		status_t BadMediaAddonsMayCrashHere(bool ImTheAddonServer = false);

static	status_t AddCleanupFunction(
				void (*function)(void * cookie),
				void * cookie);
static	status_t RemoveCleanupFunction(
				void (*function)(void * cookie),
				void * cookie);

		typedef std::list<pair<media_node, sync_q> > sync_node_list;
		sync_node_list m_syncNodeList;
		BLocker m_syncNodeListLock;

		void _RegisterSync(const media_node & node, const sync_q & cmd);
		void _CancelSync(const media_node & node, const sync_q & cmd);

static	void roster_cleanup();

private:
	friend class BMediaRoster;
	friend class media_node;

		BMessenger _mServer;

static	status_t Turnaround(
				BMessage * request,
				BMessage * reply,
				bigtime_t send_timeout = DEFAULT_TIMEOUT,
				bigtime_t reply_timeout = DEFAULT_TIMEOUT);

		BTimeSource * GetSysTimeSrcClone(media_node_id nodeID);

//fixme	this really needs to be a regular map<>, or something else
		typedef rt_map<media_buffer_id, BBuffer *, 4096,
			SortedArray<media_buffer_id, pair<media_buffer_id, BBuffer *>,
			_select1st<pair<const media_buffer_id, BBuffer *>, const media_buffer_id>,
			_sorted_array_4096 > > buffers_map;

		buffers_map _mBuffers;
		int32 _mBufBenaphore;
		sem_id _mBufSemaphore;
		int32 _m_buf_wait_count;
		sem_id _m_buf_wait;
		typedef std::map<area_id, pair<area_id, int32> > areas_map;
		areas_map _mAreas;
		int32 _mAreaBenaphore;
		sem_id _mAreaSemaphore;
		BTimeSource * _mSystemTimeSource;
		bool _mInMediaAddonServer;
		BLocker _mNodeMapLock;

		typedef std::map<media_node, std::multimap<int32, BMessenger> > watch_map;
		watch_map _m_nodeWatch;
		BLocker _m_nodeWatchLock;

		typedef std::map<media_node_id, std::pair<media_node, int32> > referenced_node_map;
		referenced_node_map _mReferencedNodes;

		std::map<media_addon_id, BMediaAddOn*> _mLoadedAddons;
		std::map<media_node_id, BMediaNode*> _mLocalInstances;
		std::map<media_node_id, media_node_id> _mDefaultIDMap;

		typedef std::map<media_node_id,_BTimeSourceP*> systimesrc_map;
		systimesrc_map _mSysTimeSrcMap; // locked with the areas lock
		void (*_mDefaultHook)(int32, void *);
		void * _mDefaultCookie;

		typedef std::list<pair<void (*)(void *), void *> > cleanup_func_list;
static	cleanup_func_list _m_cleanupFuncs;
static	BLocker _m_cleanupLock;

		media_node_id FindRealIDForDefault(media_node_id);
		status_t ReleaseNodeP(const media_node&);


		status_t RegisterNodeP(BMediaNode * node, bool acquireReference);

		// If the node is already reference locally, returns the node and
		// increments the local reference count.  Otherwise, increments
		// the global reference count and gets node info for this node.
		status_t AcquireNodeReference(
				media_node_id id,
				media_node * out_node);
		
		struct auto_lock_bufs {
			_BMediaRosterP * _r;
		public:
			auto_lock_bufs(_BMediaRosterP * r)
				{
					_r = r; if (_r) _r->lock_bufs();
				}
			~auto_lock_bufs()
				{
					if (_r) _r->unlock_bufs();
				}
			void unlock()
				{
					if (_r) _r->unlock_bufs();
					_r = NULL;
				}
			void lock(_BMediaRosterP * r)
				{
					if (_r) _r->unlock_bufs();
					_r = r;
					if (_r) _r->lock_bufs();
				}
		};
		friend struct auto_lock_bufs;
		void lock_bufs()
			{
				if (atomic_add(&_mBufBenaphore, -1) != 1)
					acquire_sem(_mBufSemaphore);
			}
		void unlock_bufs()
			{
				if (atomic_add(&_mBufBenaphore, 1) != 0)
					release_sem(_mBufSemaphore);
			}

		struct auto_lock_areas {
			_BMediaRosterP * _r;
		public:
			auto_lock_areas(_BMediaRosterP * r)
				{
					_r = r; if (_r) _r->lock_areas();
				}
			~auto_lock_areas()
				{
					if (_r) _r->unlock_areas();
				}
		};
		friend struct auto_lock_areas;
		void lock_areas()
			{
				if (atomic_add(&_mAreaBenaphore, -1) != 1)
					acquire_sem(_mAreaSemaphore);
			}
		void unlock_areas()
			{
				if (atomic_add(&_mAreaBenaphore, 1) != 0)
					release_sem(_mAreaSemaphore);
			}
};


struct timecode_info;

/* string truncate copy */
status_t _set_default_timecode(const timecode_info * in_timecode);


struct guid_layout {
	uint32	time_low;
	uint16	time_mid;
	uint16	time_hi_and_version;
	uint8	clock_seq_hi_and_reserved;
	uint8	clock_seq_low;
	uint8	node[6];
};
struct guid_shared_mem {
	sem_id access;
	int32 chunk_cnt;
	guid_layout next_guid;
};

extern BLocker guid_lock;
extern guid_shared_mem * _g_ptr;


class _StReleaseNode
{
		media_node & _m_node;
		bool _release;
public:
		_StReleaseNode(
				media_node & node) :
			_m_node(node)
			{
				_release = true;
			}
		~_StReleaseNode()
			{
				if (_release && (_m_node != media_node::null))
				{
					BMediaRoster * r = BMediaRoster::CurrentRoster();
					if (r != NULL)
					{
						r->ReleaseNode(_m_node);
					}
				}
			}
		void Clear()
			{
				_release = false;
			}
};


#endif /* _TRINITY_P_H */
