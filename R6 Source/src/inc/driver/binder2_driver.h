
#ifndef BINDER2_DRIVER_H
#define BINDER2_DRIVER_H

#include <support2/SupportDefs.h>
#include <Drivers.h>

#define ROOT_DESCRIPTOR 0

enum transaction_flags {
	tfInline = 0x01,			// not yet implemented
	tfSynchronous = 0x02,		// obsolete
	tfRootObject = 0x04			// contents are the component's root object
};

struct binder_transaction_data
{
	// The first two are only used for bcTRANSACTION and brTRANSACTION,
	// identifying the target and contents of the transaction.
	union {
		size_t	handle;		// target descriptor of command transaction
		void	*ptr;		// target descriptor of return transaction
	} target;
	uint32	code;			// transaction command
	
	// General information about the transaction.
	uint32	flags;
	int32	priority;		// requested/current thread priority
	size_t	data_size;		// number of bytes of data
	size_t	offsets_size;	// number of bytes of object offsets
	
	// If this transaction is inline, the data immediately
	// follows here; otherwise, it ends with a pointer to
	// the data buffer.
	union {
		struct {
			const void	*buffer;	// transaction data
			const void	*offsets;	// binder object offsets
		} ptr;
		uint8	buf[8];
	} data;
};

enum BinderDriverReturnProtocol {
	brERROR = -1,
	/*
		int32: error code
	*/
	
	brOK = 0,
	brTIMEOUT,
	brWAKEUP,
	/*	No parameters! */
	
	brTRANSACTION,
	brREPLY,
	/*
		binder_transaction_data: the received command.
	*/
	
	brACQUIRE_RESULT,
	/*
		int32: 0 if the last bcATTEMPT_ACQUIRE was not successful.
		Else the remote object has acquired a primary reference.
	*/
	
	brTRANSACTION_COMPLETE,
	/*
		No parameters... always refers to the last transaction requested
		(including replies).  Note that this will be sent even for asynchronous
		transactions.
	*/
	
	brINCREFS,
	brACQUIRE,
	brRELEASE,
	brDECREFS,
	/*
		void *:	ptr to binder
	*/
	
	brATTEMPT_ACQUIRE,
	/*
		int32:	priority
		void *: ptr to binder
	*/
	
	brEVENT_OCCURRED,
	/*
		This is returned when the bcSET_NEXT_EVENT_TIME has elapsed.
		At this point the next event time is set to B_INFINITE_TIMEOUT,
		so you must send another bcSET_NEXT_EVENT_TIME command if you
		have another event pending.
	*/
	
	brFINISHED
};

enum BinderDriverCommandProtocol {
	bcNOOP = 0,
	/*	No parameters! */

	bcTRANSACTION,
	bcREPLY,
	/*
		binder_transaction_data: the sent command.
	*/
	
	bcACQUIRE_RESULT,
	/*
		int32:  0 if the last brATTEMPT_ACQUIRE was not successful.
		Else you have acquired a primary reference on the object.
	*/
	
	bcFREE_BUFFER,
	/*
		void *: ptr to transaction data received on a read
	*/
	
	bcTRANSACTION_COMPLETE,
	/*
		No parameters... send when finishing an asynchronous transaction.
	*/
	
	bcINCREFS,
	bcACQUIRE,
	bcRELEASE,
	bcDECREFS,
	/*
		int32:	descriptor
	*/
	
	bcATTEMPT_ACQUIRE,
	/*
		int32:	priority
		int32:	descriptor
	*/
	
	bcRESUME_THREAD,
	/*
		int32:	thread ID
	*/
	
	bcSET_THREAD_ENTRY,
	/*
		void *:	thread entry function for new threads created to handle tasks
		void *: argument passed to those threads
	*/
	
	bcREGISTER_LOOPER,
	/*
		No parameters.
		Register a spawned looper thread with the device.  This must be
		called by the function that is supplied in bcSET_THREAD_ENTRY as
		part of its initialization with the binder.
	*/
	
	bcENTER_LOOPER,
	bcEXIT_LOOPER,
	/*
		No parameters.
		These two commands are sent as an application-level thread
		enters and exits the binder loop, respectively.  They are
		used so the binder can have an accurate count of the number
		of looping threads it has available.
	*/
	
	bcSYNC
	/*
		No parameters.
		Upon receiving this command, the driver waits until all
		pending asynchronous transactions have completed.
	*/
};

enum {
	BINDER_SET_WAKEUP_TIME = B_DEVICE_OP_CODES_END+1,
	BINDER_SET_ROOT,
	BINDER_WAIT_FOR_ROOT,
	BINDER_SET_IDLE_TIMEOUT,
	BINDER_SET_REPLY_TIMEOUT,
	BINDER_GET_API_VERSION,
};

#if BINDER_DEBUG_LIB

/*	Below are calls to access the binder when debugging the driver from
	user space by compiling it as libbinderdbg and linking libbe2 with it. */

extern int			open_binder(int teamID=0);
extern status_t		close_binder(int desc);
extern status_t		ioctl_binder(int desc, int cmd, void *data, int32 len);
extern ssize_t		read_binder(int desc, void *data, size_t numBytes);
extern ssize_t		write_binder(int desc, void *data, size_t numBytes);

#else

#include <unistd.h>
inline int			open_binder(int ) { return open("/dev/misc/binder",O_RDWR); };
inline status_t		close_binder(int desc) { return close(desc); };
inline status_t		ioctl_binder(int desc, int cmd, void *data, int32 len) { return ioctl(desc,cmd,data,len); };
inline ssize_t		read_binder(int desc, void *data, size_t numBytes) { return read(desc,data,numBytes); };
inline ssize_t		write_binder(int desc, void *data, size_t numBytes) { return write(desc,data,numBytes); };

#endif

#endif
