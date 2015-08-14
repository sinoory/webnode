
/*
 * Adapted from hmac.c (HMAC-MD5) for use by SHA1.
 * by <mcr@sandelman.ottawa.on.ca>. Test cases from RFC2202.
 *
 */

/*
** Function: hmac_sha1
*/

#include "zebra.h"
#include "sha1sum.h"

#ifdef HMAC_SHA1_DATA_PROBLEMS
unsigned int sha1_data_problems = 0;
#endif

/*
unsigned char*  text;                pointer to data stream
int             text_len;            length of data stream
unsigned char*  key;                 pointer to authentication key
int             key_len;             length of authentication key
unsigned char*  digest;              caller digest to be filled in
*/
void lrad_hmac_sha1(const unsigned char *text, int text_len,
	       const unsigned char *key, int key_len,
	       unsigned char *digest)
{
         struct sha1_ctx context;
        unsigned char k_ipad[65];    /* inner padding -
                                      * key XORd with ipad
                                      */
        unsigned char k_opad[65];    /* outer padding -
                                      * key XORd with opad
                                      */
        unsigned char tk[20];
        int i;
        /* if key is longer than 64 bytes reset it to key=SHA1(key) */
        if (key_len > 64) {

                 struct sha1_ctx      tctx;

                sha1_init_ctx(&tctx);
                sha1_process_bytes( key, key_len, &tctx);
                sha1_finish_ctx(&tctx, tk);

                key = tk;
                key_len = 20;
        }

#ifdef HMAC_SHA1_DATA_PROBLEMS
	if(sha1_data_problems)
	{
		int j,k;

		printf("\nhmac-sha1 key(%d): ", key_len);
		j=0; k=0;
		for (i = 0; i < key_len; i++) {
			if(j==4) {
				printf("_");
				j=0;
			}
			j++;

			printf("%02x", key[i]);
		}
		printf("\nDATA: (%d)    ",text_len);

		j=0; k=0;
		for (i = 0; i < text_len; i++) {
		  if(k==20) {
		    printf("\n            ");
		    k=0;
		    j=0;
		  }
		  if(j==4) {
		    printf("_");
		    j=0;
		  }
		  k++;
		  j++;

		  printf("%02x", text[i]);
		}
		printf("\n");
	}
#endif


        /*
         * the HMAC_SHA1 transform looks like:
         *
         * SHA1(K XOR opad, SHA1(K XOR ipad, text))
         *
         * where K is an n byte key
         * ipad is the byte 0x36 repeated 64 times

         * opad is the byte 0x5c repeated 64 times
         * and text is the data being protected
         */

        /* start out by storing key in pads */
        memset( k_ipad, 0, sizeof(k_ipad));
        memset( k_opad, 0, sizeof(k_opad));
        memcpy( k_ipad, key, key_len);
        memcpy( k_opad, key, key_len);

        /* XOR key with ipad and opad values */
        for (i = 0; i < 64; i++) {
                k_ipad[i] ^= 0x36;
                k_opad[i] ^= 0x5c;
        }
        /*
         * perform inner SHA1
         */
        sha1_init_ctx(&context);                   /* init context for 1st
                                              * pass */
        sha1_process_bytes( k_ipad, 64, &context );      /* start with inner pad */
        sha1_process_bytes( text, text_len, &context ); /* then text of datagram */
        sha1_finish_ctx(&context, digest );          /* finish up 1st pass */
        /*
         * perform outer MD5
         */
        sha1_init_ctx(&context);                   /* init context for 2nd
                                              * pass */
        sha1_process_bytes( k_opad, 64, &context );     /* start with outer pad */
        sha1_process_bytes( digest, 20, &context );     /* then results of 1st
                                              * hash */
        sha1_finish_ctx(&context, digest);          /* finish up 2nd pass */

#ifdef HMAC_SHA1_DATA_PROBLEMS
	if(sha1_data_problems)
	{
	  int j;

		printf("\nhmac-sha1 mac(20): ");
		j=0;
		for (i = 0; i < 20; i++) {
			if(j==4) {
				printf("_");
				j=0;
			}
			j++;

			printf("%02x", digest[i]);
		}
		printf("\n");
	}
#endif
}

void HmacSha1( u8 *pu8Data, size_t sDataLen,
				u8 *pu8Key, size_t sKeyLen,
				u8*pu8Digest, size_t *psOutLen  )
{
	int k = 0;
	int len = 0;
	size_t sMaxLen = 0;
	u8 digest[20];

	if( NULL == pu8Data
		|| 0 == sDataLen
		|| NULL == pu8Key
		|| 0 == sKeyLen
		|| NULL == pu8Digest )
	{
		return;
	}
	
	lrad_hmac_sha1( pu8Data, sDataLen, pu8Key, sKeyLen, digest );
	
	sMaxLen = *psOutLen;
	for( k = 0; k < 20 && len < ( int )sMaxLen; k++ )
	{
		len += snprintf( ( char* )&pu8Digest[len], sMaxLen - len, "%02x", digest[k] & 0xFF );
	}
	*psOutLen = len;

}

/*
Test Vectors (Trailing '\0' of a character string not included in test):

  key =         "Jefe"
  data =        "what do ya want for nothing?"
  data_len =    28 bytes
  digest =		effcdf6ae5eb2fa2d27416d5f184df9c259a7c79

  key =         0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

  key_len       16 bytes
  data =        0xDDDDDDDDDDDDDDDDDDDD...
                ..DDDDDDDDDDDDDDDDDDDD...
                ..DDDDDDDDDDDDDDDDDDDD...
                ..DDDDDDDDDDDDDDDDDDDD...
                ..DDDDDDDDDDDDDDDDDDDD
  data_len =    50 bytes
  digest =      0x56be34521d144c88dbb8c733f0e8b3f6
*/

#ifdef HMACSHA1_DEBUG
/*
 *  cc -DHMACSHA1_DEBUG -I ../include/ hmac.c md5.c -o hmacsha1
 *
 *  ./hmacsha1 Jefe "what do ya want for nothing?"
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	u8 digest[41];
	size_t digest_len = sizeof( digest );
	u8 *key;
	size_t key_len;
	u8 *text;
	size_t text_len;
	int i;

	if( argc < 3 )
	{
		fprintf( stderr, "usage:\n\t%s key data\n\t echo -n 'data' | openssl dgst -hmac 'key' -sha1\n", argv[0] );
		return -1;
	}
	
	key = ( u8* )argv[1];
	key_len = strlen( ( const char* )key );

	text = ( u8* )argv[2];
	text_len = strlen( ( const char* )text );

	HmacSha1( text, text_len, key, key_len, digest, &digest_len );

	printf("\t%s\t->\t%s\n", digest, text );

	return 0;
}

#endif

