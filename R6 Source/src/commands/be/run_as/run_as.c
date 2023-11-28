
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>

extern char **environ;

char *concat(const char *left, const char *right)
{
	char *x = (char *) malloc(strlen(left) + strlen(right) + 1);
	strcpy(x,left);
	strcat(x,right);
	return x;
}

void setup_env(const char *shell, const char *user, const char *home)
{
	char *term = getenv("TERM");
	
	environ = (char **) malloc(sizeof(char *) * 2);
	environ[0] = 0;
	
	putenv("PATH=/bin");
	putenv(concat("HOME=",home));
	putenv(concat("SHELL=",shell));
	putenv(concat("USER=",user));
	putenv(concat("LOGNAME=",user));
	if(term) putenv(concat("TERM=",term));
}

int main(int argc, char *argv[])
{
	int i,uid,gid;
	char **args;
	struct passwd *pw;

	if(argc < 2) {
		fprintf(stderr,"usage: run_as <uid> [ <program> [ <arg> ... ] ]\n");
		return 1;
	}

	uid = atoi(argv[1]);

	if(argc > 2){
		args = (char **) malloc(sizeof(char*) * argc);
		for(i=0;i<(argc-2);i++) args[i] = argv[i+2];
		args[i] = NULL;
	} else {
		args = (char **) malloc(sizeof(char*) * 2);
		args[0] = "/bin/sh";
		args[1] = NULL;
	}

	if(pw = getpwuid(uid)){
		gid = pw->pw_gid;
//		setup_env(args[0],pw->pw_name,pw->pw_dir);
	} else {
		gid = 0;
//		setup_env(args[0],"baron","/boot/home");
	}	
		
	setgid(gid);
	setuid(uid);
	execv(args[0],args);
	return 0;
}

