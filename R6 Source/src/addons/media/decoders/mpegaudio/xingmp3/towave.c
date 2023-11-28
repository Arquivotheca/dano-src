/*---- towave.c --------------------------------------------
  32 bit version only

decode mpeg Layer I/II/III file using portable ANSI C decoder,
output to pcm wave file.

mod 8/19/98 decode 22 sf bands

mod 5/14/98  allow mpeg25 (dec8 not supported for mpeg25 samp rate)

mod 3/4/98 bs_trigger  bs_bufbytes  made signed, unsigned may 
            not terminate properly.  Also extra test in bs_fill.

mod 8/6/96 add 8 bit output to standard decoder

ver 1.4 mods 7/18/96 32 bit and add asm option

mods 6/29/95  allow MS wave file for u-law.  bugfix u-law table dec8.c

mods 2/95 add sample rate reduction, freq_limit and conversions.
          add _decode8 for 8Ks output, 16bit 8bit, u-law output.
          add additional control parameters to init.
          add _info function

mod 5/12/95 add quick window cwinq.c

mod 5/19/95 change from stream io to handle io

mod 11/16/95 add Layer I

mod 1/5/95   integer overflow mod iup.c

ver 1.3
mod 2/5/96   portability mods
             drop Tom and Gloria pcm file types

ver 2.0
mod 1/7/97   Layer 3 (float mpeg-1 only)
    2/6/97   Layer 3 MPEG-2

ver 3.01     Layer III bugfix crc problem 8/18/97
ver 3.02     Layer III fix wannabe.mp3 problem 10/9/97
ver 3.03     allow mpeg 2.5  5/14/98
  
Decoder functions for _decode8 are defined in dec8.c.  Useage
is same as regular decoder.

Towave illustrates use of decoder.  Towave converts
mpeg audio files to 16 bit (short) pcm.  Default pcm file
format is wave. Other formats can be accommodated by
adding alternative write_pcm_header and write_pcm_tailer
functions.  The functions kbhit and getch used in towave.c
may not port to other systems.

The decoder handles all mpeg1 and mpeg2 Layer I/II  bitstreams.  

For compatability with the asm decoder and future C versions,
source code users are discouraged from making modifications
to the decoder proper.  MS Windows applications can use wrapper
functions in a separate module if decoder functions need to
be exported.

NOTE additional control parameters.

mod 8/6/96 standard decoder adds 8 bit output

decode8 (8Ks output) convert_code:
   convert_code = 4*bit_code + chan_code
       bit_code:   1 = 16 bit linear pcm
                   2 =  8 bit (unsigned) linear pcm
                   3 = u-law (8 bits unsigned)
       chan_code:  0 = convert two chan to mono
                   1 = convert two chan to mono
                   2 = convert two chan to left chan
                   3 = convert two chan to right chan

decode (standard decoder) convert_code:
             0 = two chan output
             1 = convert two chan to mono
             2 = convert two chan to left chan
             3 = convert two chan to right chan
     or with 8 = 8 bit output 
          (other bits ignored)

decode (standard decoder) reduction_code:
             0 = full sample rate output
             1 = half rate
             2 = quarter rate

-----------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#endif
#include <fcntl.h>		/* file open flags */
#include <errno.h>
#include <sys/types.h>		/* someone wants for port */
#include <sys/stat.h>		/* forward slash for portability */
#include "mhead.h"		/* mpeg header structure, decode protos */
#include "port.h"



static char default_file[] = "TEST.MP3";
static char default_outfile[] = "TEST.WAV";



/*********  bitstream buffer */
#define BS_BUFBYTES 60000U
static unsigned char *bs_buffer = NULL;
static unsigned char *bs_bufptr;
static int bs_trigger;
static int bs_bufbytes;
static int handle = -1;

/********************** */

/********  pcm buffer ********/
#define PCM_BUFBYTES  60000U
static unsigned char *pcm_buffer;
static unsigned int pcm_bufbytes;
static unsigned int pcm_trigger = (PCM_BUFBYTES - 2500 * sizeof(short));
static int handout = -1;

/****************************/
static int bs_fill();


static int out_help();
static int out_mpeg_info(MPEG_HEAD * h, int bitrate_arg);
int ff_decode(char *filename,
	      char *fileout,
	      int reduction_code,
	      int convert_code,
	      int decode8_flag,
	      int freq_limit,
	      int integer_decode);
int cvt_to_wave_test();
int write_pcm_header_wave(int handout,
			  int samprate, int channels, int bits, int type);
int write_pcm_tailer_wave(int handout, unsigned int pcm_bytes);


main(int argc, char *argv[])
{
   int i, k;
   char *filename;
   char *fileout;
   int convert_code;
   int reduction_code;
   int freq_limit;
   int integer_decode;
   int decode8_flag;		/* decode to 8KHz */


   /****** process command line args */
   filename = default_file;
   fileout = default_outfile;
   convert_code = 0;
   reduction_code = 0;
   freq_limit = 24000;
   integer_decode = 0;
   decode8_flag = 0;		/* decode to 8KHz */
   for (k = 0, i = 1; i < argc; i++)
   {
      if (argv[i][0] != '-')
      {
	 if (k == 0)
	    filename = argv[i];
	 if (k == 1)
	    fileout = argv[i];
	 k++;
	 continue;
      }
      switch (argv[i][1])
      {
	 case 'h':
	 case 'H':
	    out_help();
	    return 0;
	 case 'c':
	 case 'C':
	    convert_code = atoi(argv[i] + 2);
	    break;
	 case 'r':
	 case 'R':
	    reduction_code = atoi(argv[i] + 2);
	    break;
	 case 'f':
	 case 'F':
	    freq_limit = atoi(argv[i] + 2);
	    break;
	 case 'i':
	 case 'I':
	    integer_decode = 1;
	    break;
	 case 'd':
	 case 'D':
	    if (atoi(argv[i] + 2) == 8)
	       decode8_flag = 1;
	    break;
      }
   }


   ff_decode(filename, fileout,
	     reduction_code, convert_code, decode8_flag,
	     freq_limit, integer_decode);


   printf("\n");
   return 0;
}

static int out_help()
{

   printf("\n"
	  "\n -D8 option decode8 (8Ks output) convert_code:"
	  "\n    convert_code = 4*bit_code + chan_code"
	  "\n        bit_code:   1 = 16 bit linear pcm"
	  "\n                    2 =  8 bit (unsigned) linear pcm"
	  "\n                    3 = u-law (8 bits unsigned)"
	  "\n        chan_code:  0 = convert two chan to mono"
	  "\n                    1 = convert two chan to mono"
	  "\n                    2 = convert two chan to left chan"
	  "\n                    3 = convert two chan to right chan"
	  "\n decode (standard decoder) convert_code:"
	  "\n              0 = two chan output"
	  "\n              1 = convert two chan to mono"
	  "\n              2 = convert two chan to left chan"
	  "\n              3 = convert two chan to right chan"
	  "\n              (higher bits ignored)"
	  "\n decode (standard decoder) reduction_code:"
	  "\n              0 = full sample rate output"
	  "\n              1 = half rate"
	  "\n              2 = quarter rate"
	  "\n -I  option selects integer decoder"
      );

   return 1;
}

int ff_decode(char *filename, char *fileout,
	      int reduction_code, int convert_code, int decode8_flag,
	      int freq_limit, int integer)
{
   int framebytes;
   int u;
   MPEG_HEAD head;
   unsigned int nwrite;
   IN_OUT x;
   int in_bytes, out_bytes;
   DEC_INFO decinfo;
   int bitrate;

   typedef struct
   {
      int (*decode_init) (MPEG_HEAD * h, int framebytes_arg,
			  int reduction_code, int transform_code,
			  int convert_code, int freq_limit);
      void (*decode_info) (DEC_INFO * info);
        IN_OUT(*decode) (unsigned char *bs, short *pcm);
   }
   AUDIO;
   static AUDIO audio;
   static AUDIO audio_table[2][2] =
   {
      {
	 {audio_decode_init, audio_decode_info, audio_decode},
	 {audio_decode8_init, audio_decode8_info, audio_decode8},
      },
      {
	 {i_audio_decode_init, i_audio_decode_info, i_audio_decode},
	 {audio_decode8_init, audio_decode8_info, audio_decode8},
      }
   };



   printf("\nMPEG input file: %s", filename);
   printf("\n    output file: %s", fileout);

   /*-----select decoder --------------*/
   if (decode8_flag && (convert_code >= 4))
      audio = audio_table[integer & 1][1];
   else
      audio = audio_table[integer & 1][0];



   in_bytes = out_bytes = 0;

   handout = -1;
   pcm_buffer = NULL;
   pcm_bufbytes = 0;

   bs_buffer = NULL;
   handle = -1;
   bs_bufbytes = 0;
   bs_bufptr = bs_buffer;
   bs_trigger = 2500;
   /*--- test for valid cvt_to_wave compile ---*/
   if (cvt_to_wave_test() != 0)
   {
      printf("\n INVALID CVT_TO_WAVE COMPILE");
      goto abort;
   }

   /*------ open mpeg file --------*/
   handle = open(filename, O_RDONLY | O_BINARY);
   if (handle < 0)
   {
      printf("\n CANNOT_OPEN_INPUT_FILE\n");
      goto abort;
   }

   /*--- allocate bs buffer ----*/
   bs_buffer = malloc(BS_BUFBYTES);
   if (bs_buffer == NULL)
   {
      printf("\n CANNOT_ALLOCATE_BUFFER\n");
      goto abort;
   }

   /*--- fill bs buffer ----*/
   if (!bs_fill())
      goto abort;

   /*---- parse mpeg header  -------*/
   framebytes = head_info2(bs_buffer, bs_bufbytes, &head, &bitrate);
   if (framebytes == 0)
   {
      printf("\n BAD OR UNSUPPORTED MPEG FILE\n");
      goto abort;
   }

   /*--- display mpeg info --*/
   out_mpeg_info(&head, bitrate);


   /*---- create pcm file ------*/
   handout = open(fileout, O_RDWR | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
   if (handout < 0)
   {
       printf("\n CANNOT CREATE OUTPUT FILE\n");
       goto abort;
   }

   /*---- allocate pcm buffer --------*/
   pcm_buffer = malloc(PCM_BUFBYTES);
   if (pcm_buffer == NULL)
   {
      printf("\n CANNOT ALLOCATE PCM BUFFER\n");
      goto abort;
   }

   /*---- init decoder -------*/
   if (!audio.decode_init(&head, framebytes,
			  reduction_code, 0, convert_code, freq_limit))
   {
      printf("\n DECODER INIT FAIL \n");
      goto abort;
   }

   /*---- get info -------*/
   audio.decode_info(&decinfo);

   /*---- info display -------*/
   printf("\n output samprate = %6ld", decinfo.samprate);
   printf("\n output channels = %6d", decinfo.channels);
   printf("\n output bits     = %6d", decinfo.bits);
   printf("\n output type     = %6d", decinfo.type);

   /*---- write pcm header -------*/
   if (!write_pcm_header_wave(handout,
	    decinfo.samprate, decinfo.channels, decinfo.bits, decinfo.type))
   {
      printf("\n FILE WRITE ERROR\n");
      goto abort;
   }

   /*--- init wave converter ---*/
   cvt_to_wave_init(decinfo.bits);

   printf("\n");

   /*----- DECODE -----*/
   for (u = 0;;)
   {
      if (!bs_fill())
	 break;
      if (bs_bufbytes < framebytes)
	 break;			/* end of file */

      x = audio.decode(bs_bufptr, (short *) (pcm_buffer + pcm_bufbytes));

      if (x.in_bytes <= 0)
      {
	 printf("\n BAD SYNC IN MPEG FILE\n");
	 break;
      }
printf("encode += %d output += %d\n", x.in_bytes, x.out_bytes); 
      bs_bufptr += x.in_bytes;
      bs_bufbytes -= x.in_bytes;
      pcm_bufbytes += x.out_bytes;
      u++;
      if (pcm_bufbytes > pcm_trigger)
      {
	 pcm_bufbytes = cvt_to_wave(pcm_buffer, pcm_bufbytes);
	 nwrite = write(handout, pcm_buffer, pcm_bufbytes);
	 if (nwrite != pcm_bufbytes)
	 {
	    printf("\n FILE WRITE ERROR\n");
	    break;
	 }
	 out_bytes += pcm_bufbytes;
	 pcm_bufbytes = 0;
      }
      in_bytes += x.in_bytes;
      if ((u & 63) == 1)
	 printf("\r frames %6ld   bytes in %6ld    bytes out %6ld",
		u, in_bytes, out_bytes);
   }


   if (pcm_bufbytes > 0)
   {
      pcm_bufbytes = cvt_to_wave(pcm_buffer, pcm_bufbytes);
      nwrite = write(handout, pcm_buffer, pcm_bufbytes);
      if (nwrite != pcm_bufbytes)
      {
	 printf("\n FILE WRITE ERROR\n");
      }
      out_bytes += pcm_bufbytes;
      pcm_bufbytes = 0;
   }
   printf("\n frames %6ld   bytes in %6ld    bytes out %6ld",
	  u, in_bytes, out_bytes);

   /*---- write pcm tailer -------*/
   write_pcm_tailer_wave(handout, out_bytes);


   printf("\n    output file: %s", fileout);

 abort:
   close(handle);
   close(handout);
   free(bs_buffer);
   free(pcm_buffer);
   return 1;
}
/*-------------------------------------------------------------*/
static int bs_fill()
{
   unsigned int nread;

   if (bs_bufbytes < 0)
      bs_bufbytes = 0;		// signed var could be negative

   if (bs_bufbytes < bs_trigger)
   {
      memmove(bs_buffer, bs_bufptr, bs_bufbytes);
      nread = read(handle, bs_buffer + bs_bufbytes, BS_BUFBYTES - bs_bufbytes);
      if ((nread + 1) == 0)
      {
	 /*-- test for -1 = error --*/
	 bs_trigger = 0;
	 printf("\n FILE_READ_ERROR\n");
	 return 0;
      }
      bs_bufbytes += nread;
      bs_bufptr = bs_buffer;
   }

   return 1;
}

static int out_mpeg_info(MPEG_HEAD * h, int bitrate_arg)	/* info only */
{
   int bitrate;
   int samprate;
   static char *Layer_msg[] =
   {"INVALID", "III", "II", "I"};
   static char *mode_msg[] =
   {"STEREO", "JOINT", "DUAL", "MONO"};
   static int sr_table[8] =
   {22050L, 24000L, 16000L, 1L,
    44100L, 48000L, 32000L, 1L};

   bitrate = bitrate_arg / 1000;
   printf("\n Layer %s ", Layer_msg[h->option]);

   printf("  %s ", mode_msg[h->mode]);
   samprate = sr_table[4 * h->id + h->sr_index];
   if ((h->sync & 1) == 0)
      samprate = samprate / 2;	// mpeg25

   printf(" %d ", samprate);
   printf("  %dKb ", bitrate);
   if ((h->mode == 1) && (h->option != 1))
      printf("  %d stereo bands ", 4 + 4 * h->mode_ext);
   if (h->prot == 0)
      printf(" (CRC)");

   return 0;
}
