//
// BeSSL class test program
//
#include <OS.h>
#include "BeSSL.h"

int main(int argc, char **argv)
{
	BeSSL bessl;
	unsigned char buf[1024];
    uint32 len = sizeof(buf);	
	int err = bessl.InitCheck();
	char *host = "www.verisign.com";
	const char message[] = "GET /%s HTTP/1.0\r\n\r\n";
	char msg[1024];
	char *url = "";
	be_ssl_info_t sit;
	
	if(argc > 1) host = argv[1];
	url = strstr(host, "/");
	if(url==0)
		url = "";
	else
	{
		*url = '\0';
		url++;
	}

	sprintf(msg, message, url);
	uint32 msglen = strlen(msg);
	
	
	if(err != 0)
	{
		printf("InitCheck: err = %d\n", err);
		exit(-1);
	}
	
	err = bessl.Connect(host);
	
	if(err != 0)
	{
		printf("Connect: err = %d\n", err);
		exit(-1);
	}
	
	bessl.GetInfo(&sit);
	printf("*****\nConnection info:\nSSL version: %s\nSSL cipher: %s\nSSL secret key length: %d\n*****\n",
		sit.ssl_version, sit.ssl_cipher, sit.ssl_keylength);
		 
	err = bessl.Write((const unsigned char *)msg, msglen);
	
	if(err <= 0)
	{
		printf("Write: err = %d\n", err);
		exit(-1);
	}
	
	err = 1;
	while(err > 0)
	{
        len = sizeof(buf);
        memset(buf, 0, len);                                                    
		err = bessl.Read(buf, len);
		fwrite((const char *) buf, 1, len, stdout);
	}
	if(err < 0)
	{
		printf("Read: err = %d\n", err);
		exit(-1);
	}
	
	return 0;

}


