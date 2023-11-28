/*	MediaAddOn.h	*/

#include "trinity_p.h"
#include "MediaAddOn.h"

#include <string.h>
#include <stdlib.h>

#include <Mime.h>



#if DEBUG > 0
	#define FPRINTF fprintf
#else
	#define FPRINTF(x...)
#endif




/*	the implementation assumes a blocking factor of 4 for the format arrays	*/

dormant_flavor_info::dormant_flavor_info()
{
	name = info = NULL;
	in_formats = out_formats = NULL;
	in_format_count = out_format_count = 0;
	node_info.name[0] = 0;
	node_info.addon = -1;
	node_info.flavor_id = -1;
}

dormant_flavor_info::~dormant_flavor_info()
{
	free(name);
	free(info);
	delete[] in_formats;
	delete[] out_formats;
}

dormant_flavor_info::dormant_flavor_info(const dormant_flavor_info & that) : BFlattenable()
{
	assign_atoms(that);
	node_info = that.node_info;

	if (that.name) name = strdup(that.name); else name = NULL;
	if (that.info) info = strdup(that.info); else info = NULL;

	/*	the implementation assumes a blocking factor of 4 for the format arrays	*/

	if (that.in_format_count && !that.in_formats || that.out_format_count && !that.out_formats) {
		fprintf(stderr, "Bad dormant_flavor_info passed by Node\n");
	}
	in_formats = new media_format[(that.in_format_count+3)&~3];
	memcpy(const_cast<media_format *>(in_formats), that.in_formats, that.in_format_count*sizeof(media_format));
	out_formats = new media_format[(that.out_format_count+3)&~3];
	memcpy(const_cast<media_format *>(out_formats), that.out_formats, that.out_format_count*sizeof(media_format));
}

dormant_flavor_info& dormant_flavor_info::operator=(const dormant_flavor_info & that)
{
	free(name);
	free(info);
	delete[] in_formats;
	delete[] out_formats;

	assign_atoms(that);
	node_info = that.node_info;

	if (that.name) name = strdup(that.name); else name = NULL;
	if (that.info) info = strdup(that.info); else info = NULL;

	/*	the implementation assumes a blocking factor of 4 for the format arrays	*/

	if (that.in_format_count && !that.in_formats || that.out_format_count && !that.out_formats) {
		fprintf(stderr, "Bad dormant_flavor_info passed by Node\n");
	}
	in_formats = new media_format[(that.in_format_count+3)&~3];
	memcpy(const_cast<media_format *>(in_formats), that.in_formats, that.in_format_count*sizeof(media_format));
	out_formats = new media_format[(that.out_format_count+3)&~3];
	memcpy(const_cast<media_format *>(out_formats), that.out_formats, that.out_format_count*sizeof(media_format));

	return *this;
}

dormant_flavor_info & dormant_flavor_info::operator=(const flavor_info & that)
{
	free(name);
	free(info);
	delete[] in_formats;
	delete[] out_formats;

	assign_atoms(that);

	if (that.name) name = strdup(that.name); else name = NULL;
	if (that.info) info = strdup(that.info); else info = NULL;

	/*	the implementation assumes a blocking factor of 4 for the format arrays	*/

	if (that.in_format_count && !that.in_formats || that.out_format_count && !that.out_formats) {
		fprintf(stderr, "Bad flavor_info passed by Node\n");
	}
	in_formats = new media_format[(that.in_format_count+3)&~3];
	memcpy(const_cast<media_format *>(in_formats), that.in_formats, that.in_format_count*sizeof(media_format));
	out_formats = new media_format[(that.out_format_count+3)&~3];
	memcpy(const_cast<media_format *>(out_formats), that.out_formats, that.out_format_count*sizeof(media_format));

	return *this;
}

void dormant_flavor_info::set_name(const char * in_name)
{
	free(name);
	if (in_name) name = strdup(in_name); else name = NULL;
}

void dormant_flavor_info::set_info(const char * in_info)
{
	free(info);
	if (in_info) info = strdup(in_info); else info = NULL;
}

void dormant_flavor_info::add_in_format(const media_format & in_format)
{
	/*	the implementation assumes a blocking factor of 4 for the format arrays	*/

	if (!(in_format_count & 3)) {
		media_format * new_in = new media_format[in_format_count+4];
		if (in_formats) {
			memcpy(new_in, in_formats, sizeof(media_format)*in_format_count);
			delete[] in_formats;
		}
		in_formats = new_in;
	}
	const_cast<media_format &>(in_formats[in_format_count++]) = in_format;
}

void dormant_flavor_info::add_out_format(const media_format & out_format)
{
	/*	the implementation assumes a blocking factor of 4 for the format arrays	*/

	if (!(out_format_count & 3)) {
		media_format * new_out = new media_format[out_format_count+4];
		if (out_formats) {
			memcpy(new_out, out_formats, sizeof(media_format)*out_format_count);
			delete[] out_formats;
		}
		out_formats = new_out;
	}
	const_cast<media_format &>(out_formats[out_format_count++]) = out_format;
}


bool dormant_flavor_info::IsFixedSize() const
{
	return false;
}

type_code dormant_flavor_info::TypeCode() const
{
	return '__DF';
}

ssize_t dormant_flavor_info::FlattenedSize() const
{
	return sizeof(uint32) + sizeof(*this) + sizeof(int32) * 2 +
		(name ? strlen(name) : 0) + (info ? strlen(info) : 0) +
		sizeof(media_format) * (in_format_count + out_format_count);
}

status_t dormant_flavor_info::Flatten(void *buffer, ssize_t size) const
{
	char * d = (char *)buffer;
	size_t s = FlattenedSize();
	if ((size_t)size < s) {
		FPRINTF(stderr, "dormant_flavor_info::Flatten() buffer size %ld too small\n", size);
		return B_ERROR;
	}

	/* version for future expansion */

	uint32 v = 1;
	memcpy(d, &v, sizeof(v));
	d += sizeof(v);

	/* actual data items */

	memcpy(d, this, sizeof(*this));
	d += sizeof(*this);
	int32 l = name ? strlen(name) : 0;
	memcpy(d, &l, sizeof(l));
	d += sizeof(l);
	if (l) {
		memcpy(d, name, l);
		d += l;
	}
	l = info ? strlen(info) : 0;
	memcpy(d, &l, sizeof(l));
	d += sizeof(l);
	if (l) {
		memcpy(d, info, l);
		d += l;
	}
	if (in_formats) memcpy(d, in_formats, sizeof(media_format) * in_format_count);
	d += sizeof(media_format) * in_format_count;
	if (out_formats) memcpy(d, out_formats, sizeof(media_format) * out_format_count);

	return B_OK;
}

status_t dormant_flavor_info::Unflatten(type_code c, const void *buf, ssize_t size)
{
	const char * d = (const char *) buf;
	ssize_t s = size;

	if (!AllowsTypeCode(c)) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() type code '%c%c%c%c' is bad.\n",
			(char)(c>>24), (char)(c>>16), (char)(c>>8), (char)c);
		return B_ERROR;
	}

	uint32 v;
	if (!d || s < (ssize_t)sizeof(v)) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() bad buf or size 0x%x %d\n",
			(int)d, (int)s);
		return B_ERROR;
	}
	memcpy(&v, d, sizeof(v));
	if (v != 1) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() bad version %d\n", (int)v);
		return B_ERROR;
	}
	d += sizeof(v);
	s -= sizeof(v);

	if (s < (ssize_t)sizeof(*this)) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() bad size #1 %d\n", (int)size);
		return B_ERROR;
	}

	if (in_formats) {
		delete[] in_formats;
	}
	if (out_formats) {
		delete[] out_formats;
	}

	const dormant_flavor_info * dfi = (const dormant_flavor_info *)d;
	assign_atoms(*dfi);
	node_info = dfi->node_info;
	d += sizeof(*this);
	s -= sizeof(*this);

	int32 l;
	if (s < (ssize_t)sizeof(l)) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() bad size #2 %d\n", (int)size);
		return B_ERROR;
	}
	memcpy(&l, d, sizeof(l));
	d += sizeof(l);
	s -= sizeof(l);
	if (l > 0) {
		name = (char *)malloc(l+1);
		memcpy(name, d, l);
		name[l] = 0;
		d += l;
		s -= l;
	}

	if (s < ssize_t(sizeof(l))) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() bad size #3 %d\n", (int)size);
		return B_ERROR;
	}
	memcpy(&l, d, sizeof(l));
	d += sizeof(l);
	s -= sizeof(l);
	if (l > 0) {
		info = (char *)malloc(l+1);
		memcpy(info, d, l);
		info[l] = 0;
		d += l;
		s -= l;
	}

	if (s < (ssize_t)(sizeof(media_format) * (in_format_count + out_format_count))) {
		FPRINTF(stderr, "dormant_flavor_info::Unflatten() bad size #4 %d\n", (int)size);
		return B_ERROR;
	}
	if (in_format_count) {
		/* block by 4 */
		in_formats = new media_format[(in_format_count+3)&~3];
		memcpy(const_cast<media_format *>(in_formats), d, sizeof(media_format)*in_format_count);
		d += sizeof(media_format)*in_format_count;
		s -= sizeof(media_format)*in_format_count;
	}
	if (out_format_count) {
		/* block by 4 */
		out_formats = new media_format[(out_format_count+3)&~3];
		memcpy(const_cast<media_format *>(out_formats), d, sizeof(media_format)*out_format_count);
		d += sizeof(media_format)*out_format_count;
		s -= sizeof(media_format)*out_format_count;
	}

	return B_OK;
}

void dormant_flavor_info::assign_atoms(const flavor_info & that)
{
	kinds = that.kinds;
	flavor_flags = that.flavor_flags;
	internal_id = that.internal_id;
	possible_count = that.possible_count;
	in_format_count = that.in_format_count;
	in_format_flags = that.in_format_flags;
	out_format_count = that.out_format_count;
	out_format_flags = that.out_format_flags;
	memcpy(_reserved_, that._reserved_, sizeof(_reserved_));
}






BMediaAddOn::BMediaAddOn(
	image_id image)
{
	_m_image = image;
	_m_addon = -1;
	_m_owner_hook = 0;
	_m_owner_cookie = 0;
}


BMediaAddOn::~BMediaAddOn()
{
}


int32
BMediaAddOn::CountFlavors()
{
	return 0;
}

status_t
BMediaAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	n = n;
	out_info = out_info;
	return B_ERROR;
}

status_t 
BMediaAddOn::GetConfigurationFor(BMediaNode */*your_node*/, BMessage */*into_message*/)
{
	return B_OK;
}

bool 
BMediaAddOn::WantsAutoStart()
{
	return false;
}

status_t 
BMediaAddOn::AutoStart(int /*in_count*/, BMediaNode **/*out_node*/, int32 */*out_internal_id*/, bool */*out_has_more*/)
{
	return B_ERROR;
}

BMediaNode *
BMediaAddOn::InstantiateNodeFor(
	const flavor_info * info, 
	BMessage * /*config*/, 
	status_t * /*out_error*/)
{
	info = info;
	return NULL;
}


status_t
BMediaAddOn::InitCheck(
	const char ** out_failure_text)
{
	*out_failure_text = "Unimplemented node feature";
	return B_ERROR;
}


status_t
BMediaAddOn::SniffRef(
	const entry_ref & /*file*/,
	BMimeType * io_mime_type,
	float * out_quality,
	int32 * out_internal_id)
{
	io_mime_type->Unset();
	*out_quality = -2;
	*out_internal_id = -1;
	return B_ERROR;
}


status_t
BMediaAddOn::SniffType(
	const BMimeType & /*type*/,
	float * out_quality,
	int32 * out_internal_id)
{
	*out_quality = -2;
	*out_internal_id = -1;
	return B_ERROR;
}

status_t 
BMediaAddOn::GetFileFormatList(
	int32 /*flavor_id*/,
	media_file_format * /*out_writable_formats*/,
	int32 /*in_write_items*/,
	int32 * /*out_write_items*/,
	media_file_format * /*out_readable_formats*/,
	int32 /*in_read_items*/,
	int32 * /*out_read_items*/,
	void * /*_reserved*/)
{
	return B_ERROR;
}


status_t
BMediaAddOn::SniffTypeKind(
	const BMimeType & type,
	uint64 /*kinds*/, 
	float * out_quality,
	int32 * out_internal_id,
	void * /*_reserved*/)
{
	status_t err = SniffType(type, out_quality, out_internal_id);
	if (err == B_OK)
		fprintf(stderr, "Old Node (%ld); calling SniffType(%s) works (but what kind?)\n",
				ImageID(), type.Type());
	return err;
}


image_id
BMediaAddOn::ImageID()
{
	return _m_image;
}


media_addon_id
BMediaAddOn::AddonID()
{
	return _m_addon;
}


static BLocker gAddonOwnerLock("be:addon_owner_lock");

status_t
BMediaAddOn::NotifyFlavorChange()
{
	status_t err = B_OK;
	gAddonOwnerLock.Lock();
	if (_m_owner_hook != 0) {
		err = (*_m_owner_hook)(_m_owner_cookie, this);
	}
	gAddonOwnerLock.Unlock();
	return err;
}

void
BMediaAddOn::SetOwner(
	status_t (*hook)(void *, BMediaAddOn *),
	void * cookie)
{
	gAddonOwnerLock.Lock();
	_m_owner_hook = hook;
	_m_owner_cookie = cookie;
	gAddonOwnerLock.Unlock();
}


status_t
BMediaAddOn::_Reserved_MediaAddOn_0(void *)
{
	/*	No longer virtual; used for GetFileFormatList.	*/
	/*	Old users will not support that function, and	*/
	/*	thus returning error is good enough.		*/
	return B_ERROR;
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_1(void *)
{
	return -2;	//	new function called on old Node (magic value)
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_2(void *)
{
	return B_ERROR;
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_3(void *)
{
	return B_ERROR;
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_4(void *)
{
	return B_ERROR;
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_5(void *)
{
	return B_ERROR;
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_6(void *)
{
	return B_ERROR;
}

status_t
BMediaAddOn::_Reserved_MediaAddOn_7(void *)
{
	return B_ERROR;
}

