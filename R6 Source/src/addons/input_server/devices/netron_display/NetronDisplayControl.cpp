#include "NetronDisplayControl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <Debug.h>
#include <Autolock.h>

#define DEVICE_NAME  "/dev/ports/serial1"
//#define DEVICE_NAME_2  "/dev/ports/serial2"

#define REQUEST_PACKET_HEADER 0x81
#define REQUEST_PACKET_TYPE_WRITE 0x01
#define REQUEST_PACKET_TYPE_READ 0x09

#define RESPONSE_PACKET_HEADER 0x90

static void
dump_data(char *prefix, uint8 *data, size_t data_len)
{
	const int line_len = 75;
	uint32 i;
	int prefix_len = strlen(prefix);
	char string_buf[line_len+1];
	char *cur_p;

	if(prefix_len+3 > line_len) {
		_sPrintf("bad prefix: %s", prefix);
		return;
	}
	strcpy(string_buf, prefix);
	cur_p = string_buf + prefix_len;
	for(i = 0; i < data_len; i++) {
		if(cur_p + 3 - string_buf > line_len) {
			_sPrintf("%s\n", string_buf);
			sprintf(string_buf, "%*s", prefix_len, "|");
			cur_p = string_buf + prefix_len;
		}
		sprintf(cur_p, " %02x", data[i]);
		cur_p += 3;
	}
	_sPrintf("%s\n", string_buf);
}


int32
NetronDisplayControl::input_loop(void *arg)
{
	((NetronDisplayControl *)arg)->ReadPackets();
	return 0;
}

status_t 
NetronDisplayControl::InitDevice(const char *const device_name)
{
	serial_fd = open(device_name, O_CLOEXEC|O_EXCL|O_RDWR|O_NONBLOCK);
	if(serial_fd < 0) {
		_sPrintf("Serial open failed, %s\n", strerror(errno));
		return errno;
	}
	fcntl(serial_fd, F_SETFL, fcntl(serial_fd, F_GETFL) & ~O_NONBLOCK);
	ioctl(serial_fd, TCGETA, &t);
	t.c_cflag &= ~ (CBAUD | CSIZE);
	t.c_cflag |= B9600 | CS8 | CLOCAL | CSTOPB;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	ioctl(serial_fd, TCSETA, &t);
	return B_NO_ERROR;
}

NetronDisplayControl::NetronDisplayControl(int debug_level)
	: debug_level(debug_level)
{
	serial_fd = -1;
	response_port = -1;
	input_thread = -1;
	old_display_protocol = false;
	pending_byte = -1;
	
	fail_count = 0;
	waiting_for_keymap_request = false;

	init_status = InitDevice(DEVICE_NAME);
	//if(init_status != B_NO_ERROR)
	//	init_status = InitDevice(DEVICE_NAME_2);
	if(init_status != B_NO_ERROR)
		return;
	
	response_port = create_port(1, "display command response");
	if(response_port < 0) {
		if(debug_level >= 1)
			_sPrintf("could not create port, %s\n", strerror(response_port));
		init_status = response_port;
		return;
	}

	input_thread = spawn_thread(input_loop, "monitor input",
	                            B_DISPLAY_PRIORITY, this);
	if(input_thread < 0) {
		if(debug_level >= 1)
			_sPrintf("could not start input thread, %s\n", strerror(input_thread));
		init_status = input_thread;
		return;
	}
	init_status = resume_thread(input_thread);
	if(init_status < B_NO_ERROR)
		return;
		
	key_state = ~0;
	init_status = EnableKeyMessages();

	if(key_state != ~0UL) {
		init_status = B_NO_ERROR;
	}
	else {
		uint8 dummy_val;
		if(debug_level >= 1)
			_sPrintf("Key Map Request returned without key map, "
			         "try old protocol\n");
		key_state = 0;
		old_display_protocol = true;
		init_status = GetRegister(0, &dummy_val);
	}
}

NetronDisplayControl::~NetronDisplayControl()
{
	status_t rv;
	if(!old_display_protocol) {
		status_t err;
		uint8 packet[] = { 0x81, 0x01, 0x60, 0x01, 0x00, 0xff };
		err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
		if(err != B_NO_ERROR) {
			if(debug_level >= 1)
				_sPrintf("Key Map message Off returned %s\n", strerror(err));
		}
	}
	kill_thread(input_thread);
	wait_for_thread(input_thread, &rv);
	close(serial_fd);
	delete_port(response_port);
}

status_t 
NetronDisplayControl::EnableKeyMessages()
{
	status_t err;

	{
		uint8 packet[] = { 0x81, 0x01, 0x60, 0x01, 0x01, 0xff };
		err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
		if(err != B_NO_ERROR) {
			if(debug_level >= 1)
				_sPrintf("Key Map message On failed\n");
		}
	}
	{
		uint8 packet[] = { 0x81, 0x01, 0x60, 0x02, 0x01, 0xff };
		waiting_for_keymap_request = true;
		err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
		if(err != B_NO_ERROR) {
			waiting_for_keymap_request = false;
			if(debug_level >= 1)
				_sPrintf("Key Map Request failed\n");
		}
	}
	return err;
}

status_t 
NetronDisplayControl::SendRawPacket(uint8 *packet, size_t packet_len)
{
	BAutolock autolock(send_lock);
	if(debug_level >= 2)
		dump_data("sent packet:                ->", packet, packet_len);
	if(write(serial_fd, packet, packet_len) != (ssize_t)packet_len) {
		if(debug_level >= 1)
			_sPrintf("write failed, %s\n", strerror(errno));
		return B_IO_ERROR;
	}
	return B_NO_ERROR;
}

status_t 
NetronDisplayControl::SendPacket(bigtime_t timeout,
                                 uint8 *packet, size_t packet_len,
                                 uint8 *response, size_t *response_len)
{
	status_t err = B_NO_ERROR;
	status_t response_code;
	ssize_t read_len;
	size_t response_size;

	for(int retry_count = 0; retry_count < 3; retry_count++) {
		if(response_len != NULL)
			response_size = *response_len;
		else
			response_size = 0;
	
	//	err = response_sem = create_sem(0, "display command complete");
	//	if(err < B_NO_ERROR)
	//		return err;
		if(read_port_etc(response_port, &response_code, NULL, 0,
		                 B_RELATIVE_TIMEOUT, 0) >= 0) {
			if(debug_level >= 1)
				_sPrintf("SendPacket: flushed previous response\n");
		}
		
		if(retry_count > 0 && debug_level >= 1)
			_sPrintf("SendPacket: retry\n");

		err = SendRawPacket(packet, packet_len);
		if(err != B_NO_ERROR)
			return err;
		read_len = read_port_etc(response_port, &response_code, response,
		                         response_size, B_RELATIVE_TIMEOUT, timeout);
		if(read_len < 0) {
			if(debug_level >= 1)
				if(err == B_TIMED_OUT)
					_sPrintf("SendPacket: no reponse\n");
				else
					_sPrintf("SendPacket: read_port failed, %s\n", strerror(read_len));
			err = read_len;
			fail_count++;
			continue;
		}
		if(response_code < 0)
			err = response_code;
		else
			err = B_NO_ERROR;
		//if(read_len > 0 && debug_level >= 2)
		//	dump_data("SendPacket: got data:", response, read_len);
		if(response_len && response_code >= 0) {
			*response_len = read_len;
			if((size_t)response_code != response_size)
				if(debug_level >= 2)
					_sPrintf("SendPacket got %d bytes, expected %d\n",
					       response_code, response_size);
			if((size_t)response_code > response_size)
				err = EMSGSIZE;
		}
		if(err == B_NO_ERROR)
			return err;
	//	err = acquire_sem_etc(response_sem, 1, B_RELATIVE_TIMEOUT, 200000);
	//	if(err != B_NO_ERROR) {
	//		_sPrintf("SendPacket failed %s\n", strerror(err));
	//	}
	//	delete_sem(response_sem);
	//	response_sem = -1;
	}
	return err;
}

status_t
NetronDisplayControl::KeyPressed(uint8 key_code, bool down)
{
	if(debug_level >= 1) {
		if(down)
			_sPrintf("Key down: ");
		else
			_sPrintf("Key up: ");
		switch(key_code) {
			case POWER_KEY:        _sPrintf("Power\n"); break;
			case EMAIL_KEY:        _sPrintf("E-mail\n"); break;
			case MEDIA_KEY:        _sPrintf("Media\n"); break;
			case WEB_KEY:          _sPrintf("Web\n"); break;
			case VOLUME_UP_KEY:    _sPrintf("Volume Up\n"); break;
			case VOLUME_DOWN_KEY:  _sPrintf("Volume Down\n"); break;
	
			case CALENDAR_KEY:     _sPrintf("Old Calendar\n"); break;
			case IRADIO_KEY:       _sPrintf("Old iRadio\n"); break;
			case MENU_KEY:         _sPrintf("Old Menu\n"); break;
			default:
				_sPrintf("unknown key 0x%02x\n", key_code);
		}
	}
	return B_NO_ERROR;
}

status_t 
NetronDisplayControl::TestPattern(uint8 pattern)
{
	_sPrintf("Got test pattern 0x%02x\n", pattern);
	return B_NO_ERROR;
}

status_t 
NetronDisplayControl::PowerOnReset()
{
	return B_NO_ERROR;
}

void 
NetronDisplayControl::ProcessNewKeyState(uint32 new_key_state)
{
	if(debug_level >= 2)
		_sPrintf("key state 0x%08x -> 0x%08x\n", key_state, new_key_state);
	if(key_state != ~0UL)
		for(int i = 0; i < 31; i++) {
			if((key_state & 1 << i) != (new_key_state & 1 << i))
				KeyPressed(i, (new_key_state & 1 << i) != 0);
		}
	key_state = new_key_state;
}

status_t
NetronDisplayControl::ParseRequestPacket(uint8 *data, size_t data_len)
{
	switch(data[1]) {
		case REQUEST_PACKET_TYPE_WRITE:
			switch(data[2]) {
				case 0x61: {
					uint32 new_key_state;
					if(old_display_protocol) {
						switch(data[3]) {
							case OLD_NO_KEY:
								new_key_state = 0;
								break;
							case OLD_POWER_KEY:
								new_key_state = key_state | 1 << POWER_KEY;
								break;
							case OLD_VOLUME_UP:
								new_key_state = key_state | 1 << VOLUME_UP_KEY;
								break;
							case OLD_VOLUME_DOWN:
								new_key_state = key_state | 1 << VOLUME_DOWN_KEY;
								break;
							case OLD_EMAIL_KEY:
								new_key_state = key_state | 1 << EMAIL_KEY;
								break;
							case OLD_WEB_KEY:
								new_key_state = key_state | 1 << WEB_KEY;
								break;
							case OLD_CALENDAR_KEY:
								new_key_state = key_state | 1 << CALENDAR_KEY;
								break;
							case OLD_IRADIO_KEY:
								new_key_state = key_state | 1 << IRADIO_KEY;
								break;
							case OLD_MENU_KEY:
								new_key_state = key_state | 1 << MENU_KEY;
								break;
							default:
								new_key_state = key_state;
								_sPrintf("Got unknown keycode\n");
						}
					}
					else {
						new_key_state = data[3];
					}
					
					ProcessNewKeyState(new_key_state);
					
					if(waiting_for_keymap_request) {
						waiting_for_keymap_request = false;
						write_port_etc(response_port, B_NO_ERROR, NULL, 0,
						               B_RELATIVE_TIMEOUT, 0);
					}

					} break;
				case 0x62:	return TestPattern(data[3]);
				case 0x70:
					if(data[3] == 0x50 && data[4] == 0xff) {
						if(debug_level >= 1)
							_sPrintf("got Power On Reset\n");
						return PowerOnReset();
					}
					else
						if(debug_level >= 1)
							dump_data("got write request: ", data, data_len);
					break;
				default:
					if(debug_level >= 1)
						dump_data("got write request: ", data, data_len);
			}
			break;
		case REQUEST_PACKET_TYPE_READ:
			if(debug_level >= 1)
				dump_data("got read request from display: ", data, data_len);
			break;
		default:
			if(debug_level >= 1)
				dump_data("got unknown request packet: ", data, data_len);
			return B_IO_ERROR;
	}
	return B_NO_ERROR;
}

status_t
NetronDisplayControl::ParseResponsePacket(uint8 *data, size_t data_len)
{
	switch(data[1]) {
		case 0x50: { /* Completion */
#if 0
			int i;
			if(debug_level >= 2) {
				if(data[2] == 0xff)
					_sPrintf("Got Completion Packet (no data)");
				else
					_sPrintf("Got Completion Packet, data:");
				for(i = 2; i < data_len - 1; i++) {
					_sPrintf(" %02x", data[i]);
	#if 0
					if((data[i] & 0xf0) != 0x00) {
						_sPrintf("bad data\n");
						dump_data("packet: ", data, data_len);
						return B_NO_ERROR;
	//					return B_IO_ERROR;
					}
					if(i % 2 == 0)
						_sPrintf(" ");
					_sPrintf("%01x", data[i]);
	#endif
				}
				_sPrintf("\n");
			}
#endif
			write_port_etc(response_port, data_len-3, data+2, data_len-3,
			               B_RELATIVE_TIMEOUT, 0);
			//if(response_sem >= 0)
			//	release_sem(response_sem);
			//else
			//	_sPrintf("Completion packet unexpected\n");
			return B_NO_ERROR;
		}
		case 0x60:
			if(data[3] == 0xff) {
				if(debug_level >= 1)
					_sPrintf("Got Error Packet, error code 0x%02x\n", data[2]);
				write_port_etc(response_port, B_ERROR, NULL, 0,
				               B_RELATIVE_TIMEOUT, 0);
				return B_NO_ERROR;
			}
			break;
	}
	if(debug_level >= 1)
		dump_data("got bad response packet: ", data, data_len);
	return B_NO_ERROR;
}

status_t
NetronDisplayControl::ReadPacket()
{
	uint32 i;
	uint8  data[64];
	
	if(pending_byte >= 0) {
		data[0] = pending_byte;
		pending_byte = -1;
	}
	else
	{
		t.c_cc[VMIN] = 1;
		t.c_cc[VTIME] = 0;
		ioctl(serial_fd, TCSETA, &t);
		
		if(read(serial_fd, &data, 1) < 1) {
			if(debug_level >= 1)
				_sPrintf("read header failed, %s\n", strerror(errno));
			return errno < 0 ? errno : B_ERROR;
		}
	}
	
	#if 0
	if(data[0] != REQUEST_PACKET_HEADER &&
	   data[0] != RESPONSE_PACKET_HEADER) {
		if(debug_level >= 1)
			_sPrintf("expecting header, got %02x\n", data[0]);
		return B_IO_ERROR;
	}
	#endif
	
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	ioctl(serial_fd, TCSETA, &t);
	
	for(i = 1; i < sizeof(data); i++) {
		if(read(serial_fd, &data[i], 1) < 1) {
			if(debug_level >= 1)
				_sPrintf("\nread packet data failed, %s\n", strerror(errno));
			return errno < 0 ? errno : B_ERROR;
		}
		if(data[i] == 0xff)
			break;
		if((data[i] & 0x80) == 0x80) {
			if(debug_level >= 1)
				if((data[0] & 0x80) == 0x80)
					dump_data("got short packet:           <-", data, i);
				else
					dump_data("got bad data:               <-", data, i);
			pending_byte = data[i];
			return B_BAD_VALUE;
		}
	}
	if(i == sizeof(data)) {
		if(debug_level >= 1) {
			_sPrintf("packet too long:");
			for(i = 0; i < sizeof(data); i++)
				_sPrintf(" %02x", data[i]);
			do {
				if(read(serial_fd, &data[0], 1) < 1) {
					_sPrintf("\nread packet data failed, %s\n", strerror(errno));
					return B_IO_ERROR;
				}
				_sPrintf(" %02x", data[0]);
				i++;
			} while((data[0] & 0x80) != 0x80);
			_sPrintf(" (size %d)\n", i);
			if(data[0] != 0xff) {
				pending_byte = data[0];
				return B_BAD_VALUE;
			}
		}
		return B_IO_ERROR;
	}
	if(data[0] != REQUEST_PACKET_HEADER &&
	   data[0] != RESPONSE_PACKET_HEADER) {
		if(debug_level >= 1)
			dump_data("got unknown packet:         <-", data, i + 1);
		return B_IO_ERROR;
	}

	if(debug_level >= 2)
		dump_data("got packet:                 <-", data, i + 1);
	
	if(data[0] == REQUEST_PACKET_HEADER)
		return ParseRequestPacket(data, i+1);
	else /* if(data[0] == RESPONSE_PACKET_HEADER) */
		return ParseResponsePacket(data, i+1);
}

void
NetronDisplayControl::ReadPackets()
{
	status_t err;
	while(1) {
		err = ReadPacket();
		if(err != B_NO_ERROR) {
			if(old_display_protocol) {
				if(debug_level >= 1)
					_sPrintf("ReadPacket failed, assume no keys are pressed\n");
				ProcessNewKeyState(0);
			}
			else {
				uint8 packet[] = { 0x81, 0x01, 0x60, 0x02, 0x01, 0xff };
				if(debug_level >= 1)
					_sPrintf("ReadPacket failed, request current key state\n");
				SendRawPacket(packet, sizeof(packet));
			}
		}
	}
}

status_t
NetronDisplayControl::GetRegister(uint8 reg, uint8 *value)
{
	uint8 packet[5];
	uint8 response[2];
	size_t response_len = 2;
	status_t err;
	packet[0] = 0x81;
	packet[1] = 0x09;
	packet[2] = 0x20;
	packet[3] = reg;
	packet[4] = 0xff;
	
	err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet),
	                 response, &response_len);
	if(err != B_NO_ERROR)
		return err;
	if(response_len != 2) {
		if(debug_level >= 1)
			_sPrintf("GetRegister: reply length wrong, expected 2 bytes, got %d\n",
			       response_len);
		return B_IO_ERROR;
	}
	*value = (response[0] << 4) | (response[1] & 0x0f);
	return B_NO_ERROR;
}

status_t
NetronDisplayControl::SetRegister(uint8 reg, uint8 value)
{
	uint8 packet[7];
	packet[0] = 0x81;
	packet[1] = 0x01;
	packet[2] = 0x20;
	packet[3] = reg;
	packet[4] = value >> 4;
	packet[5] = value & 0x0f;
	packet[6] = 0xff;

	return SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet));
}

status_t
NetronDisplayControl::GetFactorySetting(uint8 reg, uint8 *value)
{
	uint8 packet[6];
	uint8 response[2];
	size_t response_len = 2;
	status_t err;
	packet[0] = 0x81;
	packet[1] = 0x09;
	packet[2] = 0x23;
	packet[3] = 0x04;
	packet[4] = reg;
	packet[5] = 0xff;
	
	err = SendPacket(NETRON_NORMAL_TIMEOUT, packet, sizeof(packet),
	                 response, &response_len);
	if(err != B_NO_ERROR)
		return err;
	if(response_len != 2) {
		if(debug_level >= 1)
			_sPrintf("GetFactorySetting: reply length wrong, expected 2 bytes, got %d\n",
			       response_len);
		return B_IO_ERROR;
	}
	*value = (response[0] << 4) | (response[1] & 0x0f);
	return B_NO_ERROR;
}

uint32 
NetronDisplayControl::GetKeyState()
{
	return key_state;
}

void 
NetronDisplayControl::SetDebugLevel(int new_debug_level)
{
	if(new_debug_level == debug_level)
		return;
	_sPrintf("Debug level changed from %d to %d\n", debug_level, new_debug_level);
	debug_level = new_debug_level;
}

