// UuencodeToRawAdapter.cpp -- UUEncoded text to binary
// by Allen Brunson  January 8, 2001

#include <ctype.h>           // Character classification
#include <stdio.h>           // sprintf(), etc.

#include <Debug.h>           // ASSERT() macro
#include <OS.h>              // Basic OS defines
#include <DataIO.h>          // Memory buffer thingie
#include <String.h>          // String buffer
#include "UuencodeToRawAdapter.h"


/****************************************************************************/
/*                                                                          */
/***  Test data                                                           ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // If testing

const uint8  binary[] = {5, 4, 3, 2, 1, 0};        // Final binary

static const TESTDATA  testData[] =                // Data for tests
  {
    {                                              // Test case  0
      "begin 644 boob\r\n"                         // Text version
      "&0F5)" /* illegal char-> */ "\xFE" "1$4*\r\n"
      "`\r\n",
   // "end\r\n",   // Not terminated!  But it is accepted
      
      (const uint8*) "BeIDE\n",                    // Binary version
      
      "boob",                                      // Recovered filename
      1000, 6                                      // Buffer, final size
    },
    
    {                                              // Test case  1
      "    \r\n"                                   // Text version
      "begin 644   Binary.Stiff   \r\n"
      "&!00#`@$`"
      "end\r\n"
      "  \r\n",
      
      binary,                                      // Binary version
      
      "Binary.Stiff",                              // Recovered filename
      4, 6                                         // Buffer, final size
    },
    
    {                                              // Test case  2
      "begin 644 foobar.txt\r\n"
      "M0F5)1$4@4WES=&5M($EN8VQU9&4@4&%T:',*+2TM+2TM+2TM+2TM+2TM+2TM\r\n"
      "M+2TM+2TM+2T*\"B]-87)T:6%N+W-O=7)C92]R96PO:&5A9&5R<R]P;W-I>`HO\r\n"
      "M36%R=&EA;B]S;W5R8V4O<F5L+VEN<W1A;&PO:34X-B]D979E;&]P+VAE861E\r\n"
      "\r\n"  // Blank line!  This is accepted
      "M<G,O8W!P\"B]B;V]T+V1E=F5L;W`O;&EB+W@X-@HO36%R=&EA;B]S;W5R8V4O\r\n"
      "M<F5L+W-R8R]I;F,O;65D:6%?<`HO36%R=&EA;B]S;W5R8V4O<F5L+W-R8R]I\r\n"
      "M;F,O:6YT97)F86-E7W`*+TUA<G1I86XO<V]U<F-E+W)E;\"]S<F,O:6YC+W-U\r\n"
      "M<'!O<G1?<`HO36%R=&EA;B]S;W5R8V4O<F5L+W-R8R]I;F,O;W-?<`HO36%R\r\n"
      "M=&EA;B]S;W5R8V4O<F5L+W-R8R]T<F%C:V5R\"B]-87)T:6%N+W-O=7)C92]R\r\n"
      "596PO:&5A9&5R<R]I;G1E<F9A8V4*\r\n"
      "`\n"           // UNIX/BeOS line end!  It works
      "end\r\n",
      
      (const uint8*)                               // Binary version
      "BeIDE System Include Paths\n"
      "--------------------------\n"
      "\n"
      "/Martian/source/rel/headers/posix\n"
      "/Martian/source/rel/install/i586/develop/headers/cpp\n"
      "/boot/develop/lib/x86\n"
      "/Martian/source/rel/src/inc/media_p\n"
      "/Martian/source/rel/src/inc/interface_p\n"
      "/Martian/source/rel/src/inc/support_p\n"
      "/Martian/source/rel/src/inc/os_p\n"
      "/Martian/source/rel/src/tracker\n"
      "/Martian/source/rel/headers/interface\n",
      
      "foobar.txt",                                // Recovered filename
      10, 381                                      // Buffer, final size
    },
    
    {NULL, NULL, NULL, 0, 0}                       // End of the list
  };

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::UuencodeToRawAdapter()                        ***/
/*                                                                          */
/****************************************************************************

This is the class constructor.                                              */

UuencodeToRawAdapter::UuencodeToRawAdapter(        // UuencodeToRawAdapter()
 BDataIO* source, bool owning)
  {
    this->buf           = source;                  // Save BDataIO pointer
    this->lineBytes     =  0;                      // No bytes in line yet
    this->lineOffset    = -1;                      // Not in the line yet
    this->mode          = mStart;                  // Set starting mode
    this->overflowCount = 0;                       // No overflow yet
    this->owning        = owning;                  // Save owning flag
    
    memset(this->overflow, 0,                      // Wipe overflow bytes
     sizeof (this->overflow));
  }                                                // UuencodeToRawAdapter()


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::~UuencodeToRawAdapter()                       ***/
/*                                                                          */
/****************************************************************************

This is the class destructor.                                               */

UuencodeToRawAdapter::~UuencodeToRawAdapter(void)  // ~UuencodeToRawAdapter()
  {
    if (owning)                                    // If we own it
      {
        delete buf;                                // Delete it
        buf = NULL;                                // Invalidate the pointer
      }  
  }                                                // ~UuencodeToRawAdapter()


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::CharToDigit()                                 ***/
/*                                                                          */
/****************************************************************************

This procedure takes a character from the UUENCODE alphabet and converts
it into a digit in the range 0 to 63.                                       */

int8 UuencodeToRawAdapter::CharToDigit(int8 ch)    // Begin CharToDigit()
  {
    if (ch == '`') ch = 0x20;                      // '`' maps to zero
    ch -= 0x20;                                    // Char to digit
    return ch;                                     // Whoomp!  There it is
  }                                                // End CharToDigit()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::Filename()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure retrieves the filename that was found in the input stream.   */

void UuencodeToRawAdapter::Filename(BString* str)  // Begin Filename()
  {
    if (!str) return;                              // Stupidity check
    str->SetTo(filename);                          // Hello, sailor
  }                                                // End Filename()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::Mode()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure returns the mode the adapter is currently in: mStart means
that we haven't gotten the 'start 666 filename' line yet, mData means we
are reading through the data lines, mEnd means the 'end' line has been
found so we're done.                                                        */

UuencodeToRawAdapter::MODE                         // Begin Mode()
 UuencodeToRawAdapter::Mode(void)
  {
    return mode;                                   // There you go
  }                                                // End Mode()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::Read()                                        ***/
/*                                                                          */
/****************************************************************************

This is the all-important function which returns the next chunk of binary
data.  It will return 1 or greater until all data is exhausted when it will
return -1.                                                                  */

ssize_t UuencodeToRawAdapter::Read(void* buffer,   // Begin Read()
 size_t size)
  {
    ssize_t  bytes;                                // Total bytes read
    
    ASSERT(buffer && size >= 4);                   // Don't give a tiny buffer
    
    while (true)                                   // Loop until stuff done
      switch (mode)                                // Decision on mode
        {
          case mStart:                             // Start mode
            if (!ReadFilename()) return -1;        // Read filename
            break;
          
          case mData:                              // Reading data
            bytes = ReadData(buffer, size);        // Read next data chunk
            if (bytes >= 1) return bytes;          // Return if data read
            break;
      
          case mEnd:                               // Finished
            return 0;                              // You are over, dude
          
          default:                                 // Unknown mode
            ASSERT(false);                         // Blam!
            return -1;                             // Error
        }
  }                                                // End Read()


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::ReadData()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure reads as many data lines as necessary out of the input to
fill the client's buffer.                                                   */

ssize_t UuencodeToRawAdapter::ReadData(void*       // Begin ReadData()
 buffer, size_t size)
  {
    ssize_t      bytes = 0;                        // Total bytes output
    int32        len;                              // Length of string
    int32        next;                             // Bytes on the line
    uint8*       ptr = (uint8*) buffer;            // Where to write
    int32        remain = size;                    // Bytes remaining
    const char*  str;                              // String pointer
    
    ASSERT(mode == mData);                         // Verify mode
    
    while (remain >= 1)                            // Loop until buffer full
      {
        if (lineOffset < 0)                        // If time to read
          {
            lineOffset = 0;                        // Clear offset
            lineBytes = 0;                         // Clear byte count
          
            if (!ReadLine())                       // Read next line
              {
                mode = mEnd;                       // No more lines
                return bytes;                      // Return final count
              }
              
            len = line.Length() - 1;               // Length minus length byte
            if (len < 0) len = 0;                  // If line is blank
            str = line.String();                   // Get string pointer
        
            if ((str[0] == 'e' || str[0] == 'E') &&// Starts with 'e'
             (str[1] == 'n' || str[1] == 'N') &&   // Next is 'n'
             (str[2] == 'd' || str[2] == 'D') &&   // Next is 'd'
             (isspace(str[3]) || str[3] == 0))     // Followed by whitespace
              {
                mode = mEnd;                       // You are done
                return bytes;                      // Return final count
              }
        
            lineBytes = CharToDigit(str[0]);       // Total bytes on line
        
            if (lineBytes < 0) lineBytes = 0;      // Not too small
            if (lineBytes > len) lineBytes = len;  // Not too big
          }                                        // End line read
        
        str = line.String();                       // Get string pointer
        next = ReadDataLine(&str[1], ptr, remain); // Output the bytes
        
        bytes += next;                             // More bytes written
        ptr += next;                               // Pointer goes forward
        remain -= next;                            // Fewer left to go
      }                                            // End line read loop
    
    return bytes;                                  // Return total bytes
  }                                                // End ReadData()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::ReadDataLine()                                ***/
/*                                                                          */
/****************************************************************************

Once a single line has been picked out of the input stream this procedure
decodes the text to the caller's output buffer.  Since the caller's buffer
might not be a multiple of three some of the bytes might have to be saved
in overflow.                                                                */

ssize_t UuencodeToRawAdapter::ReadDataLine(const   // Begin ReadData()
 char* line, void* buffer, size_t size)
  {
    uint32  bytes = 0;                             // Total bytes output
    int8    ch;                                    // Char from line
    bool    end = false;                           // Line end reached?
    uint32  out;                                   // Total output
    uint8*  ptr = (uint8*) buffer;                 // Where to put the bytes
    uint32  remain;                                // Remaining buffer space
    uint8   temp[4];                               // Temp buffer
    uint32  word = 0;                              // Four-to-three conversion
    
    
    //*
    //***  Verify input parameters
    //*
    
    ASSERT(lineOffset >= 0);                       // Should have data
    
    
    //*
    //***  If stuff left over from last time, use it
    //*
    
    if (overflowCount)                             // If there's old stuff
      {
        ASSERT(size >= overflowCount);             // It better fit
        memcpy(ptr, overflow, overflowCount);      // Spit it out
        
        ptr += overflowCount;                      // Move forward
        bytes += overflowCount;                    // Wrote a bit more
        overflowCount = 0;                         // It's gone
      }
    
    
    //*
    //***  Loop through chars in the line
    //*
    
    for ( ; !end && bytes < size; lineOffset++)    // Loop on line bytes
      {
        ch = line[lineOffset];                     // Get next input char
        
        if (lineBytes <= 0 || ch == 0)             // If line end reached
          {
            end = true;                            // Set the flag
            lineOffset = -10;                      // STAY negative, dammit
          }  
        
        if (!end)                                  // End not yet reached
          {
            ch = CharToDigit(ch);                  // Text to digit
            
            if (ch >= 0 && ch <= 63)               // If in range
              word = wordAdd(word, ch);            // Add this character
          }    
        
        if (end || wordFull(word))                 // If time to output
          {
            if ((size - bytes) >= 3)               // If it will all fit
              {
                out = wordOutput(ptr, word);       // Directly into buffer
                ptr += out;                        // Pointer moves forward
                bytes += out;                      // A few more output
              }  
            else                                   // Might not all fit
              {
                out = wordOutput(temp, word);      // Into temp buffer
                remain = size - bytes;             // Size remaining
                
                if (out <= remain)                 // It fits after all
                  {
                    memcpy(ptr, temp, out);        // Spit it out
                    ptr += out;                    // Move forward
                    bytes += out;                  // A few more output
                  }
                else                               // Doesn't all fit
                  {
                    memcpy(ptr, temp, remain);     // Copy all that fits
                    ptr += remain;                 // Move forward
                    bytes += remain;               // This much more
                    
                    overflowCount = out - remain;  // Compute overflow
                    
                    memcpy(overflow, temp + remain,// Save in overflow buffer
                     overflowCount);
                  }
              }                                    // End 'might not fit'
              
            word = 0;                              // Clear word content
            lineBytes -= out;                      // That many fewer
          }                                        // End three-byte output
      }                                            // End char loop
    
    return bytes;                                  // Return bytes output
  }                                                // End ReadDataLine()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::ReadFilename()                                ***/
/*                                                                          */
/****************************************************************************

This procedure reads through the input until it gets to a 'begin' line
and reads the filename.  It returns true on success, false on failure.      */

bool UuencodeToRawAdapter::ReadFilename(void)      // Begin ReadFilename()
  {
    char  word[300];                               // Word from buffer
    
    ASSERT(mode == mStart);                        // Verify mode
    
    while (true)                                   // Loop to get filename
      {
        if (!ReadLine()) return false;             // Get next line
        
        strWord(&line, 0, word, sizeof (word));    // Get first word
        if (strcasecmp(word, "begin")) continue;   // If not valid start
        
        strWord(&line, 1, word, sizeof (word));    // Get attributes
        if (!word[0]) continue;                    // If not there
        
        strWord(&line, 2, word, sizeof (word));    // Get filename
        if (!word[0]) continue;                    // If not there
        
        filename.SetTo(word);                      // Save filename
        mode = mData;                              // Ready for data
        return true;                               // Success
      }
  }                                                // End ReadFilename()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::ReadLine()                                    ***/
/*                                                                          */
/****************************************************************************

This internal function reads the next available line from the source
buffer.  It returns true on success, false if no more data is available.    */

bool UuencodeToRawAdapter::ReadLine(void)          // Begin ReadLine()
  {
    ssize_t  bytes = 0;                            // Total bytes read
    char     ch;                                   // Next char from buffer
    ssize_t  count;                                // Count on this read
    
    line.SetTo(NULL);                              // Clear existing line
    
    while (true)                                   // Loop to read bytes
      {
        count = buf->Read(&ch, 1);                 // Read from buffer
        if (count <= 0) break;                     // If nothing read
        ASSERT(count == 1);                        // Should be one
        
        if (ch == 0) ch = SPACE;                   // Don't allow NULLs!
        bytes += 1;                                // Add current byte
        
        if (ch == CR || ch == LF)                  // If line end reached
          {
            if (ch == CR) buf->Read(&ch, 1);       // Discard final LF
            ASSERT(ch == LF);                      // It better be this
            break;                                 // Stop the loop
          }
          
        line.Append(ch, 1);                        // In you go
      }                                            // End read loop
    
    strStripLeft(&line);                           // Remove left whites
    strStripRight(&line);                          // Remove right whites
    return (bytes >= 1);                           // If bytes were read
  }                                                // End ReadLine()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::strStrip()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure squishes all whitespace out of a BString.                    */

void UuencodeToRawAdapter::strStrip(BString* str)  // Begin strStrip()
  {
    char   ch;                                     // Char from string
    int32  i;                                      // Loop counter
    
    for (i = 0; ; i++)                             // Loop through chars
      {
        ch = str->ByteAt(i);                       // Get next char
        if (ch == 0) break;                        // Quit at the end
        
        if (isspace(ch))                           // If it's whitespace
          {
            str->Remove(i, 1);                     // Kill whitespace
            i--;                                   // Re-do this index
          }  
      }                                            // End char loop
  }                                                // End strStrip()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::strStripLeft()                                ***/
/*                                                                          */
/****************************************************************************

This procedure removes whitespace from the beginning of a BString.          */

void UuencodeToRawAdapter::strStripLeft(BString*   // Begin strStripLeft()
 str)
  {
    char  ch;                                      // Char from string
    
    while (true)                                   // Loop over start chars
      {
        ch = str->ByteAt(0);                       // Get first char
        if (!isspace(ch)) break;                   // Stop at first non-white
        str->Remove(0, 1);                         // Kill whitespace char
      }                                            // End char loop
  }                                                // End strStripLeft()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::strStripRight()                               ***/
/*                                                                          */
/****************************************************************************

This procedure removes whitespace from the end of a BString.                */

void UuencodeToRawAdapter::strStripRight(BString*  // Begin strStripRight()
 str)
  {
    char   ch;                                     // Char from string
    int32  len;                                    // String length
    
    while (true)                                   // Loop over end chars
      {
        len = str->Length();                       // Get length
        if (len <= 0) break;                       // Stop if no more
        
        ch = str->ByteAt(len - 1);                 // Get last char
        if (!isspace(ch)) break;                   // Stop at first non-white
        str->Remove(len - 1, 1);                   // Kill whitespace char
      }                                            // End char loop
  }                                                // End strStripRight()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::strWord()                                     ***/
/*                                                                          */
/****************************************************************************

This procedure gets the Nth "word" out of a string, where a "word" is any
number of characters delimited with whitespace.  It returns true if it
found the word, false if not.                                               */

bool UuencodeToRawAdapter::strWord(BString* str,   // Begin strWord()
 uint32 num, char* word, uint32 wordSize)
  {
    uint32       n, s, w;                          // Loop counters
    const char*  src = str->String();              // String buffer
    
    for (s = 0, n = 0; ; n++)                      // Loop to find Nth word
      {
        word[0] = 0;  w = 0;                       // Clear current word
        while (src[s] && isspace(src[s])) s++;     // Past leading whites
        
        for ( ; src[s] && !isspace(src[s]); s++)   // Loop for word chars
          if (w < (wordSize - 1))                  // If this char fits
            word[w++] = src[s];                    // Put it in
            
        word[w] = 0;                               // Terminate word
        if (word[0] && n == num) return true;      // If word found
        if (!src[s]) {word[0] = 0; return false;}  // If string finished
      }
  }                                                // End strWord()
  
  
/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::test()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure tests the class.                                             */

#ifdef TEST_SCAFFOLDING                            // If testing
void UuencodeToRawAdapter::test(void)              // Begin test()
  {
    testRead();                                    // Test data reading
    testWord();                                    // Test word finder
  }                                                // End test()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::testRead()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure reads the test buffer and checks the result.                 */

#ifdef TEST_SCAFFOLDING                            // If testing
void UuencodeToRawAdapter::testRead(void)          // Begin testRead()
  {
    uint8                  buf[10000];             // Final decoded binary
    ssize_t                bytes;                  // Bytes from adapter
    const TESTDATA*        data;                   // Test case data
    BString                filename;               // Recovered filename
    int32                  final;                  // Final size
    bool                   finish;                 // About to finish
    uint32                 i;                      // Loop counter
    BMemoryIO*             io;                     // Positioner dealie
    uint32                 len;                    // Data length
    uint8*                 ptr;                    // Where to decode to
    UuencodeToRawAdapter*  uudecode;               // Test adapter
    
    for (i = 0; testData[i].text; i++)             // Loop for test cases
      {
        data = &testData[i];                       // Get this pointer
        len = strlen(data->text);                  // Get text length
        memset(buf, 0, sizeof (buf));              // Wipe target bytes
        
        io = new BMemoryIO(data->text, len);       // Create positioner
        uudecode = new UuencodeToRawAdapter(io);   // Create and "own" io
        
        final  = 0;                                // Clear total bytes
        finish = false;                            // Not finished
        
        for (ptr = buf; ; ptr += data->bufferSize) // Loop to decode
          {
            bytes = uudecode->Read(ptr,            // Read next chunk
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
        ASSERT(final == data->binaryLen);          // Exactly this long
        
        uudecode->Filename(&filename);             // Get the filename
        ASSERT(!filename.Compare(data->filename)); // Check it
        
        ASSERT(!memcmp(data->binary, buf,          // Verify contents
         data->binaryLen));
        
        delete uudecode;                           // Get rid of adapter
        uudecode = NULL;                           // Invalidate pointer
      }
  }                                                // End testRead()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::testWord()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure tests the word-picker function.                              */

#ifdef TEST_SCAFFOLDING                            // If testing
void UuencodeToRawAdapter::testWord(void)          // Begin testWord()
  {
    bool       rval;                               // Return value
    BString    str;                                // Buffer
    char       word[100];                          // Final word
    
    str.SetTo(NULL); word[0] = 1;                  // Clear it
    rval = strWord(&str, 0, word, sizeof (word));  // Try to get word
    ASSERT(!rval && !word[0]);                     // Shouldn't be there
    
    str.SetTo(" \t  This \t  woog  \t ");          // This is the test
    
    rval = strWord(&str, 0, word, 3);              // Not enough space
    ASSERT(rval && !strcmp(word, "Th"));           // Only a bit
    
    rval = strWord(&str, 0, word, sizeof (word));  // Get word zero
    ASSERT(rval && !strcmp(word, "This"));         // Here it is
    
    rval = strWord(&str, 1, word, sizeof (word));  // Get word one
    ASSERT(rval && !strcmp(word, "woog"));         // Here it is
    
    rval = strWord(&str, 2, word, sizeof (word));  // Get word two
    ASSERT(!rval && !word[0]);                     // It isn't there
  }                                                // End testWord()
#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::wordAdd()                                     ***/
/*                                                                          */
/****************************************************************************

This procedure adds another character to a word and returns the result.     */

uint32 UuencodeToRawAdapter::wordAdd(uint32 word,  // Begin wordAdd()
 char ch)
  {
    uint32  count;                                 // Current byte count
    
    ASSERT(ch >= 0 && ch <= 63);                   // Verify char range
    count = (word >> 24);                          // Get current count
    
    switch (count)                                 // Decision on char count
      {
        case 0: word |= (ch << 18); break;         // Zero chars
        case 1: word |= (ch << 12); break;         // One char
        case 2: word |= (ch <<  6); break;         // Two bytes
        case 3: word |= (ch);       break;         // Three bytes
        default: {ASSERT(false); return word;}     // Shouldn't happen
      }
    
    count++;                                       // That's one more
    word &= 0x00FFFFFFL;                           // Zero the top bits
    word |= (count << 24);                         // Set the new count
    
    return word;                                   // There's yer answer
  }                                                // End wordAdd()


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::wordFull()                                    ***/
/*                                                                          */
/****************************************************************************

This procedure returns true if a decode word is full-up, false if not.      */

bool UuencodeToRawAdapter::wordFull(uint32 word)   // Begin wordFull()
  {
    uint32  count;                                 // Current byte count
    
    count = (word >> 24);                          // Get current count
    return (count >= 4);                           // Four is full
  }                                                // End wordFull()


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::wordOutput()                                  ***/
/*                                                                          */
/****************************************************************************

This procedure outputs a four-char group as three binary output bytes.  It
assumes the buffer given has at least enough room to hold three bytes and
returns the number of characters it added.                                  */

uint32 UuencodeToRawAdapter::wordOutput(void* buf, // Begin wordOutput()
 uint32 word)
  {
    uint8   ch;                                    // Next byte value
    uint32  count;                                 // Total chars
    uint32  i;                                     // Loop counter
    uint32  output = 0;                            // Total output bytes
    uint8*  ptr = (uint8*) buf;                    // Where to output
    
    count = (word >> 24);                          // Get char count
    if (count == 0) return 0;                      // Exit if empty
    
    ASSERT(count >= 2 && count <= 4);              // Verify char count
    
    switch (count)                                 // Decision on char count
      {
        case 2: output = 1; break;                 // 2 chars: 1 output byte
        case 3: output = 2; break;                 // 3 chars: 2 output bytes
        case 4: output = 3; break;                 // 4 chars: 3 output bytes
      }
    
    for (i = 0; i < output; i++)                   // Up to three bytes
      {
        ch     = (word & 0x00FF0000) >> 16;        // Get this byte
        word   = word << 8;                        // To next byte
        ptr[i] = ch;                               // Set this value
      }                                            // End output loop
      
    return i;                                      // Return byte count
  }                                                // End wordOutput()


/****************************************************************************/
/*                                                                          */
/***  UuencodeToRawAdapter::Write()                                       ***/
/*                                                                          */
/****************************************************************************

It is an error to "write" into read-only UUENCODEd data so this function
always returns an error.                                                    */

ssize_t UuencodeToRawAdapter::Write(const          // Begin Write()
 void* , size_t )
  {
    return B_ERROR;
  }                                                // End Write()
  
  
/****************************************************************************/
/*                                                                          */
/***  main()                                                              ***/
/*                                                                          */
/****************************************************************************

This main() runs the test cases against the code.                           */

#ifdef TEST_SCAFFOLDING                            // If testing
int main(void)                                     // Begin main()
  {
    UuencodeToRawAdapter::test();                  // Call test function
    return 0;                                      // Have to return something
  }                                                // End main()
#endif                                             // End testing


