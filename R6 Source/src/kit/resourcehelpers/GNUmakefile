	
	return B_OK;
}


static i2o_info *
init_device(const char *_name, const pci_info *pci)
{
	int i;
	i2o_info *ii;
	int iobase, iosize, irq;
	physical_entry pe[2];
	char name[32];
	i2o_context *c;
	
	for(ii = first_info; ii; ii = ii->next){
		if((ii->bus == pci->bus) && (ii->device == pci->device) 
		   && (ii->function == pci->function)){
			dprintf("i