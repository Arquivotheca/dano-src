/***************************************************************************
* From Appendix B of:                                                      *
* Network Working Group                                          C. Newman *
* Internet Draft: SCRAM-MD5 SASL Mechanism                        Innosoft *
* Document: draft-newman-auth-scram-02.txt                    January 1998 *
*                                                    Expires in six months *
***************************************************************************/


/* hmac-md5.h -- HMAC_MD5 functions */

#define HMAC_MD5_SIZE 16

/* intermediate MD5 context */

typedef struct HMAC_MD5_CTX_s {
    MD5_CTX ictx, octx;
} HMAC_MD5_CTX;


/* intermediate HMAC state
 *  values stored in network byte order (Big Endian)
 */
typedef struct HMAC_MD5_STATE_s {
    UINT4 istate[4];
    UINT4 ostate[4];
} HMAC_MD5_STATE;


/* One step hmac computation
 *
 * digest may be same as text or key
 */
void hmac_md5(const unsigned char *text, int text_len,
           const unsigned char *key, int key_len,
           unsigned char digest[HMAC_MD5_SIZE]);


/* create context from key
 */
void hmac_md5_init(HMAC_MD5_CTX *hmac,
        const unsigned char *key, int key_len);


/* precalculate intermediate state from key
 */
void hmac_md5_precalc(HMAC_MD5_STATE *hmac,
           const unsigned char *key, int key_len);


/* initialize context from intermediate state
 */
void hmac_md5_import(HMAC_MD5_CTX *hmac, HMAC_MD5_STATE *state);

#define hmac_md5_update(hmac, text, text_len) MD5Update(&(hmac)->ictx, (text), (text_len))


/* finish hmac from intermediate result.
 *  Intermediate result is erased.
 */
void hmac_md5_final(unsigned char digest[HMAC_MD5_SIZE],
         HMAC_MD5_CTX *hmac);

