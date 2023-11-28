/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: main.c
 *   project : MPEG Layer-3 Audio Encoder
 *   author  : Stefan Gewinner gew@iis.fhg.de
 *   date    : 1999-08-18
 *   contents/description: simple console front end application
 *                         demonstrating library usage
 *
 * $Header: /home/cvs/menc/ciEncoder/common/Attic/demo.c,v 1.1.2.6 1999/12/02 14:48:41 gew Exp $
 *
\***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "mp3encode.h"
#include "inputfile.h"

#define PCMBUFSIZE 1152
#define BITBUFSIZE 8192

int main (int argc, char *argv [])
{
  mp3EncHandle hEncoder ;

  unsigned int sr, chan ;

  short pcmbuf [PCMBUFSIZE] ;
  unsigned char *feedPtr ;

  unsigned char bitbuf [BITBUFSIZE] ;
  int bytesInput = 0 ;

  FILE *outfile ;

  if (argc < 3)
  {
    printf ("USAGE: %s infile outfile [bitrate]\n", argv [0]) ;
    return 1 ;
  }

  // open the audio input file

  if (!openInputFile (argv [1]))
  {
    printf ("couldn't open input file %s\n", argv [1]) ;
    return 1 ;
  }

  // open the mp3 output file

  outfile = fopen (argv [2], "wb") ;
  if (!outfile)
  {
    printf ("couldn't create output file %s\n", argv [2]) ;
    return 1 ;
  }

  // determine input file parameters

  sr = getSampleRateFromInputFile () ;
  chan = getChannelCountFromInputFile () ;

  // open the encoder library

  hEncoder = mp3EncOpen (sr, chan, mp3EncQualityFast) ;

  // set the target bitrate if given

  if (argc > 3)
  {
    unsigned int bitrate = atol (argv [3]) ;
    if (bitrate)
    {
      mp3EncOutputFormat myFormat ;

      myFormat.bitRate = bitrate ;
      myFormat.sampleRate = 0 ; // default
      myFormat.numChannels = 0 ; // default

      if (!mp3EncSetOutputFormat (hEncoder, &myFormat))
        fprintf (stderr, "unsupported output format!\n") ;
    }
  }

  if (outfile)
  {
    // encoding loop

    for ( ;; )
    {
      unsigned int bytesConsumed ;
      int bytesWritten ;

      if (bytesInput == 0)
      {
        // feed an arbitrary amount of data

        bytesInput = readFromInputFile (pcmbuf, PCMBUFSIZE * sizeof (short)) ;
        feedPtr = (unsigned char *) pcmbuf ;
      }

      // call the actual encoding routine

      bytesWritten = mp3EncEncode (hEncoder,
                                   feedPtr,
                                   bytesInput,
                                   &bytesConsumed,
                                   bitbuf,
                                   BITBUFSIZE) ;

      // all done, bail out

      if (!bytesInput && !bytesWritten)
        break ;

      if (bytesWritten < 0)
      {
        fprintf (stderr, "mp3EncEncode() failed\n") ;
        break ;
      }

      // advance input pointer and state

      feedPtr += bytesConsumed ;
      bytesInput -= bytesConsumed ;

      // write bitstream to mp3 file

      fwrite (bitbuf, 1, bytesWritten, outfile) ;
    }

    // clean up

    fclose (outfile) ;
  }

  mp3EncClose (hEncoder) ;

  closeInputFile () ;

  return 0 ;
}
