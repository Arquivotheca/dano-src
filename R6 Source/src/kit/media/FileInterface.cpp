/*	FileInterface.cpp	*/

#include "FileInterface.h"
#include "tr_debug.h"
#include "trinity_p.h"

#include <string.h>


BFileInterface::~BFileInterface()
{
}


BFileInterface::BFileInterface() :
	BMediaNode("%%%FileInterface%%%")
{
	AddNodeKind(B_FILE_INTERFACE);
	dlog("BFileInterface::BFileInterface()");
}


status_t
BFileInterface::HandleMessage(
	int32 message,
	const void * data,
	size_t /*size*/)
{
	status_t err = B_OK;
	switch (message) {
	case FI_GET_LENGTH: {
		get_length_a ans;
		ans.cookie = ((get_length_q *)data)->cookie;
		ans.error = GetDuration(&ans.length);
		write_port(((get_length_q *)data)->reply, FI_GET_LENGTH_REPLY, 
			&ans, sizeof(ans));
		} break;
	case FI_SNIFF_FILE: {
		entry_ref ref(((sniff_file_q *)data)->device, 
			((sniff_file_q *)data)->directory, 
			((sniff_file_q *)data)->name);
		sniff_file_a ans;
		ans.cookie = ((sniff_file_q *)data)->cookie;
		ans.mime_type[0] = 0;
		ans.error = SniffRef(ref, ans.mime_type, &ans.quality);
		write_port(((sniff_file_q *)data)->reply, FI_SNIFF_FILE_REPLY, 
			&ans, sizeof(ans));
		} break;
	case FI_SPECIFY_FILE: {
		entry_ref ref(((specify_file_q *)data)->device, 
			((specify_file_q *)data)->directory, 
			((specify_file_q *)data)->name);
		specify_file_a ans;
		ans.cookie = ((specify_file_q *)data)->cookie;
		ans.error = SetRef(ref, ((specify_file_q *)data)->create, 
			&ans.length);
		write_port(((specify_file_q *)data)->reply, FI_SPECIFY_FILE_REPLY, 
			&ans, sizeof(ans));
		} break;
	case FI_GET_CUR_FILE: {
		get_cur_file_a ans;
		ans.cookie = ((get_cur_file_q *)data)->cookie;
		entry_ref ent;
		ans.error = GetRef(&ent, ans.mime_type);
		ans.device = ent.device;
		ans.directory = ent.directory;
		strncpy(ans.name, ent.name ? ent.name : "", 256);
		ans.name[255] = 0;
		write_port(((get_cur_file_q *)data)->reply, FI_GET_CUR_FILE_REPLY, 
			&ans, sizeof(ans));
		} break;
	case FI_ITERATE_FILE_FORMATS: {
		iterate_file_formats_a ans;
		ans.cookie = ((iterate_file_formats_q *)data)->cookie;
		ans.error = GetNextFileFormat(&ans.cookie, &ans.format);
		write_port(((iterate_file_formats_q *)data)->reply, 
			FI_ITERATE_FILE_FORMATS_REPLY, &ans, sizeof(ans));
		} break;
	case FI_DISPOSE_FILE_FORMAT_COOKIE: {
		DisposeFileFormatCookie(((dispose_file_format_cookie_q *)data)->cookie);
		} break;
	default:
#if DEBUG
		if ((message & 0xffffff00) == 0x40000500) {
			dlog("Unknown message in BFileInterface::HandleMessage [%x]", message);
			abort();
		}
#endif
		err = B_ERROR;
		break;
	}
	return err;
}


status_t
BFileInterface::GetNextFileFormat(
	int32 * cookie,
	media_file_format * out_format)
{
	cookie = cookie;
	out_format = out_format;
	return B_ERROR;
}


void
BFileInterface::DisposeFileFormatCookie(
	int32 cookie)
{
	cookie = cookie;
}


status_t
BFileInterface::GetDuration(
	bigtime_t * out_time)
{
	out_time = out_time;
	return B_ERROR;
}


status_t
BFileInterface::SniffRef(
	const entry_ref & /*file*/,
	char * out_mime_type,
	float * out_quality)
{
	out_mime_type = out_mime_type;
	out_quality = out_quality;
	return B_ERROR;
}


status_t
BFileInterface::SetRef(
	const entry_ref & /*file*/,
	bool create,
	bigtime_t * out_time)
{
	create = create;
	out_time = out_time;
	return B_ERROR;
}


status_t
BFileInterface::GetRef(
	entry_ref * out_ref,
	char * out_mime_type)
{
	out_ref = out_ref;
	out_mime_type = out_mime_type;
	return B_ERROR;
}



		/* Mmmh, stuffing! */
status_t
BFileInterface::_Reserved_FileInterface_0(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_1(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_2(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_3(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_4(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_5(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_6(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_7(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_8(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_9(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_10(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_11(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_12(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_13(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_14(void *)
{
	return B_ERROR;
}

status_t
BFileInterface::_Reserved_FileInterface_15(void *)
{
	return B_ERROR;
}

