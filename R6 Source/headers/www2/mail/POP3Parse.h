// POP3Parse.h -- Reads RFC822 message bodies to MimeMessage struct
// by Allen Brunson  June 1, 2001

#ifndef _POP3PARSE_H                             // If file not defined
#define _POP3PARSE_H                             // Start it now

#include "MimeMessage.h"     // MimeMessage container
#include <String.h>          // BString class
#include <DataIO.h>          // BDataIO class
#include <String.h>          // BString class
#include <UTF8.h>            // Charset defines and functions


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
    hnumContentEncoding,                           // Transfer encoding
    hnumContentID,                                 // Content-ID
    hnumContentType,                               // Content-Type
    hnumDate,                                      // Date/Time
    hnumFrom,                                      // "Author" and "E-mail"
    hnumLines,                                     // Line count
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
    ENCODING       fEncoding;                      // Transfer encoding
    time_t         fDateTime;                      // Date/time
    int32          fLines;                         // Line count
    
    BString        fBoundary[boundaryTotal];       // Boundary strings
    BString        fBuf;                           // Holds header in progress
    BString        fContentID;                     // Content-ID header
    BString        fDate;                          // Date header
    BString        fFilename;                      // Attachment filename
    BString        fFrom;                          // Who it's from
    BString        fSubject;                       // Subject string

    
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
    
    static const char* encodingString(ENCODING     // ENCODING to string
                        encoding);
    static ENCODING encodingValue(const char* str);// String to ENCODING
    
    static bool    splitFrom(const char* value,    // Splits up a From:
                    BString* author,               //  header
                    BString* email);
    
    

    
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
    bool           procContentID(const char*       // Content-ID processor
                    value);                
    bool           procContentType(const           // Content type processor
                    char* str);                
    bool           procDate(const char* value);    // Date processor
    bool           procFrom(const char* value);    // From processor
    bool           procLines(const char* value);   // Lines processor
    bool           procSubject(const char* value); // Subject processor
    
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
  };                                               // End HDRBUF
      
      
/****************************************************************************/
/*                                                                          */
/***  POP3Parse class                                                     ***/
/*                                                                          */
/****************************************************************************/

class POP3Parse                                    // Begin POP3Parse class
  {
    //*
    //***  Various class defines
    //*
    
    public:
    
    
    //*
    //***  Class data
    //*
    
    protected:
    
    BDataIO*      fData;                           // Read message from here
    HDRBUF*       fHdrBuf;                         // Current header buffer
    MimeMessage*  fMessage;                        // Info goes here
    uint32        fOffset;                         // Current offset
    uint32        fPrevLine;                       // Offset at previous line
    
    
    //*
    //***  The public interface
    //*
    
    public:
    
                   POP3Parse(void);                // Constructor
                   ~POP3Parse(void);               // Destructor
    
    bool           Parse(BDataIO* data,            // The Big Kahuna
                    MimeMessage* message);
    
    #ifdef TEST_SCAFFOLDING
    static void    test(void);                     // Tests the class
    #endif
    
    
    //*
    //***  Private function prototypes
    //*
    
    private:
    
    void           clear(void);                    // Clears class data
    
    static bool    headerEnd(const char* str);     // Is this the end?
    bool           headerSave(MessagePart* part,   // Saves part data
                    uint32 offset, uint32 major,
                    uint32 minor);
    bool           headerSaveRoot(void);           // Saves first headers
    bool           readLine(BString* str);         // Gets next line
    bool           readPart(MessagePart* parent,   // Reads one part
                    uint32 minor);
    void           sectionEnd(MessagePart* part);  // Section is over
    
    #ifdef TEST_SCAFFOLDING
    static void    testRead(void);                 // Reads messages
    static void    testSplitFrom(void);            // Tests from splitter
    #endif
  };                                               // End POP3Parse class


/****************************************************************************/
/*                                                                          */
/***  POP3Parse class                                                     ***/
/*                                                                          */
/****************************************************************************

Overview
--------

This class reads the body of an RFC822 from a BDataIO, presumably from a
POP3 server but it could have come from anywhere, and fills in a
MimeMessage class with information about its sections and attachments.


Header buffer
-------------

The header buffer is a little glob of information that is held onto as we
are making our way through the message body.  It gets initialized to blank
values at the start and is dragged through to the very end.  Its character
set, encoding type, content type, and so on will change to reflect whatever
is proper for the MIME section that we currently find ourselves in.  It
also contains one or more boundary strings necessary for finding the beginning
and end of MIME sections.  Since a MIME message may have another MIME message
embedded inside it we need to keep track of more than one boundary string
at a time.  Once the boundary stop signal is found then that level of nesting
is over with and the boundary string itself can be thrown away.


Stuff to do
-----------

At this point I'm not sure if I'm supposed to expect the "." on a line
by itself to signal the end of a message or not.  Right now it does not.

*/

#endif                                             // End _POP3PARSE_H
