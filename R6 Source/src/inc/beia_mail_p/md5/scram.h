/***************************************************************************
* Derived From Appendix C of:                                              *
* Network Working Group                                          C. Newman *
* Internet Draft: SCRAM-MD5 SASL Mechanism                        Innosoft *
* Document: draft-newman-auth-scram-02.txt                    January 1998 *
*                                                    Expires in six months *
***************************************************************************/


/* scram.h -- scram utility functions */

/* size of SCRAM_MD5 salt and verifier */

#define SCRAM_MD5_SALTSIZE   8
#define SCRAM_MD5_DATASIZE  16
#define SCRAM_MD5_COUNTSIZE  4

/* SCRAM verifier */

typedef struct SCRAM_MD5_VRFY_s {
    unsigned char salt[SCRAM_MD5_SALTSIZE];
    unsigned char clidata[SCRAM_MD5_DATASIZE];
    unsigned char svrdata[SCRAM_MD5_DATASIZE];
} SCRAM_MD5_VRFY;


/* Client proof message */

typedef struct SCRAM_MD5_CLIENT_s {
    unsigned char secprops[4];
    unsigned char cproof[SCRAM_MD5_DATASIZE];
} SCRAM_MD5_CLIENT;


/* Message Integrity controls */

#define  SCRAM_SECMASK       0x03  
#define  SCRAM_NOSECURITY    0x01
#define  SCRAM_INTEGRITY     0x02

typedef struct SCRAM_MD5_INTEGRITY_s {
    UINT4          inpktcnt;
    UINT4          outpktcnt;
        /* contains a packet count & the MAC integrity key */
    unsigned char  prefix[SCRAM_MD5_COUNTSIZE+HMAC_MD5_SIZE];
    unsigned char  iproof[HMAC_MD5_SIZE];
    int            macinuse;
    UINT4          maxoutcipher;
    UINT4          maxincipher;
} SCRAM_MD5_INTEGRITY;



/* generate SCRAM-MD5 verifier
 *  vptr      -- gets result
 *  salt      -- contains salt of SCRAM_MD5_SALTSIZE
 *  pass      -- passphrase or verifier
 *  passlen   -- len of pass/verifier (0 ok if NUL terminated)
 *  plainflag -- 1 = plaintext passphrase,
 *               0 = result of hmac_md5_precalc()
 *  clientkey -- cache for client proof, usually NULL
 */

void scram_md5_vgen(SCRAM_MD5_VRFY      *vptr,
                    const unsigned char *salt,
                    const char          *pass, int passlen,
                    int                  plainflag,
                    unsigned char       *clientkey);


/* scram secret action type */

#define SCRAM_CREDENTIAL 0 /* generate replies using credentials */
#define SCRAM_PLAINTEXT  1 /* generate replies using plaintext */
#define SCRAM_VERIFY     2 /* use SCRAM_MD5_VRFY to verify client,
                              and generate server reply */

/* scram integrity action type */

#define SCRAM_MAC        3 /* use SCRAM_MD5_INTEGRITY to
                              generate MAC proof for
                              outgoing packets */    
/*      SCRAM_VERIFY          use SCRAM_MD5_INTEGRITY to
        (as above)            verify MAC proof for 
                              incoming packets */



/* generate or verify SCRAM-MD5
 * input params:
 *  cchal         -- client challenge string
 *  cchallen      -- length of client challenge
 *  schal         -- server challenge string
 *  schallen      -- length of server challenge
 *  secret        -- passphrase, credentials, or verifier
 *  secretlen     -- length of passphrase (0 ok if NUL terminated)
 *  action        -- see above
 * in/out:
 *  clidata       -- client data for client response
 * output:
 *  sproof        -- server proof of length SCRAM_MD5_DATASIZE
 *  integrity_key -- integrity key of length SCRAM_MD5_DATASIZE
 *                   caller may pass NULL if no integrity needed
 * returns:
 *  -2 if params invalid
 *  -1 if verify fails
 *   0 on success
 */

int scram_md5_generate(const char *cchal, int cchallen,
                       const char *schal, int schallen,
                       const char *secret, int secretlen,
                       int action,
                       SCRAM_MD5_CLIENT *clidata,
                       unsigned char *sproof,
                       unsigned char *integrity_key);


/* generate or verify SCRAM-MD5 message integrity
 * input params;
 *  packet         -- data packet 
 *  packetlen      -- length of data packet
 *  action         -- see above (SCRAM_VERIFY & SCRAM_MAC)
 * in/out:
 *  icontrols      -- SCRAM_MD5_INTEGRITY values
 *
 * returns:
 *  -2 if params invalid
 *  -1 if verify fails
 *   0 on success
 */

int scram_md5_integrity(const char           *packet, int packetlen,
                        SCRAM_MD5_INTEGRITY  *icontrols,
                        int                   action); 
