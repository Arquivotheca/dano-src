// POP3Parse.h -- Reads RFC822 message bodies to MimeMessage struct
// by Allen Brunson  June 1, 2001

#ifndef _POP3PARSE_H                             // If file not defined
#define _POP3PARSE_H                             // Start it now

#include "MimeMessage.h"     // MimeMessage container
#include <List.h>            // BList class
#include <String.h>          // BString class
#include <UTF8.h>            // Charset defines and functions

#ifdef TEST_SCAFFOLDING
#include <File.h>            // BFile class (for testing only)
#endif


/****************************************************************************/
/*                                                                          */
/***  Charset defines                                                     ***/
/*                                                                          */
/****************************************************************************/

enum CHARSET                                       // Character set values
  {
    csetEUC_KR       = B_EUC_KR_CONVERSION,        // EUC-KR
    csetISO_2022_JP  = B_JIS_CONVERSION,           // ISO-2022-JP
    csetISO_8859_1   = B_ISO1_CONVERSION,          // ISO-8859-1
    csetISO_8859_2   = B_ISO2_CONVERSION,          // ISO-8859-2
    csetISO_8859_3   = B_ISO3_CONVERSION,          // ISO-8859-3
    csetISO_8859_4   = B_ISO4_CONVERSION,          // ISO-8859-4
    csetISO_8859_5   = B_ISO5_CONVERSION,          // ISO-8859-5
    csetISO_8859_6   = B_ISO6_CONVERSION,          // ISO-8859-6
    csetISO_8859_7   = B_ISO7_CONVERSION,          // ISO-8859-7
    csetISO_8859_8   = B_ISO8_CONVERSION,          // ISO-8859-8
    csetISO_8859_9   = B_ISO9_CONVERSION,          // ISO-8859-9
    csetISO_8859_10  = B_ISO10_CONVERSION,         // ISO-8859-10
    csetISO_8859_13  = B_ISO13_CONVERSION,         // ISO-8859-13
    csetISO_8859_14  = B_ISO14_CONVERSION,         // ISO-8859-14
    csetISO_8859_15  = B_ISO15_CONVERSION,         // ISO-8859-15
    csetKOI8_R       = B_KOI8R_CONVERSION,         // KOI8R (Russian)
    csetMacintosh    = B_MAC_ROMAN_CONVERSION,     // Macintosh
    csetWindows1251  = B_MS_WINDOWS_1251_CONVERSION,// Windows 1251
    csetWindows1252  = B_MS_WINDOWS_CONVERSION,    // Windows 1252
        
    csetUTF8         = 200,                        // Made-up UTF8 number
    csetUS_ASCII     = 201,                        // Made-up US-ASCII number
        
    csetUnknown      = 300,                        // Couldn't tell ya
    csetDefault      = csetISO_8859_1              // Default character set
  };
      
struct CHARSETINDEX                                // Charset to strings
  {
    CHARSET      charset;                          // Charset value
    const char*  str;                              // Associated string
  };
      
      
/****************************************************************************/
/*                                                                          */
/***  Miscellaneous MIME defines                                          ***/
/*                                                                          */
/****************************************************************************/

enum ASCII                                         // ASCII character defines
  {
    TAB   = 0x09,                                  // Tab character
    CR    = 0x0D,                                  // Carriage return
    LF    = 0x0A,                                  // Line feed
    SPACE = 0x20,                                  // Good ol' space
  };

enum CONTENT                                       // Content types
  {
    contUnknown,                                   // Couldn't tell ya
        
    contMultiAlt,                                  // multipart/alternative
    contMultiMixed,                                // multipart/mixed
    contMultiRelated,                              // multipart/related
    contMultiOther,                                // multipart/*
        
    contTextHTML,                                  // text/html
    contTextPlain,                                 // text/plain
    
    contDefault = contTextPlain,                   // Default if not found
  };
      
struct CONTENTINDEX                                // Content type to strings
  {
    CONTENT      content;                          // Content enum
    const char*  str;                              // String
  };
  
enum DISPOSITION                                   // Dispositon types
  {
    dispUnknown,                                   // Couldn't tell ya
    dispAttached,                                  // Attached
    dispInline,                                    // Inline
    
    dispDefault = dispInline                       // Default if not found
  };
  
struct DISPOSITIONINDEX                            // String to enum struct
  {
    DISPOSITION  disposition;                      // Disposition type
    const char*  str;                              // String
  };  

enum ENCODING                                      // Encoding types
  {
    encUnknown,                                    // Couldn't tell ya
    encBase64,                                     // Base64 binary
    encBit7,                                       // 7-bit text
    encBit8,                                       // 8-bit text
    encQuotePrint,                                 // Quoted printable text
        
    encDefault = encBit8                           // Default if not found
  };
  
struct ENCODINGINDEX                               // String to enum struct
  {
    ENCODING     encoding;                         // Encoding type
    const char*  str;                              // String
  };  

enum HDRNUM                                        // Headers we examine
  {
    hnumUnknown,                                   // Couldn't tell ya
    hnumCc,                                        // Carbon copy addresses
    hnumContentDisposition,                        // Content-Disposition
    hnumContentEncoding,                           // Transfer encoding
    hnumContentID,                                 // Content-ID
    hnumContentType,                               // Content-Type
    hnumDate,                                      // Date/Time
    hnumFrom,                                      // From addresses
    hnumInReplyTo,                                 // A message ID
    hnumLines,                                     // Line count
    hnumRecipient,                                 // Recipient addresses
    hnumReplyTo,                                   // ReplyTo addresses
    hnumSender,                                    // Sender addresses
    hnumSubject,                                   // Subject
  };
      

/****************************************************************************/
/*                                                                          */
/***  The all-important header buffer class                               ***/
/*                                                                          */
/****************************************************************************/

class HDRBUF                                       // Header buffer
  {
    //*
    //***  A bunch of defines
    //*
    
    public:
    
    enum GENERAL                                   // General defines
      {
        boundaryTotal = 4,                         // Total boundary strings
      };
      
    enum VALUESTR                                  // Header value strings
      {
        vstrCc,                                    // CC:
        vstrContentID,                             // Content-ID:
        vstrDate,                                  // Date:
        vstrFilename,                              // From a couple of headers
        vstrFrom,                                  // From:
        vstrInReplyTo,                             // Replied-to message-ID
        vstrRecipient,                             // Recipient:
        vstrReplyTo,                               // Reply-To:
        vstrSender,                                // Sender:
        vstrSubject,                               // Subject:
        
        vstrTotal,                                 // Total strings
        vstrUnknown                                // Couldn't tell ya
      };
      
    typedef bool (HDRBUF::*READPROC)(const char* value);
    
    struct DELIMITER                               // A delimiter set
      {
        char  left;                                // The left delimiter
        char  right;                               // The right delimiter
      };
      
    struct ENCODE                                  // Encoded word data
      {
        CHARSET  charset;                          // Encoded charset
        char     type;                             // Type: B or Q
        char*    ptr;                              // Pointer to the meat
        int32    len;                              // Length of the meat
        int32    total;                            // Total length
      };
      
    struct INFO                                    // Info on one header
      {
        HDRNUM       hnum;                         // Header number
        const char*  name;                         // Header name
        READPROC     proc;                         // Proc that parses it
        VALUESTR     vnum;                         // Where it's saved
      };
     
    
    //*
    //***  Class data
    //*
    
    public:
    
    static DELIMITER  delim[];                     // Delimiter list
    static INFO       headerInfo[];                // Header list
    
    int32          fBoundaryCount;                 // Current boundary count
    CHARSET        fCharset;                       // Character set
    CONTENT        fContent;                       // Content type
    DISPOSITION    fDisposition;                   // Disposition
    time_t         fDateTime;                      // Date/time
    ENCODING       fEncoding;                      // Transfer encoding
    int32          fLines;                         // Line count
    
    BString        fBoundary[boundaryTotal];       // Boundary strings
    BString        fBuf;                           // Holds header in progress
    BString        fValue[vstrTotal];              // Value strings

    
    //*
    //***  Public interface
    //*
    
    public:
    
                   HDRBUF(void);                   // Constructor
                   ~HDRBUF(void);                  // Destructor
                   
    HDRNUM         Add(const char* str);           // Adds header to buffer
    bool           Boundary(const char* str,       // Is this a MIME boundary?
                    bool* end);
    void           ClearNextSection(void);         // Clears for next section
    bool           End(void);                      // Finish unprocessed parts
    
    
    //*
    //***  Public utility functions
    //*
    
    public:
    
    static const char* charsetString(CHARSET       // CHARSET to string
                    charset);                
    static CHARSET charsetValue(const char* str);  // String to CHARSET
    
    static const char* contentString(CONTENT       // CONTENT to string
                    content);
    static CONTENT contentValue(const char* str);  // String to CONTENT
    
    static const char* dispositionString(          // DISPOSITION to string
                        DISPOSITION disposition);
    static DISPOSITION dispositionValue(const      // string to DISPOSITION
                        char* str);                    
    static const char* encodingString(ENCODING     // ENCODING to string
                        encoding);
    static ENCODING encodingValue(const char* str);// String to ENCODING
    
    static bool    splitAddr(const char* value,    // Splits up an address
                    BString* author,               //  from a header
                    BString* email);
    static bool    splitMany(const char* value,    // Splits up many addrs
                    BList* list);                  //  from one header
                    
    #ifdef TEST_SCAFFOLDING
    static void    test(void);
    #endif
    
    
    //*
    //***  Private function prototypes
    //*
    
    private:
    
    void           boundaryAdd(const char* str);   // Saves MIME boundary
    int32          boundaryCount(void);            // Total boundaries
    BString*       boundaryCurrent(void);          // Gets current boundary
    void           boundaryDel(void);              // Throws away newest
    
    static char    charDecode(const char* str);    // "=XX" to char
    static bool    charIsHexDigit(char ch);        // Is this a hex digit?
    
    static bool    charsetConvert(CHARSET          // Character set conversion
                    srcCharset, const char* src,   //  routine
                    CHARSET dstCharset,
                    BString* dst,
                    int32* state = NULL);
    static bool    charsetIsValid(CHARSET charset);// Is it any good?
    static CHARSET charsetMakeValid(CHARSET        // Forces it to be good
                    charset);
    static void    charsetUSASCII(BString* str);   // Forces to 7-bit
    
    static char*   delimFindLeft(const char* str,  // Finds left match for
                    const char* end);              //  right delimiter
    static char*   delimFindRight(const char* str);// Finds right for left
    static bool    delimLeft(char ch);             // Is it a left delimiter?
    static char    delimMatchLeft(char ch);        // Finds right for left
    static char    delimMatchRight(char ch);       // Finds left for right
    static bool    delimPair(char left,char right);// Is this a pair?
    static bool    delimRemove(char* str);         // Removes delimiters
    static bool    delimRemove(BString* str);      // Removes delimiters
    static bool    delimRight(char ch);            // Is it a right delimiter?
    
    static void    hdrDecode(BString* dst,         // Header charset
                    const char* src,               //  conversion
                    CHARSET charset);
    static bool    hdrDecodeFind(const char* str,  // Parses RFC2047 encoded
                    ENCODE* encode);               //  words
    static void    hdrDecodeWordB(BString* dst,    // Decodes B-encoded words
                    const ENCODE* encode);
    static void    hdrDecodeWordQ(BString* dst,    // Decodes Q-encoded words
                    const ENCODE* encode);                
    
    static bool    lineContin(const char* str);    // A continuation line?
    
    bool           procContentEncoding(const       // Content encoding
                    char* value);                  //  processor
    bool           procContentDisposition(const    // Content disposition
                    char* value);                  //  processor
    bool           procContentType(const           // Content type processor
                    char* str);                
    bool           procDate(const char* value);    // Date processor
    bool           procLines(const char* value);   // Lines processor
    
    HDRNUM         scan(void);                     // Scans current contents
    
    static bool    split(const char* str,          // Splits "name: value"
                    BString* name, BString* value);//  headers into pieces
                    
    static void    strTrim(BString* str);          // Trims leading/trailing
    static void    strTrim(char* str);             // Trims leading/trailing
    
    static bool    subval(const char* str,         // Picks out "name=value"
                    const char* name,              //  strings
                    BString* value);
    static bool    subvalGet(const char* str,      // Picks out phrases
                    BString* value);
                    
    #ifdef TEST_SCAFFOLDING
    static void    testBoundary(void);
    static void    testDispose(BList* list);
    static void    testSplitAddr(void);
    static void    testSplitMany(void);
    #endif
  };                                               // End HDRBUF
      
      
/****************************************************************************/
/*                                                                          */
/***  POP3Parse class                                                     ***/
/*                                                                          */
/****************************************************************************/

class POP3Parse                                    // Begin POP3Parse class
  {
    //*
    //***  Class data
    //*
    
    protected:
    
    bool          fBoundaryAdded;                  // New boundary string?
    HDRBUF        fHdrBuf;                         // Header buffer
    bool          fInHeaders;                      // In headers?
    BString       fLine;                           // Next input line
    MimeMessage*  fMessage;                        // Info goes here
    MessagePart*  fCurrent;                        // Current message part
    MessagePart*  fRoot;                           // Root message part
    
    uint32        fOffset;                         // Current offset
    uint32        fOffsetLineCurr;                 // Current line start
    uint32        fOffsetLineNext;                 // Next line start
    
    
    //*
    //***  The public interface
    //*
    
    public:
    
                   POP3Parse(MimeMessage* message);// Constructor
                   ~POP3Parse(void);               // Destructor
    
    MimeMessage*   Finish(void);                   // Flush remaining
    
    ssize_t        Write(const void* buffer,       // Adds bytes to parse
                    size_t numBytes);                
    
    #ifdef TEST_SCAFFOLDING
    static void    test(void);                     // Tests the class
    #endif
    
    
    //*
    //***  Private function prototypes
    //*
    
    private:
    
    void           lineProcess(void);              // Got another line
    
    bool           sectionStart(void);             // Saves part data
    bool           sectionStartRoot(void);         // Saves first headers
    void           sectionStop(int32 offset);      // Current section is over
    
    #ifdef TEST_SCAFFOLDING
    static void    testRead(void);                 // Reads messages
    static void    testReadOne(BFile* file,        // Reads one file
                    int32 index);
    static void    testReadSection(BFile* file,    // Reads bytes from
                    const MessagePart* part);      //  one section
    static void    testWrite(void);                // Writes MimeMessages
    static void    testWriteOne(BFile* file,       // Writes one file
                    int32 index);
    #endif
  };                                               // End POP3Parse class


/****************************************************************************/
/*                                                                          */
/***  POP3Parse class                                                     ***/
/*                                                                          */
/****************************************************************************

Overview
--------

This class reads the body of an RFC822 message via repeated Write() calls,
presumably from a POP3 server but it could have come from anywhere, and
fills in a MimeMessage class with information about its sections and
attachments.

It works like this: Construct a POP3Parse object with a pointer to a
newly constructed blank MimeMessage.  Call POP3Parse::Write() as many
times as necessary to write all parts of the message into the class.
Finally, when all message bytes have been read, call POP3Parse::Finish() to
allow the class to do final processing.  Voila, the MimeMessage will be
completely filled out. 


Header buffer
-------------

The header buffer is a little glob of information that is held onto as we
are making our way through the message body.  It gets initialized to blank
values at the start and is dragged through to the very end.  Its character
set, encoding type, content type, and so on will change to reflect whatever
is proper for the MIME section that we currently find ourselves in.  It
also contains zero or more boundary strings necessary for finding the
beginning and end of MIME sections.  Since a MIME message may have another
MIME message embedded inside it we need to keep track of more than one
boundary string at a time.  Once the boundary stop signal is found then that
level of nesting is over with and the boundary string itself can be thrown
away.


Notes on section parsing
------------------------

A MimeMessage contains one or more MessagePart class instances that represent
sections in the message file.  If the message contains only one section
(including non-MIME messages) then the MimeMessage will have only the
root MessagePart.  If there are more than one MIME section then the root
MessagePart will contain at least one MessagePart, which might contain other
MessageParts, and so on.  The ID of the root section will be "1", its first
child will be "1.1", its second child will be "1.2", and so on.  If "1.1"
has children the first one will be "1.1.1", and so on.  Any MessagePart that
contains other message parts will have its isContainer bool set.

POP3Parse saves a start offset and byte count for each MessagePart.  The
root MessagePart will always have a start offset of zero (at the start of
the root headers) and its size will be the total number of bytes in the
message.  Other sections' start offsets will be at the beginning of their
headers and end just before the MIME message boundary string that signals
their end.  Note that a section may contain other sections so in that case
you may run into boundary strings inside of it.


Test code
---------

The test code and a main() is compiled if TEST_SCAFFOLDING is defined.  It
looks in a subdir called TestData for text files that contain RFC822 message
bodies to work on: msg01.txt, msg02.txt, and so on.  The files' line endings
can be either CR/LF or just LF.  BeMail message files will probably work,
although I haven't tried it.  The first time it tries to open msgXX.txt and
it doesn't work it assumes it has reached the end of the list.  If there is
no corresponding flattened MimeMessage it will create one for each message
file: msg01.msg, msg02.msg, and so on.  If a .msg file already exists
it is left alone.  (If you want it to re-generate the flattened MimeMessage
you must manually delete the .msg yourself.)  On the read pass it opens
each message .txt file, parses it, and compares it to its flattened
counterpart to make sure they are the same.  If they aren't it will ASSERT().

*/

#endif                                             // End _POP3PARSE_H
