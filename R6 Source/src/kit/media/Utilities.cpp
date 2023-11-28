/*	Utilities.cpp	*/


#include "trinity_p.h"
#include "tr_debug.h"

#include <priv_syscalls.h>
#include <string.h>
#include <image.h>
#include <parsedate.h>
#include <MediaRoster.h>



const char _trinity_name[] = "libmedia.so";
const char _trinity_date[] = __DATE__ " " __TIME__;



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
		sprintf(buf, "raw_audio;%g;%ld;0x%lx;%ld;0x%lx", f.u.raw_audio.frame_rate, f.u.raw_audio.channel_count, 
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
		sprintf(buf, "caudio;%s;%g;%ld;(%g;%ld;0x%lx;%ld;0x%lx)", mstr, f.u.encoded_audio.bit_rate, 
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
		sprintf(buf, "cvideo;%s;%g;%g;%ld;(%g;0x%x;%ld;%ld;%ld;%ld;%s;%d;%d)", cstr, 
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
	dassert(strlen(buf) < size);	/* make sure we don't overwrite */
	return ok;
}

status_t 
media_realtime_init_image(
	image_id image,
	uint32 flags)
{
	image_info imginfo;
	status_t err = get_image_info(image, &imginfo);
	if (err != B_OK)
		return err;
		
	uint32 system_flags;
	err = BMediaRoster::Roster()->GetRealtimeFlags(&system_flags);
	if (err != B_OK)
		return err;
	
	if (((flags & B_MEDIA_REALTIME_AUDIO) && (system_flags & B_MEDIA_REALTIME_AUDIO)) ||
		((flags & B_MEDIA_REALTIME_VIDEO) && (system_flags & B_MEDIA_REALTIME_VIDEO))) {
		// Forgive me now for this sin I am about to commit...
		
//		printf("locking memory for image %s", imginfo.name);
		err = _klock_memory_(imginfo.text, imginfo.text_size, 0);
		if (err != B_OK) {
//			printf("Error %s occured while locking memory\n",
//				strerror(err));
			return B_MEDIA_REALTIME_UNAVAILABLE;
		}

//		printf("Locked text\n");
		
		err = _klock_memory_(imginfo.data, imginfo.data_size, 0);
		if (err != B_OK) {
//			printf("Error %s occured while locking memory\n",
//				strerror(err));
			return B_MEDIA_REALTIME_UNAVAILABLE;
		}
			
//		printf("Locked data\n");
	} else {
//		printf("Ignore request to lock image areas\n");
		return B_MEDIA_REALTIME_DISABLED;
	}
	
	return B_OK;
}


status_t 
media_realtime_init_thread(
	thread_id thread,
	size_t stack_used,
	uint32 flags)
{
	thread_info thinfo;
	status_t err = get_thread_info(thread, &thinfo);
	if (err != B_OK)
		return err;
		
	uint32 system_flags;
	err = BMediaRoster::Roster()->GetRealtimeFlags(&system_flags);
	if (err != B_OK)
		return err;
	
	if (((flags & B_MEDIA_REALTIME_AUDIO) && (system_flags & B_MEDIA_REALTIME_AUDIO)) ||
		((flags & B_MEDIA_REALTIME_VIDEO) && (system_flags & B_MEDIA_REALTIME_VIDEO))) {
	
//		printf("Locking stack of thread %s\n", thinfo.name);
		
		uint8 *locked_stack_top;
		area_id ar = area_for(thinfo.stack_base);
		if (ar < B_OK) {
//			printf("Couldn't find area for stack: this should happen\n");
			return B_MEDIA_REALTIME_UNAVAILABLE;
		}
		
		
		area_info ainfo;
		if (get_area_info(ar, &ainfo) < B_OK) {
//			printf("Couldn't get info for stack area\n");
			return B_MEDIA_REALTIME_UNAVAILABLE;
		}

//		printf("stack base %p.  Stack end %p.  Stack area start: %p stack area size %x\n",
//			thinfo.stack_base, thinfo.stack_end, ainfo.address, ainfo.size);
				
		locked_stack_top = (uint8*)((uint32) thinfo.stack_end - (uint32)stack_used);
		if ((char*) locked_stack_top < (char*) thinfo.stack_base) {
//			printf("Invalid locked stack size: %p\n", stack_used);
			return B_BAD_VALUE;
		}

//		printf("_kmap_pages_(%p, %x)\n", locked_stack_top, stack_used);
		size_t round = (size_t) locked_stack_top & (B_PAGE_SIZE - 1);
		err = _kmap_pages_(locked_stack_top - round, (stack_used + round + B_PAGE_SIZE - 1)
			& ~(B_PAGE_SIZE - 1));
		if (err != B_OK) {
//			printf("Error %s commiting pages\n", strerror(err));
			return B_MEDIA_REALTIME_UNAVAILABLE;
		}
		
//		printf("_klock_memory_(%p, %x)\n", locked_stack_top, stack_used);
		err = _klock_memory_(locked_stack_top, stack_used, 0);
		if (err != B_OK) {
//			printf("Error locking memory\n");
			return B_MEDIA_REALTIME_UNAVAILABLE;
		}
		
//		printf("Locked.\n");
	} else {
//		printf("Ignore request to lock thread stack\n");
		return B_MEDIA_REALTIME_DISABLED;
	}
	
	return B_OK;
}
