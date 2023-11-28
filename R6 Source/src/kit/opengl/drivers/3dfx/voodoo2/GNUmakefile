(port, data);
}

void
_linux_outl(unsigned long data, unsigned short port)
{
	(*pci->write_io_32)(port, data);
}

void
_linux_outb(unsigned char data, unsigned short port)
{
	(*pci->write_io_8)(port, data);
}


unsigned char _linux_inb(unsigned short port)
{
	unsigned char res;

	res = (*pci->read_io_8)(port);
	return (res);
}

unsigned short _linux_inw(unsigned short port)
{
	unsigned short res;

	res = (*pci->read_io_16)(port);
	return (res);
}

unsigned long _linux_inl(unsigned short port) 
{
	unsigned long res;

	res = (*pci->read_io_32)(port);
	return (res);
}

void
linux_ether_init(void)
{
	locksem = create_sem(1, "le_lock");
	timerlocksem = create_sem(1, "le_timer_lock");	
}

void
linux_ether_uninit(void)
{
	delete_sem(locksem);
	delete_sem(timerlocksem);
}

void *
linux_ether_probe(int *ncards, int (*init_module)(void))
{
	struct device *dev = NULL;
	int n;

	Lock();
	devices = NULL;
	if (init_module() == 0) {
		dev = devices;
		n = 0;
		while (devices) {
			n++;
			devices = devices->next;
		}
		*ncards = n;
	}
	Unlock();
	if (dev) {
		dev->sklist = NULL;
		dev->sksem = B_ERROR;
		alock_init(&dev->ps, &dev->lock);
		return ((void *)dev);
	}
	return (NULL);
}

int 
linux_ether_open(void *cookie, int cardno, void **session_cookie)
{
	struct device *dev = (struct device *)cookie;
	int n;
	int res;

	for (n = 0; n < cardno; n++) {
		if (dev == NULL) {
			return (EPERM);
		}
		dev = dev->next;
	}
	res = dev->init(dev);
	if (res != 0) {
		return (-ABS(res));
	}
	if (dev->open == NULL) {
		return (ENODEV);
	}
	res = dev->open(dev);
	if (res != 0) {
		return (-ABS(res));
	}
	dev->sksem = create_sem(0, "sk");
	*session_cookie = dev;
	return (0);
}

int
linux_ether_getaddr(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;

	memcpy(buf, dev->dev_addr, 6);
	return (B_NO_ERROR);
}

int
linux_ether_getframesize(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;
	const int hardcoded = 1514;

	memcpy(buf, &hardcoded, sizeof(hardcoded));
	return (B_NO_ERROR);
}

#define addreq(a, b) (memcmp(a, b, 6) == 0)

int
linux_ether_addmulti(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;
	struct dev_mc_list **lpp;
	struct dev_mc_list *tmp;

	for (lpp = &dev->mc_list; lpp && *lpp; lpp = &(*lpp)->next) {
		if (addreq((*lpp)->dmi_addr, buf)) {
			return (B_ERROR);
		}
	}
	tmp = malloc(sizeof(*tmp));
	memcpy(tmp->dmi_addr, buf, 6);
	if(lpp)
		*lpp = tmp;
	tmp->next = NULL;
	dev->mc_count++;
	dev->set_multicast_list(dev);
	return (B_NO_ERROR);
}

int
linux_ether_remmulti(void *cookie, char *buf)
{
	struct device *dev = (struct device *)cookie;
	struct dev_mc_list **lpp;
	struct dev_mc_list *tmp;

	for (lpp = &dev->mc_list; lpp && *lpp; lpp = &(*lpp)->next) {
		if (addreq((*lpp)->dmi_addr, buf)) {
			tmp = *lpp;
			*lpp = (*lpp)->next;
			free(tmp);
			dev->mc_count--;
			dev->set_multicast_list(dev);
			return (B_NO_ERROR);
		}
	}
	return (B_ERROR);
}

int
linux_ether_setpromisc(void *cookie, int promisc)
{
	struct device *dev = (struct device *)cookie;
	
	if (promisc) {
		dev->flags |= IFF_PROMISC;
	} else {
		dev->flags &= ~IFF_PROMISC;
	}
	dev->set_multicast_list(dev);
	return (B_NO_ERROR);
}

int
linux_ether_close(void *cookie)
{
	struct device *dev = (struc