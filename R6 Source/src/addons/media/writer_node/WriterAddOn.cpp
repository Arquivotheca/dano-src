
#include <stdio.h>
#include <list>

#include <Mime.h>
#include <MediaFormats.h>
#include <Autolock.h>
#include <MediaDefs.h>

#include "WriterAddOn.h"
#include "WriterNode.h"


#if NDEBUG
	#define FUNC(x)
#else
	#define FUNC(x) printf x
#endif


struct my_flavor_info : public flavor_info
{
public:
		my_flavor_info();
		~my_flavor_info();
		my_flavor_info & operator=(const my_flavor_info & other);
		my_flavor_info(const my_flavor_info & other);

		my_flavor_info & operator+=(const media_format & format);	//	just to annoy you
		my_flavor_info & operator+=(const media_codec_info & codec);

private:
		void assign(const my_flavor_info & other);
		void dispose();

		int32 codec_count;
		media_codec_info * codecs;
};

my_flavor_info::my_flavor_info()
{
	name = NULL;
	info = NULL;
	in_format_count = 0;
	out_format_count = 0;
	codec_count = 0;
	in_formats = 0;
	out_formats = 0;
	codecs = 0;
	internal_id = 0;
}

my_flavor_info::~my_flavor_info()
{
	dispose();
}

my_flavor_info::my_flavor_info(const my_flavor_info & other)
{
	if (&other == this) return;
	assign(other);
}

my_flavor_info &
my_flavor_info::operator=(const my_flavor_info & other)
{
	if (&other == this) return *this;
	dispose();
	assign(other);
	return *this;
}

void
my_flavor_info::dispose()
{
	delete[] in_formats;
	delete[] out_formats;
	delete[] codecs;
	free(name);
	free(info);
	in_format_count = 0;
	out_format_count = 0;
	codec_count = 0;
	in_formats = 0;
	out_formats = 0;
	name = 0;
	info = 0;
	if (internal_id != 0) {
		delete reinterpret_cast<media_file_format *>(internal_id);
	}
	internal_id = 0;
}

void
my_flavor_info::assign(const my_flavor_info & other)
{
	memcpy(this, &other, sizeof(*this));
	name = other.name ? strdup(other.name) : 0;
	info = other.info ? strdup(other.info) : 0;
	if (in_format_count) {
		in_formats = new media_format[in_format_count];
		memcpy(const_cast<media_format *>(in_formats), other.in_formats, sizeof(media_format)*in_format_count);
	}
	if (out_format_count) {
		out_formats = new media_format[out_format_count];
		memcpy(const_cast<media_format *>(out_formats), other.out_formats, sizeof(media_format)*out_format_count);
	}
	if (internal_id != 0) {
		media_file_format * mff = new media_file_format;
		*mff = *reinterpret_cast<media_file_format *>(other.internal_id);
		internal_id = reinterpret_cast<int32>(mff);
		printf("my_flavor_info::assign() from %lx to %lx\n",
				other.internal_id, internal_id);
	}
	if (codec_count != 0) {
		codecs = new media_codec_info[codec_count];
		memcpy(codecs, other.codecs, sizeof(media_codec_info)*codec_count);
	}
}

my_flavor_info &
my_flavor_info::operator+=(const media_format & format)
{
	int32 new_in_format_count = in_format_count+1;
	printf("my_flavor_info::a %d\n", new_in_format_count);
	media_format * nmf = new media_format[new_in_format_count];
	if (!nmf) return *this;
	if (in_format_count) memcpy(nmf, in_formats, sizeof(media_format)*in_format_count);
	nmf[in_format_count] = format;
	in_format_count = new_in_format_count;
	delete[] in_formats;
	in_formats = nmf;
	printf("my_flavor_info::b %d\n", in_format_count);
	return *this;
}

my_flavor_info &
my_flavor_info::operator+=(const media_codec_info & codec)
{
	int32 new_codec_count = codec_count+1;
	printf("my_flavor_info::c %d\n", new_codec_count);
	media_codec_info * nmf = new media_codec_info[new_codec_count];
	if (!nmf) return *this;
	if (codec_count) memcpy(nmf, codecs, sizeof(media_format)*codec_count);
	nmf[codec_count] = codec;
	codec_count = new_codec_count;
	delete[] codecs;
	codecs = nmf;
	printf("my_flavor_info::d %d\n", codec_count);
	return *this;
}





WriterAddOn::WriterAddOn(image_id image) : BMediaAddOn(image), m_flavorLock("WriterAddOn::m_flavorLock")
{
	FUNC(("WriterAddOn::WriterAddOn(%ld)\n", image));
	int32 cookie = 0;
	media_file_format mff;
	memset(&mff, 0, sizeof(mff));
	printf("WriterAddOn::a\n");
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		my_flavor_info * mfi = new my_flavor_info;
		printf("WriterAddOn::b (%p, %s, %s)\n", mfi, mff.short_name, mff.pretty_name);
		mfi->name = strdup(mff.short_name);
		mfi->info = strdup(mff.pretty_name);
		mfi->kinds = B_FILE_INTERFACE | B_BUFFER_CONSUMER;
		mfi->flavor_flags = 0;
		mfi->internal_id = reinterpret_cast<int32>(new media_file_format(mff));
		printf("mfi->internal_id = %p\n", mfi->internal_id);
		mfi->possible_count = 0;
		mfi->in_format_flags = 0;
		//	file formats
		media_format ramf;
		media_format aff;
		media_codec_info ei;
		//hplus	fixme later:
		//	This is not really the best implementation, but we can't ask
		//	a file format which specific classes are supported without
		//	having an input format already ready. Until that comes along,
		//	here we are.
		static media_type types[] = {
			B_MEDIA_RAW_AUDIO, B_MEDIA_RAW_VIDEO,
			// B_MEDIA_ENCODED_AUDIO, B_MEDIA_ENCODED_VIDEO,
			B_MEDIA_MIDI, B_MEDIA_PARAMETERS,
			B_MEDIA_UNKNOWN_TYPE
		};
		for (int ix=0; ix<sizeof(types)/sizeof(types[0]); ix++) {
			int32 encc = 0;
			memset(&ramf, 0, sizeof(ramf));
			ramf.type = types[ix];
			if (get_next_encoder(&encc, &mff, &ramf, &aff, &ei) == B_OK) {
				printf("WriterAddOn::c (%p)\n", mfi);
				*mfi += ramf;
				*mfi += ei;
			}
		}
		printf("WriterAddOn::d (%p)\n", mfi);
		m_flavors.push_back(mfi);
		printf("WriterAddOn::e (%d)\n", m_flavors.size());
		memset(&mff, 0, sizeof(mff));
	}
	printf("WriterAddOn::f\n");
	if (m_flavors.size() == 0) {
		m_error = B_MEDIA_BAD_FORMAT;
		m_error_str = "There are no available media writer add-ons.";
	}
	else {
		m_error = B_OK;
		m_error_str = "";
	}
}


WriterAddOn::~WriterAddOn()
{
	m_flavorLock.Lock();
	FUNC(("WriterAddOn::~WriterAddOn()\n"));
	for (vector<my_flavor_info *>::iterator ptr(m_flavors.begin()); ptr != m_flavors.end(); ptr++) {
		delete *ptr;
	}
	m_flavors.clear();
}

status_t 
WriterAddOn::InitCheck(const char **out_failure_text)
{
	FUNC(("WriterAddOn::InitCheck(0x%x)\n", out_failure_text));
	*out_failure_text = m_error_str.c_str();
	return m_error;
}

int32 
WriterAddOn::CountFlavors()
{
	BAutolock lock(m_flavorLock);
	FUNC(("WriterAddOn::CountFlavors()\n"));
	return m_flavors.size();
}

status_t 
WriterAddOn::GetFlavorAt(int32 n, const flavor_info **out_info)
{
	BAutolock lock(m_flavorLock);
	FUNC(("WriterAddOn::GetFlavorAt(%ld, 0x%lx)\n", n, out_info));
	if ((n < 0) || (n >= m_flavors.size())) {
		return B_BAD_INDEX;
	}
	*out_info = m_flavors[n];
	printf("info->internal_id = %p\n", (*out_info)->internal_id);
	return B_OK;
}

BMediaNode *
WriterAddOn::InstantiateNodeFor(const flavor_info *info, BMessage *config, status_t *out_error)
{
	BAutolock lock(m_flavorLock);
	FUNC(("WriterAddOn::InstantiateNodeFor('%s':0x%x, 0x%lx, 0x%lx)\n", info->name, info->internal_id, config, out_error));
	*out_error = B_OK;
	printf("info->internal_id = %p\n", info->internal_id);
	return new WriterNode(this, info, "WriterNode", out_error);
}

status_t 
WriterAddOn::GetConfigurationFor(BMediaNode *your_node, BMessage *into_message)
{
	FUNC(("WriterAddOn::GetConfigurationFor(%d, 0x%lx)\n", your_node->ID(), into_message));
	//hplus	fixme:
	//	we should get the current codecs and save off in that message
	return B_OK;
}

bool 
WriterAddOn::WantsAutoStart()
{
	return false;
}

status_t 
WriterAddOn::AutoStart(int in_count, BMediaNode **out_node, int32 *out_internal_id, bool *out_has_more)
{
	FUNC(("WriterAddOn::AutoStart(%d, 0x%lx, 0x%lx, 0x%lx)\n", in_count, out_node, out_internal_id, out_has_more));
	*out_has_more = false;
	return B_BAD_INDEX;
}

status_t 
WriterAddOn::SniffRef(const entry_ref &file, BMimeType *io_mime_type, float *out_quality, int32 *out_internal_id)
{
	FUNC(("WriterAddOn::SniffRef('%s', 0x%lx, 0x%lx, 0x%lx)\n", file.name, io_mime_type, out_quality, out_internal_id));
	//	we only do writing, so we shouldn't really sniff...
	return B_ERROR;
}

status_t 
WriterAddOn::GetFileFormatList(int32 flavor_id, media_file_format *out_writable_formats, int32 in_write_items, int32 *out_write_items, media_file_format *out_readable_formats, int32 in_read_items, int32 *out_read_items, void *_reserved)
{
	BAutolock lock(m_flavorLock);
	FUNC(("WriterAddOn::GetFileFormatList(%d, 0x%lx, %d, 0x%lx, 0x%lx, %d, 0x%ld, 0x%lx)\n", flavor_id, out_writable_formats, in_write_items, out_write_items, out_readable_formats, in_read_items, out_read_items, _reserved));
	if (!flavor_id) return B_BAD_INDEX;
	if (out_read_items) *out_read_items = 0;
	if (!out_writable_formats || !out_write_items || (in_write_items < 1)) return B_BAD_VALUE;
	for (vector<my_flavor_info *>::iterator ptr(m_flavors.begin()); ptr != m_flavors.end(); ptr++) {
		if ((*ptr)->internal_id == flavor_id) {
			*out_writable_formats = *reinterpret_cast<media_file_format *>(flavor_id);
			*out_write_items = 1;
			return B_OK;
		}
	}
	return B_MEDIA_BAD_FORMAT;
}

status_t 
WriterAddOn::SniffTypeKind(const BMimeType &type, uint64 in_kinds, float *out_quality, int32 *out_internal_id, void *_reserved)
{
	BAutolock lock(m_flavorLock);
	FUNC(("WriterAddOn::SniffTypeKind('%s', 0x%Lx, 0x%lx, 0x%lx, 0x%lx)\n", type.Type(), in_kinds, out_quality, out_internal_id, _reserved));
	if ((in_kinds & ~(B_BUFFER_CONSUMER | B_CONTROLLABLE | B_FILE_INTERFACE)) != 0LL) {
		return B_MEDIA_BAD_NODE;	//	we don't know other kinds, so no use trying
	}
	if ((in_kinds & B_BUFFER_CONSUMER) == 0LL) {
		return B_MEDIA_BAD_NODE;	//	we only do consumers
	}
	printf("SniffTypeKind::a %p\n", &m_flavors);
	for (vector<my_flavor_info *>::iterator ptr(m_flavors.begin()); ptr != m_flavors.end(); ptr++) {
		const media_file_format * mff = reinterpret_cast<const media_file_format *>((*ptr)->internal_id);
		printf("SniffTypeKind::b %p\n", mff);
		if (!mff) continue;
		printf("SniffTypeKind::c %p\n", type.Type());
		if (!strcasecmp(type.Type(), mff->mime_type)) {
			printf("SniffTypeKind::d %d %p\n", (*ptr)->internal_id, (*ptr)->name);
			*out_quality = 0.3;		//fixme hplus:	I wish we could have a better number here
			*out_internal_id = (*ptr)->internal_id;
			return B_OK;
		}
	}
	return B_MEDIA_BAD_FORMAT;
}


BMediaAddOn *
make_media_addon(image_id you)
{
	FUNC(("WriterAddOn: make_media_addon(%ld)\n", you));
	return new WriterAddOn(you);
}

