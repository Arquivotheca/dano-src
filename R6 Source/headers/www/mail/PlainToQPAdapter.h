// PlainToQPAdapter.h -- Converts UTF8 text to quoted-printable
// by Allen Brunson  January 12, 2001

#ifndef _PLAINTOQPADAPTER_H                        // If file not defined
#define _PLAINTOQPADAPTER_H                        // Start it now

#include <DataIO.h>
#include <String.h>


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter class                                              ***/
/*                                                                          */
/****************************************************************************/

class PlainToQPAdapter: public BDataIO             // Begin PlainToQPAdapter
  {
    //*
    //***  Private data
    //*
    
    private:
    
    enum ASCII                                     // ASCII defines
      {
        TAB   = 0x09,
        CR    = 0x0D,
        LF    = 0x0A,
        SPACE = 0x20,
      };
      
    enum GENERAL                                   // General defines
      {
        lineLenMax = 76,                           // Max line length
      };
      
    uint32    encoding;                            // Encoding to use
    bool      owning;                              // Delete source at end?
    BDataIO*  source;                              // Where data comes from
    
    BString   buf;                                 // Quoted-printable output
    uint32    bufLen;                              // How many bytes
    uint32    bufOffset;                           // Where we are in it
    
    
    //*
    //***  Public function prototypes
    //*
    
    public:
    
                     PlainToQPAdapter(BDataIO*     // Constructor
                      source, uint32 encoding,
                      bool owning = true);
                      
    virtual          ~PlainToQPAdapter(void);      // Destructor

    virtual ssize_t  Read(void* buffer,            // Gets next chunk
                      size_t size);
                      
    static void      test(void);                   // Tests the class                  
    
    virtual ssize_t  Write(const void* buffer,     // Not implemented
                      size_t size);
                      

    //*
    //***  Private function prototypes
    //*
    
    private:
    
    static uint32    charEncodeLen(char ch,        // How many bytes will
                      bool lineEnd);               //  it occupy?
    
    static bool      charEncodeNeeded(uchar ch,    // Must encode this char?
                      bool lineEnd);
    
    static bool      charHexDigit(char ch);        // Is it a hex digit?
    
    static inline uint32 charOutput(BString* str,  // Outputs char to the
                          uchar ch, bool encode);  //  string
    
    void             convertToQP(BString* in,      // Converts one line
                      BString* out);
    
    bool             fill(void);                   // Fills buffer
    bool             readLine(BString* str);       // Gets a line from input
    
    static void      testRead(void);               // Tests read function
  };                                               // End PlainToQPAdapter


/****************************************************************************/
/*                                                                          */
/***  Testing stuff                                                       ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // If testing

struct TESTDATA                                    // Data for one test
  {
    const char*  in;                               // Original text
    const char*  out;                              // Output text
    uint32       encoding;                         // Encoding to use
    int32        bufferSize;                       // Buffer size to use
  };
  
int  main(void);                                   // Does module tests

#endif                                             // End test stuff


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter class                                              ***/
/*                                                                          */
/****************************************************************************


Overview
--------

This class takes in text in UTF8 format and converts it to quoted printable
in a given character set.  It is fairly forgiving of weird input, for
instance it will accept CR/LF or LF line ends (but would be confused by
CR line ends, the way they do things on the Macintosh).  All lines it outputs
will be CR/LF terminated.


Functions
---------

PlainToQPAdapter::PlainToQPAdapter() should be called with a pointer to the
BDataIO to take input from, an enum from UTF8.h that represents the character
set to encode to, and a bool indicating whether or not the BDataIO will be
deleted in the destructor or not.

PlainToQPAdapter::Read() should be called as many times as necessary to get
each chunk of quoted-printable text out of the module.  When all text has been
retrieved it will return -1.  No chunk returned will ever be null-terminated,
if you want that you'll have to do it yourself.


Dependencies
------------

The class reads data from a BDataIO object and uses convert_from_utf8()
from libtextencoding.so to do the heavy lifting.  It also makes use of
BString and a few C library routines.


Quoted-printable reference
--------------------------

See RFC2045 for the full poop on quoted-printable.  It is a way to create
a 7-bit-friendly representation of text that is mostly US-ASCII but might
contain a few odd characters or very long lines.  The main points:

* ASCII values 33 through 60 inclusive and 62 through 126 inclusive can be
  represented as themselves.  Other octets should be represented as =XX where
  XX are two uppercase hex digits.
  
* Tab and space (ASCII 9 and 32) may be represented as themselves EXCEPT if
  they come at the very end of a line.  The idea is that message transfer
  agents may decide to add or remove whitespace at the end of a line which
  would screw you up.  So in that case you'd want to encode them in the =XX
  format.
  
* A line can be up to 76 chars long including everything up to but not
  including the terminating CR/LF.  If you come across a line in the input
  that's longer than that, cut the line in two with a "soft line break,"
  which is an equals sign by itself as the very last character on the line.

  
Testing
-------

If you set the compiler define TEST_SCAFFOLDING then test cases and a main()
will be compiled.  Run the resulting program to do the tests.  If a problem
is found the code will ASSERT().


Maintenance notes
-----------------

Don't forget that '=XX' occurring in the input stream will have to be dealt
with in such a way that it won't be interpreted when our quoted-printable
output is decoded on the receiving end.  I deal with it by encoding the
equals sign.  So for example "=AA" in the input would become "=3DAA" in
the output.  When decoding on the other end the opposite will be done and
you'll be back to "=AA".

*/

#endif                                             // End _PLAINTOQPADAPTER_H
