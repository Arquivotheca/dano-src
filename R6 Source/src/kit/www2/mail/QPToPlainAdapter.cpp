// QPToPlainAdapter.cpp -- Quoted-printable to UTF8 text
// Started by somebody else, finished by Allen Brunson  January 19, 2001

#include <ctype.h>
#include <stdio.h>
#include <Debug.h>
#include <String.h>
#include <UTF8.h>
#include "QPToPlainAdapter.h"


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
      "\r\ninput=A9  ",                            // Input text
      "\r\ninput" B_UTF8_COPYRIGHT "\r\n",         // Output text
      B_ISO1_CONVERSION, 1                         // Charset, buffer size
    },
    
    {                                              // Test case  2
      "line1\t =20\r\nline2=09 \r\n",              // Input text
      "line1\t  \r\nline2\t\r\n",                  // Output text
      B_ISO2_CONVERSION, 6                         // Charset, buffer size
    },
    
    {                                              // Test case  3
      "bad: =3DBB\r\n",                            // Input text
      "bad: =BB\r\n",                              // Output text
      B_ISO1_CONVERSION, 10                        // Charset, buffer size
    },
    
    {                                              // Test case  4
      "0123456789012345678901234567890123456789"   // Input text
      "01234567890123456789012345678901234=\r\n"
      "56789\r\n",
      
      "0123456789012345678901234567890123456789"   // Output text
      "0123456789012345678901234567890123456789"
      "\r\n",
      
      B_ISO2_CONVERSION, 1000                      // Charset, buffer size
    },
    
    {                                              // Test case  5
      "0123456789012345678901234567890123456789"   // Tricky boundary case
      "0123456789012345678901234567890123=\n"
      " =A9\n",
      
      "0123456789012345678901234567890123456789"   // Output text
      "0123456789012345678901234567890123"
      " " B_UTF8_COPYRIGHT "\r\n",
      
      B_ISO1_CONVERSION, 1000                      // Charset, buffer size
    },
    
    {NULL, NULL}                                   // End of the list
  };

#endif


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::QPToPlainAdapter()                                ***/
/*                                                                          */
/****************************************************************************

This is the class constructor.                                              */

QPToPlainAdapter::QPToPlainAdapter(BDataIO* source,// Begin QPToPlainAdapter()
 uint32 encoding, bool owning)
  {
    this->bufOffset = 0;                           // Clear offset
    this->bufLen    = 0;                           // Clear length
    this->encoding  = encoding;                    // Save encoding
    this->owning    = owning;                      // Save owning state
    this->source    = source;                      // Save data source
  }                                                // End QPToPlainAdapter()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::~QPToPlainAdapter()                               ***/
/*                                                                          */
/****************************************************************************

This is the class destructor.                                               */

QPToPlainAdapter::~QPToPlainAdapter(void)          // ~QPToPlainAdapter()
  {
    if (owning)                                    // If we own it
      {
        delete source;                             // Delete it
        source = NULL;                             // Invalidate pointer
      }  
  }                                                // ~QPToPlainAdapter()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::charHexDigit()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure returns true if a character is a valid hexidecimal digit,
false if not.                                                               */

bool QPToPlainAdapter::charHexDigit(char ch)       // Begin charHexDigit()
  {
    if (ch >= '0' && ch <= '9') return true;       // 0-9
    if (ch >= 'a' && ch <= 'f') return true;       // a-f
    if (ch >= 'A' && ch <= 'F') return true;       // A-F
    
    return false;                                  // Not a hex digit
  }                                                // End charHexDigit()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::charOutput()                                      ***/
/*                                                                          */
/****************************************************************************

Once two hex digits from a =XX construct have been picked out of the input
stream this procedure re-constitutes them into a regular byte and outputs
it into the given BString.                                                  */

void QPToPlainAdapter::charOutput(BString* str,    // Begin charOutput()
 char digit1, char digit2)
  {
    char  buf[4];                                  // Built-up string
    int   out;                                     // Final char
    
    buf[0] = digit1;                               // Create hex string
    buf[1] = digit2;
    buf[2] = 0;
    
    sscanf(buf, "%X", &out);                       // Re-constitute
    str->Append((char)out, 1);                     // Stick it in
  }                                                // End charOutput()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::convertFromQP()                                   ***/
/*                                                                          */
/****************************************************************************

This procedure converts quoted-printable text back into "regular" stuff
with the original bytes and line breaks it had before being encoded.        */

void QPToPlainAdapter::convertFromQP(BString* in,  // Begin convertFromQP()
 BString* out)
  {
    char    ch, next, final;                       // Chars from line
    uint32  i;                                     // Loop counter
    
    for (i = 0; ; i++)                             // Loop through input
      {
        ch = in->ByteAt(i);                        // Get next byte
        
        if (ch != 0)                               // If not at the end
          next = in->ByteAt(i + 1);                // Get next byte
        else                                       // At the end
          next = 0;                                // Clear this
        
        if (ch == '=' && charHexDigit(next))       // Possible '=XX' coming
          {
            final = in->ByteAt(i + 2);             // Get last bit
            
            if (charHexDigit(final))               // If it's official
              {
                charOutput(out, next, final);      // Spit it out
                i += 2;                            // Go past next two
                continue;                          // On to next char
              }
          }  
          
        if (!ch || !next)                          // If at input end
          {
            if (ch != '=')                         // If not '='
              {
                if (ch) out->Append(ch, 1);        // Spit out last char
                out->Append("\r\n");               // Terminate the line
              }  
            
            break;                                 // Stop the loop
          }
        else                                       // Not at input end
          {  
            out->Append(ch, 1);                    // Spit it out
          }  
      }                                            // End input loop
  }                                                // End convertFromQP()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::fill()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure fills the storage buffer with one line from the BDataIO.  It
converts the input from the given character to quoted-printable, then to
UTF8.  It returns true if there is something in the buffer to process,
false if we are finished.                                                   */

bool QPToPlainAdapter::fill(void)                  // Begin fill()
  {
    BString   decode;                              // Intermediary output
    int32     decodeLen;                           // Intermediary length
    BString   line;                                // Line from file
    char*     ptr;                                 // Output pointer
    int32     ptrLen;                              // How much is there
    status_t  rval;                                // Convert return value
    int32     state = 0;                           // Conversion state
    
    buf.SetTo(NULL);                               // Clear existing stuff
    bufLen = 0;                                    // Clear length
    bufOffset = 0;                                 // Clear offset
    
    if (!readLine(&line)) return false;            // Get next input line
    
    convertFromQP(&line, &decode);                 // From QP back to "plain"
    decodeLen = decode.Length();                   // Get length in bytes
    line.SetTo(NULL);                              // Free line memory

    ptrLen = decodeLen + (decodeLen / 2) + 10;     // Set dest buffer length
    ptr = buf.LockBuffer(ptrLen);                  // Prepare for output
    if (!ptr) return false;                        // If it failed
    
    if (decodeLen == 0)                            // If nothing to do
      {
        rval = B_OK;                               // Pretend it worked
        ptrLen = 0;                                // Nothing collected
      }
    else                                           // Bytes to convert
      {  
        rval = convert_to_utf8(encoding,           // Convert to UTF8
         decode.String(), &decodeLen,
         ptr, &ptrLen, &state);
      }   
     
    if (rval == B_OK) ptr[ptrLen] = 0;             // Must be terminated
    
    buf.UnlockBuffer();                            // Back to normal 
    if (rval != B_OK) return false;                // If conversion failed
    
    bufLen = buf.Length();                         // Set new length
    return true;                                   // Success
  }                                                // End fill()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::Read()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure gets the next chunk of UTF8 text out of the adapter.  When
it returns zero or below you are finished.                                  */

ssize_t QPToPlainAdapter::Read(void* buffer,       // Begin Read()
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
/***  QPToPlainAdapter::readLine()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure reads the next line from the input into 'str'.  RFC 2045
says you are supposed to ignore whitespace at the end of a quoted-printable
line so I lop it off.  It returns true if it collected a line, false if end
of input has been reached.                                                  */

bool QPToPlainAdapter::readLine(BString* str)      // Begin readLine()
  {
    ssize_t  bytes;                                // Bytes read
    char     ch;                                   // Byte from input
    uint32   len;                                  // Final length
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
      
    while (true)                                   // Loop over end chars
      {
        len = str->Length();                       // Get length
        if (len <= 0) break;                       // Stop if no more
        
        ch = str->ByteAt(len - 1);                 // Get last char
        if (!isspace(ch)) break;                   // Stop at first non-white
        str->Remove(len - 1, 1);                   // Kill whitespace char
      }                                            // End char loop
    
    return (total >= 1);                           // Got a line?    
  }                                                // End readLine()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::test()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure runs all the other test procedures.                          */

#ifdef TEST_SCAFFOLDING
void QPToPlainAdapter::test(void)                  // Begin test()
  {
    testRead();                                    // Read test cases
  }                                                // End test()


/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::testRead()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure runs the test cases against the class.                       */

void QPToPlainAdapter::testRead(void)              // Begin testRead()
  {
    uint8              buf[10000];                 // Final decoded binary
    ssize_t            bytes;                      // Bytes from adapter
    const TESTDATA*    data;                       // Test case data
    int32              final;                      // Final size
    bool               finish;                     // About to finish
    uint32             i;                          // Loop counter
    BMemoryIO*         io;                         // Positioner dealie
    uint32             len;                        // Data length
    uint8*             ptr;                        // Where to decode to
    QPToPlainAdapter*  qp;                         // Test adapter
    
    for (i = 0; testData[i].in; i++)               // Loop for test cases
      {
        data = &testData[i];                       // Get this pointer
        len = strlen(data->in);                    // Get input length
        memset(buf, 0, sizeof (buf));              // Wipe target bytes
        
        io = new BMemoryIO(data->in, len);         // Create positioner
        
        qp = new QPToPlainAdapter(io,              // Create and "own" io
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
#endif

/****************************************************************************/
/*                                                                          */
/***  QPToPlainAdapter::Write()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure is unimplemented and unnecessary.                            */

ssize_t QPToPlainAdapter::Write(const void* ,// Begin Write()
 size_t )
  {
    return B_ERROR;
  }                                                // End Write()


/****************************************************************************/
/*                                                                          */
/***  main()                                                              ***/
/*                                                                          */
/****************************************************************************

This main() is here just for testing purposes.                              */

#ifdef TEST_SCAFFOLDING                            // If testing
int main(void)                                     // Begin main()
  {
    QPToPlainAdapter::test();                      // Test the class
    return 0;                                      // Have to return something
  }                                                // End main()
#endif                                             // End testing

