#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include <OS.h>
#include <image.h>

#include <Errors.h>
#include <Locker.h>
#include <Autolock.h>

#include <Entry.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>

#include <MediaFormats.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "Extractor.h"
#include "Detractor.h"
#include "Decoder.h"
#include "MediaWriter.h"
#include "Encoder.h"
#include "addons.h"

using namespace BPrivate;

//#define FUNCTION(x) printf x
#define FUNCTION(X)

//---------------------------------------------------------------


media_encode_info::media_encode_info()
{
	flags = 0;
	start_time = 0;
	time_to_encode = B_INFINITE_TIMEOUT;
	used_data_size = 0;
	file_format_data = NULL;
	file_format_data_size = 0;
	codec_data = NULL;
	codec_data_size = 0;
}

media_decode_info::media_decode_info()
{
	time_to_decode = B_INFINITE_TIMEOUT;
	file_format_data = NULL;
	file_format_data_size = 0;
	codec_data = NULL;
	codec_data_size = 0;
}



status_t
get_next_file_format(int32 *cookie, media_file_format *mfi)
{
	_AddonManager *mgr = __get_writer_manager();
	image_id       imgid;
	status_t     (*infofunc)(media_file_format *mfi);
	int32          id;

	if (!cookie) return B_BAD_VALUE;

	if (*cookie < 0)
		return B_BAD_INDEX;

	while (1) {
		imgid = mgr->GetNextAddon(cookie, &id);
		if (imgid <= 0)            // hmmm, then there's probably no more
			return B_BAD_INDEX;
	
		image_info imginfo;
		if (get_image_info(imgid, &imginfo) < 0) {
			mgr->ReleaseAddon(id);
			continue;
		}

		if (get_image_symbol(imgid, "get_mediawriter_info",
							 B_SYMBOL_TYPE_TEXT, (void **)&infofunc) == B_OK) {


			memset(mfi, 0, sizeof(*mfi));
			infofunc(mfi);                  // the addon fills it in 

			struct stat st;
			stat(imginfo.name, &st);

			mfi->id.node        = st.st_ino;
			mfi->id.device      = st.st_dev;
			mfi->id.internal_id = id;
			mgr->ReleaseAddon(id);
			break;
		}

		mgr->ReleaseAddon(id);
	}
	
	return B_OK;
}

bool 
does_file_accept_format(const media_file_format *mfi, media_format *format, uint32 flags)
{
	_AddonManager	*writer_mgr  = __get_writer_manager();
	image_id		writer_imgid;
	int32			(*accept_func)(const media_format *mf, uint32 flags);
	status_t		err;

	writer_imgid = writer_mgr->GetAddonAt(mfi->id.internal_id);
	if(writer_imgid < 0)
		return writer_imgid;
	err = get_image_symbol(writer_imgid, "accepts_format", B_SYMBOL_TYPE_TEXT,
	                       (void **)&accept_func);
	if(err == B_NO_ERROR)
		err = accept_func(format, flags);
	writer_mgr->ReleaseAddon(mfi->id.internal_id);
	
	return err == B_OK;
}

bool
does_file_accept_format(const media_file_format *mfi, const media_format *format)
{
	return does_file_accept_format(mfi, const_cast<media_format*>(format), 0);
}

status_t 
get_next_encoder(int32 *cookie,
                 const media_file_format *mfi,
                 const media_format *input_format,
                 const media_format *output_format,
                 media_codec_info *mci,
                 media_format *accepted_input_format,
                 media_format *accepted_output_format)
{
	_AddonManager *encoder_mgr = __get_encoder_manager();
	image_id       imgid;
	Encoder     *(*func)(void);
	Encoder     *(*nth_func)(int32);
	Encoder       *enc;
	int32          enc_id;
	int32          enc_sub_id;
	int32          addon_cookie;

	if (*cookie < 0)
		return B_BAD_INDEX;

	while (1) {
		addon_cookie = *cookie & 0xffff;
		imgid = encoder_mgr->GetNextAddon(&addon_cookie, &enc_id);
		if (imgid <= 0)            // hmmm, then there's probably no more
			return B_BAD_INDEX;
		
		enc_sub_id = *cookie >> 16;
		if(enc_sub_id == 0) {
			if (get_image_symbol(imgid, "instantiate_encoder",
								 B_SYMBOL_TYPE_TEXT, (void **)&func) != B_OK) {
	
				encoder_mgr->ReleaseAddon(enc_id);
				*cookie += 0x10000;
				continue;
			}
			*cookie = addon_cookie;
			enc = func(); 
			if (enc == NULL) {
				encoder_mgr->ReleaseAddon(enc_id);
				continue;
			}
		}
		else {
			*cookie += 0x10000;
			if (get_image_symbol(imgid, "instantiate_nth_encoder",
								 B_SYMBOL_TYPE_TEXT, (void **)&nth_func) != B_OK) {
				encoder_mgr->ReleaseAddon(enc_id);
				*cookie = addon_cookie;
				continue;
			}
			enc = nth_func(enc_sub_id - 1); 
			if (enc == NULL) {
				encoder_mgr->ReleaseAddon(enc_id);
				*cookie = addon_cookie;
				continue;
			}
		}

		if(enc->GetCodecInfo(mci) != B_OK) {
			delete enc;
			encoder_mgr->ReleaseAddon(enc_id);
			continue;
		}

		media_format tmp_input_format;
		media_format tmp_output_format;
		if(input_format)
			tmp_input_format = *input_format;
		if(output_format)
			tmp_output_format = *output_format;

		if(input_format || output_format ||
		   accepted_input_format || accepted_output_format || mfi) {
			status_t err;
			media_file_format tmp_mfi;
			
			if(mfi) {
				tmp_mfi = *mfi;
			}
			else {
				memset(&tmp_mfi, 0, sizeof(tmp_mfi));
			}
			err = enc->SetFormat(&tmp_mfi, &tmp_input_format,
			                     &tmp_output_format);
			if( (err != B_OK)
			  || (input_format && !tmp_input_format.Matches(input_format))
			  || (output_format && !tmp_output_format.Matches(output_format))
			  || (mfi && memcmp(mfi, &tmp_mfi, sizeof(tmp_mfi)) != 0)) {
				delete enc;
				encoder_mgr->ReleaseAddon(enc_id);
				continue;
			}
		}
		
		if(mfi) {
			if(!does_file_accept_format(mfi, &tmp_output_format)) {
				delete enc;
				enc = NULL;
			}
		}

		if (enc != NULL) {
			if(accepted_input_format)
				*accepted_input_format = tmp_input_format;
			if(accepted_output_format)
				*accepted_output_format = tmp_output_format;

			mci->id = enc_id;
			mci->sub_id = enc_sub_id;
			delete enc;
			encoder_mgr->ReleaseAddon(enc_id);
			break;
		}
		encoder_mgr->ReleaseAddon(enc_id);
	}
	return B_OK;
}


status_t
get_next_encoder(int32 *cookie,
				 const media_file_format *mfi,
				 const media_format *input_format, 
				 media_format *output_format,
				 media_codec_info *mci)
{
	return get_next_encoder(cookie, mfi, input_format, NULL, mci, NULL, output_format);
}

status_t 
get_next_encoder(int32 *cookie, media_codec_info *ei)
{
	return get_next_encoder(cookie, NULL, NULL, NULL, ei, NULL, NULL);
}



//---------------------------------------------------------------




BMediaFile::BMediaFile(const entry_ref * ref)
{
	FUNCTION(("BMediaFile::BMediaFile(const entry_ref * ref)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	fFile = new BFile(ref, B_READ_ONLY);
	if ((fErr = fFile->InitCheck()) != B_OK) {
		Init();
		delete fFile;
		fFile = NULL;
		return;
	}
	
	InitReader(fFile);
}

BMediaFile::BMediaFile(const entry_ref * ref, int32 flags)
{
	FUNCTION(("BMediaFile::BMediaFile(const entry_ref * ref, int32 flags)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	if (flags & B_MEDIA_FILE_REPLACE_MODE) {
		fFile = NULL;
		Init();
		fErr = EPERM;
		return;
	}
	fFile = new BFile(ref, B_READ_ONLY);
	if ((fErr = fFile->InitCheck()) != B_OK) {
		Init();
		delete fFile;
		fFile = NULL;
		return;
	}
	
	InitReader(fFile, flags);
}

BMediaFile::BMediaFile(BDataIO *source)
{
	FUNCTION(("BMediaFile::BMediaFile(BDataIO *source)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	fFile = NULL;
	InitReader(source);
}

BMediaFile::BMediaFile(BDataIO *source, int32 flags)
{
	FUNCTION(("BMeBMediaFile::BMediaFile(BDataIO *source, int32 flags)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	fFile = NULL;
	if (flags & B_MEDIA_FILE_REPLACE_MODE) {
		Init();
		fErr = EPERM;
		return;
	}
	InitReader(source, flags);
}

void
BMediaFile::InitReader(BDataIO *source, int32 flags)
{
	FUNCTION(("BMediaFile::InitReader(BDataIO *source, int32 flags)\n"));
	MediaExtractor	*extractor = new MediaExtractor(flags);

	Init();
	
	fErr = extractor->SetSource(source, &fTrackNum);
	
	if(fErr == B_NO_ERROR) {
		fExtractor = extractor;
		return;
	}
	delete extractor;
	
	// try to find one of them 'Detractor' thingies
	BDirectory dir("/boot/beos/system/add-ons/media/detractors");
	if(dir.InitCheck()==B_OK)
	{
		entry_ref ref;
		while(B_OK==dir.GetNextRef(&ref))
		{
			BEntry entry(&ref);
			BPath path;
			entry.GetPath(&path);
			image_id img=load_add_on(path.Path());
			if(img>=0)
			{
				Detractor   *(*make_detractor)(void);
				if (get_image_symbol(img, "instantiate_detractor",
									 B_SYMBOL_TYPE_TEXT,
									 (void **)&make_detractor) != B_OK) {
					unload_add_on(img);
					continue;
				}
				
				Detractor *detractor = make_detractor();
				if (detractor == NULL) {
					unload_add_on(img);
					continue;
				}
				if (detractor->SetTo(source) != B_OK) {
					unload_add_on(img);
					continue;
				}
				fDetractor = detractor;
				fDetractorImage = img;
				fTrackNum = fDetractor->CountTracks();
				fErr = B_OK;
				return;
			}
		}
	}
}


BMediaFile::BMediaFile(const entry_ref *ref, const media_file_format * mfi, int32 flags)
{
	FUNCTION(("BMediaFile::BMediaFile(const entry_ref *ref, const media_file_format * mfi, int32 flags)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	fFile = new BFile(ref, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((fErr = fFile->InitCheck()) != B_OK) {
//		fErr = B_MEDIA_NO_HANDLER;
		Init();
		delete fFile;
		fFile = NULL;
		return;
	}
	
	InitWriter(fFile, mfi, flags);
}


BMediaFile::BMediaFile(BDataIO *source, const media_file_format * mfi, int32 flags)
{
	FUNCTION(("BMediaFile::BMediaFile(BDataIO *source, const media_file_format * mfi, int32 flags)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	fFile = NULL;
	InitWriter(source, mfi, flags);
}

BMediaFile::BMediaFile(const media_file_format * mfi, int32 flags)
{
	FUNCTION(("BMediaFile::BMediaFile(const media_file_format * mfi, int32 flags)\n"));
	fTrackList = NULL;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter = NULL;
	fFileClosed = false;
	fFile = NULL;
	InitWriter(NULL, mfi, flags);
}

status_t BMediaFile::SetTo(const entry_ref *ref)
{
	FUNCTION(("BMediaFile::SetTo(const entry_ref *ref)\n"));
	fFile = new BFile(ref, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((fErr = fFile->InitCheck()) != B_OK) {
		Init();
		delete fFile;
		fFile = NULL;
		return fErr;
	}
	return SetTo(fFile);
}

status_t BMediaFile::SetTo(BDataIO *source)
{
	FUNCTION(("BMediaFile::SetTo(BDataIO *source)\n"));
	if (fWriter != NULL) {
		fErr = fWriter->SetSource(source);
	} else {
		fErr = B_MEDIA_NO_HANDLER;
	}
	return fErr;
}

void
BMediaFile::InitWriter(BDataIO *source, const media_file_format * mfi, int32 /*flags*/)
{
	image_id 	   imgid;
	MediaWriter *(*func)(void);
	status_t   	 (*infofunc)(media_file_format *mfi);
	int32		   file_fmt_id = mfi->id.internal_id;
	
	Init();
	
	fWriterMgr  = __get_writer_manager();
	fEncoderMgr = __get_encoder_manager();
	if (fWriterMgr == NULL || fEncoderMgr == NULL) {
		fErr = B_NO_MEMORY;
		delete fWriterMgr;
		delete fEncoderMgr;
		fWriterMgr = fEncoderMgr = NULL;
		return;
	}

	imgid = fWriterMgr->GetAddonAt(file_fmt_id);
	if (imgid <= 0) {
		fErr = B_BAD_INDEX;
		return;
	}
	
	if (get_image_symbol(imgid, "get_mediawriter_info",
						 B_SYMBOL_TYPE_TEXT, (void **)&infofunc) != B_OK) {
		fWriterMgr->ReleaseAddon(file_fmt_id);
		fErr = B_MISSING_SYMBOL;
		return;
	}

	fMFI=*mfi;
	infofunc(&fMFI);   // save off a copy of the media_file_format 

	if (get_image_symbol(imgid, "instantiate_mediawriter",
						 B_SYMBOL_TYPE_TEXT, (void **)&func) != B_OK) {
		fWriterMgr->ReleaseAddon(file_fmt_id);
		fErr = B_MISSING_SYMBOL;
		return;
	}

	fWriter = func();
	if (fWriter != NULL) {
		if(source)
			fErr = fWriter->SetSource(source);
		else
			fErr = B_OK;
		fWriterID = file_fmt_id;
	} else {
		fWriterMgr->ReleaseAddon(file_fmt_id);
		fErr = B_MEDIA_NO_HANDLER;
	}
}


void
BMediaFile::Init()
{
	fTrackNum  = 0;
	fExtractor = NULL;
	fDetractor = NULL;
	fDetractorImage = -1;
	fWriter    = NULL;
	fFileClosed = false;

	fTrackList = new BList();
}


BMediaFile::~BMediaFile()
{
	BMediaTrack	  *track;

	if (fWriter != 0 && !fFileClosed) {
		(void)CloseFile();
	}
	if (fTrackList != 0) while (fTrackList->CountItems() > 0) {
		track = (BMediaTrack*)fTrackList->RemoveItem((int32)0);
		delete track;
	}
	delete fTrackList;
	fTrackList = NULL;

	if (fExtractor) {
		delete fExtractor;
		fExtractor = NULL;
	}

	if (fDetractor) {
		delete fDetractor;
		fDetractor = NULL;
	}
	if (fDetractorImage>=0)
	{
		unload_add_on(fDetractorImage);
		fDetractorImage = -1;
	}

	if (fWriter) {
		delete fWriter;
		fWriter = NULL;
		fWriterMgr->ReleaseAddon(fWriterID);
		fWriterID = -1;
		fFileClosed = false;
	}

	if (fFile)
		delete fFile;
	fFile = NULL;
}

status_t
BMediaFile::InitCheck() const
{
	FUNCTION(("BMediaFile::InitCheck() const\n"));
	return fErr;
}

status_t
BMediaFile::GetFileFormatInfo(media_file_format *mfi) const
{
	FUNCTION(("BMediaFile::GetFileFormatInfo(media_file_format *mfi) const\n"));
	memset(mfi, 0, sizeof(media_file_format));

	if (fExtractor) {
		fExtractor->GetFileFormatInfo(mfi);
	} else if (fDetractor) {
		fDetractor->GetFileFormatInfo(mfi);
	} else if (fWriter) {
		*mfi = fMFI;
	} else {
		return B_NO_INIT;
	}

	return B_OK;
}


const char *
BMediaFile::Copyright(void) const
{
	FUNCTION(("BMediaFile::Copyright(void) const\n"));
	if (fWriter || fExtractor == NULL)
		return NULL;

	return fExtractor->Copyright();
}

int32
BMediaFile::CountTracks() const
{
	FUNCTION(("BMediaFile::CountTracks() const\n"));
	return fTrackNum;
}

BMediaTrack *
BMediaFile::TrackAt(int32 index)
{
	FUNCTION(("BMediaFile::TrackAt(int32 index)\n"));
	BMediaTrack		*track;

	if (!fExtractor && !fDetractor) {
		return NULL;
	}
	if ((index < 0) || (index >= fTrackNum))
		return NULL;

	if(fExtractor)
		track = new BMediaTrack(fExtractor, index);
	else
		track = new BMediaTrack(fDetractor, index);

	if (track->InitCheck() < B_OK) {
		delete track;
		track = 0;
	}
	else {
		fTrackList->AddItem((void*)track);
	}
	return track;
}


status_t
BMediaFile::ReleaseTrack(BMediaTrack *track)
{
	if (fTrackList->RemoveItem(track)) {
		delete track;
		return B_OK;
	}
	return B_ERROR;
}


status_t
BMediaFile::ReleaseAllTracks(void)
{
	while (fTrackList->CountItems() > 0) {
		delete (BMediaTrack*)fTrackList->RemoveItem((int32)0);
	}

	return B_OK;
}


BMediaTrack *
BMediaFile::CreateTrack(media_format *mf, const media_codec_info *mci, uint32 /*flags*/)
{
	BMediaTrack *track;
	image_id imgid;
	Encoder *(*func)(void);
	Encoder *(*nth_func)(int32);
	Encoder *enc;
	media_format out_fmt;
	media_format requested_format = *mf;

	if (fWriter == NULL) {
		fErr = B_BAD_TYPE;
		return NULL;
	}

	if (mci == NULL || mci->id < 0) {
		fErr = B_BAD_INDEX;
		return NULL;
	}
	
	imgid = fEncoderMgr->GetAddonAt(mci->id);
	if (imgid <= 0) {
		fErr = B_MEDIA_ADDON_FAILED;
		return NULL;
	}
	
	if(mci->sub_id == 0) {
		if ((fErr = get_image_symbol(imgid, "instantiate_encoder",
							 B_SYMBOL_TYPE_TEXT, (void **)&func)) != B_OK) {
			fEncoderMgr->ReleaseAddon(mci->id);
			return NULL;
		}
		enc = func();
	}
	else {
		if ((fErr = get_image_symbol(imgid, "instantiate_nth_encoder",
							 B_SYMBOL_TYPE_TEXT, (void **)&nth_func)) != B_OK) {
			fEncoderMgr->ReleaseAddon(mci->id);
			return NULL;
		}
		enc = nth_func(mci->sub_id-1); 
	}
	
	if ((fErr = enc->SetFormat(&fMFI, mf, &out_fmt)) != B_OK) {
		delete enc;
		fEncoderMgr->ReleaseAddon(mci->id);
		return NULL;
	}
	if(!format_is_compatible(*mf, requested_format)) {
		fErr = B_MEDIA_BAD_FORMAT;
		delete enc;
		fEncoderMgr->ReleaseAddon(mci->id);
		return NULL;
	}

// [em 19may00] let the track start the encoder when it's ready.
//              makes user-adjustable parameters doable without a 'reconfigure encoder' API
//	if ((fErr = enc->StartEncoder()) != B_OK) {
//		delete enc;
//		fEncoderMgr->ReleaseAddon(mci->id);
//		return NULL;
//	}
	
	track = new BMediaTrack(fWriter, fTrackNum++, &out_fmt, enc, const_cast<media_codec_info *>(mci));
	if (track) {
		if((fErr = fWriter->AddTrack(track)) != B_NO_ERROR) {
			delete track;
			fTrackNum--;
			return NULL;
		}
		fTrackList->AddItem((void*)track);
	} else {
		fErr = B_NO_MEMORY;
		fTrackNum--;
	}

	return track;
}

BMediaTrack *
BMediaFile::CreateTrack(media_format *mf, uint32 flags)
{
	BMediaTrack *track;

	if (fWriter == NULL) {
		fErr = B_BAD_TYPE;
		return NULL;
	}

	const char* encName = 0;
	switch(mf->type)
	{
	case B_MEDIA_RAW_AUDIO:
		encName = "raw-audio";
		break;
	case B_MEDIA_RAW_VIDEO:
		encName = "raw-video";
		break;
	default:
		encName=0;
	}
	
	if(!encName || (flags & B_CODEC_INHIBIT_RAW_ENCODER))
	{
		track = new BMediaTrack(fWriter, fTrackNum++, mf, NULL, NULL);
	
		if (track) {
			if((fErr = fWriter->AddTrack(track)) != B_NO_ERROR) {
				delete track;
				fTrackNum--;
				return NULL;
			}
			fTrackList->AddItem((void*)track);
		}
		else {
			fErr = B_NO_MEMORY;
			fTrackNum--;
		}
		return track;

	}

	media_codec_info codecInfo;
	int32 cookie = 0;
	status_t err;
	media_format outputFormat = *mf;
	while((err = get_next_encoder(
		&cookie, &fMFI, mf, &outputFormat, &codecInfo)) == B_OK)
	{
		if(!strcmp(codecInfo.short_name, encName))
			break;
	}
	if(err < B_OK)
	{
		fErr = B_NAME_NOT_FOUND;
		return NULL;
	}
	
	return CreateTrack(mf, &codecInfo, 0);
}

// wrappers for deprecated forms of CreateTrack()

#if _R5_COMPATIBLE_

extern "C" {

#if __GNUC__
	BMediaTrack* CreateTrack__10BMediaFileP12media_formatPC16media_codec_info(
		BMediaFile* this_ptr, media_format *mf, const media_codec_info *mci)
#else
	BMediaTrack* CreateTrack__10BMediaFileFP12media_formatPC16media_codec_info(
		BMediaFile* this_ptr, media_format *mf, const media_codec_info *mci)
#endif
	{
		return this_ptr->CreateTrack(mf, mci, 0);
	}
	
#if __GNUC__
	BMediaTrack* CreateTrack__10BMediaFileP12media_format(
		BMediaFile* this_ptr, media_format *mf)
#else
	BMediaTrack* CreateTrack__10BMediaFileFP12media_format(
		BMediaFile* this_ptr, media_format *mf)
#endif
	{
		return this_ptr->CreateTrack(mf, B_CODEC_INHIBIT_RAW_ENCODER);
	}
};

#endif  // R5_COMPATIBLE


status_t
BMediaFile::AddCopyright(const char *data)
{
	if (fWriter == NULL)
		return B_BAD_TYPE;

	return fWriter->AddCopyright(data);
}


status_t
BMediaFile::AddChunk(int32 type, const void *data, size_t size)
{
	if (fWriter == NULL)
		return B_BAD_TYPE;

	return fWriter->AddChunk(type, (const char *)data, size);
}


status_t
BMediaFile::CommitHeader(void)
{
	if (fWriter == NULL)
		return B_BAD_TYPE;

	if (fTrackList != 0)
	{
		for(int i=0;;i++)
		{
			BMediaTrack *track = (BMediaTrack*)fTrackList->ItemAt(i);
			if(!track)
				break;
			track->CommitHeader();
		}
	}

	return fWriter->CommitHeader();
}

status_t
BMediaFile::CloseFile(void)
{
	int32			i, count;
	BMediaTrack		*track;

	if (fWriter == NULL)
		return B_BAD_TYPE;
	if (fFileClosed)
		return EALREADY;
	fFileClosed = true;

	count = fTrackList->CountItems();
	for (i=0; i<count; i++) {
		track = (BMediaTrack*)fTrackList->ItemAt(i);
		track->Flush();
	}
	return fWriter->CloseFile();
}


status_t 
BMediaFile::GetParameterWeb(BParameterWeb **/*outWeb*/)
{
	return B_ERROR;
}


BParameterWeb *
BMediaFile::Web()
{
	BParameterWeb* w;
	return (GetParameterWeb(&w) == B_OK) ? w : 0;
}


status_t
BMediaFile::GetParameterValue(int32 /*id*/, void */*valu*/, size_t */*size*/)
{
	return B_ERROR;
}


status_t
BMediaFile::SetParameterValue(int32 /*id*/, const void */*valu*/, size_t /*size*/)
{
	return B_ERROR;
}


BView *
BMediaFile::GetParameterView()
{
	return NULL;
}


status_t
BMediaFile::ControlFile(int32 selector, void * data, size_t size)
{
	if (fExtractor)
	{
		return fExtractor->ControlFile(selector, data, size);
	}
	else if (fDetractor)
	{
		return fDetractor->ControlFile(selector, data, size);
	}
	else if (fWriter)
	{
		return fWriter->ControlFile(selector, data, size);
	}
	return EBADF;
}


status_t BMediaFile::Perform(int32 /*selector*/, void * /*data*/)
{
	return B_ERROR;
}

status_t BMediaFile::_Reserved_BMediaFile_0(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_1(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_2(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_3(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_4(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_5(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_6(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_7(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_8(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_9(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_10(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_11(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_12(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_13(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_14(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_15(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_16(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_17(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_18(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_19(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_20(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_21(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_22(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_23(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_24(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_25(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_26(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_27(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_28(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_29(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_30(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_31(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_32(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_33(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_34(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_35(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_36(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_37(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_38(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_39(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_40(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_41(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_42(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_43(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_44(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_45(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_46(int32 /*arg*/, ...) { return B_ERROR; }
status_t BMediaFile::_Reserved_BMediaFile_47(int32 /*arg*/, ...) { return B_ERROR; }
