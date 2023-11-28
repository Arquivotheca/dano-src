// KeyedMD5.h -- Code for CRAM-MD5 authentification
// Gathered from other sources by Allen Brunson  December 18, 2000

#ifndef _KEYED_MD5_H                                // If file not defined
#define _KEYED_MD5_H                                // Start it now

#include <SupportDefs.h>
#include "md5.h"


/****************************************************************************/
/*                                                                          */
/***  Base64 encoding and decoding                                        ***/
/*                                                                          */
/****************************************************************************/

ssize_t  base64decode(const char* in, char* out,   // Base64 decoding
 uint32 outSize);
ssize_t  base64encode(const char* in, char* out,   // Base64 encoding
 uint32 outSize);


/****************************************************************************/
/*                                                                          */
/***  Keyed MD5 routines                                                  ***/
/*                                                                          */
/****************************************************************************/

void  hmac_challenge_response(const char*          // Does the whole damn
 challenge,                                        //  thing
 const char* username, const char* password,
 char* response, uint32 responseSize);

void  hmac_md5(const char* text, int text_len,     // Keyed MD5 hashing
 const char* key, int key_len,
 uint8* digest);
 
void  hmac_digest_str(const uint8* digest,         // Digest to string
 char* str, uint32 strSize); 


/****************************************************************************/
/*                                                                          */
/***  Testing defines                                                     ***/
/*                                                                          */
/****************************************************************************/

#ifdef TEST_SCAFFOLDING                            // Only for testing

struct RESPDATA                                    // One challenge/response
  {
    const char*  username;
    const char*  password;
    const char*  challenge;
    const char*  response;
  };
      
struct TESTDATA                                    // Data for one MD5 test
  {
    const char*  str;                              // Original string
    const char*  hash;                             // Hashed value
  };  

int   main(void);                                  // For testing

void  testBase64(void);                            // Base64 encoding/decoding
void  testKeyedMD5(void);                          // Keyed MD5 stuff
void  testMD5(void);                               // Basic MD5 tests
void  testResponse(void);                          // The whole enchilada

#endif                                             // End test defines


/****************************************************************************/
/*                                                                          */
/***  Keyed MD5 authentication module                                     ***/
/*                                                                          */
/****************************************************************************


Overview
--------

This module contains code necessary to handle MD5 challenge-response
username and password encryption as used in IMAP and SMTP.  It may be
suitable for other protocols as well, for all I know.


Functions
---------

hmac_challenge_response() is the only function that code outside this module
needs to call.  It will need the base64-encoded challenge sent by the IMAP
or SMTP server minus any decoration that might come before or after the
base64 characters (don't forget to strip CR/LF chars).  It also needs pointers
to the username and password to log in with.  Finally give it a pointer to
a char buffer that will receive the base64-encoded response to send back
to the server and the size of the buffer in bytes.  A 200-byte buffer should
be more than enough.

 
Testing
-------

The module contains a main() and very thorough test functions that will only
be compiled if TEST_SCAFFOLDING is #defined.  It will ASSERT() in case of
failure.


Stuff to do
-----------

Find a terminated strncpy() replacement.  Needed for hmac_digest_str() and
possibly others.


Dependencies
------------

md5.c, md5.h.  Taken from the GCC codebase (I think).  I only added a C++
guard to the header file and added a define to make the code use memcpy()
versus the mysterious bcopy() I know nothing about.

Base64ToRawAdapter.cpp, RawToBase64Adapter.cpp, and the corresponding
header files.  Base64 "adapters" originally written by Jeff Bush.


References
----------

RFC 2195  IMAP/POP AUTHorize extension
RFC 2104  HMAC: Keyed-Hashing for Message Authentication

*/

#endif                                             // End KEYED_MD5_H
