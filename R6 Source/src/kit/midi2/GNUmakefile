hen
     you #define DEBUG_PKCS5V2 passwords, salts, iteration counts and
     derived keys are printed to stderr.
     [Steve Henson]

  *) Copy the flags in ASN1_STRING_dup().
     [Roman E. Pavlov <pre@mo.msk.ru>]

  *) The x509 application mishandled signing requests containing DSA
     keys when the signing key was also DSA and the parameters didn't match.

     It was supposed to omit the parameters when they matched the signing key:
     the verifying software was then supposed to automatically use the CA's
     parameters if they were absent from the end user certificate.

     Omitting parameters is no longer recommended. The test was also
     the wrong way round! This was probably due to unusual behaviour in
     EVP_cmp_parameters() which returns 1 if the parameters match. 
     This meant that parameters were omitted when they *didn't* match and
     the certificate was useless. Certificates signed with 'ca' didn't have
     this bug.
     [Steve Henson, reported by Doug Erickson <Doug.Erickson@Part.NET>]

  *) Memory leak checking (-DCRYPTO_MDEBUG) had some problems.
     The interface is as follows:
     Applications can use
         CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON) aka MemCheck_start(),
         CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_OFF) aka MemCheck_stop();
     "off" is now the default.
     The library internally uses
         CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_DISABLE) aka MemCheck_off(),
         CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ENABLE) aka MemCheck_on()
     to disable memory-checking temporarily.

     Some inconsistent states that previously were possible (and were
     even the default) are now avoided.

     -DCRYPTO_MDEBUG_TIME is new and additionally stores the current time
     with each memory chunk allocated; this is occasionally more helpful
     than just having a counter.

     -DCRYPTO_MDEBUG_THREAD is also new and adds the thread ID.

     -DCRYPTO_MDEBUG_ALL enables all of the above, plus any future
     extensions.
     [Bodo Moeller]

  *) Introduce "mode" for SSL structures (with defaults in SSL_CTX),
     which largely parallels "options", but is for changing API behaviour,
     whereas "options" are about protocol behaviour.
     Initial "mode" flags are:

     SSL_MODE_ENABLE_PARTIAL_WRITE   Allow SSL_write to report success when
             