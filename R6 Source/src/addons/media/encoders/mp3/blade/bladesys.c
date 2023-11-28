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

/******************************************************************************
			>>> BLADESYS <<<

The intention of this file is to keep all system specific things away from the
rest of the code.

******************************************************************************/



#include		<stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <time.h>
#include		<fcntl.h>
#include    "system.h"
#ifdef  MSWIN
	#include	<windows.h>
#include    <conio.h> 
#include    <io.h> 
#endif

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


/******************************************************************************

									>>> SYSTEM FOR PRIORITY HANDLING <<<

******************************************************************************/


#ifdef OS2

typedef struct
{
  ULONG ulClass;
  LONG loDelta;
} OS2PRIORITIES;

OS2PRIORITIES OS2PrioTab[] =
{
  { PRTYC_IDLETIME, 0 },
  { PRTYC_IDLETIME, 31 },
  { PRTYC_REGULAR, -31 },
  { PRTYC_REGULAR, 0 },
  { PRTYC_REGULAR, 31 },
  { PRTYC_TIMECRITICAL, 0 },
};

#endif


int		setPriority( char * pPrioString )
{

#ifdef  MSWIN
	HANDLE	hThread;

	hThread = GetCurrentThread();

	if( pPrioString == NULL )
	  SetThreadPriority( hThread, THREAD_PRIORITY_LOWEST );				/* Set default priority if NULL! */
  else if( strcmp( pPrioString, "HIGHEST" ) == 0 )
	  SetThreadPriority( hThread, THREAD_PRIORITY_HIGHEST );
  else if( strcmp( pPrioString, "HIGHER" ) == 0 )
	  SetThreadPriority( hThread, THREAD_PRIORITY_ABOVE_NORMAL );
  else if( strcmp( pPrioString, "NORMAL" ) == 0 )
	  SetThreadPriority( hThread, THREAD_PRIORITY_NORMAL );
  else if( strcmp( pPrioString, "LOWER" ) == 0 )
	  SetThreadPriority( hThread, THREAD_PRIORITY_BELOW_NORMAL );
  else if( strcmp( pPrioString, "LOWEST" ) == 0 )
	  SetThreadPriority( hThread, THREAD_PRIORITY_LOWEST );
  else if( strcmp( pPrioString, "IDLE" ) == 0 )
	  SetThreadPriority( hThread, THREAD_PRIORITY_IDLE );
  else
    return	FALSE;

#endif


#ifdef OS2
  APIRET	rc;
	int			prio;

	if( pPrioString == NULL )
	  prio = 1;																										/* Set default priority if NULL! */
  else if( strcmp( pPrioString, "HIGHEST" ) == 0 )
	  prio = 5;
  else if( strcmp( pPrioString, "HIGHER" ) == 0 )
	  prio = 4;
  else if( strcmp( pPrioString, "NORMAL" ) == 0 )
	  prio = 3;
  else if( strcmp( pPrioString, "LOWER" ) == 0 )
	  prio = 2;
  else if( strcmp( pPrioString, "LOWEST" ) == 0 )
	  prio = 1;
  else if( strcmp( pPrioString, "IDLE" ) == 0 )
	  prio = 0;
  else
    return	FALSE;
  
	if ((rc = DosSetPriority (PRTYS_PROCESS,
                            OS2PrioTab[prio].ulClass,
                            OS2PrioTab[prio].loDelta,
                            0L)) != NO_ERROR)
  {
    printf (" DosSetPriority error : rc = %u\n", rc);
    exit(1);
  }
#endif

	/* Include Prioritysettings for other systems here... */

	return	TRUE;
}


/* Some systems are by default using text-input/output instead of binary.:-( */

void	prepStdin( void )
{
#ifdef	MSWIN
	_setmode(_fileno(stdin), _O_BINARY );		
#endif
}

void	prepStdout( void )
{
#ifdef	MSWIN
	_setmode(_fileno(stdout), _O_BINARY );		
#endif
}


/******************************************************************************

											>>> ROUTINES FOR KEYHANDLING <<<

******************************************************************************/


int		be_kbhit( void )
{
#ifdef	MSWIN
	return	kbhit();
#endif
#ifdef	OS2
	return	DosKeyAvailable();
#endif
	return	0;
}

int		be_getch( void )
{
#if	defined(MSWIN) || defined(OS2)
	return	getch();
#endif
	return	0;
}

/******************************************************************************

		>>> EXPAND WILDCARDS FOR SYSTEMS THAT DON'T DO IT AUTOMATICALLY <<<

******************************************************************************/

#ifdef	WILDCARDS

typedef struct argLinkDef	argLink;

struct argLinkDef
{
	argLink	* psNext;
	char		* pString;
};


int findFirstMatch( char * pFileName, char * wpName );
int findNextMatch(  char * pFileName, char * wpName );


/*____ expandWildcards() ____________________________________________________*/

void *	expandWildcards( int * pArgc, char ** pArgv[] )
{
	argLink	* pArgLink = NULL;
	argLink ** wppLink = &pArgLink;
	argLink	* pLink;

	int		oldArgc, newArgc;
	char	** oldArgv, ** newArgv;

	int		i, x;
	char	temp[MAX_NAMELEN];

	oldArgc = * pArgc;
	newArgc = 0;
	oldArgv = * pArgv;
	
	/* Main loop */

	for( i = 0 ; i < oldArgc ; i++ )
	{
		if( strchr( oldArgv[i], '*' ) != NULL || strchr( oldArgv[i], '?' ) != NULL )
		{
			x = findFirstMatch( oldArgv[i], temp );
			while( x == TRUE )
			{
				pLink = (argLink *) malloc( sizeof(argLink) + strlen(temp) + 1 );
				strcpy( ((char *) &pLink[1]), temp );
				pLink->pString = ((char *) &pLink[1]);

				* wppLink = pLink;
				wppLink = &pLink->psNext;
				newArgc++;
				
				x = findNextMatch( oldArgv[i], temp );	
			}			
		}
		else
		{
			pLink = (argLink *) malloc( sizeof(argLink) );
			* wppLink = pLink;
			pLink->pString = oldArgv[i];
			wppLink = &pLink->psNext;
			newArgc++;
		}

	}
	* wppLink = NULL;											/* Terminate the link.*/

	/* Generate new Argv-table */

	newArgv = (char **) malloc( newArgc*4 );
	pLink = pArgLink;
	for( i = 0 ; i < newArgc ; i++ )
	{
		newArgv[i] = pLink->pString;
		pLink = pLink->psNext;		
	}
	
	* pArgc = newArgc;
	* pArgv = newArgv;
	return	pArgLink;
}

/*____ freeExpandWildcardMem() ______________________________________________*/

void	freeExpandWildcardMem( void * pArgLink )
{
	argLink * pLink, * pFree;

	pLink = (argLink *) pArgLink;
	while( pLink != NULL )
	{
		pFree = pLink;
		pLink = pLink->psNext;
		free( pFree );
	}
}



#ifdef  MSWIN

static struct   _finddata_t sFind;
static long             hFind;

/*____ findFirstMatch() - MSWIN version ______________________________________*/

int findFirstMatch( char * pFileName, char * wpName )
{
  int             x;

  hFind = _findfirst( pFileName, &sFind );
  if( hFind == -1 )
   	return  FALSE;

  if( (sFind.attrib & _A_SUBDIR) != 0 )
    return findNextMatch( pFileName, wpName );


  strcpy( wpName, pFileName );
  for( x = strlen(wpName)-1 ; wpName[x] != '\\' && wpName[x] !='/'
      	&& wpName[x] != ':' && x > 0 ; x-- );

  if( x != 0 )
   	x++;
  strcpy( wpName + x, sFind.name );
  return TRUE;
}


/*____ findNextMatch() - MSWIN version _______________________________________*/
int findNextMatch( char * pFileName, char * wpName )
{
	int		x;

  while( 1 )
  {
   	if( _findnext( hFind, &sFind ) != 0 )
      return  FALSE;

    if( (sFind.attrib & _A_SUBDIR) == 0 )
    {
      strcpy( wpName, pFileName );
      for( x = strlen(wpName)-1 ; wpName[x] != '\\' && wpName[x] !='/'
           && wpName[x] != ':' && x > 0 ; x-- );

      if( x != 0 )
        x++;
      strcpy( wpName + x, sFind.name );
      return TRUE;
    }
	}
}

#endif  /* MSWIN */

#ifdef OS2

static HDIR          hdirFindHandle = HDIR_SYSTEM;
static FILEFINDBUF3  FindBuffer     = {0};
static ULONG         ulResultBufLen = sizeof(FILEFINDBUF3);
static ULONG         ulFindCount    = 1;

/*____ findFirstMatch() - OS/2 version _______________________________________*/

int     findFirstMatch( char * pFileName, char * wpName )
{
        int             x;
        APIRET rc = NO_ERROR;

        ulFindCount = 1;
        rc = DosFindFirst( pFileName,
                           &hdirFindHandle,
                           FILE_NORMAL,
                           &FindBuffer,
                           ulResultBufLen,
                           &ulFindCount,
                           FIL_STANDARD);

        if (rc != NO_ERROR)
          return FALSE;

        if( (FindBuffer.attrFile & FILE_DIRECTORY) != 0 )
                return findNextMatch( pFileName, wpName );

        strcpy( wpName, pFileName );
        for( x = strlen(wpName)-1 ; wpName[x] != '\\' && wpName[x] !='/'
                         && wpName[x] != ':' && x > 0 ; x-- );

        if( x != 0 )
                x++;
        strcpy( wpName + x, FindBuffer.achName );
        return TRUE;
}

/*____ findNextMatch() - OS/2 version ________________________________________*/

int     findNextMatch( char * pFileName, char * wpName )
{
        int             x;
        APIRET rc = NO_ERROR;

        while( 1 )
        {
                ulFindCount = 1;

                if ((rc = DosFindNext(hdirFindHandle,
                                      &FindBuffer,
                                      ulResultBufLen,
                                      &ulFindCount)) != NO_ERROR)
                {
                  if ((rc = DosFindClose(hdirFindHandle)) != NO_ERROR)
                    printf("DosFindClose error: return code = %u\n",rc);
                  return FALSE;
                }

                if( (FindBuffer.attrFile & FILE_DIRECTORY) == 0 )
                {
                        strcpy( wpName, pFileName );
                        for( x = strlen(wpName)-1 ; wpName[x] != '\\' && wpName[x] !='/'
                                         && wpName[x] != ':' && x > 0 ; x-- );

                        if( x != 0 )
                                x++;
                        strcpy( wpName + x, FindBuffer.achName );
                        return TRUE;
                }
        }
}

#endif /* OS/2 */


#endif /* WILDCARDS */



