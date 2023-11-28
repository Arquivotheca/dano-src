#include <Drivers.h>

typedef struct {
	uint32 index;
	uint32 bytes;
	uint32 data;
} emu_reg_spec;

typedef struct {
	area_id area;
	uint32 size;
	void* addr;
	void* phys_addr;
	int32 map_entries;
	void** phys_map;
} emu_area_spec;

typedef struct {
	uint32 global_data;
	uint32 low_channel_data;
	uint32 high_channel_data;
	int32 midi_bytes;
	uint8 midi_data[32];
	bigtime_t timestamp;
} emu_int_data;

enum {
	EMU_READ_SE_REG = 'EmuA',
	EMU_WRITE_SE_REG,
	EMU_READ_SE_PTR,
	EMU_WRITE_SE_PTR,
	EMU_READ_AC97,
	EMU_WRITE_AC97,
	EMU_READ_MODEM_REG,
	EMU_WRITE_MODEM_REG,
	EMU_READ_JOY_REG,
	EMU_WRITE_JOY_REG,
	EMU_READ_JOYSTICK,
	EMU_ALLOC_AREA,
	EMU_ALLOC_CONTIGUOUS_AREA,
	EMU_LOCK_RANGE,
	EMU_UNLOCK_RANGE,
	EMU_GET_SUBSYSTEM_ID,
	EMU_GET_INTERRUPT_SEM,
	EMU_SET_INTERRUPT_SEM,
	EMU_GET_INTERRUPT_DATA,
	EMU_SET_CALLBACK_THREAD,
	EMU_GET_CALLBACK_THREAD
};
