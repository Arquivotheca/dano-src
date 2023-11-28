/* inet.c
 *
 * A socket class. The socket functions are minimal, only superceeding some
 * of the hard-to-use stuff in Berkeley sockets. Mostly, the job of this
 * is to ease socket creation and make a set of platform-independent defines,
 * typedefs, etc.
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#if !defined(__JSE_MAC__)  || (defined(__JSE_MAC__) && defined(USE_MAC_WINSOCK))

#include "jsetypes.h"
#include "jselib.h"
#include "jsemem.h"
#include "dbgprntf.h"
#include "unixfunc.h"
#include "sesyshdr.h"
#include "inet.h"
#include "jseopt.h"


#if !defined(JSETOOLKIT_CORE)

#ifndef max
   #define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
   #define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#if defined(__JSE_NWNLM__)
  NETDB_DEFINE_CONTEXT
  NETINET_DEFINE_CONTEXT
#endif

/*
 * First we need a function to get the host given a host name
 */
static int hostaddr(jsechar *name,struct sockaddr_in *addr)
{
  struct hostent *hp;

  if( name==0 || *name==0 ) return 0;           /* We need a name! */
  if( isdigit(*name) )                          /* Is it 128.174.etc? */
    {
      addr->sin_addr.s_addr = inet_addr(name);
    } else {
      hp = gethostbyname(name);                 /* else it is jack.edu */
      if( hp==NULL) return 0;
      memcpy((ubyte *)&addr->sin_addr,hp->h_addr,sizeof(addr->sin_addr));
    }
  return 1;
}


/*
 * Then of course we need something to work out the port
 */
static int hostport(jsechar *name,struct sockaddr_in *addr)
{
  struct servent *sp;

  if( name==0 || *name==0 ) return 0;
  if( isdigit(*name) )
    {
      addr->sin_port = htons((unsigned short)atoi(name));
    } else {
      sp = getservbyname(name, "tcp");
      if (sp == NULL) return 0;
      addr->sin_port = (unsigned short)sp->s_port;
    }
  return 1;
}

/*
 * We got the information, let's connect!
 */
static SOCKET hostconnect(struct sockaddr_in *addr)
{
  SOCKET s;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if( s==INVALID_SOCKET ) return SOCKET_ERROR;
  addr->sin_family = AF_INET;
  if( connect(s, (struct sockaddr *)addr, sizeof(*addr))<0 )
    {
      closesocket(s);
      return SOCKET_ERROR;
    }
  return s;
}

   void
jsesocketBlocking(struct jseSocket *This)
{
  unsigned long non_blocking;

  if( This->sock==SOCKET_ERROR ) return;

  non_blocking = 0;

#if defined(__JSE_UNIX__) || defined(__JSE_OS2TEXT__) || defined(__JSE_OS2PM__) || \
    defined(__JSE_NWNLM__)
  ioctl(This->sock,FIONBIO,(ubyte *)&non_blocking,sizeof(non_blocking));
#elif defined(__JSE_WIN16__) || defined(__JSE_WIN32__)
  ioctlsocket(This->sock,FIONBIO,&non_blocking);
#elif defined(__JSE_MAC__)
  /* Do Nothing.  Don't know why... */
#else
  #error dont know how to make socket blocking in this OS
#endif
}

   void
jsesocketDelete(struct jseSocket *This)
{
   if( SOCKET_ERROR != This->sock )
   {
      closesocket(This->sock);
      /*This->sock = SOCKET_ERROR;*/
   }
   jseMustFree(This);
}

   static struct jseSocket * NEAR_CALL
jsesocketNew()
{
   struct jseSocket *This = jseMustMalloc(struct jseSocket,sizeof(struct jseSocket));
   memset(This,0,sizeof(*This));
   return This;
}

   struct jseSocket *
jsesocketNewPort(int port)
{
  struct sockaddr_in mysin;
  struct jseSocket *This = jsesocketNew();
  
  This->sock = socket(AF_INET,SOCK_STREAM,0);

  mysin.sin_addr.s_addr = INADDR_ANY;
  mysin.sin_port = htons((unsigned short)port);
  mysin.sin_family = AF_INET;

  if( bind(This->sock,(struct sockaddr *)&mysin,sizeof(mysin)) || (listen(This->sock,5)<0) )
    {
      closesocket(This->sock); This->sock = SOCKET_ERROR;
    }

  jsesocketBlocking(This);
  return This;
}

   struct jseSocket *
jsesocktNewSocket(struct jseSocket *s);

   struct jseSocket *
jsesocktNewSocket(struct jseSocket *s)
{
  /*struct sockaddr_in mysin;*/
  jsechar *strptr;
  struct jseSocket *This = jsesocketNew();

  struct sockaddr addr;
  #ifdef _AIX
    unsigned long length = sizeof(addr);
  #else
    int length = sizeof(addr);
  #endif

  This->sock = accept(s->sock,&addr,&length);
  jsesocketBlocking(This);
  strptr = inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr);
  strncpy(This->connected_to,strptr,sizeof(This->connected_to));
  return This;
}


   struct jseSocket *
jsesocketNewSocketAddrLen(struct jseSocket *s,jsechar *host,int len)
{
  jsechar *strptr;
  struct jseSocket *This = jsesocketNew();

  struct sockaddr addr;
  #ifdef _AIX
    unsigned long length = sizeof(addr);
  #else
    int length = sizeof(addr);
  #endif
  This->sock = accept(s->sock,&addr,&length);
  jsesocketBlocking(This);
  strptr = inet_ntoa(((struct sockaddr_in *)&addr)->sin_addr);
  strncpy(host,strptr,len);
  strncpy(This->connected_to,strptr,sizeof(This->connected_to));
  return This;
}

   void
jsesocketHostName(jsechar *buffer,int length)
{
  gethostname(buffer,length);
}

   void
jsesocketHostAddr(jsechar *buffer,int length)
{
  gethostname(buffer,length);
  jsesocketTranslateToNumber(buffer);
}

   int
jsesocketTranslateToNumber(jsechar *addr)
{
  struct hostent *he;
  struct in_addr *ptr;
  
  if( isdigit(*addr) ) return 1;  /* already done. */

  he = gethostbyname(addr);

  if( he==NULL ) return 0;

  ptr = (struct in_addr *)he->h_addr;

#if BIG_ENDIAN
  sprintf(addr,"%d.%d.%d.%d",
          ptr->s_addr>>24,(ptr->s_addr>>16)&255,
          (ptr->s_addr>>8)&255,ptr->s_addr&255);
#else
  sprintf(addr,"%d.%d.%d.%d",
          ptr->s_addr&255,(ptr->s_addr>>8)&255,
          (ptr->s_addr>>16)&255,ptr->s_addr>>24);
#endif
  return 1;
}

   struct jseSocket *
jsesocketNewHostPort(jsechar *host,int port)
{
  jsechar buffer[128];
  struct sockaddr_in addr;
  jsechar buffer2[30];
  struct jseSocket *This = jsesocketNew();

  if( host==NULL )
    {
      jsesocketHostName(buffer,sizeof(buffer)); host = buffer;
    }

  sprintf(buffer2,"%u",port);
  This->sock = SOCKET_ERROR;
  if( hostaddr(host,&addr) && hostport(buffer2,&addr) )
    This->sock = hostconnect(&addr);

  jsesocketBlocking(This);

  if( This->sock != SOCKET_ERROR ) {
     strncpy(This->connected_to,host,sizeof(This->connected_to));
  }else {
     strncpy(This->connected_to,"no connection",sizeof("no connection"));
  }
  return This;
}

#if defined(_WIN16_) || defined(_DOS16_)
   #define MAX_INET_CHUNK_SIZE   0x4000
   inline uword16 MaxSameSegmentIncrement(ubyte _HUGE_ * Ptr)
      { return ((uword16)Ptr) ? ~((uword16)Ptr) + 1 : 0xFFFF ; };
#elif defined(IP_MAXPACKET)
   #define MAX_INET_CHUNK_SIZE   (IP_MAXPACKET - 0x400)
#else
   #define MAX_INET_CHUNK_SIZE   0x7000
#endif

   int
jsesocketSendString(struct jseSocket *This,const jsechar *buffer)
{
   return (int)jsesocketSendBuffer(This,(const ubyte *)buffer,strlen(buffer)*sizeof(jsechar));
}

   slong
jsesocketSendBuffer(struct jseSocket *This,const ubyte _HUGE_ *buffer,slong len)
{
   slong Sent;
   slong TotalSent = 0;
   while ( TotalSent < len ) {
      /* figure out how big the next send can safely be */
      slong ChunkSize = min(MAX_INET_CHUNK_SIZE,len - TotalSent);
      #if defined(_WIN16_) || defined(_DOS16_)
         /* there can be segment wrapping problems, so make sure this send would not wrap to the next segment */
         uword16 MaxSegmentAddition = MaxSameSegmentIncrement(buffer);
         if ( MaxSegmentAddition < ChunkSize )
            ChunkSize = MaxSegmentAddition;
      #endif
      assert( 0 < ChunkSize );
      if ( (Sent = send( This->sock, (char *)buffer, (int)ChunkSize, 0 )) < 1 )
         break;
      TotalSent += Sent;
      buffer += Sent;
   }
   return TotalSent;
}


   slong
jsesocketRecv(struct jseSocket *This,ubyte _HUGE_ *buffer,slong len)
{
  slong ReadCount;
  slong TotalRecv;
   
  if( This->buf_end > This->buf_start )
    {
      int num_chars = This->buf_end - This->buf_start;    /* NO, not +1 */
      if( num_chars>len ) num_chars = (int)len;
      memcpy(buffer,This->recv_buffer+This->buf_start,num_chars);
      This->buf_start += num_chars;
      return num_chars;
    }
    
   TotalRecv = 0;
   while ( TotalRecv < len ) {
      /* figure out how big the next send can safely be */
      slong ChunkSize = min(MAX_INET_CHUNK_SIZE,len - TotalRecv);
      #if defined(_WIN16_) || defined(_DOS16_)
         /* there can be segment wrapping problems, so make sure this send would not wrap to the next segment */
         uword16 MaxSegmentAddition = MaxSameSegmentIncrement(buffer);
         if ( MaxSegmentAddition < ChunkSize )
            ChunkSize = MaxSegmentAddition;
      #endif
      assert( 0 < ChunkSize );
      if ( (ReadCount = recv( This->sock, (char *)buffer, (int)ChunkSize, 0 )) < 1 )
         break;
      TotalRecv += ReadCount;
      buffer += ReadCount;
   }
   return TotalRecv;
}

   int
jsesocketRecvLine(struct jseSocket *This,jsechar *buffer,int maxchars)
{
  int current = 0;
  int eollength = 2;

  buffer[0] = '\0';

  while( 1 )
    {
      jsechar *loc;
      int num_chars;

      if( This->buf_start>=This->buf_end )
        {
          This->buf_start = 0;
          jsesocketBlocking(This);
          This->buf_end = recv(This->sock,This->recv_buffer,sizeof(This->recv_buffer)-1,0);
          if( This->buf_end<1 ) return -1;    /* bad */
          This->recv_buffer[This->buf_end] = '\0';
        }

      loc = strstr(This->recv_buffer+This->buf_start,EOL);

      /* allow lines to be terminated via '\n' also. */
      if( loc==NULL && (loc = strchr(This->recv_buffer+This->buf_start,'\n'))!=NULL )
        eollength = 1;

      if( loc )
        num_chars = loc-(This->recv_buffer+This->buf_start);
      else
        num_chars = This->buf_end - This->buf_start;

      if( num_chars>maxchars-current )
        {
          num_chars = maxchars-current -1;       /* make space for '\0' */
        }

      strncpy(buffer+current,This->recv_buffer+This->buf_start,num_chars);
      buffer[current+=num_chars] = '\0';
      This->buf_start += num_chars;

      /* if managed to send back everything and found EOL, skip over it. */
      if( loc && loc==This->recv_buffer+This->buf_start )
        {
          This->buf_start += eollength;
          return current;
        }

      /* we check to see if we filled up the buffer. If so, we return
         regardless of having found EOL or not. Otherwise, go back and
         wait for EOL again. */
      if( This->buf_start<This->buf_end ) return current;
    }
}

/* ---------------------------------------------------------------------- */

#if defined(__JSE_WIN32__) || defined(__JSE_WIN16__) || (defined(__JSE_MAC__) && defined(USE_MAC_WINSOCK))
/*
 * Returns a boolean as to the success of starting winsock.
 */
#if defined(__JSE_WIN16__)
int __far start_winsock(void)
#else
int start_winsock(void)
#endif
{
  WSADATA data;
  #if defined(__JSE_MAC__)
        u_short version;
  #else
        WORD version;
  #endif
  int ret;

  version = (1<<8) | 1;
  ret = !WSAStartup(version,&data);
  return ret;
}

#if defined(__JSE_WIN16__)
void __far stop_winsock(void)
#else
void stop_winsock(void)
#endif
{
  WSACleanup();
}
#endif

#endif /* !defined(JSETOOLKIT_CORE) */

#else
   ALLOW_EMPTY_FILE
#endif /* !defined(__JSE_MAC__) || (defined(__JSE_MAC__) && defined(USE_MAC_WINSOCK)) */
