/*
  File:		fut_io.c		@(#)fut_io.h	13.15 04/07/95
  Author:	Kit Enscoe, George Pawle

  Internal interface file for io.c, iomf.c, and xdr.c.

  Defines the fut_io_t structure which encodes ramp tables
  and shared tables, forming the header for a file. Shared tables
  are only read/written once while ramp tables are re-computed as
  needed. This scheme allows us to minimize the amount of I/O time
  and disk space consumed by futs.
  
  the format of a binary fut file:
  	I/O header
  	idstr
  	shared/common input tables
  	channels, each consisting of
  		input tables
  		output table
  		grid table

  PROPRIETARY NOTICE: The  software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on their
  designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1989-1993 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code. All rights reserved.

*/

#ifndef FUT_IO_HEADER
#define FUT_IO_HEADER

/*
  I/O codes used in fut_hdr_t (below) for encoding tables.
  There are two fields in an io code, the "code" and the "data".
  The data field is used only for FUTIO_SHARED and FUTIO_RAMP in
  which case it gives the channel number of the table to be shared
  or the grid size for a ramp input table respectively.
 */
 
#define FUTIO_DATA		(0x0ffff)	/* mask for code data */
#define FUTIO_CODE		(0xf0000)	/* mask for code number */
#define FUTIO_NULL		(0x00000)	/* table is null */
#define FUTIO_SHARED	(0x10000)	/* table is shared (data=chan#) */
#define FUTIO_RAMP		(0x20000)	/* table is a ramp (data=size) */
#define FUTIO_UNIQUE	(0x30000)	/* table is unique */

/* Note: for future portable use of FUTIO_VERSION with multi-byte values, it
	would be much better to define integer constants instead of chars, since
	the file I/O will preserve the integer value (it's OK for a single char).
*/

#define FUTIO_VERSION	'0'

/*
 * Fixed header for fut files.	For backwards compatibility, this
 * structure should never increase in size.
 */
typedef struct { /* size = 500 */
	int32	magic;				/* = FUT_MAGIC for postive file id */
	int32	version;			/* allow for future modifications */
	int32	idstr_len;			/* strlen(fut->idstr)+1 or 0 */
	int32	order;				/* interpolation order */
	int32	icode[FUT_NCHAN];	/* codes for common input tables */
	struct chan_hdr_s {			/* codes for each channel: */
		int16	size[FUT_NCHAN];	/* grid table dimensions */
		int32	icode[FUT_NCHAN];	/* input table codes */
		int32	ocode;			/* output table code */
		int32	gcode;			/* grid table code */
	} chan[FUT_NCHAN];
	int32	more;				/* more data follows if TRUE */
} fut_hdr_t, fut_far* fut_hdr_ptr_t;

typedef struct chan_hdr_s chan_hdr_t, fut_far* chan_hdr_ptr_t;


#if defined(__cplusplus) || defined(SABR)
extern "C" {
#endif

#if !defined(KPNONANSIC)
int	fut_io_encode ARGS((fut_ptr_t, fut_hdr_ptr_t));
int	fut_io_decode ARGS((fut_ptr_t, fut_hdr_ptr_t));

fut_ptr_t	fut_read_fut	ARGS((KpFd_p, fut_hdr_ptr_t));

int			fut_write_hdr	ARGS((KpFd_p, fut_hdr_ptr_t));
int 		fut_read_hdr	ARGS((KpFd_p, fut_hdr_ptr_t));
int 		fut_read_futhdr ARGS((KpFd_p, fut_hdr_ptr_t));

int			fut_write_idstr ARGS((KpFd_p, char fut_far*, fut_hdr_ptr_t));
int 		fut_read_idstr	ARGS((KpFd_p, char fut_far* fut_far*, fut_hdr_ptr_t));

int			fut_write_tbls	ARGS((KpFd_p, fut_ptr_t, fut_hdr_ptr_t));
int			fut_read_tbls	ARGS((KpFd_p, fut_ptr_t, fut_hdr_ptr_t));

fut_chan_ptr_t	fut_read_chan	ARGS((KpFd_p, chan_hdr_ptr_t));
fut_itbl_ptr_t	fut_read_itbl	ARGS((KpFd_p));
fut_otbl_ptr_t	fut_read_otbl	ARGS((KpFd_p));
fut_gtbl_ptr_t	fut_read_gtbl	ARGS((KpFd_p, int32));
int				fut_write_chan	ARGS((KpFd_p, fut_chan_ptr_t, chan_hdr_ptr_t));
int				fut_write_itbl	ARGS((KpFd_p, fut_itbl_ptr_t));
int				fut_write_otbl	ARGS((KpFd_p, fut_otbl_ptr_t));
int				fut_write_gtbl	ARGS((KpFd_p, fut_gtbl_ptr_t));

fut_ptr_t	fut_read_mfut		ARGS((KpFd_p, fut_hdr_ptr_t, Fixed_p));
int32		fut_readMFutHdr		ARGS((KpFd_p, fut_hdr_ptr_t));
int32		fut_readMFutTbls	ARGS((KpFd_p, fut_ptr_t fut_far*, fut_hdr_ptr_t, Fixed_p));
int32		fut_readMFutMTbls	ARGS((KpFd_p, Fixed_p));
int32		fut_readMFutITbls	ARGS((KpFd_p, fut_ptr_t, int32, int32));
int32		fut_readMFutGTbls	ARGS((KpFd_p, fut_ptr_t, int32));
int32		fut_readMFutOTbls	ARGS((KpFd_p, fut_ptr_t, int32, int32));
int32		fut_writeMFutHdr	ARGS((KpFd_p, fut_ptr_t, int32));
int32		fut_writeMFutTbls	ARGS((KpFd_p, fut_ptr_t, Fixed_p, int32, int32));
int32		fut_writeMFutMTbls	ARGS((KpFd_p, Fixed_p));
int32		fut_writeMFutITbls	ARGS((KpFd_p, fut_ptr_t, int32));
int32		fut_writeMFutGTbls	ARGS((KpFd_p, fut_ptr_t, int32));
int32		fut_writeMFutOTbls	ARGS((KpFd_p, fut_ptr_t, int32, int32));

void		makeIdentityMatrix	ARGS((Fixed_p, int32));
int32		isIdentityMatrix	ARGS((Fixed_p, int32));

char_p		setDefaultFP (ioFileChar_p, char_p);

void 		fut_swab_hdr	ARGS((fut_hdr_ptr_t));
void 		fut_swab_itbl	ARGS((fut_itbl_ptr_t));
void 		fut_swab_otbl	ARGS((fut_otbl_ptr_t));
void 		fut_swab_gtbl	ARGS((fut_gtbl_ptr_t));

#endif	/* end of !defined(KPNONANSIC) */

#if defined(__cplusplus) || defined(SABR)
}
#endif
#endif /* FUT_IO_HEADER */

