
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <OS.h>

static struct termios save_termios;
 
void make_raw(int fd)
{
	struct termios t;
	tcgetattr(fd, &t);

	t.c_lflag &= (~(ECHO|ECHOE|ECHOK|ECHONL|ICANON));  /* no echoback */
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(fd,TCSANOW,&t);
}

void make_cooked(int fd)
{
	tcsetattr(fd,TCSANOW,&save_termios);
}

enum {
	PLAIN, ESCAPED, ANSI
};

#define beep() write(out,"\07",1);

#define MAXLINE 255
#define MAXHIST 128

#define CTRL(x) (x - 'a' + 1)

typedef struct line line;

struct line
{
	line *prev;
	line *next;
	char buf[MAXLINE+1];
};

static line *first = NULL;
static line *last = NULL;
static unsigned linecount = 0;

void init_readline(int in)
{
	if(!first){
		tcgetattr(in, &save_termios);
		first = last = (line*) malloc(sizeof(line)); 
		first->prev = first;
		first->next = first;
		first->buf[0] = 0;
	}
}

static char tcc_path[1024];
static char tcc_name[256];
static char tcc_maybe[256];

int tab_complete(char *buffer, int *pos, int *tcc)
{
	char *start,*end,*x;
	char tcc_temp[2048];
	DIR *dir;
	dirent_t *de;
	struct stat si;

	end = buffer + *pos;
	start = end - 1;
	if(start < buffer) return 0;
	
	while((*start != ' ') && (start > buffer)) start--;
	
	if(*start == ' ') start++;

	tcc_maybe[0] = 0;	
	strncpy(tcc_path, start, end-start);
	tcc_path[end-start] = 0;
	
	x = strrchr(tcc_path,'/');
	
	if(x) {
		if(*(x+1)){
			strcpy(tcc_name,x+1);
			if(*x != '/'){
				*(x+1) = '/';
				*(x+2) = 0;
			} else {
				*(x+1) = 0;
			}
		} else {
			tcc_name[0] = 0;
		}
	} else {
		strcpy(tcc_name,tcc_path);
		tcc_path[0] = 0;
	}
	
	if(tcc_path[0] != '/'){
		sprintf(tcc_temp,"./%s",tcc_path);
	}  else {
		strcpy(tcc_temp,tcc_path);
	}
	
	
#ifdef READLINETEST
	printf("\ntab complete: '%c' \"%s\" / \"%s\"\n",*(end-1),tcc_path,tcc_name);
#endif
	dir = opendir(tcc_temp);
	if(dir) {
		if(*tcc == 1) {
			/* we partial matched last time -- show options */
			int p = 0,l,nl;
			nl = strlen(tcc_name);
			printf("\n");
			while((de = readdir(dir))) {
				if(!strcmp(de->d_name,".")) continue;
				if(!strcmp(de->d_name,"..")) continue;
				if(nl && strncmp(tcc_name,de->d_name,nl)) continue;
				l = strlen(de->d_name);
				if(p && (p + l > 76)) {
					printf("\n");
					p = 0;
				}
				printf("%s ",de->d_name);
				p += l + 1;
			}
			if(p) printf("\n");
			closedir(dir);
			return 1;
		} else if (tcc_name[0]) {
			/* look for a match */
			while((de = readdir(dir))) {
				if(!strcmp(de->d_name,".")) continue;
				if(!strcmp(de->d_name,"..")) continue;
			
				if(!strncmp(tcc_name,de->d_name,strlen(tcc_name))){
					if(tcc_maybe[0]){
						int i;
						*tcc = 1; /* partial match */
						/* more than one match .. how much is similar? */
						for(i=0;tcc_maybe[i] && de->d_name[i];i++){
							if(tcc_maybe[i] != de->d_name[i]) break;
						}
						tcc_maybe[i] = 0;
					} else {
						*tcc = 0; /* match */
						strcpy(tcc_maybe,de->d_name);
					}
				}
			}
		} else {
			/* partial match -- directory only */
			*tcc = 1;
		}
		closedir(dir);

		if(tcc_maybe[0]) {
			strcpy(tcc_name,tcc_maybe);
	
			sprintf(tcc_temp,"%s%s",tcc_path,tcc_name);
			if(stat(tcc_temp,&si) == 0) {
				if(S_ISDIR(si.st_mode)) {
					if(tcc_temp[strlen(tcc_temp)-1]!='/'){
						strcat(tcc_name,"/");
					}
				}
			}
		}
	}
	
	*start = 0; 
#ifdef READLINETEST
	printf("tab complete: \"%s\" \"%s\" \"%s\" \"%s\"\n",
		   buffer,tcc_path,tcc_name,end);
#endif
	
	sprintf(tcc_temp,"%s%s%s%s",buffer,tcc_path,tcc_name,end);
	*pos = strlen(buffer) + strlen(tcc_path) + strlen(tcc_name);
	
	strcpy(buffer,tcc_temp);
	
	return 1;
}

void
ctrlc(int s)
{
	// empty signal handler
}

void readline(int in, int out, char *prompt, char *buf)
{
	unsigned char c;
	int bpos,blen;
	int i;
	int state = PLAIN;
	line *history = last;
	int tcc;

	void (*prev)(int)= signal(SIGINT, ctrlc);
	thread_id   thid = find_thread(NULL);

	setpgid( thid, thid);	
	ioctl(0, 'pgid', thid);
	ioctl(1, 'pgid', thid);
	ioctl(2, 'pgid', thid);	


	buf[0] = 0;

	make_raw(in);	

	goto restart;
		
bell:
	write(out, "\07", 1);
	goto dirty;
	
restart:
	bpos = blen = strlen(buf);
	tcc = 0;

dirty:	
	write(out, "\r", 1);
	write(out,"\033[K",3); // clear-to-eol			
	write(out, prompt, strlen(prompt));	
	if(blen) write(out, buf, blen);
	for(i=0;i<(blen - bpos);i++) write(out, "\b", 1);
		
	for(;;){
		if(read(in, &c, 1) == 1) {
			if(c != 9) tcc = 0;
				
			switch(state){
			case PLAIN:
				switch(c){
				case CTRL('a'): /* go to beginning */
					bpos = 0; 
					goto dirty;
				
				case CTRL('e'): /* go to end */
					bpos = blen; 
					goto dirty;
					
				case CTRL('k'): /* clear to end */
					buf[bpos] = 0; 
					goto restart;
					
				case CTRL('b'): /* back */
					goto arrow_left;
				
				case CTRL('f'): /* forward */
					goto arrow_right;
				
				case CTRL('p'): /* previous */
					goto arrow_up;
					
				case CTRL('n'): /* next */
					goto arrow_down;
				
				case 9: /* tab */
					if(tab_complete(buf, &bpos, &tcc)) {
						blen = strlen(buf);
						if(tcc) goto bell;
						goto dirty;
					}
					goto bell;
				
				case 27: /* ansi escape start */
					state = ESCAPED; 
					break;
				
				case CTRL('d'): /* close shell  and/or delete under cursor */
					if(blen== 0) {
						write(out, "\n\r", 2);
						make_cooked(in);
						exit(0);
					}
				case 127: /* Delete Under Cursor */
					if(blen != bpos){
						for(i = bpos; i < blen; i++) buf[i] = buf[i+1];
						blen--;
						goto dirty;
					}					
					break;
					
				case '\b': /* Delete To Left */
					if(bpos == 0) goto bell;
					bpos--;
					blen--;
					memcpy(buf + bpos, buf + bpos + 1, blen - bpos + 1);
					goto dirty;
				
				case '\n':
				case '\r':
					goto done;
					
				default:
					if((c < ' ') || (c > 126)) goto bell;
				
					if(blen == MAXLINE) goto bell;
									
					if(bpos < blen){
						/* make room for new char */
						blen++;
						for(i = blen; i > bpos; i--) buf[i] = buf[i-1];
						buf[blen+1]=0;
						buf[bpos] = c;
						bpos++;
						goto dirty;
					}
				
					buf[bpos] = c;
					bpos++;
					blen++;
					buf[bpos] = 0;
					write(out, &c, 1);
					break;
				}
				break;
				
			case ESCAPED:
				if(c == '[') {
					state = ANSI;
				} else {
					state = PLAIN;
				}
				break;
				
			case ANSI:
				state = PLAIN;
				
				if(c == 'A'){
arrow_up:
					if(history->prev != last){
						history = history->prev;
						strcpy(buf, history->buf);
						goto restart;
					} else {
						goto bell;
					}
				}
				if(c == 'B'){
arrow_down:
					if(history != last){
						history = history->next;
						strcpy(buf, history->buf);
					} else {
						buf[0] = 0;
					}
					goto restart;
				}
				if(c == 'D'){
arrow_left:
				if(bpos == 0) goto bell;
					bpos--;
					goto dirty;
				}
				if(c == 'C'){
arrow_right:
					if(bpos == blen) goto bell;
					bpos++;
					goto dirty;
				}
				
				goto bell;
			}
		} else {
			if(errno== EINTR) {
				write(out, "\n", 1);
				*buf= 0;
				goto restart;
			} else {
				exit(0);
			}
		}
	}
	
done:
	write(out, "\n\r", 2);
	make_cooked(in);
	
	if(buf[0]){
		/* add to history ring */
		if(linecount < MAXHIST){
			line *l = (line*) malloc(sizeof(line));
			l->prev = first;
			l->next = first->next;
			first->next->prev = l;
			first->next = l;
			first = l;
			linecount ++;
		} else {
			last = last->next;
			first = first->next;
		}
		strcpy(first->buf, buf);	
	}
	
	signal(SIGINT, prev);
}

#if READLINETEST
int main(int argc, char *argv[])
{
	char buf[MAXLINE+1];
	init_readline(0);
	for(;;){
		readline(0, 1, "prompt> ",buf);
		if(!strcmp(buf,"quit")) break;
		printf("action: %s\n",buf);
	}
	return 0;
}
#endif
