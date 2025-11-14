#include <stdio.h>

#include <time.h>
#include <sys/mman.h>
#include <unistd.h>

#include "library.h"

#include <openssl/bio.h> /* BasicInput/Output streams */
#include <openssl/err.h> /* errors */
#include <openssl/ssl.h> /* core library */

#include <openssl/rsa.h>
#include <openssl/pem.h>


int main(){

	printf("in main, creating private key..");

	const int kBits = 1024;
	const int kExp = 3; 

	int keylen;
	char *pem_key;

	RSA *rsa = RSA_generate_key(kBits, kExp, 0, 0);

	printf("&RSA: %#p\n", rsa);

	BIO *bio = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL);

	keylen = BIO_pending(bio);
	pem_key = calloc(keylen+1, 1); /* Null-terminate */
	BIO_read(bio, pem_key, keylen);

	printf("pem: %s (%#p)\n", pem_key, pem_key);


	printf("in main, calling test library func\n");

	test();	

	return	0;
}
