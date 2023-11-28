// iad/src/commands/be/dynamicip.c
//
// very simple shelltool to rewrite your beia-network file to use
// a dynamic IP address (undoes the work of staticip).
//
// maintainer: dsandler@be.com
//
//
// i added some new lines in here to make sure we rewrite the other
// net device information as it seems to get erased under some
// unknown circumstances... can't wait for bone!!! - hopbot@be.com

#include <stdio.h>

int main(long argc, char **argv) {
	char buf[1024];
	char *eol;
	FILE *oldcf;
	FILE *cf;
	
	if (argc!=1) {
		printf("usage: dynamicip\n"
		       "\tRewrites /boot/home/config/settings/beia-network to use DHCP.\n"
		       "\tWARNING: this is a hack. Please install BONE.\n");
		exit(1);
	}

	buf[0] = '\0';
	
	oldcf = fopen("/boot/home/config/settings/beia-network", "r");
	cf = fopen("/boot/home/config/settings/beia-network-dhcp", "w");

	if (oldcf == NULL) {
		fprintf(stderr, "dynamicip: error: could not open beia-network\n");
		exit(1);
	}

	if (cf == NULL) {
		fprintf(stderr, "dynamicip: error: could not open beia-network-dhcp for writing\n");
		exit(1);
	}

	while(1) {
		if (fgets(buf, 1024, oldcf) == NULL) break;
		if (buf[0] == '\0')
			break;

		eol = (char*)strrchr(buf, '\n'); // find the last newline
			if (eol != NULL) *eol = '\0';
			
		if (strncmp(buf,"interface0_ipaddress=", strlen("interface0_ipaddress=")) == 0

			|| strncmp(buf,"autodial=", strlen("autodial=")) == 0
			|| strncmp(buf,"interfaces=", strlen("interfaces=")) == 0
			|| strncmp(buf,"interface0_addon=", strlen("interface0_addon=")) == 0
			|| strncmp(buf,"interface0_devicelink=", strlen("interface0_devicelink=")) == 0
			|| strncmp(buf,"interface0_deviceirq=", strlen("interface0_deviceirq=")) == 0
			|| strncmp(buf,"interface0_deviceport=", strlen("interface0_deviceport=")) == 0
			|| strncmp(buf,"interface0_dhcpdelay=", strlen("interface0_dhcpdelay=")) == 0

			|| strncmp(buf,"interface0_netmask=", strlen("interface0_netmask=")) == 0
			|| strncmp(buf,"interface0_router=", strlen("interface0_router=")) == 0
			|| strncmp(buf,"DNS_PRIMARY=", strlen("DNS_PRIMARY=")) == 0
			|| strncmp(buf,"interface0_dhcp=", strlen("interface0_dhcp=")) == 0)
			;
		else {
			fprintf(cf, "%s\n", buf);
		}
	}
	fclose(oldcf);

	fprintf(cf, "interfaces=\"interface0\"\n");
	fprintf(cf, "interface0_addon=\"ne2000\"\n");
	fprintf(cf, "interface0_devicelink=\"/dev/net/ether\"\n");
	fprintf(cf, "interface0_deviceirq=\"9\"\n");
	fprintf(cf, "interface0_deviceport=\"300\"\n");
	fprintf(cf, "interface0_dhcpdelay=\"1\"\n");

	fprintf(cf, "interface0_ipaddress=\"0.0.0.0\"\n", argv[1]);
	fprintf(cf, "interface0_netmask=\"0.0.0.0\"\n", argv[2]);
	fprintf(cf, "interface0_router=\"0.0.0.0\"\n", argv[3]);
	fprintf(cf, "interface0_dhcp=\"1\"\n");
	fclose(cf);
	
	rename("/boot/home/config/settings/beia-network",
		"/boot/home/config/settings/beia-network-old");
	rename("/boot/home/config/settings/beia-network-dhcp",
		"/boot/home/config/settings/beia-network");
	
	exit(0);
}