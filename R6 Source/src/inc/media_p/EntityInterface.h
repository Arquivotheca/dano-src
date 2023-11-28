/*	EntityInterface.h	*/
/*	Copyright 1998 Be Incorporated. All rights reserved.	*/

#if !defined(_ENTITY_INTERFACE_H)
#define _ENTITY_INTERFACE_H

#include <MediaNode.h>


class BEntityInterface :
	public virtual BMediaNode
{
private:

virtual	~BEntityInterface();

protected:

		enum {
			B_ENTITY_NAME_LENGTH = 64
		};

		BEntityInterface();

		status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);

virtual	status_t IterateEntities(
				int32 * cookie,
				int32 * out_entity_id) = 0;
virtual	void DisposeEntityCookie(
				int32 cookie) = 0;
virtual	status_t FindEntity(
				const char * name,
				int32 * out_entity_id) = 0;

virtual	status_t GetEntityInfo(
				int32 entity_id,
				media_format * out_format,
				bigtime_t * out_start_time,
				bigtime_t * out_stop_time,
				char * out_name) = 0;
virtual	status_t GetEntityParameter(
				int32 entity_id,
				void * out_buffer,
				size_t * io_buffer_size,
				type_code * out_type) = 0;

virtual	status_t NewEntity(
				const char * name,
				media_format * io_format,
				int32 * out_entity_id) = 0;
virtual	status_t SetEntityParameter(
				int32 entity_id,
				const void * in_data,
				size_t in_size,
				type_code in_type) = 0;

};

#endif /* _ENTITY_INTERFACE_H */
