#include <Messenger.h>
#include <GDispatcher.h>
#include <GLooper.h>
#include <Autolock.h>
#include <String.h>
#include <atomic.h>
#include <Debug.h>
#include <stdio.h>
#include <malloc.h>
#include <fs_info.h>
#include <fs_attr.h>
#include <Binder.h>
#include <NodeMonitor.h>
#include "token.h"
#include "message_util.h"
#include "BinderPriv.h"

enum {
	dfQuitting = 0x00000001
};

#define GAVE_UP_ALPHA -2
#define LOOPER_DIE_TIMEOUT 1000000*5

#define checkpoint
//printf("thid=%d -- %s:%d -- %s\n",find_thread(NULL),__FILE__,__LINE__,__FUNCTION__);

BMessenger::BMessenger(GHandler *handler)
{
	thread_info thinfo;
	get_thread_info(find_thread(NULL),&thinfo);

	fTeam = thinfo.team;
	fPort = handler->Port();
	fHandlerToken = handler->Token();
	fPreferredTarget = false;
}

GDispatcher::GDispatcher(int32 maxThreads)
{
	m_maxThreads = maxThreads;
	m_threads = 0;
	m_pendingHandlers = NULL;
	m_activeHandlers = NULL;
	m_alphaThread = false;
	m_state = 0;
	m_pendingPort = create_port(100,"pendingPort");
	m_rcvbuf = malloc(1024);
	m_rcvbufSize = 1024;
	IncRefs();

	new GLooper(this);
	new GLooper(this);
}

GDispatcher::~GDispatcher()
{
	if (m_pendingPort) close_port(m_pendingPort);
}

void 
GDispatcher::RegisterGLooper(GLooper *)
{
	atomic_add(&m_threads,1);
}

void 
GDispatcher::UnregisterGLooper(GLooper *)
{
	if (atomic_add(&m_threads,-1) == 1)
		new GLooper(this);
	else {
		if (!m_alphaThread && !m_activeEvent.Fire(1))
			new GLooper(this);
	}
}

void GDispatcher::MaybeSpawn()
{
	m_lock.Lock();
	if (!m_alphaThread && !m_activeEvent.Fire(1)) {
		new GLooper(this);
	}
	m_lock.Unlock();
}

void GDispatcher::_delete()
{
	m_lock.Lock();
	m_state |= dfQuitting;
	close_port(m_pendingPort);
	m_activeEvent.Shutdown();
	m_lock.Unlock();
}

bool GDispatcher::InsertHandler(GHandler **handlerP, GHandler *handler)
{
	GHandler *p = *handlerP;

	if (!p) {
		*handlerP = handler;
		handler->m_next = handler->m_prev = NULL;
		return true;
	};

	bigtime_t nextMsg = handler->NextMessage();
	if (p->NextMessage() >= nextMsg) {
		(*handlerP)->m_prev = handler;
		*handlerP = handler;
		handler->m_next = p;
		handler->m_prev = NULL;
	} else {
		while (p->m_next && (p->m_next->NextMessage() < nextMsg)) p = p->m_next;
		handler->m_next = p->m_next;
		handler->m_prev = p;
		if (p->m_next) p->m_next->m_prev = handler;
		p->m_next = handler;
	};
	return false;
};

void GDispatcher::UnscheduleHandler(GHandler *handler)
{
	bool hadRef =  ((m_activeHandlers == handler) ||
					(m_pendingHandlers == handler) ||
					handler->m_prev || handler->m_next);

	if (hadRef) {
		if (m_activeHandlers == handler)	m_activeHandlers = handler->m_next;
		if (m_pendingHandlers == handler)	m_pendingHandlers = handler->m_next;
		if (handler->m_next) handler->m_next->m_prev = handler->m_prev;
		if (handler->m_prev) handler->m_prev->m_next = handler->m_next;
		handler->m_prev = handler->m_next = NULL;
	}

	if (hadRef) handler->DecRefs(this);
}

void GDispatcher::ScheduleHandler(GHandler *handler)
{
	thread_id me = find_thread(NULL);

	if (m_alphaThread != me) m_lock.Lock();
		scheduling s = handler->StartSchedule();
		GHandler *oldPendingHandler = m_pendingHandlers;
		if (s != CANCEL_SCHEDULE) {
			bigtime_t stamp = handler->NextMessage();
			bigtime_t now = system_time();
			handler->IncRefs(this);
			UnscheduleHandler(handler);

			if ((stamp <= now) && (!m_pendingHandlers || (m_pendingHandlers->NextMessage() > stamp))) {
				InsertHandler(&m_activeHandlers,handler);
				if (!m_activeEvent.Fire(1)) {
					if (m_alphaThread && (m_alphaThread != me)) {
						write_port(m_pendingPort,0,NULL,0);
					} else if (m_threads < m_maxThreads) {
						new GLooper(this);
					};
				};
			} else
				InsertHandler(&m_pendingHandlers,handler);
			handler->DoneSchedule();
		} else
			UnscheduleHandler(handler);
		
		if ((oldPendingHandler != m_pendingHandlers) && m_alphaThread && (m_alphaThread != me))
			write_port(m_pendingPort,0,NULL,0);
		
	if (m_alphaThread != me) m_lock.Unlock();
}

/*
void GDispatcher::ScheduleHandler(GHandler *handler)
{
	int32 code=0;

	handler->IncRefs();

	m_lock.Lock();
	bigtime_t oldestStamp = handler->NextMessage();
	GHandler **p = &m_handlers;
	while (*p && ((*p)->NextMessage() < oldestStamp))
		p = &(*p)->m_nextHandler;
	handler->m_nextHandler = *p;
	*p = handler;
	m_nextStamp = m_handlers->NextMessage();
	m_lock.Unlock();

	DispatcherState oldVal=m_state,newVal;
	do {
		newVal = oldVal;
		newVal.requestAtom++;
		if ((oldVal.requestAtom == 0) &&
			(oldVal.loopers < m_maxThreads))
			newVal.loopers++;
	} while (!cmpxchgState(&m_state,&oldVal,newVal));

	if (oldVal.requestAtom < 0) write_port(m_port,code,NULL,0);
	if (newVal.loopers > oldVal.loopers) new GLooper(this);
}
*/

#if 0
char displayable(uint8 c)
{
	if ((c < 32) || (c > 127)) return '.';
	return c;
}

#define IO_SIZE 1024

class uspace_io {
	public:
		
		uint8 *buffer,stackBuffer[IO_SIZE];
		int32 pos,len,error,token,bufSize;
		uint8 *outBuffer,outStackBuffer[IO_SIZE];
		int32 outPos;
		port_id replyPort;
		
		void assert_size(int32 size);
		
		void write(BinderNode::property &prop) {
			int32 size = prop.FlattenedSize();
			assert_size(size+4);
			if (error) return;
			write(size);
			outPos += prop.FlattenTo(outBuffer+outPos);
		};
		void write(int32 intData) {
			assert_size(4);
			if (error) return;
			*((int32*)(outBuffer+outPos)) = intData;
			outPos += 4;
		};
		void write(USpaceFSCommand cmd) {
			write((int32)cmd);
		};
		void write(int64 intData) {
			assert_size(8);
			if (error) return;
			*((int64*)(outBuffer+outPos)) = intData;
			outPos += 8;
		};
		void write(const void *voidData, int32 dataLen) {
			assert_size(dataLen);
			if (error) return;
			memcpy(outBuffer+outPos,voidData,dataLen);
			outPos += dataLen;
		};
		void write(const char *stringData) {
			int32 slen = strlen(stringData);
			assert_size(slen+4);
			if (error) return;
			write(slen);
			memcpy(outBuffer+outPos,stringData,slen);
			outPos += slen;
		};
		
		void reset() { outPos = 0; pos = 0; };

		int32 reply(int32 replyError) {
			if (error) replyError = error;
			return write_port_etc(
						replyPort,
						replyError,
						outBuffer,outPos,
						B_TIMEOUT,
						2000000);
		};
/*
		int32 recv() {
			int32 lerror;
			ssize_t size;
			error = B_OK;
			if ((lerror = read_port_etc(
						recvPort,
						&replyPort,buffer,
						IO_SIZE,0,0)) < 0) return error = lerror;
			size = *((uint32*)buffer);
			if (size > IO_SIZE) return error = B_ERROR;
			len = size;
			pos = 4;
			return error;
		};
*/
		int32 read(int32 *intData) {
			if (error) return error;
			if ((pos+4) > len) return error=B_IO_ERROR;
			*intData = *((int32*)(buffer+pos));
			pos += 4;
			return B_OK;
		};
		int32 read(int64 *intData) {
			if (error) return error;
			if ((pos+8) > len) return error=B_IO_ERROR;
			*intData = *((int64*)(buffer+pos));
			pos += 8;
			return B_OK;
		};
		int32 read(void *voidData, int32 dataLen) {
			if (error) return error;
			if ((pos+dataLen) > len) return error=B_IO_ERROR;
			memcpy(voidData,buffer+pos,dataLen);
			pos += dataLen;
			return B_OK;
		};
		int32 read(char **stringData) {
			if (error) return error;
			int32 slen,lerror;
			if ((lerror=read(&slen))) return error=lerror;
			*stringData = (char*)malloc(slen+1);
			if ((lerror=read(*stringData,slen))) {
				free(*stringData);
				*stringData = NULL;
				return error=lerror;
			};
			(*stringData)[slen] = 0;
			return B_OK;
		};
		int32 read(BinderNode::property &prop) {
			if (error) return error;
			int32 size;
			read(&size);
			if ((pos+size) > len) return error=B_IO_ERROR;
			pos += prop.UnflattenFrom(buffer+pos);
			return B_OK;
		};
		void dump_readbuf()
		{
			int32 start = 0;
			while (start < len) {
				int32 end = start + 16;
				if (end > len) end = len;
				for (int32 i=start;i<end;i++)
					printf("%02x  ",buffer[i]);
				printf("'");
				for (int32 i=start;i<end;i++)
					printf("%c",displayable(buffer[i]));
				printf("'\n");
				start += 16;
			}
		};

		uspace_io(void *buf) {
			buffer = stackBuffer;
			outBuffer = outStackBuffer;
			bufSize = IO_SIZE;
			error = 0;
			len = *((int32*)buf);
			if (len > IO_SIZE) buffer = (uint8*)malloc(len);
			memcpy(buffer,buf,len);
			pos = 4;
			read(&replyPort);
			outPos = 0;
		};

		~uspace_io() {
			if (buffer && (buffer != stackBuffer)) free(buffer);
			if (outBuffer && (outBuffer != outStackBuffer)) free(outBuffer);
		}

};


void uspace_io::assert_size(int32 size)
{
	if (error) return;
	if ((outPos + size) > bufSize) {
		if (outBuffer == outStackBuffer) {
			outBuffer = (uint8*)malloc(outPos+size);
			if (!outBuffer) {
				error = ENOMEM;
				return;
			}
			memcpy(outBuffer,outStackBuffer,outPos);
			bufSize = pos+size;
		} else {
			uint8 *newBuf = (uint8*)realloc(outBuffer,outPos+size);
			if (!newBuf) {
				error = ENOMEM;
				return;
			}
			outBuffer = newBuf;
			bufSize = outPos+size;
		}
	}
}

void 
GDispatcher::RegistryRequest()
{
	bool sync = false;
	bool startHosting = false;
	int32 token,cmd,err=B_ERROR;
	BinderNode::property prop;
	BinderNode *reg;
	uspace_io io(m_rcvbuf);

	m_alphaThread = 0;
	m_lock.Unlock();

	io.read(&token);
	GHandler *h = LookupToken(token);
	if (!h) {
		io.reply(ENOENT);
		m_lock.Lock();
		return;
	}

	reg = dynamic_cast<BinderNode*>(h);
	if (reg == NULL) {
		h->DecRefs(gDefaultTokens);
		io.reply(ENOENT);
		m_lock.Lock();
		return;
	}

	bool reply = true;
	io.read(&cmd);

//	printf("Command is %ld\n",cmd);
	
	switch (cmd) {
		case USPACEFS_CONNECT: {
			reg->Acquire();
			err = B_OK;
		} break;
		case USPACEFS_DISCONNECT:
		case USPACEFS_REMOVE_VNODE: {
			reg->Disconnect(this);
			err = B_OK;
			reply = false;
		} break;
		case USPACEFS_MOUNT: {
			int32 flags = B_FS_IS_SHARED;
			io.write(flags);
			io.write("registry");
			io.write("registryfs");
			reg->Acquire();
			sync = true;
//			startHosting = true;
			io.write((int32)find_thread(NULL));
			err = B_OK;
		} break;
		case USPACEFS_UNMOUNT: {
			reg->Release();
			err = B_OK;
		} break;
		case USPACEFS_OPENDIR: {
			int32 omode;
			void *cookie;
			io.read(&omode);
			err = reg->OpenProperties(&cookie,NULL);
			io.write(&cookie,sizeof(void*));
		} break;
		case USPACEFS_CLOSEDIR: {
			err = B_OK;
		} break;
		case USPACEFS_FREEDIRCOOKIE: {
			void *cookie;
			io.read(&cookie,sizeof(void*));
			err = reg->CloseProperties(cookie);
		} break;
		case USPACEFS_REWINDDIR: {
/*			void *cookie;
			io.read(&cookie,sizeof(void*));
			err = reg->RewindProperties(cookie);
*/			err = B_ERROR;
		} break;
		case USPACEFS_READDIR: {
			void *cookie;
			char str[1024];
			int32 len = 1024;
			io.read(&cookie,sizeof(void*));
			err = reg->NextProperty(cookie,str,&len);
			if (err == B_OK) io.write(str);
		} break;
		case USPACEFS_GETPROPERTY: {
			BinderNode::property_list args;
			BinderNode::property *theArgs = NULL;
			int32 argsSize,argCount,len,thid=-1;
			char *name;
			io.read(&name);
			io.read(&len);
			io.read(&argsSize);
			if (argsSize > 4) {
				io.read(&argCount);
//				printf("GETPROPERTY '%s' %ld %ld\n",name,argCount,argsSize);
				if (argCount < 0) argCount = 0;
				if (argCount) {
					theArgs = new BinderNode::property[argCount];
					for (int32 i=0;i<argCount;i++) {
						io.read(theArgs[i]);
						args.AddItem(&theArgs[i]);
					}
				}
			}
			
			/* Read args */
			get_status_t getErr = reg->ReadProperty(name,prop,args);
			if (!getErr.error) {
				if (prop.Type() == BinderNode::property::object)
					prop.Object()->StartHosting();
				if (prop.IsObject()) {
					thid = find_thread(NULL);
					sync = true;
				}
				if (prop == BinderNode::property::undefined) getErr.error = ENOENT;
			}

			io.write(&getErr,sizeof(getErr));
			io.write(thid);
			if (!getErr.error) io.write(prop);

			if (theArgs) delete [] theArgs;
			free(name);
			err = B_OK;
		} break;
		case USPACEFS_PUTPROPERTY: {
//			io.dump_readbuf();
			char *name;
			io.read(&name);
			io.read(prop);
			put_status_t putErr = reg->WriteProperty(name,prop);
			io.write(&putErr,sizeof(putErr));
			free(name);
			err = B_OK;
		} break;
		case USPACEFS_IOCTL: {
		} break;
		case USPACEFS_FSYNC: {
		} break;
		case USPACEFS_REDIRECT: {
			int32 flag;
			io.read(&flag);
			err = reg->SetPrivateFlags(pfRedirect,flag);
		} break;
		default: {
			err = B_ERROR;
		}
	}

	if (reply) {
		io.reply(err);
		if ((sync||startHosting) && !m_activeEvent.Fire(1)) {
			new GLooper(this);
		}
		if (sync) receive_data(&token,NULL,0);
	}

	if (startHosting) reg->StartHosting();
	
	reg->DecRefs(gDefaultTokens);
	m_lock.Lock();
}
#endif

status_t GDispatcher::ReadMsg(ssize_t size)
{
	if (size > m_rcvbufSize) {
		free(m_rcvbuf);
		m_rcvbuf = malloc(size);
		m_rcvbufSize = size;
	};
	
	BMessage *msg = NULL;
	BinderNode *binder;
	status_t err;
	int32 code,token;
	while ((err = read_port(m_pendingPort, &code, m_rcvbuf, m_rcvbufSize)) == B_INTERRUPTED);

	if (err >= 0) {
//		printf("code = '%.4s' ... %ld\n",&code,err);
		if (code == BINDER_REQUEST_CODE) {
			m_alphaThread = 0;
			BinderNode::ProcessMessage(m_rcvbuf, size, &m_lock);
			//RegistryRequest();
			m_lock.Lock();
			return GAVE_UP_ALPHA;
		} else if (code == STD_SEND_MSG_CODE) {
			uint32 what = *((uint32*)m_rcvbuf);
			if (what != 'PUSH') {
				msg = new BMessage();
				err = msg->Unflatten((char*)m_rcvbuf);
				if (err) {
					delete msg;
					msg = NULL;
				}
			}
	
			if (msg) {
				token = _get_message_target_(msg);
				if (token == NO_TOKEN)
					delete msg;
				else {
					GHandler *handler = LookupToken(token);
					if (handler) {
						if ((msg->what == B_NODE_MONITOR) &&
							(msg->FindInt32("opcode") == B_ATTR_CHANGED) &&
							((binder = dynamic_cast<BinderNode*>(handler)))) {
//							msg->PrintToStream();
							const char *data = msg->FindString("attr");
							if (data && (strlen(data) >= 8)) {
								uint32 event;
								sscanf(data,"%08lx",&event);
								m_alphaThread = 0;
								m_lock.Unlock();
//								printf("DoNotification(%08x,'%s')\n",event,data+8);
								binder->DoNotification(event,data+8);
								m_lock.Lock();
								delete msg;
								return GAVE_UP_ALPHA;
							}
							delete msg;
						} else {
//							msg->PrintToStream();
							handler->PostMessage(msg);
						}
						handler->DecRefs(gDefaultTokens);
					} else {
//						printf("what='%.4s'\n",&msg->what);
						delete msg;
					}
				}
			}
		}
	}
	
	return B_OK;
}

status_t GDispatcher::DispatchMessage()
{
	GHandler *handler;
	bigtime_t nextMsg,now;
	status_t err;

	m_lock.Lock();
	while (1) {
		GLooper::RegisterThis();
		if ((handler = m_activeHandlers)) {
			m_activeHandlers = handler->m_next;
			handler->m_next = handler->m_prev = NULL;
			if (m_activeHandlers) m_activeHandlers->m_prev = NULL;
			handler->DeferScheduling();
			m_lock.Unlock();
			handler->DispatchMessage();
			handler->DecRefs(this);
			m_lock.Lock();
		} else {
			if (!m_alphaThread) {
				int32 countTransferred = 0;
				nextMsg = B_INFINITE_TIMEOUT;
				now = system_time();
				while ((handler = m_pendingHandlers) &&
					   ((nextMsg = handler->NextMessage()) <= now)) {
					m_pendingHandlers = handler->m_next;
					handler->m_next = handler->m_prev = NULL;
					if (m_pendingHandlers) m_pendingHandlers->m_prev = NULL;
					InsertHandler(&m_activeHandlers,handler);
					countTransferred++;
				};
				
				if (countTransferred) {
					if (countTransferred > 1) {
						if (!m_activeEvent.Fire(countTransferred-1) &&
							(m_threads < m_maxThreads)) {
							new GLooper(this);
						};
					};
				} else {
					m_alphaThread = find_thread(NULL);
					m_lock.Unlock();
					while ((err = port_buffer_size_etc(m_pendingPort,B_ABSOLUTE_TIMEOUT,nextMsg)) == B_INTERRUPTED);
					m_lock.Lock();
					while (err >= 0) {
						err = ReadMsg(err);
						if (err) break;
						err = port_buffer_size_etc(m_pendingPort,B_TIMEOUT,0);
					};
					if (err != GAVE_UP_ALPHA) {
						m_alphaThread = 0;
						if ((err != B_TIMED_OUT) && (err != B_WOULD_BLOCK)) {
							PRINT(("Last thread exitting!\n"));
//							int32 threadsLeft = --m_threads;
//							uint32 state = m_state;
							m_lock.Unlock();
//							if (!threadsLeft && (state & dfQuitting)) delete this;
							return err;
						}
					}
				};
			} else {
				m_lock.Unlock();
				while ((err = m_activeEvent.Wait(system_time()+LOOPER_DIE_TIMEOUT)) == B_INTERRUPTED);
				m_lock.Lock();
				if (err) {
//					int32 threadsLeft = --m_threads;
//					uint32 state = m_state;
					m_lock.Unlock();
//					if (!threadsLeft && (state & dfQuitting)) delete this;
					return err;
				};
			};
		};
	};
	
	return B_ERROR;
};

port_id GDispatcher::Port()
{
	return m_pendingPort;
}

static void new_handler_token(int16 /*type*/, void* data)
{
	((GHandler*)data)->IncRefs(gDefaultTokens);
}

static bool get_handler_token(int16 /*type*/, void* data)
{
	((GHandler*)data)->IncRefs(gDefaultTokens);
	return true;
}

static void remove_handler_token(int16 /*type*/, void* data)
{
	((GHandler*)data)->DecRefs(gDefaultTokens);
}

int32 GDispatcher::ObtainToken(GHandler *handler)
{
	return gDefaultTokens->NewToken(100, handler,
									new_handler_token);
}

GHandler* GDispatcher::LookupToken(int32 token)
{
	if (token != NO_TOKEN) {
		GHandler* h = NULL;
		if (gDefaultTokens->GetToken(token, 100,
									 (void**)&h, get_handler_token) >= B_OK) {
			return h;
		}
	}
	
	return NULL;
}

void GDispatcher::InvalidateToken(int32 token)
{
	return gDefaultTokens->RemoveToken(token, remove_handler_token);
}

/*
status_t GDispatcher::DispatchMessage()
{
	int32 code,err,oldRequestAtom;
	DispatcherState oldVal=m_state,newVal;

	do {
		newVal = oldVal;
		newVal.requestAtom--;
	} while (!cmpxchgState(&m_state,&oldVal,newVal));

	if (oldVal.requestAtom < 0)
		err = read_port(m_port,&code,NULL,0);
	else if (oldVal.requestAtom == 0) {
		err = read_port_etc(m_port,&code,NULL,0,B_TIMEOUT,LOOPER_DIE_TIMEOUT);
		while (err == B_TIMED_OUT) {
			do {
				newVal = oldVal;
				if (oldVal.requestAtom < 0) {
					newVal.requestAtom++;
					newVal.loopers--;
				};
			} while (!cmpxchgState(&m_state,&oldVal,newVal));
			if (newVal.loopers < oldVal.loopers) return err;
			err = read_port_etc(m_port,&code,NULL,0,B_TIMEOUT,LOOPER_DIE_TIMEOUT);
		};
	};

	if (err == B_OK) {
		m_lock.Lock();
			GHandler *handler = m_handlers;
			if (!handler) debugger("Handler list is empty!");
			m_handlers = handler->m_nextHandler;
			handler->IncRefs();
		m_lock.Unlock();
		
		handler->DeferScheduling();
		handler->DispatchMessage();
		handler->ResumeScheduling();
	
		handler->DecRefs();
		
		return B_OK;
	};
	
	return err;
};
*/
