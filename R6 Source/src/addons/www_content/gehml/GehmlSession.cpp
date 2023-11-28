//******************************************************************************
//
//	File:		session.cpp
//
//	Description:	AppSession class.
//			connection session control.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <OS.h>
#include <String.h>

#ifndef _APPLICATION_H
#include "Application.h"
#endif
#ifndef _INTERFACE_DEFS_H
#include "InterfaceDefs.h"
#endif
#ifndef _SESSION_H
#include <session.h>
#endif

#include <token.h>
#include <shared_support.h>

#ifndef	_MATH_H
#include <math.h>
#endif

#ifdef REMOTE_DISPLAY
#include "TCPIPSocket.h"
#endif // REMOTE_DISPLAY

#define VALIDATE_LOCKING 1

long	cnt = 0;

#ifdef SYNC_CALL_LOG

#define LOG_OUTPUT_QUANTUM	5000
#define REPORT_TOP_COUNT	5
#define CALLSTACK_DEPTH		9

enum {
	SESSION_READING = 1,
	SESSION_WRITING
};

#define DebugPrintf(a) printf a
#define grFree(a) free(a)
#define grMalloc(a,b) malloc(a)
#define grRealloc(a,b,c) realloc(a,b)

#include "Unmangle.h"

template <class t>
class BArray {

	private:

		t		*items;
		int32	numItems;
		int32	numSlots;
		int32	blockSize;

inline	int32	AssertSize(int size)
{
	if (size > numSlots) {
		if (numSlots<=0) numSlots = blockSize;
		if (numSlots<=0) numSlots = 1;
		while (size > numSlots) numSlots *= 2;
		t *tmp = (t*)grRealloc(items,numSlots*sizeof(t),"AssertSize");
		if (!tmp) return -1;
		items = tmp;
	};
	return size;
};

	public:

inline			BArray(int _blockSize=256)
{
	blockSize = _blockSize;
	numItems = numSlots = 0;
	items = NULL;
};

inline			BArray(BArray<t> &copyFrom)
{
	blockSize = copyFrom.blockSize;
	numSlots = 0;
	items = NULL;
	AssertSize(copyFrom.numSlots);
	numItems = copyFrom.numItems;
	memcpy(items,copyFrom.items,numItems*sizeof(t));
};

inline			~BArray()
{
	if (items)
		grFree(items);
};

inline	t*		Items()
{
	return items;
};

inline	void		SetList(t* newList, int32 listSize)
{
	if (items)
		grFree(items);
	items = newList;
	numSlots = listSize;
	numItems = listSize;
};

inline	void		SetSlots(int32 slots)
{
	if (numSlots != slots) {
		numSlots = slots;
		if (numItems > numSlots)
			numItems = numSlots;
		t *tmp = (t*)grRealloc(items,numSlots*sizeof(t),"SetSlots");
		if (!tmp) return;
		items = tmp;
	};
};

inline	void		SetItems(int32 count)
{
	if (AssertSize(count) < 0)
		return;
	numItems = count;
};

inline	void		Trim()
{
	SetSlots(numItems);
};

inline	int32	CountItems()
{ return numItems; };

inline	void		RemoveItems(int32 index, int32 len)
{
	memmove(items+index,items+index+len,sizeof(t)*(numItems-index-len));
	numItems-=len;
};

inline	void		RemoveItem(int32 index)
{
	RemoveItems(index,1);
};

inline	int32	AddArray(BArray<t> *a)
{
	if (AssertSize(numItems + a->numItems) < 0)
		return -1;
	memcpy(items+numItems,a->items,a->numItems*sizeof(t));
	numItems = numItems + a->numItems;
	return a->numItems;
};

inline	int32	AddItem(const t &theT)
{
	if (AssertSize(numItems+1) < 0)
		return -1;
	items[numItems] = theT;
	numItems++;
	return numItems-1;
};

inline	void		MakeEmpty()
{
	numSlots = 0;
	numItems = 0;
	if (items!=NULL) {
		grFree(items);
		items = NULL;
	};
};

inline	t&		ItemAt(int index)
				{ return items[index]; };

inline	t&		operator[](int index)
				{ return items[index]; };
};

struct symbol {
	uint32	addr;
	uint32	name;
};

char *			symbolNames = NULL;
int32			symbolNamesSize = 0;
int32			symbolNamesPtr = 0;
BArray<symbol>	symbolTable;

int symbol_cmp(const void *p1, const void *p2)
{
	const symbol *sym1 = (const symbol*)p1;
	const symbol *sym2 = (const symbol*)p2;
	return (sym1->addr == sym2->addr)?0:
			((sym1->addr > sym2->addr)?1:-1);
};

void load_symbols()
{
	image_info info;
	char name[255];
	void *location;
	int32 symType,cookie=0,n=0,nameLen=255;
	symbol sym;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		image_id id = info.id;
		n = 0;
		while (get_nth_image_symbol(id,n,name,&nameLen,&symType,&location) == B_OK) {
			while ((symbolNamesPtr + nameLen) > symbolNamesSize) {
				if (symbolNamesSize > 0) symbolNamesSize *= 2;
				else symbolNamesSize = 1024;
				symbolNames = (char*)realloc(symbolNames,symbolNamesSize);
			};
			sym.addr = (uint32)location;
			sym.name = symbolNamesPtr;
			symbolTable.AddItem(sym);
			strcpy(symbolNames+symbolNamesPtr,name);
			symbolNamesPtr += nameLen;
			nameLen = 255;
			symType = B_SYMBOL_TYPE_ANY;
			n++;
		};
	};
	qsort(symbolTable.Items(),symbolTable.CountItems(),sizeof(symbol),&symbol_cmp);
};

const char * lookup_symbol(uint32 addr, uint32 *offset)
{
	int32 i;
	if (symbolNames == NULL) load_symbols();
	if (symbolNames == NULL) return "";
	int32 count = symbolTable.CountItems();
	for (i=0;i<(count-1);i++) {
		if ((addr >= symbolTable[i].addr) && (addr < symbolTable[i+1].addr)) break;
	};
	*offset = addr - symbolTable[i].addr;
	return symbolNames + symbolTable[i].name;
};

class CallStack {
	public:

		uint32			m_caller[CALLSTACK_DEPTH];

						CallStack();
		unsigned long 	GetCallerAddress(int level);
		void 			SPrint(char *buffer);
		void 			Print();
		void 			Update(int32 ignoreDepth=0);

		CallStack		&operator=(const CallStack &from);
		bool			operator==(CallStack) const;
};

class CallTreeNode {
	public:
	uint32					addr;
	int32					count;
	CallTreeNode *			higher;
	CallTreeNode *			lower;
	CallTreeNode *			parent;
	BArray<CallTreeNode*>	branches;
	
							~CallTreeNode();
	void					PruneNode();
	void					ShortReport();
	void					LongReport(char *buffer, int32 bufferSize);
};

class CallTree : public CallTreeNode {
	public:
	CallTreeNode *			highest;
	CallTreeNode *			lowest;

							CallTree(const char *name);
							~CallTree();
	void					Prune();
	void					AddToTree(CallStack *stack);
	void					Report(int32 count, bool longReport=false);
};

CallTree synchronous("sync");

#endif // SYNC_CALL_LOG

/*---------------------------------------------------------------*/

#ifdef	DEBUG_PROTO

void	AppSession::pdebug(long port_id, long session_id, message *a_message)
{
	long	time;

	time = system_time() / 100;
	fprintf(debug_file, "%ld %ld %ld\n", time, port_id, session_id);
	fprintf(debug_file, "%lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx\n",	
			 a_message->what,
			 a_message->parm1,
			 a_message->parm2,
			 a_message->parm3,
			 a_message->parm4,
			 a_message->parm5,
			 a_message->parm6,
			 a_message->parm7,
			 a_message->parm8,
			 a_message->parm9,
			 a_message->parm10,
			 a_message->parm11,
			 a_message->parm12,
			 a_message->parm13,
			 a_message->parm14,
			 a_message->parm15);

}
#endif

/*---------------------------------------------------------------*/

inline void memcpy2(void *dst, const void *src, int32 count)
{
	uint32 *out = (uint32*)src;
	uint32 *in = (uint32*)dst;
	while (count >= 4) {
		*in++ = *out++;
		count-=4;
	};
	if (count > 0) {
		*((uint8*)in) = *((uint8*)out);
		if (count > 1) {
			*(((uint8*)in)+1) = *(((uint8*)out)+1);
			if (count > 2) {
				*(((uint8*)in)+2) = *(((uint8*)out)+2);
			};
		};
	};
};

static long	new_id()
{
	long	tmp;

	tmp = system_time();
	
	if (tmp == 0)
		tmp++;
	else if (tmp < 0)
		tmp = -tmp;
		
	return(tmp);
}

/*--------------------------------------------------------------------*/

#ifdef REMOTE_DISPLAY
AppSession::AppSession(TCPIPSocket *sock, bool validate_locking)
{
	r_message = (message*)malloc(MAX_MSG_SIZE);
	s_message = (message*)malloc(MAX_MSG_SIZE);
	session_id = new_id();

	cur_pos_send = 4;
	cur_pos_receive = MAX_MSG_SIZE;
	receive_size = MAX_MSG_SIZE;
	send_port = B_BAD_PORT_ID;
	receive_port = B_BAD_PORT_ID;
	message_buffer = 0;
	delete_send_port = false;
	this->validate_locking = validate_locking;
	socket = sock;
	sock->SetBlocking(false);
	sock->SetBlocking(true);
	cur_cmd = cur_token = NO_TOKEN;
}
#endif

/*--------------------------------------------------------------------*/

static BString make_lock_name(long s_port, long r_port)
{
	char lockName[32];
	sprintf(lockName, "p:%ld<->%ld:%ld",s_port,r_port,be_app->Team());
	return BString(lockName);
}

AppSession::AppSession(long s_port, long r_port, bool validate_locking)
	: session_lock(make_lock_name(s_port, r_port).String())
{
	message	 a_message;

	r_message = (message*)malloc(MAX_MSG_SIZE);
	s_message = (message*)malloc(MAX_MSG_SIZE);
	session_id = new_id();

	cur_pos_send = 4;
	cur_pos_receive = MAX_MSG_SIZE;
	receive_size = MAX_MSG_SIZE;
	send_port = s_port;
	receive_port = r_port;
	message_buffer = 0;
	delete_send_port = false;
	this->validate_locking = validate_locking;
	socket = NULL;
	cur_cmd = cur_token = NO_TOKEN;

	a_message.what = INIT_SESSION;
	a_message.parm1 = session_id;
	a_message.parm2 = r_port;
	
	while (write_port(s_port, 0, &a_message, 64) == B_INTERRUPTED)
		PRINT(("psend error1\n"));
}
	
/*---------------------------------------------------------------*/

static BString make_lock_name(const char *name)
{
	char lockName[32];
	sprintf(lockName, "p:%16.16s:%ld",name,be_app->Team());
	return BString(lockName);
}

AppSession::AppSession(long s_port, const char *name, bool validate_locking)
	: session_lock(make_lock_name(name).String())
{
	char portName[32];
	message	 a_message;

	r_message = (message*)malloc(MAX_MSG_SIZE);
	s_message = (message*)malloc(MAX_MSG_SIZE);
	session_id = new_id();

	cur_pos_send = 4;
	cur_pos_receive = MAX_MSG_SIZE;
	receive_size = MAX_MSG_SIZE;
	sprintf(portName, "p>%16.16s:%ld",name,be_app->Team());
	send_port = create_port(12, portName);
	sprintf(portName, "p<%16.16s:%ld",name,be_app->Team());
	receive_port = create_port(12, portName);
	delete_send_port = false;
	this->validate_locking = validate_locking;
	message_buffer = 0;
	socket = NULL;
	cur_cmd = cur_token = NO_TOKEN;

	state = 0;
	
	a_message.what = INIT_SESSION;
	a_message.parm1 = session_id;
	a_message.parm2 = receive_port;
	a_message.parm3 = send_port;
	
	while (write_port(s_port, 0, &a_message, 64) == B_INTERRUPTED)
		PRINT(("psend error1\n"));
}
	
/*---------------------------------------------------------------*/

AppSession::~AppSession()
{
	free(r_message);
	free(s_message);
}

/*-------------------------------------------------------------*/

int32 
AppSession::send_buffer()
{
	#ifndef REMOTE_DISPLAY
		return write_port(send_port, session_id, s_message, s_message->what);
	#else
		if (!socket) return write_port(send_port, session_id, s_message, s_message->what);
		return socket->Send(&s_message->parm1, s_message->what-4);
	#endif
}

int32 
AppSession::recv_buffer()
{
	#ifndef REMOTE_DISPLAY
		return read_port(receive_port, &m_msgCode, r_message, MAX_MSG_SIZE);
	#else
		if (!socket) return read_port(receive_port, &m_msgCode, r_message, MAX_MSG_SIZE);
		int32 r;
		r_message->what = MAX_MSG_SIZE-4;
		r = socket->Receive(&r_message->parm1,&r_message->what);
		r_message->what += 4;
		m_msgCode = session_id;
		return r;
	#endif
}

/*-------------------------------------------------------------*/
// this one is special since it will exit if the next message
// coming from the port is not for the current session but
// is a generic message.
// trust me.

long	AppSession::sread2(long size, void *buffer)
{
	long	size0;
	long	avail;
	char	started;

#if VALIDATE_LOCKING
	if (validate_locking) {
		if (!session_lock.IsLocked())
			debugger("AppSession::sread2() called without lock held!");
	}
#endif

#ifdef SYNC_CALL_LOG
	if (state == SESSION_WRITING) {
		CallStack stack;
		stack.Update(1);
		synchronous.AddToTree(&stack);
	};
	state = SESSION_READING;
#endif //SYNC_CALL_LOG

	started = 0;

	while (size > 0) {
		if (cur_pos_receive == receive_size) {
			do {
				while (recv_buffer() == B_INTERRUPTED)
					PRINT(("preceive error1\n"));
				if (m_msgCode != session_id) {
					add_to_buffer(r_message);
					if (started == 0)
						return(-1);
				}
			} while(m_msgCode != session_id);

			cur_pos_receive = 4;
			receive_size = r_message->what;
		}
		avail = receive_size - cur_pos_receive;
		
		if (size < avail)
			size0 = size;
		else
			size0 = avail;

		started = 1;
		memcpy2(buffer, ((char *)r_message) + cur_pos_receive, size0);
		
		size -= size0;
		buffer = (char *)buffer + size0;
		cur_pos_receive += size0;
	}
	return(0);
}			

/*-------------------------------------------------------------*/

void	AppSession::sread(long size, void *buffer)
{
	long	size0;
	long	avail;

#if VALIDATE_LOCKING
	if (validate_locking) {
		if (!session_lock.IsLocked())
			debugger("AppSession::sread() called without lock held!");
	}
#endif

#ifdef SYNC_CALL_LOG
	if (state == SESSION_WRITING) {
		CallStack stack;
		stack.Update(1);
		synchronous.AddToTree(&stack);
	};
	state = SESSION_READING;
#endif // SYNC_CALL_LOG

	while (size > 0) {
		if (cur_pos_receive == receive_size) {
			do {
				m_msgCode = 0;
				while (recv_buffer() == B_INTERRUPTED)
					PRINT(("preceive error1\n"));
				if (m_msgCode != session_id) {
					add_to_buffer(r_message);
				}
			} while(m_msgCode != session_id);

			cur_pos_receive = 4;
			receive_size = r_message->what;
		}
		avail = receive_size - cur_pos_receive;
		
		if (size < avail)
			size0 = size;
		else
			size0 = avail;

		memcpy2(buffer, ((char *)&(r_message->what)) + cur_pos_receive, size0);
		
		size -= size0;
		buffer = (char *)buffer + size0;
		cur_pos_receive += size0;
	}
}			


/*-------------------------------------------------------------*/

void	AppSession::sreadd(long size, void *buffer)
{
	long	size0;
	long	avail;

	while (size > 0) {
		PRINT(("size = %ld\n", size));
		if (cur_pos_receive == receive_size) {
			do {
				PRINT(("start receive\n"));
				while (recv_buffer() == B_INTERRUPTED)
					PRINT(("preceive error1\n"));
				if (m_msgCode != session_id) {
					add_to_buffer(r_message);
				}
			} while(m_msgCode != session_id);
			PRINT(("got one\n"));

			cur_pos_receive = 4;
			receive_size = r_message->what;
		}
		avail = receive_size - cur_pos_receive;
		
		if (size < avail)
			size0 = size;
		else
			size0 = avail;

		PRINT(("memcpy %ld\n", size0));
		memcpy2(buffer, ((char *)&(r_message->what)) + cur_pos_receive, size0);
		
		size -= size0;
		buffer = (char *)buffer + size0;
		cur_pos_receive += size0;
	}
}			

/*-------------------------------------------------------------*/

char	*AppSession::sread()
{
	short	len;
	char	*buffer;
	
	sread(2, &len);
	buffer = (char *)malloc(len);
	sread(len, buffer);
	return(buffer);
}

/*-------------------------------------------------------------*/

void	AppSession::sread(BString* into)
{
	short	len;
	
	sread(2, &len);
	if (len > 0) {
		char* buffer = into->LockBuffer(len-1);
		if (buffer) sread(len, buffer);
		else drain(len);
		into->UnlockBuffer(len-1);
	}
}

/*-------------------------------------------------------------*/

void	AppSession::sread(long size, session_buffer* buffer)
{
	void* buf = buffer->reserve(size);
	if (buf) sread(size, buf);
	else {
		buffer->reserve(0);
		drain(size);
	}
}

/*-------------------------------------------------------------*/

int32	AppSession::drain(int32 size)
{
	int32 r=0,s;
	char buf[256];
	while (size) {
		s = 256;
		if (size < s) s = size;
		sread(s,buf);
		size -= s;
	};
	return r;
}

/*-------------------------------------------------------------*/

void	AppSession::swrite(const char *a_string)
{
	short	len;

	len = strlen(a_string) + 1;
	swrite(2, &len);
	swrite(len, a_string);
}

/*-------------------------------------------------------------*/

void	AppSession::swrite(const session_buffer& buffer)
{
	swrite(buffer.size(), buffer.retrieve());
}

/*-------------------------------------------------------------*/

void	AppSession::swrite(long size, const void *buffer)
{
	long	free_space;
	long	size0;

#if VALIDATE_LOCKING
	if (validate_locking) {
		if (!session_lock.IsLocked())
			debugger("AppSession::swrite() called without lock held!");
	}
#endif

#ifdef SYNC_CALL_LOG
	state = SESSION_WRITING;
#endif // SYNC_CALL_LOG

	free_space = MAX_MSG_SIZE - cur_pos_send;
	if (free_space > size) {
		memcpy2(((char *)&(s_message->what)) + cur_pos_send, buffer, size);
		cur_pos_send += size;
		return;
	}

	while(size > 0) {
		if (size < free_space)
			size0 = size;	
		else
			size0 = free_space;

		memcpy2(((char *)&(s_message->what)) + cur_pos_send, buffer, size0);
		
		cur_pos_send += size0;
		buffer = (char *)buffer + size0;
		size -= size0;

		if (cur_pos_send == MAX_MSG_SIZE) {
			s_message->what = cur_pos_send;
			while (send_buffer() == B_INTERRUPTED)
				PRINT(("psend2 error\n"));
			cur_pos_send = 4;
		}
		free_space = MAX_MSG_SIZE - cur_pos_send;
	}
}

/*-------------------------------------------------------------*/

void*	AppSession::inplace_write(long size)
{
#if VALIDATE_LOCKING
	if (validate_locking) {
		if (!session_lock.IsLocked())
			debugger("AppSession::inplace_write() called without lock held!");
	}
#endif

	if (size > MAX_MSG_SIZE)
		return NULL;
	
#ifdef SYNC_CALL_LOG
	state = SESSION_WRITING;
#endif // SYNC_CALL_LOG

	if (size > (MAX_MSG_SIZE - cur_pos_send))
		flush();
	
	void* pos = ((char *)&(s_message->what)) + cur_pos_send;
	cur_pos_send += size;
	return pos;
}

/*-------------------------------------------------------------*/

void	AppSession::sread_rect(BRect *r)
{
	long	v[4];

	sread(4*sizeof(long), v);
	
	r->left = v[0];
	r->top  = v[1];
	r->right = v[2];
	r->bottom = v[3];
}

/*-------------------------------------------------------------*/

void	AppSession::sread_point(BPoint *p)
{
	long	v[2];

	sread(2*sizeof(long), v);
	
	p->x = v[0];
	p->y = v[1];
}

/*-------------------------------------------------------------*/

void	AppSession::swrite_point(const BPoint *p)
{
	long	v[2];

	v[0] = (long) p->x;
	v[1] = (long) p->y;

	swrite(2*sizeof(long), v);
}

/*-------------------------------------------------------------*/

void	AppSession::swrite_rect(const BRect *r)
{
	long	v[4];

	v[0] = (long) floor(r->left+0.5);
	v[1] = (long) floor(r->top+0.5);
	v[2] = (long) floor(r->right+0.5);
	v[3] = (long) floor(r->bottom+0.5);

	swrite(4*sizeof(long), v);
}

void	AppSession::swrite_rect(const clipping_rect *r)
{
	swrite(sizeof(clipping_rect), (void *)r);
}

/*-------------------------------------------------------------*/

void	AppSession::sread_coo(float *value)
{
	long	v;

	sread(4, &v);
	*value = v;
}

/*-------------------------------------------------------------*/

void	AppSession::swrite_coo(const float *value)
{
	long	v;

	v = (long) *value;

	swrite(4, &v);
}


/*-------------------------------------------------------------*/

static void	dump_ports()
{
	long	i;
	long	n;

	for (i = 0; i < 60; i++) {
		n = port_count(i);
		if (n > 2)
			printf("port %ld = %ld\n", i, n);
	}
	snooze(80000);
}

/*-------------------------------------------------------------*/

void	AppSession::flush_debug()
{
	long	i;


	if (cur_pos_send > 4) {
		
		i = 20;


		while ((i > 0) && (port_count(send_port) > 8)) {
			snooze(2000);
			i--;
		}

		s_message->what = cur_pos_send;
		PRINT(("sp = %ld\n", send_port));
		snooze(40000);
		while (send_buffer() == B_INTERRUPTED)
			PRINT(("psend3 error\n"));
		PRINT(("out %ld\n", (int) system_time()));
		snooze(40000);
		cur_pos_send = 4;
	}
}

/*-------------------------------------------------------------*/

void	AppSession::flush()
{
	if (cur_pos_send > 4) {
		s_message->what = cur_pos_send;
		while (send_buffer() == B_INTERRUPTED)
			PRINT(("psend3 error\n"));
		cur_pos_send = 4;
	}
}

/*-------------------------------------------------------------*/

void	AppSession::full_sync()
{
	long bid;
	swrite_l(GR_SYNC);
	flush();
	sread(4, &bid);
}

/*-------------------------------------------------------------*/

void	AppSession::xclose()
{
	flush();
}

/*-------------------------------------------------------------*/

void	AppSession::sclose()
{
	flush();
	if (delete_send_port) delete_port(send_port);
	delete_port(receive_port);
#if REMOTE_DISPLAY
	if (socket) delete socket;
#endif
}

/*-------------------------------------------------------------*/

bool	AppSession::lock() const
{
	return session_lock.Lock();
}

/*-------------------------------------------------------------*/

bool	AppSession::lock(int32 token, int32 cmd)
{
	if (!session_lock.Lock()) return false;
	select(token, cmd);
	return true;
}

/*-------------------------------------------------------------*/

void	AppSession::unlock() const
{
	if (!session_lock.IsLocked()) {
		debugger("AppSession::unlock() called when not held!");
		return;
	}
	session_lock.Unlock();
}

/*-------------------------------------------------------------*/

void	AppSession::select(int32 token, int32 cmd)
{
	if (token != NO_TOKEN && (cur_token != token || cur_cmd != cmd)) {
		cur_token = token;
		cur_cmd = cmd;
		swrite_l(cur_cmd);
		swrite_l(cur_token);
	}
}

/*-------------------------------------------------------------*/

void	AppSession::forget_last()
{
	cur_token = cur_cmd = NO_TOKEN;
}

/*-------------------------------------------------------------*/

void	AppSession::add_to_buffer(message *a_message)
{
	char	*new_one;
	char	*tmp;

	new_one = (char *)malloc(sizeof(message) + 4);
	memcpy2(new_one + 4, (char *)a_message, sizeof(message));
	*((long *)new_one) = 0;

	tmp = message_buffer;

	if (tmp == 0) {
		message_buffer = new_one;
		return;
	}
	else {
		while (*((long *)tmp)) {
			tmp = (char *)*((long *)tmp);
		}
		*((long *)tmp) = (long)new_one;
	}
}

/*-------------------------------------------------------------*/

long	AppSession::get_other(message *a_message)
{
	char	*tmp;

	if (message_buffer == 0)
		return(-1);
	
	memcpy2((char *)a_message, message_buffer + 4, sizeof(message));
	
	tmp = message_buffer;
	message_buffer =  (char *)*((long *)tmp);
	free(tmp);
	return(0);
}

/*-------------------------------------------------------------*/

AppSession	*session_list;

/*-------------------------------------------------------------*/

#ifdef SYNC_CALL_LOG

CallStack::CallStack()
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = 0;
};

#if !__INTEL__
static __asm unsigned long * get_caller_frame();

static __asm unsigned long *
get_caller_frame ()
{
	lwz     r3, 0 (r1)
	blr
}

#endif

#define bogus_addr(x)  ((x) < 0x80000000)

void CallStack::Update(int32 ignoreDepth)
{
	for (int32 i = 1; i <= CALLSTACK_DEPTH; i++) {
		m_caller[i - 1] = GetCallerAddress(i+ignoreDepth);
	};
};

unsigned long CallStack::GetCallerAddress(int level)
{
#if __INTEL__
	uint32 fp = 0, nfp, ret;

	level += 2;
	
	fp = get_stack_frame();
	if (bogus_addr(fp))
		return 0;
	nfp = *(ulong *)fp;
	while (nfp && --level > 0) {
		if (bogus_addr(fp))
			return 0;
		nfp = *(ulong *)fp;
		ret = *(ulong *)(fp + 4);
		if (bogus_addr(ret))
			break;
		fp = nfp;
	}

	return ret;
#else
	unsigned long *cf = get_caller_frame();
	unsigned long ret = 0;
	
	level += 1;
	
	while (cf && --level > 0) {
		ret = cf[2];
		if (ret < 0x80000000) break;
		cf = (unsigned long *)*cf;
	}

	return ret;
#endif
}

void CallStack::SPrint(char *buffer)
{
	char tmp[32];
	buffer[0] = 0;
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		sprintf(tmp, " 0x%lx", m_caller[i]);
		strcat(buffer, tmp);
	}
}

void CallStack::Print()
{
	char tmp[32];
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		DebugPrintf((" 0x%x", m_caller[i]));
		if (!m_caller[i]) break;
	}
}

CallStack &CallStack::operator=(const CallStack& from)
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = from.m_caller[i];
	return *this;
}

bool CallStack::operator==(CallStack s) const
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) if (m_caller[i] != s.m_caller[i]) return false;
	return true;
}

CallTreeNode::~CallTreeNode()
{
	PruneNode();
};

void CallTreeNode::PruneNode()
{
	for (int32 i=0;i<branches.CountItems();i++)
		delete branches[i];
	branches.SetItems(0);
};

void CallTreeNode::ShortReport()
{
	if (!parent) return;
	parent->ShortReport();
	if (parent->parent)
		DebugPrintf((", %08x",addr));
	else
		DebugPrintf(("%08x",addr));
};

void CallTreeNode::LongReport(char *buffer, int32 bufferSize)
{
	uint32 offs;
	if (!parent) return;
	parent->LongReport(buffer,bufferSize);
	demangle(lookup_symbol(addr,&offs),buffer,bufferSize);
	DebugPrintf(("  0x%08x: <%s>+0x%08x\n",addr,buffer,offs));
};

CallTree::CallTree(const char *name)
{
	addr = 0;
	count = 0;
	higher = lower = highest = lowest = parent = NULL;
};

CallTree::~CallTree()
{
	Prune();
};

void CallTree::Prune()
{
	PruneNode();
	highest = lowest = higher = lower = NULL;
	count = 0;
};

void CallTree::Report(int32 rcount, bool longReport)
{
	char buffer[128];
	CallTreeNode *n = highest;
	DebugPrintf(("%d total hits, reporting top %d\n",count,rcount));
	DebugPrintf(("-------------------------------------------------\n"));
	while (n && rcount--) {
		if (longReport) {
			DebugPrintf(("%d hits-------------------------------\n",n->count));
			n->LongReport(buffer,128);
		} else {
			DebugPrintf(("%d hits --> ",n->count));
			n->ShortReport();
			DebugPrintf(("\n"));
		};
		n = n->lower;
	};
};

void CallTree::AddToTree(CallStack *stack)
{
	CallTreeNode *n,*next,*replace;
	int32 i,index = 0;

	if (!stack->m_caller[0]) return;

	n = this;
	while (stack->m_caller[index]) {
		for (i=0;i<n->branches.CountItems();i++) {
			next = n->branches[i];
			if (next->addr == stack->m_caller[index]) goto gotIt;
		};
		next = new CallTreeNode;
		next->addr = stack->m_caller[index];
		next->higher = NULL;
		next->lower = NULL;
		next->count = 0;
		next->parent = n;
		n->branches.AddItem(next);
		gotIt:
		n = next;
		index++;
		if (index == CALLSTACK_DEPTH) break;
	};
	if (n->count == 0) {
		n->higher = lowest;
		if (lowest) lowest->lower = n;
		else highest = n;
		lowest = n;
	};
	count++;
	n->count++;
	while (n->higher && (n->count > n->higher->count)) {
		replace = n->higher;
		replace->lower = n->lower;
		if (replace->lower == NULL) lowest = replace;
		else replace->lower->higher = replace;
		n->lower = replace;
		n->higher = replace->higher;
		if (n->higher == NULL) highest = n;
		else n->higher->lower = n;
		replace->higher = n;
	};
	
	if (!(count % LOG_OUTPUT_QUANTUM)) {
		Report(REPORT_TOP_COUNT,true);
	};
};

#endif // SYNC_CALL_LOG
