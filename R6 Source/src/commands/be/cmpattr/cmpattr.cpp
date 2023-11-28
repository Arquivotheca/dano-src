#include <vector>
#include <string>
#include <algorithm>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <fs_attr.h> 




status_t cmpattr(const char* f1,const char* f2, vector<string> & ignore_list);
void usage(const char* name);


bool quiet=false;


int main(const int argc,const char** argv)
{
	int i=1,fd1,fd2;
	vector<string> ignore_list;
	
	while(argv[i][0]=='-')
	{
		if( strcmp(argv[i],"-i")==0 || strcmp(argv[i],"--ignore")==0)
		{
			ignore_list.push_back(*(new string(argv[++i])));
			i++;
		}
		if( strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0 )
		{
			usage(argv[0]);
		}
		if( strcmp(argv[i],"-q")==0 || strcmp(argv[i],"--quiet")==0 )
		{
			quiet=true;
			i++;
		}
	}
	
	if(i != argc-2)
	{
		usage(argv[0]);
	}
	
	
	int ret=cmpattr(argv[i],argv[i+1],ignore_list);
	if( ret < 0 )
	{
		return 2;
	}
	else
	{
		return ret;
	}
}


void usage(const char* name)
{
	printf("Usage: %s [options] File1 File2 \n",name);
	printf("returns 0 if the attributes match, 1 if they don't. and >1 on error.\n");
	printf("\t-h --help             Output this help.\n");
	printf("\t-q --quiet            Output Nothing.\n");
	printf("\t-i --ignore AttrName  Ignores the attribute named AttrName when comparing files. \n");
	
	exit(2);
}
	
status_t cmpattr(const char* f1,const char* f2, vector<string> & ignore_list)
{
	DIR *a1,*a2;
	struct dirent* e;
	attr_info info1,info2;
	char *buf1,*buf2;
	int fd1,fd2;

	
	if( (fd1=open(f1,O_RDONLY))==-1 )
	{
		fprintf(stderr,"couldn't open file: %s Error: ", f1);
		perror("");
		return errno;
	}
	
	if( (fd2=open(f2,O_RDONLY))==-1 )
	{
		fprintf(stderr,"couldn't open file: %s Error: ", f2);
		perror("");
		return errno;
	}
	
	
	if( !(a1=fs_fopen_attr_dir(fd1)) )
	{
		fprintf(stderr,"fd: %i Error: ",fd1);
		perror("");
		return errno;
	}
	
	if( !(a2=fs_fopen_attr_dir(fd2)) )
	{
		fprintf(stderr,"fd: %i Error: ",fd2);
		perror("");
		return errno;
	}

	
	while(e=fs_read_attr_dir(a1))
	{
		string str(e->d_name);
		if( find(ignore_list.begin(),ignore_list.end(),str)!=ignore_list.end() )
		{
			if(!quiet)
			{
				printf("Ignoring %s \n",e->d_name);
			}
			continue;
		}
		
		
		
		if(fs_stat_attr(fd1,e->d_name,&info1)==-1)
		{
			if(!quiet)
			{
				fprintf(stderr,"stating %s should not be here. Error: ",e->d_name);
			}
			perror("");
			return errno;
		}
		
		if(fs_stat_attr(fd2,e->d_name,&info2)==-1)
		{
			if(errno==B_ENTRY_NOT_FOUND)
			{
				if(!quiet)
				{
					printf("Attribute: %s present in %s but not in %s \n",e->d_name,f1,f2);
				}
				return errno;
			}
			else
			{
				fprintf(stderr,"stating %s should not be here. Error: ",e->d_name);
				perror("");
				return errno;
			}
		}
		else
		{
			if( info1.type==info2.type && info1.size==info2.size )
			{
				buf1=(char*)malloc(info1.size);
				buf2=(char*)malloc(info2.size);
				
				if(buf1 && buf2)
				{
					fs_read_attr(fd1,e->d_name,info1.type,0,buf1,info1.size);
					fs_read_attr(fd2,e->d_name,info2.type,0,buf2,info2.size);
					if(memcmp(buf1,buf2,info1.size)==0)
					{
						continue;
					}
					else
					{
						if(!quiet)
						{
							printf("The content of the attribute: %s differs in %s and %s \n",e->d_name,f1,f2);
						}
						return 1;
					}
					
				}		
			}
			else
			{
				if(!quiet)
				{
					printf("The content of the attribute: %s differs in %s and %s \n",e->d_name,f1,f2);			
				}
				return 1;
			}
		}
	}
	
	if(errno==B_OK)
	{
		if(!quiet)
		{
			printf("Attributes are matching \n");
		}
		return 0;
	}
	else
	{
		fprintf(stderr,"Error while reading attr dir : %i Error: ", a1->fd);
		perror("");
		return errno;
	}
}

