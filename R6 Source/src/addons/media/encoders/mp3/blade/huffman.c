/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression, and might
		contain smaller or larger sections that are directly taken
		from ISO's reference code.

		All changes to the ISO reference code herein are either
		copyrighted by Tord Jansson (tord.jansson@swipnet.se)
		or sublicensed to Tord Jansson by a third party.

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

#include <stdlib.h>
#include <stdio.h>
#include "system.h"
#include "common.h"
#include "huffman.h"

HUFFBITS dmask = 1 << (sizeof(HUFFBITS)*8-1);
unsigned int hs = sizeof(HUFFBITS)*8;

struct huffcodetab ht[HTN];     /* array of all huffcodtable headers    */
                                /* 0..31 Huffman code table 0..31       */
                                /* 32,33 count1-tables                  */

/* read the huffman encode table */
int read_huffcodetab( void )
{

        int             index = 0;

  char line[100],command[40],huffdata[40];
  unsigned int t,i,j,k,nn,x,y,n=0;
  unsigned int xl, yl, len;
  HUFFBITS h;
  int   hsize;

  hsize = sizeof(HUFFBITS)*8;
  do
        {
    strcpy( line, aHuffcode[index++] );
/*      fgets(line,99,fi); */
  } while ((line[0] == '#') || (line[0] < ' ') );

  do
        {
    while ((line[0]=='#') || (line[0] < ' '))
                {
            strcpy( line, aHuffcode[index++] );
/*      fgets(line,99,fi); */
    }

    sscanf(line,"%s %s %u %u %u",command,ht[n].tablename,
                                 &xl,&yl,&ht[n].linbits);
    if (strcmp(command,".end")==0)
      return n;
    else if (strcmp(command,".table")!=0) {
      fprintf(stderr,"huffman table %u data corrupted\n",n);
      return -1;
    }
    ht[n].linmax = (1<<ht[n].linbits)-1;

    sscanf(ht[n].tablename,"%u",&nn);
    if (nn != n) {
      fprintf(stderr,"wrong table number %u\n",n);
      return(-2);
    }

    ht[n].xlen = xl;
    ht[n].ylen = yl;

    do {

            strcpy( line, aHuffcode[index++] );
/*      fgets(line,99,fi); */
    } while ((line[0] == '#') || (line[0] < ' '));

    sscanf(line,"%s %u",command,&t);
    if (strcmp(command,".reference")==0) {
      ht[n].ref   = t;

      ht[n].table = ht[t].table;
      ht[n].hlen  = ht[t].hlen;
      if ( (xl != ht[t].xlen) ||
           (yl != ht[t].ylen)  ) {
        fprintf(stderr,"wrong table %u reference\n",n);
        return (-3);
      };
      do {
            strcpy( line, aHuffcode[index++] );
/*                      fgets(line,99,fi); */
      } while ((line[0] == '#') || (line[0] < ' ') );
    }
    else {
        ht[n].ref  = -1;
#ifdef NO_ZERO_CALLOC
      ht[n].table=(HUFFBITS *) calloc((xl*yl)+1,sizeof(HUFFBITS));
#else
      ht[n].table=(HUFFBITS *) calloc(xl*yl,sizeof(HUFFBITS));
#endif
      if (ht[n].table == NULL) {
         fprintf(stderr,"unsufficient heap error\n");
         return (-4);
      }
#ifdef NO_ZERO_CALLOC
      ht[n].hlen=(unsigned char *) calloc((xl*yl)+1,sizeof(unsigned char));
#else
      ht[n].hlen=(unsigned char *) calloc(xl*yl,sizeof(unsigned char));
#endif
      if (ht[n].hlen == NULL) {
         fprintf(stderr,"unsufficient heap error\n");
         return (-4);

      }
      for (i=0; i<xl; i++) {
        for (j=0;j<yl; j++) {
          if (xl>1)
            sscanf(line,"%u %u %u %s",&x, &y, &len,huffdata);
          else
            sscanf(line,"%u %u %s",&x,&len,huffdata);
          h=0;k=0;
          while (huffdata[k]) {
            h <<= 1;
            if (huffdata[k] == '1')
              h++;
            else if (huffdata[k] != '0'){
              fprintf(stderr,"huffman-table %u bit error\n",n);
              return (-5);
            };
            k++;
          };
          if (k != len) {
           fprintf(stderr,
              "warning: wrong codelen in table %u, pos [%2u][%2u]\n",
               n,i,j);
          };
          ht[n].table[i*xl+j] = h;
          ht[n].hlen[i*xl+j] = (unsigned char) len;
          do {
            strcpy( line, aHuffcode[index++] );
/*                                      fgets(line,99,fi); */
          } while ((line[0] == '#') || (line[0] < ' '));
        }
      }
    }
    n++;
  } while (1);
}



