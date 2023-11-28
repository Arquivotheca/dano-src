
#include <support2_p/BinderIPC.h>

#include <os_p/priv_syscalls.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

namespace B {
namespace Private {

IBinder::ptr	root;
BLocker			rootLock;
int32			driverDesc = -1;
uint32			rootNodeHandle = 0;

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

int32 open_driver()
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

const IBinder::ptr & get_root()
{
	if(rootNodeHandle == 0) {
		rootLock.Lock();
		if(driverDesc < 0) driverDesc = open_driver();
		if(driverDesc >= 0 && rootNodeHandle == 0) {
			status_t err;
			binder_cmd cmd;
			cmd.command = BINDER_GET_ROOT_NODE_ID;
			err = do_binder_command(&cmd);
			if(err >= B_OK) {
				err = open_node(cmd.data.nodeid, &rootNodeHandle);
				if(err >= B_OK)
					root = new RBinder(rootNodeHandle);
				else
					rootNodeHandle = 0;
			}
		}
		rootLock.Unlock();
	}
	return root;
}

int32 open_node(struct binder_node_id &node_id, uint32 *node_handle, bool startHosting)
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

int32 do_binder_command(binder_cmd *cmd, IBinder::value *prop)
{
#if 0
	status_t err;
	cmd->reflection.cookie = 0;
	while((!(err = _kioctl_(driverDesc,BINDER_CMD,cmd,sizeof(binder_cmd)))) &&
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

		binder n = BDispatcher::LookupToken(cmd->reflection.token);
		cmd->reflection.reflection_result = B_BAD_VALUE;
		if(n == NULL) {
			printf("Binder: no node for reflection token %d\n",
			       (int)cmd->reflection.token);
			continue;
		}
		switch (cmd->reflection.command) {
			case REFLECT_CMD_GET:
				if (!prop/* || !args*/) {
					printf("Got unexpected get reflect on non-get operation!\n");
					cmd->reflection.reflection_result = err = B_BAD_VALUE;
					break;
				}
				//printf("Successfully reflecting get of '%s'\n",cmd->data.get.name);
				cmd->data.get.result.error = n->Get(cmd->data.get.name,*prop);
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
				cmd->data.put.result.error = n->Put(cmd->data.put.name,*prop);
				cmd->reflection.reflection_result = cmd->data.put.result.error;
				break;
			
			case REFLECT_CMD_RELEASE: {
				n->Release();
				cmd->reflection.reflection_result = B_OK;
			} break;

			case REFLECT_CMD_ACQUIRE: {
				n->Acquire();
				cmd->reflection.reflection_result = B_OK;
			} break;
/*
			case REFLECT_CMD_OPEN_PROPERTIES:
				//printf("Got REFLECT_CMD_OPEN_PROPERTIES reflection\n");
				cmd->reflection.reflection_result =
					n->OpenProperties(&cmd->data.property_iterator.local_cookie, NULL);
				if(cmd->reflection.reflection_result < B_OK)
					break; 
			
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
*/			
			case REFLECT_CMD_REDIRECT:
			case REFLECT_CMD_UNREDIRECT:
			default:
				printf("Got unexpected reflection! %d\n",
				       (int)cmd->reflection.command);
				break;
		}
	}
	return err;
#endif
}

int32 handle_reflect_punt(binder_cmd *cmd)
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
	else {
		int32 token;
		UspaceIO io(buf,buf_size);
		io.read(&token);
		IBinder::ptr b = BDispatcher::LookupToken(token);
		if (b != NULL) ((BBinder*)b.ptr())->ProcessMessage(io);
		else {
			io.write((port_id)-1);
			io.write((status_t)ENOENT);
			io.reply();
		}
	}
	free(buf);
	return B_OK;
}

#if 0
int32 get_property(uint32 handle, property_id name, IBinder::value &out)
{
	binder_cmd cmd;
	char stack_buffer[256];
	cmd.command = BINDER_GET_PROPERTY;
	copystring(cmd.data.get.name, name.Name(), sizeof(cmd.data.get.name) - 1);
	if (handle == 0) {
		printf("Binder: get_property: desc == 0\n");
		return B_ERROR;
	}
	cmd.node_handle = handle;
	cmd.data.get.return_node_handle = 0;
	cmd.data.get.returnBuf = stack_buffer;
	cmd.data.get.returnBufSize = sizeof(stack_buffer);
	
	*((int32*)cmd.data.get.returnBuf) = BBinder::value::null;
	cmd.data.get.argsSize = 0;
	cmd.data.get.args = NULL;
	cmd.data.get.successful_reflect = false;
	int32 err = do_binder_command(&cmd,&out);

	if (err == 0 && !cmd.data.get.successful_reflect) {
		if (cmd.data.get.return_node_handle != 0) {
			out = IBinder::value::undefined;
			
			binder_node_id node_id;
			node_id = *((binder_node_id*)(((uint8*)cmd.data.get.returnBuf)+4+sizeof(dev_t)));

			port_info pi;
			thread_info ti;
			IBinder::ptr h;
			get_port_info(node_id.port,&pi);
			get_thread_info(find_thread(NULL),&ti);
//				printf("************ %d,%d,%d\n",pi.team,ti.team,vnid.address.token);
			if ((pi.team == ti.team) &&
				(h = BDispatcher::LookupToken(node_id.token)) != NULL) {
				out = h;
//				out.m_type = BBinder::value::object;
//				out.m_value.object = h.ptr();
//				out.m_format = BBinder::value::f_object;
//				out.m_value.object->Acquire(&prop);

				binder_cmd dcmd;
				dcmd.command = BINDER_DISCONNECT;
				dcmd.node_handle = cmd.data.get.return_node_handle;
				do_binder_command(&dcmd);
			} else {
//					printf("grabbed remote object for '%s'\n",name);
				out = IBinder::ptr(new RBinder(cmd.data.get.return_node_handle));
//				out.m_type = BBinder::value::object;
//				out.m_format = BBinder::value::f_descriptor;
//				out.m_value.descriptor = cmd.data.get.return_node_handle;
			}
		} else
			out.UnflattenFrom(cmd.data.get.returnBuf);
	}
	if(cmd.data.get.returnBuf != stack_buffer)
		free(cmd.data.get.returnBuf);
//	if (data) free(data);
	if (err) cmd.data.get.result.error = err;
	return cmd.data.get.result.error;
}

int32 put_property(uint32 handle, property_id name, const IBinder::value &in)
{
	binder_cmd cmd;
	cmd.command = BINDER_PUT_PROPERTY;

	if(handle == 0) {
		printf("Binder: kPutProperty: desc == 0\n");
		return B_ERROR;
	}

	copystring(cmd.data.put.name, name.Name(), sizeof(cmd.data.put.name) - 1);
	cmd.node_handle = handle;
	void *data = cmd.data.put.value = malloc(cmd.data.put.valueLen = in.FlattenedSize());
	if(data == NULL)
		return B_NO_MEMORY;

	in.FlattenTo(data);

	int32 err = do_binder_command(&cmd,const_cast<BBinder::value*>(&in));

	if (data) free(data);
	if (err) cmd.data.put.result.error = err;
	return cmd.data.put.result.error;
}
#endif

} } // namespace B::Private
