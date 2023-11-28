#include <OS.h>
#include <stdio.h>
#include <scsi.h>
#include <CAM.h>

#define IOSMAX (8192 / sizeof(cam_iostat))

int main(int argc, char *argv)
{
	area_id area;
	int i;
	cam_iostat *is;
	double tb,ts;
	double _b,_s;
	
	if((area = find_area("CAM_IO_STATS")) < 0){
		fprintf(stderr,"no SCSI CAM statspage\n");
		return 0;
	}

	i = clone_area("camstats_local", (void **) &is, B_ANY_ADDRESS, B_READ_AREA, area);
	if(i < 0) {
		fprintf(stderr,"cannot clone aic78xx statspage\n");
		return 0;
	}

printf("serial no.  path  targ  op  segs  len (b)     time (us)   bytes/sec\n");
printf("----------  ----  ----  --  ----  ----------  ----------  ----------\n");

	tb = ts = 0;
	for(i=0;i<IOSMAX;i++){
		if(!is[i].serial) continue;
		_b = ((double) is[i].bytes);
		_s = (((double) is[i].micros) / 1000000.0);
	printf("%10d  %4d  %4d  %02x  %4d  %10d  %10d  %15d\n",
		is[i].serial, is[i].path, is[i].target, is[i].scsi_op,
		is[i].sgcount, is[i].bytes, is[i].micros, ((int) (_b/_s)) );
		/* (is[i].micros/1000) > 0 ? (is[i].bytes / (is[i].micros/1000)) : 0); */
		tb += is[i].bytes;
		ts += is[i].micros;

	}
	ts /= 1000000.0;
	printf("\navg bytes/sec = %d\n", (int) (tb/ts));
}

