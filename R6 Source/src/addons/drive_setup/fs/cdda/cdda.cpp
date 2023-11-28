
#include <unistd.h> 
#include <scsi.h> 
#include <stdio.h>
#include <string.h>
#include "cdda_io.h"

#define USE_CDDB


#if defined(USE_CDDB)
void GetVolumeName(ulong ID, char *name);
#endif

typedef struct { 
  char    partition_name[B_FILE_NAME_LENGTH]; 
  char    partition_type[B_FILE_NAME_LENGTH]; 
  char    file_system_short_name[B_FILE_NAME_LENGTH]; 
  char    file_system_long_name[B_FILE_NAME_LENGTH]; 
  char    volume_name[B_FILE_NAME_LENGTH]; 
  char    mounted_at[B_FILE_NAME_LENGTH]; 
  uint32  logical_block_size; 
  uint64  offset;    /* in logical blocks from start of session */ 
  uint64  blocks;    /* in logical blocks */ 
  bool    hidden;    /* non-file system partition */ 
  bool    reserved1; 
  uint32  reserved2; 
} partition_data;

typedef struct { 
  uint64  offset;    /* in device blocks */ 
  uint64  blocks;    /* number of blocks in session */ 
  bool    data;      /* true: data session, session */ 
} session_data;


extern "C" _EXPORT  bool ds_fs_id(partition_data*, int32, uint64, int32);;

bool ds_fs_id(partition_data *partition, int32 dev, uint64 session_offset, int32 block_size)
{
	t_CDtoc toc;
	bool result;

	result = false;

	if(ioctl(dev, B_SCSI_GET_TOC, &toc)==B_OK)
	{
		// quick, dirty
		if(session_offset==0x7fffffff)
		{
			strcpy(partition->file_system_short_name,"cdda");
			strcpy(partition->file_system_long_name,"Audio Filesystem");
			strcpy(partition->volume_name,"Audio CD");
#if defined(USE_CDDB)
			GetVolumeName(cddb_discid(&toc),partition->volume_name);
#endif
			result=true;
		}
	}
	else
	{
		// printf("FS add-on loaded for non-CD\n");
	}

	return result;
}

#if defined(USE_CDDB)

static int cddb_sum(int n);
static int cddb_sum(int n) 
{
    char buf[12];
    char *p;
    unsigned long ret = 0;

    /* For backward compatibility this algoritm must not change */
    sprintf(buf, "%lu", n);
    for (p=buf; *p != '\0'; p++)
        ret += (*p-'0');
    return ret;
}

unsigned long cddb_discid(t_CDtoc *cdtoc);
unsigned long cddb_discid(t_CDtoc *cdtoc) 
{
    int i;
    int n=0;
    int t=0;

    /* For backward compatibility this algoritm must not change */
    for (i=0;i<((cdtoc->maxtrack-cdtoc->mintrack))+1;i++) 
    {
        n+=cddb_sum((cdtoc->tocentry[i].address.minute*60)+cdtoc->tocentry[i].address.second);
        t+= ((cdtoc->tocentry[i+1].address.minute*60)+cdtoc->tocentry[i+1].address.second) -
            ((cdtoc->tocentry[i].address.minute*60)+cdtoc->tocentry[i].address.second);
    }
    return (((n%0xff) << 24) + (t << 8) + cdtoc->maxtrack);
}

char *SanitizeString(char *ss);
char *SanitizeString(char *ss)
{
	char *s=ss;
	while(s&&(*s!='\0'))
	{
		if(*s=='/')
			*s='-';
		s++;
	}
	return ss;
}


static int ReadLine(int fd, char *buffer, int maxfill);
static int ReadLine(int fd, char *buffer, int maxfill)
{
	char c;
	int numread;
	int total=0;
	
	do {
		numread=read(fd,&c,1);
		if((numread==1)&&(c!='\n'))
		{
			*buffer++=c;
			maxfill--;
			total++;
		}
	} while((numread==1)&&(c!='\n')&&(maxfill>0));
	*buffer='\0';
	return total;
}


void GetVolumeName(ulong ID, char *name)
{
	char filename[128];
	char trackname[B_FILE_NAME_LENGTH+2];
	
	sprintf(filename,"%s/%08x",cddbfiles,ID);
	int file=open(filename,O_RDONLY);
	
	if(file>=0)
	{
		//printf("reading from %s\n",filename);
		if(ReadLine(file,trackname,B_FILE_NAME_LENGTH)!=0)
			strcpy(name,trackname);
		SanitizeString(name);
		close(file);
	}
//	else
//		printf("couldn't open file\n");
}
#endif

