
#ifndef I2O_BUS_MANAGER_P
#define I2O_BUS_MANAGER_P

#include <i2o.h>

#define MAX_TO_IOP_MESSAGES   64          // Max Inbound Status Packets      
#define MAX_FROM_IOP_MESSAGES 64          // Max Outbound Message Packets    
#define WAIT_FOR_IOP_RESET    5000000     // IOP Reset timeout value
#define MAX_GET_STATUS_RETRY  20000000    // Number Of Retries for Get Status
#define WAIT_FOR_GET_STATUS   5000

#define EMPTY_QUEUE           0xffffffff  // return value for an enpty queue 

#define TO_IOP_FIFO_OFFSET    0x40        // Byte offset for to IOP fifo
#define FROM_IOP_FIFO_OFFSET  0x44        // Byte offset for from IOP fifo
#define INT_STATUS_OFFSET     0x30        // Byte offset for int status reg 
#define INT_MASK_OFFSET       0x34        // Byte offset for int mask reg 

typedef struct _i2o_context
{
	struct _i2o_context *next; /* chain pointer for free list / active list */
	
	sem_id done;               /* completion/notification semaphore */
	void *mframe;              /* pointer to outbound/inbound message frames */
	uint32 offset;             /* offset of outbound/inbound message frames */
	void *storage;             /* additional storage for this transaction */
	uint32 storage_phys;       /* physical address of additional storage */
} i2o_context;

struct i2o_info
{
	uchar bus, device, function;	
	struct i2o_info *next;
	
	uint32 iomap_phys;
	uint32 vbase;
	uint32 *to_iop_fifo;
	uint32 *from_iop_fifo;
	uint32 *int_status;
	
	void *workspace;
	uint32 workspace_phys;
	uint32 workspace_size;
	
	area_id iomap_area;
	area_id workspace_area;	

	char *name;

	sem_id ctxt_count;
	sem_id ctxt_lock;
	i2o_context *busy;
	i2o_context *free;
	
	uint32 irq;
	int handling_irqs;
	
	uint32 lct_count;
	I2O_LCT_ENTRY *lct;
	
	I2O_IOP_ENTRY systab_entry;
};


void i2o_release_msg(i2o_info *ii, i2o_context *ctxt);
void *i2o_post_msg(i2o_info *ii, i2o_context *ctxt);
i2o_context *i2o_alloc_msg(i2o_info *ii, void **mframe);

#endif