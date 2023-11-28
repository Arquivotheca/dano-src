//
//	WORDconsts.h
//
#ifndef __WORDCONSTS_H__
#define __WORDCONSTS_H__

#include <StorageKit.h>
#include <InterfaceKit.h>
#include <stdio.h>
#include <string.h>
#include <UTF8.h>

#include "WORDstructs.h"

#define INIT_BUFFER_SIZE	1024

#define POINTS_PER_LINE		12
#define TWIPS_PER_POINT		20
#define TWIPS_PER_LINE		(POINTS_PER_LINE * TWIPS_PER_POINT)

#define MAX_FIELD_DATA_SIZE	256

// My Piece Table - used for in memory version
typedef struct piecetbl
{
	long	cpMax;		// char position where this text piece stops
	long	numChars;	// num of chars in this piece
	bool	isUNICODE;	// pcd.fc will have been adjusted for this
	long	fcStart;	// FC of beginning of piece
	ushort	prm;		// the Word PRM (sprms)
} PieceTbl;


// My Bin Table - used for in memory version
typedef struct bintbl
{
	long	fcMax;		// FC that is the upper limit for this pn
	long	pn;			// page number
} BinTbl;


// My File Shape Table - used for in memory version
typedef struct fspatbl
{
	long	cp;			// CP for the special char 0x08 that marks the shape in the text stream
	FSPA	fspa;		// file shape address
} FspaTbl;

// used to simplify use of para and char styles
enum {S_PARA = 0, S_CHAR, S_TYPE_COUNT};

// footnote, endnote, and annotation text
enum {N_FOOTNOTE = 0, N_ANNOTATION, N_ENDNOTE, N_TYPE_COUNT};
 
#endif // __WORDCONSTS_H__

