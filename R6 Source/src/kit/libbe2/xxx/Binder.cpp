
#include <String.h>
#include <Binder.h>
#include <Dispatcher.h>
#include <Autolock.h>
#include <priv_syscalls.h>
#include <fs_attr.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <Debug.h>
#include <AssociativeArray.h>
#include <TLS.h>
#include <TokenSpace.h>

#include <binder_driver.h>

enum {
	pfRedirect = 0x00000001
};

#define checkpoint_static
// printf("thid=%ld -- %s:%d -- %s\n",(int32)find_thread(NULL),__FILE__,__LINE__,__PRETTY_FUNCTION__);
#define checkpoint printf("thid=%ld (%08x) -- %s:%d -- %s\n",(int32)find_thread(NULL),this,__FILE__,__LINE__,__PRETTY_FUNCTION__);

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


BBinder::property_list BBinder::empty_arg_list;

enum {
	B_LINK_FROM				= 0x00000100,
	B_INHERIT				= 0x00000200,
	B_STACK					= 0x00000400,
	B_ALL_USER				= B_PROPERTY_ADDED|B_PROPERTY_REMOVED|B_PROPERTY_CHANGED
};

namespace B {
namespace Support2 {

struct _link_data
{
	struct link {
		atom_ref<BBinder>				node;
		atom_ref<BBinderListener>		listener;
		uint32							mask;
		BString							name;
		
		link() : mask(0) {};
	};

	int32					flags;
	SmartArray<link>		list;
	
	_link_data() : flags(0) {};
};

class BBinderProxy : public BBinder
{
	public:

												BBinderProxy(uint32 descriptor);
		virtual									~BBinderProxy();

		virtual	uint32							Flags() const;

		virtual	status_t						OpenProperties(void **cookie, void *copyFrom);
		virtual	status_t						NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t						CloseProperties(void *cookie);

		virtual	get_status_t					ReadProperty(const char *name, BBinder::property &property, const BBinder::property_list &args);
		virtual	put_status_t					WriteProperty(const char *name, const BBinder::property &property);

		virtual	status_t						StartHosting() { return B_OK; }
		virtual	void							DoNotification(uint32 event, const char *name);

		virtual	void							Released();
		virtual	binder						Copy(bool deep);
		
		void									StartWatching();

		uint32									m_descriptor;
		int32									m_flags;
		AssociativeArray<BString,property>		m_cache;
};

} } // namespace B::Support2

static BLocker dummyProxyRefLock;
static binder dummyProxyRef;
static BLocker rootProxyRefLock;
static binder rootProxyRef;
static int binderDesc = -1;
static uint32 rootNodeHandle = 0;

dispatcher	BBinder::g_defaultDispatcher;
int32		BBinder::g_defaultDispatcherCreated = 0;

void BBinder::Init(BDispatcher *dispatcher)
{
	if (!dispatcher) {
		if (!atomic_or(&g_defaultDispatcherCreated,1))
			g_defaultDispatcher = new BDispatcher();
		while (g_defaultDispatcher.ptr() == NULL) snooze(20000);
		dispatcher = g_defaultDispatcher.ptr();
	};

	m_dispatcher = dispatcher;
	m_token = NO_TOKEN;
}

BDispatcher *
BBinder::Dispatcher()
{
	return m_dispatcher.ptr();
}

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

BBinder *dummyNode()
{
	if (dummyProxyRef.ptr()) return dummyProxyRef.ptr();
	dummyProxyRefLock.Lock();
	if (!dummyProxyRef.ptr()) dummyProxyRef = new BBinderProxy(0);
	dummyProxyRefLock.Unlock();
	return dummyProxyRef.ptr();
}

BBinder *
BBinder::rootNode()
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
			err = BBinder::do_binder_command(&cmd);
			if(err >= B_OK) {
				err = open_node(cmd.data.nodeid, &rootNodeHandle);
				if(err >= B_OK)
					rootProxyRef = new BBinderProxy(rootNodeHandle);
				else
					rootNodeHandle = 0;
			}
		}
		rootProxyRefLock.Unlock();
	}
	return rootProxyRef.ptr();
}
/*
static BLocker gBinderDispatcherLock;
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

char displayable(uint8 c)
{
	if ((c < 32) || (c > 127)) return '.';
	return c;
}

#define IO_SIZE 1024

namespace BPrivate {

class uspace_io {
	public:
		
		uint8 *buffer,stackBuffer[IO_SIZE];
		int32 pos,len,error,token,bufSize;
		uint8 *outBuffer,outStackBuffer[IO_SIZE];
		int32 outPos;
		port_id replyPort;
		
		void assert_size(int32 size);
		
		void write(BBinder::property &prop) {
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
		int32 read(BBinder::property &prop) {
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

} // namespace BPrivate

using namespace BPrivate;

/* always release buffer_lock */
void 
BBinder::ProcessMessage(void *buf, size_t buf_size, BLocker *buffer_lock)
{
	port_id sync_port = -1;
	bool startHosting = false;
	bool reply_registered = false;
	int32 token,cmd,err=B_ERROR;
	BBinder::property prop;
	atom_ptr<BBinder> reg;
	uspace_io io(buf, buf_size);

	if(buffer_lock != NULL)
		buffer_lock->Unlock();
	
	io.read(&token);
	reg = LookupByToken(token).acquire();
	if (reg.ptr() == NULL) {
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

	//printf("Binder::ProcessMessage, Command is %ld\n",cmd);
	
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
			BBinder::property_list args;
			BBinder::property *theArgs = NULL;
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
					theArgs = new BBinder::property[argCount];
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
				if (prop.Type() == BBinder::property::object)
					prop.Object()->StartHosting();
				if (prop.IsObject()) {
					sync_port = ioctl(binderDesc, BINDER_UNREGISTER_REPLY_THREAD_AND_GET_SYNC_PORT, io.replyPort);
					if(sync_port >= 0)
						reply_registered = false;
					else
						printf("Binder: BINDER_GET_SYNC_REPLY_PORT failed for port %d, "
						       "%s\n", (int)io.replyPort, strerror(errno));
				}
				if (prop == BBinder::property::undefined) getErr.error = ENOENT;
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
			printf("Binder::ProcessMessage: got unknown request %d\n",
			       (int)cmd);
			err = B_ERROR;
			io.write((port_id)-1); // no sync
			io.write(err);
		}
	}
	
	//printf("Binder::ProcessMessage, Command %ld completed\n", cmd);

	if(reply != (io.replyPort >= 0))
		printf("Binder::ProcessMessage, error reply %d when replyPort %d\n",
		       reply, (int)io.replyPort);

	if(sync_port >= 0 && !reply)
		printf("Binder::ProcessMessage, error reply %d when sync_port %d\n",
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
			//printf("Binder::ProcessMessage, sync on port %d\n",
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
					printf("Binder::ProcessMessage, error waiting for sync,"
					       " %s\n", strerror(err));
					goto err;
				}
				if(buf_size != 0) {
					printf("Binder::ProcessMessage, got bad message while "
					       "waiting for sync, size %d\n", (int)buf_size);
					goto err;
				}
				do
					err = read_port(sync_port, &code, NULL, 0);
				while(err == B_INTERRUPTED);
				if(code == BINDER_SYNC_CODE) {
					//printf("Binder::ProcessMessage, got sync on port %d\n",
					//       (int)sync_port);
					break;
				}
				if(code != BINDER_PUNT_CODE) {
					printf("Binder::ProcessMessage, got bad message type "
					       "while waiting for sync, %d\n", (int)code);
					goto err;
				}
				do
					err = buf_size = port_buffer_size_etc(sync_port,
						B_RELATIVE_TIMEOUT, 10000000);
				while(err == B_INTERRUPTED);
				if(err < B_OK) {
					printf("Binder::ProcessMessage, error waiting for punt "
					       "command, %s\n", strerror(err));
					goto err;
				}
				void *buf = malloc(buf_size);
				if(buf == NULL) {
					printf("Binder::ProcessMessage, out of memory\n");
					goto err;
				}
				do
					err = read_port(sync_port, &code, buf, buf_size);
				while(err == B_INTERRUPTED);
				if(code != BINDER_REQUEST_CODE) {
					printf("Binder::ProcessMessage, got bad message type "
					       "while waiting for punt command, %d\n", (int)code);
					free(buf);
					goto err;
				}
				//printf("Binder::ProcessMessage, processing command while waiting for sync\n");
				ProcessMessage(buf, buf_size);
				//printf("Binder::ProcessMessage, processing command done\n");
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
	//printf("Binder::ProcessMessage, obj %p ref count = %d\n", reg, rc - 1);
//	m_lock.Lock();
}

status_t
BBinder::handle_reflect_punt(binder_cmd *cmd)
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
		printf("Binder::handle_reflect_punt, bad message type %d\n", (int)code);
	else
		ProcessMessage(buf, buf_size);
	free(buf);
	return B_OK;
}

int BBinder::do_binder_command(binder_cmd *cmd, property *prop, const property_list *args)
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
		atom_ptr<BBinder> n = LookupByToken(cmd->reflection.token).acquire();
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

void BBinder::Disconnect(void *tag)
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
			//printf("Binder(%d): node %d-%d stopped hosting\n",
			//       (int)find_thread(NULL), (int)Port(), (int)Token());
			m_hostingNodeHandle = 0;
			if (m_linkData) atomic_and(&m_linkData->flags,~pfRedirect);
		}
	}
	Release(tag);
}

status_t
BBinder::open_node(struct binder_node_id node_id, uint32 *node_handle, bool startHosting)
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

get_status_t BBinder::kGetProperty(uint32 desc, const char *name, BBinder::property &prop, const BBinder::property_list &args)
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
	
	*((int32*)cmd.data.get.returnBuf) = BBinder::property::null;
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
	int32 err = do_binder_command(&cmd,&prop,&args);

	if (err == 0 && !cmd.data.get.successful_reflect) {
		if (cmd.data.get.return_node_handle != 0) {
			prop.Undefine();
			
			binder_node_id node_id;
			node_id = *((binder_node_id*)(((uint8*)cmd.data.get.returnBuf)+4+sizeof(dev_t)));

			port_info pi;
			thread_info ti;
			atom_ptr<BBinder> h;
			get_port_info(node_id.port,&pi);
			get_thread_info(find_thread(NULL),&ti);
//				printf("************ %d,%d,%d\n",pi.team,ti.team,vnid.address.token);
			if ((pi.team == ti.team) && (node_id.token != (uint32)NO_TOKEN) &&
				(h = BBinder::LookupByToken(node_id.token).acquire()).ptr()) {
				prop.m_type = BBinder::property::object;
				prop.m_value.object = h.ptr();
				prop.m_format = BBinder::property::f_object;
				prop.m_value.object->Acquire(&prop);
				prop.m_value.object->DecRefs(gDefaultTokens);
//					printf("grabbed local object for '%s'\n",name);
				binder_cmd dcmd;
				dcmd.command = BINDER_DISCONNECT;
				dcmd.node_handle = cmd.data.get.return_node_handle;
				BBinder::do_binder_command(&dcmd);
			} else {
//					printf("grabbed remote object for '%s'\n",name);
				prop.m_type = BBinder::property::object;
				prop.m_format = BBinder::property::f_descriptor;
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

put_status_t BBinder::kPutProperty(uint32 desc, const char *name, const BBinder::property &property)
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

	int32 err = BBinder::do_binder_command(&cmd,const_cast<BBinder::property*>(&property));

	if (data) free(data);
	if (err) cmd.data.put.result.error = err;
	return cmd.data.put.result;
}

void BBinder::BindMeUp2() {};
void BBinder::BindMeUp3() {};
void BBinder::BindMeUp4() {};
void BBinder::BindMeUp5() {};
void BBinder::BindMeUp6() {};
void BBinder::BindMeUp7() {};
void BBinder::BindMeUp8() {};

BBinder::BBinder()
{
	Init(NULL);
	m_hostingNodeHandle = 0;
	m_linkData = NULL;
}

BBinder::BBinder(BDispatcher *dispatcher)
{
	Init(dispatcher);
	m_hostingNodeHandle = 0;
	m_linkData = NULL;
}

BBinder::BBinder(BBinder *, bool)
{
	m_hostingNodeHandle = 0;
	m_linkData = NULL;
}

BBinder::~BBinder()
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

void BBinder::Released()
{
	if(m_hostingNodeHandle != 0) {
		status_t err;
		binder_cmd cmd;
		printf("Binder: Cleanup called on hosted node\n");
		cmd.command = BINDER_STOP_HOSTING;
		cmd.node_handle = m_hostingNodeHandle;
		err = do_binder_command(&cmd);
		if(err >= B_OK) {
			int32 token;
			m_lock.Lock();

			token = m_token;
			m_token = NO_TOKEN - 1;
			m_hostingNodeHandle = 0;
			if (m_linkData) {
				delete m_linkData;
				m_linkData = NULL;
			}
			m_lock.Unlock();

			if ((token != NO_TOKEN - 1) && (token != NO_TOKEN))
				m_dispatcher->InvalidateToken(token);
		}
	}
}

struct mountparms {
	port_id				port;
	int32				token;
};

port_id BBinder::Port()
{
	return m_dispatcher->Port();
}

int32 BBinder::Token()
{
	BAutolock _auto1(m_lock);
	if (m_token == (NO_TOKEN - 1)) return NO_TOKEN;
	if (m_token == NO_TOKEN) m_token = m_dispatcher->ObtainToken(this);
	return m_token;
}

static bool get_handler_token(int16 /*type*/, void* data)
{
	((BBinder*)data)->IncRefs(gDefaultTokens);
	return true;
}

atom_ref<BBinder> BBinder::LookupByToken(int32 token)
{
	if (token != NO_TOKEN) {
		BBinder* h = NULL;
		if (gDefaultTokens->GetToken(token, 100,
									 (void**)&h, get_handler_token) >= B_OK) {
			atom_ref<BBinder> ar = h;
			h->DecRefs();
			return ar;
		}
	}
	
	return NULL;
}

status_t
BBinder::Mount(const char *path)
{
	printf("Binder::Mount...\n");
	if(strcmp(path, "/binder") != 0) {
		printf("Binder::Mount... failed, %s is not the root path\n", path);
		return B_ERROR;
	}
	status_t err;

	rootProxyRefLock.Lock();
	if(binderDesc < 0) {
		binderDesc = open_driver();
	}
	if(rootNodeHandle != 0) {
		binder_cmd cmd;
		cmd.command = BINDER_DISCONNECT;
		cmd.node_handle = rootNodeHandle;
		BBinder::do_binder_command(&cmd);
		rootNodeHandle = 0;
	}
	binder_cmd cmd;
	cmd.command = BINDER_SET_ROOT_NODE_ID;
	cmd.data.nodeid.port = Port();
	cmd.data.nodeid.token = Token();
	err = do_binder_command(&cmd);
	rootProxyRefLock.Unlock();
	if(err < B_OK)
		return err;

	err = StartHosting();

	return err;
}

status_t
BBinder::StartHosting()
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
BBinder::SetPrivateFlags(uint32 flag, bool on)
{
	if(on && (flag & pfRedirect) && m_hostingNodeHandle == 0) {
		printf("Binder: SetPrivateFlags pfRedirect on unhosted node\n");
		return B_BAD_VALUE;
	}

	if (!m_linkData && !on) return B_OK;

	m_lock.Lock();
	if (!m_linkData) m_linkData = new _link_data;
	m_lock.Unlock();

	if (on) atomic_or(&m_linkData->flags,flag);
	else atomic_and(&m_linkData->flags,~flag);

	return B_OK;
}

BBinder::property 
BBinder::Root()
{
	return rootNode();
//	new BinderProxy(open("/binder",O_RDWR|O_CLOEXEC));
}

BBinder::property 
BBinder::Property(const char *path, BBinder::property_list &args)
{
	BBinder::property prop;
	GetProperty(path,prop,args);
	return prop;
}

BBinder::property 
BBinder::Property(const char *path, const BBinder::property *argList, ...)
{
	va_list vl;
	BBinder::property *arg;
	BBinder::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BBinder::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BBinder::property *))) { args.AddItem(arg); i++; };
	va_end(vl);
	
	return Property(path,args);
}

class BBinderListenerToCallback : public BBinderListener {

	public:

		void *							m_userData;
		BBinder::observer_callback 	m_callbackFunc;
			
		BBinderListenerToCallback::BBinderListenerToCallback(
			binder node,
			void *userData,
			BBinder::observer_callback callbackFunc,
			uint32 observeMask,
			const char *name) {
			m_userData = userData;
			m_callbackFunc = callbackFunc;
			StartListening(node,observeMask,name);
		}
		
		status_t Overheard(binder , uint32 observed, BString name)
		{
			return m_callbackFunc(m_userData,observed,(void*)name.String());
		}
};

BBinder::observer_token 
BBinder::AddObserverCallback(void *userData, observer_callback callbackFunc, uint32 observeMask, const char *name)
{
	return (observer_token) new BBinderListenerToCallback(this,userData,callbackFunc,observeMask,name);
}

status_t 
BBinder::RemoveObserverCallback(observer_token observer)
{
	BBinderListener *ptr = (BBinderListener*)observer;
	atom_ref<BBinderListener> obj = ptr;
	return RemoveObserver(obj);
}

status_t 
BBinder::NotifyListeners(uint32 event, const char *name)
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

status_t BBinder::_link(uint32 linkFlags, binder node)
{
	int32 i;
	m_lock.Lock();

	if (!m_linkData) m_linkData = new _link_data;

	for (i=0;i<m_linkData->list.Count();i++) {
		if (m_linkData->list[i].node == node) {
			if ((m_linkData->list[i].mask & linkFlags) == linkFlags) {
				m_lock.Unlock();
				return B_OK;
			} else if ((m_linkData->list[i].mask & (linkFlags & B_LINK_FROM)) != (linkFlags & B_LINK_FROM)) {
				m_lock.Unlock();
				return B_ERROR;
			} else
				break;
		}
	}
	
	if (i == m_linkData->list.Count()) {
		i = m_linkData->list.AddItem();
		m_linkData->list[i].node = node;
	}

	m_linkData->list[i].mask |= linkFlags;
	m_lock.Unlock();

	if (!(linkFlags & B_LINK_FROM)) {
		property flags = (int32)(linkFlags | B_LINK_FROM);
		property linkTo = this;
		node->Property("_link",&flags,&linkTo,NULL);
		WillRewritten(node,B_SOMETHING_CHANGED,NULL);
	}

	return B_OK;
}

status_t BBinder::_unlink(uint32 linkFlags, binder node)
{
	int32 i;
	
	if (!m_linkData) return B_ERROR;
	
	m_lock.Lock();
	if (m_linkData) {
		SmartArray< atom_ref<BBinder> > listCopy;
		for (i=0;i<m_linkData->list.Count();i++) {
			if (!node.ptr() || (m_linkData->list[i].node == node)) {
				if ((m_linkData->list[i].mask & linkFlags) == linkFlags) {
					listCopy.AddItem(m_linkData->list[i].node);
					m_linkData->list.RemoveItem(i--);
				}
			}
		}
		m_lock.Unlock();

		if (listCopy.Count() && !(linkFlags & B_LINK_FROM)) {
			property_list args;
			property linkFlags((int32)(linkFlags | B_LINK_FROM));
			property linkTo(this);
			args.AddItem(&linkFlags);
			args.AddItem(&linkTo);
			for (i=0;i<listCopy.Count();i++) Property("_unlink",args);
		}

		return B_OK;
	}
	
	m_lock.Unlock();
	return B_ERROR;
}

void BBinder::AddObserver(const atom_ref<BBinderListener> &observer, uint32 eventMask, const char *name)
{
	int32 i;
	eventMask &= B_ALL_USER;

	BBinderProxy *proxy = dynamic_cast<BBinderProxy*>(this);
	if (proxy) proxy->StartWatching();

	m_lock.Lock();
	if (!m_linkData) m_linkData = new _link_data;
	i = m_linkData->list.AddItem();
	m_linkData->list[i].listener = observer;
	m_linkData->list[i].mask = eventMask;
	if (name) m_linkData->list[i].name = name;
	m_lock.Unlock();
}
		
status_t BBinder::RemoveObserver(const atom_ref<BBinderListener> &observer)
{
	int32 i,err=B_ERROR;

	if (!m_linkData) return err;

	m_lock.Lock();
	if (m_linkData) {
		for (i=0;i<m_linkData->list.Count();i++) {
			if (m_linkData->list[i].listener == observer) {
				m_linkData->list.RemoveItem(i--);
				err = B_OK;
			}
		}
	}
	m_lock.Unlock();
	return err;
}

status_t BBinder::WillRewritten(binder /*disgruntledAncestor*/, uint32 event, const char *name)
{
	NotifyListeners(event,name);
}

void BBinder::DoNotification(uint32 event, const char *name)
{
	if (!m_linkData) return;

	m_lock.Lock();
	if (m_linkData) {
		int32 i;
		SmartArray< atom_ptr<BBinderListener> > listCopy;
		SmartArray< atom_ptr<BBinder> > inherits;
	
		for (i=0;i<m_linkData->list.Count();i++) {
			_link_data::link &link = m_linkData->list[i];
			if ((link.mask & event) &&
				(!(link.mask & B_NAME_KNOWN) ||
				 !(event & B_NAME_KNOWN) ||
				 (name && (link.name == name)))) {
				atom_ptr<BBinderListener> ptr = link.listener.acquire();
				if (!ptr.ptr()) m_linkData->list.RemoveItem(i--);
				else listCopy.AddItem(ptr);
			} else if ((link.mask & (B_INHERIT|B_LINK_FROM)) == (B_INHERIT|B_LINK_FROM)) {
				atom_ptr<BBinder> ptr = link.node.acquire();
				if (!ptr.ptr()) m_linkData->list.RemoveItem(i--);
				else inherits.AddItem(ptr);
			}
		}
		m_lock.Unlock();
		if (listCopy.Count()) {
			BString nameStr;
			if (name) nameStr = (const char*)name;
			for (i=0;i<listCopy.Count();i++)
				listCopy[i]->Overheard(this,event,nameStr);
		}
		if (inherits.Count()) {
			property_list args;
			property theOrigin(this);
			property flags((int32)event);
			property propName;
			args.AddItem(&theOrigin);
			args.AddItem(&flags);
			if (name) {
				propName = (const char*)name;
				args.AddItem(&propName);
			}
			for (i=0;i<inherits.Count();i++) {
				inherits[i]->Property("_rewriteWill",args);
			}
		}
	} else
		m_lock.Unlock();
}

BBinder::iterator::iterator(BBinder *node)
{
	if (!node || (node->OpenProperties(&cookie,NULL) < 0)) cookie = NULL;
	else parent = node;
}

BBinder::iterator::~iterator()
{
	if (cookie) parent->CloseProperties(cookie);
}

const BBinder::iterator &
BBinder::iterator::operator=(const iterator &copy)
{
	if (cookie) parent->CloseProperties(cookie);

	BBinder *node = copy.parent.ptr();
	if (!node || (node->OpenProperties(&cookie,copy.cookie) < 0)) cookie = NULL;
	else parent = node;
	
	return *this;
}

BString 
BBinder::iterator::Next()
{
	if (cookie) {
		char buf[1024];
		int32 len=128;
		if (parent->NextProperty(cookie,buf,&len) == B_OK)
			return BString(buf);
	}

	return BString();
}

BBinder::iterator 
BBinder::Properties()
{
	return iterator(this);
}

put_status_t 
BBinder::PutProperty(const char *name, const BBinder::property &property)
{
	if (m_linkData && (m_linkData->flags & pfRedirect)) return kPutProperty(m_hostingNodeHandle,name,property);
	return WriteProperty(name,property);
}

get_status_t 
BBinder::GetProperty(const char *name, BBinder::property &property, const BBinder::property_list &args)
{
	if (m_linkData && (m_linkData->flags & pfRedirect)) return kGetProperty(m_hostingNodeHandle,name,property,args);
	return ReadProperty(name,property,args);
}

get_status_t 
BBinder::GetProperty(const char *name, BBinder::property &property, const BBinder::property *argList, ...)
{
	va_list vl;
	BBinder::property *arg;
	BBinder::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BBinder::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BBinder::property *))) { args.AddItem(arg); i++; };
	va_end(vl);
	
	return GetProperty(name,property,args);
}

uint32 
BBinder::Flags() const
{
	return 0;
}

status_t 
BBinder::OpenProperties(void **, void *)
{
	return B_OK;
}

status_t 
BBinder::NextProperty(void *, char *, int32 *)
{
	return ENOENT;
}

status_t 
BBinder::CloseProperties(void *)
{
	return B_OK;
}

put_status_t 
BBinder::WriteProperty(const char *, const BBinder::property &)
{
	return put_status_t(B_ERROR,true);
}

get_status_t 
BBinder::ReadProperty(const char *name, BBinder::property &return_val, const BBinder::property_list &args)
{
	if (!strcmp(name,"_link")) {
		if (args.Count() < 2) return B_ERROR;
		return_val = _link(args[0].Number(),args[1].Object());
		return B_OK;
	} else if (!strcmp(name,"_unlink")) {
		binder node;
		if (args.Count() < 1) return B_ERROR;
		if (args.Count() == 2) node = args[1].Object();
		return_val = _unlink(args[0].Number(),node);
		return B_OK;
	} else if (!strcmp(name,"_rewriteWill")) {
		if (args.Count() < 2) return B_ERROR;
		const char *propName = NULL;
		if ((args.Count() >= 3) && args[2].String().Length()) propName = args[2].String().String();
		WillRewritten(args[0].Object(),args[1].Number(),propName);
		return B_OK;
	}
	
	get_status_t gst = ENOENT;
	if (!m_linkData) return gst;

	m_lock.Lock();
	if (m_linkData) {
		for (int32 i=0;i<m_linkData->list.Count();i++) {
			if ((m_linkData->list[i].mask & (B_INHERIT|B_LINK_FROM)) == B_INHERIT) {
				atom_ptr<BBinder> ptr = m_linkData->list[i].node.acquire();
				gst = ptr->GetProperty(name,return_val,args);
				if (!gst.error) break;
			}
		}
	}
	m_lock.Unlock();
	
	return gst;
}

int BBinder::kSeverLinks(int32 desc, uint32 linkFlags)
{
	binder_cmd cmd;
	cmd.command = BINDER_BREAK_LINKS;
	cmd.data.break_links.flags = linkFlags;
	cmd.node_handle = desc;
	return BBinder::do_binder_command(&cmd);
}

status_t 
BBinder::Unstack()
{
	return kSeverLinks(m_hostingNodeHandle,linkAugment|linkFilter|linkTo);
}

status_t 
BBinder::Topple()
{
	return kSeverLinks(m_hostingNodeHandle,linkAugment|linkFilter|linkFrom);
}

status_t 
BBinder::RenounceAncestry()
{
	property flags((int32)B_INHERIT);
	return (status_t)Property("_unlink",&flags,NULL).Number();
}

status_t 
BBinder::DisownChildren()
{
	property flags((int32)(B_INHERIT|B_LINK_FROM));
	return (status_t)Property("_unlink",&flags,NULL).Number();
}

status_t 
BBinder::StackOnto(binder object)
{
	StartHosting();
	
	binder_cmd cmd;
	uint32 desc;
	BBinder::property prop(object);

	cmd.command = BINDER_STACK;
	cmd.data.node_handle = m_hostingNodeHandle;
	//cmd.data.nodeid.port = Port();
	//cmd.data.nodeid.token = Token();
	prop.Remotize();
	desc = prop.m_value.descriptor;
	if(prop.m_format == BBinder::property::f_object)
		desc = ((BBinderProxy*)prop.m_value.object)->m_descriptor;
	cmd.node_handle = desc;
	return do_binder_command(&cmd);
}

status_t 
BBinder::InheritFrom(binder object)
{
	property flags((int32)B_INHERIT);
	property linkTo(object);
	return (status_t)Property("_link",&flags,&linkTo,NULL).Number();
/*
	StartHosting();

	binder_cmd cmd;
	uint32 desc;

	BBinder::property prop(object);

	prop.Remotize();
	desc = prop.m_value.descriptor;
	if(prop.m_format == BBinder::property::f_object)
		desc = ((BinderProxy*)prop.m_value.object)->m_descriptor;
	cmd.command = BINDER_INHERIT;
	cmd.data.node_handle = desc;
	cmd.node_handle = m_hostingNodeHandle;
	return do_binder_command(&cmd);
*/
}

binder BBinder::Copy(bool)
{
	return new BBinder();
}

/************************************************************************************/

BBinderObserver::BBinderObserver(const binder &node, uint32 eventMask, const char *name)
{
	m_object = node;
	m_token = node->AddObserverCallback(this,(BBinder::observer_callback)Callback,eventMask,name);
}

BBinderObserver::~BBinderObserver()
{
	m_object->RemoveObserverCallback(m_token);
	
}

const binder &
BBinderObserver::Object()
{
	return m_object;
}

status_t 
BBinderObserver::ObservedChange(uint32 , const char *)
{
	return B_OK;
}

status_t 
BBinderObserver::Callback(BBinderObserver *This, uint32 eventMask, void *name)
{
	return This->ObservedChange(eventMask,(char *)name);
}

/************************************************************************************/

BBinderListener::BBinderListener()
{
}

status_t 
BBinderListener::StartListening(binder node, uint32 eventMask, const char *propertyName)
{
	node->AddObserver(this,eventMask,propertyName);
}

status_t 
BBinderListener::StopListening(binder node)
{
	node->RemoveObserver(this);
}

BBinderListener::~BBinderListener()
{
}

status_t 
BBinderListener::Overheard(binder , uint32, BString)
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

BBinderProxy::BBinderProxy(uint32 descriptor)
{
	m_flags = 0;
	m_descriptor = descriptor;
}

void BBinderProxy::StartWatching()
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
BBinderProxy::Released()
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
			printf("BBinderProxy::StopWatching failed, %s\n", strerror(err));
		}
		//_kstop_notifying_(Port(),Token());
	}
	BBinder::Released();
}

BBinderProxy::~BBinderProxy()
{
	if(m_descriptor != 0) {
		binder_cmd cmd;
		cmd.command = BINDER_DISCONNECT;
		cmd.node_handle = m_descriptor;
		do_binder_command(&cmd);
	}
}

binder BBinderProxy::Copy(bool)
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

	return new BBinderProxy(cmd.data.connect.return_node_handle);
}

uint32 
BBinderProxy::Flags() const
{
	uint32 flags = 0;
	if (m_descriptor != 0) flags |= isValid;
	return flags;
}

status_t 
BBinderProxy::OpenProperties(void **cookie, void *copyCookie)
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
BBinderProxy::NextProperty(void *cookie, char *nameBuf, int32 *len)
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
BBinderProxy::CloseProperties(void *cookie)
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
BBinderProxy::ReadProperty(const char *name, BBinder::property &property, const BBinder::property_list &args)
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
BBinderProxy::WriteProperty(const char *name, const BBinder::property &property)
{
	m_cache.Remove(name);
	return kPutProperty(m_descriptor,name,property);
}

void 
BBinderProxy::DoNotification(uint32 event, const char *name)
{
	/* flush some cache, if neccessary */
	if (event & B_SOMETHING_CHANGED) {
		if (event & B_NAME_KNOWN)
			m_cache.Remove(name);
		else
			m_cache.MakeEmpty();
	}
	BBinder::DoNotification(event,name);
}

/************************************************************************************/

BBinder::property BBinder::property::undefined;

status_t 
BBinder::property::InstantiateRemote()
{
	if (m_format == f_descriptor) {
		BBinder *node = new BBinderProxy(m_value.descriptor);
		node->Acquire(this);
		m_format = f_object;
		m_value.object = node;
	}
	return B_OK;
}

BBinder::property::type BBinder::property::Type() const
{
	return m_type;
}

int32 
BBinder::property::FlattenedSize() const
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
BBinder::property::FlattenTo(void *buffer) const
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
			BBinderProxy *proxy = dynamic_cast<BBinderProxy*>(m_value.object);
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
BBinder::property::UnflattenFrom(void *buffer)
{
	Undefine();

	uint8 *buf = (uint8*)buffer;
	int32 t = *((int32*)buf); buf += 4;
//	printf("Unflattening %ld\n",t);
	m_type = (BBinder::property::type)t;
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
		atom_ptr<BBinder> h;
		get_port_info(node_id.port,&pi);
		get_thread_info(find_thread(NULL),&ti);
		if ((pi.team == ti.team) && (node_id.token != (uint32)NO_TOKEN) &&
			(h = BBinder::LookupByToken(node_id.token).acquire()).ptr()) {
			m_type = BBinder::property::object;
			m_value.object = h.ptr();
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
BBinder::property::Remotize()
{
	if (m_format == f_object) {
		status_t err;
		uint32 desc = 0;
		BBinderProxy *proxy = dynamic_cast<BBinderProxy*>(m_value.object);
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
BBinder::property::Number() const
{
	if (m_format == f_number) return m_value.number;
	double d = 0;
	if (m_format == f_string) sscanf(m_value.string,"%lf",&d);
	return d;
}

BString 
BBinder::property::String() const
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
BBinder::property::TypedRaw() const
{
	if (m_type != typed_raw) return NULL;
	
	return (binder_typed_raw *)m_value.typed_raw;
}

atom_ptr<BBinder> 
BBinder::property::Object() const
{
	BBinder::property &t = const_cast<BBinder::property&>(*this);
	t.InstantiateRemote();
	if (m_format == f_object) return m_value.object;
	return dummyNode();
}

void 
BBinder::property::Undefine()
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
BBinder::property::operator>=(const BBinder::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) >= ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) >= ((BString)other);
	return false;
}

bool 
BBinder::property::operator<=(const BBinder::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) <= ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) <= ((BString)other);
	return false;
}

bool 
BBinder::property::operator>(const BBinder::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) > ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) > ((BString)other);
	return false;
}

bool 
BBinder::property::operator<(const BBinder::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) < ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) < ((BString)other);
	return false;
}

bool 
BBinder::property::operator==(const BBinder::property &other) const
{
	if ((m_type == null) && (other.m_type == null)) return true;
	if (IsNumber() && other.IsNumber())
		return ((double)*this) == ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) == ((BString)other);
	return false;
}

bool 
BBinder::property::operator!=(const BBinder::property &other) const
{
	if (IsNumber() && other.IsNumber())
		return ((double)*this) != ((double)other);
	if (IsString() && other.IsString())
		return ((BString)*this) != ((BString)other);
	return false;
}

const BBinder::property &
BBinder::property::operator=(const BBinder::property &other)
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
const BBinder::property &
BBinder::property::operator=(const property_ref &ref)
{
	*this = ref();
	return *this;
}

const BBinder::property &
BBinder::property::operator=(class Binder *reg)
{
	Undefine();
	
	BBinderProxy *proxy = dynamic_cast<BBinderProxy*>(reg);
	if (proxy)	m_type = remote_object;
	else		m_type = object;
	m_format = f_object;
	m_value.object = reg;
	m_value.object->Acquire();
	return *this;
}

const BBinder::property &
BBinder::property::operator=(const char *str)
{
	Undefine();
	m_type = string;
	m_format = f_string;
	m_value.string = strdup(str);
	return *this;
}

const BBinder::property &
BBinder::property::operator=(double num)
{
	Undefine();
	m_type = number;
	m_format = f_number;
	m_value.number = num;
	return *this;
}
*/
bool 
BBinder::property::IsUndefined() const
{
	return (m_type == null);
}

bool 
BBinder::property::IsNumber() const
{
	return (m_type == number);
}

bool 
BBinder::property::IsString() const
{
	return (m_type == string);
}

bool 
BBinder::property::IsObject() const
{
	return (m_type == object);
}

bool 
BBinder::property::IsTypedRaw() const
{
	return (m_type == typed_raw);
}

bool 
BBinder::property::IsRemoteObject() const
{
	return	(m_type == object) &&
			(
				(m_format == f_descriptor) ||
				(
					(m_format == f_object) &&
					dynamic_cast<BBinderProxy*>(m_value.object)
				)
			);
}

BBinder::property::property()
{
	m_type = null;
	m_format = f_null;
}

BBinder::property::property(const BBinder::property &other)
{
	m_type = null;
	m_format = f_null;
	*this = other;
}

BBinder::property::property(const BBinder::property_ref &ref)
{
	m_type = null;
	m_format = f_null;
	if (ref.m_base.ptr()) {
		if (ref.m_name) ref.m_base->GetProperty(ref.m_name,*this);
		else *this = ref.m_base;
	}
}

void BBinder::property::Init(BBinder *value)
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

void BBinder::property::Init(double value)
{
	m_type = number;
	m_format = f_number;
	m_value.number = value;
}

void BBinder::property::Init(const char *value)
{
	m_type = string;
	m_format = f_string;
	m_value.string = strdup(value);
}

void BBinder::property::Init(type_code type, void *value, size_t len)
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

BBinder::property::~property()
{
	Undefine();
}

BBinder *
BBinder::property::operator->()
{
	InstantiateRemote();
	return (m_format == f_object)?m_value.object:dummyNode();
}

BBinder::property_ref
BBinder::property::operator/(const char *name)
{
	InstantiateRemote();
	return property_ref((m_format == f_object)?m_value.object:NULL,name);
}

BBinder::property_ref
BBinder::property::operator[](const char *name)
{
	InstantiateRemote();
	return property_ref((m_format == f_object)?m_value.object:NULL,name);
}

/************************************************************************************/

put_status_t 
BBinder::property_ref::Put(const property &prop) const
{
	if (m_base.ptr()) return m_base->PutProperty(m_name,prop);
	return B_BAD_VALUE;
}

get_status_t 
BBinder::property_ref::Get(property &returnVal) const
{
	returnVal.Undefine();
	if (m_base.ptr()) return m_base->GetProperty(m_name,returnVal);
	return B_BAD_VALUE;
}

get_status_t 
BBinder::property_ref::Get(property &returnVal, const property_list &args) const
{
	returnVal.Undefine();
	if (m_base.ptr()) return m_base->GetProperty(m_name,returnVal,args);
	return B_BAD_VALUE;
}

get_status_t 
BBinder::property_ref::Get(property &returnVal, const property *argList, ...) const
{
	va_list vl;
	BBinder::property *arg;
	BBinder::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BBinder::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BBinder::property *))) { args.AddItem(arg); i++; };
	va_end(vl);

	returnVal.Undefine();
	if (m_base.ptr()) return m_base->GetProperty(m_name,returnVal,args);
	return B_BAD_VALUE;
}

BBinder::property 
BBinder::property_ref::operator ()() const
{
	BBinder::property prop;
	if (m_base.ptr()) m_base->GetProperty(m_name,prop);
	return prop;
}

BBinder::property 
BBinder::property_ref::operator ()(const property_list &args) const
{
	BBinder::property prop;
	if (m_base.ptr()) m_base->GetProperty(m_name,prop,args);
	return prop;
}

BBinder::property
BBinder::property_ref::operator ()(const property *argList, ...) const
{
	va_list vl;
	BBinder::property *arg;
	BBinder::property_list args;
	int32 i=0;

	va_start(vl,argList);
	if (argList) { args.AddItem(const_cast<BBinder::property*>(argList)); i++; };
	while ((arg = va_arg(vl,BBinder::property *))) { args.AddItem(arg); i++; };
	va_end(vl);
	
	BBinder::property prop;
	if (m_base.ptr()) m_base->GetProperty(m_name,prop,args);
	return prop;
}

/*
BBinder::property_ref::operator BBinder::property() const
{
	BBinder::property prop;
	if (m_base) m_base->GetProperty(m_name,prop);
	return prop;
}
*/

const BBinder::property_ref &
BBinder::property_ref::operator=(const BBinder::property &prop)
{
	if (m_base.ptr()) m_base->PutProperty(m_name,prop);
	return *this;
}

BBinder::property_ref::property_ref(BBinder *base, const char *name)
{
	m_base = base;
	m_name = strdup(name);
	if (!m_name) m_name = strdup("");
}

BBinder::property_ref::~property_ref()
{
	free(m_name);
}

/************************************************************************************/

enum {
	bcfOrdered = 0x00000001
};

BBinderContainer::BBinderContainer()
{
	m_flags = 0;
	m_perms = permsRead;
}

BBinderContainer::BBinderContainer(BBinderContainer *other, bool deep) : BBinder(other,deep)
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

binder 
BBinderContainer::Copy(bool deep)
{
	return new BBinderContainer(this,deep);
}

BBinderContainer::~BBinderContainer()
{
}

status_t 
BBinderContainer::WillRewritten(binder disgruntledAncestor, uint32 event, const char *name)
{
	int32 i;
	if (!name || !FindName(name,i)) BBinder::WillRewritten(disgruntledAncestor,event,name);
}

bool 
BBinderContainer::Ordered()
{
	return m_flags & bcfOrdered;
}

status_t 
BBinderContainer::SetOrdered(bool isOrdered)
{
	if (isOrdered)
		m_flags |= bcfOrdered;
	else
		m_flags &= ~bcfOrdered;
	return B_OK;
}

uint32
BBinderContainer::Flags() const
{
	return isValid;
}

struct store_cookie {
	int32 index;
};

status_t 
BBinderContainer::OpenProperties(void **cookie, void *copyCookie)
{
	store_cookie *c = new store_cookie;
	if (copyCookie) *c = *((store_cookie*)copyCookie);
	else c->index = 0;
	*cookie = c;
	return B_OK;
}

status_t 
BBinderContainer::NextProperty(void *cookie, char *nameBuf, int32 *len)
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
BBinderContainer::RewindProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	c->index = 0;
	return B_OK;
}

status_t 
BBinderContainer::CloseProperties(void *cookie)
{
	store_cookie *c = (store_cookie*)cookie;
	delete c;
	return B_OK;
}

bool 
BBinderContainer::FindName(const char *name, int32 &index)
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
BBinderContainer::AddProperty(const char *name, const BBinder::property &prop, uint16 perms)
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
BBinderContainer::RemoveProperty(const char *name)
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
BBinderContainer::HasProperty(const char *name)
{
	int32 i;
	BAutolock _auto(m_listLock);
	return FindName(name,i);
}

uint16 
BBinderContainer::Permissions()
{
	return m_perms;
}

status_t 
BBinderContainer::SetPermissions(uint16 perms)
{
	m_perms = perms;
	return B_OK;
}

put_status_t 
BBinderContainer::WriteProperty(const char *name, const BBinder::property &prop)
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
BBinderContainer::ReadProperty(const char *name, BBinder::property &property, const BBinder::property_list &args)
{
	int32 i;
	property.Undefine();

	BAutolock _auto(m_listLock);

	uint16 perms = m_perms;

	if (FindName(name,i)) {
		if (!(m_propertyList[i].perms & permsInherit)) perms = m_propertyList[i].perms;
		if (!(perms & permsRead)) return EPERM;
		if (!m_propertyList[i].value.IsUndefined()) {
			property = m_propertyList[i].value;
			return get_status_t(B_OK,true);
		}
	}

	return BBinder::ReadProperty(name,property,args);
}
