
#include <String.h>
#include <Binder.h>
#include <priv_syscalls.h>
#include <fs_attr.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <Debug.h>
//#include <BinderPriv.h>
#include <NodeMonitor.h>
#include <AssociativeArray.h>
#include <TLS.h>
#include "token.h"

#include <binder_driver.h>

enum {
	pfRedirect = 0x00000001
};

#define checkpoint_static
// printf("thid=%ld -- %s:%d -- %s\n",(int32)find_thread(NULL),__FILE__,__LINE__,__PRETTY_FUNCTION__);
#define checkpoint
//printf("thid=%ld (%08x) -- %s:%d -- %s\n",(int32)find_thread(NULL),this,__FILE__,__LINE__,__PRETTY_FUNCTION__);

static int
copystring(char *dest, const char *src, int maxlen)
{
	int len = 0;
	while(len < maxlen && *src != '\0') {
		*dest++ = *src++;
		len++;
	}
	*dest++ = '\0';
	return len;
}


BinderNode::property_list BinderNode::empty_arg_list;

static bool get_handler_token(int16 /*type*/, void* data)
{
	((GHandler*)data)->IncRefs(gDefaultTokens);
	return true;
}

#if 0
union registry_vnode_id {
	ino_t				value;
	struct {
		port_id			port;
		uint32			token;
	} address;
};
#endif

class _observer_data
{
	struct observer {
		atomref<BinderListener>			obj;
		uint32							mask;
		BString							name;
	};

	Gehnaphore				lock;
	SmartArray<observer>	list;

	public:

							_observer_data() {};
	
		int32				AddObserver(const atomref<BinderListener> &observer, uint32 eventMask, const char *name)
		{
			int32 i,token = B_ERROR;

			lock.Lock();
			i = list.AddItem();
			list[i].obj = observer;
			list[i].mask = eventMask;
			if (name) list[i].name = name;
			lock.Unlock();
			return token;
		}
		
		status_t			RemoveObserver(const atomref<BinderListener> &observer)
		{
			int32 i,err=B_ERROR;

			lock.Lock();
			for (i=0;i<list.Count();i++) {
				if (list[i].obj == observer) {
					list.RemoveItem(i--);
					err = B_OK;
				}
			}
			lock.Unlock();
			return err;
		}

		status_t			Notify(binder_node node, uint32 event, const char *name)
		{
			int32 i;
			SmartArray< atomref<BinderListener> > listCopy;

			lock.Lock();
			for (i=0;i<list.Count();i++) {
				if ((list[i].mask & event) &&
					!(list[i].mask & B_NAME_KNOWN) ||
					!(event & B_NAME_KNOWN) ||
					(list[i].name == name))
					listCopy.AddItem(list[i].obj);
			}
			lock.Unlock();

			for (i=0;i<listCopy.Count();i++)
				listCopy[i]->Overheard(node,event,(const char*)name);

			return B_OK;
		}
};

class BinderProxy : public BinderNode
{
	public:

												BinderProxy(uint32 descriptor);
		virtual									~BinderProxy();

		virtual	uint32							Flags() const;

		virtual	status_t						OpenProperties(void **cookie, void *copyFrom);
		virtual	status_t						NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t						CloseProperties(void *cookie);

		virtual	get_status_t					ReadProperty(const char *name, BinderNode::property &property, const BinderNode::property_list &args);
		virtual	put_status_t					WriteProperty(const char *name, const BinderNode::property &property);

		virtual	status_t						StartHosting() { return B_OK; }
		virtual	void							DoNotification(uint32 event, const char *name);

		virtual	void							Cleanup();
		virtual	binder_node						Copy(bool deep);
		
		void									StartWatching();

		uint32									m_descriptor;
		int32									m_flags;
		AssociativeArray<BString,property>		m_cache;
};


static Gehnaphore dummyProxyRefLock;
static binder_node dummyProxyRef;
static Gehnaphore rootProxyRefLock;
static binder_node rootProxyRef;
static int binderDesc = -1;
static uint32 rootNodeHandle = 0;

static int
open_driver()
{
	const char * driver_name = "/dev/misc/binder";
	int fd;
	int driver_api_version;
	
	fd = open(driver_name, O_RDWR | O_CLOEXEC);
	if(fd < 0) {
		printf("Binder: Could not open %s, %s\n", driver_name, strerror(errno));
		return -1;
	}
	driver_api_version = ioctl(fd, BINDER_GET_API_VERSION, 0);
	if(driver_api_version != CURRENT_BINDER_API_VERSION) {
		printf("Binder: driver api mismatch. Driver uses version %d, "
		       "libbe uses version %d\n", driver_api_version,
		       CURRENT_BINDER_API_VERSION);
		close(fd);
		return -1;
	}
	return fd;
}

BinderNode *dummyNode()
{
	if (dummyProxyRef) return dummyProxyRef;
	dummyProxyRefLock.Lock();
	if (!dummyProxyRef) dummyProxyRef = new BinderProxy(0);
	dummyProxyRefLock.Unlock();
	return dummyProxyRef;
}

BinderNode *
BinderNode::rootNode()
{
	if(rootNodeHandle == 0) {
		rootProxyRefLock.Lock();
		if(binderDesc < 0) {
			binderDesc = open_driver();
		}
		if(binderDesc >= 0 && rootNodeHandle == 0) {
			status_t err;
			binder_cmd cmd;
			cmd.command = BINDER_GET_ROOT_NODE_ID;
			err = BinderNode::do_binder_command(&cmd);
			if(err >= B_OK) {
				err = open_node(cmd.data.nodeid, &rootNodeHandle);
				if(err >= B_OK)
					rootProxyRef = new BinderProxy(rootNodeHandle);
				else
					rootNodeHandle = 0;
			}
		}
		rootProxyRefLock.Unlock();
	}
	return rootProxyRef;
}
/*
static Gehnaphore gBinderDispatcherLock;
static dispatcher gBinderDispatcher;

static GDispatcher *BinderDispatcher()
{
	GDispatcher *d = NULL;
	gBinderDispatcherLock.Lock();
	if ((GDispatcher*)gBinderDispatcher == NULL) {
		gBinderDispatcher = new GDispatcher();
	}
	d = gBinderDispatcher;
	gBinderDispatcherLock.Unlock();
	return d;
}
*/

// extern int32 GLooper_TLS;

/* Most of this nasty stuff can be avoided by a smarter implementation of binderfs */
/* And, indeed, it now is. */
/*
void avoid_deadlock(int32 desc)
{
	struct stat st;
	dev_t devid = -1;
	registry_vnode_id vnid; vnid.value = -1;
	if ((GLooper_TLS != -1) && !_krstat_(desc, NULL, &st, FALSE)) {
		devid = st.st_dev;
		vnid.value = st.st_ino;
		GDispatcher *dispatcher = (GDispatcher*)tls_get(GLooper_TLS);
		if (dispatcher && (dispatcher->Port()==vnid.address.port)) dispatcher->MaybeSpawn();
	}
}

void avoid_deadlock_vnid(registry_vnode_id vnid)
{
	if (GLooper_TLS != -1) {
		GDispatcher *dispatcher = (GDispatcher*)tls_get(GLooper_TLS);
		if (dispatcher && (dispatcher->Port()==vnid.address.port)) dispatcher->MaybeSpawn();
	}
}
*/

BinderNode * lookup_node(int32 token)
{
	if (token != NO_TOKEN) {
		GHandler* h = NULL;
		if (gDefaultTokens->GetToken(token, 100, (void**)&h, get_handler_token) >= B_OK) {
			BinderNode *n = dynamic_cast<BinderNode*>(h);
			if (n) return n;
			if (h) h->DecRefs(gDefaultTokens);
		}
	}
	
	return NULL;
}

void handle_punt(reflection_struct &/*reflect*/)
{
}

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

		int32 reply() {
			return write_port_etc(
						replyPort,
						BINDER_REPLY_CODE,
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

		uspace_io(void *buf, size_t buf_size) {
			buffer = stackBuffer;
			outBuffer = outStackBuffer;
			bufSize = IO_SIZE;
			error = 0;
			len = *((int32*)buf);
			if(len != (int)buf_size) {
				printf("uspace_io bad bufsize %d != %d\n", (int)len, (int)buf_size);
			}
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

/* always release buffer_lock */
void 
BinderNode::ProcessMessage(void *buf, size_t buf_size, Gehnaphore *buffer_lock)
{
	port_id sync_port = -1;
	bool startHosting = false;
	bool reply_registered = false;
	int32 token,cmd,err=B_ERROR;
	BinderNode::property prop;
	BinderNode *reg;
	uspace_io io(buf, buf_size);

	if(buffer_lock != NULL)
		buffer_lock->Unlock();
	
	io.read(&token);
	reg = lookup_node(token);
	if (reg == NULL) {
		io.write((port_id)-1);
		io.write((status_t)ENOENT);
		io.reply();
		return;
	}

	bool reply = true;
	io.read(&cmd);

	if(io.replyPort >= 0) {
		if(rootNodeHandle == 0)
			rootNode();
		if(ioctl(binderDesc, BINDER_REGISTER_REPLY_THREAD, io.replyPort) < 0)
			printf("Binder: BINDER_REGISTER_REPLY_THREAD failed for port %d, "
			       "%s\n", (int)io.replyPort, strerror(errno));
		else
			reply_registered = true;
	}

	//printf("BinderNode::ProcessMessage, Command is %ld\n",cmd);
	
	switch (cmd) {
		case BINDER_REQUEST_CONNECT: {
			//printf("Binder: BINDER_REQUEST_CONNECT\n");
			reg->Acquire();
			err = B_OK;
			io.write((port_id)-1); // no sync
			io.write(err);
		} break;
		case BINDER_REQUEST_DISCONNECT: {
			//printf("Binder: BINDER_REQUEST_DISCONNECT\n");
			reg->Disconnect(NULL);
			err = B_OK;
			reply = false;
		} break;
		case BINDER_REQUEST_OPEN_PROPERTIES: {
			void *cookie;
			err = reg->OpenProperties(&cookie,NULL);
			io.write((port_id)-1); // no sync
			io.write(err);
			io.write(&cookie,sizeof(void*));
		} break;
		case BINDER_REQUEST_CLOSE_PROPERTIES: {
			void *cookie;
			io.read(&cookie,sizeof(void*));
			err = reg->CloseProperties(cookie);
			io.write((port_id)-1); // no sync
			io.write(err);
		} break;
		case BINDER_REQUEST_NEXT_PROPERTY: {
			void *cookie;
			char str[1024];
			int32 len = 1024;
			io.read(&cookie,sizeof(void*));
			err = reg->NextProperty(cookie,str,&len);
			//printf("BINDER_REQUEST_NEXT_PROPERTY: %s\n", strerror(err));
			io.write((port_id)-1); // no sync
			io.write(err);
			if(err == B_OK)
				io.write(str);
		} break;
		case BINDER_REQUEST_GET_PROPERTY: {
			//printf("BINDER_REQUEST_GET_PROPERTY\n");
			BinderNode::property_list args;
			BinderNode::property *theArgs = NULL;
			int32 argsSize,argCount;//,len;
			char *name;
			io.read(&name);
			//printf("BINDER_REQUEST_GET_PROPERTY name = %s\n", name);
			//io.read(&len);
			io.read(&argsSize);
			//printf("BINDER_REQUEST_GET_PROPERTY argsSize = %d\n", (int)argsSize);
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
		//printf("BINDER_REQUEST_GET_PROPERTY ok\n");
				if (prop.Type() == BinderNode::property::object)
					prop.Object()->StartHosting();
				if (prop.IsObject()) {
					sync_port = ioctl(binderDesc, BINDER_UNREGISTER_REPLY_THREAD_AND_GET_SYNC_PORT, io.replyPort);
					if(sync_port >= 0)
						reply_registered = false;
					else
						printf("Binder: BINDER_GET_SYNC_REPLY_PORT failed for port %d, "
						       "%s\n", (int)io.replyPort, strerror(errno));
				}
				if (prop == BinderNode::property::undefined) getErr.error = ENOENT;
			}

			io.error = B_OK;
			io.write(sync_port);
			io.write(&getErr,sizeof(getErr));
			if (!getErr.error) io.write(prop);

			if (theArgs) delete [] theArgs;
			free(name);
			err = B_OK;
		} break;
		case BINDER_REQUEST_PUT_PROPERTY: {
//			io.dump_readbuf();
			char *name;
			io.read(&name);
			io.read(prop);
			put_status_t putErr = reg->WriteProperty(name,prop);
			io.write((port_id)-1); // no sync
			io.write(&putErr,sizeof(putErr));
			free(name);
			err = B_OK;
		} break;
		case BINDER_REQUEST_REDIRECT: {
			int32 flag;
			io.read(&flag);
			err = reg->SetPrivateFlags(pfRedirect,flag);
			io.write((port_id)-1); // no sync
			io.write(err);
		} break;
		case BINDER_REQUEST_NOTIFY: {
			int32 event;
			int32 event_size;
			char *data;
			io.read(&data);
			io.read(&event_size);
			if(event_size == 4) {
				io.read(&event);
				reg->DoNotification(event, data);
				err = B_OK;
			}
			else
				err = B_ERROR;
			io.write((port_id)-1); // no sync
			io.write(err);
		} break;
		default: {
			printf("BinderNode::ProcessMessage: got unknown request %d\n",
			       (int)cmd);
			err = B_ERROR;
			io.write((port_id)-1); // no sync
			io.write(err);
		}
	}
	
	//printf("BinderNode::ProcessMessage, Command %ld completed\n", cmd);

	if(reply != (io.replyPort >= 0))
		printf("BinderNode::ProcessMessage, error reply %d when replyPort %d\n",
		       reply, (int)io.replyPort);

	if(sync_port >= 0 && !reply)
		printf("BinderNode::ProcessMessage, error reply %d when sync_port %d\n",
		       reply, (int)sync_port);

	if(reply_registered) {
		if(ioctl(binderDesc, BINDER_UNREGISTER_REPLY_THREAD, io.replyPort) < 0)
			printf("Binder: BINDER_UNREGISTER_REPLY_THREAD failed for port %d, "
			       "%s\n", (int)io.replyPort, strerror(errno));
		else
			reply_registered = false;
	}

	if (reply) {
		//printf("Binder: reply to cmd %d, len %d\n", (int)cmd, (int)io.outPos);
		io.reply();
//		if ((sync||startHosting) && !m_activeEvent.Fire(1)) {
//			m_threads++;
//			new GLooper(this);
//		}

		if(sync_port >= 0) {
			//printf("BinderNode::ProcessMessage, sync on port %d\n",
			//       (int)sync_port);
			//receive_data(&token,NULL,0);
			while(1) {
				status_t err;
				int32 code;
				ssize_t buf_size;
				do
					err = buf_size = port_buffer_size_etc(sync_port,
						B_RELATIVE_TIMEOUT, 10000000);
				while(err == B_INTERRUPTED);
				if(err < B_OK) {
					printf("BinderNode::ProcessMessage, error waiting for sync,"
					       " %s\n", strerror(err));
					goto err;
				}
				if(buf_size != 0) {
					printf("BinderNode::ProcessMessage, got bad message while "
					       "waiting for sync, size %d\n", (int)buf_size);
					goto err;
				}
				do
					err = read_port(sync_port, &code, NULL, 0);
				while(err == B_INTERRUPTED);
				if(code == BINDER_SYNC_CODE) {
					//printf("BinderNode::ProcessMessage, got sync on port %d\n",
					//       (int)sync_port);
					break;
				}
				if(code != BINDER_PUNT_CODE) {
					printf("BinderNode::ProcessMessage, got bad message type "
					       "while waiting for sync, %d\n", (int)code);
					goto err;
				}
				do
					err = buf_size = port_buffer_size_etc(sync_port,
						B_RELATIVE_TIMEOUT, 10000000);
				while(err == B_INTERRUPTED);
				if(err < B_OK) {
					printf("BinderNode::ProcessMessage, error waiting for punt "
					       "command, %s\n", strerror(err));
					goto err;
				}
				void *buf = malloc(buf_size);
				if(buf == NULL) {
					printf("BinderNode::ProcessMessage, out of memory\n");
					goto err;
				}
				do
					err = read_port(sync_port, &code, buf, buf_size);
				while(err == B_INTERRUPTED);
				if(code != BINDER_REQUEST_CODE) {
					printf("BinderNode::ProcessMessage, got bad message type "
					       "while waiting for punt command, %d\n", (int)code);
					free(buf);
					goto err;
				}
				//printf("BinderNode::ProcessMessage, processing command while waiting for sync\n");
				ProcessMessage(buf, buf_size);
				//printf("BinderNode::ProcessMessage, processing command done\n");
				free(buf);
			}
		}
	}

	if (startHosting) reg->StartHosting();
err:
	if(sync_port >= 0)
		ioctl(binderDesc, BINDER_FREE_SYNC_REPLY_PORT, sync_port);
	//int rc =
	reg->DecRefs(gDefaultTokens);
	//printf("BinderNode::ProcessMessage, obj %p ref count = %d\n", reg, rc - 1);
//	m_lock.Lock();
}

status_t
BinderNode::handle_reflect_punt(binder_cmd *cmd)
{
	status_t err;
	//printf("REFLECT_CMD_PUNT:\n");
	ssize_t buf_size;
	do
		err = buf_size = port_buffer_size_etc(cmd->reflection.token, B_RELATIVE_TIMEOUT, 1000000);
	while(err == B_INTERRUPTED);
	if(err < B_OK)
		return err;
	void *buf = malloc(buf_size);
	if(buf == NULL)
		return B_NO_MEMORY;
	int32 code;
	do
		err = read_port(cmd->reflection.token, &code, buf, buf_size);
	while(err == B_INTERRUPTED);
	if(code != BINDER_REQUEST_CODE)
		printf("BinderNode::handle_reflect_punt, bad message type %d\n", (int)code);
	else
		ProcessMessage(buf, buf_size);
	free(buf);
	return B_OK;
}

int BinderNode::do_binder_command(binder_cmd *cmd, property *prop, const property_list *args)
{
	status_t err;
	cmd->reflection.cookie = 0;
	while((!(err = _kioctl_(binderDesc,BINDER_CMD,cmd,sizeof(binder_cmd)))) &&
	      cmd->reflection.active) {
		if(cmd->reflection.command == REFLECT_CMD_PUNT) {
			handle_reflect_punt(cmd);
			cmd->reflection.reflection_result = B_OK;
			continue;
		}
		if(cmd->reflection.command == REFLECT_CMD_REALLOCATE_GET_BUFFER) {
			void *new_return_buf = malloc(cmd->data.get.returnBufSize);
			if(new_return_buf == NULL) {
				cmd->reflection.reflection_result = B_NO_MEMORY;
				break;
			}
			cmd->data.get.returnBuf = new_return_buf;
			cmd->reflection.reflection_result = B_OK;
			continue;
		}
		BinderNode *n = lookup_node(cmd->reflection.token);
		cmd->reflection.reflection_result = B_BAD_VALUE;
		if(n == NULL) {
			printf("Binder: no node for reflection token %d\n",
			       (int)cmd->reflection.token);
			continue;
		}
		switch (cmd->reflection.command) {
			case REFLECT_CMD_GET:
				if (!prop || !args) {
					printf("Got unexpected get reflect on non-get operation!\n");
					cmd->reflection.reflection_result = err = B_BAD_VALUE;
					break;
				}
				//printf("Successfully reflecting get of '%s'\n",cmd->data.get.name);
				cmd->data.get.result = n->ReadProperty(cmd->data.get.name,*prop,*args);
				if(cmd->data.get.result.error == B_OK)
					cmd->data.get.successful_reflect = true;
				cmd->reflection.reflection_result = cmd->data.get.result.error;
				break;

			case REFLECT_CMD_PUT:
				if (!prop) {
					printf("Got unexpected put reflect on non-put operation!\n");
					break;
				}
				//printf("Successfully reflecting put of '%s'\n",cmd->data.put.name);
				cmd->data.put.result = n->WriteProperty(cmd->data.put.name,*prop);
				cmd->reflection.reflection_result = cmd->data.put.result.error;
				break;
			
			case REFLECT_CMD_REDIRECT:
			case REFLECT_CMD_UNREDIRECT:
			case REFLECT_CMD_RELEASE:
			case REFLECT_CMD_ACQUIRE: {
				const char *op = "unknown";
				switch (cmd->reflection.command) {
					case REFLECT_CMD_ACQUIRE:		op = "acquire"; n->Acquire(); break;
					case REFLECT_CMD_RELEASE:		op = "release"; n->Release(); break;
					case REFLECT_CMD_REDIRECT:		op = "redirect"; n->SetPrivateFlags(pfRedirect,true); break;
					case REFLECT_CMD_UNREDIRECT:	op = "unredirect"; n->SetPrivateFlags(pfRedirect,false); break;
					default: break;
				}
				//printf("Successfully reflected %s\n",op);
				cmd->reflection.reflection_result = B_OK;
			} break;
			
			case REFLECT_CMD_OPEN_PROPERTIES:
				//printf("Got REFLECT_CMD_OPEN_PROPERTIES reflection\n");
				cmd->reflection.reflection_result =
					n->OpenProperties(&cmd->data.property_iterator.local_cookie, NULL);
				if(cmd->reflection.reflection_result < B_OK)
					break; 
				/* fall through */
			
			case REFLECT_CMD_NEXT_PROPERTY:
				//printf("Got REFLECT_CMD_NEXT_PROPERTY reflection\n");
				cmd->data.property_iterator.len = PROPERTY_NAME_LEN-1;
				cmd->reflection.reflection_result =
					n->NextProperty(cmd->data.property_iterator.local_cookie,
					                cmd->data.property_iterator.name,
					                &cmd->data.property_iterator.len);
				break;

			case REFLECT_CMD_CLOSE_PROPERTIES:
				//printf("Got BINDER_CLOSE_PROPERTIES reflection\n");
				cmd->reflection.reflection_result =
					n->CloseProperties(cmd->data.property_iterator.local_cookie);
				break;
			
			case REFLECT_CMD_NOTIFY:
				if(cmd->command == BINDER_NOTIFY_CALLBACK) {
					n->DoNotification(cmd->data.notify.event,
					                  cmd->data.notify.name);
				}
				else {
					n->DoNotification(B_SOMETHING_CHANGED, NULL);
				}
				cmd->reflection.reflection_result = B_OK;
				break;
			
			default:
				printf("Got unexpected reflection! %d\n",
				       (int)cmd->reflection.command);
				break;
		}
		n->DecRefs(gDefaultTokens);
	}
	return err;
}

void BinderNode::Disconnect(void *tag)
{
	if(m_hostingNodeHandle != 0) {
		status_t err;
		binder_cmd cmd;

		cmd.command = BINDER_STOP_HOSTING;
		cmd.node_handle = m_hostingNodeHandle;
		err = do_binder_command(&cmd);
		if(err < B_OK) {
			printf("Binder: Disconnect aborted\n");
		}
		else {
			//printf("Disconnect!\n");
			//printf("Binder(%d): node %d-%d stopped hosting\n",
			//       (int)find_thread(NULL), (int)Port(), (int)Token());
			m_privateFlags &= ~pfRedirect;
			m_hostingNodeHandle = 0;
		}
	}
	Release(tag);
}

status_t
BinderNode::open_node(struct binder_node_id node_id, uint32 *node_handle, bool startHosting)
{
	status_t err;
	binder_cmd cmd;

	cmd.command = startHosting ? BINDER_START_HOSTING : BINDER_CONNECT;
	cmd.data.connect.node_id = node_id;
	cmd.data.connect.source_node_handle = 0;
	err = do_binder_command(&cmd);
	if(err >= B_OK) {
		*node_handle = cmd.data.connect.return_node_handle;
	}
	return err;
}

get_status_t kGetProperty(uint32 desc, const char *name, BinderNode::property &prop, const BinderNode::property_list &args)
{
	binder_cmd cmd;
	char stack_buffer[256];
	cmd.command = BINDER_GET_PROPERTY;
	copystring(cmd.data.get.name, name, sizeof(cmd.data.get.name) - 1);
	if(desc == 0) {
		printf("Binder: kGetProperty: desc == 0\n");
		return B_ERROR;
	}
	cmd.node_handle = desc;
	cmd.data.get.return_node_handle = 0;
	cmd.data.get.returnBuf = stack_buffer;
	cmd.data.get.returnBufSize = sizeof(stack_buffer);
	
	*((int32*)cmd.data.get.returnBuf) = BinderNode::property::null;
	cmd.data.get.argsSize = 0;
	void *data = NULL;
	if (args.Count()) {
		cmd.data.get.argsSize = 4;
		for (int32 i=0;i<args.Count();i++) cmd.data.get.argsSize += args[i].FlattenedSize() + 4;
		uint8 *p = (uint8*)(data = malloc(cmd.data.get.argsSize));
		if(data == NULL)
			return B_NO_MEMORY;
		*((int32*)p) = args.Count(); p+=4;
		for (int32 i=0;i<args.Count();i++) {
			*((int32*)p) = args[i].FlattenedSize(); p+=4;
			p += args[i].FlattenTo(p);
		}
	}
	cmd.data.get.args = data;

	cmd.data.get.successful_reflect = false;
	int32 err = BinderNode::do_binder_command(&cmd,&prop,&args);

	if (err == 0 && !cmd.data.get.successful_reflect) {
		if (cmd.data.get.return_node_handle != 0) {
			prop.Undefine();
			
			binder_node_id node_id;
			node_id = *((binder_node_id*)(((uint8*)cmd.data.get.returnBuf)+4+sizeof(dev_t)));

			port_info pi;
			thread_info ti;
			GHandler* h = NULL;
			get_port_info(node_id.port,&pi);
			get_thread_info(find_thread(NULL),&ti);
//				printf("************ %d,%d,%d\n",pi.team,ti.team,vnid.address.token);
			if ((pi.team == ti.team) && (node_id.token != (uint32)NO_TOKEN) &&
				(gDefaultTokens->GetToken(node_id.token, 100, (void**)&h, get_handler_token) >= B_OK) && h) {
				prop.m_type = BinderNode::property::object;
				prop.m_value.object = (BinderNode*)h;
				prop.m_format = BinderNode::property::f_object;
				prop.m_value.object->Acquire(&prop);
				prop.m_value.object->DecRefs(gDefaultTokens);
//					printf("grabbed local object for '%s'\n",name);
				binder_cmd dcmd;
				dcmd.command = BINDER_DISCONNECT;
				dcmd.node_handle = cmd.data.get.return_node_handle;
				BinderNode::do_binder_command(&dcmd);
			} else {
//					printf("grabbed remote object for '%s'\n",name);
				prop.m_type = BinderNode::property::object;
				prop.m_format = BinderNode::property::f_descriptor;
				prop.m_value.descriptor = cmd.data.get.return_node_handle;
			}
		} else
			prop.UnflattenFrom(cmd.data.get.returnBuf);
	}
	if(cmd.data.get.returnBuf != stack_buffer)
		free(cmd.data.get.returnBuf);
	if (data) free(data);
	if (err) cmd.data.get.result.error = err;
	return cmd.data.get.result;
}

put_status_t kPutProperty(uint32 desc, const char *name, const BinderNode::property &property)
{
	binder_cmd cmd;
	cmd.command = BINDER_PUT_PROPERTY;

	if(desc == 0) {
		printf("Binder: kPutProperty: desc == 0\n");
		return B_ERROR;
	}

	copystring(cmd.data.put.name, name, sizeof(cmd.data.put.name) - 1);
	cmd.node_handle = desc;
	void *data = cmd.data.put.value = malloc(cmd.data.put.valueLen = property.FlattenedSize());
	if(data == NULL)
		return B_NO_MEMORY;

	property.FlattenTo(data);

	int32 err = BinderNode::do_binder_command(&cmd,const_cast<BinderNode::property*>(&property));

	if (data) free(data);
	if (err) cmd.data.put.result.error = err;
	return cmd.data.put.result;
}

void BinderNode::BindMeUp1() {};
void BinderNode::BindMeUp2() {};
void BinderNode::BindMeUp3() {};
void BinderNode::BindMeUp4() {};
void BinderNode::BindMeUp5() {};
void BinderNode::BindMeUp6() {};
void BinderNode::BindMeUp7() {};
void BinderNode::BindMeUp8() {};

BinderNode::BinderNode() : GHandler()
{
	m_privateFlags = 0;
	m_hostingNodeHandle = 0;
	m_observerData = NULL;
}

BinderNode::BinderNode(BinderNode *, bool) : GHandler()
{
	m_privateFlags = 0;
	m_hostingNodeHandle = 0;
	m_observerData = NULL;
}

BinderNode::~BinderNode()
{
	if(m_hostingNodeHandle != 0) {
		status_t err;
		binder_cmd cmd;
		cmd.command = BINDER_STOP_HOSTING;
		cmd.node_handle = m_hostingNodeHandle;
		err = do_binder_command(&cmd);
		if(err >= B_OK)
			m_hostingNodeHandle = 0;
	}
}

void BinderNode::Cleanup()
{
	if(m_hostingNodeHandle != 0) {
		status_t err;
		binder_cmd cmd;
		printf("Binder: Cleanup called on hosted node\n");
		cmd.command = BINDER_STOP_HOSTING;
		cmd.node_handle = m_hostingNodeHandle;
		err = do_binder_command(&cmd);
		if(err >= B_OK) {
			m_privateFlags &= ~pfRedirect;
			m_hostingNodeHandle = 0;
		}
	}
	GHandler::Cleanup();
}

struct mountparms {
	port_id				port;
	int32				token;
};

status_t
BinderNode::Mount(const char *path)
{
	printf("BinderNode::Mount...\n");
	if(strcmp(path, "/binder") != 0) {
		printf("BinderNode::Mount... failed, %s is not the root path\n", path);
		return B_ERROR;
	}
	status_t err;

	rootProxyRefLock.Lock();
	if(binderDesc < 0) {
		binderDesc = open_driver();
	}

	err = StartHosting();
	if(err < B_OK)
		goto err;

	if(rootNodeHandle != 0) {
		binder_cmd cmd;
		cmd.command = BINDER_DISCONNECT;
		cmd.node_handle = rootNodeHandle;
		BinderNode::do_binder_command(&cmd);
		rootNodeHandle = 0;
	}
	binder_cmd cmd;
	cmd.command = BINDER_SET_ROOT_NODE_ID;
	cmd.data.nodeid.port = Port();
	cmd.data.nodeid.token = Token();
	err = do_binder_command(&cmd);
err:
	rootProxyRefLock.Unlock();
	return err;
}

status_t
BinderNode::StartHosting()
{
	int32 err = B_OK;
	if(m_hostingNodeHandle == 0) {
		binder_node_id node_id;
		node_id.port = Port();
		node_id.token = Token();
		err = open_node(node_id, &m_hostingNodeHandle, true);
		//if(err >= B_OK)
		//	printf("Binder(%d): node %d-%d started hosting\n",
		//	       (int)find_thread(NULL), (int)Port(), (int)Token());
	}
	return err;
}

status_t 
BinderNode::SetPrivateFlags(uint32 flag, bool on)
{
	uint32 flags = m_privateFlags;

	if(on && (flag & pfRedirect) && m_hostingNodeHandle == 0) {
		printf("Binder: SetPrivateFlags pfRedirect on unhosted node\n");
		return B_BAD_VALUE;
	}

	if (on) flags |= flag;
	else flags &= ~flag;
	
	m_privateFlags = flags;
	return B_OK;
}

BinderNode::property 
BinderNode::Root()
{
	return rootNode();
//	new BinderProxy(open("/binder",O_RDWR|O_CLOEXEC));
}

BinderNode::property 
BinderNode::Property(const char *path, BinderNode::property_list &args)
{
	BinderNode::property prop;
	GetProperty(path,prop,args);
	return prop;
}

BinderNode::property 
BinderNode::Property(const char *path, const BinderNode::property *argList, ...)
{
	va_list vl;
	BinderNode::property *arg;
	BinderNode::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BinderNode::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BinderNode::property *))) { args.AddItem(arg); i++; };
	va_end(vl);
	
	return Property(path,args);
}

void
BinderNode::AddObserver(const atomref<BinderListener> &observer, uint32 observeMask, const char *name)
{
	if (!m_observerData) m_observerData = new _observer_data;
	BinderProxy *proxy = dynamic_cast<BinderProxy*>(this);
	if (proxy) proxy->StartWatching();
	m_observerData->AddObserver(observer,observeMask,name);
}

status_t 
BinderNode::RemoveObserver(const atomref<BinderListener> &observer)
{
	if (!m_observerData) return B_ERROR;
	return m_observerData->RemoveObserver(observer);
}

class BinderListenerToCallback : public BinderListener {

	public:

		void *							m_userData;
		BinderNode::observer_callback 	m_callbackFunc;
			
		BinderListenerToCallback::BinderListenerToCallback(
			binder_node node,
			void *userData,
			BinderNode::observer_callback callbackFunc,
			uint32 observeMask,
			const char *name) {
			m_userData = userData;
			m_callbackFunc = callbackFunc;
			StartListening(node,observeMask,name);
		}
		
		status_t Overheard(binder_node , uint32 observed, BString name)
		{
			return m_callbackFunc(m_userData,observed,(void*)name.String());
		}
};

BinderNode::observer_token 
BinderNode::AddObserverCallback(void *userData, observer_callback callbackFunc, uint32 observeMask, const char *name)
{
	return (observer_token) new BinderListenerToCallback(this,userData,callbackFunc,observeMask,name);
}

status_t 
BinderNode::RemoveObserverCallback(observer_token observer)
{
	BinderListener *ptr = (BinderListener*)observer;
	atomref<BinderListener> obj = ptr;
	return m_observerData->RemoveObserver(obj);
}

status_t 
BinderNode::NotifyListeners(uint32 event, const char *name)
{
//	printf("NotifyListeners(%08x,'%s')\n",event,name);

	binder_cmd cmd;
	cmd.command = BINDER_NOTIFY_CALLBACK;
	
	DoNotification(event,name);
	if(m_hostingNodeHandle != 0) {
		cmd.data.notify.event = event;
		if (name) copystring(cmd.data.notify.name, name, sizeof(cmd.data.notify.name) - 1);
		else cmd.data.notify.name[0] = 0;
		cmd.node_handle = m_hostingNodeHandle;
		return do_binder_command(&cmd);
	}

	return B_OK;
}

void 
BinderNode::DoNotification(uint32 event, const char *name)
{
//	printf("DoNotification(%08x,'%s')\n",event,name);
	if (!m_observerData) return;
	m_observerData->Notify(this,event,name);
}

BinderNode::iterator::iterator(BinderNode *node)
{
	if (!node || (node->OpenProperties(&cookie,NULL) < 0)) cookie = NULL;
	else parent = node;
}

BinderNode::iterator::~iterator()
{
	if (cookie) parent->CloseProperties(cookie);
}

const BinderNode::iterator &
BinderNode::iterator::operator=(const iterator &copy)
{
	if (cookie) parent->CloseProperties(cookie);

	BinderNode *node = copy.parent;
	if (!node || (node->OpenProperties(&cookie,copy.cookie) < 0)) cookie = NULL;
	else parent = node;
	
	return *this;
}

BString 
BinderNode::iterator::Next()
{
	if (cookie) {
		char buf[1024];
		int32 len=128;
		if (parent->NextProperty(cookie,buf,&len) == B_OK)
			return BString(buf);
	}

	return BString();
}

BinderNode::iterator 
BinderNode::Properties()
{
	return iterator(this);
}

put_status_t 
BinderNode::PutProperty(const char *name, const BinderNode::property &property)
{
	if (m_privateFlags & pfRedirect) return kPutProperty(m_hostingNodeHandle,name,property);
	return WriteProperty(name,property);
}

get_status_t 
BinderNode::GetProperty(const char *name, BinderNode::property &property, const BinderNode::property_list &args)
{
	if (m_privateFlags & pfRedirect) return kGetProperty(m_hostingNodeHandle,name,property,args);
	return ReadProperty(name,property,args);
}

get_status_t 
BinderNode::GetProperty(const char *name, BinderNode::property &property, const BinderNode::property *argList, ...)
{
	va_list vl;
	BinderNode::property *arg;
	BinderNode::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BinderNode::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BinderNode::property *))) { args.AddItem(arg); i++; };
	va_end(vl);
	
	return GetProperty(name,property,args);
}

uint32 
BinderNode::Flags() const
{
	return 0;
}

status_t 
BinderNode::OpenProperties(void **, void *)
{
	return B_OK;
}

status_t 
BinderNode::NextProperty(void *, char *, int32 *)
{
	return ENOENT;
}

status_t 
BinderNode::CloseProperties(void *)
{
	return B_OK;
}

put_status_t 
BinderNode::WriteProperty(const char *, const BinderNode::property &)
{
	return put_status_t(B_ERROR,true);
}

get_status_t 
BinderNode::ReadProperty(const char *, BinderNode::property &, const BinderNode::property_list &)
{
	return B_ERROR;
}

int kSeverLinks(int32 desc, uint32 linkFlags)
{
	binder_cmd cmd;
	cmd.command = BINDER_BREAK_LINKS;
	cmd.data.break_links.flags = linkFlags;
	cmd.node_handle = desc;
	return BinderNode::do_binder_command(&cmd);
}

status_t 
BinderNode::Unstack()
{
	return kSeverLinks(m_hostingNodeHandle,linkAugment|linkFilter|linkTo);
}

status_t 
BinderNode::Topple()
{
	return kSeverLinks(m_hostingNodeHandle,linkAugment|linkFilter|linkFrom);
}

status_t 
BinderNode::RenounceAncestry()
{
	return kSeverLinks(m_hostingNodeHandle,linkAugment|linkInherit|linkFrom);
}

status_t 
BinderNode::DisownChildren()
{
	return kSeverLinks(m_hostingNodeHandle,linkAugment|linkInherit|linkTo);
}

status_t 
BinderNode::StackOnto(binder_node object)
{
	StartHosting();
	
	binder_cmd cmd;
	uint32 desc;
	BinderNode::property prop(object);

	cmd.command = BINDER_STACK;
	cmd.data.node_handle = m_hostingNodeHandle;
	//cmd.data.nodeid.port = Port();
	//cmd.data.nodeid.token = Token();
	prop.Remotize();
	desc = prop.m_value.descriptor;
	if(prop.m_format == BinderNode::property::f_object)
		desc = ((BinderProxy*)prop.m_value.object)->m_descriptor;
	cmd.node_handle = desc;
	return do_binder_command(&cmd);
}

status_t 
BinderNode::InheritFrom(binder_node object)
{
	StartHosting();

	binder_cmd cmd;
	uint32 desc;

	BinderNode::property prop(object);

	prop.Remotize();
	desc = prop.m_value.descriptor;
	if(prop.m_format == BinderNode::property::f_object)
		desc = ((BinderProxy*)prop.m_value.object)->m_descriptor;
	cmd.command = BINDER_INHERIT;
	cmd.data.node_handle = desc;
	cmd.node_handle = m_hostingNodeHandle;
	return do_binder_command(&cmd);
}

binder_node BinderNode::Copy(bool)
{
	return new BinderNode();
}

/************************************************************************************/

BinderObserver::BinderObserver(const binder_node &node, uint32 eventMask, const char *name)
{
	m_object = node;
	m_token = node->AddObserverCallback(this,(BinderNode::observer_callback)Callback,eventMask,name);
}

BinderObserver::~BinderObserver()
{
	m_object->RemoveObserverCallback(m_token);
	
}

const binder_node &
BinderObserver::Object()
{
	return m_object;
}

status_t 
BinderObserver::ObservedChange(uint32 , const char *)
{
	return B_OK;
}

status_t 
BinderObserver::Callback(BinderObserver *This, uint32 eventMask, void *name)
{
	return This->ObservedChange(eventMask,(char *)name);
}

/************************************************************************************/

BinderListener::BinderListener()
{
}

status_t 
BinderListener::StartListening(binder_node node, uint32 eventMask, const char *propertyName)
{
	node->AddObserver(this,eventMask,propertyName);
	
	return B_OK;
}

status_t 
BinderListener::StopListening(binder_node node)
{
	return node->RemoveObserver(this);
}

BinderListener::~BinderListener()
{
}

status_t 
BinderListener::Overheard(binder_node , uint32, BString)
{
	return B_OK;
}

/************************************************************************************/

struct prop_cookie {
	int32 stage;
	uint32 cookie;
};

extern int _kstart_watching_vnode_(dev_t dev, ino_t ino, ulong flags, port_id port, long token);
extern int _kstop_watching_vnode_(dev_t dev, ino_t ino, port_id port, long token);
extern int _kstop_notifying_(port_id port, long token);

BinderProxy::BinderProxy(uint32 descriptor)
{
	m_flags = 0;
	m_descriptor = descriptor;
}

void BinderProxy::StartWatching()
{
	if (!(m_flags & 1) && !(atomic_or(&m_flags,1) & 1)) {
		status_t err;
		binder_cmd cmd;
		cmd.command = BINDER_START_WATCHING;
		cmd.node_handle = m_descriptor;
		cmd.data.nodeid.port = Port();
		cmd.data.nodeid.token = Token();
		err = do_binder_command(&cmd);
		if(err < B_OK) {
			printf("BinderProxy::StartWatching failed, %s\n", strerror(err));
		}
#if 0
		struct stat st;
		if (!_krstat_(m_descriptor, NULL, &st, FALSE)) {
			_kstart_watching_vnode_(st.st_dev,st.st_ino,B_WATCH_ATTR,Port(),Token());
		} else {
			close(m_descriptor);
			m_descriptor = -1;
		}
#endif
	}
}

void 
BinderProxy::Cleanup()
{
	if((m_descriptor != 0) && (m_flags & 1)) {
		status_t err;
		binder_cmd cmd;
		cmd.command = BINDER_STOP_WATCHING;
		cmd.node_handle = m_descriptor;
		cmd.data.nodeid.port = Port();
		cmd.data.nodeid.token = Token();
		err = do_binder_command(&cmd);
		if(err < B_OK) {
			printf("BinderProxy::StopWatching failed, %s\n", strerror(err));
		}
		//_kstop_notifying_(Port(),Token());
	}
	BinderNode::Cleanup();
}

BinderProxy::~BinderProxy()
{
	if(m_descriptor != 0) {
		binder_cmd cmd;
		cmd.command = BINDER_DISCONNECT;
		cmd.node_handle = m_descriptor;
		do_binder_command(&cmd);
	}
}

binder_node BinderProxy::Copy(bool)
{
	printf("Cannot copy a proxy node!\n");
	binder_cmd cmd;
	status_t err;

	cmd.command = BINDER_CONNECT;
	cmd.data.connect.node_id.port = -1;
	cmd.data.connect.source_node_handle = m_descriptor;
	err = do_binder_command(&cmd);
	if(err < B_OK)
		return NULL;

	return new BinderProxy(cmd.data.connect.return_node_handle);
}

uint32 
BinderProxy::Flags() const
{
	uint32 flags = 0;
	if (m_descriptor != 0) flags |= isValid;
	return flags;
}

status_t 
BinderProxy::OpenProperties(void **cookie, void *copyCookie)
{
	*cookie = NULL;
	prop_cookie *c = new prop_cookie;
	if (copyCookie) *c = *((prop_cookie*)copyCookie);
	else {
		c->stage = 0;
		c->cookie = 0;
	}
	*cookie = c;
	return B_OK;
}

status_t 
BinderProxy::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	prop_cookie *c = (prop_cookie*)cookie;
	switch (c->stage) {
		case 0: {
			status_t err;
			binder_cmd cmd;
			cmd.command = BINDER_OPEN_PROPERTIES;
			cmd.node_handle = m_descriptor;
			err = do_binder_command(&cmd);
			if(err != B_OK)
				return err;
			c->cookie = cmd.data.property_iterator.cookie;
			c->stage = 1;
		}
		case 1: {
			status_t err;
			binder_cmd cmd;
			cmd.command = BINDER_NEXT_PROPERTY;
			cmd.data.property_iterator.cookie = c->cookie;

			err = do_binder_command(&cmd);
			if(err != B_OK)
				return err;
			*len = copystring(nameBuf, cmd.data.property_iterator.name, *len);
			return B_OK;
		}
		case 2: {
			return ENOENT;
		}
	}
	return B_ERROR;
}

status_t 
BinderProxy::CloseProperties(void *cookie)
{
	status_t err = B_OK;
	prop_cookie *c = (prop_cookie*)cookie;

	if(c->cookie != 0) {
		binder_cmd cmd;
		cmd.command = BINDER_CLOSE_PROPERTIES;
		cmd.data.property_iterator.cookie = c->cookie;
		err = do_binder_command(&cmd);
	}

	delete c;
	return err;
}

get_status_t 
BinderProxy::ReadProperty(const char *name, BinderNode::property &property, const BinderNode::property_list &args)
{
	if ((args.Count() == 0) && m_cache.Lookup(name,property)) return get_status_t(B_OK,true);

	//if(m_descriptor == 0)
	//	printf("Binder: ReadProperty called on Dummy Proxy\n");

	get_status_t r = kGetProperty(m_descriptor,name,property,args);
	if ((args.Count() == 0) && !r.error && r.resultCacheable && !property.IsRemoteObject() && (!property.IsString() || (property.String().Length() < 256))) {
		StartWatching();
		m_cache.Insert(name,property);
	}
	return r;
}

put_status_t 
BinderProxy::WriteProperty(const char *name, const BinderNode::property &property)
{
	m_cache.Remove(name);
	return kPutProperty(m_descriptor,name,property);
}

void 
BinderProxy::DoNotification(uint32 event, const char *name)
{
	/* flush some cache, in neccessary */
	if (event & B_SOMETHING_CHANGED) {
		if (event & B_NAME_KNOWN)
			m_cache.Remove(name);
		else
			m_cache.MakeEmpty();
	}
	BinderNode::DoNotification(event,name);
}

/************************************************************************************/

BinderNode::property BinderNode::property::undefined;

status_t 
BinderNode::property::InstantiateRemote()
{
	if (m_format == f_descriptor) {
		BinderNode *node = new BinderProxy(m_value.descriptor);
		node->Acquire(this);
		m_format = f_object;
		m_value.object = node;
	}
	return B_OK;
}

BinderNode::property::type BinderNode::property::Type() const
{
	return m_type;
}

int32 
BinderNode::property::FlattenedSize() const
{
	switch (m_type) {
		case string:				return 8 + strlen(m_value.string);
		case number:				return 12;
		case object:				return 4 + sizeof(dev_t) + sizeof(ino_t);
		case typed_raw:				
			return (sizeof(binder_typed_raw) + ((binder_typed_raw *)(m_value.typed_raw))->len);
		case null:
		default:					return 4;
	}
}

int32 
BinderNode::property::FlattenTo(void *buffer) const
{
	uint8 *buf = (uint8*)buffer;
	*((int32*)buf) = m_type;  buf += 4;
	if (m_type == string) {
		int32 len = strlen(m_value.string);
		*((int32*)buf) = len;
		buf += sizeof(int32);
		char *p = m_value.string;
		while (len--) *buf++ = *p++;
	} else if (m_type == number) {
		*((double*)buf) = m_value.number;  buf += sizeof(double);
	} else if (m_type == typed_raw) {
		binder_typed_raw *sourcedata = (binder_typed_raw *)m_value.typed_raw;
		binder_typed_raw *targetdata = (binder_typed_raw *)buf;
		targetdata->len = sourcedata->len;
		targetdata->type = sourcedata->type;
		memcpy((void *)targetdata->buffer, (void *)sourcedata->buffer, sourcedata->len);
		buf += (sizeof(binder_typed_raw) + sourcedata->len);	
	} else if (m_type == object) {
		uint32 desc = m_value.descriptor;
		dev_t devid = 0;
		binder_node_id node_id; node_id.port = -1;
		if (m_format == f_object) {
			BinderProxy *proxy = dynamic_cast<BinderProxy*>(m_value.object);
			if (proxy) desc = proxy->m_descriptor;
			else {
				m_value.object->StartHosting();
				node_id.port = m_value.object->Port();
				node_id.token = m_value.object->Token();
				goto writeIt;
			}
		}
		binder_cmd cmd;
		cmd.command = BINDER_GET_NODE_ID;
		cmd.node_handle = desc;
		if(do_binder_command(&cmd) >= B_OK)
			node_id = cmd.data.nodeid;

		writeIt:
		*((dev_t*)buf) = devid;  buf += sizeof(dev_t);
		*((binder_node_id*)buf) = node_id;  buf += sizeof(binder_node_id);
	}
	
	return buf - ((uint8*)buffer);
}

int32 
BinderNode::property::UnflattenFrom(void *buffer)
{
	Undefine();

	uint8 *buf = (uint8*)buffer;
	int32 t = *((int32*)buf); buf += 4;
//	printf("Unflattening %ld\n",t);
	m_type = (BinderNode::property::type)t;
	if (m_type == string) {
		int32 len = *((int32*)buf);
		buf += sizeof(int32);
		m_value.string = (char*)malloc(len+1);
		if(m_value.string == NULL) {
			buf += len;
			Undefine();
		}
		else {
			char *p = m_value.string;
			while (len--) *p++ = *buf++;
			*p++ = 0;
			m_format = f_string;
	//		printf("string = '%s'\n",value.string);
		}
	} else if (m_type == number) {
		m_value.number = *((double*)buf);  buf += sizeof(double);
		m_format = f_number;
	} else if (m_type == typed_raw) {
		binder_typed_raw *sourcedata = (binder_typed_raw *)buf;
		
		m_value.typed_raw = malloc(sizeof(binder_typed_raw) + sourcedata->len);
		binder_typed_raw *targetdata = (binder_typed_raw *)m_value.typed_raw;
		
		targetdata->len = sourcedata->len;
		targetdata->type = sourcedata->type;
		m_format = f_typed_raw;
		memcpy((void *)targetdata->buffer, (void *)sourcedata->buffer, sourcedata->len);	
	} else if (m_type == object) {
		binder_node_id node_id;
		buf += sizeof(dev_t);
		node_id = *((binder_node_id*)buf);  buf += sizeof(binder_node_id);
		
		port_info pi;
		thread_info ti;
		GHandler* h = NULL;
		get_port_info(node_id.port,&pi);
		get_thread_info(find_thread(NULL),&ti);
		if ((pi.team == ti.team) && (node_id.token != (uint32)NO_TOKEN) &&
			(gDefaultTokens->GetToken(node_id.token, 100, (void**)&h, get_handler_token) >= B_OK) && h) {
			m_type = BinderNode::property::object;
			m_value.object = (BinderNode*)h;
			m_format = f_object;
			m_value.object->Acquire(this);
			m_value.object->DecRefs(gDefaultTokens);
		} else {
			if(open_node(node_id, &m_value.descriptor) < B_OK)
				Undefine();
			else
				m_format = f_descriptor;
		}
	}
	
	return buf - ((uint8*)buffer);
}

status_t 
BinderNode::property::Remotize()
{
	if (m_format == f_object) {
		status_t err;
		uint32 desc = 0;
		BinderProxy *proxy = dynamic_cast<BinderProxy*>(m_value.object);
		if (proxy) {
			binder_cmd cmd;
		
			cmd.command = BINDER_CONNECT;
			cmd.data.connect.node_id.port = -1;
			cmd.data.connect.source_node_handle = proxy->m_descriptor;
			err = do_binder_command(&cmd);
			if(err < B_OK)
				return err;
			desc = cmd.data.connect.return_node_handle;
		} else {
			m_value.object->StartHosting();
			binder_node_id node_id;
			node_id.port = m_value.object->Port();
			node_id.token = m_value.object->Token();
			err = open_node(node_id, &desc);
			if(err < B_OK)
				return err;
		}

		m_value.object->Release(this);
		m_value.descriptor = desc;
		m_type = object;
		m_format = f_descriptor;
	}
	
	return B_OK;
}

double 
BinderNode::property::Number() const
{
	if (m_format == f_number) return m_value.number;
	double d = 0;
	if (m_format == f_string) sscanf(m_value.string,"%lf",&d);
	return d;
}

BString 
BinderNode::property::String() const
{
	if (m_type == object) return BString("<object>");
	if (m_format == f_string) return BString(m_value.string);
	if (m_format == f_number) {
		char str[512];
		sprintf(str,"%f",m_value.number);
		return BString(str);
	}
	if (m_format == f_typed_raw) return BString("<typed_raw>");
	
	return BString("<undefined>");
}

binder_typed_raw * 
BinderNode::property::TypedRaw() const
{
	if (m_type != typed_raw) return NULL;
	
	return (binder_typed_raw *)m_value.typed_raw;
}

atom<BinderNode> 
BinderNode::property::Object() const
{
	BinderNode::property &t = const_cast<BinderNode::property&>(*this);
	t.InstantiateRemote();
	if (m_format == f_object) return m_value.object;
	return dummyNode();
}

void 
BinderNode::property::Undefine()
{
	if (m_format == f_string)
		free(m_value.string);
	else if (m_format == f_object)
		m_value.object->Release(this);
	else if (m_format == f_descriptor) {
		binder_cmd cmd;
		cmd.command = BINDER_DISCONNECT;
		cmd.node_handle = m_value.descriptor;
		do_binder_command(&cmd);
	}
	else if (m_format == f_typed_raw)
		free(m_value.typed_raw);

	m_value.number = 0;
	m_type = null;
	m_format = f_null;
}

bool 
BinderNode::property::operator>=(const BinderNode::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) >= ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) >= ((BString)other);
	return false;
}

bool 
BinderNode::property::operator<=(const BinderNode::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) <= ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) <= ((BString)other);
	return false;
}

bool 
BinderNode::property::operator>(const BinderNode::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) > ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) > ((BString)other);
	return false;
}

bool 
BinderNode::property::operator<(const BinderNode::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) < ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) < ((BString)other);
	return false;
}

bool 
BinderNode::property::operator==(const BinderNode::property &other) const
{
	if ((m_type == null) && (other.m_type == null)) return true;
	if (IsNumber() && other.IsNumber())
		return ((double)*this) == ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) == ((BString)other);
	return false;
}

bool 
BinderNode::property::operator!=(const BinderNode::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) != ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) != ((BString)other);
	return false;
}

const BinderNode::property &
BinderNode::property::operator=(const BinderNode::property &other)
{
	Undefine();
	m_type = other.m_type;
	m_format = other.m_format;
	if (m_format == f_string)
		m_value.string = strdup(other.m_value.string);
	else if (m_format == f_descriptor) {
		binder_cmd cmd;
		status_t err;
		
		cmd.command = BINDER_CONNECT;
		cmd.data.connect.node_id.port = -1;
		cmd.data.connect.source_node_handle = other.m_value.descriptor;
		err = do_binder_command(&cmd);
		if(err < B_OK)
			Undefine();
		else
			m_value.descriptor = cmd.data.connect.return_node_handle;
	}
	else if (m_format == f_object) {
		m_value.object = other.m_value.object;
		m_value.object->Acquire(this);
	} else if (m_format == f_typed_raw) {
		binder_typed_raw *source = (binder_typed_raw *)other.m_value.typed_raw;
		
		m_value.typed_raw = malloc(sizeof(binder_typed_raw) + source->len);
		binder_typed_raw *target = (binder_typed_raw *)m_value.typed_raw;
		
		target->len = source->len;
		target->type = source->type;
		
		memcpy((void *)(target->buffer), (void *)(source->buffer), source->len);
	} else
		m_value = other.m_value;
	return *this;
}

/*
const BinderNode::property &
BinderNode::property::operator=(const property_ref &ref)
{
	*this = ref();
	return *this;
}

const BinderNode::property &
BinderNode::property::operator=(class BinderNode *reg)
{
	Undefine();
	
	BinderProxy *proxy = dynamic_cast<BinderProxy*>(reg);
	if (proxy)	m_type = remote_object;
	else		m_type = object;
	m_format = f_object;
	m_value.object = reg;
	m_value.object->Acquire();
	return *this;
}

const BinderNode::property &
BinderNode::property::operator=(const char *str)
{
	Undefine();
	m_type = string;
	m_format = f_string;
	m_value.string = strdup(str);
	return *this;
}

const BinderNode::property &
BinderNode::property::operator=(double num)
{
	Undefine();
	m_type = number;
	m_format = f_number;
	m_value.number = num;
	return *this;
}
*/
bool 
BinderNode::property::IsUndefined() const
{
	return (m_type == null);
}

bool 
BinderNode::property::IsNumber() const
{
	return (m_type == number);
}

bool 
BinderNode::property::IsString() const
{
	return (m_type == string);
}

bool 
BinderNode::property::IsObject() const
{
	return (m_type == object);
}

bool 
BinderNode::property::IsTypedRaw() const
{
	return (m_type == typed_raw);
}

bool 
BinderNode::property::IsRemoteObject() const
{
	return	(m_type == object) &&
			(
				(m_format == f_descriptor) ||
				(
					(m_format == f_object) &&
					dynamic_cast<BinderProxy*>(m_value.object)
				)
			);
}

BinderNode::property::property()
{
	m_type = null;
	m_format = f_null;
}

BinderNode::property::property(const BinderNode::property &other)
{
	m_type = null;
	m_format = f_null;
	*this = other;
}

BinderNode::property::property(const BinderNode::property_ref &ref)
{
	m_type = null;
	m_format = f_null;
	if (ref.m_base) {
		if (ref.m_name) ref.m_base->GetProperty(ref.m_name,*this);
		else *this = ref.m_base;
	}
}

void BinderNode::property::Init(BinderNode *value)
{
	if(value == NULL) {
		m_type = null;
		m_format = f_null;
		return;
	}
	
	m_type = object;
	m_format = f_object;
	m_value.object = value;
	m_value.object->Acquire(this);
}

void BinderNode::property::Init(double value)
{
	m_type = number;
	m_format = f_number;
	m_value.number = value;
}

void BinderNode::property::Init(const char *value)
{
	m_type = string;
	m_format = f_string;
	m_value.string = strdup(value);
}

void BinderNode::property::Init(type_code type, void *value, size_t len)
{
	m_type = typed_raw;
	m_format = f_typed_raw;
	
	size_t buflen = (len + sizeof(binder_typed_raw));	
	
	m_value.typed_raw = malloc(buflen);
	
	binder_typed_raw *data_struct = (binder_typed_raw *)m_value.typed_raw;
	data_struct->len = len;
	data_struct->type = type;
	
	memcpy((void *)(data_struct->buffer), value, len);	
}

BinderNode::property::~property()
{
	Undefine();
}

BinderNode *
BinderNode::property::operator->()
{
	InstantiateRemote();
	return (m_format == f_object)?m_value.object:dummyNode();
}

BinderNode::property_ref
BinderNode::property::operator/(const char *name)
{
	InstantiateRemote();
	return property_ref((m_format == f_object)?m_value.object:NULL,name);
}

BinderNode::property_ref
BinderNode::property::operator[](const char *name)
{
	InstantiateRemote();
	return property_ref((m_format == f_object)?m_value.object:NULL,name);
}

/************************************************************************************/

put_status_t 
BinderNode::property_ref::Put(const property &prop) const
{
	if (m_base) return m_base->PutProperty(m_name,prop);
	return B_BAD_VALUE;
}

get_status_t 
BinderNode::property_ref::Get(property &returnVal) const
{
	returnVal.Undefine();
	if (m_base) return m_base->GetProperty(m_name,returnVal);
	return B_BAD_VALUE;
}

get_status_t 
BinderNode::property_ref::Get(property &returnVal, const property_list &args) const
{
	returnVal.Undefine();
	if (m_base) return m_base->GetProperty(m_name,returnVal,args);
	return B_BAD_VALUE;
}

get_status_t 
BinderNode::property_ref::Get(property &returnVal, const property *argList, ...) const
{
	va_list vl;
	BinderNode::property *arg;
	BinderNode::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BinderNode::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BinderNode::property *))) { args.AddItem(arg); i++; };
	va_end(vl);

	returnVal.Undefine();
	if (m_base) return m_base->GetProperty(m_name,returnVal,args);
	return B_BAD_VALUE;
}

BinderNode::property 
BinderNode::property_ref::operator ()() const
{
	BinderNode::property prop;
	if (m_base) m_base->GetProperty(m_name,prop);
	return prop;
}

BinderNode::property 
BinderNode::property_ref::operator ()(const property_list &args) const
{
	BinderNode::property prop;
	if (m_base) m_base->GetProperty(m_name,prop,args);
	return prop;
}

BinderNode::property
BinderNode::property_ref::operator ()(const property *argList, ...) const
{
	va_list vl;
	BinderNode::property *arg;
	BinderNode::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BinderNode::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BinderNode::property *))) { args.AddItem(arg); i++; };
	va_end(vl);
	
	BinderNode::property prop;
	if (m_base) m_base->GetProperty(m_name,prop,args);
	return prop;
}

/*
BinderNode::property_ref::operator BinderNode::property() const
{
	BinderNode::property prop;
	if (m_base) m_base->GetProperty(m_name,prop);
	return prop;
}
*/

const BinderNode::property_ref &
BinderNode::property_ref::operator=(const BinderNode::property &prop)
{
	if (m_base) m_base->PutProperty(m_name,prop);
	return *this;
}

BinderNode::property_ref::property_ref(BinderNode *base, const char *name)
{
	m_base = base;
	m_name = strdup(name);
	if (!m_name) m_name = strdup("");
}

BinderNode::property_ref::~property_ref()
{
	free(m_name);
}

/************************************************************************************/

enum {
	bcfOrdered = 0x00000001
};

BinderContainer::BinderContainer()
{
	m_flags = 0;
	m_perms = permsRead;
}

BinderContainer::BinderContainer(BinderContainer *other, bool deep) : BinderNode(other,deep)
{
	m_flags = other->m_flags;
	m_perms = other->m_perms;
	
	other->m_listLock.Lock();
	m_propertyList.AssertSize(other->m_propertyList.Count());
	for (int32 i=0;i<other->m_propertyList.Count();i++)
		m_propertyList[i] = other->m_propertyList[i];
	other->m_listLock.Unlock();

	if (deep) {
		for (int32 i=0;i<m_propertyList.Count();i++)
			if (m_propertyList[i].value.IsObject())
				m_propertyList[i].value = m_propertyList[i].value.Object()->Copy(true);
	}
}

binder_node 
BinderContainer::Copy(bool deep)
{
	return new BinderContainer(this,deep);
}

BinderContainer::~BinderContainer()
{
}

bool 
BinderContainer::Ordered()
{
	return m_flags & bcfOrdered;
}

status_t 
BinderContainer::SetOrdered(bool isOrdered)
{
	if (isOrdered)
		m_flags |= bcfOrdered;
	else
		m_flags &= ~bcfOrdered;
	return B_OK;
}

uint32
BinderContainer::Flags() const
{
	return isValid;
}

struct store_cookie {
	int32 index;
};

status_t 
BinderContainer::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) *c = *((store_cookie*)copyCookie);
	else c->index = 0;
	*cookie = c;
	return B_OK;
}

status_t 
BinderContainer::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	status_t err = ENOENT;
	store_cookie *c = (store_cookie*)cookie;
	m_listLock.Lock();
	if (c->index < m_propertyList.CountItems()) {
		char *str = m_propertyList[c->index++].name;
		*len = copystring(nameBuf, str, *len);
		err = B_OK;
	}
	m_listLock.Unlock();
	return err;
}

status_t 
BinderContainer::RewindProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	c->index = 0;
	return B_OK;
}

status_t 
BinderContainer::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

bool 
BinderContainer::FindName(const char *name, int32 &index)
{
	int32 cmp, mid, low = 0, high = m_propertyList.CountItems()-1;

	if (m_flags & bcfOrdered) {
		for (int32 i=0;i<=high;i++) {
			if (!strncmp(name,m_propertyList[i].name,PROPERTY_NAME_LEN-1)) {
				index = i;
				return true;
			}
		}
		index = high+1;
		return false;
	}

	while (low <= high) {
		mid = (low + high)/2;
		cmp = strncmp(name,m_propertyList[mid].name,PROPERTY_NAME_LEN-1);
		if (cmp == 0) {
			index = mid;
			return true;
		} else if (cmp < 0) {
			high = mid-1;
		} else
			low = mid+1;
	}
	
	index = low;
	return false;
}

status_t 
BinderContainer::AddProperty(const char *name, const BinderNode::property &prop, uint16 perms)
{
	int32 i;
	m_listLock.Lock();
	if (FindName(name,i)) {
		m_propertyList[i].value = prop;
		m_propertyList[i].perms = perms;
		m_listLock.Unlock();
		NotifyListeners(B_PROPERTY_CHANGED,name);
		return B_OK;
	}
	
	property_record &prop2 = m_propertyList.InsertItem(i);
	copystring(prop2.name, name, PROPERTY_NAME_LEN-1);
	prop2.value = prop;
	prop2.perms = perms;
	m_listLock.Unlock();
	NotifyListeners(B_PROPERTY_ADDED,name);
	return B_OK;
}

status_t 
BinderContainer::RemoveProperty(const char *name)
{
	int32 i;
	m_listLock.Lock();
	if (FindName(name,i)) {
		m_propertyList.RemoveItem(i);
		m_listLock.Unlock();
		NotifyListeners(B_PROPERTY_REMOVED,name);
		return B_OK;
	}
	m_listLock.Unlock();
	return ENOENT;
}

bool 
BinderContainer::HasProperty(const char *name)
{
	int32 i;
	GehnaphoreAutoLock _auto(m_listLock);
	return FindName(name,i);
}

uint16 
BinderContainer::Permissions()
{
	return m_perms;
}

status_t 
BinderContainer::SetPermissions(uint16 perms)
{
	m_perms = perms;
	return B_OK;
}

put_status_t 
BinderContainer::WriteProperty(const char *name, const BinderNode::property &prop)
{	
	int32 i;
	uint16 perms = m_perms;
	status_t err = EPERM;

	m_listLock.Lock();
	if (FindName(name,i)) {
		if (!(m_propertyList[i].perms & permsInherit)) perms = m_propertyList[i].perms;

		if (prop.IsUndefined()) {
			if (perms & permsDelete) {
				m_propertyList.RemoveItem(i);
				m_listLock.Unlock();
				NotifyListeners(B_PROPERTY_REMOVED,name);
				return B_OK;
			}
		}

		if (prop.IsRemoteObject()) {
			if (!(perms & permsMount)) {
				printf("BinderContainer::WriteProperty '%s' --> not a mount point!\n", name);
				err = EPERM;
				goto err;
			}
		} else if (prop.IsObject()) {
			if (!(perms & permsMount) && !(perms & permsWrite)) {
				printf("BinderContainer::WriteProperty '%s' --> cannot write local object to property!\n", name);
				err = EPERM;
				goto err;
			}
		} else {
			if (!(perms & permsWrite)) {
				printf("BinderContainer::WriteProperty '%s' --> property not writable!\n", name);
				err = EPERM;
				goto err;
			}
		}
			
		if (m_propertyList[i].value.IsRemoteObject()) {
			if (!(perms & permsWrite) && !((perms & permsMount) && (prop.IsUndefined() || prop.IsRemoteObject()))) {
				printf("BinderContainer::WriteProperty --> '%s' cannot unmount! (0x%08x)\n",name, perms);
				err = EPERM;
				goto err;
			}
		}

		m_propertyList[i].value = prop;
		m_listLock.Unlock();
		NotifyListeners(B_PROPERTY_CHANGED,name);
		return B_OK;
	}
	
	if (!prop.IsUndefined()) {
		if (prop.IsRemoteObject()) {
			if (!(perms & permsMount)) {
				err = EPERM;
				goto err;
			}
		} else if (prop.IsObject()) {
			if (!(perms & permsCreate) && !(perms & permsMount)) {
				err = EPERM;
				goto err;
			}
		} else {
			if (!(perms & permsCreate)) {
				err = EPERM;
				goto err;
			}
		}
		property_record &prop2 = m_propertyList.InsertItem(i);
		copystring(prop2.name, name, PROPERTY_NAME_LEN-1);
		prop2.value = prop;
		prop2.perms = perms | permsDelete;
		m_listLock.Unlock();
		NotifyListeners(B_PROPERTY_ADDED,name);
		return B_OK;
	}
	err = B_OK;
err:
	m_listLock.Unlock();
	return err;
}

get_status_t 
BinderContainer::ReadProperty(const char *name, BinderNode::property &property, const BinderNode::property_list &)
{
	int32 i;
	property.Undefine();

	GehnaphoreAutoLock _auto(m_listLock);

	uint16 perms = m_perms;

	if (FindName(name,i)) {
		if (!(m_propertyList[i].perms & permsInherit)) perms = m_propertyList[i].perms;
		if (!(perms & permsRead)) return EPERM;
		if (!m_propertyList[i].value.IsUndefined()) {
			property = m_propertyList[i].value;
			return get_status_t(B_OK,true);
		}
	}

	return ENOENT;
}
