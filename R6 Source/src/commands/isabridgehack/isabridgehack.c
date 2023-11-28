#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<OS.h>
#include	<PCI.h>

/*
"bus dev func offset value"
cfoutb 0 12 0 0x70 0x70
cfoutb 0 12 0 0x71 0x02
cfoutb 0 12 0 0x72 0xE7
quit
*/

int main( void )
{
	int32 			i;
	pci_info		device;
	
	printf( "Fudging isa-to-pci bridge to make it work...\n" );
	
	for( i=0; get_nth_pci_info( i, &device ) == B_OK; i++ )
	{
		if( (device.vendor_id == 0x1078)&&(device.device_id == 0x0100) )
		{
			write_pci_config( device.bus, device.device, device.function, 0x70, 1, 0x70 );
			write_pci_config( device.bus, device.device, device.function, 0x71, 1, 0x02 );
			write_pci_config( device.bus, device.device, device.function, 0x72, 1, 0xE7 );
			break;
		}
	}
	
	return 0;
}
