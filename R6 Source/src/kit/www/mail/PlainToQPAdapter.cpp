// PlainToQPAdapter.cpp -- Converts UTF8 text to quoted-printable
// by Allen Brunson  January 12, 2001

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <UTF8.h>
#include "PlainToQPAdapter.h"


/****************************************************************************/
/*                                                                          */
/***  Test data                                                           ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING

static const TESTDATA  testData[] =                // Data for tests
  {
    {                                              // Test case  0
      "",                                          // Input text
      "",                                          // Output text
      B_ISO1_CONVERSION, 2                         // Charset, buffer size
    },
    
    {                                              // Test case  1
      "\r\ninput" B_UTF8_COPYRIGHT,                // Input text
      "\r\ninput=A9\r\n",                          // Output text
      B_ISO1_CONVERSION, 1                         // Charset, buffer size
    },
    
    {                                              // Test case  2
      "line1\t  \nline2\t",                        // Input text
      "line1\t =20\r\nline2=09\r\n",               // Output text
      B_ISO2_CONVERSION, 6                         // Charset, buffer size
    },
    
    {                                              // Test case  3
      "bad: =BB\r\n",                              // Input text
      "bad: =3DBB\r\n",                            // Output text
      B_ISO1_CONVERSION, 10                        // Charset, buffer size
    },
    
    {                                              // Test case  4
      "0123456789012345678901234567890123456789"   // Input text
      "0123456789012345678901234567890123456789",
      
      "0123456789012345678901234567890123456789"   // Output text
      "01234567890123456789012345678901234=\r\n"
      "56789\r\n",
      
      B_ISO2_CONVERSION, 1000                      // Charset, buffer size
    },
    
    {                                              // Test case  5
      "0123456789012345678901234567890123456789"   // Tricky boundary case
      "0123456789012345678901234567890123"
      " " B_UTF8_COPYRIGHT,
      
      "0123456789012345678901234567890123456789"   // Output text
      "0123456789012345678901234567890123=\r\n"
      " =A9\r\n",
      
      B_ISO1_CONVERSION, 1000                      // Charset, buffer size
    },
    
    {NULL, NULL}                                   // End of the list
  };

#endif


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::PlainToQPAdapter()                                ***/
/*                                                                          */
/****************************************************************************

This is the class constructor.                                              */

PlainToQPAdapter::PlainToQPAdapter(BDataIO*        // Begin PlainToQPAdapter()
 source, uint32 encoding, bool owning)
  {
    this->bufLen    = 0;                           // Nothing in buffer
    this->bufOffset = 0;                           // Reset offset
    
    this->encoding  = encoding;                    // Save conversion type
    this->owning    = owning;                      // Save owning flag
    this->source    = source;                      // Save source address
  }                                                // End PlainToQPAdapter()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::~PlainToQPAdapter()                               ***/
/*                                                                          */
/****************************************************************************

This is the class destructor.                                               */

PlainToQPAdapter::~PlainToQPAdapter(void)          // Beg ~PlainToQPAdapter()
  {
    if (owning)                                    // If we own source
      {
        delete source;                             // Delete it
        source = NULL;                             // Invalidate pointer
      }  
  }                                                // End ~PlainToQPAdapter()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::charEncodeLen()                                   ***/
/*                                                                          */
/****************************************************************************

This procedure returns how many bytes a character is going to occupy when
it is output: 1 if it can be represented as itself, 3 if it will have to
be encoded.  If given a null it assumes the char will not be output and
will therefore return zero.                                                 */

uint32 PlainToQPAdapter::charEncodeLen(char ch,    // Begin charEncodeLen()
 bool lineEnd)
  {
    if (ch == 0) return 0;                         // Don't output nulls
    
    if (charEncodeNeeded(ch, lineEnd))             // If encoding needed
      return 3;                                    // That'll be three bytes
    else                                           // No encoding
      return 1;                                    // Just one
  }                                                // End charEncodeLen()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::charEncodeNeeded()                                ***/
/*                                                                          */
/****************************************************************************

This procedure returns true if a character needs to be encoded, false if it
can be represented as itself.  We have to know if this will be the last
character on the line because whitespace can only be represented as itself
if it is NOT at the end of the line.                                        */

bool PlainToQPAdapter::charEncodeNeeded(uchar ch,  // Begin charEncodeNeeded()
 bool lineEnd)
  {
    if (ch >= 33 && ch <=  60) return false;       // This range is okay
    if (ch >= 62 && ch <= 126) return false;       // So is this one
    
    if (ch == SPACE || ch == TAB)                  // If whitespace
      {
        if (lineEnd)                               // If at line end
          return true;                             // Must be encoded
        else                                       // Not at line end
          return false;                            // Can be itself
      }
    else                                           // Not whitespace
      {      
        return true;                               // Must be encoded
      }  
  }                                                // End charEncodeNeeded()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::charHexDigit()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure returns true if a character is a valid hexidecimal digit,
false if not.                                                               */

bool PlainToQPAdapter::charHexDigit(char ch)       // Begin charHexDigit()
  {
    if (ch >= '0' && ch <= '9') return true;       // 0-9
    if (ch >= 'a' && ch <= 'f') return true;       // a-f
    if (ch >= 'A' && ch <= 'F') return true;       // A-F
    
    return false;                                  // Not a hex digit
  }                                                // End charHexDigit()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::charOutput()                                      ***/
/*                                                                          */
/****************************************************************************

This procedure takes 'ch' as input, encodes it or not based on the 'encode'
bool, stores it to 'str', and returns the number of bytes it added.  It
won't add nulls to the string.                                              */

uint32 PlainToQPAdapter::charOutput(BString* str,  // Begin charOutput()
 uchar ch, bool encode)
  {
    char    buf[8];                                // Output buffer
    uint32  rval;                                  // Total added
    
    if (ch == 0) return 0;                         // Don't add nulls
    
    if (encode)                                    // If encode needed
      {
        sprintf(buf, "=%02X", ch);                 // Do it like this
        rval = 3;                                  // That's three bytes
      }
    else                                           // No encode needed
      {
        buf[0] = ch;                               // Put it here
        buf[1] = 0;                                // Terminate string
        rval   = 1;                                // Just one byte
      }
      
    str->Append(buf);                              // Add it
    return rval;                                   // Return added length
  }                                                // End charOutput()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::convertToQP()                                     ***/
/*                                                                          */
/****************************************************************************

This procedure loops through an input line which has been converted to the
destination character set and outputs it as a buffer of one or more lines
in quoted-printable format.                                                 */

void PlainToQPAdapter::convertToQP(BString* in,    // Begin convertToQP()
 BString* out)
  {
    char    ch, next;                              // Two chars from input
    bool    encode;                                // Encode needed?
    uint32  i;                                     // Loop counter
    bool    chLineEnd, lineEnd;                    // Line almost over?
    uint32  lineLen;                               // Line length so far
    uint32  test;                                  // Test line len
    
    for (i = 0, lineLen = 0; ; i++)                // Loop through input
      {
        ch = in->ByteAt(i);                        // Get next byte
        
        if (ch != 0)                               // If not at line end
          next = in->ByteAt(i + 1);                // Get next char
        else                                       // At line end
          next = 0;                                // Clear next char  
        
        test = lineLen;                            // This much so far
        test += charEncodeLen(ch, true);           // Add ch, worst case
        test += charEncodeLen(next, true);         // Add next, worst case
        if (next) test += 1;                       // Might have to add '='
        
        if (!next || test > lineLenMax)            // No next char, won't fit
          {
            lineEnd = true;                        // About to end line
            chLineEnd = !next;                     // Next char means '=' end
            encode=charEncodeNeeded(ch, chLineEnd);// Figure out encoding
          }  
        else                                       // Still a ways to go
          {
            lineEnd = false;                       // Line won't end
            chLineEnd = false;                     // Char will not end line
            
            if (ch == '=' && charHexDigit(next))   // If '=XX' coming
              encode = true;                       // Definitely encode it
            else                                   // No '=XX' coming
              encode=charEncodeNeeded(ch,chLineEnd);// Figure it out
          }  
          
        lineLen += charOutput(out, ch, encode);    // Output this char
        
        if (lineEnd)                               // If it's time
          {
            if (next) out->Append("=");            // Need soft line break?
            out->Append("\r\n");                   // Terminate the line
            lineLen = 0;                           // On to next line
          }
          
        if (!ch || !next) break;                   // If all chars done
      }                                            // End input loop
  }                                                // End convertToQP()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::fill()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure fills the storage buffer with one line from the BDataIO.  It
converts the input from UTF8 to the destination character set then converts
to quoted-printable.  It returns true if there is something in the buffer
to process, false if we are finished.                                       */

bool PlainToQPAdapter::fill(void)                  // Begin fill()
  {
    BString   encode;                              // Intermediary output
    BString   line;                                // Line from file
    int32     lineLen;                             // Line length
    char*     ptr;                                 // Output pointer
    int32     ptrLen;                              // How much is there
    status_t  rval;                                // Convert return value
    int32     state = 0;                           // Conversion state
    
    buf.SetTo(NULL);                               // Clear existing stuff
    bufLen = 0;                                    // Clear length
    bufOffset = 0;                                 // Clear offset
    
    if (!readLine(&line)) return false;            // Get next input line
    lineLen = line.Length();                       // Get length in bytes
    
    ptrLen = lineLen + (lineLen / 2) + 10;         // Set dest buffer length
    ptr = encode.LockBuffer(ptrLen);               // Prepare for output
    if (!ptr) return false;                        // If it failed
    
    if (lineLen == 0)                              // If nothing to do
      {
        rval = B_OK;                               // Pretend it worked
        ptrLen = 0;                                // Nothing collected
      }
    else                                           // Bytes to convert
      {  
        rval = convert_from_utf8(encoding,         // Convert to dest charset
         line.String(), &lineLen,
         ptr, &ptrLen, &state);
      }   
     
    if (rval == B_OK) ptr[ptrLen] = 0;             // Must be terminated
    
    encode.UnlockBuffer();                         // Back to normal 
    if (rval != B_OK) return false;                // If conversion failed
    
    convertToQP(&encode, &buf);                    // To quoted-printable
    bufLen = buf.Length();                         // Set new length
    
    return true;                                   // Success
  }                                                // End fill()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::Read()                                            ***/
/*                                                                          */
/****************************************************************************

This is The Big Kahuna!  It returns the next chunk of quoted-printable
text.  When it returns zero or below you are finished.                      */

ssize_t PlainToQPAdapter::Read(void* buffer,       // Begin Read()
 size_t size)
  {
    size_t   bufSize;                              // How much in buffer?
    ssize_t  bytes = 0;                            // Total bytes output
    uint32   len;                                  // Bytes to copy
    uint8*   ptr = (uint8*) buffer;                // Where to write to
    size_t   remain = size;                        // Space left for caller
    
    ASSERT(buffer && size >= 1);                   // Validate buffer
    
    while (remain >= 1)                            // Copy until full
      {
        if (bufOffset >= bufLen)                   // If buffer empty
          if (!fill()) break;                      // Fill it
          
        bufSize = bufLen - bufOffset;              // Remaining in buffer
        
        if (remain >= bufSize)                     // If it will all fit
          len = bufSize;                           // Copy it all
        else                                       // Won't all fit
          len = remain;                            // Copy all that fits
        
        memcpy(ptr, &buf[bufOffset], len);         // Copy for caller
        
        bufOffset += len;                          // Go past copied bytes
        ptr += len;                                // And in caller too
        bytes += len;                              // This much more
        remain -= len;                             // Less to go
      }
    
    return bytes;                                  // There's yer byte count
  }                                                // End Read()


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::readLine()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure reads the next line from the input into 'str'.  It returns
true if it collected a line, false if end of input has been reached.        */

bool PlainToQPAdapter::readLine(BString* str)      // Begin readLine()
  {
    ssize_t  bytes;                                // Bytes read
    char     ch;                                   // Byte from input
    ssize_t  total = 0;                            // Total bytes read
    
    while (true)                                   // Loop to read chars
      {
        bytes = source->Read(&ch, 1);              // Read next char
        if (bytes <= 0) break;                     // If end of input
        
        ASSERT(bytes == 1);                        // Should be just one
        total += bytes;                            // Update total
        
        if (ch == 0) ch = SPACE;                   // Don't allow nulls!
        
        if (ch == CR || ch == LF)                  // If line end reached
          {
            if (ch == CR)                          // If it's CR
              {
                bytes = source->Read(&ch, 1);      // Get LF after it
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
/***  PlainToQPAdapter::test()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure runs all the test functions.                                 */

#ifdef TEST_SCAFFOLDING                            // If testing
void PlainToQPAdapter::test(void)                  // Begin test()
  {
    testRead();                                    // Run test cases
  }                                                // End test()
#endif                                             // End testing

 
/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::testRead()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure runs test cases against the class.                           */

#ifdef TEST_SCAFFOLDING                            // If testing
void PlainToQPAdapter::testRead(void)              // Begin testRead()
  {
    uint8                  buf[10000];             // Final decoded binary
    ssize_t                bytes;                  // Bytes from adapter
    const TESTDATA*        data;                   // Test case data
    int32                  final;                  // Final size
    bool                   finish;                 // About to finish
    uint32                 i;                      // Loop counter
    BMemoryIO*             io;                     // Positioner dealie
    uint32                 len;                    // Data length
    uint8*                 ptr;                    // Where to decode to
    PlainToQPAdapter*      qp;                     // Test adapter
    
    for (i = 0; testData[i].in; i++)               // Loop for test cases
      {
        data = &testData[i];                       // Get this pointer
        len = strlen(data->in);                    // Get input length
        memset(buf, 0, sizeof (buf));              // Wipe target bytes
        
        io = new BMemoryIO(data->in, len);         // Create positioner
        
        qp = new PlainToQPAdapter(io,              // Create and "own" io
         data->encoding);
        
        final  = 0;                                // Clear total bytes
        finish = false;                            // Not finished
        
        for (ptr = buf; ; ptr += data->bufferSize) // Loop to decode
          {
            bytes = qp->Read(ptr,                  // Read next chunk
             data->bufferSize);
             
            ASSERT(bytes <= data->bufferSize);     // Not too much, please
            
            if (finish)                            // If finished
              ASSERT(bytes < 0);                   // Should get nothing
            
            if (bytes < data->bufferSize)          // If buffer didn't fill
              finish = true;                       // Almost finished
            
            if (!finish)                           // If not finished
              ASSERT(bytes == data->bufferSize);   // Fill buffer, please
            
            if (bytes <= 0)                        // If all done
              {
                ASSERT(finish);                    // Should be set
                break;                             // Exit the loop
              }
            
            final += bytes;                        // This many more
          }
          
        ASSERT(final <= (int32)(sizeof (buf)));    // Don't overshoot
        ASSERT(final == (int32)strlen(data->out)); // Exactly this long
        
        ASSERT(!memcmp(data->out, buf,             // Verify contents
         strlen(data->out)));
        
        delete qp;                                 // Get rid of adapter
        qp = NULL;                                 // Invalidate pointer
      }
  }                                                // End testRead()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  PlainToQPAdapter::Write()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure is unimplemented and is only here because all the other
adapters have it.  It serves about the same function as your appendix.      */

ssize_t PlainToQPAdapter::Write(const              // Begin Write()
 void* , size_t )
  {
    return B_ERROR;                                // Aaaa-UUUUUUUUUU-gaah
  }                                                // End Write()


/****************************************************************************/
/*                                                                          */
/***  main()                                                              ***/
/*                                                                          */
/****************************************************************************

This main() is here only for testing purposes.                              */

#ifdef TEST_SCAFFOLDING                            // If testing
int main(void)                                     // Begin main()
  {
    PlainToQPAdapter::test();                      // Test the class
  }                                                // End main()
#endif                                             // End testing
 
