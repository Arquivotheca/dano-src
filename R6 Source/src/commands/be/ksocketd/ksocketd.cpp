

#define DEBUG 1
#include <Debug.h>
#include <OS.h>
#include <errno.h>
#include <socket.h>
#include <string.h>
#include "ksocket_internal.h"

#include <Application.h>
#include <Roster.h>

sem_id printf_sem;

class KSocketApp : public BApplication 
{
	public:
		KSocketApp();		
		~KSocketApp();
		void ReadyToRun();
};

static void send_reply (port_id port, const void *reply, size_t size)
{
	status_t result;
	
	do
	{
		result=write_port (port,0,reply,size);
	}
	while (result==B_INTERRUPTED);
	
	ASSERT (result>=B_OK);
}

static void ks_socket (const struct ks_socket_param *param)
{
	struct ks_socket_reply reply;
	
	reply.result=socket (param->family,param->type,param->proto);
	reply.header.error=errno;
	
	send_reply (param->header.port,&reply,sizeof(reply));
}

static void ks_multi1 (int32 code, const struct ks_bind_param *param)
{
	struct ks_bind_reply reply;
	
	if (code==KS_BIND)
		reply.result=bind (param->fd,(const struct sockaddr *)param->addr,param->size);
	else
		reply.result=connect (param->fd,(const struct sockaddr *)param->addr,param->size);
		
	reply.header.error=errno;
	
	send_reply (param->header.port,&reply,sizeof(reply));
}

static void ks_multi2 (int32 code, const struct ks_getsockname_param *param)
{
	struct sockaddr *addr=(struct sockaddr *)malloc(param->size);
	int size=param->size;
	int result;
	size_t replySize;
	struct ks_getsockname_reply *reply;
	
	switch (code)
	{
		case KS_GETSOCKNAME:
			result=getsockname(param->fd,addr,&size);
			break;
		case KS_GETPEERNAME:
			result=getpeername(param->fd,addr,&size);
			break;
		case KS_ACCEPT:
			result=accept(param->fd,addr,&size);
			break;
	}

	if (result<0)
		size=0;
	
	replySize=sizeof(struct ks_getsockname_reply)+size;
	reply=(struct ks_getsockname_reply *)malloc(replySize);
	reply->result=result;	
	reply->header.error=errno;

	if (result>=0)
	{
		reply->size=size;
		memcpy (reply->addr,addr,size);
	}
	
	send_reply (param->header.port,reply,replySize);

	free (reply);
	free (addr);
}

static void ks_recvfrom (const struct ks_recvfrom_param *param)
{
	char *buffer=(char *)malloc(param->size);
	struct sockaddr *addr=(struct sockaddr *)malloc(param->fromlen);
	int fromlen=param->fromlen;
	ssize_t result;
	size_t replySize;
	struct ks_recvfrom_reply *reply;
	
	result=recvfrom(param->fd,buffer,param->size,param->flags,addr,&fromlen);

	if (result<0)
		replySize=sizeof(struct ks_recvfrom_reply);
	else
		replySize=sizeof(struct ks_recvfrom_reply)+fromlen+result;
		
	reply=(struct ks_recvfrom_reply *)malloc(replySize);
	reply->result=result;	
	reply->header.error=errno;

	if (result>=0)
	{
		reply->fromlen=fromlen;
		memcpy (reply->data,buffer,result);
		memcpy (reply->data+result,addr,fromlen);
	}
	
	send_reply (param->header.port,reply,replySize);

	free (reply);
	free (addr);
	free (buffer);
}

static void ks_sendto (const struct ks_sendto_param *param)
{
	struct ks_sendto_reply reply;
	
	reply.result=sendto (param->fd,param->data,param->size,param->flags,
							(const struct sockaddr *)(param->data+param->size),
							param->tolen);
		
	reply.header.error=errno;
	
	send_reply (param->header.port,&reply,sizeof(reply));
}

static void ks_select (const struct ks_select_param *param)
{
	struct ks_select_reply reply;
	
	reply.readfds = param->readfds;
	reply.writefds = param->writefds;
	reply.exceptfds = param->exceptfds;
	reply.timeout = param->timeout;
	

	reply.result = select(param->nfds, &(reply.readfds), &(reply.writefds),
							&(reply.exceptfds), &(reply.timeout));	

	reply.header.error = errno;

	send_reply (param->header.port, &reply, sizeof(reply));		
	
}
static void ks_recv (const struct ks_recv_param *param)
{
	char *buffer=(char *)malloc(param->size);
	ssize_t result;
	size_t replySize;
	struct ks_recv_reply *reply;
	
	result=recv(param->fd,buffer,param->size,param->flags);

	if (result<0)
		replySize=sizeof(struct ks_recvfrom_reply);
	else
		replySize=sizeof(struct ks_recvfrom_reply)+result;
		
	reply=(struct ks_recv_reply *)malloc(replySize);
	reply->result=result;	
	reply->header.error=errno;

	if (reply->result <= 0) {
		printf("ks_recv failed (result %d) with err %d, %s\n", reply->result, errno, strerror(errno));
	}


	if (result>=0)
		memcpy (reply->data,buffer,result);
	
	send_reply (param->header.port,reply,replySize);

	free (reply);
	free (buffer);
}

static void ks_send (const struct ks_send_param *param)
{
	struct ks_send_reply reply;
	
	reply.result=send (param->fd,param->data,param->size,param->flags);
		
	if (reply.result <= 0) {
		printf("ks_send failed to send on socket %d : (result %d) with err %d, %s : tried to send %d bytes\n",
			param->fd, reply.result, errno, strerror(errno), param->size);
	}

	reply.header.error=errno;
	
	send_reply (param->header.port,&reply,sizeof(reply));
}

static void ks_listen (const struct ks_listen_param *param)
{
	struct ks_listen_reply reply;
	
	reply.result=listen (param->fd,param->backlog);
	reply.header.error=errno;
	
	send_reply (param->header.port,&reply,sizeof(reply));
}

static void ks_closesocket (const struct ks_closesocket_param *param)
{
	struct ks_closesocket_reply reply;
	
	reply.result=closesocket (param->fd);
	reply.header.error=errno;
	
	send_reply (param->header.port,&reply,sizeof(reply));
}

static status_t client_func (void *buffer)
{
	thread_id from;
	int32 code=receive_data (&from,NULL,0);
		
	ASSERT (code>=B_OK);

	switch (code)
	{
		case KS_SOCKET:
			ks_socket ((const struct ks_socket_param *)buffer);
			break;
		
		case KS_BIND:
		case KS_CONNECT:
			ks_multi1 (code,(const struct ks_bind_param *)buffer);
			break;

		case KS_GETSOCKNAME:
		case KS_GETPEERNAME:
		case KS_ACCEPT:
			ks_multi2 (code,(const struct ks_getsockname_param *)buffer);
			break;
			
		case KS_RECVFROM:
			ks_recvfrom ((const struct ks_recvfrom_param *)buffer);
			break;
			
		case KS_SENDTO:
			ks_sendto ((const struct ks_sendto_param *)buffer);
			break;
			
		case KS_RECV:
			ks_recv ((const struct ks_recv_param *)buffer);
			break;
			
		case KS_SEND:
			ks_send ((const struct ks_send_param *)buffer);
			break;
			
		case KS_LISTEN:
			ks_listen ((const struct ks_listen_param *)buffer);
			break;
			
		case KS_CLOSESOCKET:
			ks_closesocket ((const struct ks_closesocket_param *)buffer);
			break;
			
		case KS_SELECT:
			ks_select ((const struct ks_select_param *)buffer);
			break;
			
		case KS_MESSAGE:
		{
			while (acquire_sem (printf_sem)==B_INTERRUPTED) ;
			
			printf ("%s\n",buffer);

			while (release_sem (printf_sem)==B_INTERRUPTED) ;			
			break;
		}
	}
	
	free (buffer);
		
	return B_OK;
}

static status_t ksocket_func (void *buf)
{
	char *buffer;
	ssize_t bufferSize;
	int32 code;
	thread_id tid;
	status_t result;
	port_id port;

 	printf_sem=create_sem (1,"printf_sem");
	
	if (printf_sem<B_OK)
		return B_ERROR;
			
	port=create_port (10,KSOCKET_DAEMON_NAME);
		
	if (port<B_OK)
	{
		delete_sem (printf_sem);
		return B_ERROR;
	}

	while (true)
	{
	    do
		{
			bufferSize=port_buffer_size (port);
		}
		while (bufferSize==B_INTERRUPTED);
	
		ASSERT (bufferSize>=B_OK);
		
		buffer=(char *)malloc(bufferSize);
	
		do
		{
			result=read_port (port,&code,buffer,bufferSize);
		}
		while (result==B_INTERRUPTED);

		ASSERT (result>=B_OK);

		tid=spawn_thread (client_func,"ksocketd_client",B_NORMAL_PRIORITY,buffer);
		
		ASSERT (tid>=B_OK);
		
		result=resume_thread (tid);
		
		ASSERT (result>=B_OK);
		
		result=send_data (tid,code,NULL,0);
		
		ASSERT (result>=B_OK);
	}
}


KSocketApp::KSocketApp()
	: BApplication("application/x-vnd.Be-KSOCKETAPP")
{
}

KSocketApp::~KSocketApp()
{
	delete_sem (printf_sem);
}

void KSocketApp::ReadyToRun()
{
  	thread_id tid;
	status_t result;
	
	tid=spawn_thread (ksocket_func, "ksocket_func", B_NORMAL_PRIORITY, NULL);
	result=resume_thread (tid);
}

main()
{
 app_info info;
		
	//make sure we do not run this application more then once. 
	if (be_roster->GetAppInfo("application/x-vnd.Be-KSOCKETAPP", &info) < B_NO_ERROR){ 
		new KSocketApp;	
		be_app->Run();
		delete be_app;
	}

	return 0;
}



