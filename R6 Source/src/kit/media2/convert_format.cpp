#include "convert_format.h"

using B::Support2::BValue;

namespace B {
namespace Media2 {

IValueOutput::ptr
export_format_values(const media_format &format, IValueOutput::arg stream)
{
	stream->Write(BValue(B_FORMATKEY_MEDIA_TYPE, BValue::Int32(format.type)));
	switch (format.type)
	{
		case B_MEDIA_RAW_AUDIO:
		{
			const media_multi_audio_format & f = format.u.raw_audio;
			const media_multi_audio_format & w = media_multi_audio_format::wildcard;
			if (f.frame_rate != w.frame_rate)		stream->Write(BValue(B_FORMATKEY_FRAME_RATE, BValue::Float(f.frame_rate)));
			if (f.channel_count != w.channel_count)	stream->Write(BValue(B_FORMATKEY_CHANNEL_COUNT, BValue::Int32(f.channel_count)));
			if (f.format != w.format)				stream->Write(BValue(B_FORMATKEY_RAW_AUDIO_TYPE, BValue::Int32(f.format)));
			if (f.byte_order != w.byte_order)		stream->Write(BValue(B_FORMATKEY_BYTE_ORDER, BValue::Int32(f.byte_order)));
			if (f.buffer_size != w.buffer_size && f.format != w.format && f.channel_count != w.channel_count)
			{
				stream->Write(BValue(
					B_FORMATKEY_BUFFER_FRAMES,
					BValue::Int32(f.buffer_size / (f.format & 0x0f) * f.channel_count)));
			}

			// +++ media_multi_audio_info::channel_mask
			// +++ media_multi_audio_info::valid_bits
			// +++ media_multi_audio_info::matrix_mask

			break;
		}
		case B_MEDIA_RAW_VIDEO:
		{
			const media_raw_video_format & f = format.u.raw_video;
			// +++
			break;
		}
		case B_MEDIA_ENCODED_AUDIO:
		{
			const media_encoded_audio_format & f = format.u.encoded_audio;
			// +++
			break;
		}
		case B_MEDIA_ENCODED_VIDEO:
		{
			const media_encoded_video_format & f = format.u.encoded_video;
			// +++
			break;
		}
		case B_MEDIA_MULTISTREAM:
		{
			const media_multistream_format & f = format.u.multistream;
			// +++
			break;
		}
		default:
			break;
	}
	return stream;
}

status_t 
import_format_value(const BValue &value, media_format &format)
{
	media_multi_audio_format & f_ra = format.u.raw_audio;
	media_raw_video_format & f_rv = format.u.raw_video;
	media_encoded_audio_format & f_ea = format.u.encoded_audio;
	media_encoded_video_format & f_ev = format.u.encoded_video;
	media_multistream_format & f_m = format.u.multistream;

	BValue k, v;
	void * cookie = 0;
	status_t err = value.GetNextItem(&cookie, &k, &v);
	if (err < B_OK) return err;
	
	if (k == B_FORMATKEY_MEDIA_TYPE)		return v.GetInt32((int32*)&format.type);
	if (k == B_FORMATKEY_RAW_AUDIO_TYPE)	return v.GetInt32((int32*)&f_ra.format);
	if (k == B_FORMATKEY_FRAME_RATE)		return v.GetFloat(&f_ra.frame_rate);
	if (k == B_FORMATKEY_BYTE_ORDER)		return v.GetInt32((int32*)&f_ra.byte_order);
	if (k == B_FORMATKEY_CHANNEL_COUNT)
	{
		int32 channel_count;
		err = v.GetInt32(&channel_count);
		if (err < B_OK) return err;
		if (channel_count && !f_ra.channel_count && f_ra.buffer_size)
		{
			f_ra.buffer_size *= channel_count;
		}
		f_ra.channel_count = channel_count;
		return B_OK;
	}	
	if (k == B_FORMATKEY_BUFFER_FRAMES)
	{
		int32 frames;
		err = v.GetInt32(&frames);
		if (err < B_OK) return err;
		f_ra.buffer_size = (f_ra.channel_count) ?
			frames * f_ra.channel_count :
			frames;
		return B_OK;
	}
	
	return B_BAD_INDEX;
}

} } // B::Media2
