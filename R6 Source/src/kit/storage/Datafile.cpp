//******************************************************************************
//
//      File:           datafile.cpp
//
//      Description:    Data Storage class
//      
//      Written by:     Benoit Schillings
//
//      Copyright 1992-4 , Be Incorporated
//
//      Change History:
//                      Too many to mention here.
//
//
//******************************************************************************/
//
/*
	Datafile structure :

	Header block :
		- Offset table ptr	-------|
		- Hole table ptr	----|  |
		- Record count          |  |
							    |  |
		HOLE TABLE <------------   |
								   |
								   |
		OFFSET TABLE <--------------						
			-entry 0 is the offset table itself
			-entry 1 in the hole table itself


Entry in hole table :
---------------------
        long    offset;
        long    size;
		long	type;
*/

/*------------------------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <OS.h>
#include <Debug.h>

#include "Datafile.h"

#if __INTEL__
#include <pe.h>
#elif __POWERPC__
#include <pef.h>
#endif

/*------------------------------------------------------------------*/

#define	SYSTEM_ENTRY	1001

/*------------------------------------------------------------------*/
#define	parent( i)	(((i)-1) / 2)
#define	lchild( i)	((i)*2 + 1)
#define	rchild( i)	((i)*2 + 2)

static void
_swap_(unsigned char *p, unsigned char *q, unsigned int s)
{
	unsigned char t;

	while (s--) {
		t = *p;
		*p++ = *q;
		*q++ = t;
	}
}

static void
heapup(unsigned char a[], unsigned int n, int i, int s, int	(*c)(void *, void *))
{
	unsigned int j, k;

	for(;;) {
		j = lchild( i);
		if (j >= n)
			break;
		k = rchild( i);
		if ((k < n)
		&& ((*c)( a+s*j, a+s*k) < 0))
			j = k;
		if ((*c)( a+s*i, a+s*j) >= 0)
			break;
		_swap_( a+s*i, a+s*j, s);
		i = j;
	}
}

static void
_hsort_(void *a, unsigned int num, unsigned int size, int (*compare)(void *, void *))
{
	unsigned int i;
	unsigned char *array = (unsigned char *)a;

	if (num < 2)
		return;
	i = parent( num-1);
	for(;;) {
		heapup( array, num, i, size, compare);
		if (!i)
			break;
		--i;
	}
	while (--num) {
		_swap_( array+size*num, array, size);
		heapup( array, num, 0, size, compare);
	}
}
/*------------------------------------------------------------------*/

static long	cget_unique()
{
	long	unique;

	do {
		unique = system_time()+time(0);
	} while(unique == -1);

	return(unique);
}

/*------------------------------------------------------------------*/

long    RTDF::move_block(long from, long to, long size)
{
		char    *buffer;
		long    buffer_size;
		long    to_copy;
		char    ok;
		long    result;
		long    size0 = size;
		long    to0 = to;

		cache_on = 0;
		inval_cache(-1);

		if (from == to)
				return(0);
		if (size <= COPY_SIZE) {
				buffer_size = size;
		}
		else {
				buffer_size = COPY_SIZE;
		}

		ok = 0;

		while ((ok == 0) && (buffer_size > 0)) {
				buffer = (char *)malloc(buffer_size); 
				if (buffer == 0) {
						buffer_size >>= 1;
				}
				else {
						ok = 1;
				}
		}
		if (buffer_size == 0)
				return(-1);

		while (size > 0) {
				if (size < buffer_size)
						to_copy = size;
				else
						to_copy = buffer_size;
				result = read_at(from, buffer, to_copy);
				if (result < 0) {
						SERIAL_PRINT(("copy ERROR !!!\n"));
						free(buffer);
						return(-1);
				}
				result = write_at(to, buffer, to_copy);
				from += to_copy;
				to   += to_copy;
				size -= to_copy;
		}
		free(buffer);

		return(0);
}


/*------------------------------------------------------------------*/

long    RTDF::read_at(long where, void *buffer, long size)
{
		int     result;

		
		if ((cache_on) && (size < data_cache_size)) {
				if ((data_cache_base >= 0) && 
					(where >= data_cache_base) &&
					((where + size) < (data_cache_base + data_cache_size))) {
						memcpy(buffer,
							   data_cache + (where - data_cache_base),
							   size);
						return(0);
				}
				result = file->ReadAt(sec_offset+where,
					data_cache, data_cache_size);

				if (result < 0) {       
						printf("read error %ld %ld!!!!!!\n", where, size);
						return(-1);
				}
				memcpy(buffer, data_cache, size);
				data_cache_base = where;
				return(0);
		}
		

		result = file->ReadAt(sec_offset+where, buffer, size);
		if (result == size)
				return(0);
		printf("read error %ld %ld!!!!!!\n", where, size);
		return(-1);
}

/*------------------------------------------------------------------*/

long    RTDF::write_at(long where, const void *buffer, long size)
{
		int     result;

		inval_cache(-1);
		result = file->WriteAt(sec_offset+where, buffer, size);
		if (result == size)
				return(0);
		printf("write error!!!!!!!!!\n");
		return(-1);
}

/*------------------------------------------------------------------*/

long    RTDF::read_hole(long entryno, df_entry *entry)
{
		long    offset;
		long    result;

		if (entryno >= hole_table_size)
				return(-1);

		offset = hole_table_ptr + (entryno * sizeof(df_entry));

		result = read_at(offset, entry, sizeof(df_entry));
		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::find_hole_starting(long block_end)
{
		df_entry        entries[HOLE_BUFFER];
		long            i;
		long            j;
		long            cnt;
		long            offset;
		long            error;

		i = hole_table_size;
		offset = 0;

		while (i > 0) {
				if (i > HOLE_BUFFER)
						cnt = HOLE_BUFFER;
				else
						cnt = i;

				error = read_at(
								hole_table_ptr + offset * sizeof(df_entry),
								entries, cnt * sizeof(df_entry));
				if (error < 0)
						return(-1);

				for (j = 0; j < cnt; j++) {
						if ((entries[j].offset == block_end) &&
							(entries[j].size > 0)) {
								return(offset + j);
						}
				}

				offset += cnt;
				i -= cnt;
		}
		return(-1);
}

/*------------------------------------------------------------------*/

long    RTDF::find_hole(long size)
{
		long            i;
		long            j;
		long            cnt;
		long            offset;
		long            error;
		long            big;
		long            entry_size;
		df_entry        entries[HOLE_BUFFER];

		if (size > biggest_hole)
				return(-1);

		big = 0;

		i = hole_table_size;
		offset = 0;

		while (i > 0) {
				if (i > HOLE_BUFFER)
						cnt = HOLE_BUFFER;
				else
						cnt = i;

				error = read_at(
								hole_table_ptr + offset * sizeof(df_entry),
								entries, cnt * sizeof(df_entry));
				if (error < 0)
						return(-1);

				for (j = 0;j < cnt;j++) {
						entry_size = entries[j].size;
						if (entry_size > big)
								big = entries[j].size;
						
						if (entry_size >= size) {
								return(offset + j);
						}
				}

				offset += cnt;
				i -= cnt;
		}

		biggest_hole = big;
		return(-1);
}

/*------------------------------------------------------------------*/

long    RTDF::read_offset(long entryno, df_entry *entry)
{
		long    offset;
		long    result;
		long    cnt;

		if ((entryno >= offset_table_size) || (entryno < 0))
				return(-1);
		
		if ((cache_base >= 0) && (entryno >= cache_base) &&
		   (entryno < (cache_base + df_cache_size))) {
				*entry = cache_df[entryno - cache_base];
				return(0);
		}

		offset = offset_table_ptr + (entryno * sizeof(df_entry));

		cache_base = -1;
		cnt = df_cache_size;

		if ((offset_table_size - entryno) < cnt) {
				cnt = offset_table_size - entryno;
		}

		result = read_at(
						 offset,
						 &(cache_df[0]),
						 sizeof(df_entry) * cnt);

		if (result < 0)
				return(-1);

		cache_base = entryno;
		*entry = cache_df[0];

		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::write_offset(long entryno, df_entry *entry)
{
		long    offset;
		long    result;

		inval_cache(entryno);

		if (entryno >= offset_table_size) {
				result = resize_offset_table(offset_table_size + DIR_GROW);
				rewrite_offsets();
				if (result < 0)
						return(-1);
		}

		offset = offset_table_ptr + (entryno * sizeof(df_entry));
		result = write_at(offset, entry, sizeof(df_entry));
		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::write_hole(long entryno, df_entry *entry)
{
		long    offset;
		long    result;

/* don't keep track of holes smaller than <MIN_HOLE> */

		if (entry->size < MIN_HOLE)
				entry->size = -1;

		if (entry->size > biggest_hole)
				biggest_hole = entry->size;

		if (entryno >= hole_table_size) {
				result = resize_hole_table(hole_table_size + HOLE_GROW);
				if (result < 0)
						return(-1);
		}
		offset = hole_table_ptr + (entryno * sizeof(df_entry));
		result = write_at(offset, entry, sizeof(df_entry));
		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::get_new_space(long size_req, df_entry *hint_entry, df_entry *new_entry)
{
		long      i;
		long      block_start;
		long      block_end;
		long      increase;
		long      result;
		df_entry  hole;
		off_t	  size;
		char	  c;

		c = 0;
		if (file->GetSize(&size))
			return -1;
		size -= sec_offset;

		if (hint_entry != 0) {    /*if we've got a hint   */
				increase    = size_req - hint_entry->size;
				block_start = hint_entry->offset;
				block_end   = block_start + hint_entry->size;
				if (block_end == size) {
						new_entry->offset = block_start;
						new_entry->size   = size_req;
						result = file->WriteAt(sec_offset+size+increase-1, &c, 1);
						if (result != 1)
								return(-1);
						return(0);
				}

				/*check for a hole starting at the end of the cur block*/

				if (increase <= biggest_hole) {
						i = find_hole_starting(block_end);
						if (i >= 0) {
								if (read_hole(i, &hole) < 0)
										return(-1);
								if (increase <= hole.size) {
										hole.size -= increase;
										hole.offset += increase;
										if (write_hole(i, &hole)<0)
												return(-1);
										new_entry->offset = block_start;
										new_entry->size   = size_req;
										return(0);
								}
						}
				}
		}

		
		result = find_hole(size_req);
		if (result >= 0) {
				if (read_hole(result, &hole) < 0)
						return(-1);
				new_entry->offset = hole.offset;
				new_entry->size   = size_req;
				hole.size   -= size_req;
				hole.offset += size_req;
				if (write_hole(result, &hole) < 0)
						return(-1);
				return(0);
		}
		
		new_entry->offset = size;
		new_entry->size   = size_req;
		result = file->WriteAt(sec_offset+size+size_req-1, &c, 1);
		if (result != 1)
				return(-1);
		return(0);
}

/*------------------------------------------------------------------*/

long    RTDF::add_offset(df_entry *entry)
{
		long            i;
		long            start;
		df_entry        o_entry;
		int             result;

		start = free_entry_hint;
		if (start < RESERVE_COUNT)
				start = RESERVE_COUNT;

		for (i = start; i < offset_table_size; i++) {
				if (read_offset(i, &o_entry) < 0)
						return(-1);
				if (o_entry.size < 0) {
						free_entry_hint = i + 1;
						result = write_offset(i, entry);
						if (result < 0)
								return(-1);
						else
								return(i);
				}
		}

		inval_cache(cache_base);
		result = resize_offset_table(offset_table_size + DIR_GROW);
		rewrite_offsets();
		if (result < 0)
				return(-1);
		return(add_offset(entry));
}

/*------------------------------------------------------------------*/

long    RTDF::merge_hole(df_entry *hole_entry)
{
		df_entry        entry;
		df_entry        buffer[HOLE_BUFFER];
		long            i;
		long            j;
		long            offset;
		long            cnt;
		long            result;

		entry.size   = hole_entry->size;
		entry.offset = hole_entry->offset;

		i = hole_table_size;
		offset = 0;

		while (i > 0) {
				if (i > HOLE_BUFFER)
						cnt = HOLE_BUFFER;
				else
						cnt = i;

				result = read_at(
								 hole_table_ptr + offset * sizeof(df_entry),
								&buffer,
								cnt * sizeof(df_entry));
				if (result < 0)
						return(-1);
							  
				for (j = 0; j < cnt; j++)
						if (buffer[j].size > 0) {
								if (buffer[j].offset == (entry.offset + entry.size)) {
										entry.size += buffer[j].size;
										write_hole(offset + j, &entry);
										if (entry.size > biggest_hole)
												biggest_hole = entry.size;
										return(0);
								}
								if ((buffer[j].offset + buffer[j].size) == entry.offset) {
										buffer[j].size += entry.size;
										write_hole(offset + j, &buffer[j]);
										if (buffer[j].size > biggest_hole)
												biggest_hole = buffer[j].size;
										return(0);
								}
						}
				
				offset += cnt;
				i -= cnt;
		}
		return(-1);
}

/*------------------------------------------------------------------*/

long    RTDF::add_hole(df_entry *entry)
{
		long            i;
		long            n;
		long            j;
		long            offset;
		long            cnt;
		long            error;
		long            entry_size;
		int             result;
		df_entry        entries[HOLE_BUFFER];

		
		if (merge_hole(entry) == 0)
				return(0);

		if (entry->size < MIN_HOLE)
				return(0);
		

		n = hole_table_size;
		offset = 0;
		i = 0;

		while (n > 0) {
				if (n > HOLE_BUFFER)
						cnt = HOLE_BUFFER;
				else
						cnt = n;

				error = read_at(
								hole_table_ptr + offset * sizeof(df_entry),
								entries, cnt * sizeof(df_entry));
				if (error < 0)
						return(-1);

				for (j = 0;j < cnt;j++) {
						i++;
						entry_size = entries[j].size;
						if (entry_size <= 0) {
								result = write_hole(i, entry);
								if (result < 0)
										return(-1);
								return(0);
						}
				}

				offset += cnt;
				n -= cnt;
		}
		
		free_entry_hint = offset_table_size;
		result = resize_hole_table(hole_table_size + HOLE_GROW);
		if (result < 0)
				return(-1);
		return(add_hole(entry));
}

/*------------------------------------------------------------------*/

long    RTDF::resize_hole_table(long newsize)
{
		df_entry        an_entry;
		df_entry        new_entry;
		df_entry        clear_entry;
		long            result;
		long            old_size;
		long            i;

		if (newsize > HOLE_GROW_TRESH)
				newsize += HOLE_GROW1;
		printf("hole size = %ld\n", newsize);
		old_size = hole_table_size;
		result = read_offset(1, &an_entry);
		if(result < 0)
				return(-1);

		result = get_new_space(newsize * sizeof(df_entry), &an_entry, &new_entry);

		if (result < 0)
				return(-1);

		if (new_entry.offset != an_entry.offset) {
				move_block(an_entry.offset, new_entry.offset, an_entry.size);
		}

		result = write_offset(1, &new_entry);
		hole_table_ptr = new_entry.offset;
		hole_table_size = newsize;

		clear_entry.offset = 0;
		clear_entry.size = 0;
		for (i = old_size; i < newsize; i++) {
				result = write_hole(i, &clear_entry);
				if (result < 0)
						return(result);
		}
		if (new_entry.offset != an_entry.offset) {
				add_hole(&an_entry);
		}
		return(result);
}

/*-----------------------------------------------------------------*/

long    RTDF::resize_offset_table(long newsize)
{
		df_entry        an_entry;
		df_entry        new_entry;
		long            result;
		long            old_size;
		long            i;

		old_size = offset_table_size;
		if (old_size > BIG_TRESH1)
				newsize += DIR_GROW1;
		if (old_size > BIG_TRESH2)
				newsize += DIR_GROW2;
		printf("resize to %ld\n", newsize);
		
		result = read_offset(0L, &an_entry);
		if (result < 0)
				return(-1);

		result = get_new_space(newsize * sizeof(df_entry), &an_entry, &new_entry);
		if (result < 0)
				return(-1);

		offset_table_ptr = new_entry.offset;
		offset_table_size = newsize;

		if (an_entry.offset != new_entry.offset) {
				move_block(an_entry.offset, new_entry.offset, an_entry.size);
				add_hole(&an_entry);
		}

		result = write_offset(0L, &new_entry);

		new_entry.offset = 0;
		new_entry.size = -1;
		new_entry.type = -1;
		for (i = old_size; i < newsize; i++) {
				result = write_offset(i, &new_entry);
				if (result < 0)
						return(result);
		}
		inval_cache(cache_base);

		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::get_record_offset(long ref)
{
		df_entry        blk_desc;
		long            result;

		ref += RESERVE_COUNT;
		result = read_offset(ref, &blk_desc);
		if (result < 0)
				return(-1);
		return(blk_desc.offset);
}

/*------------------------------------------------------------------*/

long    RTDF::get_count()
{
		return(offset_table_size - RESERVE_COUNT);
}

/*------------------------------------------------------------------*/

long    RTDF::get_type(recref ref)
{
		df_entry        blk_desc;
		long            result;

		if (empty)
			return(-1);
		
		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		cache_on = 1;   
		ref += RESERVE_COUNT;

		result = read_offset(ref, &blk_desc);
		release_sem(sem);

		if (result < 0)
				return(-1);     
		return(blk_desc.type);
}

/*------------------------------------------------------------------*/

long    RTDF::get_size(recref ref)
{
		df_entry        blk_desc;
		long            result;

		if (empty)
			return(-1);

		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		cache_on = 1;   
		ref += RESERVE_COUNT;

		result = read_offset(ref, &blk_desc);
		release_sem(sem);
		if (result < 0)
				return(-1);     
		return(blk_desc.size);
}

/*------------------------------------------------------------------*/

long    RTDF::set_size(recref ref, long size, char no_move)
{
		long            result;
		df_entry        blk_desc;
		df_entry        new_desc;


		cache_on = 0;   
		ref += RESERVE_COUNT;
		
		inval_cache(ref);
		
		result = read_offset(ref, &blk_desc);
		if (result < 0)
				return(-1);

		if (blk_desc.size == 0) {
				result = get_new_space(size, 0, &new_desc);
				new_desc.type = blk_desc.type;
				write_offset(ref, &new_desc);
				return(0);
		}
		
		if (size == blk_desc.size)
				return(0);

		if (size > blk_desc.size) {
				result = get_new_space(size, &blk_desc, &new_desc);
				if (result < 0)
						return(-1);
				if (new_desc.offset != blk_desc.offset) {
						if (!no_move) {
								move_block(
										   blk_desc.offset,
										   new_desc.offset,
										   blk_desc.size);
						}
						add_hole(&blk_desc);
				}
				new_desc.type = blk_desc.type;
				write_offset(ref, &new_desc);
				return(0);
		}
		
/* reduce record size */

		new_desc.offset = blk_desc.offset + size;
		new_desc.size   = blk_desc.size - size;
		result = add_hole(&new_desc);
		if (result < 0)
				return(-1);
 
		blk_desc.size = size;
		result = write_offset(ref, &blk_desc);
		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::new_block(long type, long size, df_entry *block_info, long reserved_id)
{
		long            result;
		df_entry        entry;


		cache_on = 0;   
		header_dirty = 1;
		result = get_new_space(size, (df_entry *)0, &entry);
		if (result < 0)
				return(-1);

		entry.type = type;
		if (reserved_id > 0) {
			write_offset(reserved_id+RESERVE_COUNT, &entry);
			result = reserved_id+RESERVE_COUNT;
		}
		else
			result = add_offset(&entry);

		block_info->offset = entry.offset;
		block_info->size   = entry.size;
		block_info->type   = type;
		record_count++;
		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::reserve_offset(long type)
{
		df_entry        entry;
		long            result;

		cache_on = 0;   
		entry.size = 0;
		entry.offset = 0;
		entry.type = type;

		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		header_dirty = 1;
		result = add_offset(&entry);
		release_sem(sem);
		return(result - RESERVE_COUNT);
}

/*------------------------------------------------------------------*/

void    RTDF::inval_cache(long ref)
{
		data_cache_base = -10000;

		if ((ref < cache_base) || (ref > (cache_base + df_cache_size)))
				return;

		cache_base = -10000;        
}

/*------------------------------------------------------------------*/

long    RTDF::add(const void *dataptr, long type, long size, long reserved_id)
{
		df_entry        blk_desc;
		long            ref;
		long            result;
		df_header		header;


		while (acquire_sem(sem) == B_INTERRUPTED)
			;

		if (empty)
			create_header();
		if (!file_dirty) {
			cache_on = 0;
			file_dirty = true;
			header.signature = (SIGNATURE << 16) | VERSION;
			header.unique = unique;
			header.offset_table_ptr = offset_table_ptr;
			header.hole_table_ptr = hole_table_ptr;
			header.free_entry_hint = free_entry_hint;
			header.record_count = record_count;     
			header.dirty_count = 1;
			write_at(0, &header, sizeof(header));
		}

		header_dirty = 1;
		cache_on = 0;   
		ref = new_block(type, size, &blk_desc, reserved_id);

		if (ref < 0) {
				release_sem(sem);
				return(-1);
		}
		result = write_at(blk_desc.offset, dataptr, size);
		release_sem(sem);

		if (result < 0) {
				return(-1);
		}
		return(ref - RESERVE_COUNT);
}

/*------------------------------------------------------------------*/

int     RTDF::remove(recref ref)
{
		long            result;
		df_entry        blk_desc;
		df_header		header;

		if (empty)
			return(-1);
		if (!file_dirty) {
			cache_on = 0;
			file_dirty = true;
			header.signature = (SIGNATURE << 16) | VERSION;
			header.unique = unique;
			header.offset_table_ptr = offset_table_ptr;
			header.hole_table_ptr = hole_table_ptr;
			header.free_entry_hint = free_entry_hint;
			header.record_count = record_count;     
			header.dirty_count = 1;
			write_at(0, &header, sizeof(header));
		}

		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		header_dirty = 1;
		cache_on = 0;   
		ref += RESERVE_COUNT;

		inval_cache(ref);

		result = read_offset(ref, &blk_desc);
		if (result < 0) {
				release_sem(sem);
				return(-1);
		}

		result = add_hole(&blk_desc);
		if (result < 0) {
				release_sem(sem);
				return(-1);
		}
		
		blk_desc.size = 0;
		result = write_offset(ref, &blk_desc);
		record_count--;
		release_sem(sem);

		return(result);
}

/*------------------------------------------------------------------*/

long    RTDF::read_part(recref ref, long pos, long size, void *buffer)
{
		long            result;
		df_entry        blk_desc;

		if (empty)
			return(-1);
		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		ref += RESERVE_COUNT;
		result = read_offset(ref, &blk_desc);
		if (result < 0) {
				release_sem(sem);
				return(-1);
		}

		if ((pos + size) > (blk_desc.size)) {
				size = blk_desc.size - pos;
				if (size < 0) {
						release_sem(sem);
						return(0);      
				}
		}

		result = read_at(
						 blk_desc.offset + pos,
						 buffer,
						 size);
		release_sem(sem);

		if (result < 0)
				return(-1);

		return(size);
}

/*------------------------------------------------------------------*/

long    RTDF::write_part(recref ref, long pos, long size, const void *buffer)
{
		long            result;
		df_entry        blk_desc;
		char            no_move = 0;
		df_header		header;


		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		if (empty)
			create_header();
		if (!file_dirty) {
			cache_on = 0;
			file_dirty = true;
			header.signature = (SIGNATURE << 16) | VERSION;
			header.unique = unique;
			header.offset_table_ptr = offset_table_ptr;
			header.hole_table_ptr = hole_table_ptr;
			header.free_entry_hint = free_entry_hint;
			header.record_count = record_count;     
			header.dirty_count = 1;
			write_at(0, &header, sizeof(header));
		}

		header_dirty = 1;
		cache_on = 0;   
		ref += RESERVE_COUNT;
		
		inval_cache(ref);
		
		
		result = read_offset(ref, &blk_desc);
		if (result < 0) {
				release_sem(sem);
				printf("read offset failed in write_part\n");
				return(-1);
		}
		
		if ((pos + size) > (blk_desc.size)) {
				if (pos == 0)
						no_move = 1;
				result = set_size(ref - RESERVE_COUNT, pos + size, no_move);
				if (result < 0) {
						release_sem(sem);
						printf("set size failed in write_part\n");
						return(-1);
				}
				read_offset(ref, &blk_desc);
		}
		
		result = write_at(
						  blk_desc.offset + pos,
						  buffer,
						  size);

		
		if (result < 0)
				printf("write at failed in write_part\n");
		release_sem(sem);

		return(size);
}

/*------------------------------------------------------------------*/

int    RTDF::get(recref ref, void *buffer, long max_size)
{
		df_entry        blk_desc;
		long            result;
		long            size;

		if (empty)
			return(-1);

		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		cache_on = 1;   
		ref += RESERVE_COUNT;

		result = read_offset(ref, &blk_desc);
		
		if (result < 0) {
				release_sem(sem);
				return(-1);
		}
		
		size = blk_desc.size;

		if (max_size < size)
				size = max_size;

		result = read_at(blk_desc.offset, buffer, size);

		release_sem(sem);

		return(result);
}


/*------------------------------------------------------------------*/

void    RTDF::max_cache()
{
		df_cache_size = DF_MAX_CACHE_SIZE;
		cache_base = -1;        
}

/*------------------------------------------------------------------*/

void    RTDF::normal_cache()
{
		df_cache_size = DF_CACHE_SIZE;
		cache_base = -1;        
}

/*------------------------------------------------------------------*/

RTDF::RTDF()
{
		file = NULL;
		data_cache_size = 512;
		data_cache = (char *)malloc(data_cache_size);
		data_cache_base = -1000;
		cache_on = 1;
		df_cache_size = DF_CACHE_SIZE;
		sem = create_sem(1, "df_sem");
		is_read_only = 0;
}

/*------------------------------------------------------------------*/

RTDF::~RTDF()
{
		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		if (file)
				close_df();
		release_sem(sem);
		delete_sem(sem);

		if (data_cache)
				free(data_cache);
}

/*------------------------------------------------------------------*/

long	RTDF::determine_file_specs()
{
#if __POWERPC__
	pef_cheader		cheader;
	pef_sheader		*sheaders;
	off_t			size;
	long			res;
	long			n;
	long			i;

	if (file->GetSize(&size))
		return -1;

	res = file->ReadAt(0, &cheader, sizeof(pef_cheader));
	if (res != sizeof(pef_cheader))
		return -1;

	if ((cheader.magic1 != PEF_MAGIC1) || (cheader.magic2 != PEF_MAGIC2))
		return -1;

	if (cheader.cont_id == RES_CONTID) {
		sec_offset = sizeof(pef_cheader);
		empty = false;
		return 0;
	}

	if (cheader.cont_id != PEF_CONTID)
		return -1;

	n = cheader.snum * sizeof(pef_sheader);
	sheaders = (pef_sheader *) malloc(n);
	res = file->ReadAt(sizeof(pef_cheader), sheaders, n);
	if (res != n) {
		free(sheaders);
		return -1;
	}

	sec_offset = 0;
	for(i=0; i<cheader.snum; i++)
		if (sheaders[i].offset + sheaders[i].raw_size > sec_offset)
			sec_offset = sheaders[i].offset + sheaders[i].raw_size;

	free(sheaders);

	empty = (sec_offset == size);

	return 0;

#elif __INTEL__

	int				err;
	int				offset;
	char			pe_buf[4];
	off_t			size;
	
	err = file->Read(pe_buf, 2);
	if (err != 2) {
		PRINT(("Not able to read executable\n"));
		return -1;
	}

	if ((pe_buf[0] == 'R') && (pe_buf[1] == 'S')) {
		sec_offset = 4;
		empty = false;
		return 0;
	}

	if ((pe_buf[0] != 'M') || (pe_buf[1] != 'Z')) {
		PRINT(("DOS header signature mismatch. read--%2x %2x\n", pe_buf[0], pe_buf[1]));
		return -1;
	}

	file->Seek(PE_SIGNATURE_OFFSET, 0);
	err = file->Read(&offset, 4);
	if (err != 4) {
		PRINT(("PE: couldn't read PE signature offset.\n"));
		return -1;
	}

	file->Seek(offset, 0);

	err = file->Read(pe_buf, SIZE_OF_PE_SIGNATURE);
	if (err != SIZE_OF_PE_SIGNATURE) {
		PRINT(("PE: couldn't read PE signature.\n"));
		return -1;
	}

	if ((pe_buf[0] != 'P') || (pe_buf[1] != 'E')) {
		PRINT(("PE: PE signature mismatch.\n"));
		return -1;
	}

	offset += SIZE_OF_PE_SIGNATURE + sizeof(coff_header) +
			sizeof(standard_flds) +	sizeof(nt_specific) +
			sizeof(data_directories) * RESRC_TAB;
	file->Seek(offset, 0);

	err = file->Read(&offset, 4);
	if (err != 4) {
		PRINT(("PE: couldn't read resource offset...\n"));
		return -1;
	}

	if (offset == 0) {
		if (file->GetSize(&size))
			return -1;
		sec_offset = size;
		empty = true;
		return 0;
	}
			
	empty = false;
	sec_offset = offset;
	return 0;

#endif /* __POWERPC__ */	
	
}

/*------------------------------------------------------------------*/

long	RTDF::create_non_empty_file()
{
#if __POWERPC__
		pef_cheader		cheader;
		long			res;

		cheader.magic1 = PEF_MAGIC1;
		cheader.magic2 = PEF_MAGIC2;
		cheader.cont_id = RES_CONTID;

		res = file->WriteAt(0, &cheader, sizeof(pef_cheader));
		if (res != sizeof(pef_cheader))
			return -1;

		sec_offset = sizeof(pef_cheader);

		return 0;
#elif __INTEL__


		char		buf[2];
		long		res;
	
		buf[0] = 'R';
		buf[1] = 'S';
		res = file->WriteAt(0, buf, 2);
		if (res != 2)
			return -1;
		empty = false;
		sec_offset = 4;
		return 0;

#endif /* __POWERPC__ */		
}

/*------------------------------------------------------------------*/

long	RTDF::create_header()
{
		long            i;
		df_header       header;
		df_entry        an_entry;

		cache_on = 0;

		header.offset_table_ptr = sizeof(df_header);
		header.hole_table_ptr = sizeof(df_header) + sizeof(df_entry) * INITIAL_DIR_SIZE;
		header.free_entry_hint =0;
		header.signature = (SIGNATURE << 16) | VERSION;
		header.unique = cget_unique();
		unique = header.unique;
		header.dirty_count = 1;

		for (i = 0; i < 10; i++)
				header.reserved[i] = 0;

		write_at(0L, &header, sizeof(header));

		offset_table_ptr = header.offset_table_ptr;
		hole_table_ptr = header.hole_table_ptr;
		free_entry_hint = header.free_entry_hint;
		biggest_hole = 0;
		record_count = 0;
		cache_base = -1;

		an_entry.offset = 0;
		an_entry.size = -1;
		an_entry.type   = SYSTEM_ENTRY;

		offset_table_size = INITIAL_DIR_SIZE;
		hole_table_size = INITIAL_HOLE_SIZE;

		for (i = 2; i < INITIAL_DIR_SIZE; i++) {
				write_offset(i, &an_entry);
		}

		for (i=0; i < INITIAL_HOLE_SIZE; i++) {
				write_hole(i, &an_entry);
		}

		an_entry.offset = header.offset_table_ptr;
		an_entry.size   = sizeof(df_entry) * INITIAL_DIR_SIZE;
		an_entry.type   = SYSTEM_ENTRY;
		write_offset(0L, &an_entry);

		an_entry.offset = header.hole_table_ptr;
		an_entry.size   = sizeof(df_entry) * INITIAL_HOLE_SIZE;
		an_entry.type   = SYSTEM_ENTRY;
		write_offset(1L, &an_entry);

		empty = false;
		was_empty = true;

		return 0;
}

/*------------------------------------------------------------------*/

long	RTDF::read_header()
{
		df_header       header;
		df_entry        an_entry;

		cache_on = 0;   
		
		read_at(0L, &header, sizeof(header));

		if ((header.signature >> 16) != SIGNATURE)
			return(-1);

		unique = header.unique;
		if (header.dirty_count)
			return(-1);

		offset_table_ptr = header.offset_table_ptr;
		hole_table_ptr = header.hole_table_ptr;
		free_entry_hint = header.free_entry_hint;
		record_count = header.record_count;
		cache_base = -1;
		read_at(
				offset_table_ptr,       
				&an_entry,
				sizeof(df_entry));

		offset_table_size = an_entry.size / sizeof(an_entry);
		read_at(
				offset_table_ptr + sizeof(an_entry),
				&an_entry,
				sizeof(df_entry));

		hole_table_size = an_entry.size / sizeof(an_entry);
		header_dirty = 0;
		return (0);
}

/*------------------------------------------------------------------*/

int     RTDF::open_df(BFile *a_file, bool read_only)
{
		file = a_file;

		is_read_only = read_only;
		file_dirty = false;
		was_empty = false;

		if (determine_file_specs()) {
			file = NULL;
			return (-1);
		}

		if (empty)
			return 0;

		if (read_header() < 0) {
			file = NULL;
			return (-1);
		}
		return 0;
}


/*------------------------------------------------------------------*/

int     RTDF::close_df()
{
		df_header       header;
		long            result;

		if (empty)
			return 0;

		cache_on = 0;   
		result = read_at(0, &header, sizeof(df_header));
		if (result < 0)
			return(-1);

		header.offset_table_ptr = offset_table_ptr;
		header.hole_table_ptr = hole_table_ptr;
		header.free_entry_hint = free_entry_hint;
		header.record_count = record_count;     
		header.dirty_count = 0;

		if (!is_read_only && file_dirty) {
#if __INTEL__
			if (was_empty) {
				int		offset;
				
				file->Seek(PE_SIGNATURE_OFFSET, 0);
				file->Read(&offset, 4);
				offset += SIZE_OF_PE_SIGNATURE + sizeof(coff_header) +
						sizeof(standard_flds) +	sizeof(nt_specific) +
						sizeof(data_directories) * RESRC_TAB;
				file->Seek(offset, 0);
				if (file->Write(&sec_offset, 4) != 4)
					return(-1);
			}
#endif
			result = write_at(0, &header, sizeof(df_header));
		}

		inval_cache(cache_base);
		file = NULL;

		return(result);
}

/*------------------------------------------------------------------*/

int		RTDF::rewrite_offsets()
{
		df_header       header;
		long			result;

		cache_on = 0;   
		result = read_at(0, &header, sizeof(df_header));

		header.offset_table_ptr = offset_table_ptr;
		header.hole_table_ptr = hole_table_ptr;
		header.free_entry_hint = free_entry_hint;
		header.record_count = record_count;     

		result = write_at(0, &header, sizeof(df_header));
		return(result);
}

/*------------------------------------------------------------------*/

int     RTDF::flush()
{
		df_header       header;
		long            result;

		if (empty)
				return(0);

		if (header_dirty == 0)
				return(0);

		while (acquire_sem(sem) == B_INTERRUPTED)
			;
		cache_on = 0;   
		result = read_at(0, &header, sizeof(df_header));
		if (result < 0) {
				printf("flush_df::this should never append\n");
				release_sem(sem);
				return(-1);
		}

		header.offset_table_ptr = offset_table_ptr;
		header.hole_table_ptr = hole_table_ptr;
		header.free_entry_hint = free_entry_hint;
		header.record_count = record_count;     

		result = write_at(0, &header, sizeof(df_header));

		header_dirty = 0;

		if (result > 0)
				result = 0;

		release_sem(sem);
		return(result);
}

/*------------------------------------------------------------------*/

int     RTDF::create_df(BFile *a_file)
{
		header_dirty = 1;
		file = a_file;
		is_read_only = false;
		file_dirty = true;

		if (create_non_empty_file())
			return (-1);

		if (create_header())
			return (-1);

		return(0);
}

/*----------------------------------------------------------------*/

long	RTDF::get_unique()
{
	return(unique);
}

/*----------------------------------------------------------------*/

long	RTDF::get_sem()
{
	return(sem);
}

/*----------------------------------------------------------------*/


int		RTDF::load_offset_table()
{
	long		count = offset_table_size;
	long		offset;	
	long		result;
	df_entry	*xtable;	
	long		i;
	
	xtable = (df_entry *)malloc(sizeof(df_entry) * count);
	table = (sort_entry *)malloc(sizeof(sort_entry) * count);

	offset = offset_table_ptr;

	result = read_at(
					offset,
					xtable,
					sizeof(df_entry) * count);
	
	for (i = 0; i < count; i++) {
		table[i].offset = xtable[i].offset;
		table[i].size = xtable[i].size;
		table[i].type = xtable[i].type;
		table[i].index = i;	
	}
	free((char *)xtable);

	return(result);
}

/*----------------------------------------------------------------*/

static int	f_df_cmp(const void *p1, const void *p2)
{
	sort_entry	*f1 = (sort_entry *)p1;
	sort_entry	*f2 = (sort_entry *)p2;

	if (f1->offset < f2->offset)
		return(-1);
	else
		return(1);
}

/*----------------------------------------------------------------*/

static int	f_df1_cmp(const void *p1, const void *p2)
{
	sort_entry	*f1 = (sort_entry *)p1;
	sort_entry	*f2 = (sort_entry *)p2;

	if (f1->index < f2->index)
		return(-1);
	else
		return(1);
}

/*----------------------------------------------------------------*/

void	RTDF::sort_offset_table()
{
	long	i;
	long	total_size;

	_hsort_(table, offset_table_size, sizeof(sort_entry), f_df_cmp);

	total_size = 0;
	for (i = 0; i < offset_table_size; i++) {
		if (table[i].size > 0)
			total_size += table[i].size;
	}
}

/*----------------------------------------------------------------*/

void	RTDF::sort_by_entry()
{
	_hsort_(table, offset_table_size, sizeof(sort_entry), f_df1_cmp);
	/*
	for (i = 0; i < offset_table_size; i++) {
		if (table[i].size > 0) {	
			SERIAL_PRINT(("entry %ld, %ld %ld %ld\n", table[i].index, table[i].offset, table[i].size, table[i].type));
		}
	}
	*/
}

/*----------------------------------------------------------------*/

long	RTDF::rewrite_hole_table()
{
	long		i;
	long		count = hole_table_size;
	df_entry	tmp;

	tmp.offset = 0;
	tmp.size = -1;
	tmp.type = 0;

	for (i = 0; i < hole_table_size; i++) {
		write_hole(i, &tmp);
	}
	return(0);
}

/*----------------------------------------------------------------*/

long	RTDF::adjust_end_of_file()
{
	return(0);
}

/*----------------------------------------------------------------*/

long	RTDF::start_of_data()
{
	return(sizeof(df_header));
}

/*----------------------------------------------------------------*/

long	RTDF::move_record(long i, long current_pos)
{
	if (table[i].size <= 0)
		return(current_pos);

	move_block(table[i].offset, current_pos, table[i].size);
	table[i].offset = current_pos;
	if (table[i].index == 0) {
		offset_table_ptr = current_pos;
	}
	if (table[i].index == 1) {
		hole_table_ptr = current_pos;
	}

	return(current_pos + table[i].size);
}	

/*----------------------------------------------------------------*/

long	RTDF::rewrite_offset_table()
{
	long		i;
	long		count = offset_table_size;
	df_entry	tmp;

	
	for (i = 0; i < count; i++) {
		tmp.offset = table[i].offset;
		tmp.size = table[i].size;
		tmp.type = table[i].type;
		write_offset(table[i].index, &tmp);
	}
	inval_cache(-1);
	flush();
	return(0);
}	

/*----------------------------------------------------------------*/

void	RTDF::do_clear_entry(long i)
{
	table[i].offset = 0;
	table[i].size = -1;
	table[i].type = 0;
}

/*----------------------------------------------------------------*/
