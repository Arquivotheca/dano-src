#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <openssl/sha.h>
#include <openssl/dsa.h>
#include <openssl/des.h>

#define IO_BUF_SIZE		16384

int do_info( int argc, char **argv );
int do_sha1( int argc, char **argv );
int do_mkparameters( int argc, char **argv );
int do_mkkey( int argc, char **argv );
int do_sign( int argc, char **argv );
int do_verify( int argc, char **argv );

/* dsa make parameters callback: prints anoying characters and stuff... */
static void dsa_cb( int p, int n, void *arg );

/* Simple parser for ascii parameters*/
/* Parameters are stored in the following format: "name=value\n"*/
/* get_next_parameter() will copy the name to "name", the value to "value" and return*/
/* a pointer to the first character of the next parameter or NULL if no more parameters.*/
static char *get_next_parameter( const char *s, char *name, char *value );

static char *write_numeric_param( char *s, char *name, unsigned char *number, int length, int columns );

/* Open the file specified in "path" and try to parse as DSA parameters file.*/
static int load_dsa_parameters( DSA *dsa, const char *path );

/* Open the file specified in "path" and try to load the DSA public key. */
static int load_key( BIGNUM **key, const char *path );

/* Prompt the user with "prompt" and read string from stdin with terminal echo disabled into "password"*/
/* Setting "verify" to 1 will make the user enter the password a second time to verify it.*/
/* Returns "0" if on success, "1" if "verify" failed, or < 0 on error.*/
static int get_password( char *password, int length, const char *prompt, int verify );

/* memmem as described in GNU C lib... not the most efficient, but it works */
/* If memmem is available in your C lib, you can remove this version */
static void *memmem( const void *needle, size_t needle_len, const void *haystack, size_t haystack_len );

int main( int argc, char **argv )
{
	if( argc == 1 || strcmp( "--help", argv[1] ) == 0 )
		return do_info( argc, argv );
	
	argc--;
	argv++;
	
	if( strcmp( "sha1", argv[0] ) == 0 )
		return do_sha1( argc, argv );
	else if( strcmp( "mkparams", argv[0] ) == 0 )
		return do_mkparameters( argc, argv );
	else if( strcmp( "mkkey", argv[0] ) == 0 )
		return do_mkkey( argc, argv );
	else if( strcmp( "sign", argv[0] ) == 0 )
		return do_sign( argc, argv );
	else if( strcmp( "verify", argv[0] ) == 0 )
		return do_verify( argc, argv );
	else
		return do_info( argc + 1, argv - 1 );
}

int do_info( int argc, char **argv )
{
	printf(
		"usage: %s function [parameters] ...\n\n"
		"DSA Digital Signature Utility:\n"
		"Use \"%s function --help\" for details on individual functions.\n\n"
		"Functions:\n"
		"\tsha1 - calculate and print sha1 message digest of file\n"
		"\tmkparams - generate DSA parameters used by other functions\n"
		"\tmkkey - generate new public/private key pair\n"
		"\tsign - sign a document\n"
		"\tverify - verify a document\n\n",
		argv[0], argv[0]
	 );	
	return 1;
}

int do_sha1( int argc, char **argv )
{
	if( argc == 1 || strcmp( "--help", argv[1] ) == 0 )
	{
		printf( "usage: %s file\n\n"
		"This calculates and prints the 160 bit sha1 message digest of a file.\n"
		"It's provided as a diagnostic utility.\n\n",
		argv[0] );
		return 1;
	}
	else
	{
		int				fd, i;
		int				status = 1;
		ssize_t			size;
		unsigned char	*buffer;
		
		SHA_CTX			sha;
		unsigned char	md[SHA_DIGEST_LENGTH];
		
		if( (buffer = (unsigned char *)malloc( IO_BUF_SIZE )) == NULL )
		{
			fprintf( stderr, "Insufficient memory to for buffer\n" );
			return 1;
		}
		
		if( (fd = open( argv[1], O_RDONLY )) < 0 )
		{
			fprintf( stderr, "Could not open file \"%s\" because: %s\n", argv[1], strerror(errno) );
			goto exit_0;
		}
		SHA1_Init( &sha );
		while( (size = read( fd, buffer, IO_BUF_SIZE ))>0 )
			SHA1_Update( &sha, buffer, size );
		SHA1_Final( md, &sha );
		
		for( i=0; i<SHA_DIGEST_LENGTH; i++ )
			sprintf( buffer + (i<<1), "%02X", md[i] );
		
		printf( "%s\n", buffer );
		status = 0;
		exit_1:
		close(fd);
		exit_0:
		free( buffer );
		return status;
	}
}

int do_mkparameters( int argc, char **argv )
{
	if( argc < 3 || strcmp( "--help", argv[1] ) == 0 )
	{
		printf( "usage: %s key_length output_file [seed_file]\n\n"
		"The other DSA functions require a common set of parameters.\n"
		"Theese parameters are public and can be used with multiple keys.\n"
		"This function generates the needed parameters (p,q,g) which can \n"
		"either be seeded by the pseudo-random number generator or an\n"
		"optional \"seed file.\" If a seed file is used, it's format should be:\n\n"
		"\t\"seed: a1b2c3...\\n\" where 'a1b2c3...' is a 20 digit hex number (160 bits).\n\n"
		"The \"key_length\" parameter is the number of bits used for keys\n"
		"utilizing this set of parameters: 1024 is a good number.\n\n",
		argv[0] );
		return 1;
	}
	else
	{
		int				fd, i;
		int				status = 1;
		ssize_t			size;
		
		unsigned char	*buffer;
		char			p_name[32], p_value[4096], *next;
		
		unsigned char	seed[SHA_DIGEST_LENGTH];
		int				seed_len = 0;
		int				bits;
		
		DSA 			*dsa = NULL;
		unsigned long	h;
		int				counter;
		
		char			number[4096];
		unsigned int	number_len;
		
		if( (buffer = (unsigned char *)malloc( IO_BUF_SIZE )) == NULL )
		{
			fprintf( stderr, "Insufficient memory for buffer\n" );
			return 1;
		}
		
		/* If a seed file was specified, load the seed file */
		if( argc > 3 )
		{
			char	num[4];
			
			if( (fd = open( argv[3], O_RDONLY )) < 0 )
			{
				fprintf( stderr, "Could not open file \"%s\" because: %s\n", argv[1], strerror(errno) );
				goto exit_0;
			}
			
			size = read( fd, buffer, IO_BUF_SIZE-1 );
			close(fd);
			
			buffer[size] = 0;
			next = buffer;
			
			/* Get the value of the seed */
			next = get_next_parameter( next, p_name, p_value );
			
			/* Sanity check to verify this is a seed and has the right number of characters */
			if( (strcmp( p_name, "seed" ) != 0) || (strlen(p_value) != SHA_DIGEST_LENGTH << 1) )
			{
				fprintf( stderr, "Error reading seed file\n" );
				goto exit_0;
			}
			
			/* Convert seed from hex/ascii to binary format */
			num[3] = 0;
			for( i=0; i<SHA_DIGEST_LENGTH; i++ )
			{
				num[0] = p_value[i<<1];
				num[1] = p_value[(i<<1) + 1];
				seed[i] = strtoul(num, NULL, 16);
			}
			
			seed_len = SHA_DIGEST_LENGTH;
			
			/* Print the seed to verify parsing */
			fprintf( stderr, "seed: " );
			for( i=0; i<SHA_DIGEST_LENGTH; i++ )
				sprintf( buffer + (i*3), "%02X:", seed[i] );
			fprintf( stderr, "%s\n", buffer );
		}
		
		bits = strtoul( argv[1], NULL, 0 );
		fprintf( stderr, "generating parameters: " );
		fflush( stderr );
		
		/* Generate DSA paramters p, q, and g */
		/* This will probably take some time... */
		if( (dsa = DSA_generate_parameters( bits, seed_len ? seed : NULL, seed_len, &counter, &h, dsa_cb, NULL )) == NULL )
		{
			printf( "Could not generate parameters!\n" );
			return 1;
		}
		
		next = buffer;
		
		/* Convert parameters to ascii and write to buffer */
		number_len = BN_bn2bin( dsa->p, number );
		next = write_numeric_param( next, "p", number, number_len, 32 );
		
		number_len = BN_bn2bin( dsa->q, number );
		next = write_numeric_param( next, "q", number, number_len, 32 );
		
		number_len = BN_bn2bin( dsa->g, number );
		next = write_numeric_param( next, "g", number, number_len, 32 );
		
		/* Create parameters file and write buffer */
		if( (fd = open( argv[2], O_WRONLY | O_CREAT | O_TRUNC )) < 0 )
		{
			fprintf( stderr, "Could not create output file \"%s\" because: %s\n", argv[2], strerror(errno) );
			goto exit_1;
		}
		write( fd, buffer, ((unsigned char *)next) - buffer );
		close( fd );
		
		status = 0;
		exit_1:
		DSA_free( dsa );
		exit_0:
		free( buffer );
		return status;
	}
}

static void dsa_cb( int p, int n, void *arg )
{
	char c;
	
	switch( p )
	{
		case 0:
			c = '.';
			break;
		case 1:
			c = '+';
			break;
		case 2:
			c = '*';
			break;
		case 3:
			c = '\n';
			break;
		default:
			
	}
	
	fwrite ( &c, 1, 1, stderr );
	fflush( stderr );
}

int do_mkkey( int argc, char **argv )
{
	if( argc < 3 || strcmp( "--help", argv[1] ) == 0 )
	{
		printf( "usage: %s params_file key_name\n\n"
		"This function will generate a new public/private key pair utilizing\n"
		"the DSA parameters specified in \"params_file.\"\n\n"
		"Two files will be created: \"key_name.priv\" and \"key_name.pub\".\n"
		"key_name.priv - This contains the DSA private key encrypted with DES.\n"
		"\tThis file is required to sign documents and should be \n"
		"\tprotected from unauthorized access.\n\n"
		"key_name.pub - This contains the DSA public key in ascii format.\n"
		"\tThis file is required to verify documents signed with\n"
		"\tthe private key. It can be made public without risk of\n"
		"\tcompromising the private key.\n\n",
		argv[0] );
		return 1;
	}
#if !NO_PASSWORD
	else if( !isatty( STDIN_FILENO ) )
	{
		fprintf( stderr, "No tty to accept password!\n" );
		return 1;
	} 
#endif
	else
	{
		int				fd, i;
		int				status = 1;
		ssize_t			size;
		unsigned char	*buffer;
		
		DSA 			*dsa = NULL;
		
		char			number[4096];
		unsigned int	number_len;
		
		if( (buffer = (unsigned char *)malloc( IO_BUF_SIZE )) == NULL )
		{
			fprintf( stderr, "Insufficient memory for buffer\n" );
			return 1;
		}
		
		/* Allocate and init DSA structure */
		if( (dsa = DSA_new()) == NULL )
		{
			fprintf( stderr, "Could not create dsa?\n" );
			goto exit_0;
		}
		
		/* Get the DSA parameters from the specified file */
		if( load_dsa_parameters( dsa, argv[1] ) < 0 )
		{
			fprintf( stderr, "Could not load DSA parameters!\n" );
			goto exit_1;
		}
		
		/* Generate new key pair */
		printf( "Generating key pair...\n" );
		DSA_generate_key( dsa );
		
#if !NO_PASSWORD
		/* Encrypt and store private key */
		{
			int					presult;
			des_cblock 			*in, *out;
			des_cblock			key;
			des_key_schedule 	ks;
			unsigned char 		output[24];
			char				password[256];
			SHA_CTX				sha;
			unsigned char		hash[SHA_DIGEST_LENGTH];
			
			/* Ask the user for a password */
			if( (presult = get_password( password, 256, "Set signature password: ", 1 ) != 0) )
			{
				if( presult == 1 )
					fprintf( stderr, "Passwords do not match!\n" );
				else
					fprintf( stderr, "Error getting password: %s\n", strerror( presult ) );
				goto exit_1;
			}
			
			/* Use sha1 to hash the password and use the first 64 bits as the DES key */
			SHA1_Init( &sha );
			SHA1_Update( &sha, password, strlen(password) );
			SHA1_Final( hash, &sha );
			memcpy( key, hash, 8 );
			
			/* Convert private key big number to 160 bit binary format */
			if( BN_bn2bin( dsa->priv_key, buffer ) != 20 )
			{
				fprintf( stderr, "Private key was not 160 bits!?\n" );
				goto exit_1;
			}
			
			/* Setup DES schedule with key */
			if( des_key_sched( &key, ks ) != 0 )
			{
				fprintf( stderr, "Bad Key!\n" );
				goto exit_1;
			}
			
			/* Setup input and output; encrypt 160 bit private key */
			in = (des_cblock *)buffer;
			out = (des_cblock *)output;
			
			/* Yes, it's ECB mode and not the more desireable CBC mode... live with it! */
			for( i=0; i<3; i++ )
				des_ecb_encrypt( in++, out++, ks, DES_ENCRYPT );
			
			/* Purge from memory anything which could be used to extract the unencrypted key */
			/* Just becaue your paranoia doesn't mean they aren't out to get you! */
			BN_clear_free( dsa->priv_key );
			dsa->priv_key = NULL;
			memset( password, 0, 64 );
			memset( hash, 0, SHA_DIGEST_LENGTH );
			memset( &ks, 0, sizeof(ks) );
			memset( buffer, 0, 24 );
			memset( key, 0, 8 );
			
			/* Write the encrypted private key to file */
			sprintf( buffer, "%s.priv", argv[2] );
			if( (fd = open( buffer, O_WRONLY | O_CREAT | O_TRUNC )) < 0 )
			{
				fprintf( stderr, "Could not create private key file \"%s\" because: %s\n", buffer, strerror(errno) );
				goto exit_1;
			}
			
			write( fd, output, 24 );
			close( fd );
		}
#else
		sprintf( buffer, "%s.priv", argv[2] );
		if( (fd = open( buffer, O_WRONLY | O_CREAT | O_TRUNC )) < 0 )
		{
			fprintf( stderr, "Could not create private key file \"%s\" because: %s\n", buffer, strerror(errno) );
			goto exit_1;
		}
		
		number_len = BN_bn2bin( dsa->priv_key, number );
		write_numeric_param( buffer, "dsa_private_key", number, number_len, 32 );
		
		write( fd, buffer, strlen( buffer ) );
		close( fd );
#endif
		/* Create public key file */
		sprintf( buffer, "%s.pub", argv[2] );
		if( (fd = open( buffer, O_WRONLY | O_CREAT | O_TRUNC )) < 0 )
		{
			fprintf( stderr, "Could not create public key file \"%s\" because: %s\n", buffer, strerror(errno) );
			goto exit_1;
		}
		
		/* Convert private key to ascii and write to public key file as plaintext (it's public, right?) */
		number_len = BN_bn2bin( dsa->pub_key, number );
		write_numeric_param( buffer, "dsa_public_key", number, number_len, 32 );
		
		write( fd, buffer, strlen( buffer ) );
		close( fd );
		
		status = 0;
		exit_1:
		DSA_free( dsa );
		exit_0:
		free( buffer );
		return status;
	}
}


int do_sign( int argc, char **argv )
{
	if( argc < 4 || strcmp( "--help", argv[1] ) == 0 )
	{
		printf( "usage: %s params_file key target [password]\n\n"
		"This will append a DSA signature to the file specified in \"target\"\n"
		"using the specified key. The private key, which is required to\n"
		"sign a document, is encrypted and will require the password originally\n"
		"entered when the key was created. While this provides some proteciton from the\n"
		"unauthorized use of your private key, you should try to avoid letting this\n"
		"key fall into the wrong hands; it is easier to crack the encryption on the\n"
		"private key than it is to forge a signature knowing only the public key.\n\n",
		argv[0] );
		return 1;
	}
	else
	{
		int				fd, i;
		int				status = 1;
		ssize_t			size, total_size = 0;
		unsigned char	*buffer, *next;
		
		DSA 			*dsa = NULL;
		
		char			number[4096];
		unsigned int	number_len;
		
		SHA_CTX			sha;
		unsigned char	md[SHA_DIGEST_LENGTH];
		
		if( (buffer = (unsigned char *)malloc( IO_BUF_SIZE )) == NULL )
		{
			fprintf( stderr, "Insufficient memory to for buffer\n" );
			return 1;
		}
		
		/* Allocate and init DSA structure */
		if( (dsa = DSA_new()) == NULL )
		{
			fprintf( stderr, "Could not create dsa?\n" );
			goto exit_0;
		}
		
		/* Get the DSA parameters from the specified file */
		if( load_dsa_parameters( dsa, argv[1] ) < 0 )
		{
			fprintf( stderr, "Could not load DSA parameters!\n" );
			goto exit_1;
		}
		/* Allocate new private key structure */
		if( (dsa->priv_key = BN_new()) == NULL )
		{
			fprintf( stderr, "Could not allocate storage for private key!\n" );
			goto exit_1;
		}
		
		/* Open target file for hashing */
		if( (fd = open( argv[3], O_RDONLY )) < 0 )
		{
			fprintf( stderr, "Could not open file \"%s\" because: %s\n", argv[3], strerror(errno) );
			goto exit_0;
		}
		
		/* Secure hash the file */
		SHA1_Init( &sha );
		while( (size = read( fd, buffer, IO_BUF_SIZE ))>0 )
		{
			SHA1_Update( &sha, buffer, size );
			total_size += size;
		}
		SHA1_Final( md, &sha );
		
		close( fd );
		
#if !NO_PASSWORD
		{
			int					presult;
			des_cblock 			*in, *out;
			des_cblock			key;
			des_key_schedule 	ks;
			unsigned char 		input[24];
			char				password[256];
			unsigned char		hash[SHA_DIGEST_LENGTH];
			
			/* Read the encrypted private key file */
			sprintf( buffer, "%s.priv", argv[2] );
			if( (fd = open( buffer, O_RDONLY )) < 0 )
			{
				fprintf( stderr, "Could not open private key file \"%s\" because: %s\n", buffer, strerror(errno) );
				goto exit_1;
			}
			
			if( read( fd, input, 24 ) != 24 )
			{
				fprintf( stderr, "This private key file sucks! Sorry...\n" );
				close( fd );
				goto exit_1;
			}
			close( fd );
			
			/* Ask the user for a password */
			if(argv[4]) {
				memset(password, 0, sizeof(password));
				strncpy(password, argv[4], sizeof(password)-1);
			} else {
				if( !isatty( STDIN_FILENO ) )
				{
					fprintf( stderr, "No tty to accept password!\n" );
					return 1;
				} else if( (presult = get_password( password, 256, "Signature password: ", 0 ) != 0) ) {
					fprintf( stderr, "Error getting password: %s\n", strerror( presult ) );
					goto exit_1;
				}
			}
			
			/* Use sha1 to hash the password and use the first 64 bits as the DES key */
			SHA1_Init( &sha );
			SHA1_Update( &sha, password, strlen(password) );
			SHA1_Final( hash, &sha );
			
			memcpy( key, hash, 8 );
			
			/* Setup DES schedule with key */
			if( des_key_sched( &key, ks ) != 0 )
			{
				fprintf( stderr, "Bad Key!\n" );
				goto exit_1;
			}
			
			/* Setup input and output; decrypt 160 bit private key */
			in = (des_cblock *)input;
			out = (des_cblock *)buffer;
			
			/* Yes, it's ECB mode and not the more desireable CBC mode... live with it! */
			for( i=0; i<3; i++ )
				des_ecb_encrypt( in++, out++, ks, DES_DECRYPT );
			
			/* Note: There is no way to tell at this point if the decrypted key is valid */
			/* The only way to know is to sign something and verify it with the public key */
			/* Have fun brute-forcing that! */
			
			/* Convert private key from 160 bit binary format to big number format */
			if( BN_bin2bn( buffer, 20, dsa->priv_key ) == NULL )
			{
				fprintf( stderr, "Could not convert binary private key to big number format!\n" );
				goto exit_1;
			}
			
			/* Purge from memory anything which could be used to extract the unencrypted key */
			/* This is with the exception of the actual bn private key, which we still need... */
			/* ...but its time _WILL_ come! */
			/* Just becaue your paranoia doesn't mean they aren't out to get you! */
			memset( password, 0, 64 );
			memset( hash, 0, SHA_DIGEST_LENGTH );
			memset( &ks, 0, sizeof(ks) );
			memset( buffer, 0, 24 );
			memset( key, 0, 8 );
		}
#else
		sprintf( buffer, "%s.priv", argv[2] );
		if( load_key( &dsa->priv_key, buffer ) != 0 )
		{
			fprintf( stderr, "Could not open private key file \"%s\" because: %s\n", buffer, strerror(errno) );
			goto exit_1;
		}
#endif		
		number_len = 4096;
		
		/* Sign the message digest */
		DSA_sign( 0, md, 20, number, &number_len, dsa );
		
		/* Time to die... That is, we can now dispose of the private key */
		BN_clear_free( dsa->priv_key );
		dsa->priv_key = NULL;
		
		/* Open Public Key File */
		/* We want to verify we did not botch the signature */
		sprintf( buffer, "%s.pub", argv[2] );
		
		if( load_key( &dsa->pub_key, buffer ) != 0 )
		{
			fprintf( stderr, "Could not open public key file \"%s\" because: %s\n", buffer, strerror(errno) );
			fprintf( stderr, "There is no way to know if the signature is valid!\n" );
			goto exit_1;
		}
		
		/* Test the signature */
		if( DSA_verify( 0, md, 20, number, number_len, dsa ) != 1 )
		{
			fprintf( stderr, "The password was not valid!\n" );
			goto exit_1;
		}
		
		/* Open the target file again with write access to append signature */
		if( (fd = open( argv[3], O_WRONLY | O_APPEND )) < 0 )
		{
			fprintf( stderr, "Could not open file \"%s\" because: %s\n", argv[3], strerror(errno) );
			goto exit_1;
		}
		
		next = buffer;
		
		next += sprintf( next, "/*** Begin DSA Signature ***/\n" );
		next = write_numeric_param( next, "dsa_sig", number, number_len, 32 );
		next += sprintf( next, "/*** End DSA Signature ***/\n" );
		write( fd, buffer, strlen( buffer ) );
		close( fd );
		
		status = 0;
		exit_1:
		DSA_free( dsa );
		exit_0:
		free( buffer );
		return status;
	}
}

int do_verify( int argc, char **argv )
{
	if( argc < 4 || strcmp( "--help", argv[1] ) == 0 )
	{
		printf( "usage: %s params_file key target\n\n"
		"This will search for a signature appended to the file specified by \"target\"\n"
		"and will use the \"key\" to verify the signature. If the signature\n"
		"is valid, the key will be stripped from the file and an exit value of 0\n"
		"will be returned; otherwise, 1 will be returned.\n\n",
		argv[0] );
		return 1;
	}
	else
	{
		int				fd, i;
		int				status = 1;
		ssize_t			size;
		off_t			file_length, sig_start, remaining;
		unsigned char	*buffer, *next, *s;
		int				needle_len;
		
		unsigned char	number[4096];
		unsigned int	number_len;
		
		char			p_name[32], p_value[4096];
		
		DSA 			*dsa = NULL;
		
		SHA_CTX			sha;
		unsigned char	md[SHA_DIGEST_LENGTH];
		
		
		if( (buffer = (unsigned char *)malloc( IO_BUF_SIZE )) == NULL )
		{
			fprintf( stderr, "Insufficient memory to for buffer\n" );
			return 1;
		}
		
		/* Allocate and init DSA structure */
		if( (dsa = DSA_new()) == NULL )
		{
			fprintf( stderr, "Could not create dsa?\n" );
			goto exit_0;
		}
		
		/* Get the DSA parameters from the specified file */
		if( load_dsa_parameters( dsa, argv[1] ) < 0 )
		{
			fprintf( stderr, "Could not load DSA parameters!\n" );
			goto exit_1;
		}
		
		/* Load the Public Key File */
		sprintf( buffer, "%s.pub", argv[2] );
		if( load_key( &dsa->pub_key, buffer ) != 0 )
		{
			fprintf( stderr, "Could not open public key file \"%s\" because: %s\n", buffer, strerror(errno) );
			goto exit_1;
		}
		
		/* Open target file for verification */
		if( (fd = open( argv[3], O_RDWR )) < 0 )
		{
			fprintf( stderr, "Could not open file \"%s\" because: %s\n", argv[3], strerror(errno) );
			goto exit_1;
		}
		
		/* Seek End of File and get the length as a bonus */
		file_length = lseek( fd, 0, SEEK_END );
		
		/* Backup by 4 Kb from the end */
		size = 4096;
		if( file_length < size )
			size = file_length;
		
		sig_start = lseek( fd, -size, SEEK_END );
		read( fd, buffer, size );
		
		/* Look for the last DSA Signature */
		buffer[size] = 0;
		needle_len = strlen( "/*** Begin DSA Signature ***/" );
		s = buffer;
		for( i=0, next = s; size && ((s = memmem( "/*** Begin DSA Signature ***/", needle_len, s, size )) != NULL); i++, s++ )
		{
			size -= s-next;
			next = s;
		}
			
		if( i == 0 )
		{
			fprintf( stderr, "No signature detected in target \"%s\"\n", argv[3] );
			goto exit_2;
		}
		
		/* Calculate file offset of signature start */
		sig_start += next - buffer;
		
		/* Find the start of the following line */
		if( (next = strpbrk( next, "\r\n" )) == NULL )
		{
			fprintf( stderr, "Signature parsing error\n" );
			goto exit_2;
		}
		next += strspn( next, "\r\n" );
		
		/* Get the signature */
		get_next_parameter( next, p_name, p_value );
		if( strcmp( p_name, "dsa_sig" ) != 0 )
		{
			fprintf( stderr, "Signature parsing error\n" );
			goto exit_2;
		}
		
		/* Get the signature length */
		if( (number_len = strlen( p_value ) >> 1) > 4096 )
		{
			fprintf( stderr, "Signature is too long!\n" );
			goto exit_2;
		}
		
		/* Convert signatue from hex/ascii to binary format */
		buffer[2] = 0;
		for( i=0; i<number_len; i++ )
		{
			buffer[0] = p_value[i<<1];
			buffer[1] = p_value[(i<<1) + 1];
			number[i] = strtoul(buffer, NULL, 16);
		}
		
		/* Secure hash the file */
		SHA1_Init( &sha );
		remaining = sig_start;
		lseek( fd, 0, SEEK_SET );
		do
		{
			size = IO_BUF_SIZE;
			if( remaining < IO_BUF_SIZE )
				size = remaining;
			
			if( (size = read( fd, buffer, size )) <= 0 )
				break;
			remaining -= size;
			SHA1_Update( &sha, buffer, size );
		} while( remaining );
		SHA1_Final( md, &sha );
		
		/* Verify the Signature */
		if( DSA_verify( 0, md, 20, number, number_len, dsa ) != 1 )
		{
			fprintf( stderr, "*** The signature is not valid! ***\n" );
			goto exit_2;
		}
		
		/* Strip the Signature from the file */
		ftruncate( fd, sig_start );
		fprintf( stderr, "The signature is valid.\n" );
		
		status = 0;
		exit_2:
		close(fd);
		exit_1:
		DSA_free( dsa );
		exit_0:
		free( buffer );
		return status;
	}
}

static char *write_numeric_param( char *s, char *name, unsigned char *number, int length, int columns )
{
	int		column;
	int		i;
	
	s += sprintf( s, "%s: \n\t", name );
	for( i=0, column = 0; i<length; i++, column++ )
	{
		if( column == columns )
		{
			s += sprintf( s, "\n\t" );
			column = 0;
		}
		s += sprintf( s, "%.2X", (int)number[i] );
	}
	s += sprintf( s, "\n" );
	return s;
}

static char *get_next_parameter( const char *s, char *name, char *value )
{
	int			spn;
	const char 	*end;
	char		*dst;
	
	name[0] = 0;
	value[0] = 0;
	
	/* Find name/value seperator ':' */
	if( (end = strpbrk( s, ":" )) == NULL )
		return NULL;
	
	/* Calculate length of name */
	spn = end - s;
	
	/* Truncate if greater that 32 bytes */
	if( spn > 31 )
		spn = 31;
	
	/* Copy name and terminate with eof */
	memcpy( name, s, spn );
	name[spn] = 0;
	
	/* Advance to first character of value */
	s = end+1;
	
	/* Set the dest. to the first character in value */
	dst = value;
	
	do
	{
		/* Skip past white space */
		s += strspn( s, " \t" );
		
		/* Find end of this line */
		if( (end = strpbrk( s, "\r\n" )) == NULL )
			return NULL;
		
		/* Calculate length of line and truncate if it will not fit in the 4096 byte buffer when copied */
		spn = end - s;
		if( spn > 4095 - (dst-value) )
			spn = 4095 - (dst-value);
		
		/* Copy this segment, advance dest pointer, and terminate with null */
		memcpy( dst, s, spn );
		dst += spn;
		*dst = 0;
		
		/* Skip to start of next line */
		s = end + strspn( end, "\r\n" );
		
	/* Continue reading value if next line starts with white space */
	} while( isspace(*s) );
	
	return (char *)s;
}

static int load_key( BIGNUM **key, const char *path )
{
	int				fd, i;
	int				status = -1;
	ssize_t			size;
	unsigned char	*buffer;
	
	char			p_name[32], p_value[4096];
	
	errno = -1;
	if( (buffer = (unsigned char *)malloc( 4096 )) == NULL )
		return ENOMEM;
	
	/* Open the specified file and read the contents into the buffer */
	if( (fd = open( path, O_RDONLY )) < 0 )
	{
		close(fd);
		goto exit_0;
	}
	
	if( (size = read( fd, buffer, 4096 )) < 0 )
	{
		close( fd );
		goto exit_0;
	}
	
	close( fd );
	buffer[size] = 0;
	
	/* Parse the signature and convert from ascii to big number format */
	get_next_parameter( buffer, p_name, p_value );
	if (( strcmp( p_name, "dsa_public_key" ) != 0 ) && 
		( strcmp( p_name, "dsa_private_key" ) != 0 ))
		goto exit_0;
	BN_hex2bn(key, p_value );
	
	status = 0;
	exit_0:
	free( buffer );
	return status;
}

static int load_dsa_parameters( DSA *dsa, const char *path )
{
	int				fd, i;
	int				status = -1;
	ssize_t			size;
	unsigned char	*buffer;
		
	char		p_name[32], p_value[4096], *p_next;
	BIGNUM		**bn;
	
	if( (buffer = (unsigned char *)malloc( IO_BUF_SIZE )) == NULL )
		return ENOMEM;
	
	if( (fd = open( path, O_RDONLY )) < 0 )
		goto exit_0;
	if( (size = read( fd, buffer, IO_BUF_SIZE-1 )) < 0 )
	{
		close(fd);
		goto exit_0;
	}
	close(fd);
	
	p_next = buffer;
	buffer[size] = 0;
	
	while( (p_next = get_next_parameter( p_next, p_name, p_value )) != NULL )
	{
		bn = NULL;
		switch( p_name[0] )
		{
			case 'p':
				bn = &dsa->p;
				break;
			case 'q':
				bn = &dsa->q;
				break;
			case 'g':
				bn = &dsa->g;
				break;
		}
		
		if( bn )
		{
			if( *bn != NULL )
			{
				BN_clear_free( *bn );
				*bn = NULL;
			}
			BN_hex2bn( bn, p_value );
		}
	}
	status = 0;
	exit_0:
	free( buffer );
	return status;
}

int get_password( char *password, int length, const char *prompt, int verify )
{
	int				plen;
	int 			status = 0;
	struct termios 	old_attr, new_attr;
	
	/* Save current state of terminal device */
	if( tcgetattr( STDIN_FILENO, &old_attr ) < 0 )
		return errno;
	
	/* Copy current state, clear the echo flag and set new state */
	new_attr = old_attr;
	new_attr.c_lflag &= ~ECHO;
	if( tcsetattr( STDIN_FILENO, TCSAFLUSH, &new_attr ) < 0 )
		return errno;
	
	/* Prompt for input */
	fputs( prompt, stdout );
	fflush( stdout );
	
	fgets( password, length, stdin );
	putchar('\n');
	plen = strlen( password );
	
	/* If verify flag is set, make user enter password again */
	if( verify )
	{
		char *temp;
		
		/* Create temporary storage for password */
		if( (temp = malloc( plen+1 )) == NULL )
			return ENOMEM;
			
		/* Prompt again */
		fputs( "Enter again to verify: ", stdout );
		fflush( stdout );
		
		/* Read password */
		fgets( temp, plen+1, stdin );
		putchar('\n');
		
		if( strcmp( password, temp ) != 0 )
			status = 1;
		
		/* Clear storage and free */
		memset( temp, 0, plen );
		free( temp );
	}
	/* Stip '\n' character from password string */
	password[plen-1] = 0;
	
	/* Restore old terminal state */
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &old_attr );
	return status;
}

/* Search for pattern in buffer */
static void *memmem( const void *needle, size_t needle_len, const void *haystack, size_t haystack_len )
{
	char		*next = (char *)haystack, *end = ((char *)haystack) + haystack_len - needle_len + 1;
	char		c = *(char *)needle;
	
	for( ; next < end; next++ )
	{
		if( c == *next )
		{
			
			if( memcmp(needle, next, needle_len) == 0 ) 
				return next;
		}
	}
	return NULL;
}
