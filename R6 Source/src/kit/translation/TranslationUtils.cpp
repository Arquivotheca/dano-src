/*	TranslationUtils.cpp
 *	$Id$
 */

#if !defined(_TRANSLATION_UTILS_H)
	#include <TranslationUtils.h>
#endif	/*	_BDATA_H	*/
#if !defined(_BITMAP_H)
	#include <Bitmap.h>
#endif	/*	_BITMAP_H	*/
#if !defined(_TRANSLATOR_ROSTER_H)
	#include <TranslatorRoster.h>
#endif	/*	_TRANSLATOR_ROSTER_H	*/
#if !defined(_DATA_IO_H)
	#include <DataIO.h>
#endif	/*	_DATA_IO_H	*/
#if !defined(_AUTO_LOCK_H)
	#include <AutoLock.h>
#endif	/*	_AUTO_LOCK_H	*/
#if !defined(_ENTRY_H)
	#include <Entry.h>
#endif	/*	_ENTRY_H	*/
#if !defined(_APPLICATION_H)
	#include <Application.h>
#endif	/*	_APPLICATION_H	*/
#if !defined(__cstring__)
	#include <string.h>
#endif	/*	__cstring__	*/
#if !defined(_BITMAP_STREAM_H)
	#include <BitmapStream.h>
#endif	/* _BITMAP_STREAM_H	*/
#if !defined(_FILE_H)
	#include <File.h>
#endif	/*	_FILE_H	*/
#if !defined(_RESOURCES_H)
	#include <Resources.h>
#endif	/*	_RESOURCES_H	*/
#if !defined(__cstdlib__)
	#include <stdlib.h>
#endif	/*	__cstdlib__	*/
#if !defined(_PATH_H)
	#include <Path.h>
#endif	/*	_PATH_H	*/
#if !defined(_AUTO_DELETE_TEM)
	#include "_AutoDelete.tem"
#endif	/*	_AUTO_DELETE_TEM */
#if !defined(_UNISTD_H)
	#include <unistd.h>
#endif	/* _UNISTD_H */
#if !defined(_TEXT_VIEW_H)
	#include <TextView.h>
#endif	/* _TEXT_VIEW_H */
#if !defined(_BYTEORDER_H)
	#include <ByteOrder.h>
#endif	/* _BYTEORDER_H */
#if !defined(__cstdio__)
	#include <stdio.h>
#endif	/* __cstdio__ */
#if !defined(_FIND_DIRECTORY_H)
	#include <FindDirectory.h>
#endif	/* _FIND_DIRECTORY_H */
#if !defined(_NODE_INFO_H)
	#include <NodeInfo.h>
#endif	/* _NODE_INFO_H */
#if !defined(_MENU_ITEM_H)
	#include <MenuItem.h>
#endif	/* _MENU_ITEM_H */
#if !defined(_MENU_H)
	#include <Menu.h>
#endif	/* _MENU_H */


#include <Roster.h>


char B_TRANSLATOR_EXT_HEADER_ONLY[] = "/headerOnly";
char B_TRANSLATOR_EXT_DATA_ONLY[] = "/dataOnly";
char B_TRANSLATOR_EXT_COMMENT[] = "/comment";
char B_TRANSLATOR_EXT_TIME[] = "/time";
char B_TRANSLATOR_EXT_FRAME[] = "/frame";
char B_TRANSLATOR_EXT_BITMAP_RECT[] = "bits/Rect";
char B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE[] = "bits/space";
char B_TRANSLATOR_EXT_BITMAP_PALETTE[] = "bits/palette";
char B_TRANSLATOR_EXT_SOUND_CHANNEL[] = "nois/channel";
char B_TRANSLATOR_EXT_SOUND_MONO[] = "nois/mono";
char B_TRANSLATOR_EXT_SOUND_MARKER[] = "nois/marker";
char B_TRANSLATOR_EXT_SOUND_LOOP[] = "nois/loop";

color_space BTranslationUtils::sBitmapSpace = B_NO_COLOR_SPACE;


BTranslationUtils::BTranslationUtils() { }
BTranslationUtils::~BTranslationUtils() { }
BTranslationUtils::BTranslationUtils(const BTranslationUtils&) { }
BTranslationUtils & BTranslationUtils::operator=(const BTranslationUtils&) { return *this; }


BBitmap *
BTranslationUtils::GetBitmap(
	const char * name,
	BTranslatorRoster * use)
{
	BBitmap * ret = GetBitmapFile(name, use);
	if (!ret) ret = GetBitmap(B_TRANSLATOR_BITMAP, name, use);
	return ret;
}


static bool
InitAppResources()
{
	if (BApplication::AppResources() == NULL) {
		return false;
	}
	return true;
}


BBitmap *
BTranslationUtils::GetBitmap(
	uint32 type,
	int32 id,
	BTranslatorRoster * use)
{
	if (!InitAppResources()) return NULL;
	size_t size = 0;
	void * data = BApplication::AppResources()->FindResource(type, id, &size);
	if (!data) return NULL;
	BMemoryIO mio(data, size);
	BBitmap * ret = GetBitmap(&mio, use);
	free(data);
	return ret;
}


BBitmap *
BTranslationUtils::GetBitmap(
	uint32 type,
	const char * name,
	BTranslatorRoster * use)
{
	if (!InitAppResources()) return NULL;
	size_t size = 0;
	void * data = BApplication::AppResources()->FindResource(type, name, &size);
	if (!data) return NULL;
	BMemoryIO mio(data, size);
	BBitmap * ret = GetBitmap(&mio, use);
	free(data);
	return ret;
}


BBitmap *
BTranslationUtils::GetBitmapFile(
	const char * path,
	BTranslatorRoster * use)
{
	BFile file;
	char * ptr = (char *)path;
	if (!ptr) return NULL;
	if (!*ptr) return NULL;
	if (*ptr != '/') /* not absolute path */
	{
		/* get app directory path -- this is WAY more complicated than it should be! */
		app_info info;
		if (!be_app) return NULL;
		be_app->GetAppInfo(&info);
		BEntry app_entry(&info.ref, true);
		char path_buf[PATH_MAX];
		ptr = path_buf;
		BPath p;
		if (app_entry.GetPath(&p) < B_OK) return NULL;
		strcpy(ptr, p.Path());
		/* replace app name with the relative path or name passed in */
		strcpy(strrchr(ptr, '/')+1, path);
	}
	if (file.SetTo(ptr, O_RDONLY) < B_OK) return NULL;
	return GetBitmap(&file, use);
}


BBitmap *
BTranslationUtils::GetBitmap(
	const entry_ref * ref,
	BTranslatorRoster * use)
{
	BFile file(ref, O_RDONLY);
	if (file.InitCheck() < 0) return NULL;
	return GetBitmap(&file, use);
}


BBitmap *
BTranslationUtils::GetBitmap(
	BPositionIO * stream,
	BTranslatorRoster * use)
{
	BBitmapStream bms(sBitmapSpace);
	if (!use) use = BTranslatorRoster::Default();
	if (use->Translate(stream, NULL, NULL, &bms, B_TRANSLATOR_BITMAP))
		return NULL;
	BBitmap * ret = NULL;
	if (bms.DetachBitmap(&ret) < B_OK)
		return NULL;
	return ret;
}

void 
BTranslationUtils::SetBitmapColorSpace(color_space newSpace)
{
	sBitmapSpace = newSpace;
}

color_space 
BTranslationUtils::BitmapColorSpace()
{
	return sBitmapSpace;
}



/*	Styled text convenience functions	*/
status_t
BTranslationUtils::GetStyledText(
	BPositionIO * fromStream,
	BTextView * intoView,   /*      not NULL        */
	BTranslatorRoster * use)
{
	/*	We are lazy, and translate into a temporary stream.	*/
	/*	Then we parse that stream, putting data into the text	*/
	/*	view. We could write a BBitmapStream equivalent for	*/
	/*	text view data, but that's too complicated right now.	*/
	/*	If support for really large files is a priority, we	*/
	/*	can always use a file in /tmp as the temp stream.	*/

	BMallocIO mio;
	if (!use) use = BTranslatorRoster::Default();
	if (!use) return B_NO_INIT;
	bool text = false;
	status_t err = use->Translate(fromStream, NULL, NULL, &mio, B_STYLED_TEXT_FORMAT);
	if (err < B_OK) {
		text = true;
		err = use->Translate(fromStream, NULL, NULL, &mio, B_ASCII_TYPE);
	}
	if (err < B_OK) return err;
	if (text) {
		intoView->Insert(intoView->TextLength(), (const char *)mio.Buffer(), mio.BufferLength());
		return B_OK;
	}
	union {
		TranslatorStyledTextStreamHeader stream;
		TranslatorStyledTextTextHeader text;
		TranslatorStyledTextStyleHeader style;
		TranslatorStyledTextRecordHeader record;
		unsigned char data[1024];
	} hdr;
	err = mio.Seek(0, SEEK_END);
	if (err < 0) return err;
	size_t end = mio.Position();
	err = mio.Seek(0, SEEK_SET);
	if (err < 0) return err;
	size_t base_offset = intoView->TextLength();
	size_t running_offset = base_offset;
	/* a good MallocIO never fails */
	while (mio.Position() < end) {
		status_t cur_hdr_pos = mio.Position();
		if (mio.Read(&hdr, sizeof(hdr.record)) < sizeof(hdr.record)) {
//			printf("Can't read full header -- error\n");
			return B_IO_ERROR;
		}
		if (hdr.record.header_size > sizeof(hdr.record)) {
			if (hdr.record.header_size > sizeof(hdr)) {
				mio.Read(&hdr.data[sizeof(hdr.record)], sizeof(hdr)-sizeof(hdr.record));
				mio.Seek(hdr.record.header_size-sizeof(hdr), SEEK_CUR);
			}
			else {
				mio.Read(&hdr.data[sizeof(hdr.record)], hdr.record.header_size-sizeof(hdr.record));
			}
		}
		switch (B_BENDIAN_TO_HOST_INT32(hdr.record.magic)) {
		case TranslatorStyledTextStreamHeader::STREAM_HEADER_MAGIC:
//			printf("STREAM_HEADER_MAGIC\n");
			/* yeah, that's nice */
			break;
		case TranslatorStyledTextTextHeader::TEXT_HEADER_MAGIC: {
//			printf("TEXT_HEADER_MAGIC (%d bytes)\n", B_BENDIAN_TO_HOST_INT32(hdr.record.data_size));
				char * data = (char *)mio.Buffer();	/* oooh, bad! */
				data += cur_hdr_pos + B_BENDIAN_TO_HOST_INT32(hdr.record.header_size);
				char * xlate = data;
				char * end = data + B_BENDIAN_TO_HOST_INT32(hdr.record.data_size);
				while (xlate < end) {
					char ch = *xlate;
					if (ch == 0) 	/* fix up possibly bad data */
						*xlate = '_';
					else if (ch == 13)	/* we don't support soft line breaks */
						*xlate = 10;
					xlate++;
				}
				intoView->Insert(running_offset, data, B_BENDIAN_TO_HOST_INT32(hdr.record.data_size));
				running_offset += B_BENDIAN_TO_HOST_INT32(hdr.record.data_size);
			} break;
		case TranslatorStyledTextStyleHeader::STYLE_HEADER_MAGIC: {
//			printf("STYLE_HEADER_MAGIC (%d bytes)\n", B_BENDIAN_TO_HOST_INT32(hdr.record.data_size));
				const void * data = mio.Buffer();
				data = ((const char *)data)+cur_hdr_pos + B_BENDIAN_TO_HOST_INT32(hdr.record.header_size);
				text_run_array * styl = intoView->UnflattenRunArray(data);
				size_t begin = base_offset+B_BENDIAN_TO_HOST_INT32(hdr.style.apply_offset);
				size_t end = begin+B_BENDIAN_TO_HOST_INT32(hdr.style.apply_length);
//				fprintf(stderr, "begin %d  end %d\n", begin, end);
				intoView->SetRunArray(begin, end, styl);
				BTextView::FreeRunArray(styl);
			} break;
		default:
//			printf("Unknown (%c%c%c%c) (%d bytes)\n", B_BENDIAN_TO_HOST_INT32(hdr.record.magic)>>24, 
//				B_BENDIAN_TO_HOST_INT32(hdr.record.magic)>>16, B_BENDIAN_TO_HOST_INT32(hdr.record.magic)>>8, B_BENDIAN_TO_HOST_INT32(hdr.record.magic),
//				B_BENDIAN_TO_HOST_INT32(hdr.record.data_size));
			/* skip unknown */
			break;
		}
		mio.Seek(B_BENDIAN_TO_HOST_INT32(hdr.record.header_size)+B_BENDIAN_TO_HOST_INT32(hdr.record.data_size)+cur_hdr_pos, SEEK_SET);
	}
//	printf("All returns OK.\n");
	return B_OK;
}


status_t
BTranslationUtils::PutStyledText(
	BTextView * fromView,
	BPositionIO * intoStream,
	BTranslatorRoster * use)
{
	TranslatorStyledTextStreamHeader stream;
	TranslatorStyledTextTextHeader text;
	TranslatorStyledTextStyleHeader style;
	status_t err;

	stream.header.magic = B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextStreamHeader::STREAM_HEADER_MAGIC);
	stream.header.header_size = B_HOST_TO_BENDIAN_INT32(sizeof(stream));
	stream.header.data_size = B_HOST_TO_BENDIAN_INT32(0);
	stream.version = B_HOST_TO_BENDIAN_INT32(100);

	err = intoStream->Write(&stream, sizeof(stream));
	if (err != sizeof(stream)) return err >= 0 ? B_IO_ERROR : err;

	text.header.magic = B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextTextHeader::TEXT_HEADER_MAGIC);
	text.header.header_size = B_HOST_TO_BENDIAN_INT32(sizeof(text));
	text.header.data_size = B_HOST_TO_BENDIAN_INT32(fromView->TextLength());
	text.charset = B_HOST_TO_BENDIAN_INT32(B_UNICODE_UTF8);

	err = intoStream->Write(&text, sizeof(text));
	if (err != sizeof(text)) return err >= 0 ? B_IO_ERROR : err;
	err = intoStream->Write(fromView->Text(), fromView->TextLength());
	if (err != fromView->TextLength()) return err >= 0 ? B_IO_ERROR : err;

	text_run_array * tra;
	int32 size;
	tra = fromView->RunArray(0, text.header.data_size);
	if (tra != NULL) {
		void * data = fromView->FlattenRunArray(tra, &size);
		BTextView::FreeRunArray(tra);
		if (data == NULL) {
			return B_IO_ERROR;
		}
		style.header.magic = B_HOST_TO_BENDIAN_INT32(TranslatorStyledTextStyleHeader::STYLE_HEADER_MAGIC);
		style.header.header_size = B_HOST_TO_BENDIAN_INT32(sizeof(style));
		style.header.data_size = B_HOST_TO_BENDIAN_INT32(size);
		style.apply_offset = B_HOST_TO_BENDIAN_INT32(0);
		style.apply_length = text.header.data_size;	/* already swapped */

		err = intoStream->Write(&style, sizeof(style));
		if (err != sizeof(style)) {
			free(data);
			return err >= 0 ? B_IO_ERROR : err;
		}

		err = intoStream->Write(data, size);
		free(data);
		if (err != size) return err >= 0 ? B_IO_ERROR : err;
	}
	return B_OK;
}


status_t
BTranslationUtils::WriteStyledEditFile(
	BTextView * fromView,
	BFile * intoFile)
{
	status_t err = intoFile->SetSize(0);
	if (err < B_OK) return err;
	intoFile->Seek(0, SEEK_SET);
	err = intoFile->Write(fromView->Text(), fromView->TextLength());
	if (err != fromView->TextLength()) {
		return (err >= 0) ? B_DEVICE_FULL : err;
	}
	intoFile->RemoveAttr("styles");
	int32 size = 0;
	text_run_array * styles = fromView->RunArray(0, fromView->TextLength(), &size);
	if (!styles) {
		return B_OK;
	}
	void * data = BTextView::FlattenRunArray(styles, &size);
	BTextView::FreeRunArray(styles);
	if (!data) {
		return B_BAD_VALUE;
	}
	err = intoFile->WriteAttr("styles", B_RAW_TYPE, 0, data, size);
	free(data);
	if (err < B_OK) return err;
	BNodeInfo ninfo(intoFile);
	err = ninfo.SetType("text/plain");
	if (err > 0) err = 0;
	return err;
}



struct file_header {
	char	magic[32];
	int32 version_ltl;
};
struct file_tag {
	char tag[8];
	size_t size_ltl;
};
#define VERSION 100
static char file_magic[32] = {
	'T', 'r', 'a', 'n', 's', 'l', 'a', 't', 
	'i', 'o', 'n', ' ', 'S', 'e', 't', 't', 
	'i', 'n', 'g', 's', ' ', 'f', 'i', 'l', 
	'e',  13,  10,  26,   0, 255, 254,   0,
};

/*	We parse through the settings file each time.	*/
/*	Some future version (especially if it supports setting defaults)	*/
/*	would probably just read them once and then remember them.	*/

BMessage *
BTranslationUtils::GetDefaultSettings(
	translator_id id,
	BTranslatorRoster * r)
{
	const char * name = NULL;
	const char * info = NULL;
	int32 version = 0;
	if (r == NULL) {
		r = BTranslatorRoster::Default();
	}
	if (r->GetTranslatorInfo(id, &name, &info, &version)) {
		return NULL;
	}
	return GetDefaultSettings(name, version);
}

BMessage *
BTranslationUtils::GetDefaultSettings(
	const char * translator_name,
	int32 max_version)
{
	BPath path;
	status_t err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (err < B_OK) {
		path.SetTo("/tmp");
	}
	path.Append("Translation Settings");
	BFile file;
	if (file.SetTo(path.Path(), O_RDONLY) != B_OK) {
//		fprintf(stderr, "Translation Settings not found.\n");
		return NULL;
	}
	file_header hdr;
	if ((file.Read(&hdr, sizeof(hdr)) != sizeof(hdr)) || memcmp(hdr.magic, file_magic, sizeof(file_magic))) {
//		fprintf(stderr, "Translation Settings corrupt.\n");
		return NULL;
	}
	if (B_LENDIAN_TO_HOST_INT32(hdr.version_ltl) > VERSION) {
//		fprintf(stderr, "Translation Settings is too new (version %d, I'm %d).\n",
//			B_LENDIAN_TO_HOST_INT32(hdr.version_ltl), VERSION);
		return NULL;
	}
	file_tag tag;
	int32 got_version = -1;
	BMessage * message = NULL;
	while (file.Read(&tag, sizeof(tag)) == sizeof(tag)) {
		off_t pos = file.Position();
		if (!strncmp(tag.tag, "setting ", 8)) {
			char * data = (char *)malloc(B_LENDIAN_TO_HOST_INT32(tag.size_ltl));
			if (!data) {
//				fprintf(stderr, "Out of memory on %ld bytes for setting\n", 
//					B_LENDIAN_TO_HOST_INT32(tag.size_ltl));
				goto skip;
			}
			if (file.Read(data, B_LENDIAN_TO_HOST_INT32(tag.size_ltl)) != 
				B_LENDIAN_TO_HOST_INT32(tag.size_ltl)) {
//				fprintf(stderr, "IO Error reading %ld bytes in setting\n", 
//					B_LENDIAN_TO_HOST_INT32(tag.size_ltl));
				goto skip;
			}
			char * temp = data;
			int32 version;
			memcpy(&version, temp, sizeof(version));
			version = B_LENDIAN_TO_HOST_INT32(version);
			temp += sizeof(version);
			char * name = strdup(temp);
			temp += strlen(name)+1;
			if (strcmp(name, translator_name) || (version < got_version) || 
				(version > max_version)) {
				free(data);
				goto skip;
			}
			free(name);
			BMessage * old = message;
			message = new BMessage;
			if (message->Unflatten(temp) < B_OK) {
//				fprintf(stderr, "Message unflatten error in setting\n");
				free(name);
				free(data);
				delete message;
				message = old;
				goto skip;
			}
			delete old;
			free(data);
//			fprintf(stderr, "loaded setting %s %d\n", ss->name, ss->version);
		}
		else {
//			fprintf(stderr, "unknown tag: %.8s\n", tag.tag);
		}
	skip:
		if (file.Seek(pos+B_LENDIAN_TO_HOST_INT32(tag.size_ltl), SEEK_SET) < B_OK) {
			break;
		}
	}
//	fprintf(stderr, "done loading settings\n");
	return message;
}


status_t
BTranslationUtils::AddTranslationItems(
	BMenu * intoMenu,
	uint32 from_type,
	const BMessage * model,	/* default B_TRANSLATION_MENU */
	const char * translator_id_name, /* default "be:translator" */
	const char * translator_type_name, /* default "be:type" */
	BTranslatorRoster * use)
{
	if (use == NULL) {
		use = BTranslatorRoster::Default();
	}
	if (translator_id_name == NULL) {
		translator_id_name = "be:translator";
	}
	if (translator_type_name == NULL) {
		translator_type_name = "be:type";
	}
	translator_id * ids = NULL;
	int32 count = 0;
	status_t err = use->GetAllTranslators(&ids, &count);
	if (err < B_OK) return err;
	for (int tix=0; tix<count; tix++) {
		const translation_format * formats = NULL;
		int32 num_formats = 0;
		bool ok = false;
		err = use->GetInputFormats(ids[tix], &formats, &num_formats);
		if (err == B_OK) for (int iix=0; iix<num_formats; iix++) {
			if (formats[iix].type == from_type) {
				ok = true;
				break;
			}
		}
		if (!ok) continue;
		err = use->GetOutputFormats(ids[tix], &formats, &num_formats);
		if (err == B_OK) for (int oix=0; oix<num_formats; oix++) {
			if (formats[oix].type != from_type) {
				BMessage * itemmsg;
				if (model) {
					itemmsg = new BMessage(*model);
				}
				else {
					itemmsg = new BMessage(B_TRANSLATION_MENU);
				}
				itemmsg->AddInt32(translator_id_name, ids[tix]);
				itemmsg->AddInt32(translator_type_name, formats[oix].type);
				intoMenu->AddItem(new BMenuItem(formats[oix].name, itemmsg));
			}
		}
	}
	delete[] ids;
	return B_OK;
}



