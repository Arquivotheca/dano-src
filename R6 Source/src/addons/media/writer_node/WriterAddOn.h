
#if !defined(WriterAddOn_h)
#define WriterAddOn_h

#define BUILDING_MEDIA_ADDON 1


#include <MediaAddOn.h>
#include <vector>
#include <string>
#include <Locker.h>


struct my_flavor_info;


class WriterAddOn : public BMediaAddOn
{
public:
		WriterAddOn(image_id image);
		virtual ~WriterAddOn();

virtual	status_t InitCheck(
							const char ** out_failure_text);
virtual	int32 CountFlavors();
virtual	status_t GetFlavorAt(
							int32 n,
							const flavor_info ** out_info);
virtual	BMediaNode * InstantiateNodeFor(
							const flavor_info * info,
							BMessage * config,
							status_t * out_error);
virtual	status_t GetConfigurationFor(
							BMediaNode * your_node,
							BMessage * into_message);
virtual	bool WantsAutoStart();
virtual	status_t AutoStart(
							int in_count,
							BMediaNode ** out_node,
							int32 * out_internal_id,
							bool * out_has_more);
virtual	status_t SniffRef(
							const entry_ref & file,
							BMimeType * io_mime_type,
							float * out_quality,
							int32 * out_internal_id);
virtual	status_t GetFileFormatList(
							int32 flavor_id,			//	for this node flavor (if it matters)
							media_file_format * out_writable_formats, 	//	don't write here if NULL
							int32 in_write_items,		//	this many slots in out_writable_formats
							int32 * out_write_items,	//	set this to actual # available, even if bigger than in count
							media_file_format * out_readable_formats, 	//	don't write here if NULL
							int32 in_read_items,		//	this many slots in out_readable_formats
							int32 * out_read_items,		//	set this to actual # available, even if bigger than in count
							void * _reserved);			//	ignore until further notice
virtual	status_t SniffTypeKind(				//	Like SniffType, but for the specific kind(s)
							const BMimeType & type,
							uint64 in_kinds,
							float * out_quality,
							int32 * out_internal_id,
							void * _reserved);

private:

		BLocker m_flavorLock;
		vector<my_flavor_info *> m_flavors;
		string m_error_str;
		status_t m_error;
};

#endif	//	WriterAddOn_h
