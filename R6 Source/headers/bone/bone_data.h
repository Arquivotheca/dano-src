/*
	bone_data.h
	
	networking data chunk manipulation utilities
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/


#ifndef H_BONE_DATA
#define H_BONE_DATA

#include <iovec.h>
#include <KernelExport.h>

typedef void (*freefunc)(void *arg1);
typedef void (*extfreefunc)(void *arg2, void *arg1);

typedef enum
{
	FREE_INTERRUPT_SAFE = 	0x0001,
} free_flags_t;

/* bone_data flags */
enum
{
	/* flags masks */
	DATA_MSG_FLAGS =		0x0FFF,		/* socket msg flags */
	DATA_FLAGS =			0xF000,		/* bone data flags */
	
	/* user-space data flags */
	DATA_USERSPACE = 		0x1000,		/* contains user-space data */
	DATA_LOCKED = 			0x2000,		/* user-space data has been locked; undefined if not DATA_USERSPACE */
	
	/* checksum flags */
	DATA_DO_CKSUM = 		0x4000,		/* automatically maintain checksum on data */
	DATA_CKSUM = 			0x8000,		/* checksum is valid */
};

typedef struct bone_data
{
	struct bone_data		*next;				/* do _NOT_ move this to another location; it is used by the atomic list functions */
	
	struct sockaddr 		*src;
	struct sockaddr 		*dst;
	
	struct bone_mem_node 	*datalist;			/* the current memory image */
	struct bone_mem_node 	*freelist;			/* reference counted list of free elements */
	size_t					datalen;			/* the length of the data */
	uint32 					sequence;			/* the sequence number, offset, or other protocol specific data */
	uint16					flags;			    /* data flags */
	uint16					cksum;				/* the internet checksum of data; valid if DATA_CKSUM bit is set */
	struct mn_pool			*mnp;				/* bone_mem_node pool */
} bone_data_t;

#endif /* H_BONE_DATA */
