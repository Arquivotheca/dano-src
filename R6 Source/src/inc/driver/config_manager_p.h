#ifndef _CONFIG_MANAGER_P_H
#define _CONFIG_MANAGER_P_H

#define B_IRQ_PCI_SHAREABLE		0x80000000
#define B_IRQ_ISA_SHAREABLE		0x40000000

#define NEXT_POSSIBLE(c) \
	(c) = (struct device_configuration *)((uchar *)(c) + sizeof(struct device_configuration) \
			+ (c)->num_resources * sizeof(resource_descriptor));

#define NEXT_POSSIBLE_TYPED(c,TYPE) \
	(c) = (TYPE *)((uchar *)(c) + sizeof(struct device_configuration) \
			+ (c)->num_resources * sizeof(resource_descriptor));

struct device_configuration_container {
	uint32 magic;
	uint32 num_allocated;
	struct device_configuration *c;
};

typedef struct config_manager_for_bus_module_info {
	module_info	minfo;
	status_t	(*assign_configuration)(
						const struct possible_device_configurations *possible,
						struct device_configuration **assigned);
	status_t	(*unassign_configuration)(
						const struct device_configuration *config);

	/* helper routines for busses */
	status_t	(*count_resource_descriptors_of_type)(
						const struct device_configuration *config,
						resource_type type);
	status_t	(*get_nth_resource_descriptor_of_type)(
						const struct device_configuration *config, uint32 n,
						resource_type type, resource_descriptor *descriptor,
						uint32 len);
	status_t	(*remove_nth_resource_descriptor_of_type)(
						struct device_configuration *config, uint32 n,
						resource_type type);

	status_t	(*new_device_configuration_container)(
						struct device_configuration_container **container);
	void		(*delete_device_configuration_container)(
						struct device_configuration_container **container);
	status_t	(*add_to_device_configuration_container)(
						struct device_configuration_container *container,
						resource_descriptor *r);

	status_t	(*add_to_possible_device_configurations)(
						struct possible_device_configurations **possible,
						struct device_configuration *config);
} config_manager_for_bus_module_info;

#define	B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME	"bus_managers/config_manager/bus/v1"

typedef struct config_manager_bus_module_info {
	module_info	minfo;
	status_t	(*get_next_device_info)(
						uint32 *cookie, struct device_info *info, uint32 len);
	status_t	(*get_device_info_for)(
						uint32 id, struct device_info *info, uint32 len);

	status_t	(*set_device_configuration_for)(uint32 id,
						const struct device_configuration *config);

	status_t	(*get_size_of_current_configuration_for)(uint32 id);
	status_t	(*get_current_configuration_for)(uint32 id,
						struct device_configuration *config, uint32 len);

	status_t	(*get_size_of_possible_configurations_for)(uint32 id);
	status_t	(*get_possible_configurations_for)(uint32 id,
						struct possible_device_configurations *possible,
						uint32 len);

	status_t	(*enable_device)(uint32 id);
	status_t	(*disable_device)(uint32 id, status_t reason);
} config_manager_bus_module_info;

/* vyt: rename? */
#define B_JUMPERED_CONFIG_MANAGER_BUS "busses/config_manager/jumpered/v1"
#define B_PCI_CONFIG_MANAGER_BUS "busses/config_manager/pci/v1"
#define B_ISA_ONBOARD_CONFIG_MANAGER_BUS "busses/config_manager/isa/onboard/v1"
#define B_ISA_PNP_CONFIG_MANAGER_BUS "busses/config_manager/isa/pnp/v1"

#define CONFIG_MANAGER_DRIVER_SETTINGS_FILE "config_manager"

#endif
