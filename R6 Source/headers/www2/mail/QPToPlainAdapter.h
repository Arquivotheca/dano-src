// QPToPlainAdapter.h -- Quoted-printable to UTF8 text
// Started by somebody else, finished by Allen Brunson  January 19, 2001

#ifndef _QPTOPLAINADAPTER_H
#define _QPTOPLAINADAPTER_H

#include <DataIO.h>
#include <String.h>


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter class                                              ***/
/*                                                                          */
/****************************************************************************/

class QPToPlainAdapter: public BDataIO             // Begin QPToPlainAdapter
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
      
    uint32    encoding;                            // Character set
    bool      owning;                              // Delete on destruction?
    BDataIO*  source;                              // Where data comes from
    
    BString   buf;                                 // Storage buffer
    uint32    bufOffset;                           // Where we are in it
    uint32    bufLen;                              // Bytes in buffer
    
	
    //*
    //***  Public function prototypes
    //*
	
    public:

                     QPToPlainAdapter(BDataIO*     // Constructor
                      source, uint32 encoding,
                      bool owning = true);
      
    virtual          ~QPToPlainAdapter(void);      // Destructor

    virtual ssize_t  Read(void* buffer,            // Reads next chunk
                      size_t size);
    
#ifdef TEST_SCAFFOLDING                            // If testing
    static void      test(void);                   // Tests the class
#endif    
    virtual ssize_t  Write(const void *buffer,     // Not implemented
                      size_t size);


    //*
    //***  Private function prototypes
    //*
    
    private:
    
    static bool      charHexDigit(char ch);        // Is it a hex digit?
    static void      charOutput(BString* str,      // Re-constitutes =XX
                      char digit1, char digit2);   //  character thingy
    
    void             convertFromQP(BString* in,    // Quoted-printable to
                      BString* out);               //  UTF8 text
    
    bool             fill(void);                   // Fill buffer from source
    bool             readLine(BString* str);       // Reads line from input
#ifdef TEST_SCAFFOLDING                            // If testing
    static void      testRead(void);               // Tests read cases
#endif
  };                                               // End QPToPlainAdapter


/****************************************************************************/
/*                                                                          */
/***  Testing defines                                                     ***/
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
  
int main(void);                                    // Runs all tests

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter class                                              ***/
/*                                                                          */
/****************************************************************************


Overview
--------

This class takes in text in quoted-printable format in some charset and
converts it to UTF8 text with the original line breaks.  It is fairly
forgiving of weird input, for instance it will accept CR/LF or LF line ends
(but would be confused by CR line ends, the way they do things on the
Macintosh).  All lines it outputs will be CR/LF terminated.


Functions
---------

QPToPlainAdapter::QPToPlainAdapter() should be called with a pointer to the
BDataIO to take input from, an enum from UTF8.h that represents the character
set to decode from, and a bool indicating whether or not the BDataIO will be
deleted in the destructor or not.

QPToPlainAdapter::Read() should be called as many times as necessary to get
each chunk of UTF8 text out of the module.  When all text has been retrieved
it will return -1.  No chunk returned will ever be null-terminated, if you
want that you'll have to do it yourself.


Dependencies
------------

The class reads data from a BDataIO object and uses convert_to_utf8()
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

*/

#endif                                             // End _QPTOPLAINADAPTER_H

