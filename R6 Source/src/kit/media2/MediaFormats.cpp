
// derived from the IAD branch of MediaFormats.cpp

#include <support2/Autolock.h>
#include <string.h>
#include <stdlib.h>
#include <support2/Debug.h>
#include <stdio.h>
#include <math.h>

#include <list>

#include <media2/MediaFormats.h>

#include <support2/StdIO.h>
#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#include "AreaCloneCache.h"
#include "codec_addons.h"

#define DIAGNOSTIC(x) fprintf x
#if !NDEBUG
#define FPRINTF(x) fprintf x
#else
#define FPRINTF(x) (void)0
#endif


namespace B {
namespace Media2 {

using namespace Private;
using namespace Media2Private;
using namespace std;

static bool
audio_compare(
	const media_raw_audio_format & a,
	const media_raw_audio_format & b)
{
	if ((a.frame_rate > media_raw_audio_format::wildcard.frame_rate) && 
		(b.frame_rate > media_raw_audio_format::wildcard.frame_rate) && 
		(fabs(1.0-a.frame_rate/b.frame_rate) > 0.01)) return false;
	if ((a.channel_count > media_raw_audio_format::wildcard.channel_count) && 
		(b.channel_count > media_raw_audio_format::wildcard.channel_count) && 
		(a.channel_count != b.channel_count)) return false;
	if ((a.format > media_raw_audio_format::wildcard.format) && 
		(b.format > media_raw_audio_format::wildcard.format) && 
		(a.format != b.format)) return false;
	if ((a.byte_order > media_raw_audio_format::wildcard.byte_order) && 
		(b.byte_order > media_raw_audio_format::wildcard.byte_order) && 
		(a.byte_order != b.byte_order)) return false;
	if ((a.buffer_size > media_raw_audio_format::wildcard.buffer_size) && 
		(b.buffer_size > media_raw_audio_format::wildcard.buffer_size) && 
		(a.buffer_size != b.buffer_size)) return false;
	return true;
}


static bool
video_compare(
	const media_raw_video_format & a,
	const media_raw_video_format & b)
{
	if ((a.field_rate > media_raw_video_format::wildcard.field_rate) && 
		(b.field_rate > media_raw_video_format::wildcard.field_rate) && 
		(fabs(1.0-b.field_rate/a.field_rate) > 0.015)) return false;	// 1.5% inaccuracy
	if ((a.display.format > media_raw_video_format::wildcard.display.format) && 
		(b.display.format > media_raw_video_format::wildcard.display.format) && 
		(b.display.format != a.display.format)) return false;
	if ((a.interlace > media_raw_video_format::wildcard.interlace) && 
		(b.interlace > media_raw_video_format::wildcard.interlace) && 
		(b.interlace != a.interlace)) return false;
	if ((a.display.line_width > media_raw_video_format::wildcard.display.line_width) && 
		(b.display.line_width > media_raw_video_format::wildcard.display.line_width) && 
		(b.display.line_width != a.display.line_width)) return false;
	if ((a.display.line_count > media_raw_video_format::wildcard.display.line_count) && 
		(b.display.line_count > media_raw_video_format::wildcard.display.line_count) && 
		(b.display.line_count != a.display.line_count)) return false;
	if ((a.first_active > media_raw_video_format::wildcard.first_active) && 
		(b.first_active > media_raw_video_format::wildcard.first_active) && 
		(b.first_active != a.first_active)) return false;
	if ((a.orientation > media_raw_video_format::wildcard.orientation) && 
		(b.orientation > media_raw_video_format::wildcard.orientation) && 
		(b.orientation != a.orientation)) return false;
	if ((a.display.bytes_per_row > media_raw_video_format::wildcard.display.bytes_per_row) && 
		(b.display.bytes_per_row > media_raw_video_format::wildcard.display.bytes_per_row) && 
		(b.display.bytes_per_row != a.display.bytes_per_row)) return false;
	if ((a.pixel_width_aspect > media_raw_video_format::wildcard.pixel_width_aspect) && 
		(b.pixel_width_aspect > media_raw_video_format::wildcard.pixel_width_aspect) && 
		(b.pixel_width_aspect != a.pixel_width_aspect)) return false;
	if ((a.pixel_height_aspect > media_raw_video_format::wildcard.pixel_height_aspect) && 
		(b.pixel_height_aspect > media_raw_video_format::wildcard.pixel_height_aspect) && 
		(b.pixel_height_aspect != a.pixel_height_aspect)) return false;
	return true;
}

bool
format_is_compatible(
	const media_format & a,	/* a is the format you want to feed to something accepting b */
	const media_format & b)
{
	/* wildcards */
	if (b.type == B_MEDIA_UNKNOWN_TYPE) return true;
	if (a.type == B_MEDIA_UNKNOWN_TYPE) return true;
	/* nulls */
	if (a.type == B_MEDIA_NO_TYPE) return b.type == B_MEDIA_NO_TYPE;
	if (a.type != b.type) return false;
	/* actual formats */
	switch (a.type)
	{
	case B_MEDIA_RAW_AUDIO:
		return audio_compare(a.u.raw_audio, b.u.raw_audio);
		break;
	case B_MEDIA_RAW_VIDEO:
		return video_compare(a.u.raw_video, b.u.raw_video);
		break;
	case B_MEDIA_ENCODED_AUDIO:
		if ((a.u.encoded_audio.encoding > media_encoded_audio_format::wildcard.encoding) && 
			(b.u.encoded_audio.encoding > media_encoded_audio_format::wildcard.encoding) && 
			(a.u.encoded_audio.encoding != b.u.encoded_audio.encoding)) return false;
		if ((a.u.encoded_audio.bit_rate > media_encoded_audio_format::wildcard.bit_rate) && 
			(b.u.encoded_audio.bit_rate > media_encoded_audio_format::wildcard.bit_rate) && 
			(a.u.encoded_audio.bit_rate > b.u.encoded_audio.bit_rate)) return false;
		if ((a.u.encoded_audio.frame_size > media_encoded_audio_format::wildcard.frame_size) && 
			(b.u.encoded_audio.frame_size > media_encoded_audio_format::wildcard.frame_size) && 
			(a.u.encoded_audio.frame_size != b.u.encoded_audio.frame_size)) return false;
		return audio_compare(a.u.encoded_audio.output, b.u.encoded_audio.output);
		break;
	case B_MEDIA_ENCODED_VIDEO:
		if ((a.u.encoded_video.max_bit_rate > media_encoded_video_format::wildcard.max_bit_rate) && 
			(b.u.encoded_video.max_bit_rate > media_encoded_video_format::wildcard.max_bit_rate) && 
			(a.u.encoded_video.max_bit_rate > b.u.encoded_video.max_bit_rate)) return false;
		if ((a.u.encoded_video.encoding > media_encoded_video_format::wildcard.encoding) && 
			(b.u.encoded_video.encoding > media_encoded_video_format::wildcard.encoding) && 
			(a.u.encoded_video.encoding != b.u.encoded_video.encoding)) return false;
		if ((a.u.encoded_video.frame_size > media_encoded_video_format::wildcard.frame_size) && 
			(b.u.encoded_video.frame_size > media_encoded_video_format::wildcard.frame_size) && 
			(a.u.encoded_video.frame_size != b.u.encoded_video.frame_size)) return false;
		if ((a.u.encoded_video.forward_history > media_encoded_video_format::wildcard.forward_history) && 
			(b.u.encoded_video.forward_history > media_encoded_video_format::wildcard.forward_history) && 
			(a.u.encoded_video.forward_history > b.u.encoded_video.forward_history)) return false;
		if ((a.u.encoded_video.backward_history > media_encoded_video_format::wildcard.backward_history) && 
			(b.u.encoded_video.backward_history > media_encoded_video_format::wildcard.backward_history) && 
			(a.u.encoded_video.backward_history > b.u.encoded_video.backward_history)) return false;
		return video_compare(a.u.encoded_video.output, b.u.encoded_video.output);
		break;
	case B_MEDIA_MULTISTREAM:
		return (a.u.multistream.format == b.u.multistream.format);
		break;
	default:
		/* we can't compare other formats -- assume they're the same */;
	}
	return true;
}


bool
string_for_format(
	const media_format & f, 
	char * buf, 
	size_t size)
{
	int32 in_size;
	in_size = size;
	if (!buf) return false;
	bool ok = false;
	if (size < 64) { strncpy(buf, "too short", size); return false; }
	*buf = 0;
	switch (f.type)
	{
	case B_MEDIA_RAW_AUDIO: {
		sprintf(buf, "raw_audio;%g;%ld;%lx;%ld;%lx", f.u.raw_audio.frame_rate, f.u.raw_audio.channel_count, 
			f.u.raw_audio.format, f.u.raw_audio.byte_order, f.u.raw_audio.buffer_size);
		ok = true;
		} break;
	case B_MEDIA_RAW_VIDEO: {
		char dig[12];
		const char * ostr = dig;
		if (f.u.raw_video.orientation < B_VIDEO_TOP_LEFT_RIGHT || f.u.raw_video.orientation > B_VIDEO_BOTTOM_LEFT_RIGHT) {
			sprintf(dig, "%ld", f.u.raw_video.orientation);
		}
		else {
			const char * o[] = { "TopLR", "BotLR" };
			ostr = o[f.u.raw_video.orientation-1];
		}
		sprintf(buf, "raw_video;%g;%ld;%ld;%ld;%s;%d;%d;%d;%ld;%ld;%ld;%ld;%ld",
			f.u.raw_video.field_rate, f.u.raw_video.interlace,
			f.u.raw_video.first_active, f.u.raw_video.last_active, ostr,
			f.u.raw_video.pixel_width_aspect, f.u.raw_video.pixel_height_aspect,
			f.u.raw_video.display.format, f.u.raw_video.display.line_width,
			f.u.raw_video.display.line_count,
			f.u.raw_video.display.bytes_per_row,
			f.u.raw_video.display.pixel_offset,
			f.u.raw_video.display.line_offset);
		ok = true;
		} break;
	case B_MEDIA_ENCODED_AUDIO: {
		char mth[12];
		const char * mstr = mth;
		sprintf(mth, "%d", f.u.encoded_audio.encoding);
		sprintf(buf, "caudio;%s;%g;%ld;(%g;%ld;%lx;%ld;%lx)", mstr, f.u.encoded_audio.bit_rate, 
			f.u.encoded_audio.frame_size, f.u.encoded_audio.output.frame_rate, 
			f.u.encoded_audio.output.channel_count, f.u.encoded_audio.output.format, 
			f.u.encoded_audio.output.byte_order, f.u.encoded_audio.output.buffer_size);
		ok = true;
		} break;
	case B_MEDIA_ENCODED_VIDEO: {
		char dig[12];
		const char * cstr = dig;
		sprintf(dig, "%d", f.u.encoded_video.encoding);
		char ddig[12];
		const char * ostr = ddig;
		if (f.u.encoded_video.output.orientation < B_VIDEO_TOP_LEFT_RIGHT || f.u.encoded_video.output.orientation > B_VIDEO_BOTTOM_LEFT_RIGHT) {
			sprintf(ddig, "%ld", f.u.raw_video.orientation);
		}
		else {
			const char * o[] = { "TopLR", "BotLR" };
			ostr = o[f.u.encoded_video.output.orientation-1];
		}
		sprintf(buf, "cvideo;%s;%g;%g;%ld;(%g;%x;%ld;%ld;%ld;%ld;%s;%d;%d)", cstr, 
			f.u.encoded_video.max_bit_rate, f.u.encoded_video.avg_bit_rate, 
			f.u.encoded_video.frame_size, f.u.encoded_video.output.field_rate, 
			f.u.encoded_video.output.display.format, f.u.encoded_video.output.interlace, 
			f.u.encoded_video.output.display.line_width, f.u.encoded_video.output.display.line_count, 
			f.u.encoded_video.output.first_active, ostr, 
			f.u.encoded_video.output.pixel_width_aspect, f.u.encoded_video.output.pixel_height_aspect);
		ok = true;
		} break;
	default: {
		sprintf(buf, "%d-", f.type);
		uchar * ptr = (uchar *)&f.u;
		uchar * end = ptr+sizeof(f.u);
		size -= strlen(buf);
		buf += strlen(buf);
		while (ptr < end) {
			if (size > 2) {
				char fmt[3];
				sprintf(fmt, "%02x", *ptr);
				strcat(buf, fmt);
				size -= 2;
				buf += 2;
			}
			ptr++;
		}
		} break;
	}
	ASSERT(strlen(buf) < (size_t)in_size);	/* make sure we don't overwrite */
	return ok;
}

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
			break;
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
			break;
	};
	return B_OK;
};

//	name mangling is a compiler feature, not a platform feature
// Todo: determine of this is necessary for non backward compatible builds.
#if __GNUC__ < 3
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

// Todo: determine of this is necessary for non backward compatible builds.
extern "C" {
#if __GNUC__ < 3
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

// Todo: determine of this is necessary for non backward compatible builds.
extern "C" {
#if __GNUC__ < 3
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
			AreaCloneCache::Instance()->Release(meta_data_area);
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

	use_area = AreaCloneCache::Instance()->Clone(meta_data_area);
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

	f->use_area = AreaCloneCache::Instance()->Clone(meta_data_area, &f->meta_data);
	if (f->use_area < 0) return NULL;

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
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		if ((ti.team == team) && (thisPtr == this)) {
			AreaCloneCache::Instance()->Release(meta_data_area);
		}
	};
}

media_format &
media_format::operator=(const media_format &clone)
{
	if ((meta_data_area != B_BAD_VALUE) && meta_data_area) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		if ((ti.team == team) && (thisPtr == this)) {
			AreaCloneCache::Instance()->Release(meta_data_area);
		}
	}
	memcpy(this,&clone,sizeof(media_format));
	if ((meta_data_area != B_BAD_VALUE) && meta_data_area) {
		thread_info ti;
		get_thread_info(find_thread(NULL),&ti);
		use_area = AreaCloneCache::Instance()->Clone(meta_data_area, &meta_data);
		thisPtr = this;
		team = ti.team;
	}
	return *this;
}

} // Media2

namespace Private {
using namespace Media2;

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
		void SetPriority(
				int32 prio)
			{
				m_priority = prio;
			}
		void SetFormat(
				const media_format * in_format)
			{
				m_format = *in_format;
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
} // Private

namespace Media2 {

using namespace Private;

//	All BMediaFormats objects share a list.
//	Although creating a BMediaFormats will still re-read the list
//	to get new data from the server, this allows us to update the
//	list for everybody when someone registers new stuff.
//	The locking semantics for RewindFormats()/GetNextFormat() are
//	such that there will be no inconsistencies -- you have to hold
//	the lock when calling these functions.
BNestedLocker BMediaFormats::s_lock("BMediaFormats");
BVector<_MediaFormat*> BMediaFormats::s_formats;
int32 s_formatID = 10000;
int32 s_priority = LONG_MAX;
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
	return B_OK;
}

void
BMediaFormats::ex_clear_formats_imp()
{
	clear_formats_imp();
}

//	The theory behind this function is that someone may
//	register a combination of formats that have previously
//	been registered separately. They may also define a new
//	master media_description for the format. We thus need
//	to merge the state about these descriptions.
//	There's also the off chance that someone tries to re-register
//	an audio format as video or vice versa, which would be bad.
status_t
BMediaFormats::merge_formats(
	void *,
	int index,
	const media_format_description * descs,
	int desc_count,
	uint32 flags,
	media_format * io_format)
{
	if (flags & B_EXCLUSIVE) {
		return B_MEDIA_DUPLICATE_FORMAT;
	}
	int32 encoding = 0;
	bool all_same = true;
	list<_MediaFormat *> found;
	for (size_t ix=index; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * mf = s_formats.ItemAt(ix);
		for (int iz=0; iz<desc_count; iz++) {
			if (mf->RecognizeDescription(descs[iz])) {
				media_format that;
				mf->GetFormat(&that);
				int32 tEnc = that.Encoding();
				if (that.type != io_format->type) {
					return B_MISMATCHED_VALUES;
				}
				if (encoding && (encoding != tEnc)) {
					if (flags & BMediaFormats::B_NO_MERGE) {
						return B_MISMATCHED_VALUES;
					}
					else {
						all_same = false;
					}
				}
				encoding = tEnc;
				found.push_back(mf);
				break;
			}
		}
	}
	ASSERT(found.size() > 0);
	if (found.size() == 1) {
		if (desc_count > 1) {
			goto force_registration;
		}
		if (flags & BMediaFormats::B_SET_DEFAULT) {
			(*found.begin())->SetPriority(--s_priority);
		}
		(*found.begin())->GetFormat(io_format);
		return B_OK;
	}
force_registration:
	if (!all_same) {
		if (io_format->Encoding() != 0) {
			encoding = io_format->Encoding();
		}
		else {
			encoding = s_formatID++;	//	mint a totally new format to avoid confusion with others
		}
	}
	switch (io_format->type) {
	case B_MEDIA_ENCODED_AUDIO:
		io_format->u.encoded_audio.encoding = (media_encoded_audio_format::audio_encoding)encoding;
		break;
	case B_MEDIA_ENCODED_VIDEO:
		io_format->u.encoded_video.encoding = (media_encoded_video_format::video_encoding)encoding;
		break;
	case B_MEDIA_MULTISTREAM:
		io_format->u.multistream.format = encoding;
		break;
	default:
		/* no encoding */;
	}
	for (list<_MediaFormat *>::iterator ptr(found.begin()); ptr != found.end(); ptr++) {
		(*ptr)->SetFormat(io_format);
	}
	if (flags & BMediaFormats::B_SET_DEFAULT) {
		s_priority -= desc_count;
	}
	for (int ix=0; ix<desc_count; ix++) {
		bool gotit = false;
		for (list<_MediaFormat *>::iterator ptr(found.begin()); ptr != found.end(); ptr++) {
			if ((*ptr)->RecognizeDescription(descs[ix])) {
				gotit = true;
				(*ptr)->SetPriority(s_priority+ix);
				break;
			}
		}
		if (!gotit) {
			_MediaFormat * nmf = new _MediaFormat(*io_format, descs[ix], s_priority+ix);
			s_formats.AddItem(nmf);
		}
	}
	return B_OK;	//	whew!
}

status_t 
BMediaFormats::MakeFormatFor(
	const media_format_description *descs,
	int32 desc_count,
	media_format *io_format,
	uint32 flags,
	void *)
{
	if (!descs) return B_BAD_VALUE;
	if (desc_count < 1) return B_BAD_VALUE;
	if (!io_format) return B_BAD_VALUE;
#if _SUPPORTS_MEDIA_FORMATS
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
		BAutolock lock(s_lock.Lock());
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
#else
	BAutolock lock(s_lock.Lock());
	// look through local list for format
	for (size_t ix=0; ix<s_formats.CountItems(); ix++) {
		bool isit = false;
		_MediaFormat * mf = s_formats.ItemAt(ix);
		for (int iy=0; iy<desc_count; iy++) {
			if (mf->RecognizeDescription(descs[iy])) {
				isit = true;
				break;
			}
		}
		if (isit) {
			return merge_formats(mf, ix, descs, desc_count, flags, io_format);
		}
	}
	// if no match, mint new format
	if (io_format->type == B_MEDIA_ENCODED_AUDIO) {
		if (io_format->u.encoded_audio.encoding == 0) {
			io_format->u.encoded_audio.encoding = (media_encoded_audio_format::audio_encoding)s_formatID++;
		}
	}
	else if (io_format->type == B_MEDIA_ENCODED_VIDEO) {
		if (io_format->u.encoded_video.encoding == 0) {
			io_format->u.encoded_video.encoding = (media_encoded_video_format::video_encoding)s_formatID++;
		}
	}
	else if (io_format->type == B_MEDIA_MULTISTREAM) {
		if (io_format->u.multistream.format == 0) {
			io_format->u.multistream.format = s_formatID++;
		}
	}
	s_priority -= desc_count;
	for (int iy=0; iy<desc_count; iy++) {
		_MediaFormat * nmf = new _MediaFormat(*io_format, descs[iy], s_priority+iy);
		s_formats.AddItem(nmf);
	}
	return B_OK;
#endif
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
	BAutolock lock(s_lock.Lock());
	if (s_cleared) {
		err = get_formats();
	}
	if (err < 0) return err;
	_MediaFormat * m_that = 0;
	for (size_t ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * cur = s_formats.ItemAt(ix);
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
	BAutolock lock(s_lock.Lock());
	if (s_cleared) {
		err = get_formats();
	}
	if (err < 0) return err;
	_MediaFormat * that = 0;
	for (size_t ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * cur = s_formats.ItemAt(ix);
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
//	//	make sure it's called with the lock held
//	if (!s_lock.IsLocked()) return EPERM;
	//	get_formats() will make sure WE hold the lock
	return get_formats();
}

status_t
BMediaFormats::GetNextFormat(
	media_format * out_format,
	media_format_description * out_description)
{
//	//	make sure it's called with the lock held
//	if (!s_lock.IsLocked()) {
//		return EPERM;
//s	}
	status_t err = B_OK;
	BAutolock lock(s_lock.Lock());	//	make sure WE hold the lock
	if (s_cleared) {
		err = get_formats();
	}
	if (err < 0) return err;
	if (m_index >= (int32)s_formats.CountItems()) {
		return B_BAD_INDEX;
	}
	int ix = m_index;
	m_index++;
	s_formats.ItemAt(ix)->GetFormat(out_format);
	s_formats.ItemAt(ix)->GetDescription(out_description);
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
//	if (!s_lock.IsLocked()) return;
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
	BAutolock lock(s_lock.Lock());
#if _SUPPORTS_MEDIA_FORMATS
	//puts("clear_formats_imp()");
	for (int ix=0; ix<s_formats.CountItems(); ix++) {
		delete s_formats.ItemAt(ix);
	}
	s_formats.MakeEmpty();
	s_cleared = 1;
#endif
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
	BAutolock lock(s_lock.Lock());
	// did we already load/scan ?
	if (s_cleared) {
		// if not, scan decoders

		//	first clear out old formats (typically registered by an extractor in BMediaFile)
		FPRINTF((stderr, "Clearing old formats\n"));
		for (size_t ix=0; ix<s_formats.CountItems(); ix++) {
			delete s_formats.ItemAt(ix);
		}
		s_formats.MakeEmpty();

		_AddonManager * decoders = __get_decoder_manager();
		if(decoders) decoders->InitAddons();

		_AddonManager * encoders = __get_encoder_manager();
		if(encoders) encoders->InitAddons();

		_AddonManager * extractors = __get_extractor_manager();
		if(extractors) extractors->InitAddons();

		s_cleared = 0;
	}
	else {
		FPRINTF((stderr, "Formats already registered\n"));
	}
	return B_OK;
}

bool
BMediaFormats::is_bound(const char * addon, const media_format * formats, int32 count)
{
	BAutolock lock(s_lock.Lock());
	int32 found = 0;
	for (size_t ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * mf = s_formats.ItemAt(ix);
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
#if _SUPPORTS_MEDIA_FORMATS
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
#else
	BAutolock lock(s_lock.Lock());
	// find format
	status_t err = B_MEDIA_BAD_FORMAT;
	for (size_t ix=0; ix<s_formats.CountItems(); ix++) {
		_MediaFormat * mf = s_formats.ItemAt(ix);
		for (int iy=0; iy<count; iy++) {
			if (mf->RecognizeFormat(formats[iy])) {
				// update addon list for format
				bool gotit = false;
				for (int iz=0; iz<mf->m_addons.size(); iz++) {
					if (!strcmp(addon, mf->m_addons[iz])) {
						gotit = true;
						break;	//	 A
					}
				}
		//	A here
				if (!gotit) {
					mf->m_addons.push_back(addon);
				}
				break;	//	B
			}
		}
//	B here
	}
	return err;
#endif
}

status_t 
BMediaFormats::find_addons(const media_format *format, addon_list &addons)
{
	BAutolock lock(s_lock.Lock());
	status_t err = B_OK;
	if (s_cleared) err = get_formats_imp();
	if (err < B_OK) return err;
	err = B_MEDIA_BAD_FORMAT;
	int cnt = s_formats.CountItems();
	for (int ix=0; ix<cnt; ix++) {
		_MediaFormat * fmt = s_formats.ItemAt(ix);
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







// GUID functionality pending

bool operator==(const GUID & a, const GUID & b)
{
	return !memcmp(&a, &b, sizeof(GUID));
}

bool operator<(const GUID & a, const GUID & b)
{
	return memcmp(&a, &b, sizeof(GUID)) < 0;
}

} } // B::Media2
