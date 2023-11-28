	//kprintf("Couldn't allocate skb: %d\n", actsize);
		return (ENOBUFS);
	}
	memcpy(skb_put(sk, size), buf, size);
	if (actsize > size) {
		memset(sk->data + size, 0, actsize - size);
	}
	for (i = 0; i < TX_RETRIES; i++) {
		if (dev->hard_start_xmit(sk, dev) == 0) {
			return (size);
		}
		snooze(1000);
	}
	//kprintf("ether xmit failed\n");
	dev_kfree_skb(sk, 0);
	return (B_ERROR);
}

int
linux_ether_read(void *cookie, char *buf, size_t size, int nonblocking)
{
	status_t res;
	sem_id sem;
	struct sk_buff *sk;
	struct device *dev = (struct device *)cookie;

	ALOCK_ACQUIRE(dev);
	if (nonblocking && dev->sklist == NULL) {
		ALOCK_RELEASE(dev);
		return (0);
	}
	ALOCK_RELEASE(dev);

	do {
		res = acquire_sem(dev->sksem);
		if (res < B_NO_ERROR) {
			//kprintf("acquire sem failed\n");
			return (res);
		}

		ALOCK_ACQUIRE(dev);
		sk = dev->sklist;
		if (sk) {
			dev->sklist = dev->sklist->next;
		}
		ALOCK_RELEASE(dev);

		if (sk == NULL && nonblocking) {
			//kprintf("This shouldn't happen\n");
			return (B_ERROR);
		}
	} while (sk == NULL);
	size = min(size, sk->len);
	memcpy(buf, sk->data, size);
	dev_kfree_skb(sk, 0);
	return (size);
}


/* XXXdbg */
//extern
__inline__ int clear_bit(int nr, int * addr)
{
    int mask, retval;

    addr += nr >> 5;
    mask = 1 << (nr & 0x1f);
    retval = atomic_and((long *)addr, ~mask);
    return (retval & mask) != 0;

#if 0
    cli();
    retval = (mask & *addr) != 0;
    *addr &= ~mask;
    sti();
    return retval;
#endif
}

/* XXXdbg */


void
linux_ether_dumpstats(void *cookie)
{
	struct device *dev = (struct device *)cookie;
	struct enet_statistics *stats;

	stats = dev->get_stats(dev);
#define DOIT(x) kprintf("%s: %d\n", #x, stats->x);
	DOIT(rx_packets);
	DOIT(tx_packets);
	DOIT(rx_errors);
	DOIT(tx_errors);
	DOIT(rx_dropped);
	DOIT(tx_dropped);
	DOIT(multicast);
	DOIT(collisions);
	DOIT(rx_length_errors);
	DOIT(rx_over_errors);
	DOIT(rx_crc_errors);
	DOIT(rx_frame_errors);
	DOIT(rx_fifo_errors);
	DOIT(rx_missed_errors);
	DOIT(tx_aborted_errors);
	DOIT(tx_carrier_errors);
	DOIT(tx_fifo_errors);
	DOIT(tx_heartbeat_errors);
	DOIT(tx_window_errors);
#undef DOIT
}




/* * * * * * * * * * Phys / Virt * * * * * * * * * */
#define PVMEM_ERR 1
//#define PVMEM_ALL 1

/*
 * globals
 */
// Our favorite phys mem blocks: ph_address, size
static physical_entry 	pbtable[PHYS_ENTRIES];
// The corresponding array of vmem blocks addresses
static uchar * 			vbtable[PHYS_ENTRIES];

static uchar * phys_start_area;
static uchar * virt_start_area;


static uchar *
get_area(
			size_t size,
			area_id *areap
			)
{
	uchar * base;
	area_id id;

	size = RNDUP(size, B_PAGE_SIZE);
	id = create_area("lxw_ether",
					&base,
					B_ANY_KERNEL_ADDRESS,
					size,
					B_FULL_LOCK | B_CONTIGUOUS,
					B_READ_AREA | B_WRITE_AREA);
	if (id < B_NO_ERROR) {
		//kprintf("get_area: Can't create area, terminate\n");
		return (NULL);
	}
	if (lock_memory(base, size, B_DMA_IO | B_READ_DEVICE) != B_NO_ERROR) {
		delete_area(id);
		//kprintf("get_area: Can't lock area\n");
		return (NULL);
	}
	memset(base, NULL, size);
	*areap = id;
	return (base);
}


int
init_area(
		uchar * base
		)
{
	physical_entry 	area_phys_addr[2];		// need only the start

	// Get location & size of phys blocks into pbtable[]
	get_memory_map(base,					// vbuf to translate
					B_PAGE_SIZE -1,			// size of vbuf
					&area_phys_addr[0],		// tbl to fill out
					1);						// #of entries

	// We're relying on the fact that both virt area
	// and it's phys map are contiguous as advertised.
	// If this is untrue, use the scheme similar to rings.
	virt_start_area = base;
	phys_start_area = (uchar *) (area_phys_addr[0].address);

	return TRUE;
}
	

void *
bus_to_virt(
				ulong ph_addr
				)
{
	ulong	offset;

	offset = ph_addr - (ulong) phys_start_area;

	if ((ph_addr < (ulong) phys_start_area) || (offset > AREA_SIZE)) {
#if PVMEM_ERR
		//kprintf("b2v: Out of bounds: PA=0x%08x\n", ph_addr);
#endif
		return (virt_start_area);	// wrong addr but legal
	}

	return (virt_start_area + offset);
}


ulong
virt_to_bus(
				void * v_addr
				)
{
	ulong	offset;

	offset = (ulong) v_addr - (ulong) virt_start_area;

	if ((v_addr < virt_start_area) || (offset > AREA_SIZE)) {
#if PVMEM_ERR
		//kprintf("v2b: Out of bounds: VA=0x%08x\n", v_addr);
#endif
		return ((ulong) phys_start_area);	// wrong addr but legal
	}

	return ((ulong) phys_start_area + offset);
}



/* * * * * * * * * * Tx / Rx Rings: Phys / Virt * * * * * * * * * */

int
ring_init_addr_table(
				)
{
	int i;

	for (i = 0; i < PHYS_ENTRIES; i++) {
		pbtable[i].address = NULL;
		pbtable[i].size = NULL;
		vbtable[i] = NULL;
	}

	return TRUE;
}


/*
 * Remember PA of given VA in the table
 */
int
ring_map_n_store_addr(
				uchar * v_addr,
				int v_size
				)
{
	int i;

	// Check if it's already there?
	//for (i = 0; i < PHYS_ENTRIES; i++) {
	//	if (vbtable[i] == v_addr) {
	//		goto exit_msa;
	//	}
	//}

	for (i = 0; i < PHYS_ENTRIES; i++) {
		if (!vbtable[i]) {					// get 1st free
			vbtable[i] = v_addr;

			// Get location & size of phys blocks into pbtable[]
			get_memory_map(v_addr,			// vbuf to translate
						v_size,				// size of vbuf
						&pbtable[i],		// tbl to fill out
						1);					// #of entries

#if PVMEM_ALL
			//kprintf("Storing [%d]: VA=0x%08x, PA=0x%08x, size %d\n",
			//		i, vbtable[i], pbtable[i].address, pbtable[i].size);
#endif
			return TRUE;
		}
	}
#if PVMEM_ERR
	//kprintf("Exhausted vbtable, terminate!\n");
#endif

	return FALSE;
}


int
ring_release_addr(
				uchar * addr
				)
{
	int i;

	for (i = 0; i < PHYS_ENTRIES; i++) {
		if (vbtable[i] == addr) {		//||(pbta