The T2K documentation is in the file T2K.H
Start by reading "T2K.H" and then configure T2K through the file "CONFIG.H".

This is how you can get T2K going in 5 minutes.
(1) Put all T2K font engine .C and .H files into a directory. (37+4 files)
(2) Take the source from below in this message and create a file called MAIN.c and put into the same directory. You now how 35 + 1 files.
(3) Create a project with for instance Visual C/C++ and include all .c files.
(4) Also put a font file such as TIMES.TTF into the same directory.
(5) Compile and run!
(6) Done :-)


Here is the a simple MAIN.c code example:

/***** Begin MAIN.c *****/
/*
 * MAIN.c
 * Copyright (C) 1989-1998 all rights reserved by Type Solutions, Inc. Plaistow, NH, USA.
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Type Solutions, Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and ownership of the software or intellectual property
 * therewithin is hereby transferred.
 *
 * This information in this software is subject to change without notice
 *
 * This file is simple illustrativ example on how to use T2K.
 * In real life you also will have to pay attention to the error codes.
 *
 *  Questions and comments to:
 *       <mailto:sampo@typesolutions.com>
 *       <http://www.typesolutions.com/>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "T2K.H"


static void PrintChar( T2K *scaler )
{
	int y, x, k, w, h;
	
	w = scaler->width;
	assert( w <= scaler->rowBytes * 8 );
	h = scaler->height;
	
	/* printf("w = %d, h = %d\n", w, h ); */
	k = 0;
	for ( y = 0; y < h; y++ ) {
		for ( x = 0; x < w; x++ ) {
			char c = (char)((scaler->baseAddr[ k + (x>>3) ] & (0x80 >> (x&7)) ) ? '@' : '.');
			printf("%c", c );
		}
		printf("\n");
		k += scaler->rowBytes;
	}
}


int main(void)
{
	int errCode;
	FILE *fpID = NULL;
	unsigned long length, count;
	unsigned char *data;
	T2K_TRANS_MATRIX trans;
	unsigned short charCode;
	char *string = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;
	tsiMemObject	*mem 	= NULL;
	InputStream 	*in 	= NULL;
	sfntClass 		*font = NULL;
	T2K 			*scaler = NULL;


	printf("\n\n\n");
	printf("**********    ******     **       **\n");
	printf("    **      **     **    **     **\n");
	printf("    **              **   **   **\n");
	printf("    **              **   ** **\n");
	printf("    **             **    ****\n");
	printf("    **            **     ** **\n");
	printf("    **          **       **   **\n");
	printf("    **        **         **     **\n");
	printf("    **      **********   **       **\n\n\n");
	printf ("Hello World, this is a simple T2K Example,\n\n");
	printf ("with just printf statements for output,\n\n");
	printf ("from www.typesolutions.com !\n\n");

	/* Create the Memhandler object. */
	mem	= tsi_NewMemhandler( &errCode );
	assert( errCode == 0 );

	/* Open the font. */
	fpID	= fopen("TIMES.TTF", "rb"); assert( fpID != NULL );
	errCode	= fseek( fpID, 0L, SEEK_END ); assert( errCode == 0 );
	length	= (unsigned long)ftell( fpID ); assert( ferror(fpID) == 0 );
	errCode	= fseek( fpID, 0L, SEEK_SET ); assert( errCode == 0 ); /* rewind */
		
	/* Read the font into memory. */	
	data	= (unsigned char *)malloc( sizeof( char ) * length ); assert( data != NULL );
	count	= fread( data, sizeof( char ), length, fpID ); assert( ferror(fpID) == 0 && count == length );
	errCode	= fclose( fpID ); assert( errCode == 0 );
	/* in = New_NonRamInputStream( mem, fpID, ReadFileDataFunc, length, &errCode  ); */
	
	
		/* Create the InputStream object, with data already in memory */
		in 	= New_InputStream3( mem, data, length, &errCode ); /* */
		assert( errCode == 0 );

			/* Create an sfntClass object*/
			font = FF_New_sfntClass( mem, FONT_TYPE_TT_OR_T2K, 0, in, NULL, NULL, &errCode );
			assert( errCode == 0 );
			
				/* Create a T2K font scaler object.  */
				scaler = NewT2K( font->mem, font, &errCode );
				assert( errCode == 0 );
					/* 24 point */
					trans.t00 = ONE16Dot16 * 24;
					trans.t01 = 0;
					trans.t10 = 0;
					trans.t11 = ONE16Dot16 * 24;
					/* Set the transformation */
					T2K_NewTransformation( scaler, true, 72, 72, &trans, true, &errCode );
					assert( errCode == 0 );
					
					for ( i = 0; (charCode = string[i]) != 0; i++ ) {
						/* Create a character */
						printf("\n\n***Here comes the %c ****\n\n", (char)charCode);
						T2K_RenderGlyph( scaler, charCode, 0, 0, BLACK_AND_WHITE_BITMAP, T2K_RETURN_OUTLINES | T2K_NAT_GRID_FIT  | T2K_SCAN_CONVERT, &errCode );
						assert( errCode == 0 );
						/* Now draw the char */
						PrintChar( scaler );
						/* Free up memory */
						T2K_PurgeMemory( scaler, 1, &errCode );
						assert( errCode == 0 );
					}


				/* Destroy the T2K font scaler object. */
				DeleteT2K( scaler, &errCode );
				assert( errCode == 0 );
			
	
			/* Destroy the sfntClass object. */
			FF_Delete_sfntClass( font, &errCode );
			
		/* Destroy the InputStream object. */
		Delete_InputStream( in, &errCode  );
	
	free( data );
	/* Destroy the Memhandler object. */
	tsi_DeleteMemhandler( mem );
	
	return 0;
}

/*****  End  MAIN.c *****/

