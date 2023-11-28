/* Needed functions for the Macintosh 
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#include "jseopt.h"

#ifdef __JSE_MAC__

#  include "macfunc.h"

#  include <errors.h>
#  include <fonts.h>
#  include <processes.h>
#  if OLDROUTINELOCATIONS
#    include <strings.h>
#  else
#    include <sound.h>
#    include <textutils.h>
#  endif
#  include <qdoffscreen.h>
#  include <stdlib.h>
#  include <string.h>

jsechar *
GetExecutablePath( void )
{
   OSErr                myErr;
   ProcessInfoRec       infoRec;
   FSSpec               spec;
   ProcessSerialNumber  serialNumber;
   static jsechar        buffer[_MAX_PATH];

   buffer[0] = '\0';

   if ((myErr = GetCurrentProcess( &serialNumber)) == noErr )
   {
      infoRec.processAppSpec = & spec;
      infoRec.processInfoLength = sizeof( ProcessInfoRec );
      infoRec.processName = nil;
      if ((myErr = GetProcessInformation( &serialNumber, &infoRec)) == noErr )
      {
         FSSpecToFullPath( &spec, _MAX_PATH, buffer );
      }
   }

   if ( buffer[0] == '\0' )
      return NULL;

   return buffer;
}

   jsebool
MakeFullPath(jsechar *buffer,const jsechar *path,unsigned int buflen )
{
   const jsechar * temp;

   temp = MakeCompletePath( NULL, path );

   if( temp == NULL )
      return False;

   if( strlen(temp) > buflen )
   {
      strncpy( buffer, temp, buflen );
      buffer[buflen] = '\0';
   }
   else
      strcpy(buffer, temp);

   return True;
}



const jsechar *
MakeCompletePath( jseContext jsecontext, const jsechar * partial )
{
   static jsechar tempPath[_MAX_PATH + 100]; /* A little extra space can't hurt */
   static jsechar completePath[_MAX_PATH + 100];

   /* Now we must convert double colons */
   jsechar * temp;
   jsechar * temp_loc = tempPath;
   jsechar * complete_loc = completePath;
   int directory_depth = 0;
   int dir_length;


   if ( partial == NULL || strlen( partial ) == 0 )
      return NULL;

   if ( strlen( partial ) == 1 && partial[0] == '.' )
   { /* Just the current directory */
      if ( jsecontext != NULL )
      {
         if( MACCWD_CONTEXT != NULL )
            strcpy( tempPath, MACCWD_CONTEXT);
         else
            getcwd( tempPath, _MAX_PATH );
      }
      else
      {
         getcwd( tempPath, _MAX_PATH );
      }
   }
   else if ( isFullPath( partial ))
   {
      strcpy( tempPath, partial );
   }
   else
   {
      if ( jsecontext != NULL )
      {
         if( MACCWD_CONTEXT != NULL )
         {
            strcpy(tempPath,MACCWD_CONTEXT);
         }
         else
            getcwd( tempPath, _MAX_PATH );
      }
      else
      {
         getcwd( tempPath, _MAX_PATH );
      }

      if ( partial[0] == ':' )
         strcpy( tempPath + strlen(tempPath), partial + 1); /* Skip initial colon */
      else
         strcpy( tempPath + strlen(tempPath), partial );
   }

   while( *temp_loc != '\0' )
   {
      if((*temp_loc == ':')) /* This is a second colon in a row */
      {
         if( directory_depth > 1 ) /* We cannot back out of the volume name ( ':' is not a valid path ) */
         {
            *(complete_loc-1) = '\0'; /* Overwrite the colon */
            if((temp = strrchr( completePath, ':' )) == NULL )
               return NULL;
            complete_loc = temp + 1;
            temp_loc++;
         }
         else
         {
            temp_loc++; /* Simply skip it */
         }
      }
      else
      {
         if((temp = strchr(temp_loc, ':')) == NULL )
         {
            strcpy( complete_loc, temp_loc );
            return completePath;
         }

         dir_length = temp - temp_loc + 1; /* Includes the colon */
         strncpy( complete_loc, temp_loc, (size_t)dir_length);
         complete_loc += dir_length;
         temp_loc += dir_length;
         directory_depth++;
      }
   }

   *complete_loc = '\0';

   return completePath;
}


#  if !defined(NDEBUG)
void my_assertion_failed(jsechar * condition, jsechar * filename, int lineno);

/* This is an assertion function that is used with Nombas internally, and is needed to link to
 * Nombas's internal debug libraries.  You may remove this function if you wish.
 */

void my_assertion_failed(jsechar * condition, jsechar * filename, int lineno)
{
   static jsechar big_buffer[2048];
   sprintf( big_buffer, "Assertion failed (%s) on line %d of file \"%s\"", condition, lineno, filename);

   DoMacTextBox( big_buffer, NULL );

   exit( 1 );
}

#  endif

jsebool
isFullPath( const jsechar * path )
{
   jsechar * temp;
   jsebool   success = False;

   if ( path[0] == ':' )
      return False;
   else if ((temp = strchr(path,':')) == NULL ) /* Simple file name */
      return False;
   else
   {
      OSErr           myErr;
      HParamBlockRec  theRecord;
      unsigned char   volumeName[31];

      temp = strchr(path, ':'); /* This isn't necessary (since it was done above) but it can't hurt */
      *temp = '\0';             /* Partial is now a null terminated string with the first directory name */

      theRecord.volumeParam.ioNamePtr = volumeName;
      theRecord.volumeParam.ioVolIndex = 0;

      while( 1 )
      {
         theRecord.volumeParam.ioVolIndex++;
         myErr = PBHGetVInfoSync( &theRecord );

         if ( myErr == nsvErr ) /* We have searched all the volumes */
            break;

         if ( myErr != noErr ) /* Some sort of error occured (give up on this volume) */
            continue;

         p2cstr( volumeName );
         if ( stricmp((char *) volumeName, path) == 0 ) /* This is a volume name */
         {
            success = true;
            break;
         }
      }
      *temp = ':';
   }
   return success;
}

pascal  OSErr
FSSpecToFullPath(const FSSpec *spec, short length, jsechar *fullPath)
{
   OSErr       result = noErr;
   FSSpec      tempSpec;
   CInfoPBRec  pb;
   static jsechar temp[_MAX_PATH];

   UNUSED_PARAMETER( length );

   /* Make a copy of the input FSSpec that can be modified */
   BlockMoveData(spec, &tempSpec, sizeof(FSSpec));

   if ( tempSpec.parID == fsRtParID )
   {
      /* The object is a volume */

      p2cstr( tempSpec.name );
      strcpy( fullPath, (jsechar *) tempSpec.name );
      strcat( fullPath, ":" );
   }
   else
   {
      /* The object isn't a volume */
      p2cstr( tempSpec.name );
      strcpy( fullPath, (jsechar *) tempSpec.name );
      c2pstr((jsechar *) tempSpec.name );

      /* Put the object name in first */
      if ( result == noErr )
      {
         /* Get the ancestor directory names */
         pb.dirInfo.ioNamePtr = tempSpec.name;
         pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
         pb.dirInfo.ioDrParID = tempSpec.parID;
         do /* loop until we have an error or find the root directory */
         {
            pb.dirInfo.ioFDirIndex = -1;
            pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;
            result = PBGetCatInfoSync(&pb);
            if ( result == noErr )
            {
               p2cstr( tempSpec.name );
               strcpy(temp, (jsechar *) tempSpec.name );
               strcat(temp, ":" );
               strcat(temp, fullPath );
               strcpy(fullPath, temp );
               c2pstr((jsechar *) tempSpec.name );
            }
         } while ((result == noErr) && (pb.dirInfo.ioDrDirID != fsRtDirID));
      }
   }

   return ( result );
}

#  ifdef JSE_SELIB_DIRECTORY

OSErr DetermineVRefNum( StringPtr, short, short * ); /* Gets rid of compiler warning */


OSErr DetermineVRefNum( StringPtr pathname,
                        short vRefNum,
                        short *realVRefNum )
{
   HParamBlockRec pb;
   Str255 tempPathname;
   OSErr error;

   pb.volumeParam.ioVRefNum = vRefNum;
   if ( pathname == NULL )
   {
      pb.volumeParam.ioNamePtr = NULL;
      pb.volumeParam.ioVolIndex = 0; /* use ioVRefNum only */
   }
   else
   {
      BlockMoveData(pathname, tempPathname, pathname[0] + 1); /* make a copy of the string and */
      pb.volumeParam.ioNamePtr = (StringPtr)tempPathname;     /* use the copy so original isn't trashed */
      pb.volumeParam.ioVolIndex = -1;                         /* use ioNamePtr/ioVRefNum combination */
   }
   error = PBHGetVInfoSync(&pb);
   *realVRefNum = pb.volumeParam.ioVRefNum;
   return ( error );
}



OSErr GetDirectoryID(short vRefNum,
                 long dirID,
                 StringPtr name,
                 long *theDirID,
                 Boolean *isDirectory)
{
   CInfoPBRec pb;
   Str31 tempName;
   OSErr error;

   /* Protection against File Sharing problem */
   if ((name == NULL) || (name[0] == 0))
   {
      tempName[0] = 0;
      pb.hFileInfo.ioNamePtr = tempName;
      pb.hFileInfo.ioFDirIndex = -1; /* use ioDirID */
   }
   else
   {
      pb.hFileInfo.ioNamePtr = name;
      pb.hFileInfo.ioFDirIndex = 0; /* use ioNamePtr and ioDirID */
   }
   pb.hFileInfo.ioVRefNum = vRefNum;
   pb.hFileInfo.ioDirID = dirID;
   error = PBGetCatInfoSync(&pb);
   *isDirectory = (pb.hFileInfo.ioFlAttrib & ioDirMask) != 0;
   *theDirID = (*isDirectory) ? pb.dirInfo.ioDrDirID : pb.hFileInfo.ioFlParID;
   return ( error );
}


/* The IterateGlobals structure is used to minimize the amount of
 * stack space used when recursively calling IterateDirectoryLevel
 * and to hold global information that might be needed at any time.
*/
#    if PRAGMA_ALIGN_SUPPORTED
#      pragma options align=mac68k
#    endif
struct IterateGlobals
{
  IterateFilterProcPtr  iterateFilter; /* pointer to IterateFilterProc */
  CInfoPBRec        cPB;               /* the parameter block used for PBGetCatInfo calls */
  Str63         itemName;              /* the name of the current item */
  OSErr         result;                /* temporary holder of results - saves 2 bytes of stack each level */
  Boolean         quitFlag;            /* set to true if filter wants to kill interation */
  unsigned short      maxLevels;       /* Maximum levels to iterate through */
  unsigned short      currentLevel;    /* The current level IterateLevel is on */
  void          *yourDataPtr;          /* A pointer to caller data the filter may need to access */
};
#    if PRAGMA_ALIGN_SUPPORTED
#      pragma options align=reset
#    endif

typedef struct IterateGlobals IterateGlobals;
typedef IterateGlobals *IterateGlobalsPtr;

static  void  IterateDirectoryLevel(long dirID,
                    IterateGlobalsPtr theGlobals);

static  void  IterateDirectoryLevel(long dirID,
                    IterateGlobalsPtr theGlobals)
{
   if ((theGlobals->maxLevels == 0) ||                    /* if maxLevels is zero, we aren't checking levels */
      (theGlobals->currentLevel < theGlobals->maxLevels)) /* if currentLevel < maxLevels, look at this level */
   {
      short index = 1;

      ++theGlobals->currentLevel; /* go to next level */

      do
      { /* Isn't C great... What I'd give for a "WITH theGlobals DO" about now... */

         /* Get next source item at the current directory level */

         theGlobals->cPB.dirInfo.ioFDirIndex = index;
         theGlobals->cPB.dirInfo.ioDrDirID = dirID;
         theGlobals->result = PBGetCatInfoSync((CInfoPBPtr)&theGlobals->cPB);

         if ( theGlobals->result == noErr )
         {
            /* Call the IterateFilterProc */
            CallIterateFilterProc(theGlobals->iterateFilter, &theGlobals->cPB, &theGlobals->quitFlag, theGlobals->yourDataPtr);

            /* Is it a directory? */
            if ((theGlobals->cPB.hFileInfo.ioFlAttrib & ioDirMask) != 0 )
            {
               /* We have a directory */
               if ( !theGlobals->quitFlag )
               {
                  /* Dive again if the IterateFilterProc didn't say "quit" */
                  IterateDirectoryLevel(theGlobals->cPB.dirInfo.ioDrDirID, theGlobals);
               }
            }
         }

         ++index;                                                         /* prepare to get next item */
      } while ((theGlobals->result == noErr) && (!theGlobals->quitFlag)); /* time to fall back a level? */

      if ( theGlobals->result == fnfErr ) /* fnfErr is OK - it only means we hit the end of this level */
         theGlobals->result = noErr;

      --theGlobals->currentLevel; /* return to previous level as we leave */
   }
}


OSErr IterateDirectory(short vRefNum,
                 long dirID,
                 StringPtr name,
                 unsigned short maxLevels,
                 IterateFilterProcPtr iterateFilter,
                 void *yourDataPtr)
{
   IterateGlobals  theGlobals;
   OSErr     result;
   long      theDirID;
   short     theVRefNum;
   Boolean     isDirectory;

   /* Make sure there is a IterateFilter */
   if ( iterateFilter != NULL )
   {
      /* Get the real directory ID and make sure it is a directory */
      result = GetDirectoryID(vRefNum, dirID, name, &theDirID, &isDirectory);
      if ( result == noErr )
      {
         if ( isDirectory == true )
         {
            /* Get the real vRefNum */
            result = DetermineVRefNum(name, vRefNum, &theVRefNum);
            if ( result == noErr )
            {
               /* Set up the globals we need to access from the recursive routine. */
               theGlobals.iterateFilter = iterateFilter;
               theGlobals.cPB.hFileInfo.ioNamePtr = (StringPtr)&theGlobals.itemName;
               theGlobals.cPB.hFileInfo.ioVRefNum = theVRefNum;
               theGlobals.itemName[0] = 0;
               theGlobals.result = noErr;
               theGlobals.quitFlag = false;
               theGlobals.maxLevels = maxLevels;
               theGlobals.currentLevel = 0; /* start at level 0 */
               theGlobals.yourDataPtr = yourDataPtr;

               /* Here we go into recursion land... */
               IterateDirectoryLevel(theDirID, &theGlobals);

               result = theGlobals.result; /* set the result */
            }
         }
         else
         {
            result = dirNFErr; /* a file was passed instead of a directory */
         }
      }
   }
   else
   {
      result = paramErr; /* iterateFilter was NULL */
   }

   return ( result );
}

#  endif /* JSE_TOOLKIT_INCL_DIRECTORY */

#  define  ButtonOffset  10
#  define  ButtonWidth   68
#  define  ButtonHeight  20

jsebool qdInitialized = false;

struct MacTextBox *
   NewMacTextBox()
{
   Rect defaultRect = { 0, 0, 10, 10 }; /* This is resized later */
   Rect teRect = { 5, 5, 500, 5 };
   struct MacTextBox * This = jseMustMalloc(struct MacTextBox, sizeof(struct MacTextBox));
   short font_id;
   FontInfo info;

   This->textBox = NULL;
   This->window = NULL;
   This->OKButton = NULL;

#if !defined(IN_CODE_RESOURCE) && !defined(__CENVI__)
   if( !qdInitialized )
   {
      InitGraf(&qd.thePort);
      qdInitialized = True;
   }
#endif

   if ((This->window = NewWindow( nil, &defaultRect, nil, FALSE, dBoxProc, (WindowPtr) -1,
                                    FALSE, 0 )) == NULL )
      return NULL;

   SetPort( This->window );

   if ((This->textBox = TENew( &teRect, &teRect )) == NULL )
      return NULL;

   if ((This->OKButton = NewControl(This-> window, &defaultRect, "\pOK", TRUE, 0, 0, 0,
                                        pushButProc, 0)) == NULL )
      return NULL;

   GetFNum( "\pMonaco", &font_id );
   TextFont( font_id );
   TextSize( 9 );
   GetFontInfo( &info );
   (**This->textBox).txSize = 9;
   (**This->textBox).txFont = font_id;
   (**This->textBox).lineHeight = (short) (info.ascent + info.descent + info.leading);
   (**This->textBox).fontAscent = info.ascent;

   return This;
}

void
DeleteMacTextBox( struct MacTextBox * This )
{
   if ( This->textBox != NULL )
      TEDispose( This->textBox );
   if ( This->window != NULL )
      DisposeWindow( This->window ); /* This will dispose of the button */
   jseMustFree(This);
}


/* Finds the longest line in a string */
static int
longest_line( struct MacTextBox * This, jsechar * message, int * line_count )
{
   jsechar *temp1, *temp2;
   size_t max_length = 0;

   UNUSED_PARAMETER( This );

   /* Find the longest line */
   temp1 = (jsechar *) strchr( message, '\r' );

   *line_count = 1;
   if ( temp1 != NULL )
   {
      if ( strchr( temp1 + 1, '\r') == NULL )
      {
         max_length = (size_t) (temp1 - message);
      }
      else
      {
         while((temp2 = strchr( temp1 + 1, '\r' )) != NULL )
         {
            if ((int)(temp2 - temp1) > max_length )
            {
               max_length = (size_t) (temp2 - temp1);
            }
            temp1 = temp2;
            (*line_count)++;
         }
      }
   }
   else
   {
      max_length = strlen( message );
   }

   return (int) max_length;
}

/* Tranlates '\r\n' to '\r' and returns a COPY of the string */
static jsechar *
translate_newlines( struct MacTextBox * This, const jsechar * _message )
{
   jsechar * message = StrCpyMalloc( _message );
   jsechar * current = message;

   UNUSED_PARAMETER( This );

   while ((current = strstr( current, "\r\n" )) != NULL )
   {
      memmove( current + 1, current + 2, sizeof(jsechar)*(strlen_jsechar(current) - 1) );
   }
   return message;
}

static jsechar *
find_break( struct MacTextBox * This, jsechar * text, int max_length )
{
   jsechar * end = text + max_length + 1;
   const jsechar * break_chars = " ,:;.";
   jsebool break_found = False;
   jsechar * break_point = NULL;
   int j;
   
   UNUSED_PARAMETER( This );

   for(j = 0; (j < strlen(break_chars)) && !break_found; j++ )
   {
      jsechar break_char = break_chars[j];
      jsechar * current = end;

      for( ; current > text && !break_found; current-- )
      {
         if ( *current == break_char )
         {
            break_point = current + 1;
            break_found = True;
         }
      }
   }

   if ( !break_found )
      break_point = text + max_length + 1;

   return break_point;

}

static int
adjust_width( struct MacTextBox * This, jsechar ** message )
{
   const jsechar * indent = "\r        ";
   int number_of_lines;
   int max_length, length;
   int char_width;
   QDGlobals * qdptr;

   SetPort( This->window );
   TextFont((**This->textBox).txFont );
   TextSize((**This->textBox).txSize );

#if defined(IN_CODE_RESOURCE)
   qdptr = (QDGlobals*)(*((long*)SetCurrentA5()) - (sizeof(QDGlobals)-sizeof(GrafPtr)));
#else
   qdptr = &qd;
#endif

   char_width = TextWidth( "m", 0, 1 );
   max_length = (int) ((qdptr->screenBits.bounds.right - qdptr->screenBits.bounds.left) * .6 / char_width);
   length = longest_line( This, *message, &number_of_lines );

   if ( length > max_length )
   {
      jsechar * big_buffer = jseMustMalloc( jsechar, 
         sizeof(jsechar) * (strlen(*message) + strlen(indent) *( number_of_lines+4)));
      int temp_length;
      jsechar *temp1, *temp2, *word_break;
      temp1 = *message;
      length  = 0;
      big_buffer[0] = '\0';

      while( temp1 != NULL )
      {
         if ((temp2 = strchr( temp1, '\r' )) != NULL )
         {
            if ( temp2 - temp1 > max_length ) /* This line is too long */
            {
               word_break = find_break( This, temp1, max_length );
               temp_length = (int) (word_break - temp1) + 1;
               length = max( length, temp_length - 1 );
               strncat( big_buffer, temp1, (size_t) temp_length - 1);
               strcat( big_buffer, indent );
               temp1 = word_break;
            }
            else
            {
               temp_length = (int) (temp2 - temp1) + 1;
               length = max( length, temp_length - 1 );
               strncat( big_buffer, temp1, (size_t) temp_length );
               temp1 = temp2 + 1;
            }
         }
         else
         {
            if ( strlen( temp1 ) > max_length )
            {
               word_break = find_break( This, temp1, max_length );
               if( big_buffer[0] != '\0' )
               {
                  if( big_buffer[strlen(big_buffer)-1] != '\r' )
                     strcat( big_buffer, indent );
                  else
                     strcat( big_buffer, indent + 1 );
               }
               temp_length = (int) (word_break - temp1) + 1;
               length = max( length, temp_length - 1);
               strncat( big_buffer, temp1, (size_t)temp_length);
               temp1 = word_break;
            }
            else
            {
               if( big_buffer[0] != '\0' )
               {
                  if( big_buffer[strlen(big_buffer)-1] != '\r' )
                     strcat( big_buffer, indent );
                  else
                     strcat( big_buffer, indent + 1 );
               }
               temp_length = (int) strlen(temp1) + 1;
               length = max( length, temp_length - 1 );
               strncat( big_buffer, temp1, (size_t) temp_length );
               temp1 = NULL;
            }
         }
      }
      *message = jseMustReMalloc( jsechar, *message, sizeof(jsechar) * (strlen(big_buffer) + 1) );
      memcpy( *message, big_buffer, sizeof(jsechar)*(strlen(big_buffer) + 1) );
      jseMustFree( big_buffer );
   }

   return length * char_width + 5;
}

/* This function is fairly complex and probably could be cleaned up a bit, but it works (I hope) */
void
MacTextBoxSet( struct MacTextBox * This, const jsechar * _message )
{
   int max_length = 0;
   Rect teRect;
   jsechar * message = translate_newlines( This, _message );
   int box_width = adjust_width( This, &message );
   int box_height;
   QDGlobals * qdptr;
   
#if defined(IN_CODE_RESOURCE)
   qdptr = (QDGlobals*)(*((long*)SetCurrentA5()) - (sizeof(QDGlobals)-sizeof(GrafPtr)));
#else
   qdptr = &qd;
#endif

   assert( _message != NULL );
   assert( This->window != NULL );
   assert( This->OKButton != NULL );
   assert( This->textBox != NULL );

   SetRect( &teRect, 5, 5, (short) (10 +box_width), 5 );
   (**This->textBox).destRect = teRect;
   (**This->textBox).viewRect = teRect;
   TECalText(This->textBox);
   TESetText((void *) message, (long) strlen( message ), This->textBox );
   box_height = (**This->textBox).lineHeight * (**This->textBox).nLines;
   SetRect( &teRect, 5, 5, (short) (5 + box_width), (short) (5 + box_height) );
   (**This->textBox).destRect = teRect;
   (**This->textBox).viewRect = teRect;

   SizeControl( This->OKButton, ButtonWidth, ButtonHeight );
   MoveControl( This->OKButton, (short) (teRect.right - 2 * ButtonOffset - ButtonWidth), (short) (teRect.bottom + ButtonOffset));

   /* This isn't great positioning, but it works */
   SizeWindow( This->window, (short) (teRect.right + 5), (short) (teRect.bottom + 2 * ButtonOffset + ButtonHeight), TRUE );
   MoveWindow( This->window, (short) (qdptr->screenBits.bounds.right / 2 - (teRect.right + 5) / 2),
                       (short) (qdptr->screenBits.bounds.bottom / 4), TRUE );

   jseMustFree( message );
}

void
MacTextBoxShow( struct MacTextBox * This, void (*eventHandler)( WindowPtr window, EventRecord * event ))
{
   /* Now we enter the event loop */
   jsebool      finished = False;
   EventRecord  event;
   WindowPtr    theWindow;

   QDGlobals * qdptr;

#if defined(IN_CODE_RESOURCE)
   qdptr = (QDGlobals*)(*((long*)SetCurrentA5()) - (sizeof(QDGlobals)-sizeof(GrafPtr)));
#else
   qdptr = &qd;
#endif

   ShowWindow( This->window );
   SelectWindow( This->window );

   while( !finished )
   {
      if ( WaitNextEvent( everyEvent, &event, 60L, nil ))
      {
         switch( event.what )
         {
            case activateEvt:
               theWindow = (WindowPtr) event.message;

               if ( theWindow != This->window && eventHandler != NULL )
                  eventHandler( theWindow, &event );
               break;
            case mouseDown:
               {
                  int part = FindWindow( event.where, &theWindow );

                  if ( theWindow == This->window )
                  {
                     int control_part;
                  
                     ControlHandle control;
                     SetPort( This->window );
                     GlobalToLocal( &event.where );
                     control_part = FindControl( event.where, This->window, &control );
                     if ( control_part == kControlButtonPart && control == This->OKButton )
                     {
                        if ( kControlButtonPart == TrackControl( This->OKButton, event.where, nil ))
                        {
                           finished = TRUE;
                        }
                     }
                  }
                  else
                  {
                     SysBeep( 10 );
                  }

               }
               break;
            case keyDown: /* fall through */
            case autoKey:
               {
                  jsechar theChar = (jsechar) (event.message & charCodeMask);
                  if( '\r' == theChar || '\n' == theChar )
                  {
                     unsigned long ignored;
                     HiliteControl( This->OKButton, 10 );
                     Delay( 8, &ignored );
                     HiliteControl( This->OKButton, 0 );
                     finished = TRUE;
                  }
               }

               break;
            case updateEvt:

               theWindow = (WindowPtr) event.message;

               if ( theWindow == This->window )
               {
                  BeginUpdate( This->window );
                  {
                     Rect itemRect;
                     int buttonOval;
                  
                     SetPort( This->window );
                     TEUpdate( &(**This->textBox).viewRect, This->textBox );
                     DrawControls( This->window );

                     itemRect = (**This->OKButton).contrlRect;
                     InsetRect( &itemRect, -4, -4 );
                     buttonOval = ((itemRect.bottom - itemRect.top) / 2) + 2;
                     PenPat(&qdptr->black);
                     PenSize(3, 3);
                     FrameRoundRect(&itemRect, (short) buttonOval, (short) buttonOval);
                  }
                  EndUpdate( This->window );
               }
               else
               {
                  if ( eventHandler != NULL )
                     eventHandler( theWindow, &event );
               }

               break;

         }
      }
   }

   HideWindow( This->window );
}

void DoMacTextBox( const jsechar * message, void (*eventHandler)(WindowPtr window, EventRecord * event))
{
   struct MacTextBox * textbox = NewMacTextBox();

   if ( MacTextBoxIsValid( textbox ))
   {
      MacTextBoxSet( textbox, message );
      SysBeep( 10 );
      MacTextBoxShow( textbox, eventHandler );
   }
   else
   {
      SysBeep( 10 );
   }

   DeleteMacTextBox( textbox );
}

#endif /* __JSE_MAC__ */
