#ifndef _VOLUME_NODE_H_
#define _VOLUME_NODE_H_

#include <Binder.h>


/*=================================================================*/

class VolumeNode : public BinderContainer
{
	public:
									VolumeNode			(BLooper* mounter,
														 const char* volume_name,
														 const char* mounted_at,
														 const char* device,
														 const char* drive,
														 int block_size,
														 int total_blocks,
														 int free_blocks,
														 int removable,
														 int read_only);
		virtual	get_status_t		ReadProperty		(const char *name,
														 property &prop,
														 const property_list &args = empty_arg_list);

	private:
		BLooper*					fAutoMounterLooper;
};
#endif
