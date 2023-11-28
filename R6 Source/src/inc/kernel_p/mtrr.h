#ifndef MTRR_H
#define MTRR_H

#include <OS.h>
#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif


#define B_MTRR_MODULE_NAME			"cpu/mtrr_gen/v1"
#define B_MTRR_CPU_MODULE_DIR		"cpu/mtrr/"
#define B_MTRR_INTEL_MODULE_NAME	"cpu/mtrr/intel/v1"
#define B_MTRR_AMD_MODULE_NAME		"cpu/mtrr/amd/v1"
#define B_MTRR_CYRIX_MODULE_NAME	"cpu/mtrr/cyrix/v1"


typedef enum 
{
	UC_MEM = 0,	/* uncacheble		*/
	WC_MEM = 1, /* write combining	*/
	WT_MEM = 4,	/* write-through	*/
	WP_MEM = 5, /* write-protected	*/
	WB_MEM = 6	/* writeback		*/	
} MEM_RANGE_TYPE;

typedef struct 
{
	uint64			base;
	uint64			len;
	MEM_RANGE_TYPE	mem_type;
} mtr_info;


typedef struct
{
	module_info minfo;

	status_t (*create_mtr)(int mr_id, uint64 base, uint64 len, MEM_RANGE_TYPE mem_type, uint32 flags);
	status_t (*destroy_mtr)(int mr_id);
	status_t (*get_mtr_info)(int mr_id, mtr_info* info);
} mtrr_module;

/*** definitions for CPU-specific modules ***/

typedef enum 
{
	RANGE_CLEAR,
	RANGE_SET,
	RANGE_CLEANUP,
	MTRRS_SYNC
} RANGE_ACTION;

typedef struct
{
	module_info minfo;

	status_t (*set_mtrr)(int64 base, uint64 len, MEM_RANGE_TYPE mem_type,  int n_mtrrs, uint32 *used_mtrr);
	void (*clear_mtrr)(uint mtrr_index);
	void (*set_all_mtrrs)(const void* mtrrs);
	void* (*prepare_sync_mtrrs_info)(void);
	bool (*is_mem_range_type_supported)(MEM_RANGE_TYPE mem_type);
	void (*debug_dump_mtrrs)(void);
	void (*debug_get_mtrr_info)(uint mtrr_index, uint64* base, uint64* len, MEM_RANGE_TYPE* mem_type, bool* used);
	uint64 (*disable_mtrrs)(void);
	void (*restore_mtrrs)(uint64 mttrs_state);
	
	uint64	min_range_len;	/* minimum size of a memory range supported by the CPU */
	int		mtrr_num;		/* number of logical MTRRs */
} mtrr_cpu_module;

/* Assume that all CPUs have identical MTRRs, so don't keep flags per CPU */
typedef struct 
{
	uint64			base;	/* HACK: it's a pointer to all_mttrs struct when action == MTRRS_SYNC */
	uint64			len;
	MEM_RANGE_TYPE	mem_type;
	uint32			used_mtrrs[B_MAX_CPU_COUNT];
	int				id;	
	RANGE_ACTION	action;		
	bool			used;
} mtr_t;
/* len has to be a power of 2 and base has to be aligned
 on at least len boundary. The minimum len is 4 KByte for Intel
 and 128 kByte for AMD. */


extern void*	persistent_mtrr_info;	/* kernel var */
 

#ifdef __cplusplus
}
#endif

#endif
