t_module(isa_name, (module_info **)&isa);
}

static status_t
uninit()
{
	ddprintf("midi module uninit()\n");
	if (isa) put_module(isa_name);
	return B_OK;
}

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	case B_MODULE_INIT:
		return init();
	case B_MODULE_UNINIT:
		return uninit();
	default:
		return -1;
	}
}


static generic_mpu401_module g_mpu401[2] = {
	{
		{
			B_MPU_401_MODULE_NAME_V1,
			0,
			std_ops
		},
		create_device,
		delete_device,
		midi_open,
		midi_close,
		midi_free,
		midi_control,
		midi_read,
		midi_write,
		midi_interrupt
	},
	{
		{
			B_MPU_401_MODULE_NAME,
			0,
			std_ops
		},
		create_device,
		delete_device,
		midi_open,
		midi_close,
		midi_free,
		midi_control,
		midi_read,
		midi_write,
		midi_interrupt,
		create_device_alt
	},
};

_EXPORT generic_mpu401_module *modules[] = {
		&g_mpu401[0],
		&g_mpu401[1],
        NULL
};

                                                                                                                                                    /*	Generic MPU-401 MIDI driver module	*/
/*	For use by sound card drivers that publish a midi device.	*/

#include <Drivers.h>
#include <ISA.h>
#include <KernelExport.h>
#include <string.h>
#include <stdlib.h>

#include "midi_driver.h"

const char isa_name[] = B_ISA_MODULE_NAME;
static isa_module_info	*isa;

#define PCI_IO_WR(p,v) (*isa->write_io_8)(p,v)
#define PCI_IO_RD(p) (*isa->read_io_8)(p)
#if defined(__powerc) && defined(__MWERKS__)
#define EIEIO() __eieio()
#else
#define EIEIO()
#endif

#if DEBUG
#define KTRACE() kprintf("%s:%d\n", __FILE__, __LINE__)
#define ddprintf dprintf
#define KPRINTF kprintf
#else
#define KTRACE()
#define ddprintf
#define KPRINTF
#endif

typedef struct {
	long long	timeout;
} midi_cfg;

/* buffer size must be power of 2 */
/* we allocate MIDI_BUF_SIZE*5 bytes of memory, so be gentle */
/* 256 bytes is about 90 ms of MIDI data; if we can't serve MIDI in 90 ms */
/* we have bigger problems than overrun dropping bytes... */
#define MIDI_BUF_SIZE 256

typedef struct _midi_wr_q {
	bigtime_t	when;
	struct _midi_wr_q	*next;
	size_t		size;
	unsigned char	data[4];	/* plus more, possibly */
} midi_wr_q;

typedef struct {
	uint32		cookie;
	midi_wr_q	*write_queue;
	uchar		*in_buffer;
	bigtime_t	*in_timing;
	midi_parser_module_info
			*parser;
	bigtime_t	in_last_time;
	uint32		parser_state;
	int32		in_back_count;	/* cyclic buffer */
	int32		in_front_count;
	sem_id		init_sem;
	int32		open_count;
	int32		port_lock;
	int32		read_excl_cnt;
	sem_id		read_excl_sem;
	int			port;
	int32		read_cnt;
	sem_id		read_sem;
	int32		write_cnt;
	sem_id		write_sem;
	int32		write_sync_cnt;
	sem_id		write_sync_sem;
	thread_id	write_thread;
	sem_id		write_sleep;
	midi_cfg	config;
	void		(*interrupt_op)(int32 op, void * card);
	void *		interrupt_card;
	sem_id		write_quit_sem;
	uint32		workarounds;
} mpu401_dev;



#define MIDI_ACTIVE_SENSE 0xfe
#define SNOOZE_GRANULARITY 500

static status_t midi_open(void * storage, uint32 flags, void **cookie);
static status_t midi_close(void *cookie);
static status_t midi_free(void *cookie);
static status_t midi_control(void *cookie, uint32 op, void *data, size_t len);
static status_t midi_read(void *cookie, off_t pos, void *data, size_t *len);
static status_t midi_write(void *cookie, off_t pos, const void *data, size_t *len);
static status_t midi_timed_read(mpu401_dev * port, midi_timed_data * data);
static status_t midi_timed_write(mpu401_dev * port, midi_timed_data * data);
static status_t midi_write_thread(void * arg);
static status_t midi_write_sync(mpu401_dev * port);
static status_t midi_write_clear(mpu401_dev * port);


static midi_cfg default_midi = {
	B_MIDI_DEFAULT_TIMEOUT
};


static status_t 
configure_midi(
	mpu401_dev * port,
	midi_cfg * config,
	bool force)
{
	status_t err = B_OK;

	ddprintf("mpu401: configure_midi()\n");

	/* configure device */

	if (force || (config->timeout != port->config.timeout)) {
		port->config.timeout = config->timeout;
	}

	return err;
}


static status_t
midi_open(
	void * storage,
	uint32 flags,
	void ** cookie)
{
	int ix;
	mpu401_dev * port = NULL;
	char name2[64];
	thread_id the_thread = 0;

	ddprintf("mpu401: midi_open()\n");

	*cookie = port = (mpu401_dev *)storage;
	ddprintf("mpu401: device = %x\n", port);

	if (acquire_sem(port->init_sem) < B_OK)
		return B_ERROR;
	if (atomic_add(&port->open_count, 1) == 0) {

		/* initialize device first time */
		ddprintf("mpu401: initialize device\n");

		if (get_module(B_MIDI_PARSER_MODULE_NAME, (module_info **)&port->parser) < 0) {
			dprintf("mpu401(): midi_open() can't get_module(%s)\n", B_MIDI_PARSER_MODULE_NAME);
			goto bad_mojo;
		}
		port->parser_state = 0;
		port->in_buffer = (unsigned char *)malloc(MIDI_BUF_SIZE);
		port->in_timing = (bigtime_t *)malloc(MIDI_BUF_SIZE*sizeof(bigtime_t));
		if (!port->in_timing || !port->in_buffer) {
			goto bad_mojo0;
		}
		ddprintf("MIDI port port is %x\n", port->port);
		port->port_lock = 0;
		port->in_back_count = 0;
		port->in_front_count = 0;
		port->read_cnt = 0;
		port->read_sem = create_sem(0, "mpu401 read");
		if (port->read_sem < 0) {
			goto bad_mojo1;
	/* we really should support condition variables... */
bad_mojo7:
			delete_sem(port->write_quit_sem);
bad_mojo6:
			delete_sem(port->write_sync_sem);
bad_mojo5:
			delete_sem(port->write_sleep);
bad_mojo4:
			delete_sem(port->read_excl_sem);
bad_mojo3:
			delete_sem(port->write_sem);
bad_mojo2:
			delete_sem(port->read_sem);
bad_mojo1:
			free(port->in_buffer);
			free(port->in_timing);
bad_mojo0:
			put_module(B_MIDI_PARSER_MODULE_NAME);
bad_mojo:
			port->parser = NULL;
			port->parser_state = 0;
			port->in_buffer = NULL;
			port->in_timing = NULL;
			port->open_count = 0;
			port->read_sem = -1;
			port->write_sem = -1;
			port->read_excl_sem = -1;
			port->write_sleep = -1;
			port->write_sync_sem = -1;
			return ENODEV;
		}
		set_sem_owner(port->read_sem, B_SYSTEM_TEAM);
		port->write_cnt = 1;
		port->write_sem = create_sem(1, "mpu401 write");
		if (port->write_sem < 0) {
			goto bad_mojo2;
		}
		set_sem_owner(port->write_sem, B_SYSTEM_TEAM);
		port->read_excl_cnt = 1;
		port->read_excl_sem = create_sem(0, "mpu401 read excl");
		if (port->read_excl_sem < 0) {
			goto bad_mojo3;
		}
		set_sem_owner(port->read_excl_sem, B_SYSTEM_TEAM);
		port->write_sleep = create_sem(0, "mpu401 write sleep");
		if (port->write_sleep < 0) {
			goto bad_mojo4;
		}
		set_sem_owner(port->write_sleep, B_SYSTEM_TEAM);
		port->write_sync_cnt = 0;
		port->write_sync_sem = create_sem(0, "mpu401 write sync");
		if (port->write_sync_sem < 0) {
			goto bad_mojo5;
		}
		set_sem_owner(port->write_sync_sem, B_SYSTEM_TEAM);
		port->write_quit_sem = create_sem(0, "mpu401 write quit");
		if (port->write_quit_sem < 0) {
			goto bad_mojo6;
		}
		set_sem_owner(port->write_quit_sem, B_SYSTEM_TEAM);
		sprintf(name2, "%.23s writer", "mpu401");
		ddprintf("spawning %s\n", name2);
		port->write_thread = spawn_kernel_thread(midi_write_thread, name2, 100, port);
		if (port->write_thread < 1) {
			goto bad_mojo7;
		}
		configure_midi((mpu401_dev *)*cookie, &default_midi, true);
/*		set_thread_owner(port->write_thread, B_SYSTEM_TEAM); /* */
		/* reset UART */
		ddprintf("mpu401: reset UART\n");
		PCI_IO_WR(port->port+1, 0x3f);
		EIEIO();
		spin(1);
		/* read back status */
		port->in_buffer[0] = PCI_IO_RD(port->port);
		ddprintf("port cmd ack is %x\n", (unsigned char)port->in_buffer[0]);
		(*port->interrupt_op)(B_MPU_401_ENABLE_CARD_INT, port->interrupt_card);
		ddprintf("after interrupt_op\n");
		the_thread = port->write_thread;
	}
	release_sem(port->init_sem);

	if (the_thread > 0) {
		ddprintf("mpu401: resuming thread\n");
		resume_thread(the_thread);
	}
	dprintf("mpu401: midi_open() done (count = %d)\n", port->open_count);

	return B_OK;
}


static status_t
midi_close(
	void * cookie)
{
	mpu401_dev * port = (mpu401_dev *)cookie;
	sem_id to_del = -1;
	cpu_status cp;

	ddprintf("mpu401: midi_close()\n");

	acquire_sem(port->init_sem);

	if (atomic_add(&port->open_count, -1) == 1) {

		dprintf("mpu401: %Lx: last close detected\n", system_time());

		KTRACE();

		/* un-wedge readers and writers */
		delete_sem(port->write_sem);
		port->write_sem = -1;
		delete_sem(port->read_sem);
		port->read_sem = -1;
		delete_sem(port->read_excl_sem);
		port->read_excl_sem = -1;
		delete_sem(port->write_sleep);
		port->write_sleep = -1;
		delete_sem(port->write_sync_sem);
		port->write_sync_sem = -1;
	}

	release_sem(port->init_sem);

	return B_OK;
}


static status_t
midi_free(
	void * cookie)
{
	midi_wr_q * ent;
	mpu401_dev * port = (mpu401_dev *)cookie;
	cpu_status cp;

	ddprintf("mpu401: midi_free()\n");

	acquire_sem(port->init_sem);

	if (port->open_count == 0) {
		if (port->write_thread >= 0) {
			KTRACE();
			dprintf("mpu401: %x: midiport open_count %d in midi_free()\n", system_time(), port->open_count);
			cp = disable_interrupts();
			acquire_spinlock(&port->port_lock);
	
			/* turn off UART mode */
			PCI_IO_WR(port->port+1, 0xff); /* reset */
	
			release_spinlock(&port->port_lock);
			restore_interrupts(cp);

	/*		send_signal(SIGHUP, port->write_thread); /* */
	/*		wait_for_thread(port->write_thread, &status);	/* deadlock with psycho_killer? */ /* */
			acquire_sem(port->write_quit_sem);		/* make sure writer thread is gone */
			delete_sem(port->write_quit_sem);
			snooze(50000);					/* give thread time to die */
			port->write_thread = -1;
			/* delete all queued entries that we didn't write */
			for (ent = port->write_queue; ent!=NULL;) {
				midi_wr_q * del = ent;
				ent = ent->next;
				free(del);
			}
			port->write_queue = NULL;
			free(port->in_buffer);
			port->in_buffer = NULL;
			free(port->in_timing);
			port->in_timing = NULL;
			put_module(B_MIDI_PARSER_MODULE_NAME);

			ddprintf("mpu401: turning off card interrupts\n");
			(*port->interrupt_op)(B_MPU_401_DISABLE_CARD_INT, port->interrupt_card);
			ddprintf("after interrupt_op\n");
		}
	}

	release_sem(port->init_sem);

	return B_OK;
}


static status_t
midi_control(
	void * cookie,
	uint32 iop,
	void * data,
	size_t len)