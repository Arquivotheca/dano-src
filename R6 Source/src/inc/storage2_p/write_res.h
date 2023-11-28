
#if !defined(WRITE_RES_H)
#define WRITE_RES_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _res_map res_map;

/* The API is slightly different from BResources. find_resource() returns a pointer which is owned */
/* by the resource library, and is only valid until you replace or dispose or remove that resource. */
/* You will have to copy it yourself if you want to hang on to it after a dispose thereof. */
/* Also, this API keeps all resources in memory, which may be bad if you have megabytes of resources. */
/* Therefore, we support marking resources as in-file until they're actually needed. */
int add_resource(res_map ** map, unsigned int type, int id, const void * data, int size, const char * name);
const void * find_resource_by_id(res_map * map, unsigned int type, int id, int * out_size, const char ** out_name);
const void * find_resource_by_name(res_map * map, unsigned int type, const char * name, int * out_size, int * out_id);
int count_resources(res_map * link);
/* find_resource_by_index() and iterate_resources() don't actually load the resource data, so they may */
/* return NULL but set the type and ID fields to the correct values. Type 0 is not allowed in a file. */
const void * find_resource_by_index(res_map * map, int index, unsigned int * type, int * id, int * out_size, const char ** out_name);
int iterate_resources(res_map * list, void ** cookie, unsigned int * type, int * id, const void ** data, int * size, const char ** name);
int remove_resource(res_map * map, const void * resource);
int remove_resource_id(res_map * map, unsigned int type, int id);
int replace_resource_data(res_map * map, const void * resource, void * new_data, int size);

/* Pass the "endian" returned by position_at_map() to write_resource_file or read_resource_file for that file. */
/* Data that needs to be swapped will be swapped by the swapper function. Pass NULL to use the standard */
/* swapper function, which knows about types in TypeCodes.h as well as APPF and APPV. */
/* When writing resources, all resources are temporarily read into memory if you're writing to the same file */
/* as resources were read from. Thus, you shouldn't often write resources. */
int load_resource_type(res_map * map, unsigned int type);	/* type = 0 means all */
void dispose_resource_map(res_map * map);
int position_at_map(int fd, int for_write, int * endian);	/* >0 means map found, 0 means empty file, <0 means error */	/* finds resource part of file, if any */
int write_resource_file(res_map * map, int fd, int endian, int (*swapper)(unsigned int type, void * data, unsigned int size));	/* writes map to file */
int new_resource_map(res_map ** map, int (*swapper)(unsigned int type, void * data, unsigned int size));	/* creates an empty map */
int read_resource_file(res_map ** map, int fd, int endian, int (*swapper)(unsigned int type, void * data, unsigned int size));	/* creates map from file */

int standard_swap(unsigned int type, void * data, unsigned int size);	/* returns non-0 if swap action was taken */

#if defined(__cplusplus)
}
#endif

#endif /* WRITE_RES_H */

