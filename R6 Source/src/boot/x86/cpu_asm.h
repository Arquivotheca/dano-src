#ifndef _BT_CPU_ASM_H
#define _BT_CPU_ASM_H

uchar read_io_8(int port);
void write_io_8(int port, uchar val);
void set_stack_and_jump(char *, void (*)(void));
bigtime_t get_time_base(void);

#endif
