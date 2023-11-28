[4] = '0' + (bcd & 0x000F);
	s[3] = '0' + ((bcd & 0x00F0) >> 4);
	s[2] = '.';
	s[1] = '0' + ((bcd & 0x0F00) >> 8);
	s[0] = '0' + ((bcd & 0xF000) >> 12);
	return s[0] == '0' ? s+1 : s;
}

static void print_dev_descr(bm_usb_device *dev, usb_device_descriptor *d)
{
	char bcd[6];
	dprintf("usb version:             %s\n",bcdstr(bcd,d->usb_version));
	dprintf("class/subclass/protocol: %d / %d / %d\n",
			d->device_class,d->device_subclass,d->device_protocol);
	dprintf("max packet size 0:       %d\n",d->max_packet_size_0);
	dprintf("vendor/product id:       0x%04x / 0x%04x\n",d->vendor_id,d->product_id);
	dprintf("device version:     