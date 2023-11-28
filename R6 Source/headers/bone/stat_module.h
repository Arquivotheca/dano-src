
/*
	stat_module.h
	
	statistics module
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_STAT_MODULE
#define H_STAT_MODULE

#include <OS.h>
#include <fs_attr.h>
#include <module.h>


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STAT_TYPES 256

typedef struct stat_class
{
	char 		*mime_type;
	char 		*attrs[MAX_STAT_TYPES];
	int32		attr_types[MAX_STAT_TYPES];
	int			num_types;
	status_t	(*stat)(void *data, int attr_index, attr_info *ai);
	status_t	(*read)(void *data, int attr_index, void *outdata, size_t *outlen);
	status_t	(*write)(void *data, int attr_index, void *val, size_t *vallen);
	
} stat_class_t;


typedef struct stat_module_info
{
	struct module_info info;

	void	 *(*register_stat)(stat_class_t *sc, char *path, void *data);
	void	 (*deregister_stat)(void *cookie);
	
} stat_module_info_t;

#define STATS_MODULE "stats/stat_module"

#ifdef __cplusplus
}
#endif


#endif
