/*
  File:		io.c		@(#)io.c	13.22 09/25/97 
  Author:	Kit Enscoe, George Pawle

  This file contains functions to read and write binary fut files.

  All opens, closes, reads and writes are performed with the functions
  Kp_open, Kp_close, Kp_read, and Kp_write, respectively
  to provide an "operating system independent" i/o interface.  These
  functions are implemented differently for each operating system, and are
  defined in the library kcms_sys.

  To handle architecture dependent byte ordering, byte swapping is performed
  as neccessary when reading a file, by checking the byte ordering of the
  "magic" numbers.  The "standard" byte ordering is Most Significant Byte First
  (e.g. Sun, Macintosh) but this default can be overridden (see below).

  BEWARE of asynchronous usage of a fut while it is being written out
  since its tables may be temporarily byte reversed.

  PROPRIETARY NOTICE: The  software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on their
  designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1989-1995 Eastman Kodak Company.
  As  an  unpublished  work pursuant to Title 17 of the United
  States Code.  All rights reserved.

 */


#include "string.h" 
#include "fut.h"
#include "fut_util.h"			/* internal interface file */
#include "fut_io.h" 

/*
 * The following constant, variable, and two functions work together
 * to allow binary fut files to be written out in one of two different
 * byte orderings.  By default, files are written with Most Significant
 * Byte First.  A call to fut_set_local_msbf() prior to writing a fut
 * sets the mode to write in the ordering of the local machine.  This
 * results in the fastest reading of fut files for the local machine since
 * byte swapping is not necessary.  Calling fut_set_univ_msbf() prior to
 * fut writing results in "standard" byte ordering (the default).
 */
#define FUT_MSBF_UNIV	0xf	/* universal byte ordering (never change this!) */


/* fut_load loads a fut from the file named "filename", performing the
 * open and close automatically.  It returns a pointer to the loaded fut
 * or NULL on error.
 */
fut_ptr_t
	fut_load (char_p filename)
{
fut_ptr_t  fut;
ioFileChar	fileProps;
char_p realFileName;

	realFileName = setDefaultFP (&fileProps, filename);

	fut = fut_load_fp (realFileName, fileProps);

	return (fut);
}

/* fut_load_fp is like fut_load but
 * includes file properties in a separate function argument.
 */
fut_ptr_t
	fut_load_fp (	char_p		filename,
					ioFileChar	fileProps)
{
fut_ptr_t  fut;
KpFd_t		fd;

	if ( ! Kp_open (filename, "r", &fd, &fileProps) ) {
		return (FUT_NULL);
	}

	fut = fut_read_Kp (&fd);

	(void) Kp_close (&fd);

	return (fut);
}


/* fut_cal_crc
 *
 *	An signed 32-bit fut cyclical redundancy check (CRC)
 */
int
	fut_cal_crc (fut_ptr_t fut, int32_p crc)
{
KpFd_t  fd;
int		ret = 1;
fut_hdr_t	futio;

	if ( ! IS_FUT(fut) ) {	/* check for valid fut */
		return (0);
	}

	if ( ! Kp_open (NULL, "c", &fd, NULL) ) {
		return (-1);
	}

	if ( ! fut_io_encode (fut, &futio) ) {	/* encode futio hdr */
		return (0);
	}

	ret = fut_write_tbls (&fd, fut, &futio);	/* write out the tables */

	Kp_get_crc (&fd, (KpCrc32_t FAR *)crc);

	(void) Kp_close (&fd);

	return (ret);
}


/* fut_store stores fut to the file named "filename", performing the
 * open and close automatically.  Returns 1 on success, 0 or negative
 * on error.
 */
int
	fut_store (	fut_ptr_t	fut,
				char_p		filename)
{
int		ret;
ioFileChar	fileProps;
char_p	realFileName;

	realFileName = setDefaultFP (&fileProps, filename);

	ret = (int)fut_store_fp (fut, realFileName, fileProps);

	return (ret);
}


/* fut_store_fp is like fut_store but
 * includes file properties in a separate function argument.
 */
int32
	fut_store_fp (	fut_ptr_t	fut,
					char_p		filename,
					ioFileChar	fileProps)
{
KpFd_t	fd;
int32	ret;

	/* Open with the new e mode for exclusive.  The file
	   must be closed, or at least unlocked when done */
	if ( ! Kp_open (filename, "e", &fd, &fileProps) ) {
		return (-1);
	}

	ret = fut_write_Kp (&fd, fut);

	(void) Kp_close (&fd);

	return (ret);
}

/*
 * fut_write_Kp writes a fut to an open file descriptor.
 * Returns: 
 * 1 on success
 * 0 on invalid fut error
 * -1 on header, id string, or Kp_write error
 *  -2 to -5 on a table specific error
 */
int
	fut_write_Kp (KpFd_p fd, fut_ptr_t fut)
{
int			ret = 1;
fut_hdr_t	futio;

						/* check for valid fut */
	if ( ! IS_FUT(fut) ) {
		return (0);
	}

						/* encode futio hdr */
	if ( ! fut_io_encode (fut, &futio) ) {
		return (0);
	}

						/* write out the header */
	if ( ! fut_write_hdr (fd, &futio) ) {
		return (-1);
	}

						/* write out the id string */
	if ( ! fut_write_idstr (fd, fut->idstr, &futio) ) {
		return (-1);
	}

	ret = fut_write_tbls (fd, fut, &futio);	/* write out the tables */

	return ( ret );

} /* fut_write_Kp */

/*
 * fut_read_Kp reads a fut from an open file descriptor, returning a pointer
 * to a newly allocated fut or NULL on error.
 */
fut_ptr_t
	fut_read_Kp (KpFd_p fd)
{
fut_ptr_t	fut;
fut_hdr_t	futio;
KpF15d16_t	matrix[MF_MATRIX_DIM][MF_MATRIX_DIM];
Fixed_p		matrixP = &matrix[0][0];

	if ( ! Kp_read (fd, (fut_generic_ptr_t)&futio.magic, sizeof(int32))) {
		return (FUT_NULL);
	}

	if ((futio.magic == FUT_CIGAM) || (futio.magic == FUT_MAGIC) ) {
		fut = fut_read_fut (fd, &futio);                                        /* read a fut */
	}
	else {
#if (FUT_MSBF == 0)
		Kp_swab32 ((fut_generic_ptr_t)&futio.magic, 1);
#endif

		switch (futio.magic) {
		case MF1_TBL_ID:        /* 8 bit matrix fut */
		case MF2_TBL_ID:        /* 16 bit matrix fut */
			fut = fut_read_mfut (fd, &futio, matrixP);
			break;

		default:
			return (NULL);          /* unknown type */
		}
	}

	return (fut);
}	/* fut_read_Kp */


/*
 * fut_read_fut reads a fut from an open file descriptor, returning a pointer
 * to a newly allocated fut or NULL on error.
 */
fut_ptr_t
	fut_read_fut (KpFd_p fd, fut_hdr_ptr_t futioP)
{
fut_ptr_t  fut;

	fut = fut_alloc_fut ();				/* allocate a new fut structure */
	if ( fut == FUT_NULL ) {
		return (FUT_NULL);
	}

	if ( ! fut_read_futhdr (fd, futioP) ||	/* read in the encoded header, */
		! fut_read_idstr (fd, (char fut_far* fut_far*)&fut->idstr, futioP) || /* the id string */
		! fut_read_tbls (fd, fut, futioP) ||		/* the tables, and */
		! fut_io_decode (fut, futioP) ) {		/* decode file header */
		fut = fut_free (fut);			/* if error, free fut */
	}

	return (fut);

} /* fut_read */

/*
 * fut_write_hdr writes the header information to an open binary fut file
 * from a futio header structure.  This structure was initialized by a call to
 * fut_io_encode and contains information describing shared and identity (ramp)
 * tables and also indicates the content of the remaining part of the file.
 *
 * It returns 1 (TRUE) on success and 0 (FALSE) if a write error occured.
 */
int
	fut_write_hdr (KpFd_p fd, fut_hdr_ptr_t hdr)
{
int			i, ret;
chan_hdr_ptr_t chan;

						/* swap bytes if necessary */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_hdr (hdr);
#endif /* FUT_MSBF */

	
						/* write each element, quitting
							if write error occurs */
	ret = Kp_write (fd, (fut_generic_ptr_t)&hdr->magic,
				sizeof(int32) )				&&
		Kp_write (fd, (fut_generic_ptr_t)&hdr->version,
				sizeof(int32) )				&&
		Kp_write (fd, (fut_generic_ptr_t)&hdr->idstr_len,
				sizeof(int32) )				&&
		Kp_write (fd, (fut_generic_ptr_t)&hdr->order,
				sizeof(int32) )				&&
		Kp_write (fd, (fut_generic_ptr_t) hdr->icode,
				sizeof(int32)*FUT_NCHAN);

	for (i = 0, chan = hdr->chan; (ret > 0) && (i < FUT_NCHAN); ++i, chan++) {

		ret = Kp_write (fd, (fut_generic_ptr_t) chan->size,
					sizeof(int16)*FUT_NCHAN)		&&
			Kp_write (fd, (fut_generic_ptr_t) chan->icode,
					sizeof(int32)*FUT_NCHAN)		&&
			Kp_write (fd, (fut_generic_ptr_t)&chan->ocode,
					sizeof(int32))				&&
			Kp_write (fd, (fut_generic_ptr_t)&chan->gcode,
					sizeof(int32));
	}

	ret = (ret > 0) && Kp_write (fd, (fut_generic_ptr_t)&hdr->more,
						sizeof(int32));

	/* always swap bytes back again */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_hdr (hdr);
#endif /* FUT_MSBF */

	return  (ret);

} /* fut_write_hdr */

/*
 * fut_read_hdr reads the header information from an open binary fut file into
 * futio header structure.  This information describes shared and identity (ramp)
 * tables and indicates the content of the remaining part of the file.
 *
 * It returns 1 (TRUE) on success and 0 (FALSE)  if a read error occured.
 */
int
	fut_read_hdr (KpFd_p fd, fut_hdr_ptr_t hdr)
{
int			ret;

	if ( ! Kp_read (fd, (fut_generic_ptr_t)&hdr->magic,
				sizeof(int32)) ) {
		return  0;
	}

	ret = fut_read_futhdr (fd, hdr);
	return (ret);
}


/*
 * fut_read_futhdr reads everything after the magic number, which is
 * needed for use with alternate transform types.
 *
 * It returns 1 (TRUE) on success and 0 (FALSE)  if a read error occured.
 */
int
	fut_read_futhdr (KpFd_p fd, fut_hdr_ptr_t hdr)
{
int			i;
chan_hdr_ptr_t chan;

	if ( ! Kp_read (fd, (fut_generic_ptr_t)&hdr->version,
				sizeof(int32))				||
		! Kp_read (fd, (fut_generic_ptr_t)&hdr->idstr_len,
				sizeof(int32))				||
		! Kp_read (fd, (fut_generic_ptr_t)&hdr->order,
				sizeof(int32))				||
		! Kp_read (fd, (fut_generic_ptr_t) hdr->icode,
				sizeof(int32)*FUT_NCHAN) )	{
		return  0;
	}

	for (i=0, chan=hdr->chan; i<FUT_NCHAN; ++i, chan++) {
		if ( ! Kp_read (fd, (fut_generic_ptr_t) chan->size,
					sizeof(int16)*FUT_NCHAN)		||
			! Kp_read (fd, (fut_generic_ptr_t) chan->icode,
					sizeof(int32)*FUT_NCHAN)		||
			! Kp_read (fd, (fut_generic_ptr_t)&chan->ocode,
					sizeof(int32))				||
			! Kp_read (fd, (fut_generic_ptr_t)&chan->gcode,
					sizeof(int32)) )				{
			return  0;
		}
	}

	if ( ! Kp_read (fd, (fut_generic_ptr_t)&hdr->more,
					sizeof(int32)) )
		return 0;

	switch (hdr->magic) {
		case FUT_MAGIC: /* bytes in correct order, nothing to do */
			break;

		case FUT_CIGAM: /* bytes are reversed, swap them now! */
			fut_swab_hdr (hdr);
			break;

		default:				/* garbage, return error */
			return 0;
	}

	return	1;

} /* fut_read_hdr */

/*
 * fut_write_chan writes the tables (input, output, and grid) for an output
 * channel to an open file descriptor.  The chan_hdr_t from the fut_hdr_t
 * structure previously obtained from fut_io_encode is needed to specify which
 * tables are to be written out (shared table and ramp tables are not).
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -5 on invalid channel error
 *  -2 to -4 on a table specific error
 */
int
  fut_write_chan (KpFd_p fd, fut_chan_ptr_t chan, chan_hdr_ptr_t chanio)
{
int	i, ret = 1;

	if ( ! IS_CHAN (chan) )
		return (-5);

						/* write out the input tables */
	for ( i = 0; (i < FUT_NICHAN) && (ret > 0); i++ ) {
		if ( chanio->icode[i] == FUTIO_UNIQUE )
			ret = fut_write_itbl (fd, chan->itbl[i]);
	}

						/* write out the output table */
	if ( ret > 0 ) {
		if ( chanio->ocode == FUTIO_UNIQUE )
			ret = fut_write_otbl (fd, chan->otbl);
	}

						/* write out the grid tables */
	if ( ret > 0 ) {
		if ( chanio->gcode == FUTIO_UNIQUE )
			ret = fut_write_gtbl (fd, chan->gtbl);
	}

	return (ret);

} /* fut_write_chan */

/*
 * fut_read_chan reads the tables (input, output, and grid) for an output
 * channel from an open file descriptor.  The chan_hdr_t from the fut_hdr_t
 * structure previously read in is needed to determine which tables exist
 * for the channel and need to be read in.
 *
 * It returns a newly allocated fut_chan_t structure on success and NULL
 * on failure.
 */
fut_chan_ptr_t 
	fut_read_chan (KpFd_p fd, chan_hdr_ptr_t chanio)
{
int  			i, ret = 1;
fut_chan_ptr_t	chan;
int32			tbl_size = sizeof(fut_gtbldat_t);

					/* allocate a new chan structure */
	chan = fut_alloc_chan();
	if ( chan == FUT_NULL_CHAN ) {
		return (FUT_NULL_CHAN);
	}

						/* read in the input tables */
	for ( i = 0; (i < FUT_NICHAN) && ret; i++ ) {
		if ( chanio->icode[i] == FUTIO_UNIQUE ) {
			chan->itbl[i] = fut_read_itbl (fd);
			if (chan->itbl[i] != FUT_NULL_ITBL){
				chan->itblHandle[i] = chan->itbl[i]->handle;
			} else {
				ret = False;
			}
		}
	}

						/* read in the output table */
	if ( ret ) {
		if ( chanio->ocode == FUTIO_UNIQUE ) {
			chan->otbl = fut_read_otbl (fd);
			if ( chan->otbl != FUT_NULL_OTBL ) {
				chan->otblHandle = chan->otbl->handle;
			} else {
				ret = False;
			}
		}
	}
						/* read in the grid table */
	if ( ret ) {
		if ( chanio->gcode == FUTIO_UNIQUE ) {
			for (i = 0; i < FUT_NICHAN; i++) {
				if (chanio->size[i] != 0) {
					tbl_size *= chanio->size[i];
				}
			}
			chan->gtbl = fut_read_gtbl (fd, tbl_size);
			if (chan->gtbl != FUT_NULL_GTBL) {
				chan->gtblHandle = chan->gtbl->handle;
			} else {
				ret = False;
			}
		} else {
			ret = False;
		}
	}

	if ( ! ret ) {
		fut_free_chan (chan);
		chan = FUT_NULL_CHAN;
	}

	return (chan);

} /* fut_read_chan */

/*
 * fut_write_itbl writes an input table to an open binary fut file.
 * Individual members are written one by one to reduce machine architecture
 * dependency, while arrays of ints are written in a single write call.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 on invalid input table error
 *
 * If necessary, we swap bytes before writing out the table and swap them
 * back again afterwards.  The machine reading back the table can always
 * determine the byte ordering by examining the magic numbers.
 */
int
	fut_write_itbl (KpFd_p fd, fut_itbl_ptr_t itbl)
{
int32  ignored = 0;
int	ret;

	if ( ! IS_ITBL(itbl) )
		return (-2);

	/* swap bytes if necessary */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_itbl (itbl);
#endif /* FUT_MSBF */

	/* write out the itbl structure */
	ret = Kp_write (fd, (fut_generic_ptr_t)&itbl->magic,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&itbl->size,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t) itbl->tbl,
				sizeof(fut_itbldat_t)*(FUT_INPTBL_ENT+1));

	/* always swap bytes back again */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_itbl (itbl);
#endif /* FUT_MSBF */

	return (ret ? 1 : -1);

} /* fut_write_itbl */

/*
 * fut_read_itbl reads in an input table from an open binary fut file.
 * Individual members are read in one by one to reduce machine architecture
 * dependency, while arrays of ints are read in a single read call.  If
 * the magic number read in is byte reversed, then we swap all the bytes
 * in the structure.
 *
 * A pointer to a newly allocated table is returned, or NULL on error.
 */
fut_itbl_ptr_t
	fut_read_itbl (KpFd_p fd)
{
fut_itbl_ptr_t itbl;
int32		ignored = 0;

					/* create an itbl to read into */
	itbl = fut_new_itbl (2, FUT_NULL_IFUN);
	if ( itbl == FUT_NULL_ITBL ) {
		DIAG("fut_read_itbl: can't create new input table.\n", 0);
		return (FUT_NULL_ITBL);
	}
					/* read in itbl structure.  If read error
						then free structure and return error */
	if ( ! Kp_read (fd, (fut_generic_ptr_t)&itbl->magic,
				sizeof(int32))						||
		! (itbl->magic == FUT_IMAGIC || itbl->magic == FUT_CIGAMI) ||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t)&itbl->size,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t) itbl->tbl,
				sizeof(fut_itbldat_t)*(FUT_INPTBL_ENT+1)) )	{

		DIAG("fut_read_itbl: error reading input table.\n", 0);

		/* Note: set magic number for fut_free_itbl in case garbled */
		itbl->magic = FUT_IMAGIC;
		fut_free_itbl (itbl);
		return (FUT_NULL_ITBL);
	}

	/*
	* See if bytes need swapping.
	*/
	if ( itbl->magic == FUT_CIGAMI ) /* bytes are reversed, swap them now! */
		fut_swab_itbl (itbl);
  /*
	* Clip the input table entries to avoid memory violation errors during
	* interpolation (see note in fut_calc_itbl()).  This is only necessary
	* for older fut files created before the fix to fut_calc_itbl.  Perhaps
	* we should use a different file version number to avoid doing this
	* all the time.  On the other hand, this is only performed once on
	* input for 257 elements so why bother.
	*/
	{
		int					i;
		fut_itbldat_ptr_t	idat;
		fut_itbldat_t		imax;

				/* maximum itbl entry value */
		imax = (itbl->size - 1) << FUT_INP_DECIMAL_PT;

		for ( i=FUT_INPTBL_ENT+1, idat=itbl->tbl; --i >= 0; idat++ ) {
			/*
			* may as well check for *any* illegal values while
			* we're at it, to prevent any "mysterious" memory
			* violations due to garbled file data.  Note that
			* the unsigned compare catches illegal values both
			* negative and too large in one test.
			*/
			if ( ((unsigned long)(*idat)) >= ((unsigned long)imax)) {
				if ( *idat == imax ) {
					/* next lower value to avoid boundary */
					*idat = imax-1;
				} else {
					DIAG("fut_read_itbl: illegal value.\n", 0);
					fut_free_itbl (itbl);
					return (FUT_NULL_ITBL);
				}
			}
		}
	}

	return (itbl);

} /* fut_read_itbl */

/*
 * fut_write_otbl writes an output table to an open binary fut file.
 * Individual members are written one by one to reduce machine architecture
 * dependency, while arrays of ints are written in a single write call.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -3 on invalid output table error
 *
 * If necessary, we swap bytes before writing out the table and swap them
 * back again afterwards.  The machine reading back the table can always
 * determine the byte ordering by examining the magic numbers.
 */
int
	fut_write_otbl (KpFd_p fd, fut_otbl_ptr_t otbl)
{
int32  ignored = 0;
int	ret;

	if ( ! IS_OTBL(otbl) )
		return (-3);

	/* swap bytes if necessary */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_otbl (otbl);
#endif /* FUT_MSBF */

						/* write out the otbl structure */
	ret = Kp_write (fd, (fut_generic_ptr_t)&otbl->magic,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t) otbl->tbl,
				sizeof(fut_otbldat_t)*FUT_OUTTBL_ENT);

	/* always swap bytes back again */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_otbl (otbl);
#endif /* FUT_MSBF */

	return (ret ? 1 : -1);

} /* fut_write_otbl */


/*
 * fut_read_otbl reads in an output table from an open binary fut file.
 * Individual members are read in one by one to reduce machine architecture
 * dependency, while arrays of ints are read in a single read call.  If
 * the magic number read in is byte reversed, then we swap all the bytes
 * in the structure.
 *
 * A pointer to a newly allocated table is returned, or NULL on error.
 */
fut_otbl_ptr_t 
	fut_read_otbl (KpFd_p fd)
{
fut_otbl_ptr_t otbl;
int32		ignored = 0;

					/* create an otbl to read into */
	otbl = fut_new_otbl (FUT_NULL_OFUN);
	if ( otbl == FUT_NULL_OTBL ) {
		DIAG("fut_read_otbl: can't create new output table.\n", 0);
		return (FUT_NULL_OTBL);
	}
					/* read in otbl structure.  If read error
						then free otbl and return error */
	if ( ! Kp_read (fd, (fut_generic_ptr_t)&otbl->magic,
				sizeof(int32))						||
		! (otbl->magic == FUT_OMAGIC || otbl->magic == FUT_CIGAMO) ||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t) otbl->tbl,
				sizeof(fut_otbldat_t)*FUT_OUTTBL_ENT) )		{

		DIAG("fut_read_otbl: error reading output table.\n", 0);

		/* Note: set magic number for fut_free_otbl in case garbled */
		otbl->magic = FUT_OMAGIC;
		fut_free_otbl (otbl);
		return (FUT_NULL_OTBL);
	}

	/*
	* See if bytes need swapping.
	*/
	if ( otbl->magic == FUT_CIGAMO ) /* bytes are reversed, swap them now! */
		fut_swab_otbl (otbl);

	return (otbl);

} /* fut_read_otbl */


/*
 * fut_write_gtbl writes a grid table to an open binary fut file.
 * Individual members are written one by one to reduce machine architecture
 * dependency, while arrays of ints are written in a single write call.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -4 on invalid channel error
 *
 * If necessary, we swap bytes before writing out the table and swap them
 * back again afterwards.  The machine reading back the table can always
 * determine the byte ordering by examining the magic numbers.
 */
int
	fut_write_gtbl (KpFd_p fd, fut_gtbl_ptr_t gtbl)
{
int	ret;
int32  ignored = 0;
int32  tbl_size;

						/* make sure gtbl has grid table
							array allocated */
	if ( ! IS_GTBL(gtbl) || gtbl->tbl == (fut_gtbldat_ptr_t)0 )
		return (-4);

						/* swap bytes if necessary */
	tbl_size = gtbl->tbl_size;			/* but save tbl_size first */

#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_gtbl (gtbl);
#endif /* FUT_MSBF */

						/* write out the gtbl structure */
	ret = Kp_write (fd, (fut_generic_ptr_t)&gtbl->magic,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t)&gtbl->tbl_size,
				sizeof(int32))				&&
		Kp_write (fd, (fut_generic_ptr_t) gtbl->size,
				sizeof(int16)*FUT_NCHAN)		&&
		Kp_write (fd, (fut_generic_ptr_t) gtbl->tbl,
				tbl_size);

	/* always swap bytes back again */
#if ( FUT_MSBF_UNIV != FUT_MSBF )
	fut_swab_gtbl (gtbl);
#endif /* FUT_MSBF */

	return (ret ? 1 : -1);

} /* fut_write_gtbl */


/*
 * fut_read_gtbl reads in an grid table from an open binary fut file.
 * Individual members are read in one by one to reduce machine architecture
 * dependency, while arrays of ints are read in a single read call.  If
 * the magic number read in is byte reversed, then we swap all the bytes
 * in the structure.
 *
 * A pointer to a newly allocated table is returned, or NULL on error.
 */
fut_gtbl_ptr_t 
	fut_read_gtbl (KpFd_p fd, int32 gtbl_size)
{
fut_gtbl_ptr_t gtbl;
int32		tbl_size;
int32		ignored = 0;

					/* allocate a gtbl structure */
	gtbl = fut_alloc_gtbl ();
	if ( gtbl == FUT_NULL_GTBL ) {
		DIAG("fut_read_gtbl: can't alloc grid table struct.\n", 0);
		return (FUT_NULL_GTBL);
	}
					/* read in gtbl structure.  If read error,
						or is garbage, or no table data, then
						free gtbl structure and return error */
	if ( ! Kp_read (fd, (fut_generic_ptr_t)&gtbl->magic,
				sizeof(int32))						||
		! (gtbl->magic == FUT_GMAGIC || gtbl->magic == FUT_CIGAMG) ||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t)&ignored,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t)&gtbl->tbl_size,
				sizeof(int32))						||
		! Kp_read (fd, (fut_generic_ptr_t) gtbl->size,
				sizeof(int16)*FUT_NCHAN) )			{

		DIAG("fut_read_gtbl: error reading grid table struct.\n", 0);

		/* Note: set magic number for fut_free_gtbl in case garbled */
		gtbl->magic = FUT_GMAGIC;
		fut_free_gtbl (gtbl);
		return (FUT_NULL_GTBL);
	}
					/* Its kinda kludgy, but we must first
						get tbl_size in proper byte ordering
						before we can allocate and read in
						the grid table */
	tbl_size = gtbl->tbl_size;
	if ( gtbl->magic == FUT_CIGAMG )
		Kp_swab32 ((fut_generic_ptr_t)&tbl_size, 1);

	if ( tbl_size <= 0 || tbl_size != gtbl_size) {		/* verify tbl_size */
		DIAG("fut_read_gtbl: invalid table size.\n", 0);
		gtbl->magic = FUT_GMAGIC;		/* (in case reversed) */
		fut_free_gtbl (gtbl);
		return (FUT_NULL_GTBL);
	}

					/* allocate memory for the table */
	gtbl->tbl = fut_alloc_gtbldat (tbl_size/(int32)sizeof(fut_gtbldat_t));
	if ( gtbl->tbl == NULL ) {
		DIAG("fut_read_gtbl: can't allocate grid table.\n", 0);
		gtbl->magic = FUT_GMAGIC;		/* (in case reversed) */
		fut_free_gtbl (gtbl);
		return (FUT_NULL_GTBL);
	}
	gtbl->tblHandle = getHandleFromPtr ((fut_generic_ptr_t)gtbl->tbl);

					/* read in the table data */
	if ( ! Kp_read (fd, (fut_generic_ptr_t)gtbl->tbl, tbl_size) ) {
		DIAG("fut_read_gtbl: error reading grid table.\n", 0);
		gtbl->magic = FUT_GMAGIC;		/* (in case reversed) */
		fut_free_gtbl (gtbl);
		return (FUT_NULL_GTBL);
	}
	/*
	* See if bytes need swapping.
	*/
	if ( gtbl->magic == FUT_CIGAMG ) /* bytes are reversed, swap them now! */
		fut_swab_gtbl (gtbl);

	return (gtbl);

} /* fut_read_gtbl */


/*
 * fut_read_idstr and fut_write_idstr read and write the idstring (if
 * it exists) from and to an open file descriptor.
 * Returns: 
 * 1 on success
 * 0 on id string or Kp_write error
 */
int
	fut_write_idstr (KpFd_p fd, char fut_far * idstr, fut_hdr_ptr_t hdr)
{
int32	nbytes;

					/* Get length of idstring.  This
						includes the null terminator */
	nbytes = hdr->idstr_len;

	if ( nbytes == 0 ) {
		return (1);			/* nothing to write */
	} else if ( idstr == (char fut_far*)NULL ) {
		return (0);			/* error */
	}

	return (Kp_write (fd, (fut_generic_ptr_t)idstr, nbytes));

} /* fut_write_idstr */


int
	fut_read_idstr (KpFd_p fd, char fut_far* fut_far*  idstrp, fut_hdr_ptr_t hdr)
{
	int32	nbytes;
	char	buf[256];
					/* Get length of idstring.  This
						includes the null terminator */
	nbytes = hdr->idstr_len;

					/* If idstrp is NULL, just skip past
						the idstr by reading into a scratch
						buffer and throwing away the data */
	if ( idstrp == NULL ) {
		while (nbytes > 0) {
			if ( ! Kp_read (fd, (fut_generic_ptr_t)&buf[0],
						sizeof(buf)) ) {
				return (0);
			}
			nbytes -= sizeof(buf);
		}

	} else if ( nbytes == 0 ) {

		*idstrp = NULL;		/* If no idstring, just set to NULL */

	} else {
					/* Otherwise, idstring is present and
						caller wants it.  Allocate memory
						and read it in. */

		*idstrp = fut_alloc_idstr (nbytes);
		if ( *idstrp == NULL  )
			return (0);

		nbytes--;		/* don't read in the final null terminator */

		if ( ! Kp_read (fd, (fut_generic_ptr_t) *idstrp, nbytes) ||
			! Kp_read (fd, (fut_generic_ptr_t) &buf[0], 1) ) {

			fut_free_idstr (*idstrp);
			*idstrp = NULL;
			return (0);
		}
		/*
		* for compatibility with older futs, change multiple '\0's
		* to '\n's.  These may arise from double even padding.
		*/
		while ( --nbytes >= 0 ) {
			if ( (*idstrp)[nbytes] == '\0' ) {
				(*idstrp)[nbytes] = KP_NEWLINE;
			} else {
				break;
			}
		}
	}

	return (1);

} /* fut_read_idstr */


/*
 * fut_write_tbls writes the tables of a fut to an open file descriptor.
 * The header and idstring should have already been written to the file.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 *  -2 to -4 on a table specific error
 */
int
	fut_write_tbls (KpFd_p fd, fut_ptr_t fut, fut_hdr_ptr_t hdr)
{
int	i, ret = 1;

						/* write out the input tables */
	for ( i = 0; (i < FUT_NICHAN) && (ret > 0); i++ ) {
		if ( hdr->icode[i] == FUTIO_UNIQUE ) {
			ret = fut_write_itbl (fd, fut->itbl[i]);
		}
	}

						/* write out the output channels */
	for ( i = 0; (i < FUT_NOCHAN) && (ret > 0); i++ ) {
		if ( fut->chan[i] != 0 ) {
			ret = fut_write_chan (fd, fut->chan[i], & hdr->chan[i]);
		}
	}

	return ( (ret > 0) ? 1 : ret );

} /* fut_write_tbls */


/* fut_read_tbls reads the tables from an open file descriptor and assigns
 * them to the already allocated 'fut'.  The header and idstring must have
 * been previously read from the file.  Upon a succesful return,
 * fut_io_decode must be called to generate linear tables and share the
 * shared ones.
 *
 * Returns 1 (TRUE) on success, 0 (FALSE) on failure.
 *
 * Note: on failure, some of the tables and channels amy be partially
 * read in and assigned to the fut.  It is a good idea to immediately
 * free the fut in this case.
 */
int
	fut_read_tbls (KpFd_p fd, fut_ptr_t fut, fut_hdr_ptr_t hdr)
{
int	i;

				/* make sure fut has been allocated and
					contains the magic number. */
	if ( ! IS_FUT(fut) )
		return (0);

						/* read in the input tables */
	for ( i=0; i<FUT_NICHAN; i++ ) {
		if ( hdr->icode[i] == FUTIO_UNIQUE ) {
			fut->itbl[i] = fut_read_itbl (fd);
			if ( fut->itbl[i] == FUT_NULL_ITBL) {
				return (0);
			}
			fut->itblHandle[i] = fut->itbl[i]->handle;
		}
	}
						/* read in the output channels */
	for ( i=0; i<FUT_NOCHAN; i++ ) {
		if ( hdr->chan[i].gcode != 0 ) {
			fut->chan[i] = fut_read_chan (fd, &hdr->chan[i]);
			if (fut->chan[i] == FUT_NULL_CHAN ) {
				return (0);
			}
			fut->chanHandle[i] = fut->chan[i]->handle;
		}
	}

	return (1);

} /* fut_read_tbls */


char_p
	setDefaultFP (ioFileChar_p fileProps, char_p filename)
{
char_p realFileName;

#if defined (KPMAC)
	strcpy (fileProps->fileType, "PT  ");
	strcpy (fileProps->creatorType, "KEPS");
	if (filename == NULL) {
		fileProps->vRefNum = 0;	/* PMSP */
		realFileName = NULL;
	}
	else {
		fileProps->vRefNum = (unsigned char)filename[0];
		fileProps->vRefNum = fileProps->vRefNum << 8;
		fileProps->vRefNum += (unsigned char)filename[1];
	
		realFileName = &filename[2];
	}
#else
	if (fileProps) {}
	realFileName = filename;
#endif

	return (realFileName);
}

