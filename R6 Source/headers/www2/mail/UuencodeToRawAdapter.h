// UuencodeToRawAdapter.h -- UUEncoded text to binary
// by Allen Brunson  January 8, 2001

#ifndef _UUENCODETORAWADAPTER_H
#define _UUENCODETORAWADAPTER_H

#include <DataIO.h>          // Memory buffer thingie
#include <String.h>          // String buffer


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter class                                          ***/
/*                                                                          */
/****************************************************************************/

class UuencodeToRawAdapter: public BDataIO         // Beg UuencodeToRawAdapter
  {
    //*
    //***  Private data
    //*
    
    private:
    
    enum GENERAL                                   // General defines
      {
        TAB      = 0x09,                           // Tab
        LF       = 0x0A,                           // Line feed
        CR       = 0x0D,                           // Carriage return
        SPACE    = 0x20,                           // Space
      };
      
    enum MODE                                      // Decoding modes
      {
        mUnknown = 0,                              // Couldn't tell ya
        mStart   = 1,                              // Waiting for 'start'
        mData    = 2,                              // Processing data lines
        mEnd     = 3,                              // Got 'end'
      };  
    
    BDataIO*  buf;                                 // Where data comes from
    BString   filename;                            // Data filename
    BString   line;                                // Line from file
    int32     lineBytes;                           // BINARY bytes in line
    int32     lineOffset;                          // Current offset in line
    MODE      mode;                                // Current mode
    uint8     overflow[4];                         // Overflow buffer
    uint32    overflowCount;                       // How many in there?
    bool      owning;                              // Delete BDataIO afterward?
    
    
    //*
    //***  Public function prototypes
    //*
    
    public:
    
                     UuencodeToRawAdapter(BDataIO* // Constructor
                      source, bool owning = true);
    virtual          ~UuencodeToRawAdapter(void);  // Destructor

    void             Filename(BString* str);       // Gets data filename
    MODE             Mode(void);                   // Current mode
    
    ssize_t          Read(void *buffer,            // Data out in binary
                      size_t size);                //  format
                      
    static void      test(void);                   // Runs all tests
    
    ssize_t          Write(const void *buffer,     // Not implemented
                      size_t size);


    //*
    //***  Private function prototypes
    //*
    
    private:
    
    static inline int8  CharToDigit(int8 ch);      // UUENCODE char to digit
    
    ssize_t          ReadData(void* buffer,        // Reads data chunk
                      size_t size);
    ssize_t          ReadDataLine(const char* line,// Converts one line
                      void* buffer, size_t size);
                      
    bool             ReadFilename(void);           // Gets filename from input
    bool             ReadLine(void);               // Gets a line from source
    
    static void      strStrip(BString* str);       // Trims whitespace
    static void      strStripLeft(BString* str);   // Starting whitespace
    static void      strStripRight(BString* str);  // Ending whitespace
    static bool      strWord(BString* str,         // Gets Nth word out of
                      uint32 num, char* word,      //  the string
                      uint32 wordSize);
                      
    static void      testRead(void);               // "File" to raw data
    static void      testWord(void);               // String word finder
    
    static inline uint32  wordAdd(uint32 word,     // Adds char to decode word
                           char ch);
    static inline bool    wordFull(uint32 word);   // Is word full-up?
    static inline uint32  wordOutput(void* buf,    // Outputs to buffer
                           uint32 word);
  };                                               // End UuencodeToRawAdapter


/****************************************************************************/
/*                                                                          */
/***  Testing stuff                                                       ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // If testing

struct TESTDATA                                    // Data for one test
  {
    const char*   text;                            // Original text
    const uint8*  binary;                          // Final binary output
    const char*   filename;                        // Recovered filename
    int32         bufferSize;                      // Buffer size to use
    int32         binaryLen;                       // Binary final length
  };
  
int   main(void);                                  // Runs tests

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter                                                ***/
/*                                                                          */
/****************************************************************************


Overview
--------

This adapter takes in a text buffer in UUENCODE format and spits out raw
binary data and the filename.  It is one in the "adapter" series started
by Kenny C. and Jeff B.  The guts were filled in by lil' ol' me, Allen
Brunson.


Usage
-----

Construct a new object with a pointer to a BDataIO to read textual data
from.  Call UuencodeToRawAdapter::Read() as many times as necessary to get
all the binary data.  The adapter cannot be rewound so if you want to go
around again you have to destroy the object and start over.

The class is very forgiving in terms of the input text.  It will accept the
normal CR/LF line ends that normally come from a mail server or just LF line
ends which are normal in BeOS/UNIX text files.  There can be whitespace
before the 'begin' line or after the 'end' line, whitespace at the beginning
and end of lines, illegal characters embedded in a line (which are ignored),
and so on.


Functions
---------

UuencodeToRawAdapter::UuencodeToRawAdapter() constructs a new object with
a pointer to a BDataIO to read textual data from.  The 'owning' flag says
whether this class will delete the BDataIO on destruction or not (the default
is true).

UuencodeToRawAdapter::Filename() returns the filename that was collected
from the text.  This is useful in recreating a binary file and you might
be able to guess the data type from the extension.  You won't want to call
it before UuencodeToRawAdapter::Mode() returns mData because the filename
won't have been collected before then.

UuencodeToRawAdapter::Mode() returns the mode that the adapter is currently
in: mStart means nothing has happened yet; mData means the 'start' line has
been collected and we are doing the data; mEnd means that the 'end' line
has been read and all data has been output.

UuencodeToRawAdapter::Read() reads binary data out of the adapter one
buffer-full at a time.  It will return the number of bytes output until all
has been collected, then it returns -1.  Due to sticky implementation details
you can't call this function with a buffer that is less than four bytes long
or else the function might write more data than you can accept.


UUENCODE Reference
------------------

UUENCODE is an ancient UNIX thing that encodes three binary bytes into four
text characters, similar to the more advanced Base64 encoding.  Characters
are converted from text to a UUENCODE digit by subtracting 0x20 to get a
value between 0 and 63.  The one exception is the digit zero, which is not
encoded as a space as you might guess but rather as '`'.  Go figure.  Each
encoded line starts with a length byte that indicates the number of BINARY
bytes encoded on the line and then a series of UUENCODE characters.  An
example file:

  begin 644 foobar.txt
  M0F5)1$4@4WES=&5M($EN8VQU9&4@4&%T:',*+2TM+2TM+2TM+2TM+2TM+2TM
  M+2TM+2TM+2T*"B]-87)T:6%N+W-O=7)C92]R96PO:&5A9&5R<R]P;W-I>`HO
  M36%R=&EA;B]S;W5R8V4O<F5L+VEN<W1A;&PO:34X-B]D979E;&]P+VAE861E
  M<G,O8W!P"B]B;V]T+V1E=F5L;W`O;&EB+W@X-@HO36%R=&EA;B]S;W5R8V4O
  M<F5L+W-R8R]I;F,O;65D:6%?<`HO36%R=&EA;B]S;W5R8V4O<F5L+W-R8R]I
  M;F,O:6YT97)F86-E7W`*+TUA<G1I86XO<V]U<F-E+W)E;"]S<F,O:6YC+W-U
  M<'!O<G1?<`HO36%R=&EA;B]S;W5R8V4O<F5L+W-R8R]I;F,O;W-?<`HO36%R
  M=&EA;B]S;W5R8V4O<F5L+W-R8R]T<F%C:V5R"B]-87)T:6%N+W-O=7)C92]R
  596PO:&5A9&5R<R]I;G1E<F9A8V4*
  `
  end 
  
Notice that the length byte at the beginning has to be decoded from UUENCODE
to a digit between 0 and 63 just like all the other characters.  Therefore
you could never have more than 63 data characters on a line and the standard
seems to be 60.

The first line contains the UNIX file attributes, which are generally useless,
and the filename.  There is no indication of the MIME filetype since this
informal "standard" pre-dates MIME.  You have to guess the filetype from the
extension.


Dependencies
------------

BString, BDataIO (of course), a few standard C library functions.


Multi-threading
---------------

This class is not thread-safe.


Maintenance notes
-----------------

The variable lineOffset is used to keep track of the offset in the current
line where we are decoding characters NOT INCLUDING the length byte in the
first position.  When it's time to read a new line from the input then
lineOffset is set to a negative value.  This is only used during data
decoding though, not while reading the filename or other junk.


Testing
-------

You can set the compiler define TEST_SCAFFOLDING and compile this module
into a standalone program with its own main() that runs test cases.  It will
ASSERT() if it finds problems.


Stuff to do
-----------

The class is finished, unless somebody discovers a bug.

*/

#endif                                         // End _UUENCODETORAWADAPTER_H
