#include "mouse_protocol.h"

#include <OS.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <Drivers.h>

#if 0
#define ERROR(x) printf x; fflush(stdout)
#define WARNING(x) printf x; fflush(stdout)
#define INFO(x) printf x; fflush(stdout)
#else
#define ERROR(x)
#define WARNING(x)
#define INFO(x)
#endif

int mouse_systems_cf(uint8 *data, int bytes, serial_mouse_state *state);
int microsoft_cf(uint8 *data, int bytes, serial_mouse_state *state);
int logitech3_cf(uint8 *data, int bytes, serial_mouse_state *state);
int intellimouse_cf(uint8 *data, int bytes, serial_mouse_state *state);

bool mouse_systems_data_check(uint8 *data, int bytes, bool data_is_8bit);
bool microsoft_data_check(uint8 *data, int bytes, bool data_is_8bit);
bool logitech3_data_check(uint8 *data, int bytes, bool data_is_8bit);
bool intellimouse_data_check(uint8 *data, int bytes, bool data_is_8bit);

typedef enum {
	MOUSE_SYSTEMS = 0,
	MICROSOFT,
	LOGITECH,
	INTELLIMOUSE,
} mouse_protocol_index;

mouse_protocol protocols[] = {
	{ "Mouse Systems",
	  mouse_systems_cf, mouse_systems_data_check, 5, 5, true },
	{ "MicroSoft Mouse",
	  microsoft_cf,     microsoft_data_check,     3, 3, false },
	{ "Logitech extended MicroSoft Mouse",
	  logitech3_cf,     logitech3_data_check,     3, 4, false },
	{ "IntelliMouse",
	  intellimouse_cf,  intellimouse_data_check,  4, 4, false },
	{ NULL, NULL, NULL, 0, 0, false }
};

bool
mouse_systems_data_check(uint8 *data, int bytes, bool data_is_8bit)
{
	int i = 0;
	int j = 0;
	int start_i = 0;
	uint8 sync_mask = data_is_8bit ? 0xf8 : 0x78;
	uint8 sync_val = 0x80 & sync_mask;
retry:
	i = start_i;
	while(i < bytes && (data[i] & sync_mask) != sync_val)
		i++;
	if(i > 4)
		return false;
		
	while(i < bytes) {
		if(j == 0 && (data[i] & sync_mask) != sync_val) {
			if(start_i < 4) {
				start_i++;
				goto retry;
			}
			return false;
		}
		j = (j + 1) % 5;
		i++;
	}
	return true;
}

int
mouse_systems_cf(uint8 *data, int bytes, serial_mouse_state *state)
{
	if((data[0] & 0xf8) != 0x80) {
		WARNING(("skip bad data, 0x%02x\n", data[0]));
		return 1;
	}
	state->buttons = !(data[0]&0x04) | !(data[0]&0x02)<<2 | !(data[0]&0x01)<<1;
	state->dx = (int8)data[1]+(int8)data[3];
	state->dy = (int8)data[2]+(int8)data[4];
	state->dw = 0;
	return 5;
}

bool
microsoft_data_check(uint8 *data, int bytes, bool data_is_8bit)
{
	int i = 0;
	int j = 0;
	while(i < bytes && (data[i] & 0x40) != 0x40)
		i++;
	if(i > 2)
		return false;
		
	while(i < bytes) {
		if((data[i] & 0x40) != (j == 0 ? 0x40 : 0x00)) {
			return false;
		}
		i++;
		j = (j + 1) % 3;
	}
	return true;
}

int
microsoft_cf(uint8 *data, int bytes, serial_mouse_state *state)
{
	if((data[0] & 0x40) != 0x40) {
		WARNING(("skip bad data, 0x%02x\n", data[0]));
		return 1;
	}
	if(data[1] & 0x40) {
		WARNING(("skip bad data, 0x%02x (0x%02x)\n", data[0], data[1]));
		return 1;
	}
	if(data[2] & 0x40) {
		WARNING(("skip bad data, 0x%02x 0x%02x (0x%02x)\n",
		         data[0], data[1], data[2]));
		return 2;
	}
	state->buttons = !!(data[0]&0x20) | !!(data[0]&0x10) << 1;
	state->dx = (int8)((data[1]&0x3f)|((data[0]&0x03)<<6));
	state->dy = -((int8)((data[2]&0x3f)|((data[0]&0x0c)<<4)));
	state->dw = 0;
	return 3;
}

bool
logitech3_data_check(uint8 *data, int bytes, bool data_is_8bit)
{
	int i = 0;
	int j = 0;
	while(i < bytes && (data[i] & 0x40) != 0x40)
		i++;
	if(i > 3)
		return false;
		
	while(i < bytes) {
		if(j == 0) {
			if((data[i] & 0x40) != 0x40) {
				return false;
			}
		}
		else if(j < 3) {
			if((data[i] & 0x40) == 0x40) {
				return false;
			}
		}
		else /* j == 3 */ {
			if((data[i] & 0x40) == 0x40) {
				j = 0;
			}
			else if(data[i] & ~0x23) {
				return false;
			}
		}
		j = (j + 1) % 4;
		i++;
	}
	return true;
}

int
logitech3_cf(uint8 *data, int bytes, serial_mouse_state *state)
{
	if((data[0] & 0x40) != 0x40) {
		WARNING(("skip bad data, 0x%02x\n", data[0]));
		return 1;
	}
	if(data[1] & 0x40) {
		WARNING(("skip bad data, 0x%02x (0x%02x)\n", data[0], data[1]));
		return 1;
	}
	if(data[2] & 0x40) {
		WARNING(("skip bad data, 0x%02x 0x%02x (0x%02x)\n",
		         data[0], data[1], data[2]));
		return 2;
	}
	if(bytes == 4 && data[3] & 0x40) {
		bytes = 3;
	}
	if(bytes == 4 && data[3] & ~0x23) {
		WARNING(("skip bad data, 0x%02x (0x%02x 0x%02x 0x%02x)\n",
		         data[0], data[1], data[2], data[3]));
		return 1;
	}
	state->buttons = !!(data[0]&0x20) | !!(data[0]&0x10) << 1;
	if(bytes > 3)
		state->buttons |= !!(data[3]&0x20) << 2;
	state->dx = (int8)((data[1]&0x3f)|((data[0]&0x03)<<6));
	state->dy = -((int8)((data[2]&0x3f)|((data[0]&0x0c)<<4)));
	state->dw = 0;
	return bytes;
}

bool
intellimouse_data_check(uint8 *data, int bytes, bool data_is_8bit)
{
	int i = 0;
	int j = 0;
	while(i < bytes && (data[i] & 0x40) != 0x40)
		i++;
	if(i > 3)
		return false;
		
	while(i < bytes) {
		if((data[i] & 0x40) != (j == 0 ? 0x40 : 0x00)) {
			return false;
		}
		j = (j + 1) % 4;
		i++;
	}
	return true;
}

int
intellimouse_cf(uint8 *data, int bytes, serial_mouse_state *state)
{
	if((data[0] & 0x40) != 0x40) {
		WARNING(("skip bad data, 0x%02x\n", data[0]));
		return 1;
	}
	if(data[1] & 0x40) {
		WARNING(("skip bad data, 0x%02x (0x%02x)\n", data[0], data[1]));
		return 1;
	}
	if(data[2] & 0x40) {
		WARNING(("skip bad data, 0x%02x 0x%02x (0x%02x)\n",
		         data[0], data[1], data[2]));
		return 2;
	}
	if(data[3] & 0x40) {
		WARNING(("skip bad data, 0x%02x 0x%02x 0x%02x (0x%02x)\n",
		         data[0], data[1], data[2], data[3]));
		return 3;
	}
	state->buttons = !!(data[0]&0x20) | !!(data[0]&0x10) << 1 |
	                 !!(data[3]&0x10) << 2 | !!(data[3]&0x20) << 3;
	state->dx = (int8)((data[1]&0x3f)|((data[0]&0x03)<<6));
	state->dy = -((int8)((data[2]&0x3f)|((data[0]&0x0c)<<4)));
	state->dw = (int8)((data[3]&0xf)<<4)>>4;
	return 4;
}

mouse_protocol *
detect_mouse(int fd, bool mouse_present)
{
	int retry_count = 3;
	bool			assert;
	struct termios t;
	bool	switch_to_9600 = false;
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
	ioctl(fd, TCGETA, &t);
	t.c_cflag &= ~ (CBAUD | CSIZE);
	t.c_cflag |= B1200 | CS7 | CLOCAL;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	ioctl(fd, TCSETA, &t);
	/* now de-assert rts for 50 msec and re-assert
	 */
	snooze( 10000);
	assert = FALSE;
	ioctl(fd, TCSETRTS, &assert);
	snooze(120000);
	tcflush(fd, TCIFLUSH);
	{
		uint8	buffer[256];
		int bytesread;
		bytesread = read(fd, buffer, sizeof(buffer));
		if(bytesread > 0)
			WARNING(("got %d bytes after flush\n", bytesread));
	}
	assert = TRUE;
	ioctl(fd, TCSETRTS, &assert);

	mouse_protocol *p = NULL;
	{
		uint8	buffer[128];
		int bytesread;
retry:
		t.c_cc[VMIN] = 0;
		t.c_cc[VTIME] = 3;
		ioctl(fd, TCSETA, &t);
		int numevents;
		if(ioctl(fd, TCWAITEVENT, &numevents) < 0) {
			ERROR(("TCWAITEVENT failed, %s\n", strerror(errno)));
			return NULL;
		}
		if(numevents == 0) {
			bytesread = 0;
		}
		else {
			t.c_cc[VMIN] = 255;
			t.c_cc[VTIME] = 1;
			ioctl(fd, TCSETA, &t);
			bytesread = read(fd, buffer, sizeof(buffer));
		}
#if 0
		// dump init string
		INFO(("got %d bytes: ", bytesread));
		for(int i = 0; i < bytesread; i++) {
			INFO(("0x%02x ", buffer[i]));
		}
		INFO(("\n"));
		for(int i = 0; i < bytesread; i++) {
			if(isprint(buffer[i])) {
				INFO(("%c", buffer[i]));
			}
			else {
				INFO(("."));
			}
		}
		INFO(("\n"));
#endif
		if(bytesread >= 2 && buffer[0] == 'M' && buffer[1] == 'Z') {
			p = &protocols[INTELLIMOUSE];
		}
		else if(bytesread >= 2 && buffer[0] == 'M' && buffer[1] == '3') {
			p = &protocols[LOGITECH];
		}
		else if(bytesread >= 1 && buffer[0] == 'M') {
			p = &protocols[MICROSOFT];

			// Look for modems
			for (int32 i=0;i<bytesread-5;i++) {
				if (!strncmp((char *)buffer + i, "MODEM", 5)) {
					p = NULL;
					break;
				}
			}
		}
		else if((bytesread >= 1 && buffer[0] == 'H') ||
		        (bytesread >= 2 && buffer[1] == 'H')) {
			p = &protocols[MOUSE_SYSTEMS];
		}
		else if(mouse_present) {
			p = &protocols[MOUSE_SYSTEMS];
		}
		else {
			p = NULL;

			// Look for modems
			for (int32 i=0;i<bytesread-5;i++) {
				if (!strncmp((char *)buffer + i, "MODEM", 5)) {
					bytesread = 0;
					break;
				}
			}

			if(bytesread > 0) {
				int startb = 0;
				while(p == 0 && startb < bytesread) {
					p = find_mouse_protocol(fd, buffer+startb,
					                        bytesread-startb, NULL);
					startb++;
				}
			}
			if(p == NULL) {
				WARNING(("no mouse found\n"));
				if(bytesread > 0 && retry_count-- > 0) {
					WARNING(("retry detection\n"));
					goto retry;
				}
			}
		}
	}
	if(p == NULL)
		return NULL;
	if(p->eightbit) {
		t.c_cflag &= ~ (CBAUD | CSIZE);
		t.c_cflag |= B1200 | CS8 | CSTOPB | CLOCAL;
	}
	//switch_to_9600 = true;
	if(switch_to_9600) {
		write(fd, "*q", 2);

		t.c_cflag &= ~CBAUD;
		t.c_cflag |= B9600;
	}
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 0;
	ioctl(fd, TCSETAW, &t);
	tcflush(fd, TCIFLUSH);
	{
		uint8	buffer[256];
		int bytesread;
		bytesread = read(fd, buffer, sizeof(buffer));
		if(bytesread > 0)
			WARNING(("got %d bytes after flush\n", bytesread));
	}
	t.c_cc[VMIN] = p->max_sample_size;
	t.c_cc[VTIME] = 1;
	ioctl(fd, TCSETA, &t);
	bigtime_t us_vtime = 9000;
	ioctl(fd, TCVTIME, &us_vtime);
	return p;
}

mouse_protocol *
find_mouse_protocol(int fd, uint8 *data, int bytes, mouse_protocol *preferred_protocol)
{
	int i=0;
	int protocolnum = -1;
	int found = 0;
	bool is8bit = false;
	if(preferred_protocol) {
		is8bit = preferred_protocol->eightbit;
	}
	while(protocols[i].datacheckf) {
		if(protocols[i].datacheckf(data, bytes, is8bit)) {
			found++;
			if(protocolnum < 0)
				protocolnum = i;
			INFO(("Data (%d bytes) could be from %s\n",
			      bytes, protocols[i].name));
			if(preferred_protocol == &protocols[i]) {
				return preferred_protocol;
			}
		}
		i++;
	}
	if(found >= 1) {
		if(protocols[protocolnum].eightbit != is8bit) {
			INFO(("switch from %dbit to %dbit data\n", is8bit ? 8 : 7,
			      protocols[protocolnum].eightbit ? 8 : 7));
		}
		struct termios t;
		ioctl(fd, TCGETA, &t);
		t.c_cflag &= ~CSIZE;
		if(protocols[protocolnum].eightbit)
			t.c_cflag |= CS8 | CSTOPB | CLOCAL;
		else
			t.c_cflag |= CS7 | CLOCAL;
		t.c_cc[VMIN] = protocols[protocolnum].max_sample_size;
		t.c_cc[VTIME] = 1;
		ioctl(fd, TCSETA, &t);
		bigtime_t us_vtime = 9000;
		ioctl(fd, TCVTIME, &us_vtime);
		return &protocols[protocolnum];
	}
	else
		return NULL;
}

void 
read_mouse(int fd, mouse_protocol *p, void *cookie,
           mouse_state_changed_func *state_changed)
{
	serial_mouse_state state;
#if 0
	bigtime_t t1 = system_time();
	bigtime_t t2;
#endif

	uint8 buffer[32];
	int buffer_size = 0;
	int buffer_used;
reset:
	buffer_used = buffer_size;

	INFO(("Using %s\n", p->name));

	int min_sample_size = p->min_sample_size;
	int max_sample_size = p->max_sample_size;
	mouse_convert_func *cf = p->convertf;
	int bad_reads = 1-p->max_sample_size;
	int good_reads = 0;

	while(1) {
		int bytesread;
		int readsize = max_sample_size-(buffer_size-buffer_used);
		if(buffer_size + readsize > (int)sizeof(buffer)) {
			int shift_size = buffer_size + readsize - sizeof(buffer);
			buffer_used -= shift_size;
			buffer_size -= shift_size;
			for(int i=0; i < buffer_size; i++) {
				buffer[i] = buffer[i+shift_size];
			}
		}
		bytesread = read(fd, buffer+buffer_size, readsize);
		if(bytesread < 0) {
			ERROR(("could not read from serial port, %s\n", strerror(errno)));
			return;
		}
		buffer_size += bytesread;
#if 0
		t2 = system_time();
		INFO(("got %d of %d bytes (dt %Ld): ", bytesread, readsize, t2-t1));
		for(int i = buffer_size-bytesread; i < buffer_size; i++) {
			INFO(("0x%02x ", buffer[i]));
		}
		INFO(("\n"));
		t1 = t2;
#endif
		if(buffer_size-buffer_used < min_sample_size) {
			continue;
		}
		else {
			int used = cf(buffer+buffer_used, buffer_size-buffer_used, &state);
			buffer_used += used;
			if(used < min_sample_size) {
				bad_reads++;
				good_reads = 0;
				if(bad_reads > 0) {
					mouse_protocol *np;
					np = find_mouse_protocol(fd, buffer, buffer_size, p);
					if(np != NULL) {
						p = np;
						goto reset;
					}
					if(bad_reads > 32) {
						struct termios t;
						ioctl(fd, TCGETA, &t);
						t.c_cflag &= ~CSIZE;
						t.c_cflag |= CS7 | CLOCAL;
						t.c_cc[VMIN] = 32;
						t.c_cc[VTIME] = 1;
						ioctl(fd, TCSETA, &t);
						bytesread = read(fd, buffer, sizeof(buffer));
						if(bytesread < 0) {
							ERROR(("could not read from serial port, %s\n", strerror(errno)));
							return;
						}
						np = find_mouse_protocol(fd, buffer, bytesread, NULL);
						if(np != NULL) {
							INFO(("mouse changes to 7 bit protocol\n"));
							p = np;
							goto reset;
						}
						WARNING(("mouse is producing garbage\n"));
					}
				}
			}
			else {
				good_reads++;
				if(good_reads > 8) {
					good_reads = 8;
					bad_reads = 0;
				}
				if(bad_reads <= 0) {
					if(!state_changed(cookie, &state))
						return;
				}
			}
		}
	}
}
