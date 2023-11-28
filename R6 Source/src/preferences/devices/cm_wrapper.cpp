#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <config_manager.h>
#include <config_driver.h>

status_t get_next_device_info(int fd, bus_type bus, uint64 *cookie,
		struct device_info *info, uint32 size)
{
	status_t result;
	struct cm_ioctl_data params;

	params.magic = CM_GET_NEXT_DEVICE_INFO;
	params.bus = bus;
	params.cookie = *cookie;
	params.data = (void *)info;
	params.data_len = size;

	result = ioctl(fd, params.magic, &params, sizeof(params));

	if (result == B_OK) *cookie = params.cookie;

	return result;
}

status_t get_device_info_for(int fd, uint64 id, struct device_info *info,
		uint32 size)
{
	struct cm_ioctl_data params;

	params.magic = CM_GET_DEVICE_INFO_FOR;
	params.cookie = id;
	params.data = (void *)info;
	params.data_len = size;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_size_of_current_configuration_for(int fd, uint64 id)
{
	struct cm_ioctl_data params;

	params.magic = CM_GET_SIZE_OF_CURRENT_CONFIGURATION_FOR;
	params.cookie = id;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_current_configuration_for(int fd, uint64 id,
		struct device_configuration *current, uint32 size)
{
	struct cm_ioctl_data params;

	params.magic = CM_GET_CURRENT_CONFIGURATION_FOR;
	params.cookie = id;
	params.data = (void *)current;
	params.data_len = size;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_size_of_possible_configurations_for(int fd, uint64 id)
{
	struct cm_ioctl_data params;

	params.magic = CM_GET_SIZE_OF_POSSIBLE_CONFIGURATIONS_FOR;
	params.cookie = id;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_possible_configurations_for(int fd, uint64 id,
		struct possible_device_configurations *possible, uint32 size)
{
	struct cm_ioctl_data params;

	params.magic = CM_GET_POSSIBLE_CONFIGURATIONS_FOR;
	params.cookie = id;
	params.data = (void *)possible;
	params.data_len = size;

	return ioctl(fd, params.magic, &params, sizeof(params));
}


status_t count_resource_descriptors_of_type(
    int fd, struct device_configuration *c, resource_type type)
{
	struct cm_ioctl_data params;

	params.magic = CM_COUNT_RESOURCE_DESCRIPTORS_OF_TYPE;
	params.config = c;
	params.type = type;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

status_t get_nth_resource_descriptor_of_type(
	int fd, struct device_configuration *c, uint32 n,
	resource_type type, resource_descriptor *d, uint32 size)
{
	struct cm_ioctl_data params;

	params.magic = CM_GET_NTH_RESOURCE_DESCRIPTOR_OF_TYPE;
	params.config = c;
	params.n = n;
	params.type = type;
	params.data = d;
	params.data_len = size;

	return ioctl(fd, params.magic, &params, sizeof(params));
}

uchar mask_to_value(uint32 mask)
{
	uchar value;
	if (!mask) return 0;
	for (value=0,mask>>=1;mask;value++,mask>>=1)
		;
	return value;
}
