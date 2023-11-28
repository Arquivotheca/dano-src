#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

status_t nmb_lookup( char *name, struct in_addr *addr );

typedef struct nsp_header
{
	uint16		id;
	
	// First Byte Fields
	uint16	rd:1;
	uint16	tc:1;
	uint16	aa:1;
	uint16	opcode:4;
		#define NSP_QUERY			0x0
		#define NSP_REGISTRATION	0x5
		#define NSP_RELEASE			0x6
		#define NSP_WACK			0x7
		#define NSP_REFRESH			0x8
	uint16	r:1;
		#define	NSP_REQUEST			0
		#define NSP_RESPONSE		1
	
	// Second Byte Fields
	uint16	rcode:4;
	uint16	b:1;
	uint16	unused2:1;
	uint16	unused1:1;
	uint16	ra:1; 

	uint16		qdcount;
	uint16		ancount;
	uint16		nscount;
	uint16		arcount;
} nsp_header_t;

typedef struct ns_query
{
	uint16		type;
#define NSPQ_NB 		htons(0x0020)		// NAME QUERY REQUEST
#define NSPQ_NBSTAT 	htons(0x0021)		// NODE STATUS REQUEST
	uint16		class;
#define NSPQ_INET		htons(0x0001)		// Internet Class
} ns_query_t;

typedef struct ns_response
{
	uint16			type;
	uint16			class;
	uint32			ttl;
	uint16			length;
	uint16			addr_type;
	struct in_addr	inaddr;
} ns_response_t;

static void l1_encode( char *dst, const char *name, const char pad, const char sfx );
static int l2_encode( char *dst, const char *name, const char pad, const char sfx, const char *scope );

void l1_encode( char *dst, const char *name, const char pad, const char sfx )
{
	char *end;
	char a, b;
	
	for( end = dst + 30; dst < end && *name; dst += 2 )
	{
		a = toupper( *name++ );
		dst[0] = ((a & 0xF0) >> 4) + 'A';
		dst[1] = (a & 0x0F) + 'A';
	}
	
	a = ((pad & 0xF0) >> 4) + 'A';
	b = (pad & 0x0F) + 'A';
	
	for( ; dst < end; dst += 2 )
	{
		dst[0] = a;
		dst[1] = b;
	}
	
	dst[0] = ((sfx & 0xF0) >> 4) + 'A';
	dst[1] = (sfx & 0x0F) + 'A';
	dst[2] = '\0';
}

int l2_encode( char *dst, const char *name, const char pad, const char sfx, const char *scope )
{
	char *start = dst, *base;
	
	dst[0] = 32;
	l1_encode( dst + 1, name, pad, sfx );
	
	for( dst += 33, base = dst++, *base = 0; *scope; scope++ )
	{
		if( *scope == '.' )
			base = dst++;
		else
		{
			(*base)++;
			*dst++ = toupper( *scope );
		}
	}
	*dst = 0;
	return dst - start;
}

status_t nmb_lookup( char *name, struct in_addr *addr )
{
	int					s;
	int					i;
	struct sockaddr_in 	a;
	struct timeval		to;
	status_t			status;
	nsp_header_t		*qheader, *rheader;
	ns_query_t 			*question;
	ns_response_t		*response;
	int					qlength, rlength, nlength;
	char				query_packet[512], response_packet[512];
	
	
	if( (s = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP )) < 0 )
		return errno;
	
	memset( &a, 0, sizeof(a) );
	
	a.sin_family = AF_INET;
	a.sin_port = 0;
	a.sin_addr.s_addr = INADDR_ANY;
	
	if( bind( s, (struct sockaddr *)&a, sizeof(a) ) < 0 )
	{
		status = errno;
		goto error;
	}
	
	to.tv_sec = 1;
	to.tv_usec = 0;
	
	if( setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, &to,sizeof(to) ) < 0 )
	{
		status = errno;
		goto error;
	}
	
	qheader = (nsp_header_t *)query_packet;
	
	memset( qheader, 0, sizeof(*qheader) );
	qheader->id = ntohs(system_time());
	qheader->r = NSP_REQUEST;
	qheader->opcode = NSP_QUERY;
	qheader->rd = 1;
	qheader->b = 1;
	
	qheader->qdcount = ntohs(1);
	
	question = (ns_query_t *)(query_packet + sizeof(*qheader) + (nlength = l2_encode( query_packet + sizeof(*qheader), name, '\x20', '\x20', "" )));
	question->type = NSPQ_NB;
	question->class = NSPQ_INET;
	
	qlength = ((char *)question) - query_packet + sizeof(*question);
	memset( &a, 0, sizeof(a) );
	
	a.sin_family = AF_INET;
	a.sin_port = htons( 137 );
	a.sin_addr.s_addr = htonl( INADDR_BROADCAST );
	
	for( i = 0; i < 3; i++ )
	{
		if( (sendto( s, query_packet, qlength, 0, (struct sockaddr *)&a, sizeof(a) )) < 0 )
		{
			status = errno;
			goto error;
		}
		
		receive:
		if( (rlength = recv( s, response_packet, sizeof(response_packet), 0)) >= 0 )
		{
			if( (uint32)rlength < sizeof(*rheader) )
			{
				status = ENOENT;
				goto error;
			}
			
			rheader = (nsp_header_t *)response_packet;
			
			// If the id does not match, keep reading
			if( rheader->id != qheader->id )
				goto receive;
			// sanity check for reply
			else if( rheader->r != NSP_RESPONSE || rheader->opcode != NSP_QUERY || rheader->ancount == 0 ||
				memcmp( response_packet + sizeof(*rheader), query_packet + sizeof(*qheader), nlength ) != 0 )
			{
				status = B_ERROR;
				goto error;
			}
			break;
		}
		else if( errno != ETIMEDOUT )
		{
			status = errno;
			goto error;
		}
	}
	
	response = (ns_response_t *)(response_packet + sizeof(*rheader) + nlength);
	if( response->type != NSPQ_NB || response->class != NSPQ_INET || response->length < 6 )
	{
		status = B_ERROR;
		goto error;
	}
	
	addr->s_addr = response->inaddr.s_addr;
	status = 0;
	
	error:
	close(s);
	return status;
}
