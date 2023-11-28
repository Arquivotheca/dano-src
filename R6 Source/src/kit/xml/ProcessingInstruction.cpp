;
		}		
	}
	
	release_spinlock(&port->lock);
	return bytes ? true : false;
}

static status_t
create_device(
	int _port,
	void **out_storage,
	uint32 workarounds, 
	void (*card_interrupt_op)(int32 op, void *user_card),
	void *user_card)
{
	mpu401_dev *port;
	char name[32];
	uchar byte;
	
	ddprintf("mpu401: create_device()\n");
	
	if(!(port = (mpu401_dev *) malloc(sizeof(mpu401_dev)))) return ENOMEM;
	
	*out_storage = NULL;
	
	sprintf(name, "mpu401:%04x:read_sem", _port & 0xffff);	
	if((port->read_sem = create_sem(0,name)) < 0){		
		status_t err = port->read_sem;
		free(port);
		return err;
	}
	
	sprintf(name, "mpu401:%04x:write_sem", _port & 0xffff);
	if((port->write_sem = create_sem(1,name)) < 0){		
		status_t err = port->write_sem;
		delete_sem(port->read_sem);
		free(port);
		return err;
	}

	*out_storage = port;
	port->queue_head = 0;
	port->queue_tail = 0;
	port->queue_count = QUEUE_SIZE;
	
	port->lock = 0;
	port->read_io_8 = isa->read_io_8;
	port->write_io_8 = isa->write_io_8;
	port->timeout = B_INFINITE_TIMEOUT;
	port->status_port = _port + 1;
	port->command_port = _port + 1;
	port->data_port = _port;
	port->interrupt_op = card_interrupt_op;
	port->interrupt_card = user_card;
	port->open_count = 0;
	port->r_cookie = 0;
	port->read_func = 0;
	port->write_func = 0;

	ddprintf("mpu401: create_device() done\n");

	return B_OK;
}

static status_t
create_device_alt(
	int _port,
	void * r_cookie,
	uchar (*read_func)(void *, int),
	void (*write_func)(void *, int, uchar),
	void **out_storage,
	uint32 workarounds, 
	void (*card_interrupt_op)(int32 op, void *user_card),
	void *user_card)
{
	mpu401_dev *port;
	char name[32];
	uchar byte;
	
	ddprintf("mpu401: create_device_alt()\n");

	if(!(port = (mpu401_dev *) malloc(sizeof(mpu401_dev)))) return ENOMEM;
	
	*out_storage = NULL;
	
	sprintf(name, "mpu401a:%04x:read_sem", _port & 0xffff);
	if((port->read_sem = create_sem(0,name)) < 0){		
		status_t err = port->read_sem;
		free(port);
		return err;
	}
	
	sprintf(name, "mpu401a:%04x:write_sem", _port & 0xffff);
	if((port->write_sem = creat