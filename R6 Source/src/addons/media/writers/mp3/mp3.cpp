#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <DataIO.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaWriter.h"
#include "mp3.h"

using namespace BPrivate;

//#define DEBUG	printf
#define DEBUG	if (0) printf


static media_format mp3_format;

class make_format {
public:
	make_format();
};

make_format format_maker;

make_format::make_format()
{
	BMediaFormats fmts;
	media_format_description desc;
	memset(&desc, 0, sizeof(desc));
	desc.family = B_MPEG_FORMAT_FAMILY;
	desc.u.mpeg.id = B_MPEG_1_AUDIO_LAYER_3;
	mp3_format.type = B_MEDIA_ENCODED_AUDIO;
	status_t err = fmts.MakeFormatFor(&desc, 1, &mp3_format);
}

MediaWriter *instantiate_mediawriter(void)
{
	return new MP3Writer();
}

status_t get_mediawriter_info(media_file_format *mfi)
{
	strcpy(mfi->mime_type,      "audio/x-mpeg");
	strcpy(mfi->pretty_name,    "MP3 Audio File Format");
	strcpy(mfi->short_name,     "mp3");
	strcpy(mfi->file_extension, "mp3");

	mfi->capabilities = media_file_format::B_KNOWS_ENCODED_AUDIO |
		                media_file_format::B_WRITABLE;
	mfi->family       = B_MPEG_FORMAT_FAMILY;

	return B_OK;
}

status_t accepts_format(media_format *fmt, uint32 flags)
{
	if (fmt->type != B_MEDIA_ENCODED_AUDIO)
		return B_MEDIA_BAD_FORMAT;
	if (fmt->u.encoded_audio.encoding != mp3_format.u.encoded_audio.encoding)
		return B_MEDIA_BAD_FORMAT;
	return B_OK;
}


MP3Writer::MP3Writer()
{
	fDestination = NULL;
	fTrack       = NULL;
}

MP3Writer::~MP3Writer()
{
}


status_t MP3Writer::SetSource(BDataIO *dest)
{
	if(fDestination)
		return B_NOT_ALLOWED;
	
	fDestination = dest;
	return B_OK;
}
	
status_t MP3Writer::AddTrack(BMediaTrack *track)
{
	status_t					err;
	media_format				mf;
	BMediaFormats				formatObject;
	media_format_description	fd;

	/* Check if we didn't alreday add a audio track */
	DEBUG("#### AddTrack In\n");
	if (fTrack != NULL)
		return B_BAD_INDEX;

	/* Check if it's an encoded format that we know how to support */
	track->EncodedFormat(&mf);

	err = accepts_format(&mf, B_MEDIA_REJECT_WILDCARDS);
	if(err != B_OK) {
		return err;
	}

	return B_OK;
}

status_t MP3Writer::AddChunk(int32 type, const char *data, size_t size)
{
	return B_ERROR;
}

status_t MP3Writer::AddCopyright(const char *data)
{
	return B_OK;
}

status_t MP3Writer::AddTrackInfo(int32 track, uint32 code, const char *data,
								  size_t size)
{
	return B_OK;
}

status_t MP3Writer::CommitHeader(void)
{
	return B_OK;
}
	

status_t MP3Writer::WriteData(	int32 			tracknum,
							 	media_type 		type,
					 			const void 		*data,
								size_t 			size,
					 			media_encode_info *mei)
{
	if (tracknum != 0)
		return B_BAD_INDEX;

	if (!fDestination)
		return B_BAD_VALUE;

	return fDestination->Write(data, size);
}


status_t MP3Writer::CloseFile(void)
{
	return B_OK;
}



