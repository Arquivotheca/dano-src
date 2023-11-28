#ifndef MOUSE_PROTOCOL_H
#define MOUSE_PROTOCOL_H

#include <SupportDefs.h>

struct serial_mouse_state {
	uint32	buttons;
	int	dx;
	int dy;
	int dw;
};

typedef int (mouse_convert_func)(uint8 *data, int bytes, serial_mouse_state *state);
typedef bool (mouse_check_func)(uint8 *data, int bytes, bool data_is_8bit);

struct mouse_protocol {
	char *name;
	mouse_convert_func *convertf; 
	mouse_check_func *datacheckf; 
	int min_sample_size;
	int max_sample_size;
	bool eightbit;
};

mouse_protocol *detect_mouse(int fd, bool mouse_present);
mouse_protocol *find_mouse_protocol(int fd, uint8 *data, int bytes,
                                    mouse_protocol *current_protocol);

typedef bool (mouse_state_changed_func)(void *cookie, serial_mouse_state *state);
void read_mouse(int fd, mouse_protocol *p, void *cookie, mouse_state_changed_func *f);


#endif

