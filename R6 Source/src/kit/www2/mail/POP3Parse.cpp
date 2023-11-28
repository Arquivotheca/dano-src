// POP3Parse.cpp -- Parses RFC822 message bodies to MimeMessage class
// by Allen Brunson  June 1, 2001

#include <ctype.h>           // Character classification
#include <parsedate.h>       // parsedate() function
#include <stdio.h>           // Usual I/O stuff
#include <stdlib.h>          // sprintf(), etc.
#include <string.h>          // string functions
#include <File.h>            // BFile class (for testing)
#include <Debug.h>           // BeOS debugging stuff
#include "Base64ToRawAdapter.h"// Base64 encoding to raw bytes
#include "MimeMessage.h"     // MIME message container
#include <String.h>          // BString class
#include <UTF8.h>            // Charset defines and functions
#include "POP3Parse.h"       // POP3ToMime class


/****************************************************************************/
/*                                                                          */
/***  Misc module data                                                    ***/
/*                                                                          */
/****************************************************************************/

static char valueEmpty[] = "<None>";               // Standard non-header
static char valueUnknown[] = "Unknown";            // Couldn't tell ya


/****************************************************************************/
/*                                                                          */
/***  Charset conversion strings                                          ***/
/*                                                                          */
/****************************************************************************/

static CHARSETINDEX charsetIndex[] =               // Charset index strings
  {
    {csetEUC_KR,            "EUC-KR"},             // EUC-KR
    {csetISO_2022_JP,       "ISO-2022-JP"},        // ISO-2022-JP
    {csetISO_8859_1,        "ISO-8859-1"},         // ISO-8859-1
    {csetISO_8859_2,        "ISO-8859-2"},         // ISO-8859-2
    {csetISO_8859_3,        "ISO-8859-3"},         // ISO-8859-3
    {csetISO_8859_4,        "ISO-8859-4"},         // ISO-8859-4
    {csetISO_8859_5,        "ISO-8859-5"},         // ISO-8859-5
    {csetISO_8859_6,        "ISO-8859-6"},         // ISO-8859-6
    {csetISO_8859_7,        "ISO-8859-7"},         // ISO-8859-7
    {csetISO_8859_8,        "ISO-8859-8"},         // ISO-8859-8
    {csetISO_8859_9,        "ISO-8859-9"},         // ISO-8859-9
    {csetISO_8859_10,       "ISO-8859-10"},        // ISO-8859-10
    {csetISO_8859_13,       "ISO-8859-13"},        // ISO-8859-13
    {csetISO_8859_14,       "ISO-8859-14"},        // ISO-8859-14
    {csetISO_8859_15,       "ISO-8859-15"},        // ISO-8859-15
    {csetKOI8_R,            "KOI8-R"},             // KOI8-R (Russian)
    {csetMacintosh,         "Macintosh"},          // Macintosh
    {csetUS_ASCII,          "US-ASCII"},           // US-ASCII
    {csetUTF8,              "UTF-8"},              // UTF-8
    {csetWindows1251,       "Windows-1251"},       // Windows 1251
    {csetWindows1252,       "Windows-1252"},       // Windows 1252
    
    {csetUnknown,           NULL}                  // End of the list
  };


/****************************************************************************/
/*                                                                          */
/***  Encoding strings                                                    ***/
/*                                                                          */
/****************************************************************************/

static ENCODINGINDEX encodingIndex[] =             // Encoding index strings
  {
    {encBase64,        "Base64"},                  // Base64 binary
    {encBit7,          "7bit"},                    // 7-bit text
    {encBit8,          "8bit"},                    // 8-bit text
    {encQuotePrint,    "quoted-printable"},        // Quoted-printable text
    
    {encUnknown, NULL}                             // End of the list
  };


/****************************************************************************/
/*                                                                          */
/***  Content type strings                                                ***/
/*                                                                          */
/****************************************************************************/

static CONTENTINDEX contentIndex[] =               // Content index strings
  {
    {contMultiAlt,     "multipart/alternative"},   // Several versions
    {contMultiMixed,   "multipart/mixed"},         // All kinds of crap
    {contMultiRelated, "multipart/related"},       // Related parts
    {contMultiOther,   "multipart/other"},         // NOT A REAL TYPE
    {contTextHTML,     "text/html"},               // Stupid HTML
    {contTextPlain,    "text/plain"},              // Good ol' text
    
    {contUnknown, NULL}                            // End of the list
  };


/****************************************************************************/
/*                                                                          */
/***  Delimiter pairs list                                                ***/
/*                                                                          */
/****************************************************************************/

HDRBUF::DELIMITER HDRBUF::delim[] =                // Delimiter list
  {
    {'(', ')'},                                    // Parentheses
    {'<', '>'},                                    // Less than/greater than
    {'"', '"'},                                    // Double quotes
    {'\'', '\''},                                  // Single quotes
    {'[', ']'},                                    // Brackets
    {'{', '}'},                                    // Curly braces
    
    {0, 0}                                         // End of the list
  };


/****************************************************************************/
/*                                                                          */
/***  Header info list                                                    ***/
/*                                                                          */
/****************************************************************************/

HDRBUF::INFO  HDRBUF::headerInfo[] =               // List of header data
  {
    {
      hnumContentEncoding,
      "Content-Transfer-Encoding",
      &HDRBUF::procContentEncoding
    },
    
    {
      hnumContentID,
      "Content-ID",
      &HDRBUF::procContentID
    },
    
    {
      hnumContentType,
      "Content-Type",
      &HDRBUF::procContentType
    },
    
    {
      hnumDate,
      "Date",
      &HDRBUF::procDate
    },
    
    {
      hnumFrom,
      "From",
      &HDRBUF::procFrom
    },
    
    {
      hnumLines,
      "Lines",
      &HDRBUF::procLines
    },
    
    {
      hnumSubject,
      "Subject",
      &HDRBUF::procSubject
    },
    
    {hnumUnknown, NULL, NULL}                      // End of the list
  };
  

/****************************************************************************/
/*                                                                          */
/***  splitFrom test data                                                 ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING

struct TESTFROM
  {
    const char*  value;
    const char*  author;
    const char*  email;
    bool         rval;
  };
  
static TESTFROM testFrom[] =
  {
    {NULL, "", valueEmpty, false},
    {"",   "", valueEmpty, false},
    
    {"someguy@this.net", "", "someguy@this.net", false},
    {"<someguy@this.net>", "", "someguy@this.net", false},
    {"someguy@this.net (Some Guy)", "Some Guy", "someguy@this.net", true},
    {"<berg@dooky.net>BigDooky", "BigDooky", "berg@dooky.net", true},
    {"berg@dikky.net(BigDikky)", "BigDikky", "berg@dikky.net", true},
    {"Rocky", "", "Rocky", false},
    
    {"peterw<remove for no spam>@coastnet.com", "",
     "peterw<remove for no spam>@coastnet.com", false},
  
    {NULL, NULL, NULL, false}
  };
#endif


/****************************************************************************/
/*                                                                          */
/***  main()                                                              ***/
/*                                                                          */
/****************************************************************************

This main() is here only for testing purposes.                              */

#ifdef TEST_SCAFFOLDING                            // If testing
int main(void)                                     // Begin main()
  {
    POP3Parse::test();                             // Test the class
    return 0;                                      // Have to return something
  }                                                // End main()
#endif                                             // End testing
 
 
/****************************************************************************/
/*                                                                          */
/***  HDRBUF::HDRBUF()                                                    ***/
/*                                                                          */
/****************************************************************************

A minimal HDRBUF constructor to ensure that all the members are initialized
to something meaningful.                                                    */

HDRBUF::HDRBUF(void)                               // Begin HDRBUF()
  {
    fBoundaryCount = 0;
    fCharset       = csetDefault;
    fContent       = contDefault;
    fDateTime      = 0;
    fEncoding      = encDefault;
    fLines         = 0;
  }                                                // End HDRBUF()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::~HDRBUF()                                                   ***/
/*                                                                          */
/****************************************************************************

Header buffer destructor.                                                   */

HDRBUF::~HDRBUF(void)                              // Begin ~HDRBUF()
  {
    // Nothing to do
  }                                                // End ~HDRBUF()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::Add()                                                       ***/
/*                                                                          */
/****************************************************************************

Adds another line from the message to the header buffer.  Note that we can't
process it right away because headers can be split across two or more lines.
So we buffer contents in 'buf' until we're sure we've got a whole header
string, then parse it.  RFC822 sez you are supposed to keep whitespace that
starts the second line of a "folded" header so I add second and subsequent
lines to the buffer minus the CR/LF but otherwise unmodified.               */

HDRNUM HDRBUF::Add(const char* str)                // Begin Add()
  {
    HDRNUM  hnum;                                  // Header number
    
    if (lineContin(str))                           // If a continuation
      {
        hnum = hnumUnknown;                        // Don't read yet
        fBuf.Append(str);                          // Add to existing
      }  
    else                                           // It's a header start
      {
        hnum = scan();                             // Do existing contents
        fBuf.SetTo(str);                           // Clear old, set new
      }  
      
    return hnum;                                   // Return outcome 
  }                                                // End Add()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::Boundary()                                                  ***/
/*                                                                          */
/****************************************************************************

Given a pointer to a string from the message this procedure will tell you
if its a MIME boundary string or not.  The sequence is that there will
be a certain form of string that delineates the beginning of a MIME section
which will occur one or more times then another form that means all MIME
sections are finished which will occur only once.  The 'end' bool will tell
you if this particular string is the very end of sections or not, and if
it is, that boundary string will be thrown away, we're done with it.        */

bool HDRBUF::Boundary(const char* str, bool* end)  // Begin Boundary()
  {
    BString*     bstr;                             // Current boundary string
    int32        blen, len;                        // String lengths
    const char*  final;                            // Pointer to final chars
    const char*  ptr;                              // The guts
    
    if (!str) return false;                        // If no string there
    
    if (str[0] != '-' || str[1] != '-' || !str[2]) // If no "--" at start
      return false;                                // Get outta here
      
    bstr = boundaryCurrent();                      // Get current boundary
    if (!bstr) return false;                       // If none defined
    
    ptr  = bstr->String();                         // Get the guts
    blen = bstr->Length();                         // Get boundary length
    len  = strlen(str);                            // Get line length
    
    if (!(len == (blen + 2) || len == (blen + 4))) // If not the right length
      return false;                                // Get outta here
    
    if (memcmp(ptr, &str[2], blen)) return false;  // Must be the same
    
    final = str + blen + 2;                        // Point past the match
    
    if (*final == 0)                               // If that's the end
      {
        *end = false;                              // It's a start boundary
        return true;                               // That is all
      }
    else if (*final == '-' &&                      // If "--" on the end
     *(final + 1) == '-' && *(final + 2) == 0)
      {
        *end = true;                               // That's the end boundary
        boundaryDel();                             // Remove it
        return true;                               // That is all
      }      
      
    return false;                                  // Nope, didn't match
  }                                                // End Boundary()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::boundaryAdd()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure adds a MIME boundary string to our master list.  When we
first start there should be no boundaries defined, then if the message is
MIME multi-part we add one from the master headers, then if we hit an
embedded MIME multi-part that would add a second one so now we're nested
one level, and so on.                                                       */

void HDRBUF::boundaryAdd(const char* str)          // Begin boundaryAdd()
  {
    uint32       i;                                // Loop counter
    const char*  ptr;
    
    if (!str || !str[0]) return;                   // If no string given
    
    if (str[0] != '-' || str[1] != '-' || !str[2]) // Must start with --XXX
      {
        ASSERT(false);                             // Bad boundary string!
        return;                                    // Get outta here
      }
    
    for (i = 0;  i < boundaryTotal; i++)           // Loop the list
      {
        ptr = fBoundary[i].String();               // Get contents
        if (ptr && ptr[0]) continue;               // If it's in use
        
        fBoundary[i].SetTo(str);                   // In you go
        return;                                    // Stop looping
      }
      
    ASSERT(false);                                 // Too many boundaries!    
  }                                                // End boundaryAdd()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::boundaryCount()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure returns the total number of boundary strings we've got
right now.                                                                  */

int32 HDRBUF::boundaryCount(void)                  // Begin boundaryCount()
  {
    int32        i;                                // Loop counter
    const char*  ptr;                              // Pointer to guts
    int32        total = 0;                        // Total boundaries

    for (i = 0; i < boundaryTotal; i++)            // Loop the list
      {
        ptr = fBoundary[i].String();               // Get contents
        if (ptr && ptr[0]) total++;                // If it's in use
      }
      
    return total;                                  // That's how many
  }                                                // End boundaryCount()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::boundaryCurrent()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure returns a pointer to the current boundary string to use.
It will always be the last one in the list, due to nesting.                 */

BString* HDRBUF::boundaryCurrent(void)             // Begin boundaryCurrent()
  {
    int32        i;                                // Loop counter
    const char*  ptr;                              // Pointer to guts

    for (i = boundaryTotal - 1;  i >= 0; i--)      // Backwards through list
      {
        ptr = fBoundary[i].String();               // Get contents
        if (ptr && ptr[0]) return &fBoundary[i];   // If it's in use
      }
      
    return NULL;                                   // None are defined    
  }                                                // End boundaryCurrent()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::boundaryDel()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure removes the last (i.e., most recent) MIME boundary string
from the list.  We can do this when the boundary is found that signals the
end of this MIME multi-part message, thereby exiting one level of nesting.  */

void HDRBUF::boundaryDel(void)                     // Begin boundaryDel()
  {
    int32        i;                                // Loop counter
    const char*  ptr;
    
    for (i = boundaryTotal - 1;  i >= 0; i--)      // Backwards through list
      {
        ptr = fBoundary[i].String();               // Get contents
        if (!ptr || !ptr[0]) continue;             // If it's not there
        
        fBoundary[i].SetTo(NULL);                  // It's gone now
        break;                                     // Stop looping
      }
  }                                                // End boundaryDel()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charDecode()                                                ***/
/*                                                                          */
/****************************************************************************

This procedure checks to see if a string pointer is at the beginning of an
encoded character in the form "=XX", where the Xs are hex digits.  If the
string is an encoded char it will be returned, if not it will return 0.     */

char HDRBUF::charDecode(const char* str)           // Begin charDecode()
  {
    char    buf[4];                                // Output string
    char    ch;                                    // Char from string
    uint32  conv;                                  // Converted value
    uint32  i;                                     // Loop counter
    
    if (!str || str[0] != '=') return 0;           // Must start like this
    
    for (i = 0; i < 2; i++)                        // Loop for next two chars
      {
        ch = str[i + 1];                           // Get this char
        if (!charIsHexDigit(ch)) return 0;         // Exit if not legal
        buf[i] = ch;                               // Save the character
      }
     
    buf[2] = 0;                                    // Terminate string
            
    sscanf(buf, "%lX", &conv);                     // Convert character
    if (conv == 0) conv = SPACE;                   // Don't allow NULs
    
    return (char) conv;                            // There you go
  }                                                // End charDecode()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charIsHexDigit()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure checks a character to make sure that it is a hex digit which
is what RFC2045 et al say they must be.  It also says that they must be
uppercase but I'm being lenient and allowing lowercase also.                */

bool HDRBUF::charIsHexDigit(char ch)               // Begin charIsHexDigit()
  {
    if (ch >= '0' && ch <= '9') return true;       // If 0-9
    if (ch >= 'A' && ch <= 'F') return true;       // If A-F
    if (ch >= 'a' && ch <= 'f') return true;       // If a-f
    
    return false;                                  // Illegal character
  }                                                // End charIsHexDigit()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charsetConvert()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure takes a NULL-terminated buffer full of bytes in one charset
and converts it to a NULL-terminated buffer of bytes in a different charset.
The rules are that either the source or dest charset must be UTF8 or
else both source and dest must be the same type (which results in a simple
copy operation).                                                            */

bool HDRBUF::charsetConvert(CHARSET srcCharset,    // Begin charsetConvert()
 const char* src, CHARSET dstCharset, BString* dst,
 int32* state)
  {
    int32     bytes   = 0;                         // Total bytes written
    int32     dstSize = 0;                         // Calculated dest size
    char*     ptr     = NULL;                      // Buffer to write to
    bool      seven   = false;                     // Force 7-bit?
    int32     srcSize = strlen(src);               // Source size
    int32     st      = 0;                         // State stand-in
    status_t  status  = B_ERROR;                   // Return value
    
    
    //*
    //***  Various machinations to ensure a good transfer
    //*
    
    ASSERT(charsetIsValid(srcCharset));            // This one should be good
    ASSERT(charsetIsValid(dstCharset));            // This one too
    
    dst->SetTo(NULL);                              // Clear existing contents
    if (!state) state = &st;                       // Use stand-in if needed
    
    if (srcSize == 0) goto end;                    // If nothing to convert
    
    if (srcCharset == csetUS_ASCII)                // If made-up src set
      srcCharset = csetISO_8859_1;                 // Convert like this
      
    if (dstCharset == csetUS_ASCII)                // If made-up dst set
      {
        dstCharset = csetISO_8859_1;               // Set dest set
        seven = true;                              // Force to 7-bit
      }  
    
    
    //*
    //***  Do the conversion
    //*
    
    dstSize = (srcSize * 3) + 10;                  // This many bytes to get
    bytes   = dstSize;                             // Set max write size
    ptr     = dst->LockBuffer(dstSize);            // Reserve my byte count
    
    if (!ptr)                                      // If it failed
      {
        dst->UnlockBuffer();                       // Fergit it
        goto end;                                  // Bail
      }
      
    ptr[0] = 0;                                    // Terminate output  
    
    if (srcCharset == dstCharset)                  // Simple copy
      {
        strncpy(ptr, src, dstSize);                // Copy it over
        bytes = srcSize;                           // Set bytes copied
        status = B_OK;                             // Success
      }
    else if (srcCharset == csetUTF8)               // From UTF8
      {
        status = convert_from_utf8(dstCharset,     // Do it
         src, &srcSize, ptr, &bytes, state);
      }
    else if (dstCharset == csetUTF8)               // To UTF8
      {
        status = convert_to_utf8(srcCharset,       // Do it
         src, &srcSize, ptr, &bytes, state);
      }
    else                                           // Illegal
      {
        ASSERT(false);                             // Whoops
        status = B_ERROR;                          // Failed
      }
    
    if (bytes < 0) bytes = 0;                      // Not negative, please
    
    if (bytes >= dstSize)                          // If too many
      {
        ASSERT(false);                             // Buffer overflow
        bytes = (dstSize - 1);                     // Back to size
      }  
    
    ptr[bytes] = 0;                                // MUST BE TERMINATED
    dst->UnlockBuffer();                           // Back to normal
      
    end:                                           // Jump here to exit
    if (seven) charsetUSASCII(dst);                // Convert to 7-bit
    return (status == B_OK);                       // Return outcome
  }                                                // End charsetConvert()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charsetIsValid()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure takes a CHARSET enum as input and returns true if it is
legal, false if not.                                                        */

bool HDRBUF::charsetIsValid(CHARSET charset)       // Begin charsetIsValid()
  {
    uint32   i;                                    // Loop counter
    
    for (i = 0; charsetIndex[i].str; i++)          // Loop the list
      if (charset == charsetIndex[i].charset)      // If it's a match
        return true;                               // You can keep it
        
    return false;                                  // It wasn't found
  }                                                // End charsetIsValid()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charsetMakeValid()                                          ***/
/*                                                                          */
/****************************************************************************

This procedure takes in a suspect CHARSET value and returns it unchanged if
it's okay or the default charset otherwise.                                 */

CHARSET HDRBUF::charsetMakeValid(CHARSET charset)  // Begin charsetMakeValid()
  {
    if (charsetIsValid(charset))
      return charset;
    else
      return csetDefault;  
  }                                                // End charsetMakeValid()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charsetString()                                             ***/
/*                                                                          */
/****************************************************************************

Given a CHARSET enum this procedure will translate it to a string that
represents that type.  If the CHARSET value is unrecognized it will return
"Unknown".                                                                  */

const char* HDRBUF::charsetString(CHARSET charset) // Begin charsetString()
  {
    uint32  i;                                     // Loop counter
    
    for (i = 0; charsetIndex[i].str; i++)          // Loop for all entries
      if (charsetIndex[i].charset == charset)      // If match is found
        return charsetIndex[i].str;                // There it is
    
    return valueUnknown;                           // It wasn't found
  }                                                // End charsetString()


/****************************************************************************/
/*                                                                          */
/***  IMSG::charsetUSASCII()                                              ***/
/*                                                                          */
/****************************************************************************

If the message specified US-ASCII then that's what you're going get,
goddammit.  This procedure forces the contents of a BString to be 7-bit,
removing those newfangled "high bits" that all the teenyboppers dig these
days, when they're not "hopped up" on "dope" or god knows what.             */

void HDRBUF::charsetUSASCII(BString* str)          // Begin charsetUSASCII()
  {
    char*  buf = str->LockBuffer(0);               // The bytes
    uchar  ch;                                     // Character from string
    char*  ptr;                                    // String pointer
    
    if (!buf || !buf[0]) goto end;                 // Exit if no contents
    
    for (ptr = buf; *ptr; ptr++)                   // Loop through the string
      {
        ch = *ptr;                                 // Get this character
        
        if (ch >= 0x80)                            // If 8-bit char
          {
            ch -= 0x80;                            // Back to 7-bit
            if (ch < SPACE && ch != TAB) ch=SPACE; // Don't create ctrl chars
            *ptr = ch;                             // Put it back
          }  
      }
      
    end:                                           // Jump here to exit
    str->UnlockBuffer();                           // Finished    
  }                                                // End charsetUSASCII()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::charsetValue()                                              ***/
/*                                                                          */
/****************************************************************************

Given a string that contains a character set type this procedure will
convert it to a CHARSET enum value.  If the string isn't a legal charset
it will return csetUnknown.                                                 */

CHARSET HDRBUF::charsetValue(const char* str)      // Begin charsetValue()
  {
    uint32  i;                                     // Loop counter
    
    if (!str || !str[0]) return csetUnknown;       // If string is empty
    
    for (i = 0; charsetIndex[i].str; i++)          // Loop for all entries
      if (!strcasecmp(str, charsetIndex[i].str))   // If match is found
        return charsetIndex[i].charset;            // There it is
    
    return csetUnknown;                            // It wasn't found
  }                                                // End charsetValue()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::ClearNextSection()                                          ***/
/*                                                                          */
/****************************************************************************

A HDRBUF may be used to read the master headers, then the ones in the first
MIME section, then the second, and so on.  Most of the accumulated data
should be thrown away, so this procedure clears it, while saving stuff
we would want to keep (like the character set).                             */

void HDRBUF::ClearNextSection(void)                // Begin ClearNextSection()
  {
    fBoundaryCount = boundaryCount();              // Update boundaries
    fCharset       = charsetMakeValid(fCharset);   // Make valid for next run
    fContent       = contDefault;
    fEncoding      = encDefault;
    
    fBuf.SetTo(NULL);
    fFilename.SetTo(NULL);
    fFrom.SetTo(NULL);
    fSubject.SetTo(NULL);
  }                                                // End ClearNextSection()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::contentString()                                             ***/
/*                                                                          */
/****************************************************************************

Given a CONTENT enum this procedure will return a string that describes
it.  If it's unknown it will return "Unknown".                              */

const char* HDRBUF::contentString(CONTENT content) // Begin contentString()
  {
    uint32  i;                                     // Loop counter
    
    for (i = 0; contentIndex[i].str; i++)          // Loop for all entries
      if (contentIndex[i].content == content)      // If match is found
        return contentIndex[i].str;                // There it is
        
    return valueUnknown;                           // Sorry bub
  }                                                // End contentString()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::contentValue()                                              ***/
/*                                                                          */
/****************************************************************************

Given a string that contains a MIME content type this procedure will
convert it to a CONTENT enum value.  If the type is unknown the return
value will be contUknown.                                                   */

CONTENT HDRBUF::contentValue(const char* str)      // Begin contentValue()
  {
    char    buf[40];                               // Temp buffer
    uint32  i;                                     // Loop counter
    uint32  len;                                   // Length
    
    if (!str || !str[0]) return contUnknown;       // Empty string
    
    for (i = 0; contentIndex[i].str; i++)          // Loop for all entries
      if (!strcasecmp(str, contentIndex[i].str))   // If match is found
        return contentIndex[i].content;            // There it is
    
    len = strlen(str);                             // Get length
    if (len < 11) return contUnknown;              // If too short
    
    strncpy(buf, str, sizeof (buf));               // Copy it
    buf[10] = 0;                                   // Terminate it
    
    if (!strcasecmp(buf, "multipart/"))            // Other multi-part type
      return contMultiOther;                       // Return this
    
    return contUnknown;                            // It wasn't found
  }                                                // End contentValue()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimFindLeft()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure searches backwards through a string to find the left
delimiter to match the one given at 'end'.  If not found it will return
NULL.                                                                       */

char* HDRBUF::delimFindLeft(const char* str,       // Begin delimFindLeft()
 const char* end)
  {
    uint32       level = 0;                        // Nesting level
    char         left, right;                      // Delimiters
    const char*  ptr;                              // Search pointer
    
    if (!str || !end) return NULL;                 // If no strings
    ASSERT(delimRight(*end));                      // Must be at a delimiter
    ASSERT(str < end);                             // Verify relative position
    
    left  = delimMatchRight(*end);                 // Get left delimiter
    right = *end;                                  // Get right delimiter
    if (!left || !right) return NULL;              // If either not found
    
    for (ptr = end - 1; ptr >= str; ptr--)         // Loop to find a match
      {
        if (ptr > str && *(ptr - 1) == '\\')       // Char is quoted
          continue;                                // Ignore it
        
        if (*ptr == left)                          // Left match
          {
            if (level >= 1) {level--; continue;}   // If quoted
            break;                                 // Found it!
          }  
        else if (*ptr == right)                    // Right match
          {
            level++;                               // Just went up a level
          }
      }    
    
    if (*ptr != left) ptr = NULL;                  // If not found
    return (char*) ptr;                            // There's yer answer
  }                                                // End delimFindLeft()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimFindRight()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure searches forward through a string to find the right
delimiter to match the left one at the start.  If not found it will return
NULL.                                                                       */

char* HDRBUF::delimFindRight(const char* str)      // Begin delimFindRight()
  {
    uint32       level = 0;                        // Nesting level
    char         left, right;                      // Delimiters
    const char*  ptr;                              // Search pointer
    
    if (!str) return NULL;                         // If no string
    ASSERT(delimLeft(*str));                       // Must be at a delimiter
    
    left  = *str;                                  // Get left delimiter
    right = delimMatchLeft(left);                  // Get right delimiter
    if (!left || !right) return NULL;              // If either not found
    
    for (ptr = str + 1; *ptr; ptr++)               // Loop to find a match
      if (*ptr == right && *(ptr-1) != '\\')       // Right match
        {
          if (level >= 1) {level--; continue;}     // If nested
          break;                                   // Found it!
        }  
      else if (*ptr == left && *(ptr-1) != '\\')   // Left match
        {
          level++;                                 // Just went up a level
        }
    
    if (*ptr != right) ptr = NULL;                 // If not found
    return (char*) ptr;                            // There's yer answer
  }                                                // End delimFindRight()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimLeft()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure checks a left delimiter to see if it is in the list.         */

bool HDRBUF::delimLeft(char ch)                    // Begin delimLeft()
  {
    uint32  i;                                     // Loop counter
    
    if (!ch) return false;                         // If it's NUL
    
    for (i = 0; delim[i].left; i++)                // Loop the list
      if (ch == delim[i].left) return true;        // If it's a match
      
    return false;                                  // Not found
  }                                                // End delimLeft()


/****************************************************************************/
/*                                                                          */
/***  IMSG::delimMatchLeft()                                              ***/
/*                                                                          */
/****************************************************************************

This procedure takes a left delimiter as input and returns the right
delimiter that matches it.  If no match is found it will return NUL.        */

char HDRBUF::delimMatchLeft(char left)             // Begin delimMatchLeft()
  {
    uint32  i;                                     // Loop counter
    
    if (!left) return 0;                           // If it's NUL
    
    for (i = 0; delim[i].left; i++)                // Loop the list
      if (left == delim[i].left)                   // If it's a match
        return delim[i].right;                     // Return its mate
      
    return 0;                                      // Not found
  }                                                // End delimMatchLeft()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimMatchRight()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure takes a right delimiter as input and returns the left
delimiter that matches it.  If no match is found it will return NUL.        */

char HDRBUF::delimMatchRight(char right)           // Begin delimMatchRight()
  {
    uint32  i;                                     // Loop counter
    
    if (!right) return 0;                          // If it's NUL
    
    for (i = 0; delim[i].left; i++)                // Loop the list
      if (right == delim[i].right)                 // If it's a match
        return delim[i].left;                      // Return its mate
      
    return 0;                                      // Not found
  }                                                // End delimMatchRight()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimPair()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure checks a pair of characters to see if they are in the
delimiter list.                                                             */

bool HDRBUF::delimPair(char left, char right)      // Begin delimPair()
  {
    uint32  i;                                     // Loop counter
    
    if (!left || !right) return false;             // If one or both NUL
    
    for (i = 0; delim[i].left; i++)                // Loop the list
      if (left == delim[i].left &&                 // If left matches
       right == delim[i].right)                    // And right matches
        return true;                               // That's a match
      
    return false;                                  // Not found
  }                                                // End delimPair()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimRemove() -- char buffer version                        ***/
/*                                                                          */
/****************************************************************************

This procedure checks a string to see if it begins and ends with a regular
delimiter pair and if so, removes them.                                     */

bool HDRBUF::delimRemove(char* str)                // Begin delimRemove()
  {
    char   left, right;                            // Delimiters
    int32  len;
    
    if (!str || !str[0]) return false;             // If no string
    strTrim(str);                                  // Remove whitespace
    
    len = strlen(str);                             // Get length
    if (len < 2) return false;                     // If not long enough
    
    left  = str[0];                                // Get left char
    right = str[len - 1];                          // Get right char
    
    if (delimPair(left, right))                    // If delimited
      {
        str[0]       = SPACE;                      // Left delim to space
        str[len - 1] = SPACE;                      // Right delim to space
        
        strTrim(str);                              // Remove spaces
        return true;                               // Found and removed
      }
    else                                           // No delimiters
      {  
        return false;                              // Didn't do anything
      }  
  }                                                // End delimRemove()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimRemove() -- BString version                            ***/
/*                                                                          */
/****************************************************************************

Does the delimRemove operation on a BString.                                */

bool HDRBUF::delimRemove(BString* str)             // Begin delimRemove()
  {
    char*  ptr;                                    // Pointer to guts
    bool   rval;                                   // Outcome
    
    ptr = str->LockBuffer(0);                      // Lock down the bytes
    rval = delimRemove(ptr);                       // Call the other version
    str->UnlockBuffer();                           // Finished
    
    return rval;                                   // Return outcome
  }                                                // End delimRemove()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::delimRight()                                                ***/
/*                                                                          */
/****************************************************************************

This procedure checks a right delimiter to see if it is in the list.        */

bool HDRBUF::delimRight(char ch)                   // Begin delimRight()
  {
    uint32  i;                                     // Loop counter
    
    if (!ch) return false;                         // If it's NUL
    
    for (i = 0; delim[i].right; i++)               // Loop the list
      if (ch == delim[i].right) return true;       // If it's a match
      
    return false;                                  // Not found
  }                                                // End delimRight()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::encodingString()                                            ***/
/*                                                                          */
/****************************************************************************

Converts an ENCODING enum into the associated string.                       */

const char* HDRBUF::encodingString(ENCODING        // Begin encodingString()
 encoding)
  {
    uint32  i;                                     // Loop counter
    
    for (i = 0; encodingIndex[i].str; i++)         // Loop for all entries
      if (encodingIndex[i].encoding == encoding)   // If match is found
        return encodingIndex[i].str;               // Found it
  
    return valueUnknown;                           // Didn't find it
  }                                                // End encodingString()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::encodingValue()                                             ***/
/*                                                                          */
/****************************************************************************

Converts the value part of a Content-Transfer-Encoding header into an
ENCODING enum.  It will return encUnknown if it doesn't match.              */

ENCODING HDRBUF::encodingValue(const char* str)    // Begin encodingValue()
  {
    uint32  i;                                     // Loop counter
    
    if (!str || !str[0]) return encUnknown;        // Empty string
    
    for (i = 0; encodingIndex[i].str; i++)         // Loop for all entries
      if (!strcasecmp(str, encodingIndex[i].str))  // If match is found
        return encodingIndex[i].encoding;          // Found it
  
    return encUnknown;                             // Didn't find it
  }                                                // End encodingValue()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::End()                                                       ***/
/*                                                                          */
/****************************************************************************

Once all header lines have been read this procedure is called to process
any contents that may be buffered but not yet parsed.  Also, since the
headers are now over, this is a good time to do any character set
conversions needed.  We can't do it while reading the headers because we
might not have the right character set yet.  Finally, it returns true
if this set of headers contained a boundary line, false if not.             */

bool HDRBUF::End(void)                             // Begin End()
  {
    BString      buf;                              // Temporary buffer
    CHARSET      charset;                          // Final for conversion
    int32        count;                            // Boundary count
    const char*  ptr = fBuf.String();              // Get contents
    bool         rval;                             // Value to return
    
    if (ptr && ptr[0])                             // If anything in there
      {
        scan();                                    // Parse it
        fBuf.SetTo(NULL);                          // Clear it
      }
      
    charset = charsetMakeValid(fCharset);          // Charset for conversion
    
    hdrDecode(&buf, fSubject.String(), charset);   // Subject conversion
    fSubject.SetTo(buf);                           // Copy it back
    
    hdrDecode(&buf, fFrom.String(), charset);      // From conversion
    fFrom.SetTo(buf);                              // Copy it back
    
    count = boundaryCount();                       // Get current total
    rval  = (count > fBoundaryCount);              // Got bigger?
    fBoundaryCount = count;                        // Update it
    
    return rval;                                   // There's yer answer
  }                                                // End End()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::hdrDecode()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure is expecting to get a From, Subject, or other header that
might have non-ASCII stuff in it and converts it to UTF8.  The CHARSET
given should be the one found in the message headers, from the current MIME
section header, or a suitable default.

The proper way to encode non-ASCII characters in message headers is as
RFC2047 encoded words and if any of those are found this procedure will
convert them.  Interesting side note: RFC2047 encoded words contain their
own charset which can be different than the one used for the rest of the
message and it will be used to do the conversion.  Note that malformed
encoded words could lead to an empty dest string so I check for that and
set something innocuous for the user to see.

Real-world experience has shown that MS Outlook Express and other popular
mail clients can and do embed non-ASCII charset data in the character set
specified in the headers, so that kind of stuff is grudgingly decoded here
also.  All the more reason to ensure the CHARSET given is correct.          */

void HDRBUF::hdrDecode(BString* dst,               // Begin hdrDecode()
 const char* src, CHARSET charset)
  {
    BString      buf;                              // Intermediate buffer
    uint32       bufIdx;                           // Buffer index
    uint32       bufLen;                           // Buffer length
    const char*  bufPtr;                           // Intermediate guts
    ENCODE       encode;                           // Encoded word data
    
    ASSERT(dst && src);                            // Need these buffers
    ASSERT(charsetIsValid(charset));               // Charset must be good
    dst->SetTo(NULL);                              // Clear existing contents
    
    charsetConvert(charset, src,                   // Convert source to UTF8
     csetUTF8, &buf);                              //  output in buf
     
    bufLen = buf.Length();                         // Get buffer length
    bufPtr = buf.String();                         // Pointer to guts
    
    for (bufIdx = 0; bufIdx < bufLen; )            // Loop for RFC2047 decode
      if (hdrDecodeFind(&bufPtr[bufIdx], &encode)) // If encoded word
        {
          if (encode.type == 'B')                  // B encoding
            hdrDecodeWordB(dst, &encode);          // Use this
          else                                     // Q encoding
            hdrDecodeWordQ(dst, &encode);          // Use this
          
          bufIdx += encode.total;                  // Go past encoded word
        }
      else                                         // Not encoded word
        {
          dst->Append(bufPtr[bufIdx], 1);          // Copy the character
          bufIdx++;                                // Now go past it
        }
      
    if (dst->Length() <= 0)                        // If nothing left
      dst->SetTo(valueEmpty);                      // Set this stand-in
  }                                                // End hdrDecode()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::hdrDecodeFind()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure checks to see if a string pointer is at the start of one of
those dreadful RFC2047 encoded words.  If it is it will record a bunch of
useful information about the word in the ENCODE structure given.            */

bool HDRBUF::hdrDecodeFind(const char* str,        // Begin hdrDecodeFind()
 ENCODE* encode)
  {
    char         buf[130];                         // Temp buffer
    char         ch;                               // Test char
    const char*  end;                              // End sequence
    uint32       i;                                // Loop counter
    const char*  ptr;                              // Moves through buffer
    
    if (str[0]!='=' || str[1]!='?') return false;  // Check word start
    ptr = &str[2];                                 // Start here
    
    for (i = 0; *ptr && *ptr != '?'; i++, ptr++)   // Loop to get charset
      {
        if (i >= (sizeof (buf)) - 2) break;        // Don't overflow buffer
        buf[i] = *ptr;                             // Copy one char
      }  
      
    buf[i] = 0;                                    // Terminate string
    if (*ptr != '?') return false;                 // Should be pointing here
    ptr++;                                         // Past question mark
    
    encode->charset = charsetValue(buf);           // Get charset
    
    if (!charsetIsValid(encode->charset))          // Illegal or unknown
      return false;                                // Fergit it
    
    encode->type = toupper(*ptr);                  // Get encoding char
    ch = encode->type;                             // Copy it
    if (ch != 'Q' && ch != 'B') return false;      // Only do these types
    ptr++;                                         // Go past it
    
    if (*ptr != '?') return false;                 // Check next divider
    ptr++;                                         // Go past it
    
    encode->ptr = (char*) ptr;                     // Set word start
    if (*encode->ptr == '?') return false;         // First meat character
    if (*encode->ptr ==  0 ) return false;         // Can't be empty
    
    end = strstr(encode->ptr, "?=");               // Find end sequence
    if (!end) return false;                        // If not found
    
    encode->len = end - encode->ptr;               // Set word length
    if (encode->len <   1) return false;           // If too short
    if (encode->len > 200) return false;           // If way big
    
    encode->total = end - str + 2;                 // Set total length
    if (encode->total <  11) return false;         // If too small
    if (encode->total > 200) return false;         // If way big
    
    return true;                                   // Passed all tests
  }                                                // End hdrDecodeFind()


/****************************************************************************/
/*                                                                          */
/***  IMSG::hdrDecodeWordB()                                              ***/
/*                                                                          */
/****************************************************************************

This procedure decodes an RFC2047-encoded word that has the B encoding.     */

void HDRBUF::hdrDecodeWordB(BString* dst,          // Begin hdrDecodeWordB()
 const ENCODE* encode)
  {
    BString  out;                                  // Decoded output
    int32    size;                                 // Size calculation
    BString  str;                                  // Intermediate string
    
    ASSERT(encode->ptr && encode->ptr[0]);         // Must have this stuff
    size = encode->len + 1;                        // Set length plus NULL
    
    Base64ToRawAdapter::Decode(encode->ptr,        // Base64 to string
     size, &str);
    
    charsetConvert(encode->charset, str.String(),  // Convert to UTF8
     csetUTF8, &out);
     
    dst->Append(out);                              // Add to dest buffer
  }                                                // End hdrDecodeWordB()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::hdrDecodeWordQ()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure decodes an RFC2047-encoded word that has the Q encoding.     */

void HDRBUF::hdrDecodeWordQ(BString* dst,          // Begin hdrDecodeWordQ()
 const ENCODE* encode)
  {
    char         buf[250];                         // Output buffer
    int32        bufIdx;                           // Buffer index
    char         ch;                               // Next char from input
    BString      out;                              // Decoded output
    const char*  ptr = encode->ptr;                // Iteration pointer
    int32        ptrIdx;                           // Pointer index
    
    for (bufIdx=0,ptrIdx=0; ptrIdx < encode->len; )// Loop to output chars
      {
        if (bufIdx >= (int32)(sizeof buf)-2) break;// Don't overflow buffer
        ch = charDecode(&ptr[ptrIdx]);             // Start of encoded char?
        
        if (ch)                                    // If encoded char found
          {
            buf[bufIdx++] = ch;                    // Add to output
            ptrIdx += 3;                           // Go past string bytes
          }
        else                                       // Not encoded char
          {
            ch = ptr[ptrIdx];                      // Get next char
            if (ch == '_') ch = SPACE;             // Underscores to spaces
            
            buf[bufIdx++] = ch;                    // Put char in buffer
            ptrIdx++;                              // Did one char
          }  
      }                                            // End of char loop
      
    buf[bufIdx] = 0;                               // Terminate buffer
    
    charsetConvert(encode->charset, buf,           // Convert to UTF8
     csetUTF8, &out);
     
    dst->Append(out);                              // Add to the end
  }                                                // End hdrDecodeWordQ()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::lineContin()                                                ***/
/*                                                                          */
/****************************************************************************

This procedure returns true if a line is a "continuation" header line, i.e.,
it starts with whitespace and therefore continues a header started on an
earlier line.                                                               */

bool HDRBUF::lineContin(const char* line)          // Begin lineContin()
  {
    if (!line || !line[0]) return false;           // Totally empty?
    
    if (line[0] == SPACE) return true;             // Starts with space?
    if (line[0] == TAB)   return true;             // Starts with tab?
    
    return false;                                  // Didn't meet criteria
  }                                                // End lineContin()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procContentEncoding()                                       ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Content-Transfer-Encoding header.                    */

bool HDRBUF::procContentEncoding(const char* value)// procContentEncoding()
  {
    ENCODING  encoding;                            // Encoding from header
    
    encoding = encodingValue(value);               // Convert it
    
    if (encoding != encUnknown)                    // If it's good
      {
        fEncoding = encoding;                      // Save it
        return true;                               // Success
      }
    else                                           // No good
      {
        return false;                              // Failed
      }  
  }                                                // procContentEncoding()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procContentID()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Content-ID header.                                   */

bool HDRBUF::procContentID(const char* value)      // procContentID()
  {
    fContentID.SetTo(value);                       // Save it for later
    return true;                                   // Success
  }                                                // procContentID()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procContentType()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Content-Type header.                                 */

bool HDRBUF::procContentType(const char* value)    // Begin procContentType()
  {
    CHARSET  charset;                              // Charset from header
    CONTENT  content;                              // Content from header
    BString  str;                                  // Temp string
    
    subvalGet(value, &str);                        // Get type string
    content = contentValue(str.String());          // Convert it
    if (content != contUnknown) fContent = content;// Save it, if it's good
    
    subval(value, "charset", &str);                // Get character set
    charset = charsetValue(str.String());          // Convert it
    if (charset != csetUnknown) fCharset = charset;// Save it, if it's good
    
    subval(value, "boundary", &str);               // Fish out boundary
    boundaryAdd(str.String());                     // Stick it in
    
    subval(value, "name", &fFilename);             // Fish out filename
    return (content != contUnknown);               // Return outcome
  }                                                // End procContentType()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procDate()                                                  ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Date header.                                         */

bool HDRBUF::procDate(const char* value)           // Begin procDate()
  {
    fDate.SetTo(value);                            // Save the whole string
    
    fDateTime = parsedate(value, -1);              // Relative to now
    if (fDateTime > 0) return true;                // If it was understood
    
    fDateTime = real_time_clock_usecs() / 1000000; // Punt
    return false;                                  // Failed
  }                                                // End procDate()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procFrom()                                                  ***/
/*                                                                          */
/****************************************************************************

This procedure reads a From header.  We can't do the decoding yet since
this header might come before the one that contains the character set so
just copy it and we'll worry about it later.                                */

bool HDRBUF::procFrom(const char* value)           // Begin procFrom()
  {
    fFrom.SetTo(value);                            // Save it
    return true;                                   // Success
  }                                                // End procFrom()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procLines()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Lines header.                                        */

bool HDRBUF::procLines(const char* value)          // Begin procLines()
  {
    fLines = atoi(value);                          // Convert it
    if (fLines < 1) fLines = 1;                    // If it's ridiculous
    
    return true;                                   // Success
  }                                                // End procLines()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::procSubject()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Subject header.  Can't decode it since we might not
have the character set yet so just save it for later.                       */

bool HDRBUF::procSubject(const char* value)        // Begin procSubject()
  {
    fSubject.SetTo(value);                         // Save it
    return true;                                   // Success
  }                                                // End procSubject()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::scan()                                                      ***/
/*                                                                          */
/****************************************************************************

Okay.  At this point the work buffer contains the contents of exactly one
header.  It may have been spread across two or more lines but they have been
stitched together elsewhere, so it's time to process it.  It will return a
HDRNUM enum that specifies which type of header was processed or
hnumUnknown if the line was a header we don't care about or was invalid.    */

HDRNUM HDRBUF::scan(void)                          // Begin scan()
  {
    HDRNUM       hnum = hnumUnknown;               // Header type processed
    uint32       i;                                // Loop counter
    BString      name, value;                      // Split-up header parts
    const char*  ptr;                              // Guts of 'buf'
    bool         rval;                             // Proc outcome
    
    ptr = fBuf.String();                           // Get string pointer
    if (!ptr || !ptr[0]) return hnumUnknown;       // If nothing in there
    
    if (!split(ptr, &name, &value))                // Split it up
      return hnumUnknown;                          // If it was invalid
      
    ptr = name.String();                           // Get name pointer
    
    for (i = 0; headerInfo[i].name; i++)           // Loop header list
      if (!strcasecmp(ptr, headerInfo[i].name))    // If it matches this one
        {
          ptr  = value.String();                   // Get header contents
          hnum = headerInfo[i].hnum;               // Save header number
          rval = (this->*headerInfo[i].proc)(ptr); // Call the proc
          break;                                   // Stop the loop
        }  
    
    return hnum;                                   // Whoomp!  There it is
  }                                                // End scan()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::split()                                                     ***/
/*                                                                          */
/****************************************************************************

This procedure examines a header string in "Name: Value" format and splits
it into component parts.  It returns true if the string was parsed
successfully, false if the line wasn't a valid header string.               */

bool HDRBUF::split(const char* str, BString* name, // Begin split()
 BString* value)
  {
    char*  colon;                                  // First colon in string
    int32  len;                                    // String length
    
    name->SetTo(NULL);                             // Clear name
    value->SetTo(NULL);                            // Clear value
    
    if (!str || !str[0]) return false;             // If no string given
    
    colon = strchr(str, ':');                      // Find first colon
    if (!colon) return false;                      // If none in there
    
    len = (colon - str);                           // Get name length
    if (len < 2) return false;                     // Exit if no length
    
    name->Append(str, len);                        // Add up to colon
    strTrim(name);                                 // Remove whitespace
    if (name->Length() <= 0) return false;         // If nothing left
    
    value->Append(colon + 1);                      // Get stuff after colon
    strTrim(value);                                // Remove whitespace
    if (value->Length() <= 0) return false;        // If nothing left
    
    return true;                                   // Success
  }                                                // End split()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::splitFrom()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure takes the value part of a From: header and splits it into
the e-mail address and author name.  From: headers can have RFC2047 garbage
and non-ASCII characters in them; this procedure assumes the text has
already been converted to UTF8 elsewhere.  Apparently when IMAP parses a
From field it gives you nothing if the author name isn't present so that's
what I do here.                                                             */

bool HDRBUF::splitFrom(const char* value,          // Begin splitFrom()
 BString* author, BString* email)
  { 
    const char*  buf;                              // Work buffer guts
    BString      bufStr;                           // Work buffer
    BString      left, right;                      // Left and right halves
    int32        len, size;                        // String lengths
    char*        ptr;                              // Buffer pointer
    bool         rval = false;                     // Return value

    
    //*
    //***  Do initial setup
    //*
    
    author->SetTo(NULL);                           // Nothing to start
    email->SetTo(NULL);                            // Ditto
    
    bufStr.SetTo(value);                           // Set buffer
    strTrim(&bufStr);                              // Trim whitespace
    buf = bufStr.String();                         // Set guts pointer
    
 
    //*
    //***  See if we can parse it in "<left> <right>" form
    //*
    
    len = bufStr.Length();                         // Get string length
    if (!len) goto end;                            // If nothing left
    
    if (delimRight(buf[len - 1]))                  // Delim on the right
      {
        ptr = delimFindLeft(buf, &buf[len - 1]);   // Try to find it
        
        if (ptr)                                   // If found
          {
            size = (ptr - buf);                    // Set left size
            if (size >= 1) left.Append(buf, size); // Set left half
            
            right.SetTo(ptr);                      // Set right half
            goto deal;                             // Handle what we got
          }
      }
    else if (delimLeft(buf[0]))                    // Delim on the left
      {
        ptr = delimFindRight(buf);                 // Get full string
        
        if (ptr)                                   // If found
          {
            size = (ptr - buf) + 1;                // Set left size
            if (size >= 1) left.Append(buf, size); // Set left half
            
            right.SetTo(ptr + 1);                  // Get right side
            goto deal;                             // Handle what we got
          }
      }   
    
    
    //*
    //***  It wasn't in <left> <right> form so punt
    //*
    
    left.SetTo(NULL);                              // Yuck
    right.SetTo(value);                            // Yuck yuck
    
    
    //*
    //***  Deal with the split-up parts
    //*
    
    deal:                                          // Peace, bro
    
    strTrim(&left);                                // Remove remnants from
    strTrim(&right);                               //  splitting it up
    
    if (!left.Length() || !right.Length())         // One or zero strings
      {
        if (left.Length())                         // Only left exists
          email->SetTo(left);
        else if (right.Length())                   // Only right exists
          email->SetTo(right);
      }
    else if (strchr(right.String(), '@') ||        // If right is e-mail
     right.ByteAt(0) == '<')
      {
        author->SetTo(left);                       // Copy author
        email->SetTo(right);                       // Copy e-mail
      }
    else if (strchr(left.String(), '@') ||         // If left is e-mail
     left.ByteAt(0) == '<')
      {
        author->SetTo(right);                      // Copy author
        email->SetTo(left);                        // Copy e-mail
      }
    else if (left.ByteAt(0) == '"')                // Left starts with "
      {
        author->SetTo(left);
        email->SetTo(right);
      }
    else if (right.ByteAt(0) == '"')               // Right starts with "
      {
        author->SetTo(right);
        email->SetTo(left);
      }  
      
    
    //*
    //***  Clean up and return final outcome
    //*
    
    end:                                           // Jump here to exit
    
    delimRemove(author);                           // Remove delimiters
    delimRemove(email);
    author->RemoveSet("\\");                       // Remove quoting chars
    email->RemoveSet("\\");
    
    rval = (author->Length() && email->Length());  // If both were found
    
    if (email->Length() <= 0)                      // If no e-mail string
      email->SetTo(valueEmpty);                    // Set this
    
    return rval;                                   // Return outcome
  }                                                // End splitFrom()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::strTrim() -- char buffer version                            ***/
/*                                                                          */
/****************************************************************************

This procedure removes leading and trailing whitespace from a char buf.     */

void HDRBUF::strTrim(char* str)                    // Begin strTrim()
  {
    int32  i, j;                                   // Loop counters

    if (!str || !str[0]) return;                   // If no string given
    
    for (j = 0; str[j] && isspace(str[j]); j++);   // Go to first non-space

    if (j)                                         // If leading whitespace
      {
        for (i = 0; str[j]; i++, j++)              // Loop through string
          str[i] = str[j];                         // Move chars backward
          
        str[i] = 0;                                // Terminate string
      }    

    i = strlen(str);                               // Get current length
    
    if (i >= 1)                                    // If any remaining
      {
        for (i--; i >= 0 && isspace(str[i]); i--)  // While in whitespace
          str[i] = 0;                              // Lop off space/tab
      }
  }                                                // End strTrim()
  

/****************************************************************************/
/*                                                                          */
/***  HDRBUF::strTrim() -- BString version                                ***/
/*                                                                          */
/****************************************************************************

This procedure removes leading and trailing whitespace from a BString.      */

void HDRBUF::strTrim(BString* str)                 // Begin strTrim()
  {
    char*  buf;                                    // The string buffer
    
    buf = str->LockBuffer(1);                      // Lock it down
    strTrim(buf);                                  // Do it
    str->UnlockBuffer();                           // Finished  
  }                                                // End strTrim()
  

/****************************************************************************/
/*                                                                          */
/***  HDRBUF::subval()                                                    ***/
/*                                                                          */
/****************************************************************************

This procedure takes a pointer to a string that might contain several
sub-strings in the form "name=value;" and picks out the value part for a
given name.  It returns true if it successfully extracted the string, false
if not.                                                                     */

bool HDRBUF::subval(const char* str,               // Begin subval()
 const char* name, BString* value)
  {
    const char*  ptr;                              // Tools through string
    
    ASSERT(name && name[0] && str);                // Must have this stuff
    value->SetTo(NULL);                            // Clear value string
    
    ptr = strstr(str, name);                       // Find this name
    if (!ptr) return false;                        // Exit if not found
    
    for ( ; *ptr && *ptr != ';'; ptr++)            // Loop to find '='
      if (*ptr == '=')                             // If '=' found
        {
          ptr++;                                   // Go past '='
          return subvalGet(ptr, value);            // Pick out contents
        }
        
    return false;                                  // '=' not found
  }                                                // End subval()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::subvalGet()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure takes a pointer to the start of a "phrase" in a header
string and picks out the contents.  It might be delimited with quotation
marks or whatever and may or may not end in a semicolon.                    */

bool HDRBUF::subvalGet(const char* str,            // Begin subvalGet()
 BString* value)
  {
    char         buf[105];                         // Temp buffer
    uint32       i = 0;                            // Loop counter
    const char*  ptr;                              // String pointer
    
    value->SetTo(NULL);                            // Remove all contents
    
    for (ptr = str; *ptr && *ptr != ';'; ptr++)    // Loop for value chars
      {
        if (i >= (sizeof (buf) - 2)) break;        // Don't overrun buffer
        buf[i++] = *ptr;                           // Save that char
      }
      
    buf[i] = 0;                                    // Terminate string
    delimRemove(buf);                              // Remove delimiters
    if (!buf[0]) return false;                     // If nothing left
    
    value->SetTo(buf);                             // Save contents
    return true;                                   // Success
  }                                                // End subvalGet()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::POP3Parse()                                              ***/
/*                                                                          */
/****************************************************************************

Class constructor.                                                          */

POP3Parse::POP3Parse(void)                         // Begin POP3Parse()
  {
    clear();
  }                                                // End POP3Parse()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::~POP3Parse()                                             ***/
/*                                                                          */
/****************************************************************************

Class destructor.                                                           */

POP3Parse::~POP3Parse(void)                        // Begin ~POP3Parse()
  {
    // Nothing to do
  }                                                // End ~POP3Parse()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::clear()                                                  ***/
/*                                                                          */
/****************************************************************************

Clears all class data.                                                      */

void POP3Parse::clear(void)                        // Begin clear()
  {
    fData          = NULL;
    fHdrBuf        = NULL;
    fMessage       = NULL;
    fOffset        = 0;
    fPrevLine      = 0;
  }                                                // End clear()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::headerEnd()                                              ***/
/*                                                                          */
/****************************************************************************

You're reading along in a list of header strings, waiting for that blank
line to show up that indicates that they've ended.  This procedure returns
true if the string given is indeed that blank line, false if not.           */

bool POP3Parse::headerEnd(const char* str)         // Begin headerEnd()
  {
    if (!str || !str[0])                           // If totally empty
      return true;                                 // That's the end
    else                                           // Has contents
      return false;                                // Not finished
  }                                                // End headerEnd()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::headerSave()                                             ***/
/*                                                                          */
/****************************************************************************

After reading a set of headers this procedure will copy data out of the
HDRBUF and into the MessagePart.                                            */

bool POP3Parse::headerSave(MessagePart* part,      // Begin headerSave()
 uint32 offset, uint32 major, uint32 minor)
  {
    char          buf[50];                         // Format buffer
    const char*   ptr;
    
    part->startOffset = offset;                    // Set current offset
    
    ptr = HDRBUF::charsetString(fHdrBuf->fCharset);
    part->characterSet.SetTo(ptr);
    
    ptr = HDRBUF::encodingString(fHdrBuf->fEncoding);
    part->encoding.SetTo(ptr);
    
    ptr = fHdrBuf->fContentID.String();
    part->contentID.SetTo(ptr);
    
    sprintf(buf, "%lu.%lu", major, minor);
    part->id.SetTo(buf);
    
    return true;                                   // Success
  }                                                // End headerSave()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::headerSaveRoot()                                         ***/
/*                                                                          */
/****************************************************************************

After the headers at the start of the message have been read this
procedure saves them to the MimeMessage.                                    */

bool POP3Parse::headerSaveRoot(void)               // Begin headerSaveRoot()
  {
    BString      author, email;
    const char*  ptr;
    
    ptr = HDRBUF::contentString(fHdrBuf->fContent);
    fMessage->SetContentType(ptr);
    
    fMessage->SetDate(fHdrBuf->fDate.String());
    fMessage->SetSubject(fHdrBuf->fSubject.String());
    
    HDRBUF::splitFrom(fHdrBuf->fFrom.String(), &author, &email);
    
    ptr = author.String();
    if (ptr && ptr[0]) fMessage->AddFromName(ptr);
    
    ptr = email.String();
    if (ptr && ptr[0]) fMessage->AddFrom(ptr);
    
    return true;                                   // Success
  }                                                // End headerSaveRoot()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::Parse()                                                  ***/
/*                                                                          */
/****************************************************************************

This is the function that the whole class was written to support.  It reads
an RFC822 message body from the BDataIO and fills in info in the
MimeMessage given.  The BDataIO better be at the beginning of the message
and have a current offset of zero, otherwise the offsets and sizes recorded
in the MimeMessage will be wrong.                                           */

bool POP3Parse::Parse(BDataIO* data, MimeMessage*  // Begin Parse()
 message)
  {
    HDRBUF  hdrBuf;                                // Header buffer
    
    ASSERT(data && message);                       // Neither can be NULL
    clear();                                       // Clear existing data
    
    fData    = data;
    fHdrBuf  = &hdrBuf;
    fMessage = message;
    
    readPart(NULL, 0);                             // Read all parts
    
    clear();                                       // Clear all data
    return (fOffset >= 10);                        // Success if stuff read
  }                                                // End Parse()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::readLine()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure reads the next line from the input into 'str'.  It returns
true if it collected a line, false if end of input has been reached.  It
keeps fOffset updated with the current BDataIO offset.                      */

bool POP3Parse::readLine(BString* str)             // Begin readLine()
  {
    ssize_t  bytes;                                // Bytes read
    char     ch;                                   // Byte from input
    ssize_t  total = 0;                            // Total bytes read
    
    str->SetTo(NULL);                              // Clear existing contents
    fPrevLine = fOffset;                           // Save last line offset
    
    while (true)                                   // Loop to read chars
      {
        bytes = fData->Read(&ch, 1);               // Read next char
        if (bytes <= 0) break;                     // If end of input
        
        ASSERT(bytes == 1);                        // Should be just one
        total += bytes;                            // Update total
        fOffset += bytes;                          // Update offset
        
        if (ch == 0) ch = SPACE;                   // Don't allow nulls!
        
        if (ch == CR || ch == LF)                  // If line end reached
          {
            if (ch == CR)                          // If it's CR
              {
                bytes = fData->Read(&ch, 1);       // Get LF after it
                ASSERT(bytes == 1 && ch == LF);    // Should be LF
              }
              
            break;                                 // Stop the loop
          }
          
        str->Append(ch, 1);                        // Add char to the end
      }                                            // End read loop
      
    return (total >= 1);                           // Got a line?    
  }                                                // End readLine()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::readPart()                                               ***/
/*                                                                          */
/****************************************************************************

Reads one message part.                                                     */

bool POP3Parse::readPart(MessagePart* parent,      // Begin readPart()
 uint32 minor)
  {
    bool          boundaryNew = false;             // New boundary string?
    bool          end;                             // Section end flag
    uint32        level;                           // Next section's level
    BString       line;                            // Line from the file
    uint32        major = 0;                       // Major section number
    uint32        offset = fOffset;                // Section start offset
    MessagePart*  part;                            // New message part
    const char*   ptr;                             // String guts
    bool          rval = false;                    // Final outcome
    MessagePart*  which = NULL;                    // Which parent is right?
    
    part = new MessagePart();                      // Create new part
    if (!part) return false;                       // Exit on failure
    
    fHdrBuf->ClearNextSection();                   // Tidy up a bit
    
    while (true)                                   // Read header lines
      {
        if (!readLine(&line)) return false;        // Get next message line
        ptr = line.String();                       // Pointer to guts
        if (headerEnd(ptr)) break;                 // Exit at header end
        fHdrBuf->Add(ptr);                         // Add to header buffer
      }
  
    boundaryNew = fHdrBuf->End();                  // Flush remaining contents
    
    if (!parent)                                   // I am the root
      {
        fMessage->SetRoot(part);                   // In you go
        headerSaveRoot();                          // Save root data
        major = 1;                                 // Set section major
      }
    else                                           // I am a child
      {
        parent->subParts.AddItem(part);            // Add to part list
        major = parent->subParts.CountItems();     // Set section major
      }  
    
    headerSave(part, offset, major, minor);        // Save all data
    
    while (true)                                   // Loop to read section
      {
        if (!readLine(&line)) goto end;            // Read next line
        ptr = line.String();                       // Pointer to guts
        
        if (fHdrBuf->Boundary(ptr, &end))          // Section boundary?
          {
            if (!end)                              // If start boundary
              {
                if (!parent || boundaryNew)        // I am root or boundary
                  {
                    which = part;                  // You are my kid
                    level = minor + 1;             // Nested level
                  }  
                else                               // No new boundary
                  {
                    which = parent;                // You are my sibling
                    level = minor;                 // Level doesn't change
                  }  
              
                if (!readPart(which, level))       // Read next section
                  goto end;                        // Exit on failure
              }
              
            break;                                 // Stop the loop
          }
      }                                            // End line loop
    
    rval = true;                                   // Success if we get here
    
    end:                                           // Jump here to exit
    sectionEnd(part);                              // End this section
    return rval;                                   // Return final outcome
  }                                                // End readPart()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::sectionEnd()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure is called when we have finished reading a section.  At this
point we will have read the first line of the next section so you can't
use fOffset for size calculations.                                          */

void POP3Parse::sectionEnd(MessagePart* part)      // Begin sectionEnd()
  {
    ASSERT(part);                                  // Should have something
    if (!part) return;                             // If no pointer given
    
    ASSERT(part->size == 0);                       // Only do this once!
    part->size = fPrevLine - part->startOffset;    // Set final size
    ASSERT(part->size >= 10);                      // Verify size
  }                                                // End sectionEnd()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::test()                                                   ***/
/*                                                                          */
/****************************************************************************

Runs the other test routines.                                               */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::test(void)                         // Begin test()
  {
    testSplitFrom();                               // The from splitter
    
    testRead();                                    // Read and compare
 // testWrite();                                   // CAREFUL ABOUT THIS
  }                                                // End test()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::testRead()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure opens as many files as exist in the form msg01.txt,
msg02.txt, and so on, parses them, and checks the results against
msg01.msg, msg02.msg, etc.                                                  */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::testRead(void)                     // Begin testRead()
  {
    BFile*        file = NULL;
    char          filename[150];
    uint32        i;
    MimeMessage*  msg = NULL;
    POP3Parse     parse;
    status_t      status;
    
    for (i = 1; i <= 99; i++)                      // Loop for test files
      {
        sprintf(filename,"TestData/msg%02lu.txt",i);// Create filename
        file = new BFile(filename, B_READ_ONLY);   // Open the file
        status = file->InitCheck();                // Did it work?
        
        if (status == B_OK)                        // If open worked
          {
            msg = new MimeMessage();               // Create new MimeMessage
            parse.Parse(file, msg);                // Do it
            msg->Print(true);                      // Put it to stdout
          
            delete msg;                            // Finished
            msg = NULL;                            // Kill that pointer
          }
        
        delete file;                               // Close the file
        file = NULL;                               // Kill the pointer
        
        if (status != B_OK) break;                 // Stop at end of files
      }
  }                                                // End testRead()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::testSplitFrom()                                          ***/
/*                                                                          */
/****************************************************************************

Tests the complicated From: header splitter.                                */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::testSplitFrom(void)                // Begin testSplitFrom()
  {
    BString    auth, mail;
    TESTFROM*  data;
    uint32     i;
    bool       rval;
    
    for (i = 0; testFrom[i].author; i++)
      {
        data = &testFrom[i];
        
        rval = HDRBUF::splitFrom(data->value,
         &auth, &mail);
         
        ASSERT(!strcmp(auth.String(), data->author));
        ASSERT(!strcmp(mail.String(), data->email));
        ASSERT(data->rval == rval); 
      }    
  }                                                // End testSplitFrom()
#endif                                             // End testing


