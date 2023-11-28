/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression.

		This file doesn't contain any of the ISO reference code and
		is copyright Tord Jansson (tord.jansson@swipnet.se).

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

#include		<stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <time.h>
#include    "system.h"
#include		"codec.h"
#include    "samplein.h"
#include		"bladesys.h"

#define MAX_NAMELEN 256

#ifdef OS2
	#define INCL_DOSFILEMGR
	#define INCL_DOSERRORS
	#define INCL_DOSPROCESS
	#include <os2.h>
	#include "os2key.h"
#endif

extern	char *mystrupr(char * strng);


/*____ Structure Definitions __________________________________________________*/

typedef struct  JobDef  Job;

struct JobDef
{
        CodecInitIn		sCodec;
        SplIn         sInput;
        Job         * psNext;
        int           fDeleteSource;
        char          outputFilename[MAX_NAMELEN];
        char          sourceFilename[MAX_NAMELEN];
};


/*____ Function Prototypes ____________________________________________________*/

int     printUsage( void );
int			validateJobs( Job * psJob );
int			readGlobalSwitches( int argc, char * argv[] );
int			readLocalSwitches( int argc, char * argv[], Job * psJob );
int			addCommandlineJob( int argc, char * argv[] );
int     addFileToBatch( char * pFilename, char * pOutputFilename );
int     clearJobQueue( void );
int     removeJobQueueEntry( Job * psJob );

int     readL3EncCommandline( int argc, char * argv[] );

void		updateProgressIndicator( time_t startBatch, double batchLen, double batchDone,
                                 time_t startFile, unsigned int fileLen, unsigned int fileDone );
void    quit( int returnValue );




/*____ Static Data ____________________________________________________________*/



/* Parameters set through the commandline. */

int				wantedBitrate = -1;             /* -1 = Unspecified. */
int				wantedCRC = FALSE;
int				wantedPrivate = FALSE;
int				wantedCopyright = FALSE;
int				wantedOriginal = TRUE;
int				wantedChannelSwap = FALSE;
SampleType	wantedInputType = STEREO;

int     wantedDeleteSource = FALSE;

#ifdef	WAIT_KEY
	int     wantedQuit = FALSE;
#else
	int			wantedQuit = TRUE;
#endif
int			wantedQuiet = FALSE;
int			wantedSTDOUT = FALSE;
int			fPreparedSTDIN = FALSE;

char		prioString[256];
char	* pPrioString = NULL;

Job		* psJobQueue = NULL;

char		outputDir[MAX_NAMELEN];

FILE	* textout;

/*____ main() _________________________________________________________________*/

int main( int argc, char* argv[] )
{
	int           samplesPerFrame;
	int           nSamples;

  short         readBuffer[2304];
  int           x;

  char          mystring[2] = { 13, 0 };
  char          input;
  Job         * psTemp;

  time_t        startTimeBatch, startTimeFile, currTime;
  double        batchSamplesTotal = 0.0, batchSamplesRead = 0.0;

  CodecInitOut *pCodecInfo;
  char        * pBuffer;
  uint          encodedChunkSize;
  FILE        * fp;

	int						cmdOfs;
	char					temp[256];
#ifdef	WILDCARDS
	void				* pWildcardLink;
#endif

	/* First things first... */

 	textout = stdout;	
	outputDir[0] = 0;

#ifdef	WILDCARDS
	pWildcardLink = expandWildcards( &argc, &argv );
#endif

	cmdOfs = readGlobalSwitches( argc-1, argv+1 ) +1;

	/* Check for STDOUT */

	for( x = 1 ; x < argc ; x++ )
	{
		strcpy( temp, argv[x] );
		mystrupr( temp );
		if( strcmp( temp, "STDOUT" ) == 0 )
		{
			prepStdout();
			textout = stderr;
			break;
		}
	}

  /* Print Text */

	if( !wantedQuiet )
	{
		fprintf( textout, "\n" );
		fprintf( textout, "BladeEnc 0.82    (c) Tord Jansson       Homepage: http://www.bladeenc.cjb.net\n" );
		fprintf( textout, "===============================================================================\n" );
		fprintf( textout, "BladeEnc is free software, distributed under the Lesser General Public License.\n" );
		fprintf( textout, "See the file COPYING, BladeEnc's homepage or www.fsf.org for more details.\n" );
    fprintf( textout, "\n" );
	}

	/* Initialise batch */

	while( cmdOfs < argc )
	{
		x = addCommandlineJob( argc - cmdOfs, &argv[cmdOfs] );
		if( x == 0 )
		{
#ifdef	WILDCARDS	
			freeExpandWildcardMem( pWildcardLink );
#endif
			quit( -1 );
		}
		cmdOfs += x;
	}

#ifdef	WILDCARDS
	freeExpandWildcardMem( pWildcardLink );
#endif
	

	/* Validate job settings */

	x = validateJobs( psJobQueue );
  if( x == FALSE )
    quit( -2 );


  /* Set priority */

	if( setPriority( pPrioString ) == FALSE )
	{
		fprintf( textout, "Error: '%s' is not a valid priority setting!\n", pPrioString );
		quit(	-1 );
	};

	/* Procedure if no files found */

  if( psJobQueue == NULL )
  {
    printUsage();                                                           /* No files on the commandline */
    quit( -1 );
  }


  /* Print files to encode */

  for( x = 0, psTemp = psJobQueue ; psTemp != NULL ; x++, psTemp = psTemp->psNext );
	if( !wantedQuiet )
	  fprintf( textout, "Files to encode: %d\n\n", x );


  /* Encode */

  startTimeBatch = time( NULL );

  for( psTemp = psJobQueue ; psTemp != NULL ; batchSamplesTotal += psTemp->sInput.length, psTemp = psTemp->psNext );


  while( psJobQueue != NULL )
  {
    /* Print information */

		if( !wantedQuiet )
		{
			fprintf( textout, "Encoding:  %s\n", psJobQueue->sourceFilename );
			fprintf( textout, "Input:     %.1f kHz, %d bit, ", psJobQueue->sInput.freq/1000.f, psJobQueue->sInput.bits );
			if( psJobQueue->sInput.fReadStereo == TRUE )
				fprintf( textout, "stereo.\n" );
			else
				fprintf( textout, "mono.\n" );
			fprintf( textout, "Output:    %d kBit, ", psJobQueue->sCodec.bitrate );
			if( psJobQueue->sCodec.mode == 0 )
				fprintf( textout, "stereo.\n\n" );
			else
				fprintf( textout, "mono.\n\n" );
		}

    /* Init a new job */

    startTimeFile = time( NULL );
    pCodecInfo = codecInit( &psJobQueue->sCodec );
    samplesPerFrame = pCodecInfo->nSamples;
    pBuffer = (char *) malloc( pCodecInfo->bufferSize );
		if( strcmp( psJobQueue->outputFilename, "STDOUT" ) == 0 )
			fp = stdout;
		else
		{
			fp = fopen( psJobQueue->outputFilename, "wb" );
			if( fp == NULL )
			{
		/*  codecExit(); */
				closeInput( &psJobQueue->sInput );
				fprintf( textout, "ERROR: Couldn't create '%s'!\n", psJobQueue->outputFilename );
				quit( -1 );
			}
		}

    /* Encoding loop */

    while ( (nSamples = readSamples( &psJobQueue->sInput, samplesPerFrame, readBuffer)) > 0 )
    {
      encodedChunkSize = codecEncodeChunk( nSamples, readBuffer, pBuffer );
      if( fwrite( pBuffer, 1, encodedChunkSize, fp ) != encodedChunkSize )
      {
        fprintf( textout, "ERROR: Couldn't write '%s'! Disc probably full.\n", psJobQueue->outputFilename );
        quit( -1 );
      }

      batchSamplesRead += nSamples;

			if( !wantedQuiet )
				updateProgressIndicator( startTimeBatch, batchSamplesTotal, batchSamplesRead,
					                       startTimeFile, psJobQueue->sInput.length,
						                     psJobQueue->sInput.length - psJobQueue->sInput.samplesLeft );
			if( be_kbhit() != 0 )
			{
				input = be_getch();
				if( input == 27 )
        {
          fprintf( textout, "%s                                                                             %s", mystring, mystring );
          fprintf( textout, "Quit, are you sure? (y/n)" );
					fflush( textout );
          input = be_getch();
          if( input == 'y' || input == 'Y' )
          {
            encodedChunkSize = codecExit( pBuffer );
            if( encodedChunkSize != 0 )
              if( fwrite( pBuffer, encodedChunkSize, 1, fp ) != 1 )
              {
                fprintf( textout, "ERROR: Couldn't write '%s'! Disc probably full.\n", psJobQueue->outputFilename );
                quit( -1 );
              }
            free( pBuffer );
            closeInput( &psJobQueue->sInput );
						if( fp != stdout )
							fclose( fp );
            return  0;
          }
          else
            fprintf( textout, "%s                                                                             %s", mystring, mystring );
        }
      }
    }

    /* File done */


    encodedChunkSize = codecExit( pBuffer );
    if( encodedChunkSize != 0 )
      if( fwrite( pBuffer, encodedChunkSize, 1, fp ) != 1 )
      {
        fprintf( textout, "ERROR: Couldn't write '%s'! Disc probably full.\n", psJobQueue->outputFilename );
        quit( -1 );
      }
    if( fp != stdout )
			fclose( fp );
    free( pBuffer );
    if( psJobQueue->fDeleteSource == TRUE )
      remove( psJobQueue->sourceFilename );
    x = time( NULL ) - startTimeFile;
		if( !wantedQuiet )
		{
			fprintf( textout, "%s                                                                             %s", mystring, mystring );
			fprintf( textout, "Completed. Encoding time: %02d:%02d:%02d (%.2fX)\n\n",
               x/3600, (x/60)%60, x%60, ((float)psJobQueue->sInput.length) /
               ((psJobQueue->sInput.fReadStereo+1)*psJobQueue->sInput.freq*x) );
		}
    removeJobQueueEntry( psJobQueue );
  }

  /* Batch done */

  if( !wantedQuiet )
  {
    currTime = time( NULL ) - startTimeBatch;
    fprintf( textout, "All operations completed. Total encoding time: %02d:%02d:%02d\n",
             (int) currTime/3600, (int)(currTime/60)%60, (int) currTime%60 );

    if( !wantedQuit )
		{
			fprintf( textout, "Press ENTER to exit..." );
			be_getch();
		  fprintf( textout, "\n" );
		}
  }

  return 0;
}



/*____ quit() _________________________________________________________________*/

void    quit( int returnValue )
{
	if( !wantedQuit )
	{
		fprintf( textout, "Press ENTER to exit..." );
		be_getch();
	  fprintf( textout, "\n" );
	}
  exit( returnValue );
}



/*____ updateProgressIndicator() ______________________________________________*/

void    updateProgressIndicator( time_t startBatch, double batchLen, double batchDone,
                                 time_t startFile, unsigned int fileLen, unsigned int fileDone )
{
  time_t  currTime;
  float   percentageFile, percentageBatch;
  int     fileEta, batchEta;

  char    mystring[2] = { 13, 0 };                        /* CR without LF. */


  currTime = time( NULL );

  percentageFile = ((float)fileDone) / fileLen * 100;
  if( percentageFile < 100 )
  {
    fileEta = (int) (((float)(currTime - startFile)) / fileDone * (fileLen - fileDone) );

    percentageBatch = (float)(batchDone / batchLen * 100);
    batchEta = (int) (((float)(currTime - startBatch)) / batchDone * (batchLen - batchDone));
    fprintf( textout, "Status:   %4.1f%% done, ETA %02d:%02d:%02d          BATCH: %4.1f%% done, ETA %02d:%02d:%02d%s",
	           percentageFile, fileEta/3600, (fileEta/60)%60, fileEta%60,
           	 percentageBatch, batchEta/3600, (batchEta/60)%60, batchEta%60,
             mystring );
		fflush( textout );
  }
}

/*____ setOutputDir() _______________________________________________________*/

void	setOutputDir( char * pPath )
{
	int		i;

	strcpy( outputDir, pPath );
	i = strlen( outputDir ) -1;
	if( outputDir[i] != '\\' && outputDir[i] != '/' )
	{
		outputDir[i+1] = DIRECTORY_SEPARATOR;
		outputDir[i+2] = 0;
	}
}

/*____ readGlobalSwitches() ___________________________________________________*/

int			readGlobalSwitches( int argc, char * argv[] )
{
	int			ofs;
	char		arg[256];
	int			x, y;
	
	for( ofs = 0 ; ofs < argc ; ofs++ )
	{
		strcpy( arg, argv[ofs] );
		mystrupr( arg );
		if( arg[0] != '-' )
			return	ofs;
		if( !strcmp( arg+1, "MONO" ) || !strcmp( arg+1, "DM" ) )
			wantedInputType = DOWNMIX_MONO;
    else if( !strcmp( arg+1, "CRC" ) )
      wantedCRC = TRUE;
    else if( !strcmp( arg+1, "PRIVATE" ) || !strcmp( arg+1, "P" ) )
      wantedPrivate = TRUE;
    else if( !strcmp( arg+1, "COPYRIGHT" ) || !strcmp( arg+1, "C" ) )
      wantedCopyright = TRUE;
    else if( !strcmp( arg+1, "ORIGINAL" ) )
      wantedOriginal = TRUE;
    else if( !strcmp( arg+1, "COPY" ) )
      wantedOriginal = FALSE;
    else if( !strcmp( arg+1, "DELETE" ) || !strcmp( arg+1, "DEL" ) )
      wantedDeleteSource = TRUE;
    else if( !strcmp( arg+1, "QUIT" ) || !strcmp( arg+1, "Q" ))
      wantedQuit = TRUE;
		else if( !strcmp( arg+1, "SWAP" ) )
			wantedInputType = INVERSE_STEREO;
		else if( !strcmp( arg+1, "LEFTMONO" ) || !strcmp( arg+1, "LM" ))
			wantedInputType = LEFT_CHANNEL_MONO;
		else if( !strcmp( arg+1, "RIGHTMONO" ) || !strcmp( arg+1, "RM" ))
			wantedInputType = RIGHT_CHANNEL_MONO;
		else if( !strcmp( arg+1, "QUIET" ) )
			wantedQuiet = TRUE;
		else if( strstr( arg+1, "OUTDIR=" ) == arg+1 )
			setOutputDir( &argv[ofs][8] );
#ifdef  PRIO
    else if( strstr( arg+1, "PRIO=" ) == arg+1 )
		{
			strcpy( prioString, arg+6 );
			pPrioString = prioString;
		}
#endif
		else if( !strcmp( arg+1, "BR" ) )
		{
			ofs++;
			if( ofs == argc )
				return	ofs;
			wantedBitrate = atoi( argv[ofs] );
			if( wantedBitrate > 1000 )
				wantedBitrate /= 1000;
		}
    else
    {
      y = 0;
      for( x = 1 ; arg[x] >= '0' && arg[x] <= '9' ; x++ )
        y = y * 10 + (arg[x] - '0');
      if( arg[x] == 0 )
			{
        wantedBitrate = y;
				if( wantedBitrate > 1000 )
					wantedBitrate /= 1000;
			}
      else
				return	ofs;
		}
	}
	return  ofs;	
}


/*____ readLocalSwitches() ___________________________________________________*/

int			readLocalSwitches( int argc, char * argv[], Job * psJob )
{
	int			ofs;
	char		arg[256];
	int			x, y;
	
	for( ofs = 0 ; ofs < argc ; ofs++ )
	{
		strcpy( arg, argv[ofs] );
		mystrupr( arg );
		if( arg[0] != '-' )
			return	ofs;
		if( !strcmp( arg+1, "MONO" ) || !strcmp( arg+1, "DM" ) )
		{
			psJob->sInput.outputType = DOWNMIX_MONO;
			psJob->sCodec.mode = 3;
		}
    else if( !strcmp( arg+1, "CRC" ) )
      psJob->sCodec.fCRC = TRUE;
    else if( !strcmp( arg+1, "PRIVATE" ) || !strcmp( arg+1, "P" ) )
      psJob->sCodec.fPrivate = TRUE;
    else if( !strcmp( arg+1, "COPYRIGHT" ) || !strcmp( arg+1, "C" ) )
      psJob->sCodec.fCopyright = TRUE;
    else if( !strcmp( arg+1, "ORIGINAL" ) )
      psJob->sCodec.fOriginal = TRUE;
    else if( !strcmp( arg+1, "COPY" ) )
      psJob->sCodec.fOriginal = FALSE;
    else if( !strcmp( arg+1, "DELETE" ) || !strcmp( arg+1, "DEL" ) )
      psJob->fDeleteSource = TRUE;
		else if( !strcmp( arg+1, "SWAP" ) )
		{
			if( psJob->sInput.fReadStereo == TRUE )
			{
				psJob->sInput.outputType = INVERSE_STEREO;
				psJob->sCodec.mode = 3;
			}
		}
		else if( !strcmp( arg+1, "LEFTMONO" ) || !strcmp( arg+1, "LM" ))
		{
			if( psJob->sInput.fReadStereo == TRUE )
			{
				psJob->sInput.outputType = LEFT_CHANNEL_MONO;
				psJob->sCodec.mode = 3;
			}
		}
		else if( !strcmp( arg+1, "RIGHTMONO" ) || !strcmp( arg+1, "RM" ))
		{
			if( psJob->sInput.fReadStereo == TRUE )
			{
				psJob->sInput.outputType = RIGHT_CHANNEL_MONO;
				psJob->sCodec.mode = 3;
			}
		}
		else if( !strcmp( arg+1, "BR" ) )
		{
			ofs++;
			if( ofs == argc )
				return	ofs;
			psJob->sCodec.bitrate = atoi( argv[ofs] );
			if( psJob->sCodec.bitrate > 1000 )
			{			
				psJob->sCodec.bitrate /= 1000;
				wantedQuit = TRUE;
			}
		}
		else if( !strcmp( arg+1, "HQ" ) )
		{
			wantedQuit = TRUE;																							/* Dummy for l3enc support */
		}
    else
    {
      y = 0;
      for( x = 1 ; arg[x] >= '0' && arg[x] <= '9' ; x++ )
        y = y * 10 + (arg[x] - '0');
      if( arg[x] == 0 )
			{
        psJob->sCodec.bitrate = y;
				if( psJob->sCodec.bitrate > 1000 )
					psJob->sCodec.bitrate /= 1000;
			}
      else
				return	ofs;
		}
	}
	return  ofs;	
}



/*____ validateJobs() _________________________________________________________*/

int			validateJobs( Job * psJob )
{
  static  int     aValidBitrates[14] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };

  int							i;
	int							fOk = TRUE;

	while( psJob != NULL && fOk )
	{
		fOk = FALSE;
	  for( i = 0 ; i < 14 ; i++ )
  	  if( wantedBitrate == aValidBitrates[i] )
    	  fOk = TRUE;

		psJob = psJob->psNext;
	}

	if( fOk )
		return	TRUE;

  fprintf( textout, "ERROR: %d is not a valid bitrate!\n\n", wantedBitrate );

  fprintf( textout, "Valid bitrates are:\n\n" );

  for( i = 0 ; i < 13 ; i++ )
    fprintf( textout, "%d, ", aValidBitrates[i] );

  fprintf( textout, "and %d kBit.\n", aValidBitrates[13] );

  return  FALSE;
}


/*____ printUsage() ___________________________________________________________*/

int     printUsage( void )
{
        fprintf( textout, "Usage: bladeenc [global switches] input1 [output1 [switches]] input2 ...\n" );
        fprintf( textout, "\n" );
        fprintf( textout, "General switches:\n" );
        fprintf( textout, "  -[kbit], -br [kbit]  Set MP3 bitrate. Default is 128 (64 for mono output).\n" );
        fprintf( textout, "  -crc                 Include checksum data in MP3 file.\n" );
        fprintf( textout, "  -delete, -del        Delete sample after successful encoding.\n" );
        fprintf( textout, "  -private, -p         Set the private-flag in the output file.\n" );
        fprintf( textout, "  -copyright, -c       Set the copyright-flag in the output file.\n" );
        fprintf( textout, "  -copy                Clears the original-flag in the output file.\n" );
        fprintf( textout, "  -mono, -dm           Produce mono MP3 files by combining stereo channels.\n" );
        fprintf( textout, "  -leftmono, -lm       Produce mono MP3 files from left stereo channel only.\n" );
				fprintf( textout, "  -rightmono, -rm      Produce mono MP3 files from right stereo channel only.\n" );
        fprintf( textout, "  -swap                Swap left and right stereo channels.\n" );
				fprintf( textout, "\n" );
				fprintf( textout, "Global only switches:\n" );
        fprintf( textout, "  -quit, -q            Quit without waiting for keypress when finished.\n" );
				fprintf( textout, "  -outdir=[dir]        Save MP3 files in specified directory.\n" );
				fprintf( textout, "  -quiet               Disable screen output.\n" );
#ifdef  PRIO
        fprintf( textout, "  -prio=[prio]         Sets the task priority for BladeEnc. Valid settings are\n" );
        fprintf( textout, "                       HIGHEST, HIGHER, NORMAL, LOWER, LOWEST(default) and IDLE\n" );
#endif
        fprintf( textout, "\n" );
				fprintf( textout, "Input/output files can be replaced with STDIN and STDOUT respectively.\n" );
#ifdef  DRAG_DROP
        fprintf( textout, "To make a normal 128kBit MP3, just drag-n-drop your WAV onto the BladeEnc icon.\n" );
#endif
        fprintf( textout, "\n" );

        return  TRUE;
}

/*____ addCommandlineJob() ____________________________________________________*/

int			addCommandlineJob( int argc, char * argv[] )
{
	int		argOfs;
	char	temp[256];
	Job	* psOp, * psTemp;
  int		x;


  psOp = (Job *) malloc( sizeof( Job ) );
  psOp->psNext = NULL;


  /* Open Input File              */

	strcpy( temp, argv[0] );
	mystrupr( temp );
	if( strcmp( temp, "STDIN" ) == 0 )
	{
		if( !fPreparedSTDIN )
		{
			prepStdin();
			fPreparedSTDIN = TRUE;
		}
		strcpy( psOp->sourceFilename, "Standard input stream" );
	  x = openInput( &psOp->sInput, NULL );
	}
	else
	{
  	strcpy( psOp->sourceFilename, argv[0] );
	  x = openInput( &psOp->sInput, argv[0] );
  }

  if( x != TRUE )
  {
		switch( psOp->sInput.errcode )
		{
			case	-1:
				fprintf( textout, "ERROR: '%s' is not a WAV or AIFF file!\n", psOp->sourceFilename );
				break;
			case	-2:
				fprintf( textout, "ERROR: Couldn't open '%s'!\n", psOp->sourceFilename );
				break;
			case	-3:
				fprintf( textout, "ERROR: Unexpected end of file '%s'!\n", psOp->sourceFilename );
				break;
			case	-5:
				fprintf( textout, "ERROR: Necessary chunk missing in '%s'!\n", psOp->sourceFilename );
				break;
			case	-6:
				fprintf( textout, "ERROR: Sample '%s' is of an unknown subtype!\n", psOp->sourceFilename );
				break;
			default:
				fprintf( textout, "ERROR: Unknown error while opening '%s'!\n", psOp->sourceFilename );

		}
    free( psOp );
    return  FALSE;
  }

	if( !psOp->sInput.fReadStereo && (wantedInputType == STEREO || wantedInputType == INVERSE_STEREO) )
		psOp->sInput.outputType = DOWNMIX_MONO;
	else
		psOp->sInput.outputType = wantedInputType;


  /* Set sCodec.mode (MONO or STEREO) */

  if( psOp->sInput.outputType == DOWNMIX_MONO || psOp->sInput.outputType == LEFT_CHANNEL_MONO
      || psOp->sInput.outputType == RIGHT_CHANNEL_MONO )
    psOp->sCodec.mode = 3;                                                                  /* Force to mono... */
  else
  {
    psOp->sCodec.mode = 0;
	}

  /* Set frequency */

  if( psOp->sInput.freq != 44100 && psOp->sInput.freq != 48000
     && psOp->sInput.freq != 32000 )
  {
    fprintf( textout, "ERROR: Sample '%s' is not in 32, 44.1 or 48 kHz!\n", psOp->sourceFilename );
    closeInput( &(psOp->sInput) );
    free( psOp );
    return  FALSE;
  }

  psOp->sCodec.frequency = psOp->sInput.freq;

  /* Set bitrate */

  if( wantedBitrate == -1 )
  {
    if( psOp->sCodec.mode == 3 )
      wantedBitrate = 64;
    else
      wantedBitrate = 128;
  }
  else
    psOp->sCodec.bitrate = wantedBitrate;


  /* Set other parameters */

  psOp->sCodec.bitrate = wantedBitrate;

  psOp->sCodec.fPrivate = wantedPrivate;
  psOp->sCodec.fCRC = wantedCRC;
  psOp->sCodec.fCopyright = wantedCopyright;
  psOp->sCodec.fOriginal = wantedOriginal;
  psOp->fDeleteSource = wantedDeleteSource;

  /* Set unsupported parameters */

  psOp->sCodec.emphasis = 0;

	argOfs = 1;

  /* Check for output specification and set output name */

	psOp->outputFilename[0] = 0;
	if( argc > 1 )
	{
		strcpy( temp, argv[1] );
		mystrupr( temp );
		if( !strcmp( temp, "STDOUT" ) )
		{
			wantedSTDOUT = TRUE;
			strcpy( psOp->outputFilename, "STDOUT" );
			argOfs++;
    }
		else if( strstr( temp, ".MP3" ) != NULL )
		{
	    strcpy( psOp->outputFilename, argv[1] );
			argOfs++;
    }
	}

  /* Generate output name if not allready set */

  if( psOp->outputFilename[0] == 0 )
  {
		if( outputDir[0] != 0 )
		{
			strcpy( psOp->outputFilename, outputDir );

			strcpy( temp, psOp->sourceFilename );
			x = strlen( temp );
			while( temp[x] != '.' && x >=0 )
 				x--;
			
			if( x >= 0 )
				strcpy( temp + x, ".mp3" );
			else
			{
				x = strlen( temp );
				strcat( temp, ".mp3" );
			}
			
			while( x >= 0 && temp[x] != '\\' && temp[x] != '/' && temp[x] != ':' )
				x--;
			x++;

			strcat( psOp->outputFilename, temp + x );
		}
		else
		{
			strcpy( temp, psOp->sourceFilename );
			x = strlen( temp );
			while( temp[x] != '.' && x >=0 )
 				x--;

			if( x >= 0 )
				strcpy( temp + x, ".mp3" );
			else
				strcat( temp, ".mp3" );

			strcpy( psOp->outputFilename, temp );
		}
  }


	/* Read local switches */

	argOfs += readLocalSwitches( argc - argOfs, argv + argOfs, psOp );


  /* Put this Job in the batch */

  if( psJobQueue == NULL )
    psJobQueue = psOp;
  else
  {
    psTemp = psJobQueue;
    while( psTemp->psNext != NULL )
      psTemp = psTemp->psNext;
    psTemp->psNext = psOp;
  }
  return  argOfs;
}


/*____ clearJobQueue() ________________________________________________________*/

int     clearJobQueue( void )
{
  while( psJobQueue != NULL )
    removeJobQueueEntry( psJobQueue );
  return  TRUE;
}


/*____ removeQueueEntry() _____________________________________________________*/

int removeJobQueueEntry( Job * psJob )
{
  Job     * psPrev;

  /* Unlink specified entry */

  if( psJob == psJobQueue )
    psJobQueue = psJob->psNext;
  else
  {
    psPrev = psJobQueue;
    while( psPrev->psNext != psJobQueue && psPrev->psNext != NULL )
      psPrev = psPrev->psNext;

    if( psPrev->psNext == NULL )
      return  FALSE;

    psPrev->psNext = psJob->psNext;
  }

  /* Close open file, free the entry and return. */

  closeInput( &psJob->sInput );
  free( psJob );
  return  TRUE;
}























