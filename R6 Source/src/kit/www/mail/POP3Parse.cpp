// POP3Parse.cpp -- Parses RFC822 message bodies to MimeMessage class
// by Allen Brunson  June 1, 2001

#include <ctype.h>           // Character classification
#include <parsedate.h>       // parsedate() function
#include <stdio.h>           // Usual I/O stuff
#include <stdlib.h>          // sprintf(), etc.
#include <string.h>          // string functions
#include <Debug.h>           // BeOS debugging stuff
#include "Base64ToRawAdapter.h"// Base64 encoding to raw bytes
#include "MimeMessage.h"     // MIME message container
#include <List.h>            // BList class
#include <String.h>          // BString class
#include <UTF8.h>            // Charset defines and functions
#include "POP3Parse.h"       // Defines for this class

#ifdef TEST_SCAFFOLDING
#include <File.h>            // BFile class (for testing)
#endif


/****************************************************************************/
/*                                                                          */
/***  Misc module data                                                    ***/
/*                                                                          */
/****************************************************************************/

static char valueEmpty[] = "<None>";               // Standard non-header
static char valueUnknown[] = "Unknown";            // Couldn't tell ya

#ifdef TEST_SCAFFOLDING
static char  fmtMsg[] = "TestData/msg%02lu.msg";
static char  fmtTxt[] = "TestData/msg%02lu.txt";
#endif


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
/***  Disposition strings                                                 ***/
/*                                                                          */
/****************************************************************************/

static DISPOSITIONINDEX dispositionIndex[] =       // Disposition indexes
  {
    {dispAttached,     "attached"},                // Attached
    {dispInline,       "inline"},                  // Inline
    
    {dispUnknown, NULL}                            // End of the list
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
      hnumCc,
      "CC",
      NULL,
      vstrCc
    },
    
    {
      hnumContentDisposition,
      "Content-Disposition",
      &HDRBUF::procContentDisposition,
      vstrUnknown
    },
    
    {
      hnumContentEncoding,
      "Content-Transfer-Encoding",
      &HDRBUF::procContentEncoding,
      vstrUnknown
    },
    
    {
      hnumContentID,
      "Content-ID",
      NULL,
      vstrContentID
    },
    
    {
      hnumContentType,
      "Content-Type",
      &HDRBUF::procContentType,
      vstrUnknown
    },
    
    {
      hnumDate,
      "Date",
      &HDRBUF::procDate,
      vstrDate
    },
    
    {
      hnumFrom,
      "From",
      NULL,
      vstrFrom
    },
    
    {
      hnumInReplyTo,
      "In-Reply-To",
      NULL,
      vstrInReplyTo
    },
    
    {
      hnumLines,
      "Lines",
      &HDRBUF::procLines,
      vstrUnknown
    },
    
    {
      hnumRecipient,
      "Recipient",
      NULL,
      vstrRecipient
    },
    
    {
      hnumReplyTo,
      "Reply-To",
      NULL,
      vstrReplyTo
    },
    
    {
      hnumSender,
      "Sender",
      NULL,
      vstrSender
    },
    
    {
      hnumSubject,
      "Subject",
      NULL,
      vstrSubject
    },
    
    {hnumUnknown, NULL, NULL, vstrUnknown}         // End of the list
  };
  

/****************************************************************************/
/*                                                                          */
/***  splitAddr() test data                                               ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING

struct TESTADDR
  {
    const char*  value;
    const char*  author;
    const char*  email;
    bool         rval;
  };
  
static TESTADDR testAddr[] =
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
    HDRBUF::test();                                // Test this class
    POP3Parse::test();                             // Test that class
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
    fDisposition   = dispDefault;
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
So we buffer contents in 'fBuf' until we're sure we've got a whole header
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

    for (i = boundaryTotal - 1; i >= 0; i--)       // Backwards through list
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
    uint32  i;
    
    fBoundaryCount = boundaryCount();              // Update boundaries
    fCharset       = charsetMakeValid(fCharset);   // Make valid for next run
    fContent       = contDefault;
    fEncoding      = encDefault;
    
    fBuf.SetTo(NULL);
    
    // Don't clear boundary strings, we need them!
    
    for (i = 0; i < vstrTotal; i++)
      fValue[i].SetTo(NULL);
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
    
    ptr = str->LockBuffer(1);                      // Lock down the bytes
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
/***  HDRBUF::dispositionString()                                         ***/
/*                                                                          */
/****************************************************************************

Given a DISPOSITION enum this procedure will return a string that describes
it.  If it's unknown it will return "Unknown".                              */

const char* HDRBUF::dispositionString(DISPOSITION  // Beg dispositionString()
 disposition)
  {
    uint32  i;                                     // Loop counter
    
    for (i = 0; dispositionIndex[i].str; i++)      // Loop for all entries
      if (dispositionIndex[i].disposition == disposition)
        return dispositionIndex[i].str;            // There it is
        
    return valueUnknown;                           // Sorry bub
  }                                                // End dispositionString()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::dispositionValue()                                          ***/
/*                                                                          */
/****************************************************************************

Given a string that contains a disposition type it will return the
corresponding DISPOSITION enum.                                             */

DISPOSITION HDRBUF::dispositionValue(const         // Begin dispositionValue()
 char* str)
  {
    uint32  i;                                     // Loop counter
    
    if (!str || !str[0]) return dispUnknown;       // Empty string
    
    for (i = 0; dispositionIndex[i].str; i++)      // Loop for all entries
      if (!strcasecmp(str,dispositionIndex[i].str))// If match is found
        return dispositionIndex[i].disposition;    // There it is
    
    return dispUnknown;                            // It wasn't found
  }                                                // End dispositionValue()


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
    int32        i;                                // Loop counter
    const char*  ptr = fBuf.String();              // Get contents
    bool         rval;                             // Value to return
    BString*     value;                            // Next value string
    
    if (ptr && ptr[0])                             // If anything in there
      {
        scan();                                    // Parse it
        fBuf.SetTo(NULL);                          // Clear it
      }
      
    charset = charsetMakeValid(fCharset);          // Charset for conversion
    
    for (i = 0; i < vstrTotal; i++)                // Loop for value strings
      {
        value = &fValue[i];
        hdrDecode(&buf, value->String(), charset);
        value->SetTo(buf);
      }
    
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
message and it will be used to do the conversion.

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
    char         ch;                               // Next char from input
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
          ch = bufPtr[bufIdx++];                   // Get next character
          if (ch == TAB) ch = SPACE;               // Tabs display wrong!
          dst->Append(ch, 1);                      // Now save it
        }
      
    strTrim(dst);                                  // Trim the junk
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
            if (ch == TAB) ch = SPACE;             // Tabs display wrong
            buf[bufIdx++] = ch;                    // Add to output
            ptrIdx += 3;                           // Go past string bytes
          }
        else                                       // Not encoded char
          {
            ch = ptr[ptrIdx];                      // Get next char
            if (ch == '_') ch = SPACE;             // Underscores to spaces
            if (ch == TAB) ch = SPACE;             // Tabs display wrong
            
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
/***  HDRBUF::procContentDisposition()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure reads a Content-Disposition header.                          */

bool HDRBUF::procContentDisposition(const char*    // Beg proc..Disposition()
 value)
  {
    DISPOSITION  disposition;                      // Disposition type
    BString      str;                              // Temp string
    
    subvalGet(value, &str);                        // Get disposition
    disposition = dispositionValue(str.String());  // Convert it
    
    if (disposition != dispUnknown)                // If it's good
      fDisposition = disposition;                  // Save it
    
    subval(value, "filename", &str);               // Fish out filename
    
    if (str.Length() >= 1)                         // If it's good
      fValue[vstrFilename].SetTo(str);             // Save it
    
    return true;                                   // Success
  }                                                // End proc..Disposition()


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
    
    subval(value, "name", &str);                   // Fish out filename
    
    if (str.Length() >= 1)                         // If it's good
      fValue[vstrFilename].SetTo(str);             // Save it
    
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
    fDateTime = parsedate(value, -1);              // Relative to now
    if (fDateTime > 0) return true;                // If it was understood
    
    fDateTime = real_time_clock_usecs() / 1000000; // Punt
    return false;                                  // Failed
  }                                                // End procDate()


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
    READPROC     proc;                             // Processor proc
    const char*  ptr;                              // Guts of 'buf'
    bool         rval;                             // Proc outcome
    VALUESTR     vnum;                             // String number
    
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
          proc = headerInfo[i].proc;               // Save proc
          vnum = headerInfo[i].vnum;               // Save string number
          
          if (vnum < vstrTotal)                    // If string given
            fValue[vnum].SetTo(ptr);               // Save it
            
          if (proc) rval = (this->*proc)(ptr);     // Call the proc
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
/***  HDRBUF::splitAddr()                                                 ***/
/*                                                                          */
/****************************************************************************

This procedure takes an address string from a header that may contain an
e-mail address and/or a name and splits it into component parts.  Headers
can have RFC2047 garbage and non-ASCII characters in them; this procedure
assumes the text has already been converted to UTF8 elsewhere.  Apparently
when IMAP parses a header it gives you nothing if the author name isn't
present so that's what I do here.                                           */

bool HDRBUF::splitAddr(const char* value,          // Begin splitAddr()
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
    
    /* CAN'T DO THIS!  It removes high-order UTF-8 characters
       for some reason.
       
    author->RemoveSet("\\");                       // Remove quoting chars
    email->RemoveSet("\\");
    */
    
    rval = (author->Length() && email->Length());  // If both were found
    
    if (email->Length() <= 0)                      // If no e-mail string
      email->SetTo(valueEmpty);                    // Set this
    
    return rval;                                   // Return outcome
  }                                                // End splitAddr()


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::splitMany()                                                 ***/
/*                                                                          */
/****************************************************************************

There are a bunch of e-mail headers that can contain more than one
name/e-mail pair, separated by commas.  This procedure looks at one of
those headers and splits out the addresses into zero or more BStrings
which it saves in the BList given.                                          */

bool HDRBUF::splitMany(const char* value,          // Begin splitMany()
 BList* list)
  {
    #define delimTotal (10)                        // Max delims
    
    char      ch;                                  // Next char from string
    int32     d = 0;                               // Current delim count
    char      delim[delimTotal + 1];               // Delimiter list
    int32     i;                                   // Loop counter
    int32     len;                                 // String length
    int32     start = 0;                           // Next addr starts here
    BString*  str = NULL;                          // Next BString
    
    ASSERT(list->CountItems() == 0);               // Empty to start, please
    memset(delim, 0, sizeof (delim));              // Wipe delim list
    
    if (!value || !value[0]) return false;         // If no string given
    
    for (i = 0; ; i++)                             // Loop the string
      {
        ch = value[i];                             // Get next char
        
        if (d >= 1 && delimRight(ch) &&            // If this is a closing
         ch == delimMatchLeft(delim[d - 1]))       //  right delimiter
          {
            delim[d - 1] = 0;                      // Pop it off
            d--;                                   // That's one less
          }  
        else if (d < delimTotal && delimLeft(ch))  // If left delimiter
          {
            delim[d++] = ch;                       // Push it on the stack
          }
        
        if (ch == 0 || (d == 0 && ch == ','))      // If addr end found
          {
            len = i - start;                       // Set string length
            
            if (len >= 1)                          // If we've got something
              {
                str = new BString();               // Make a new one
                if (!str) return false;            // Exit on failure
                
                str->SetTo(&value[start], len);    // Suck it up
                strTrim(str);                      // Trim it
                
                if (str->Length() >= 1)            // If anything left
                  list->AddItem(str);              // In you go
                else                               // Nothing left
                  delete str;                      // Delete it
                    
                str = NULL;                        // Don't use it again
              }  
            
            if (ch == 0) break;                    // Stop at string end
            start = i + 1;                         // Go past comma
          }
      }
      
    return (list->CountItems() >= 1);              // Return outcome
  }                                                // End splitMany()


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
/***  HDRBUF::test()                                                      ***/
/*                                                                          */
/****************************************************************************

Class tests                                                                 */

#ifdef TEST_SCAFFOLDING
void HDRBUF::test(void)                            // Begin test()
  {
    testBoundary();
    testSplitAddr();
    testSplitMany();
  }                                                // End test()
#endif


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::testBoundary()                                              ***/
/*                                                                          */
/****************************************************************************

Boundary string tests.  Man, this just keeps tripping me up.                */

#ifdef TEST_SCAFFOLDING
void HDRBUF::testBoundary(void)                    // Begin testBoundary()
  {
    bool    end;
    HDRBUF  hdrBuf;
    
    ASSERT(!hdrBuf.Boundary("--s", &end));
    
    hdrBuf.boundaryAdd("st");
    
    ASSERT(!hdrBuf.Boundary(NULL, &end));
    ASSERT(!hdrBuf.Boundary("", &end));
    
    ASSERT(!hdrBuf.Boundary("s", &end));
    ASSERT(!hdrBuf.Boundary("st", &end));
    
    end = true;
    ASSERT(hdrBuf.Boundary("--st", &end));
    ASSERT(end == false);
  
    ASSERT(!hdrBuf.Boundary("--st---", &end));
    
    ASSERT(hdrBuf.Boundary("--st--", &end));
    ASSERT(end == true);
  }                                                // End testBoundary()
#endif


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::testDispose()                                               ***/
/*                                                                          */
/****************************************************************************

Gets rid of a BList of BStrings.                                            */

#ifdef TEST_SCAFFOLDING
void HDRBUF::testDispose(BList* list)              // Begin testDispose()
  {
    uint32    count = list->CountItems();
    uint32    i;
    BString*  str;
    
    for (i = 0; i < count; i++)
      {
        str = (BString*) list->ItemAt(i);
        delete str; str = NULL;
      }
      
    list->MakeEmpty();    
  }                                                // End testDispose()
#endif


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::testSplitAddr()                                             ***/
/*                                                                          */
/****************************************************************************

Tests the complicated address splitter.                                     */

#ifdef TEST_SCAFFOLDING                            // If testing
void HDRBUF::testSplitAddr(void)                   // Begin testSplitAddr()
  {
    BString    auth, mail;
    TESTADDR*  data;
    uint32     i;
    bool       rval;
    
    for (i = 0; testAddr[i].author; i++)
      {
        data = &testAddr[i];
        rval = splitAddr(data->value, &auth, &mail);
         
        ASSERT(!strcmp(auth.String(), data->author));
        ASSERT(!strcmp(mail.String(), data->email));
        ASSERT(data->rval == rval); 
      }    
  }                                                // End testSplitAddr()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  HDRBUF::testSplitMany()                                             ***/
/*                                                                          */
/****************************************************************************

Tests the many address splitter.                                            */

#ifdef TEST_SCAFFOLDING                            // If testing
void HDRBUF::testSplitMany(void)                   // Begin testSplitMany()
  {
    BList     list;
    BString*  str;
    
    ASSERT(!splitMany(NULL, &list));
    ASSERT(!splitMany("", &list));
    
    ASSERT(splitMany("fee,<f,i>,(f o), fum  ,  , ", &list));
    ASSERT(list.CountItems() == 4);
    
    str = (BString*) list.ItemAt(0);
    ASSERT(!strcmp(str->String(), "fee"));
    
    str = (BString*) list.ItemAt(1);
    ASSERT(!strcmp(str->String(), "<f,i>"));
    
    str = (BString*) list.ItemAt(2);
    ASSERT(!strcmp(str->String(), "(f o)"));
    
    str = (BString*) list.ItemAt(3);
    ASSERT(!strcmp(str->String(), "fum"));
    
    testDispose(&list);
    
    ASSERT(splitMany(
     "\"Bloody Pulp\" <bloody@mind.com>, "
     "\"Stupid Man\" <stoopid@man.com>", &list));
     
    ASSERT(list.CountItems() == 2);
    
    str = (BString*) list.ItemAt(0);
    ASSERT(!strcmp(str->String(), "\"Bloody Pulp\" <bloody@mind.com>"));
    
    str = (BString*) list.ItemAt(1);
    ASSERT(!strcmp(str->String(), "\"Stupid Man\" <stoopid@man.com>"));
     
    testDispose(&list); 
    
    splitMany("(((<<<><<<<\"'a,'\">>>>", &list);
    ASSERT(list.CountItems() == 1);
    testDispose(&list);
  }                                                // End testSplitMany()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::POP3Parse()                                              ***/
/*                                                                          */
/****************************************************************************

Class constructor.                                                          */

POP3Parse::POP3Parse(MimeMessage* message)         // Begin POP3Parse()
  {
    ASSERT(message);
    
    fBoundaryAdded  = false;
    fInHeaders      = true;
    fMessage        = message;
    
    fOffset         = 0;
    fOffsetLineCurr = 0;
    fOffsetLineNext = 0;
    
    fRoot           = new MessagePart();
    fCurrent        = fRoot;
    
    fRoot->id.SetTo("1");
    fMessage->SetRoot(fRoot);
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
/***  POP3Parse::Finish()                                                 ***/
/*                                                                          */
/****************************************************************************

Call this when all data has been added to the class and it's time to
finish up.                                                                  */

MimeMessage* POP3Parse::Finish(void)               // Begin Finish()
  {
    ASSERT(fRoot == fCurrent);
    sectionStop(fOffset);
    return fMessage;
  }                                                // End Finish()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::lineProcess()                                            ***/
/*                                                                          */
/****************************************************************************

Once a line has been picked out of the input this procedure gets to process
it.  This is the single most complicated and fragile function in the whole
class, it was a major bitch to write, and even tiny changes can have
profound unintended side effects.  Caveat emptor!                           */

void POP3Parse::lineProcess(void)                  // Begin lineProcess()
  {
    bool          end;                             // End of sections bool
    const char*   line = fLine.String();           // Pointer to guts
    MessagePart*  parent;                          // Which is parent?
    
    if (fInHeaders)                                // If we are in headers
      {
        if (!line || !line[0])                     // End of headers?
          {
            fBoundaryAdded = fHdrBuf.End();        // Do final processing
            fInHeaders = false;                    // Headers are over
            
            if (fCurrent == fRoot)                 // If this is the first
              sectionStartRoot();                  // Save root data
            
            sectionStart();                        // Save section data
            fHdrBuf.ClearNextSection();            // Set up to do it again
          }
        else                                       // Not end of headers
          {
            fHdrBuf.Add(line);                     // Squirrel it away
          }  
      }
    else if (fHdrBuf.Boundary(line, &end))         // If that's a boundary
      {
        if (!end)                                  // If section begin
          {
            fInHeaders = true;                     // Headers have started
            
            if (fRoot==fCurrent || fBoundaryAdded) // If adding a child
              {
                parent = fCurrent;                 // You are my kid
              }  
            else                                   // Adding a sibling
              {
                parent = fCurrent->parent;         // We have the same dad
                sectionStop(fOffsetLineCurr);      // End current section
              }  
              
            ASSERT(parent);                        // MUST have a parent
            if (!parent) return;                   // Must bail otherwise
            
            fCurrent = new MessagePart();          // Create new part
            fCurrent->parent = parent;             // Set lineage
            fCurrent->startOffset = fOffset;       // Save that offset
            
            parent->subParts.AddItem(fCurrent);    // Add to kid list
            parent->isContainer = true;            // You're a proud papa
              
            fBoundaryAdded = false;                // It's accounted for
          }
        else                                       // End of all sections
          {
            sectionStop(fOffsetLineCurr);          // Finish current section
            fCurrent = fCurrent->parent;           // Back one level
            
            ASSERT(fCurrent);                      // Don't run off the end
            if (!fCurrent) fCurrent = fRoot;       // Just in case we did
          }  
      }  
  }                                                // End lineProcess()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::sectionStart()                                           ***/
/*                                                                          */
/****************************************************************************

After reading a set of headers this procedure will copy data out of the
HDRBUF and into the current MessagePart.  It's done for the root headers
*and* for every MIME header section.                                        */

bool POP3Parse::sectionStart(void)                 // Begin sectionStart()
  {
    char         buf[50];                          // Format buffer
    int32        parts;                            // Total subparts
    const char*  ptr;                              // String guts pointer
    
    ptr = HDRBUF::charsetString(fHdrBuf.fCharset);
    fCurrent->characterSet.SetTo(ptr);
    
    ptr = HDRBUF::contentString(fHdrBuf.fContent);
    fCurrent->type.SetTo(ptr);
    
    ptr = HDRBUF::encodingString(fHdrBuf.fEncoding);
    fCurrent->encoding.SetTo(ptr);
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrContentID].String();
    fCurrent->contentID.SetTo(ptr);
    
    if (fCurrent->parent)                          // If it has a parent
      {
        parts = fCurrent->parent->subParts.CountItems();
        ASSERT(parts >= 1);                        // I SHOULD BE ADDED NOW
        sprintf(buf, ".%lu", parts);
        
        fCurrent->id.SetTo(fCurrent->parent->id);
        fCurrent->id.Append(buf);
      }
      
    ASSERT(fCurrent->id.Length() >= 1);  
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrFilename].String();
    fCurrent->name.SetTo(ptr);
    
    switch (fHdrBuf.fDisposition)
      {
        case dispAttached:
          fCurrent->disposition = MessagePart::kAttached;
          break;
          
        case dispInline:
          fCurrent->disposition = MessagePart::kInline;
          break;
      
        default:
          fCurrent->disposition = MessagePart::kNoDisposition;
          break;
      }
      
    switch (fHdrBuf.fContent)
      {
        case contMultiAlt:
          fCurrent->containerType = MessagePart::kAlternative;
          break;
          
        case contMultiMixed:
          fCurrent->containerType = MessagePart::kMixed;
          break;
          
        case contMultiRelated:
          fCurrent->containerType = MessagePart::kRelated;
          break;  
          
        default:
          fCurrent->containerType = MessagePart::kNotContainer;
          break;
      }
    
    return true;                                   // Success
  }                                                // End sectionStart()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::sectionStartRoot()                                       ***/
/*                                                                          */
/****************************************************************************

After the headers at the start of the message have been read this
procedure saves them to the MimeMessage.  This is only done once right
after the master headers are read.                                          */

bool POP3Parse::sectionStartRoot(void)             // Begin sectionStartRoot()
  {
    BString      author, email;
    int32        count, i;
    BList        list;
    const char*  ptr;
    BString*     str = NULL;
    
    
    //*
    //***  Easy stuff first
    //*
    
    ptr = HDRBUF::contentString(fHdrBuf.fContent);
    fMessage->SetContentType(ptr);
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrInReplyTo].String();
    if (ptr && ptr[0]) fMessage->SetInReplyTo(ptr);
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrDate].String();
    if (!ptr || !ptr[0]) ptr = valueEmpty;
    fMessage->SetDate(ptr);
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrSubject].String();
    if (!ptr || !ptr[0]) ptr = valueEmpty;
    fMessage->SetSubject(ptr);
    
    
    //*
    //***  Split up Cc: header
    //*
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrCc].String();
    HDRBUF::splitMany(ptr, &list);
    count = list.CountItems();
    
    for (i = 0; i < count; i++)
      {
        str = (BString*) list.ItemAt(i);
        HDRBUF::splitAddr(str->String(), &author, &email);
        delete str; str = NULL;
    
        ptr = author.String();
        if (ptr && ptr[0]) fMessage->AddCcName(ptr);
    
        ptr = email.String();
        if (ptr && ptr[0]) fMessage->AddCc(ptr);
      }
     
    list.MakeEmpty(); 

    
    //*
    //***  Split up From: header
    //*
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrFrom].String();
    HDRBUF::splitMany(ptr, &list);
    count = list.CountItems();
    
    for (i = 0; i < count; i++)
      {
        str = (BString*) list.ItemAt(i);
        HDRBUF::splitAddr(str->String(), &author, &email);
        delete str; str = NULL;
    
        ptr = author.String();
        if (ptr && ptr[0]) fMessage->AddFromName(ptr);
    
        ptr = email.String();
        if (ptr && ptr[0]) fMessage->AddFrom(ptr);
      }
     
    list.MakeEmpty(); 

    
    //*
    //***  Split up Recipient: header
    //*
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrRecipient].String();
    HDRBUF::splitMany(ptr, &list);
    count = list.CountItems();
    
    for (i = 0; i < count; i++)
      {
        str = (BString*) list.ItemAt(i);
        HDRBUF::splitAddr(str->String(), &author, &email);
        delete str; str = NULL;
    
        ptr = author.String();
        if (ptr && ptr[0]) fMessage->AddRecipientName(ptr);
    
        ptr = email.String();
        if (ptr && ptr[0]) fMessage->AddRecipient(ptr);
      }
     
    list.MakeEmpty(); 
    
    
    //*
    //***  Split up Reply-To: header
    //*
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrReplyTo].String();
    HDRBUF::splitMany(ptr, &list);
    count = list.CountItems();
    
    for (i = 0; i < count; i++)
      {
        str = (BString*) list.ItemAt(i);
        HDRBUF::splitAddr(str->String(), &author, &email);
        delete str; str = NULL;
    
        ptr = author.String();
        if (ptr && ptr[0]) fMessage->AddReplyToName(ptr);
    
        ptr = email.String();
        if (ptr && ptr[0]) fMessage->AddReplyTo(ptr);
      }
     
    list.MakeEmpty(); 
    
    
    //*
    //***  Split up Sender: header
    //*
    
    ptr = fHdrBuf.fValue[HDRBUF::vstrSender].String();
    HDRBUF::splitMany(ptr, &list);
    count = list.CountItems();
    
    for (i = 0; i < count; i++)
      {
        str = (BString*) list.ItemAt(i);
        HDRBUF::splitAddr(str->String(), &author, &email);
        delete str; str = NULL;
    
        ptr = author.String();
        if (ptr && ptr[0]) fMessage->AddSenderName(ptr);
    
        ptr = email.String();
        if (ptr && ptr[0]) fMessage->AddSender(ptr);
      }
     
    list.MakeEmpty(); 
    
    return true;                                   // Success
  }                                                // End sectionStartRoot()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::sectionStop()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure is called when we have finished reading a section.  The
offset given will be to the first byte that is NOT part of the section,
i.e., one past the end.                                                     */

void POP3Parse::sectionStop(int32 offset)          // Begin sectionStop()
  {
    ASSERT(fCurrent && offset >= 10);              // Verify input
    if (!fCurrent) return;                         // If no pointer given
    
    #ifdef TEST_SCAFFOLDING
    if (!fCurrent->parent)                         // If root section
      ASSERT(fCurrent->startOffset ==  0);         // Must start at zero
    else                                           // Not root section
      ASSERT(fCurrent->startOffset >= 10);         // Must start past zero  
    #endif
    
    ASSERT(fCurrent->size == 0);                   // Only do this once!
    
    fCurrent->size =                               // Set final size
     offset - fCurrent->startOffset;
    
    ASSERT(fCurrent->size >= 10);                  // Verify size
  }                                                // End sectionStop()


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::test()                                                   ***/
/*                                                                          */
/****************************************************************************

Runs the other test routines.                                               */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::test(void)                         // Begin test()
  {
    testWrite();                                   // Write flattened
    testRead();                                    // Read and compare
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
    BFile*    file = NULL;
    char      fname[150];
    uint32    i = 0;
    status_t  status = 0;
    
    for (i = 1; i <= 99; i++)                      // Loop for test files
      {
        sprintf(fname, fmtTxt, i);                 // Create filename
        file = new BFile(fname, B_READ_ONLY);      // Open the file
        status = file->InitCheck();                // Did it work?
        if (status == B_OK) testReadOne(file, i);  // Read it on success
        
        delete file; file = NULL;                  // Close the file
        if (status != B_OK) break;                 // Stop at end of files
      }
      
    ASSERT(i >= 2);                                // Do at least one file    
  }                                                // End testRead()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::testReadOne()                                            ***/
/*                                                                          */
/****************************************************************************

Does the details of one file read and compare.                              */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::testReadOne(BFile* file,           // Begin testReadOne()
 int32 index)
  {
    char                  buf[512];                // Read buffer
    ssize_t               bytes = 0;               // Bytes written
    char                  fname[150];
    BMallocIO             io1, io2;
    MimeMessage           msg1, msg2;
    BFile*                disk = NULL;
    POP3Parse             parse(&msg1);
    BufferedFileAdapter*  shim = NULL;
    size_t                size1 = 0, size2 = 0;
    status_t              status = 0;
    
    fname[0] = 0;
    
    while (true)                                   // Loop to read file
      {
        status = file->Read(buf, sizeof (buf));    // Get next chunk
        ASSERT(status >= 0);                       // It should work
        if (status <= 0) break;                    // Stop at file end
        
        bytes = parse.Write(buf, status);          // Put bytes into parser
        ASSERT(bytes == status);                   // Should take them all
      }
      
    parse.Finish();                                // Finish it up
    msg1.WriteToStream(&io1);                      // Flatten to buffer
    testReadSection(file, msg1.GetRoot());         // Test sections
    
    sprintf(fname, fmtMsg, index);                 // Create flattened name
    disk = new BFile(fname, B_READ_ONLY);          // Open it
    status = disk->InitCheck();                    // Did it work?
    ASSERT(status == B_OK);                        // It should have
    
    shim = new BufferedFileAdapter(disk, 512, 0);  // Intermediate thing
    status = msg2.LoadFromBuffer(*shim);           // Read it in
    ASSERT(status == B_OK);                        // It should work
    
    delete shim; shim = NULL;                      // Kill the shim
    delete disk; disk = NULL;                      // Close the file
    
    msg2.WriteToStream(&io2);                      // Now write to buffer
    
    size1 = io1.BufferLength();                    // Get first length
    size2 = io2.BufferLength();                    // Get second length
    ASSERT(size1 == size2);                        // Should be the same
    
    ASSERT(!memcmp(io1.Buffer(), io2.Buffer(), size1));
  }                                                // End testReadOne()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::testReadSection()                                        ***/
/*                                                                          */
/****************************************************************************

Checks the contents of one MessagePart against the actual file.             */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::testReadSection(BFile* file,       // Begin testReadSection()
 const MessagePart* part)
  {
    char          buf[41];                         // Bytes from file
    int32         i = 0;                           // Loop counter
    int64         offset = 0;                      // Calculated offset
    int32         parts = 0;                       // Total sub-parts
    int64         position = 0;                    // New position
    off_t         size = 0;                        // File size
    status_t      status = 0;                      // Status return
    MessagePart*  subpart = NULL;                  // Next subpart
    
    ASSERT(part->size >= (int)(sizeof (buf)));     // Would be weird otherwise
    memset(buf, 0, sizeof (buf));                  // Ensure termination
    
    offset = part->startOffset;                    // Set starting offset
    position = file->Seek(offset, SEEK_SET);       // Go there
    ASSERT(position == part->startOffset);         // It should work
    
    status = file->Read(buf, sizeof (buf) - 1);    // Read starting bytes
    ASSERT(status == sizeof (buf) - 1);            // It should work
    
    offset  = part->startOffset;                   // Start here
    offset += part->size;                          // Past end of section
    offset -= sizeof (buf) - 1;                    // To start of bytes
    
    position = file->Seek(offset, SEEK_SET);       // Go to last bit
    ASSERT(position == offset);                    // It better work
     
    status = file->Read(buf, sizeof (buf) - 1);    // Read ending bytes
    ASSERT(status == sizeof (buf) - 1);            // It should work
    
    if (part->parent == NULL)                      // If this is the root
      {
        status = file->GetSize(&size);             // Get file size
        ASSERT(status == B_OK);                    // It should work
        
        ASSERT(part->startOffset == 0);            // Should start at zero
        ASSERT(part->size == size);                // Verify size
      }
    else                                           // Not the root
      {
        ASSERT(part->startOffset > 0);             // Somewhere forward
        ASSERT(part->size >= 10);                  // At least a few bytes
      }  
    
    parts = part->subParts.CountItems();           // Count sub-parts
    ASSERT(part->isContainer == (parts >= 1));     // isContainer must be set
    
    for (i = 0; i < parts; i++)                    // Loop for sub-parts
      {
        subpart = (MessagePart*) part->subParts.ItemAt(i);
        testReadSection(file, subpart);            // Do this one
      }  
  }                                                // End testReadSection()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::testWrite()                                              ***/
/*                                                                          */
/****************************************************************************

Reads in msg01.txt, msg02.txt, and so on, parses them, then saves the
resultant flattened MimeMessage as msg01.msg, msg02.msg, etc.               */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::testWrite(void)                    // Begin testWrite()
  {
    BFile*    file = NULL;
    char      fname[150];
    uint32    i;
    status_t  status;
    
    for (i = 1; i <= 99; i++)                      // Loop for test files
      {
        sprintf(fname, fmtTxt, i);                 // Create filename
        file = new BFile(fname, B_READ_ONLY);      // Open the file
        status = file->InitCheck();                // Did it work?
        if (status == B_OK) testWriteOne(file, i); // Write it
        
        delete file; file = NULL;                  // Close the file
        if (status != B_OK) break;                 // Stop at end of files
      }
  }                                                // End testWrite()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::testWriteOne()                                           ***/
/*                                                                          */
/****************************************************************************

After one original message file has been opened this procedure parses it
and flattens the results to disk.                                           */

#ifdef TEST_SCAFFOLDING                            // If testing
void POP3Parse::testWriteOne(BFile* file,          // Begin testWrite()
 int32 index)
  {
    char         buf[97];                          // Odd-sized buffer
    char         fname[150];                       // Filename string
    uint32       mode = B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE;
    MimeMessage  msg;                              // The parsee
    BFile*       out = NULL;                       // Output file
    POP3Parse    parse(&msg);                      // The parser
    status_t     status = 0;                       // Outcome
    
    sprintf(fname, fmtMsg, index);                 // Set up filename
    
    out = new BFile(fname, B_READ_ONLY);           // Try to open it
    status = out->InitCheck();                     // Did it work?
    delete out; out = NULL;                        // Close it again
    if (status == B_OK) return;                    // Exit if already there
    
    while (true)                                   // Loop to parse contents
      {
        status = file->Read(buf, sizeof (buf));    // Read next bit
        ASSERT(status >= 0);                       // It should succeed
        if (status <= 0) break;                    // Stop if all read
        parse.Write(buf, status);                  // Stick the bytes in
      }
      
    parse.Finish();                                // Now we're done
    
    out = new BFile(fname, mode);                  // Open output file
    status = out->InitCheck();                     // Did it work?
    ASSERT(status == B_OK);                        // It should
    
    if (status == B_OK)                            // If it worked
      msg.WriteToStream(out);                      // Spit it out
    
    delete out; out = NULL;                        // Close the file
  }                                                // End testWriteOne()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  POP3Parse::Write()                                                  ***/
/*                                                                          */
/****************************************************************************

Writes another chunk of input into the parser.                              */

ssize_t POP3Parse::Write(const void* buffer,       // Begin Write()
 size_t numBytes)
  {
    char         ch;                               // Next char from input
    uint32       i;                                // Loop counter
    const char*  ptr = (const char*) buffer;       // Pointer to contents
    
    ASSERT(buffer && numBytes >= 1);               // Verify input
    if (numBytes <= 0) return 0;                   // Exit if nothing to do
    
    for (i = 0; i < numBytes; i++, ptr++)          // Loop for input bytes
      {
        ch = *ptr; if (ch == 0) ch = SPACE;        // Get next char
        fOffset++;                                 // To next offset
        
        if (ch == LF)                              // If end of line
          {
            fOffsetLineCurr = fOffsetLineNext;
            fOffsetLineNext = fOffset;
            
            lineProcess();                         // Handle the line
            fLine.SetTo(NULL);                     // Throw it away
          }
        else if (ch != CR)                         // Regular line char
          {
            fLine.Append(ch, 1);                   // Add to line
          }  
      }
  
    return numBytes;                               // Did them all
  }                                                // End Write()


