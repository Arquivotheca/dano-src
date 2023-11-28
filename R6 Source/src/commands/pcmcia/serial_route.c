#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

void republish(char *name)
{
	int fd;
	if((fd = open("/dev", O_WRONLY)) > 0){
		write(fd, name, strlen(name));
		close(fd);
	}
	return;
}

int fdserial, fdpcmcia;
unsigned short data[2], config[16];
int i;

void sigterm(int n)
{
	fprintf(stderr,"terminating on signal\n");
	data[0] = 0;
	data[1] = 0;
	lseek(fdserial,4*i,SEEK_SET);
	write(fdserial,data,4);
	republish("zz");
	close(fdserial);
	abort();
}

int main(int argc, char *argv[])
{
	char buf[128];
	
	if((fdpcmcia = open("/dev/bus/pcmcia/serial/serial0",O_RDONLY)) < 0){
		fprintf(stderr,"error: cannot open /dev/bus/pcmcia/serial/serial0\n");
		return 0;
	}
	
	if((fdserial = open("/dev/config/serial",O_RDWR)) < 0){
		close(fdpcmcia);
		fprintf(stderr,"error: cannot open /dev/config/serial\n");
		return 0;
	}
	
	read(fdpcmcia, data, 4);
	read(fdserial, config, 32);
	
	for(i=0;i<8;i++){
		if(config[i*2]){
			fprintf(stderr,"/dev/ports/serial%d: 0x%04x, irq %d\n",
					i+1,config[i*2],config[i*2+1]);
		} else {
			lseek(fdserial,4*i,SEEK_SET);
			write(fdserial,data,4);
			fprintf(stderr,"/dev/ports/serial%d: 0x%04x, irq %d (PCMCIA)\n",
					i+1,config[i*2],config[i*2+1]);			
			republish("zz");
			signal(SIGTERM, sigterm);
			signal(SIGINT, sigterm);
			signal(SIGHUP, sigterm);
			for(;;) sleep(1000);
			sigterm(0);
			break;
		}
	}
	close(fdpcmcia);
	close(fdserial);

	abort();
	
	return 0;
}
