//******************************************************************************
//
//	File:		datafile.h
//
//	Description:	Datafile structure
//	
//
//	Copyright 1993, Be Incorporated
//
//	Change History:
//
//			bgs	new today
//			bgs	Re-written as C++ class
//			bgs	added caching on df_Entries
//
//******************************************************************************/

#ifndef	RESOURCE_H
#define	RESOURCE_H

#include <File.h>

/*------------------------------------------------------------------*/

struct rsrc_desc {
		char		*next;
		long		id;
		long		df_id;
		char		*name;
};

typedef	rsrc_desc	*rsrc_desc_ptr;

/*------------------------------------------------------------------*/

struct rsrc_map {
		char				*next;
		unsigned long		type;
		rsrc_desc			*first_entry;
};

typedef	rsrc_map	*map_ptr;

/*------------------------------------------------------------------*/

#define	SIGNATURE		0x444f
#define	VERSION			0x1000

#define	RES_CONTID		'resf'

#define INITIAL_DIR_SIZE        128		/* must be a mult of DIR_GROW  */
#define INITIAL_HOLE_SIZE       30      /* must be a mult of HOLE_GROW */

#define DIR_GROW                160
#define BIG_TRESH1              300
#define DIR_GROW1               600	

#define BIG_TRESH2              4000
#define DIR_GROW2               1000

#define HOLE_GROW               40
#define	HOLE_GROW_TRESH			100
#define	HOLE_GROW1				400
#define HOLE_BUFFER				80
#define MIN_HOLE                10

#define	COPY_SIZE				32768
#define	RESERVE_COUNT			10

//----------------------------------------------------------------------

#define	FIRST_ENTRY				2

//----------------------------------------------------------------------

#define	DF_CACHE_SIZE			64
#define	DF_MAX_CACHE_SIZE		256

//----------------------------------------------------------------------

typedef struct {
        long    offset;
        long    size;
		long	type;
}       df_entry;

/*------------------------------------------------------------------*/

typedef struct {
        long    offset;
        long    size;
		long	type;
		long	index;
}       sort_entry;

/*------------------------------------------------------------------*/

typedef struct  {
		long	signature;
		long	record_count;
        long    offset_table_ptr;
        long    hole_table_ptr;
        long    free_entry_hint;
		long	dirty_count;
		long	unique;
        long    reserved[10];
}       df_header;

//----------------------------------------------------------------------

typedef struct {
		long	offset;
		long	modif;
	} df_shared;

//----------------------------------------------------------------------

#define	recref	long

//----------------------------------------------------------------------

class	RTDF	{
public:

// datafile level calls.

				RTDF();
				~RTDF();

	int			open_df(BFile *file, bool read_only);
	int			create_df(BFile *file);	
	int			close_df();
	long		get_count();
	int			flush();
	
	long		determine_file_specs();
	long		create_non_empty_file();
	long		create_header();
	long		read_header();

// record level calls.

	recref		add(const void *data, long type, long size, long reserved_id);
	long		read_part(recref ref, long pos, long size, void *buffer);
	long		write_part(recref ref, long pos, long size, const void *buffer);
	long		set_size(recref ref, long size, char no_move);
	int			remove(recref ref);
	int			get(recref ref, void *buffer, long max_size);
	int			get_debug(recref ref, void *buffer, long max_size);
	long		get_size(recref ref);
	long		get_type(recref ref);
	recref		reserve_offset(long type);
	void		max_cache();
	void		normal_cache();
	long		get_unique();
	long		get_sem();
	long		get_db_info_ptr();
	long		compress();
private:
	bool		is_file_empty();
	long    	write_at(long where, const void *buffer, long size);
	long    	read_at(long where, void *buffer, long size);
	long    	move_block(long from, long to, long size);
	long    	resize_hole_table(long new_size);
	long    	resize_offset_table(long new_size);
	long		get_record_offset(long ref);
	void		inval_cache(long ref);
	long    	read_hole(long entryno, df_entry *entry);
	long		find_hole_starting(long block_end);
	long    	find_hole(long size);
	long    	read_offset(long entryno, df_entry *entry);
	long    	write_offset(long entryno, df_entry *entry);
	long    	write_hole(long entryno, df_entry *entry);
	long    	get_new_space(long size_req, df_entry *hint_entry, df_entry *new_entry);
	long    	add_offset(df_entry *entry);
	long		merge_hole(df_entry *hole_entry);
	long    	add_hole(df_entry *entry);
	long    	new_block(long type, long size, df_entry *block_info, long reserved_id);
	int			rewrite_offsets();
	int			load_offset_table();
	void		sort_offset_table();
	void		sort_by_entry();
	long		adjust_end_of_file();
	long		rewrite_hole_table();
	long		rewrite_offset_table();
	long		move_record(long i, long current_pos);
	void		do_clear_entry(long i);
	long		start_of_data();
public:
	BFile		*file;
	char		is_read_only;
private:
	long		sec_offset;
	char		empty;
	char		was_empty;

	long		sem;
	long    	offset_table_ptr;
	long    	hole_table_ptr;
	long    	offset_table_size;
	long    	hole_table_size;
	long    	free_entry_hint;
	long    	biggest_hole;
	long		record_count;

	long		cache_base;
	long		df_cache_size;
	
	char		cache_on;
	char		*data_cache;
	char		header_dirty;
	char		file_dirty;
	long		data_cache_base;
	long		data_cache_size;
	long		unique;
	df_shared	*shared_info;
	sort_entry	*table;

	df_entry	cache_df[DF_MAX_CACHE_SIZE];
};

//----------------------------------------------------------------------

#endif
