/* ++++++++++
	ofs_map.h
	Copyright (C) 1997 Be Inc.  All Rights Reserved.

	Structures that define the first block and the partition
	table of a Be OFS file system.
 ++++++++++ */

#ifndef _OFS_MAP_H
#define _OFS_MAP_H

#define CURRENTVERSION			0x00030000L

typedef struct	Track0
	{
	long	VersionNumber;			/*version number of the fs	*/
	long	FormatDate; 			/*When format took place	*/
	long	FirstBitMapSector;		/*For allocation		*/
	long	BitMapSize;  			/*# of sectors for the bitmap 	*/
	long	FirstDirSector;			/*Start of directory		*/
	long	TotalSector;			/*Number of logical sector	*/
	long	BytePerSector;			/*only working with 512 now	*/
	long	DirBlockHint;			/*First block to look in	*/			
	long	FreeSectorHint;			/*First free sector to look	*/
	long	SectorUsed;				/*# of sectors busy on dsk	*/
	char	Removable;				/*non-zero if removebale media  */
	char	Fillerc[3];				/*filler, for alignment         */
	char	VolName[32];			/*Volume name                   */
	long	clean_shutdown;			/*non-zero if unmnt was clean   */
	long	root_ref;
	long	root_modbits;
	long	Filler[107];			/*to get 512 bytes		*/
	} Track0;

#endif /* _OFS_MAP_H */
