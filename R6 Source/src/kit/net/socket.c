0 finally it gets released.

24-Jun-97
    Added a SSL_OP_EPHEMERAL_RSA option which causes all SSLv3 RSA keys to
    use a temporary RSA key.  This is experimental and needs some more work.
    Fixed a few Win16 build problems.

23-Jun-97
    SSLv3 bug. I was not doing the 'lookup' of the CERT structure
    correctly. I was taking the SSL->ctx->default_cert when I should
    have been using SSL->cert. The bug was in ssl/s3_srvr.c

20-Jun-97
    X509_ATTRIBUTES were being encoded wrongly by apps/reg.c and the
    rest of the library. Even though I had the code required to do
    it correctly, apps/req.c was doing the wrong thing.  I have fixed
    and tested everything.

    Missing a few #ifdef FIONBIO sections in crypto/bio/bss_acpt.c.

19-Jun-97
    Fixed a bug in the SSLv2 server side first packet handling. When
    using the non-blocking test BIO, the ssl->s2->first_packet flag
    was being reset when a would-block failure occurred when reading
    the first 5 bytes of the first packet. This caused the checking
    logic to run at the wrong time and cause an error.

    Fixed a problem with specifying cipher. If RC4-MD5 were used,
    only the SSLv3 version would be picked up.  Now this will pick
    up both SSLv2 and SSLv3 versions. This required changing the
    SSL_CIPHER->mask values so that they only mask the ciphers,
    digests, authentication, export type and key-exchange algorithms.

    I found that when a SSLv23 session is established, a reused
    session, of type SSLv3 was attempting to write the SSLv2 
    ciphers, which were invalid. The SSL_METHOD->put_cipher_by_char 
    method has been modified so it will only write out cipher which
    that method knows about.  


 Changes between 0.8.0 and 0.8.1

  *) Mostly bug fixes. 
     There is an Ephemeral DH cipher problem which is fixed.

 SSLeay 0.8.0

This version of SSLeay has quite a lot of things different from the
previous version.

Basically check all callback parameters, I will be producing documentation
about how to use things in th future.  Currently I'm just getting 080 out
the door.  Please not that there are several ways to do everything, and
most of the applications in the apps directory are hybrids, some using old
methods and some using new methods.

Have a look in demos/bio for some very simple programs and
apps/s_client.c and apps/s_server.c for some more advanced versions.
Notes are definitly needed but they are a week or so away.

Anyway, some quick nots from Tim Hudson (tjh@cryptsoft.com)
---
Quick porting notes for moving from SSLeay-0.6.x to SSLeay-0.8.x to
get those people that want to move to using the new code base off to
a quick start.

Note that Eric has tidied up a lot of the areas of the API that were
less than desirable and renamed quite a few things (as he had to break
the API in lots of places anyrate). There are a whole pile of additional
functions for making dealing with (and creating) certificates a lot
cleaner.

01-Jul-97
Tim Hudson
tjh@cryptsoft.com

---8<---

To maintain code that uses both SSLeay-0.6.x and SSLeay-0.8.x you could
use something like the following (assuming you #include "crypto.h" which
is something that you really should be doing).

#if SSLEAY_VERSION_NUMBER >= 0x0800
#define SSLEAY8
#endif

buffer.h -> splits into buffer.h and bio.h so you need to include bio.h
            too if you are working with BIO internal stuff (as distinct
        from simply using the interface in an opaque manner)

#include "bio.h"    - required along with "buffer.h" if you write
              your own BIO routines as the buffer and bio
              stuff that was intermixed has been separated
              out 
            
envelope.h -> evp.h  (which should have been done ages ago)

Initialisation ... don't forget these or you end up with code that
is missing the bits required to do useful things (like ciphers):

SSLeay_add_ssl_algorithms()
(probably also want SSL_load_error_strings() too but you should have
 already had that call in place)

SSL_CTX_new()   - requires an extra method parameter
              SSL_CTX_new(SSLv23_method()) 
              SSL_CTX_new(SSLv2_method()) 
              SSL_CTX_new(SSLv3_method()) 

          OR to only have the server or the client code
              SSL_CTX_new(SSLv23_server_method()) 
              SSL_CTX_new(SSLv2_server_method()) 
              SSL_CTX_new(SSLv3_server_method()) 
          or  
              SSL_CTX_new(SSLv23_client_method()) 
              SSL_CTX_new(SSLv2_client_method()) 
              SSL_CTX_new(SSLv3_client_method()) 

SSL_set_default_verify_paths() ... renamed to the more appropriate
SSL_CTX_set_default_verify_paths()

If you want to use client certificates then you have to add in a bit
of extra stuff in that a SSLv3 server sends a list of those CAs that
it will accept certificates from ... so you have to provide a list to
SSLeay otherwise certain browsers will not send client certs.

SSL_CTX_set_client_CA_list(ctx,SSL_load_client_CA_file(s_cert_file));


X509_NAME_oneline(X)    -> X509_NAME_oneline(X,NULL,0)  
               or provide a buffer and size to copy the
               result into

X509_add_cert ->  X509_STORE_add_cert (and you might want to read the
          notes on X509_NAME structure changes too)


VERIFICATION CODE
=================

The codes have all be renamed from VERIFY_ERR_* to X509_V_ERR_* to
more accurately reflect things.

The verification callback args are now packaged differently so that
extra fields for verification can be added easily in future without
having to break things by adding extra parameters each release :-)

X509_cert_verify_error_string -> X509_verify_cert_error_string


BIO INTERNALS
=============

Eric has fixed things so that extra flags can be introduced in
the BIO layer in future without having to play with all the BIO
modules by adding in some macros.

The ugly stuff using 
    b->flags ~= (BIO_FLAGS_RW|BIO_FLAGS_SHOULD_RETRY)
becomes
    BIO_clear_retry_flags(b)

    b->flags |= (BIO_FLAGS_READ|BIO_FLAGS_SHOULD_RETRY)
becomes
    BIO_set_retry_read(b)

Also ... BIO_get_retry_flags(b), BIO_set_flags(b)



OTHER THINGS
============

X509_NAME has been altered so that it isn't just a STACK ... the STACK
is now in the "entries" field ... and there are a pile of nice functions
for getting at the details in a much cleaner manner.

SSL_CTX has been altered ... "cert" is no longer a direct member of this
structure ... things are now down under "cert_store" (see x509_vfy.h) and
things are no longer in a CERTIFICATE_CTX but instead in a X509_STORE.
If your code "knows" about this level of detail then it will need some 
surgery.

If you depending on the incorrect spelling of a number of the error codes
then you will have to change your code as these have been fixed.

ENV_CIPHER "type" got renamed to "nid" and as that is what it actually
has been all along so this makes things clearer.
ify_cert_error_string(ctx->error));

SSL_R_NO_CIPHER_WE_TRUST -> SSL_R_NO_CIPHER_LIST
            and SSL_R_REUSE_CIPHER_LIST_NOT_ZERO



 Changes between 0.7.x and 0.8.0
  
  *) There have been lots of changes, mostly the addition of SSLv3.
     There have been many additions from people and amongst
     others, C2Net has assisted greatly.
 
 Changes between 0.7.x and 0.7.x

  *) Internal development version only

SSLeay 0.6.6 13-Jan-1997

The main additions are

- assember for x86 DES improvments.
  From 191,000 per second on a pentium 100, I now get 281,000.  The inner
  loop and the IP/FP modifications are from
  Svend Olaf Mikkelsen <svolaf@inet.uni-c.dk>.  Many thanks for his
  contribution.
- The 'DES macros' introduced in 0.6.5 now have 3 types.
  DES_PTR1, DES_PTR2 and 'normal'.  As per before, des_opts reports which
  is best and there is a summery of mine in crypto/des/options.txt
- A few bug fixes.
- Added blowfish.  It is not used by SSL but all the other stuff that
  deals with ciphers can use it in either ecb, cbc, cfb64 or ofb64 modes.
  There are 3 options for optimising Blowfish.  BF_PTR, BF_PTR2 and 'normal'.
  BF_PTR2 is pentium/x86 specific.  The correct option is setup in
  the 'Configure' script.
- There is now a 'get client certificate' callback which can be
  'non-blocking'.  If more details are required, let me know.  It will
  documented more in SSLv3