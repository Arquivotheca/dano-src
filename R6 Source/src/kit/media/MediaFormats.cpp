/*	The MediaFormats stuff bypasses the MediaRoster.	*/
/*	This is so that format registration etc can happen in an	*/
/*	independent fashion.	*/

#include <Autolock.h>
#include <string.h>
#include <stdlib.h>

#include "addons.h"
#include "trinity_p.h"
#include "MediaFormats.h"

#define DIAGNOSTIC(x) fprintf x
#if !NDEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x) (void)0
#endif

const type_code B_CODEC_TYPE_INFO = 0x040807b2;

static status_t CompareMediaFormat(const media_format *format1,
	const media_format *format2)
{
	const void *meta1 = format1->MetaData();
	int32 size1 = format1->MetaDataSize();
	const void *meta2 = format2->MetaData();
	int32 size2 = format2->MetaDataSize();

	if (format1->type != format2->type) return B_MEDIA_BAD_FORMAT;
	
	if(format1->require_flags & format2->deny_flags)
		return B_MEDIA_BAD_FORMAT;
	if(format2->require_flags & format1->deny_flags)
		return B_MEDIA_BAD_FORMAT;
	
	if (meta1 && meta2 && ((size1 != size2) || memcmp(meta1,meta2,size1))) return B_MEDIA_BAD_FORMAT;
	switch (format2->type) {
		case B_MEDIA_RAW_VIDEO: {
			#define MRV(a) 	((format1->u.raw_video.a == format2->u.raw_video.a) ||											\
							 (format1->u.raw_video.a == media_raw_video_format::wildcard.a) ||							\
							 (format2->u.raw_video.a == media_raw_video_format::wildcard.a))
			#define MRVD(a)	((format1->u.raw_video.display.a == format2->u.raw_video.display.a) ||							\
							 (format1->u.raw_video.display.a == media_raw_video_format::wildcard.display.a) ||					\
							 (format2->u.raw_video.display.a == media_raw_video_format::wildcard.display.a))
			if (MRV(field_rate) && MRV(interlace) && MRV(first_active) && MRV(last_active) &&
				MRV(orientation) && MRV(pixel_width_aspect) && MRV(pixel_height_aspect) &&
				MRVD(format) && MRVD(line_width) && MRVD(line_count) && MRVD(bytes_per_row) &&
				MRVD(pixel_offset) && MRVD(line_offset)) return B_OK;
			#undef MRV
			#undef MRVD
		} break;
		case B_MEDIA_ENCODED_VIDEO: {
			#define MEV(a) 	((format1->u.encoded_video.a == format2->u.encoded_video.a) ||									\
							 (format1->u.encoded_video.a == media_encoded_video_format::wildcard.a) ||					\
							 (format2->u.encoded_video.a == media_encoded_video_format::wildcard.a))
			#define MRV(a) 	((format1->u.encoded_video.output.a == format2->u.encoded_video.output.a) ||					\
							 (format1->u.encoded_video.output.a == media_raw_video_format::wildcard.a) ||					\
							 (format2->u.encoded_video.output.a == media_raw_video_format::wildcard.a))
			#define MRVD(a)	((format1->u.encoded_video.output.display.a == format2->u.encoded_video.output.display.a) ||	\
							 (format1->u.encoded_video.output.display.a == media_raw_video_format::wildcard.display.a) ||			\
							 (format2->u.encoded_video.output.display.a == media_raw_video_format::wildcard.display.a))
			if (MEV(avg_bit_rate) && MEV(max_bit_rate) && MEV(encoding) &&
				MEV(frame_size) && MEV(forward_history) && MEV(backward_history) &&
				MRV(field_rate) && MRV(interlace) && MRV(first_active) && MRV(last_active) &&
				MRV(orientation) && MRV(pixel_width_aspect) && MRV(pixel_height_aspect) &&
				MRVD(format) && MRVD(line_width) && MRVD(line_count) && MRVD(bytes_per_row) &&
				MRVD(pixel_offset) && MRVD(line_offset)) return B_OK;
			#undef MEV
			#undef MRV
			#undef MRVD
		} break;
		case B_MEDIA_RAW_AUDIO: {
			#define MRA(a) 	((format1->u.raw_audio.a == format2->u.raw_audio.a) ||											\
							 (format1->u.raw_audio.a == media_raw_audio_format::wildcard.a) ||							\
							 (format2->u.raw_audio.a == media_raw_audio_format::wildcard.a))
			if (MRA(frame_rate) && MRA(channel_count) &&
				MRA(format) && MRA(byte_order) && MRA(buffer_size)) return B_OK;
			#undef MRA
		} break;
		case B_MEDIA_ENCODED_AUDIO: {
			#define MEA(a) 	((format1->u.encoded_audio.a == format2->u.encoded_audio.a) ||									\
							 (format1->u.encoded_audio.a == media_encoded_audio_format::wildcard.a) ||					\
							 (format2->u.encoded_audio.a == media_encoded_audio_format::wildcard.a))
			#define MRA(a) 	((format1->u.encoded_audio.output.a == format2->u.encoded_audio.output.a) ||					\
							 (format1->u.encoded_audio.output.a == media_raw_audio_format::wildcard.a) ||					\
							 (format2->u.encoded_audio.output.a == media_raw_audio_format::wildcard.a))
			if (MEA(bit_rate) && MEA(encoding) && MEA(frame_size) &&
				MRA(frame_rate) && MRA(channel_count) && MRA(format) &&
				MRA(byte_order) && MRA(buffer_size)) return B_OK;
			#undef MEA
			#undef MRA
		} break;
		default:
			; // nothing
	};
	
	return B_MEDIA_BAD_FORMAT;
};


/*	SpecializeMediaFormat will take the input formats and mangle the second to match the first.
	If the formats match initially, it will simply specialize inout_specialized where neccessary.
	If the formats don't match initially, the difference between inout_specialized when it
	comes in and when it goes out are the changes neccessary to make the formats compatible. */
	
static status_t SpecializeMediaFormat(const media_format *in_specializeTo, media_format *inout_specialized)
{
	if (!in_specializeTo || !inout_specialized) return B_BAD_VALUE;
	if (inout_specialized->type <= 0) {
		inout_specialized->type = in_specializeTo->type;
	}
	if (in_specializeTo->type != inout_specialized->type) return B_MEDIA_BAD_FORMAT;

	if (in_specializeTo->MetaData() != NULL) {
		inout_specialized->SetMetaData(in_specializeTo->MetaData(),in_specializeTo->MetaDataSize());
	};

	switch (in_specializeTo->type) {
		case B_MEDIA_RAW_VIDEO: {
			#define MRV(a) if (in_specializeTo->u.raw_video.a != media_raw_video_format::wildcard.a) inout_specialized->u.raw_video.a = in_specializeTo->u.raw_video.a;
			#define MRVD(a) if (in_specializeTo->u.raw_video.display.a != media_raw_video_format::wildcard.display.a) inout_specialized->u.raw_video.display.a = in_specializeTo->u.raw_video.display.a;
			MRV(field_rate);
			MRV(interlace);
			MRV(first_active);
			MRV(last_active);
			MRV(orientation);
			MRV(pixel_width_aspect);
			MRV(pixel_height_aspect);
			MRVD(format);
			MRVD(line_width);
			MRVD(line_count);
			MRVD(bytes_per_row);
			MRVD(pixel_offset);
			MRVD(line_offset);
			#undef MRV
			#undef MRVD
		} break;
		case B_MEDIA_ENCODED_VIDEO: {
			#define MEV(a) if (in_specializeTo->u.encoded_video.a != media_encoded_video_format::wildcard.a) inout_specialized->u.encoded_video.a = in_specializeTo->u.encoded_video.a;
			#define MRV(a) if (in_specializeTo->u.encoded_video.output.a != media_raw_video_format::wildcard.a) inout_specialized->u.encoded_video.output.a = in_specializeTo->u.encoded_video.output.a;
			#define MRVD(a) if (in_specializeTo->u.encoded_video.output.display.a != media_raw_video_format::wildcard.display.a) inout_specialized->u.encoded_video.output.display.a = in_specializeTo->u.encoded_video.output.display.a;
			MEV(avg_bit_rate);
			MEV(max_bit_rate);
			MEV(encoding);
			MEV(frame_size);
			MEV(forward_history);
			MEV(backward_history);
			MRV(field_rate);
			MRV(interlace);
			MRV(first_active);
			MRV(last_active);
			MRV(orientation);
			MRV(pixel_width_aspect);
			MRV(pixel_height_aspect);
			MRVD(format);
			MRVD(line_width);
			MRVD(line_count);
			MRVD(bytes_per_row);
			MRVD(pixel_offset);
			MRVD(line_offset);
			#undef MEV
			#undef MRV
			#undef MRVD
		} break;
		case B_MEDIA_RAW_AUDIO: {
			#define MRA(a) if (in_specializeTo->u.raw_audio.a != media_raw_audio_format::wildcard.a) inout_specialized->u.raw_audio.a = in_specializeTo->u.raw_audio.a;
			MRA(frame_rate);
			MRA(channel_count);
			MRA(format);
			MRA(byte_order);
			if (in_specializeTo->u.raw_audio.channel_count <= 0) {
				if (inout_specialized->u.raw_audio.buffer_size <= 0) {
					inout_specialized->u.raw_audio.buffer_size =
						inout_specialized->u.raw_audio.channel_count *
						in_specializeTo->u.raw_audio.buffer_size;
				}
				else {
					MRA(buffer_size);
				}
			}
			else {
				MRA(buffer_size);
			}
			#undef MRA
		} break;
		case B_MEDIA_ENCODED_AUDIO: {
			#define MEA(a) if (in_specializeTo->u.encoded_audio.a != media_encoded_audio_format::wildcard.a) inout_specialized->u.encoded_audio.a = in_specializeTo->u.encoded_audio.a;
			#define MRA(a) if (in_specializeTo->u.encoded_audio.output.a != media_raw_audio_format::wildcard.a) inout_specialized->u.encoded_audio.output.a = in_specializeTo->u.encoded_audio.output.a;
			MEA(bit_rate);
			MEA(encoding);
			MEA(frame_size);
			MRA(frame_rate);
			MRA(channel_count);
			MRA(format);
			MRA(byte_order);
			if (in_specializeTo->u.encoded_audio.output.channel_count <= 0) {
				if (inout_specialized->u.encoded_audio.output.buffer_size <= 0) {
					inout_specialized->u.encoded_audio.output.buffer_size =
						inout_specialized->u.encoded_audio.output.channel_count *
						in_specializeTo->u.encoded_audio.output.buffer_size;
				}
				else {
					MRA(buffer_size);
				}
			}
			else {
				MRA(buffer_size);
			}
			#undef MEA
			#undef MRA
		} break;
		default:
			return B_MEDIA_BAD_FORMAT;
	};
	return B_OK;
};

//	name mangling is a compiler feature, not a platform feature
#if defined(__GNUC__)
/* R4.5-x86 binary compability for removed
 * bool media_format::Matches(media_format *)
 */
extern "C" {
	bool Matches__12media_formatP12media_format(media_format *objptr,
	                                            media_format *otherFormat)
	{
		return objptr->Matches(otherFormat);
	}
}
#elif defined(__MWERKS__)
/* R4.5-ppc binary compability for removed
 * bool media_format::Matches(media_format *)
 */
extern "C" {
	bool Matches__12media_formatFP12media_format(media_format *objptr,
	                                            media_format *otherFormat)
	{
		return objptr->Matches(otherFormat);
	}
}
#else
#error no compiler we know
#endif

bool media_format::Matches(const media_format *otherFormat) const
{
	return (CompareMediaFormat(this,otherFormat) == B_OK);
}

extern "C" {
#if defined(__GNUC__)
void SpecializeTo__12media_formatP12media_format(media_format * that, const media_format * other)
#elif defined(__MWERKS__)
void SpecializeTo__12media_formatFP12media_format(media_format * that, const media_format * other)
#else
#error no compiler we know
#endif
{
	return that->SpecializeTo(other);
}
}

void media_format::SpecializeTo(const media_format *otherFormat)
{
	SpecializeMediaFormat(otherFormat,this);
}

extern "C" {
#if defined(__GNUC__)
void SetMetaData__12media_formatPvl(media_format * fmt, const void * d, int32 size)
#elif defined(__MWERKS__)
void SetMetaData__12media_formatFPvl(media_format * fmt, const void * d, int32 size)
#else
#error no compiler we know
#endif
{
	(void)fmt->SetMetaData(d, size);
}
}

status_t media_format::SetMetaData(const void *data, size_t size)
{
	if ((meta_data_area != B_BAD_VALUE) && meta_data_area) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		if ((ti.team == team) && (thisPtr == this))
			((_BMediaRosterP *)BMediaRoster::Roster())->RemoveAreaUser(meta_data_area);
	}
	
	if ((data == NULL) || (size == 0)) {
		meta_data = NULL;
		meta_data_size = 0;
		meta_data_area = B_BAD_VALUE;
		use_area = B_BAD_VALUE;
		team = B_BAD_VALUE;
		thisPtr = NULL;
		return B_OK;
	}

	meta_data_area = create_area("meta_data",&meta_data,B_ANY_ADDRESS,
		(size+4095)&(~4095),B_NO_LOCK,B_READ_AREA|B_WRITE_AREA);
	if (meta_data_area < 0) return meta_data_area;

	((_BMediaRosterP *)BMediaRoster::Roster())->RegisterDedicatedArea(meta_data_area);
	use_area = ((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(meta_data_area);
	if (use_area < 0) return use_area;

	area_info ai;
	thread_info ti;
	get_area_info(use_area,&ai);
	get_thread_info(find_thread(NULL),&ti);
	meta_data = ai.address;
	meta_data_size = size;
	memcpy(meta_data,data,size);
	team = ti.team;
	thisPtr = this;

//	printf("%08x: Setting new %08x, %d, %d\n",this,meta_data,meta_data_area,use_area);
	return B_OK;
}

const void * media_format::MetaData() const
{
	if ((meta_data_area == B_BAD_VALUE) || !meta_data_area) return NULL;

	thread_info ti;
	get_thread_info(find_thread(NULL),&ti);

	media_format *f = const_cast<media_format*>(this);

	if ((ti.team == team) && (thisPtr == f)) return meta_data;

	f->use_area = ((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(meta_data_area);
	if (f->use_area < 0) return NULL;

	area_info ai;
	get_area_info(use_area,&ai);
	f->meta_data = ai.address;

	f->team = ti.team;
	f->thisPtr = f;

	return meta_data;
};

int32 media_format::MetaDataSize() const
{
	return meta_data_size;
};

media_format::media_format(const media_format & other)
{
	memset(this, 0, sizeof(*this));
	meta_data = NULL;
	meta_data_size = 0;
	meta_data_area = B_BAD_VALUE;
	use_area = B_BAD_VALUE;
	team = B_BAD_VALUE;
	thisPtr = NULL;
	*this = other;
}

media_format::media_format()
{
	memset(this, 0, sizeof(*this));
	meta_data = NULL;
	meta_data_size = 0;
	meta_data_area = B_BAD_VALUE;
	use_area = B_BAD_VALUE;
	team = B_BAD_VALUE;
	thisPtr = NULL;
}

media_format::~media_format()
{
	if ((meta_data_area != B_BAD_VALUE) && meta_data_area) {
		_BMediaRosterP * r = (_BMediaRosterP *)BMediaRoster::CurrentRoster();
		if (r != NULL) {
			thread_info ti;
			get_thread_info(find_thread(NULL),&ti);
			if ((ti.team == team) && (thisPtr == this)) {
				r->RemoveAreaUser(meta_data_area);
			}
		};
	};
}

media_format &
media_format::operator=(const media_format &clone)
{
	if ((meta_data_area != B_BAD_VALUE) && meta_data_area) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		if ((ti.team == team) && (thisPtr == this)) {
			((_BMediaRosterP *)BMediaRoster::Roster())->RemoveAreaUser(meta_data_area);
		};
	};
	memcpy(this,&clone,sizeof(media_format));
	if ((meta_data_area != B_BAD_VALUE) && meta_data_area) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		use_area = ((_BMediaRosterP *)BMediaRoster::Roster())->NewAreaUser(meta_data_area);
		area_info ai;
		get_area_info(use_area,&ai);
		meta_data = ai.address;
		thisPtr = this;
		team = ti.team;
	};
	return *this;
}

namespace BPrivate {
class _MediaFormat {
public:
		_MediaFormat(
				const media_format & format,
				const media_format_description & description,
				uint32 priority) :
			m_format(format),
			m_description(description),
			m_priority(priority)
			{
			}
		bool RecognizeFormat(
				const media_format & format)
			{
				if (format.type != m_format.type) return false;
				switch (format.type) {
				case B_MEDIA_RAW_AUDIO:
					return format_is_compatible(format, m_format);
					break;
				case B_MEDIA_RAW_VIDEO:
					return format_is_compatible(format, m_format);
					break;
				case B_MEDIA_ENCODED_AUDIO:
					if (format.u.encoded_audio.encoding <= 0) return false;
					return format_is_compatible(format, m_format);
					break;
				case B_MEDIA_ENCODED_VIDEO:
					if (format.u.encoded_video.encoding <= 0) return false;
					return format_is_compatible(format, m_format);
					break;
				case B_MEDIA_MULTISTREAM:
					if (format.u.multistream.format <= 0) return false;
					return format_is_compatible(format, m_format);
					break;
				default:
					/* who knows? */
					;
				}
				return true;
			}
		bool RecognizeDescription(
				const media_format_description & description)
			{
				return (m_description == description);
			}
		bool PreferredOver(
				const _MediaFormat & other)
			{
				return m_priority > other.m_priority;
			}
		void GetFormat(
				media_format * out_format)
			{
				*out_format = m_format;
			}
		void GetDescription(
				media_format_description * out_description)
			{
				*out_description = m_description;
			}
		addon_list m_addons;
private:
		media_format m_format;
		media_format_description m_description;
		uint32 m_priority;
};
}
using namespace BPrivate;

//	All BMediaFormats objects share a list.
//	Although creating a BMediaFormats will still re-read the list
//	to get new data from the server, this allows us to update the
//	list for everybody when someone registers new stuff.
//	The locking semantics for RewindFormats()/GetNextFormat() are
//	such that there will be no inconsistencies -- you have to hold
//	the lock when calling these functions.
BLocker BMediaFormats::s_lock("BMediaFormats");
BList BMediaFormats::s_formats;
BMessenger BMediaFormats::s_server;
int32 BMediaFormats::s_cleared = 1;

BMediaFormats::BMediaFormats()// :
//	m_lock("BMediaFormats"), 
//	m_server(B_MEDIA_SERVER_SIGNATURE)
{
	m_lock_count = 0;
	m_index = -1;
}

BMediaFormats::~BMediaFormats()
{
	clear_formats();
	if (m_lock_count > 0) {
		DIAGNOSTIC((stderr, "Deleting a locked BMediaFormats\n"));
		while (m_lock_count-- > 0) {
			s_lock.Unlock();
		}
	}
}

status_t
BMediaFormats::InitCheck()
{
	BAutolock lock(s_lock);
	(void)get_server();
	if (s_server.IsValid()) {
		return B_OK;
	}
	if (s_server.Team() < 0) return s_server.Team();
	return B_BAD_HANDLER;
}

void
BMediaFormats::ex_clear_formats_imp()
{
	clear_formats_imp();
}

BMessenger &
BMediaFormats::get_server()
{
static int32 inited = 0;
	if (!atomic_or(&inited, 1)) {
		atexit(ex_clear_formats_imp);
	}
	BAutolock lock(s_lock);
	if (!s_server.IsValid()) {
		BMessenger msgr(B_MEDIA_SERVER_SIGNATURE);
		s_server = msgr;
	}
	return s_server;
}

status_t 
BMediaFormats::MakeFormatFor(
	const media_format_description *descs,
	int32 desc_count,
	media_format *io_format,
	uint32 flags,
	void */*_reserved*/)
{
	if (!descs) return B_BAD_VALUE;
	if (desc_count < 1) return B_BAD_VALUE;
	if (!io_format) return B_BAD_VALUE;
	BMessage msg(MEDIA_FORMAT_OP);
	msg.AddInt32("be:_op", MF_SET_FORMAT);
	msg.AddData("be:_format", B_RAW_TYPE, io_format, sizeof(*io_format));
	msg.AddInt32("be:_make_flags", flags);
	for (int ix=0; ix<desc_count; ix++) {
		msg.AddData("be:_description", B_RAW_TYPE, &descs[ix], sizeof(*descs));
	}
	BMessage reply;
	status_t err = get_server().SendMessage(&msg, &reply, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		reply.FindInt32("error", &err);
	}
	if (err >= B_OK) {
		BAutolock lock(s_lock);
		ssize_t size = 0;
		void * ptr = NULL;
		err = reply.FindData("be:_format", B_RAW_TYPE, (const void **)&ptr, &size);
		if (err >= B_OK) {
			if (ptr != NULL) {
				*io_format = *reinterpret_cast<media_format *>(ptr);
				clear_formats();
			}
			else {
				err = B_ERROR;
			}
		}
	}
	return err > 0 ? 0 : err;
}

status_t
BMediaFormats::MakeFormatFor(
	const media_format_description & desc,
	const media_format & in_format,
	media_format * out_format)
{
	*out_format = in_format;
	return MakeFormatFor(&desc, 1, out_format);
}

status_t 
BMediaFormats::GetBeOSFormatFor(uint32 fourcc, media_format *out_format,
                                media_type type)
{
	BMediaFormats				formatObject;
	media_format_description	formatDescription;
	status_t err;

	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription.family = B_BEOS_FORMAT_FAMILY;
	formatDescription.u.beos.format = fourcc;
	err = formatObject.GetFormatFor(formatDescription, out_format);
	if(err != B_NO_ERROR)
		return err;
	if(type != B_MEDIA_UNKNOWN_TYPE && type != out_format->type)
		return B_BAD_TYPE;
	return B_NO_ERROR;
}

status_t 
BMediaFormats::GetAVIFormatFor(uint32 fourcc, media_format *out_format,
                               media_type type)
{
	BMediaFormats				formatObject;
	media_format_description	formatDescription;
	status_t err;

	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription.family = B_AVI_FORMAT_FAMILY;
	formatDescription.u.avi.codec = fourcc;
	err = formatObject.GetFormatFor(formatDescription, out_format);
	if(err != B_NO_ERROR)
		return err;
	if(type != B_MEDIA_UNKNOWN_TYPE && type != out_format->type)
		return B_BAD_TYPE;
	return B_NO_ERROR;
}

status_t 
BMediaFormats::GetQuicktimeFormatFor(uint32 vendor, uint32 fourcc,
                                     media_format *out_format, media_type type)
{
	BMediaFormats				formatObject;
	media_format_description	formatDescription;
	status_t err;

	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription.family = B_QUICKTIME_FORMAT_FAMILY;

	formatDescription.u.quicktime.vendor = vendor;
	formatDescription.u.quicktime.codec = fourcc;
	err = formatObject.GetFormatFor(formatDescription, out_format);
	if(err != B_NO_ERROR)
		return err;
	if(type != B_MEDIA_UNKNOWN_TYPE && type != out_format->type)
		return B_BAD_TYPE;
	return B_NO_ERROR;
}

status_t
BMediaFormats::GetFormatFor(
	const media_format_description & desc,
	media_format * out_format)
{
	status_t err = B_OK;
	BAutolock lock(s_lock);
	if (s_cleared) {
		err = get_formats();
	}
	if (err < 0) return err;
	_MediaFormat * m_that = 0;
	for (int ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * cur = (_MediaFormat *)s_formats.ItemAt(ix);
		if (cur->RecognizeDescription(desc)) {
			if ((m_that == 0) || (cur->PreferredOver(*m_that))) {
				cur->GetFormat(out_format);
				m_that = cur;
			}
		}
	}
	return (m_that == 0) ? B_MEDIA_BAD_FORMAT : B_OK;
}

status_t
BMediaFormats::GetCodeFor(
	const media_format & format,
	media_format_family family,
	media_format_description * out_description)
{
	status_t err = B_OK;
	BAutolock lock(s_lock);
	if (s_cleared) {
		err = get_formats();
	}
	if (err < 0) return err;
	_MediaFormat * that = 0;
	for (int ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * cur = (_MediaFormat *)s_formats.ItemAt(ix);
		if (cur->RecognizeFormat(format) && ((that == 0) || cur->PreferredOver(*that))) {
			media_format_description temp;
			cur->GetDescription(&temp);
			if ((temp.family == family) || !family) {
				that = cur;
				*out_description = temp;
			}
		}
	}
	return (that == 0) ? B_MEDIA_BAD_FORMAT : B_OK;
}

status_t
BMediaFormats::RewindFormats()
{
	//	make sure it's called with the lock held
	if (!s_lock.IsLocked()) return EPERM;
	//	get_formats() will make sure WE hold the lock
	return get_formats();
}

status_t
BMediaFormats::GetNextFormat(
	media_format * out_format,
	media_format_description * out_description)
{
	//	make sure it's called with the lock held
	if (!s_lock.IsLocked()) {
		return EPERM;
	}
	status_t err = B_OK;
	BAutolock lock(s_lock);	//	make sure WE hold the lock
	if (s_cleared) {
		err = get_formats();
	}
	if (err < 0) return err;
	if (m_index >= s_formats.CountItems()) {
		return B_BAD_INDEX;
	}
	int ix = m_index;
	m_index++;
	((_MediaFormat *)s_formats.ItemAt(ix))->GetFormat(out_format);
	((_MediaFormat *)s_formats.ItemAt(ix))->GetDescription(out_description);
	return B_OK;
}

bool
BMediaFormats::Lock()
{
	bool b = s_lock.Lock();
	if (b) {
		m_lock_count++;
	}
	return b;
}

void
BMediaFormats::Unlock()
{
	if (!s_lock.IsLocked()) return;
	m_lock_count--;
	s_lock.Unlock();
}

void
BMediaFormats::clear_formats()
{
	clear_formats_imp();
	m_index = -1;
}

void
BMediaFormats::clear_formats_imp()
{
	BAutolock lock(s_lock);
	//puts("clear_formats_imp()");
	for (int ix=0; ix<s_formats.CountItems(); ix++) {
		delete (_MediaFormat *)s_formats.ItemAt(ix);
	}
	s_formats.MakeEmpty();
	s_cleared = 1;
}

status_t
BMediaFormats::get_formats()
{
	status_t err = get_formats_imp();
	if (err < B_OK) {
		return err;
	}
	m_index = 0;
	return B_OK;
}

status_t
BMediaFormats::get_formats_imp()
{
	BMessage msg(MEDIA_FORMAT_OP);
	msg.AddInt32("be:_op", MF_GET_FORMATS);
	BMessage reply;
	status_t err = get_server().SendMessage(&msg, &reply, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		reply.FindInt32("error", &err);
	}
	if (err >= B_OK) {
		BAutolock lock(s_lock);
		clear_formats_imp();
		const void * format;
		const void * description;
		ssize_t fsize;
		ssize_t dsize;
		uint32 priority = 0;
		for (int ix=0; !reply.FindData("be:_format", B_RAW_TYPE, ix, &format, &fsize) && 
				!reply.FindData("be:_description", B_RAW_TYPE, ix, &description, &dsize) &&
				!reply.FindInt32("be:_priority", ix, reinterpret_cast<int32*>(&priority)); ix++) {
			_MediaFormat * mf = new _MediaFormat(*(media_format *)format, 
					*(media_format_description *)description, priority);
			BMessage msg;
			if (!reply.FindMessage("be:_addons", ix, &msg)) {
				const char * path;
				for (int iz=0; !msg.FindString("be:_path", iz, &path); iz++) {
					mf->m_addons.push_back(path);
				}
			}
			s_formats.AddItem(mf);
		}
		s_cleared = 0;
	}
	return err > 0 ? 0 : err;
}

bool
BMediaFormats::is_bound(const char * addon, const media_format * formats, int32 count)
{
	BAutolock lock(s_lock);
	int32 found = 0;
	for (int ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * mf = (_MediaFormat *)s_formats.ItemAt(ix);
		for (int iz=0; iz<count; iz++) {
			if (mf->RecognizeFormat(formats[iz])) {
				for (int ix=0; ix<mf->m_addons.size(); ix++) {
					if (!strcmp(mf->m_addons[ix], addon)) {
						found++;
						goto another_one;
					}
				}
				return false;	//	it should be here, but isn't -- we know it's not bound
			}
		}
another_one:
		;
	}
	return (found == count);
}

status_t 
BMediaFormats::bind_addon(const char *addon, const media_format *formats, int32 count)
{
	if (is_bound(addon, formats, count)) {
		FPRINTF((stderr, "   ... already bound\n"));
		return B_OK;
	}
	BMessenger server(B_MEDIA_SERVER_SIGNATURE);
	if (!server.IsValid()) return (server.Team() < 0) ? server.Team() : B_BAD_TEAM_ID;
	BMessage msg(MEDIA_FORMAT_OP);
	msg.AddInt32("be:_op", MF_BIND_FORMATS);
	for (int ix=0; ix<count; ix++) {
		msg.AddData("be:_format", B_RAW_TYPE, formats+ix, sizeof(*formats));
	}
	msg.AddString("be:_path", addon);
	BMessage reply;
	status_t err = server.SendMessage(&msg, &reply, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);
	if (err == B_OK) {
		reply.FindInt32("error", &err);
	}
	if (err == B_OK) {
		clear_formats_imp();
	}
	return err;
}

status_t 
BMediaFormats::find_addons(const media_format *format, addon_list &addons)
{
	BAutolock lock(s_lock);
	status_t err = B_OK;
	if (s_cleared) err = get_formats_imp();
	if (err < B_OK) return err;
	err = B_MEDIA_BAD_FORMAT;
	int cnt = s_formats.CountItems();
	for (int ix=0; ix<cnt; ix++) {
		_MediaFormat * fmt = (_MediaFormat *)s_formats.ItemAt(ix);
		if (fmt->RecognizeFormat(*format)) {
			FPRINTF((stderr, "format recognized, copying %d add-ons\n", fmt->m_addons.size()));
			addons = fmt->m_addons;
			err = B_OK;
			break;
		}
	}
	return err;
}






bool 
operator==(const media_format_description & a, const media_format_description & b)
{
	if (b.family != a.family) return false;
	switch (b.family) {
	case B_BEOS_FORMAT_FAMILY:
		return b.u.beos.format == a.u.beos.format;
		break;
	case B_QUICKTIME_FORMAT_FAMILY:
		return (b.u.quicktime.codec == a.u.quicktime.codec) && 
			(b.u.quicktime.vendor == a.u.quicktime.vendor);
		break;
	case B_AVI_FORMAT_FAMILY:
		return b.u.avi.codec == a.u.avi.codec;
		break;
	case B_ASF_FORMAT_FAMILY:
		return b.u.asf.guid == a.u.asf.guid;
		break;
	case B_MPEG_FORMAT_FAMILY:
		return b.u.mpeg.id == a.u.mpeg.id;
		break;
	case B_WAV_FORMAT_FAMILY:
		return b.u.wav.codec == a.u.wav.codec;
		break;
	case B_AIFF_FORMAT_FAMILY:
		return b.u.aiff.codec == a.u.aiff.codec;
		break;
	case B_MISC_FORMAT_FAMILY:
		return (b.u.misc.file_format == a.u.misc.file_format) && 
			(b.u.misc.codec == a.u.misc.codec);
		break;
	default:
		/* whatever */
		return !memcmp(&a, &b, sizeof(a));
	}
	return true;
}

bool
operator<(const media_format_description & a, const media_format_description & b)
{
	if (a.family < b.family) return true;
	if (a.family > b.family) return false;
	switch (b.family) {
	case B_BEOS_FORMAT_FAMILY:
		return b.u.beos.format > a.u.beos.format;
		break;
	case B_QUICKTIME_FORMAT_FAMILY:
		return (b.u.quicktime.codec > a.u.quicktime.codec) || 
			((b.u.quicktime.codec == a.u.quicktime.codec) && 
			(b.u.quicktime.vendor > a.u.quicktime.vendor));
		break;
	case B_AVI_FORMAT_FAMILY:
		return b.u.avi.codec > a.u.avi.codec;
		break;
	case B_ASF_FORMAT_FAMILY:
		return a.u.asf.guid < b.u.asf.guid;
		break;
	case B_MPEG_FORMAT_FAMILY:
		return a.u.mpeg.id < b.u.mpeg.id;
		break;
	case B_WAV_FORMAT_FAMILY:
		return b.u.wav.codec > a.u.wav.codec;
		break;
	case B_AIFF_FORMAT_FAMILY:
		return b.u.aiff.codec > a.u.aiff.codec;
		break;
	case B_MISC_FORMAT_FAMILY:
		return (b.u.misc.file_format > a.u.misc.file_format) || 
			((b.u.misc.file_format == a.u.misc.file_format) && 
			(b.u.misc.codec > a.u.misc.codec));
		break;
	default:
		/* whatever */
		return memcmp(&a, &b, sizeof(a)) < 0;
	}
	return false;
}


_media_format_description::_media_format_description()
{
	memset(this, 0, sizeof(*this));
}

_media_format_description::~_media_format_description()
{
}

_media_format_description::_media_format_description(const _media_format_description & other)
{
	memcpy(this, &other, sizeof(*this));
}

_media_format_description & _media_format_description::operator=(const _media_format_description & other)
{
	memcpy(this, &other, sizeof(*this));
	return *this;
}



// media_format comparison functions

#define CMP(f)	(a.f == b.f)

bool
operator==(const media_raw_audio_format & a, const media_raw_audio_format & b)
{
	return
		CMP(frame_rate) &&
		CMP(channel_count) &&
		CMP(format) &&
		CMP(byte_order) &&
		CMP(buffer_size);
}

bool
operator==(const media_multi_audio_info & a, const media_multi_audio_info & b)
{
	return
		CMP(channel_mask) &&
		CMP(valid_bits) &&
		CMP(matrix_mask);
}

bool
operator==(const media_multi_audio_format & a, const media_multi_audio_format & b)
{
	return
		(const media_raw_audio_format&)a == (const media_raw_audio_format&)b &&
		(const media_multi_audio_info&)a == (const media_multi_audio_info&)b;
}

bool
operator==(const media_encoded_audio_format & a, const media_encoded_audio_format & b)
{
	return
		CMP(output) &&
		CMP(encoding) &&
		CMP(bit_rate) &&
		CMP(frame_size) &&
		CMP(multi_info);
}

bool
operator==(const media_video_display_info & a, const media_video_display_info & b)
{
	return
		CMP(format) &&
		CMP(line_width) &&
		CMP(line_count) &&
		CMP(bytes_per_row) &&
		CMP(pixel_offset) &&
		CMP(line_offset) &&
		CMP(flags);
}

bool
operator==(const media_raw_video_format & a, const media_raw_video_format & b)
{
	return
		CMP(field_rate) &&
		CMP(interlace) &&
		CMP(first_active) &&
		CMP(last_active) &&
		CMP(orientation) &&
		CMP(pixel_width_aspect) &&
		CMP(pixel_height_aspect) &&
		CMP(display);
}

bool
operator==(const media_encoded_video_format & a, const media_encoded_video_format & b)
{
	return
		CMP(output) &&
		CMP(avg_bit_rate) &&
		CMP(max_bit_rate) &&
		CMP(encoding) &&
		CMP(frame_size) &&
		CMP(forward_history) &&
		CMP(backward_history);
}

bool
operator==(const media_multistream_format::vid_info & a, const media_multistream_format::vid_info & b)
{
	return
		CMP(frame_rate) &&
		CMP(width) &&
		CMP(height) &&
		CMP(space) &&
		CMP(sampling_rate) &&
		CMP(sample_format) &&
		CMP(byte_order) &&
		CMP(channel_count);
}

bool
operator==(const media_multistream_format::avi_info & a, const media_multistream_format::avi_info & b)
{
	if(
		CMP(us_per_frame) &&
		CMP(width) &&
		CMP(height) &&
		CMP(type_count)
	)
	{
		size_t size;
		size = a.type_count < sizeof(a.types)/sizeof(a.types[0]) ? a.type_count*sizeof(a.types[0]) : sizeof(a.types);
		return !memcmp(a.types, b.types, size);
	}
	return false;
}

bool
operator==(const media_multistream_format & a, const media_multistream_format & b)
{
	if(
		CMP(avg_bit_rate) &&
		CMP(max_bit_rate) &&
		CMP(avg_chunk_size) &&
		CMP(max_chunk_size) &&
		CMP(flags) &&
		CMP(format)
	)
	{
// sez Jon, we don't use these
#if 0
#warning *** FULL MULTISTREAM FORMAT COMPARE ***
		switch(a.format) {
		case media_multistream_format::B_VID:
			return CMP(u.vid);
		case media_multistream_format::B_AVI:
			return CMP(u.avi);
		
		case media_multistream_format::B_ANY:
		case media_multistream_format::B_MPEG1:
		case media_multistream_format::B_MPEG2:
		case media_multistream_format::B_QUICKTIME:
		default:
			break;
		}
#endif
		return true;
	}
	return false;
}

#undef CMP

bool
operator==(const media_format & format1, const media_format & format2)
{
	const void *meta1, *meta2;
	int32 size1, size2;

	if (format1.type != format2.type ||
		format1.require_flags != format2.require_flags ||
		format1.deny_flags != format2.deny_flags)
	{
		return false;
	}
	
	meta1 = format1.MetaData();
	size1 = format1.MetaDataSize();
	meta2 = format2.MetaData();
	size2 = format2.MetaDataSize();
	if (size1 != size2 || size1 && ( (meta1 == NULL || meta2 == NULL) || memcmp(meta1,meta2,size1)))
		return false;

	switch (format2.type) {
		case B_MEDIA_RAW_AUDIO:
			return format1.u.raw_audio == format2.u.raw_audio;

		case B_MEDIA_RAW_VIDEO:
			return format1.u.raw_video == format2.u.raw_video;
		
		case B_MEDIA_MULTISTREAM:
			return format1.u.multistream == format2.u.multistream;

		case B_MEDIA_ENCODED_AUDIO:
			return format1.u.encoded_audio == format2.u.encoded_audio;

		case B_MEDIA_ENCODED_VIDEO:
			return format1.u.encoded_video == format2.u.encoded_video;
			
		default:
			//? is this a reasonable default case?
			return !memcmp(&format1.u, &format2.u, sizeof(format1.u));
	};
};


// GUID functionality pending

bool operator==(const GUID & a, const GUID & b)
{
	return !memcmp(&a, &b, sizeof(GUID));
}

bool operator<(const GUID & a, const GUID & b)
{
	return memcmp(&a, &b, sizeof(GUID)) < 0;
}
