// KeyedMD5.cpp -- Code for CRAM-MD5 authentification
// Gathered from other sources by Allen Brunson  December 18, 2000

#include <stdio.h>               // sprintf(), etc.
#include <string.h>              // strcpy(), etc.
#include <DataIO.h>
#include <Debug.h>               // ASSERT() macro
#include "KeyedMD5.h"
#include "RawToBase64Adapter.h"
#include "Base64ToRawAdapter.h"
#include "md5.h"


/****************************************************************************/
/*                                                                          */
/***  MD5 test data                                                       ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // Test-only data

static RESPDATA respData[] =
  {
    {
      "tim",
      "tanstaaftanstaaf",
      "PDE4OTYuNjk3MTcwOTUyQHBvc3RvZmZpY2UucmVzdG9uLm1jaS5uZXQ+",
      "dGltIGI5MTNhNjAyYzdlZGE3YTQ5NWI0ZTZlNzMzNGQzODkw"
    },
      
    {
      "charlesbarkley@evilla.com",
      "password",
      "PDEyODM2MS45OTE0MzIzOTZAZXZpbGxhLm1haWwucGFzLmVhcnRobGluay5uZXQ+",
      
      "Y2hhcmxlc2JhcmtsZXlAZXZpbGxhLmNvbSA1ZGNjNTR"
      "jNjlkZDY5NzkyYzI5ZWU5MDhhNjViY2IyYg=="
    },
      
    {NULL, NULL, NULL, NULL}
  };
      
static TESTDATA  testData[] =                      // Structure list
  {
    {
      "",                                          // Test  1
      "D41D8CD98F00B204E9800998ECF8427E"
    },
    
    {
      "a",                                         // Test  2
      "0CC175B9C0F1B6A831C399E269772661"
    },
   
    {
      "abc",                                       // Test  3
      "900150983CD24FB0D6963F7D28E17F72"
    },
    
    {
      "message digest",                            // Test  4
      "F96B697D7CB7938D525A2F31AAF161D0"
    },
    
    {
      "abcdefghijklmnopqrstuvwxyz",                // Test  5
      "C3FCD3D76192E4007DFB496CCA67E13B"
    },
    
    {
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                 // Test  6
      "abcdefghijklmnopqrstuvwxyz0123456789",
      "D174AB98D277D9F5A5611C2C9F419D9F"
    },
    
    {
      "1234567890123456789012345678901234567890"   // Test  7
      "1234567890123456789012345678901234567890",
      "57EDF4A22BE3C955AC49DA2E2107B67A"
    },
    
    {NULL, NULL}                                   // End of the list
  };
  
#endif                                             // End test data
  

/****************************************************************************/
/*                                                                          */
/***  base64decode()                                                      ***/
/*                                                                          */
/****************************************************************************

This procedure decodes NULL-terminated base64 text from 'in' to buffer
'out'.  It expects that 'in' will be padded with equals signs on the right
so that the length will be a multiple of four.                               */

ssize_t base64decode(const char* in, char* out,    // Begin base64decode()
 uint32 outSize)
  {
    BMallocIO*          buffer = new BMallocIO;    // Standard I/O buffer
    Base64ToRawAdapter  base64(buffer, true);      // "Own" the buffer
    uint32              len = strlen(in);          // Total bytes to decode
    ssize_t             size;                      // Final size
    
    ASSERT((len % 4) == 0);                        // Length a multiple of four
    ASSERT(out && outSize >= 10);                  // Must have this buffer
    
    buffer->Write(in, len);                        // Add string to buffer
    buffer->Seek(0, SEEK_SET);                     // Reset position

    size = base64.Read(out, outSize - 1);          // Read it back out
    out[size] = 0;                                 // Null-terminate it
    
    return size;                                   // Return final size
  }                                                // End base64decode()


/****************************************************************************/
/*                                                                          */
/***  base64encode()                                                      ***/
/*                                                                          */
/****************************************************************************

This procedure takes in the NULL-terminated buffer 'in' and base64 encodes
it to 'out'.                                                                */

ssize_t base64encode(const char* in, char* out,    // Begin base64encode()
 uint32 outSize)
  {
    BMallocIO*          buffer = new BMallocIO;    // Simple I/O buffer
    RawToBase64Adapter  base64(buffer, true);      // "Own" the buffer
    uint32              len = strlen(in);          // Length of input
    ssize_t             size = 0;                  // Final size
    
    ASSERT(out && outSize >= 10);                  // Buffer must exist
    base64.SetBreakLines(false);                   // Don't break lines please
    
    buffer->Write(in, len);                        // Send input to buffer
    buffer->Seek(0, SEEK_SET);                     // Reset pointer

    size = base64.Read(out, outSize - 1);          // Read it back out
    ASSERT(size < (ssize_t)outSize);               // Don't overflow buffer
    out[size] = 0;                                 // Null-terminate it

    return size;                                   // Return final size
  }                                                // End base64encode()


/****************************************************************************/
/*                                                                          */
/***  hmac_challenge_response()                                           ***/
/*                                                                          */
/****************************************************************************

This procedure takes in the original base64-encoded string sent by the
server and the user info and returns the base64-encoded string to send back
for CRAM-MD5 authorization.                                                 */

void hmac_challenge_response(const char* challenge,// hmac_challenge_response()
 const char* username, const char* password,
 char* response, uint32 responseSize)
  {
    uint8  digest[16];                             // Raw 16-byte digest
    char   final[201];                             // Final output
    char   str[201];                               // Temp string buffer
    
    base64decode(challenge, str, sizeof (str));    // Decode the challenge
    
    hmac_md5(str, strlen(str), password,           // Create the digest
     strlen(password), digest);
 
    hmac_digest_str(digest, str, sizeof (str));    // Digest to string
    
    sprintf(final, "%s %s", username, str);        // Final string format
    base64encode(final, response, responseSize);   // Encode it
  }                                                // hmac_challenge_response()


/****************************************************************************/
/*                                                                          */
/***  hmac_digest_str()                                                   ***/
/*                                                                          */
/****************************************************************************

This procedure converts a raw 16-byte digest into a 32-char hexadecimal
string.  NOTE: 'strSize' is currently ignored until I can find a string-cat
function that works properly ...                                            */

void hmac_digest_str(const uint8* digest,          // Begin hmac_digest_str()
 char* str, uint32 strSize)
  {
    uint32  i;                                     // Loop counter
    char    sub[20];                               // Sub-string
    
    ASSERT(str && strSize >= 33);                  // Verify string
    
    str[0] = 0;                                    // Clear string to start
    
    for (i = 0; i < 16; i++)                       // Loop for digest bytes
      {
        sprintf(sub, "%02x", digest[i]);           // Do next byte
        strcat(str, sub);                          // Add this piece
      }
  }                                                // End hmac_digest_str()


/****************************************************************************/
/*                                                                          */
/***  hmac_md5()                                                          ***/
/*                                                                          */
/****************************************************************************

This procedure does the heavy lifting of converting a text string and a
key into a hashed 16-byte raw digest.                                       */

void hmac_md5(const char* text, int text_len,      // Begin hmac_md5()
 const char* key, int key_len,
 uint8* digest)
  {
    md5_ctx  context;                              // MD5 context
    int      i;                                    // Loop counter
    char     k_ipad[65];                           // Inner pad
    char     k_opad[65];                           // Outer pad
    char     tk[16];                               // Temporary key
    
    memset(k_ipad, 0, sizeof (k_ipad));            // Wipe inner pad
    memset(k_opad, 0, sizeof (k_opad));            // Wipe outer pad
    
    if (key_len > 64)                              // Key longer than 64 bytes
      {
        md5_init_ctx(&context);                    // Start
        md5_process_bytes(key, key_len, &context); // Process it
        md5_finish_ctx(&context, tk);              // Finish
        
        key = tk;                                  // Got a new key
        key_len = sizeof (tk);                     // New key length
      }
    
    memcpy(k_ipad, key, key_len);                  // Set inner pad
    memcpy(k_opad, key, key_len);                  // Set outer pad
    
    for (i = 0; i < 64; i++)                       // Loop to set up pads
      {
        k_ipad[i] ^= 0x36;                         // Set inner pad
        k_opad[i] ^= 0x5C;                         // Set outer pad
      }
      
    md5_init_ctx(&context);                        // Start inner MD5
    md5_process_bytes(k_ipad, 64, &context);       // Do inner pad
    md5_process_bytes(text, text_len, &context);   // Process bytes
    md5_finish_ctx(&context, digest);              // Finished first pass
    
    md5_init_ctx(&context);                        // Start outer MD5
    md5_process_bytes(k_opad, 64, &context);       // Do outer pad
    md5_process_bytes(digest, 16, &context);       // Results of first pass
    md5_finish_ctx(&context, digest);              // Finished second pass
  }                                                // End hmac_md5()


/****************************************************************************/
/*                                                                          */
/***  main()                                                              ***/
/*                                                                          */
/****************************************************************************

Compiled only in test builds.  Yes Kenny, I know, you'll hate this.         */

#ifdef TEST_SCAFFOLDING                            // If testing

int main(void)                                     // Begin main()
  {
    testBase64();                                  // Base64 encoding/decoding
    testMD5();                                     // Basic MD5 stuff
    testKeyedMD5();                                // Harder MD5 stuff
    testResponse();                                // The whole thing
    
    return 0;                                      // Have to return something
  }                                                // End main()

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  testBase64()                                                        ***/
/*                                                                          */
/****************************************************************************

This procedure tests base64 encoding and decoding.                          */

#ifdef TEST_SCAFFOLDING                            // Only for testing

void testBase64(void)                              // Begin testBase64()
  {
    static char  test[] = "booga booga";           // Test string
    
    char     buf[200];                             // Test buffer
    char     out[200];                             // Out buffer
    ssize_t  size;                                 // Returned size
    
    size = base64encode(test, buf, sizeof (buf));  // Encode to base64
    ASSERT(!strcmp(buf, "Ym9vZ2EgYm9vZ2E="));      // This is the string
    ASSERT(size == 16);                            // This is the length
    
    size = base64decode(buf, out, sizeof (out));   // Decode to text
    ASSERT(!strcmp(out, test));                    // This is the string
    ASSERT(size == (ssize_t) strlen(test));        // This is the length
  }                                                // End testBase64()

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  testKeyedMD5()                                                      ***/
/*                                                                          */
/****************************************************************************

This procedure tests the hardcore keyed MD5 stuff.                          */

#ifdef TEST_SCAFFOLDING                            // If testing

void testKeyedMD5(void)                            // Begin testKeyedMD5()
  {
    static char  hashStr[] = "<1896.697170952@postoffice.reston.mci.net>";
    static char  password[] = "tanstaaftanstaaf";
    
    static char  key[] = "\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B"
                         "\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B";
    
    uint8  digest[16];                             // Digest bytes
    char   str[100];                               // Final string
    
    hmac_md5(hashStr, strlen(hashStr),             // Example from RFC2195
     password, strlen(password), digest);
     
    hmac_digest_str(digest, str, sizeof (str));    // Convert to string
    
    ASSERT(!strcasecmp(str,                        // This is the answer
     "B913A602C7EDA7A495B4E6E7334D3890"));
    
    hmac_md5("Hi There", 8, key, 16, digest);      // Example from RFC2104
    hmac_digest_str(digest, str, sizeof (str));
    
    hmac_digest_str(digest, str, sizeof (str));    // Convert to string
    
    ASSERT(!strcasecmp(str,                        // This is the answer
     "9294727A3638BB1C13F48EF8158BFC9D"));
     
    hmac_md5("what do ya want for nothing?", 28,   // Example from RFC2104
     "Jefe", 4, digest);
     
    hmac_digest_str(digest, str, sizeof (str));    // Convert to string
    
    ASSERT(!strcasecmp(str,                        // This is the answer
     "750C783E6AB0B503EAA86E310A5DB738"));
     
    hmac_md5("the data", 8,                        // Key is greater than
     "0123456789012345678901234567890123456789"    //  64 chars long
     "0123456789012345678901234567890123456789",
     80, digest); 
     
    hmac_digest_str(digest, str, sizeof (str));    // Convert to string
    
    ASSERT(!strcasecmp(str,                        // This is the answer
     "79E28888940EF19A4F68D94A90FB8099")); 
  }                                                // End testKeyedMD5()

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  testMD5()                                                           ***/
/*                                                                          */
/****************************************************************************

This procedure tests simple MD5 functions.                                  */

#ifdef TEST_SCAFFOLDING                            // If testing

void testMD5(void)                                 // Begin testMD5()
  {
    TESTDATA*  data;                               // Item to work on
    uint8      digest[16];                         // Raw digest
    uint32     i;                                  // Loop counter
    char       str[40];                            // Digest string
    
    for (i = 0; testData[i].str; i++)              // Loop the list
      {
        data = &testData[i];                       // Get this item
        
        md5_buffer(data->str,                      // Calculate MD5 digest
         strlen(data->str), digest);
         
        hmac_digest_str(digest, str, sizeof (str));// Convert it 
        ASSERT(!strcasecmp(str, data->hash));      // Verify it
      }
  }                                                // End testMD5()

#endif                                             // End testing


/****************************************************************************/
/*                                                                          */
/***  testResponse()                                                      ***/
/*                                                                          */
/****************************************************************************

This procedure tests the challenge/response procedure.                      */

#ifdef TEST_SCAFFOLDING                            // If testing

void testResponse(void)                            // Begin testResponse()
  {
    RESPDATA*  data;                               // Pointer to test data
    uint32     i;                                  // Loop counter
    char       response[200];                      // Response string
    
    for (i = 0; respData[i].username; i++)         // Loop test list
      {
        data = &respData[i];                       // Get test data
        
        hmac_challenge_response(data->challenge,   // Do it
         data->username, data->password,
         response, sizeof (response));
         
        ASSERT(!strcmp(response, data->response)); // Should be the same
      }
  }                                                // End testResponse()

#endif                                             // End testing

