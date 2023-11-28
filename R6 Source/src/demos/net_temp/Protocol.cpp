#include <Be.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "UTILS.H"
#include "protocol.h"

//--------------------------------------------------------------------

void	pdebug_print(char *label, char *p, long n)
{
	long	i;

	
	printf("%s ", label);
	
	printf("<");
	for (i = 0; i < n; i++)
		printf("%c", p[i]);
	printf(">");
	
	printf("\n");
}

//--------------------------------------------------------------------

const char* NETRESULTSTR(NetResult result)
{
	switch (result) {
		case NET_NO_ERROR:			return "NET_NO_ERROR";
		case NET_NO_SOCKETS:		return "NET_NO_SOCKETS";
		case NET_UNKNOWN_HOST:		return "NET_UNKNOWN_HOST";
		case NET_CANT_CONNECT:		return "NET_CANT_CONNECT";
		case NET_CANT_SEND:			return "NET_CANT_SEND";
		case NET_CANT_RECV:			return "NET_CANT_RECV";
		case NET_TIMEOUT:			return "NET_TIMEOUT";
		case NET_IS_CLOSED:			return "NET_IS_CLOSED";
		case NET_ALREADY_CLOSED:	return "NET_ALREADY_CLOSED";
	}
	static char s[256];
	sprintf(s,"Network Error: %d",result);
	return s;
}


//--------------------------------------------------------------------

void	set_pluses(char *s)
{
	char	c;
	
	while(c = *s) {
		if (c == ' ')
			*s = '+';
		s++;
	}
}

//--------------------------------------------------------------------

void	remove_pluses(char *s)
{
	char	c;
	
	while(c = *s) {
		if (c == '+')
			*s = ' ';
		s++;
	}
}

//--------------------------------------------------------------------

void	parse(char *raw, char *site, char *path)
{
	long	p;
	char	*copy;
	
again:;

	p = 0;
	
	copy = raw;
	
	while(((*raw != '/') || (*(raw + 1) != '/')) && (*raw)) {
		raw++;
	}
	
	raw += 2;
	
	while(*raw != '/' && *raw) {
		site[p] = *raw;
		raw++;
		p++;
	}
	site[p] = 0;
	
	strcpy(path, raw);
	if (strlen(path) == 0) {
		path[0] = '/';
		path[1] = 0;
	}
}

//--------------------------------------------------------------------

ulong finder::LookupHost(const char *host)
{
	ulong result = 0;
	
	
	hostent* h = gethostbyname(host);
	if (h && h->h_addr)
		result = *(long *)(h->h_addr);

	result = ntohl(result);
	ulong a = result;
	return result;
}


//-------------------------------------------------------------------------

	finder::finder()
{
}

//-------------------------------------------------------------------------

	hb_find::hb_find()
	: av_find()
{
}

//-------------------------------------------------------------------------

	finder::~finder()
{
}

//-------------------------------------------------------------------------

	av_find::~av_find()
{
	long	i;

	for (i = 0; i < 128; i++) {
		if (match_list[i]) free(match_list[i]);
		if (desc_list[i]) free(desc_list[i]);
	}
	
	cur_pos = 0;
}



//-------------------------------------------------------------------------

	av_find::av_find()
	: finder()
{
	long	i;

	for (i = 0; i < 128; i++) {
		match_list[i] = 0;
		desc_list[i] = 0;
	}
	
	cur_pos = 0;
}

//-------------------------------------------------------------------------


//-------------------------------------------------------------------------

	site_getter::site_getter()
{
	done_sem = create_sem(0, "site_getter");
	fbuffer = 0;
	done = 0;
	msocket = -1;
}

//-------------------------------------------------------------------------

	site_getter::~site_getter()
{
	printf("destroy\n");
	free((char *)fbuffer);
}

//-------------------------------------------------------------------------

	gif_getter::gif_getter() : site_getter()
{
}

//-------------------------------------------------------------------------

	gif_getter::~gif_getter()
{

}

//-------------------------------------------------------------------------


int	finder::Connect(char *host)
{
	sockaddr_in	addr;
	ulong	result;
	int		s;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);

	result = LookupHost(host);
	addr.sin_addr.s_addr = htonl(result);

	
	if (result == 0) {
		printf("cannot resolve %s\n", host);
		return -1;
	}
	
	s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (s < 0) {
		printf("error1\n");
		return NET_NO_SOCKETS;		// Can't get a socket
	}
	
	if (connect(s,(sockaddr *)&addr,sizeof(sockaddr_in)) < 0) {
		printf("a3 %s\n", host);
		closesocket(s);
		printf("cannot connect to %s\n", host);
		printf("error = %ld\n", NET_CANT_CONNECT);
		return NET_CANT_CONNECT;	// Can't connect! Geez!
	}
	return s;
}

//-------------------------------------------------------------------------

long finder::FillBuffer(long msocket, char *buffer)
{
	long 	size;
	long	i;
	
	size = 4096;
	size = recv(msocket, buffer, size, 0);
	
	printf("got %ld\n", size);
	return size;
}

//-------------------------------------------------------------------------

uchar	low(uchar c)
{
	if ((c >= 'A') && (c <= 'Z')) {
		c = c + 0x020;
	}
	
	return c;
}

//-------------------------------------------------------------------------

long	finder::find(char *buf, char *str, long pos, long size)
{
	long	i;

//	printf("f0 %ld %ld\n", pos, size);
	if (pos < 0)
		return -1;
		
	buf += pos;
	size -= pos;
	
	while(size > 0) {
		i = 0;
		
		while((*(str+i)) && (*(str+i) == (low(*(buf+i))))) {
			i++;
		}
		if (*(str+i) == 0) {
//			printf("f1 %ld %ld\n", pos, size);
			return pos;
		}
		
		size--;
		pos++;
		buf++;
	}
//	printf("f2 %ld %ld\n", pos, size);
	return -1;
}

//-------------------------------------------------------------------------

void	av_find::process(char *buffer, long size)
{
	long	pos;
	long	pos0;
	long	pos1;
	long	last;
	char	site[256];
	char	desc[1024];
	long	desc_p;
	char	on_off;
	long	i;
				
	pos = 0;
	
	pos = find(buffer, "<dl>", pos, size);

	while(1) {
		pos = find(buffer,"<dt>", pos, size);
		if (pos < 0)
			return;
		pos0 = find(buffer, " href=\"", pos, size);
		pos1 = find(buffer, "\">", pos0, size);
		pos0 += 6;
		memcpy(site, buffer+pos0, pos1-pos0);
		site[pos1-pos0] = 0;
		desc_p = 0;
		pos = pos1;
		last = find(buffer, "<br><", pos, size);
		
		pos1 = pos1 + 2;
		
		on_off = 0;
		for (i = pos1; i < last; i++) {
			if (buffer[i] == '<') {
				on_off++;
			}
			else
			if (buffer[i] == '>') {
				on_off--;
				desc[desc_p] = ' ';
				desc_p++;
			}
			else
			if (on_off == 0) {
				desc[desc_p] = buffer[i];
				desc_p++;
			}
		}
		desc[desc_p] = 0;
		
		if (match_list[cur_pos] != 0) {
			free((char *)match_list[cur_pos]);
		}
		if (desc_list[cur_pos] != 0) {
			free((char *)desc_list[cur_pos]);
		}
		
		match_list[cur_pos] = (char *)malloc(strlen(site) + 1);
		memcpy(match_list[cur_pos], site, strlen(site) + 1);
		desc_list[cur_pos] = (char *)malloc(strlen(desc) + 1);
		memcpy(desc_list[cur_pos], desc, strlen(desc) + 1);
		
		
		cur_pos++;
	}
}

//-------------------------------------------------------------------------

char	*av_find::get_ind_match(long i)
{
	return(match_list[i]);
}

//-------------------------------------------------------------------------

char	*av_find::get_ind_desc(long i)
{
	return(desc_list[i]);
}

//-------------------------------------------------------------------------

long	av_find::match_count()
{
	return cur_pos;
}

//-------------------------------------------------------------------------

int	av_find::doit(char *str)
{
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	search_path[512];
	char	first = 1;
	long	i;
	long	msocket;

	buffer = (char *)malloc(MAX_BUF);
	
	for (i = 0; i < 1; i++) {
		msocket = Connect("www.altavista.digital.com");
		pb = buffer;
		if (first) {
			first = 0;
			sprintf(search_path, "/cgi-bin/query?pg=q&what=web&kl=XX&q=%s&search.x=18&search.y=4", str);
		}
		else {
			sprintf(search_path, "/cgi-bin/query?pg=q&stq=%ld&what=web&kl=XX&q=%s&next.x=15&next.y=6", i*10, str);
		}
		Request(msocket, "www.altavista.digital.com", search_path);
		
		do {
			size = FillBuffer(msocket, pb);
			total += size;
			if (total > (MAX_BUF-4096)) break;
			pb += size;
		} while(size > 0);
		
		closesocket(msocket);

		process(buffer, total);
	}
	
	free((char *)buffer);
}

//-------------------------------------------------------------------------

void	hb_find::process(char *buffer, long size)
{
	long	pos;
	long	pos1;
	long	pos2;
	long	last;
	char	site[256];
	char	desc[1024];
	long	desc_p;
	char	on_off;
	long	i;
	char	buf[32];
	char	no_desc;
			
			
	pos = 0;
	
	pos = find(buffer, "verdana", pos, size);			//font name close to the start of the good stuff
	//printf("pos = %ld\n", pos);
	
	while(1) {
		pos = find(buffer,"</td>", pos, size);
		if (pos < 0)
			return;
			
		sprintf(buf, "<b>%ld", cur_pos+1);
		//printf("ms = %s\n", buf);
		pos = find(buffer, buf, pos, size);
		if (pos < 0)
			return;
		
		pos = find(buffer, "a href=\"", pos, size);
		if (pos < 0)
			return;
			
		pos2 = find(buffer, ">", pos, size);
		pos1 = find(buffer, "target=\"", pos, size);
		if (pos1 < 0) 
			return;
			
		if (pos2 < pos1) {
			pos1 = pos2;
			no_desc = 1;
		}
		else	
			no_desc = 0;
		
		pos += 7;
		pos1 -= 2;
		memcpy(site, buffer+pos, pos1-pos);
		site[pos1-pos] = 0;
		
		if (no_desc) {
			desc[0] = 0;
			pos += 4;
			goto skip;
		}
		pos = find(buffer, "</font></td>", pos, size);
		pos = find(buffer, "<td >", pos, size);
		pos1 = find(buffer, "<br>", pos+1, size);
		
		memcpy(desc, buffer + pos + 5, pos1-pos - 5);
		desc[pos1-pos-5] = 0;
		
skip:;

		if (match_list[cur_pos] != 0) {
			free((char *)match_list[cur_pos]);
		}
		if (desc_list[cur_pos] != 0) {
			free((char *)desc_list[cur_pos]);
		}
		
		match_list[cur_pos] = (char *)malloc(strlen(site) + 1);
		memcpy(match_list[cur_pos], site, strlen(site) + 1);
		desc_list[cur_pos] = (char *)malloc(strlen(desc) + 1);
		memcpy(desc_list[cur_pos], desc, strlen(desc) + 1);
		
		cur_pos++;
	}
}

//-------------------------------------------------------------------------

int	hb_find::doit(char *str)
{
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	search_path[512];
	char	first = 1;
	long	i;
	long	pos;
	long	pos1;
	long	msocket;
	char	cookie[256];
	
	buffer = (char *)malloc(MAX_BUF);
	
	msocket = Connect("www.search.hotbot.com");
	pb = buffer;
	//sprintf(search_path, "/hResult.html?SM=MC&MT=%s&SW=web&AM0=MC&AT0=words&AW0=&AM1=MN&AT1=words&AW1=&date=whenever&DV=7&DR=newer&DM=1&DD=1&DY=97&RD=AN&domain=&RG=.com&FS=&PS=A&PD=&DC=20&DE=2&OPs=MDRTP&_v=2&DU=days&search.x=13&search.y=6", str);
	sprintf(search_path, "/hResult.html?SM=MC&MT=%s&DV=7&RG=.com&DC=20&DE=2&OPs=MDRTP&_v=2&DU=days&SW=web&search.x=24&search.y=5", str);
	Request(msocket, "www.search.hotbot.com", search_path);
	printf("sp = %s\n", search_path);
	do {
		size = FillBuffer(msocket, pb);
		total += size;
		pb += size;
	} while(size > 0);
		
	closesocket(msocket);

// get the cooookie

	pos = find(buffer, "ink=", 0, total);
	if (pos < 0)
		goto failed;
		
	pos1 = find(buffer, ";", pos, total);
	if (pos1 < 0)
		goto failed;
	
	memcpy(cookie, buffer + pos + 4, pos1-pos-4);
	cookie[pos1-pos-4] = 0;

	//printf("cookie is %s\n", cookie);
	
	msocket = Connect("www.search.hotbot.com");
	pb = buffer;
	sprintf(search_path, "/%s/hResult.html?SM=MC&MT=%s&SW=web&AM0=MC&AT0=words&AW0=&AM1=MN&AT1=words&AW1=&date=whenever&DV=7&DR=newer&DM=1&DD=1&DY=97&RD=AN&domain=&RG=.com&FS=&PS=A&PD=&DC=20&DE=2&OPs=MDRTP&_v=2&DU=days&search.x=13&search.y=6", cookie, str);
	Request(msocket, "www.search.hotbot.com", search_path);
		
	do {
		size = FillBuffer(msocket, pb);
		total += size;
		if (total > (MAX_BUF-4096)) break;
		pb += size;
	} while(size > 0);
		

	process(buffer, total);
	closesocket(msocket);
	
failed:
	free((char *)buffer);
}

//-------------------------------------------------------------------------

long	gif_getter::get_gif_size()
{
	long	pos;
	long	file;

	pos = find(fbuffer, "gif87", 0, total);

	file = open("/boot/db", O_RDWR);
	write(file, fbuffer, 1000);
	close(file);
	printf("size = %ld\n", total-pos);

	return total - pos;
}

//-------------------------------------------------------------------------

CGIF	*gif_getter::get_data()
{
	long	ref;
	ushort	*p;
	long	cnt;
	long	pos;
	CGIF	*a_gif;
	long	result;
	char	*p1;
	char	c;
	long	sz;

	pos = find(fbuffer, "length:", 0, total);

	if (pos >= 0) {
		pos += 8;
		p1 =  fbuffer + pos;	

		sz = 0;
		while(c = *p1++) {
			if (c < '0')
				goto out;
			if (c > '9')
				goto out;
			sz *= 10;
			sz += (c - '0');
		}	
		out:;
	}	

	pos = find(fbuffer, "gif87", 0, total);
	

	printf("sz=%ld, tot1=%ld\n", sz, total-pos);

	if (sz != (total-pos))
		return 0;
		
	if (pos >= 0) {
		a_gif = new CGIF();
		do {
			result = a_gif->Write((uchar *)fbuffer + pos, total - pos);
			if (result > 0)
				pos += result;
		} while(result > 0);
	}
	return a_gif;
}

//-------------------------------------------------------------------------

int		site_getter::doit0()
{
	char	*pb;
	long	size;
	char	first = 1;
	long	i;
	char	site[256];
	char	path[256];
	
	total = 0;
	parse(full_path, site, path);

	if (fbuffer == 0)
		fbuffer = (char *)malloc(MAX_BUF);
	
again:;

	if (msocket < 0) {
		msocket = Connect(site);
		if (msocket<0) {
			printf("no socket for %s\n", full_path);
			closesocket(msocket);
			release_sem(done_sem);
			done = 1;
			return;
		}
	}
	pb = fbuffer;
	Request(msocket, site, path);
	
// Not really certain about what follows, but it seems to help !
	
	do {
		size = FillBuffer(msocket, pb);
		if (size == 0)
			snooze(32000);
			
		if (size == -1)
			goto out;
		total += size;
		if (total > (MAX_BUF-4096)) break;
		pb += size;
	} while((total == 0) || (size != 0));

out:;


	printf("close socket\n");
	size = FillBuffer(msocket, pb);
	printf("size again=%ld\n", size);
	closesocket(msocket);
	closesocket(msocket);
	closesocket(msocket);
	closesocket(msocket);
	closesocket(msocket);
	msocket = -1;

	if (total == 0) {
		printf("retry\n");
		goto again;
	}
		
	
	release_sem(done_sem);
	done = 1;
}

//-------------------------------------------------------------------------

long		init_p(void *p)
{
	site_getter	*g;
	
	g = (site_getter *)p;
	
	g->doit0();
	return 0;
}

//-------------------------------------------------------------------------

int		site_getter::doit(char *site_path)
{
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	first = 1;
	long	i;
	char	site[256];
	char	path[256];
	
	done = 0;
	strcpy(full_path, site_path);
	resume_thread(spawn_thread(init_p,site_path,B_NORMAL_PRIORITY,this));
}

//-------------------------------------------------------------------------

long	site_getter::get_total_bytes()
{
	return total;
}

//-------------------------------------------------------------------------

char	*site_getter::output_text(long *ocnt)
{
	long	i;
	long	cnt = 0;
	char	*out_buffer;
	char	*p;
	long	out_cnt;
	long	pos;
	
	out_buffer = (char *)malloc(32000);
	p = out_buffer;
	out_cnt = 0;
	pos = find(fbuffer, "<html", 0, total);
	if (pos == -1) {
		//printf("not found\n");
		pos = find(fbuffer, "<title", 0, total);
		if (pos == -1) {
			pos = find(fbuffer, "<br", 0, total);
		}
	}

	for (i = pos; i < total; i++) {
		if (fbuffer[i] == '<') {
			cnt++;
			goto skip;
		}
		if (fbuffer[i] == '>') {
			cnt--;
			goto skip;
		}
		if (cnt == 0) {
			*p++ = fbuffer[i];
			out_cnt++;
			if (out_cnt == 31998)
				goto out;
		}
skip:;
	}
out:;
	*ocnt = out_cnt;
	return out_buffer; 
}

//-------------------------------------------------------------------------

	phone_find::phone_find()
	: finder()
{
	long	i;

	for (i = 0; i < 128; i++) {
		match_list[i] = 0;
	}
	match_count = 0;
}

//-------------------------------------------------------------------------

	phone_find::~phone_find()
{
	long	i;
	
	for (i = 0; i < 128; i++) {
		if (match_list[i])
			free((char *)match_list[i]);
	}
}

//-------------------------------------------------------------------------

void	phone_find::process(char *buffer, long size)
{
	long	i;
	long	pos;
	long	pos1;
	long	back;
	char	no_address;
	char	last_name[64];
	char	first_name[64];
	char	city[64];
	char	state[32];
	char	zip[32];
	char	phone[32];
	char	address[64];
	
	pos = 0;
	
	//for (i = 0; i < size; i++)
	//	printf("%c", buffer[i]);
	
	pos = find(buffer, "directions", pos, size);
	if (pos < 0) {
		goto done;
	}
	//printf("x1\n");
	
	do {	
		pos = find(buffer, "&qlastname=", pos, size);
		if (pos < 0)
			goto done;
		//printf("x2\n");
		pos1 = find(buffer, "&q", pos+1, size);
		if (pos < 0)
			goto done;
		//printf("x3\n");
		pos += 11;
		memcpy(last_name, buffer+pos, pos1-pos);
		last_name[pos1-pos] = 0;
		//printf("x4\n");
		
		
		pos = find(buffer, "&qfirstname=", pos, size);
		pos1 = find(buffer, "&q", pos+1, size);
		//printf("x5\n");
		if (pos < 0)
			goto done;
		pos += 12;
		//printf("x6\n");
		memcpy(first_name, buffer+pos, pos1-pos);
		first_name[pos1-pos] = 0;
		//printf("x7\n");
		
		
		back = pos;
		address[0] = 0;
		
		pos = find(buffer, "&qaddress=", pos, size);
		//printf("pos here = %ld\n", pos);
		//printf("pos-back = %ld\n", pos-back);
		if (pos < 0 || ((pos-back)>100)) {
			pos = back;
			//printf("do skip\n");
			goto skip;
		}
		//printf("x8\n");
	
		pos1 = find(buffer, "&q", pos+1, size);
		if (pos < 0)
			goto done;
		
		pos += 10;
		memcpy(address, buffer+pos, pos1-pos);
		address[pos1-pos] = 0;
			
skip:;
		//printf("x9\n");
		
		pos = find(buffer, "&qcity=", pos, size);
		//printf("find 4 city=%ld\n", pos);
		pos1 = find(buffer, "&q", pos+1, size);
		if (pos < 0)
			goto done;
		pos += 7;
		memcpy(city, buffer+pos, pos1-pos);
		city[pos1-pos] = 0;
		//printf("city = <%s>\n", city);
		//printf("x10\n");
		
		pos = find(buffer, "&qstate=", pos, size);
		if (pos < 0)
			goto done;
		pos += 8;
		memcpy(state, buffer+pos, 2);
		state[2] = 0;
//		printf("state = <%s>\n", state);
//		printf("x11\n");
		
		
		pos = find(buffer, "&qzip=", pos, size);
		if (pos < 0)
			goto done;
		
		pos += 6;
		memcpy(zip, buffer+pos, 5);
		zip[5] = 0;
		//printf("x12\n");
	
	
		pos = find(buffer, "phone: ", pos, size);
		if (pos < 0)
			goto done;
		pos += 7;
		//printf("x13\n");
		
		memcpy(phone, buffer+pos, 13);
		phone[13] = 0;

/*
		printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n\n", last_name,
										first_name,
										city,
										state,
										zip,
										phone, address);

*/


		remove_pluses(last_name);
		remove_pluses(first_name);
		remove_pluses(city);
		remove_pluses(zip);
		remove_pluses(phone);
		remove_pluses(address);
		remove_pluses(state);
	
		add_full_person(first_name, last_name, city, zip, phone, address, state);
			
	} while(1);
	
	
done:;
}

//-------------------------------------------------------------------------

long phone_find::person_count()
{
	return match_count;
}

//-------------------------------------------------------------------------

full_person	*phone_find::get_ind_person(long i)
{
	return match_list[i];
}

//-------------------------------------------------------------------------


void phone_find::add_full_person(char *first_name,
								 char *last_name,
								 char *city,
								 char *zip,
								 char *phone,
								 char *address,
								 char *state)
{
	full_person	*p;
	long		i;
	
	for (i = 0; i < match_count; i++) {
		if (strcmp(phone, match_list[i]->phone) == 0)
			return;
	}

	p = (full_person *)malloc(sizeof(full_person));
	
	strcpy(p->first_name, first_name);
	strcpy(p->last_name, last_name);
	strcpy(p->city, city);
	strcpy(p->zip, zip);
	strcpy(p->phone, phone);
	strcpy(p->address, address);
	strcpy(p->state, state);

	match_list[match_count] = p;
	match_count++;
}

//-------------------------------------------------------------------------

int	phone_find::doit(char *first_name, char *last_name)
{
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	search_path[512];
	char	first = 1;
	long	i;
	long	msocket;
	long	msocket1;
	
	buffer = (char *)malloc(MAX_BUF);
	
	
	
	match_count = 0;
	
	msocket = Connect("www3.switchboard.com");
	msocket1 = Connect("www3.switchboard.com");
	pb = buffer;
	sprintf(search_path, "/bin/cgiqa.dll?F=%s&L=%s&T=&S=%s&SR=&MEM=212", first_name, last_name, "CA");
	Request(msocket, "www3.switchboard.com", search_path);

	sprintf(search_path, "/bin/cgiqa.dll?F=%s&L=%s&T=&S=%s&SR=&MEM=212", first_name, last_name, "");
	Request(msocket1, "www3.switchboard.com", search_path);
	
	do {
		size = FillBuffer(msocket, pb);
		total += size;
		if (total > (MAX_BUF-4096)) break;
		pb += size;
	} while(size > 0);
	closesocket(msocket);
	process(buffer, total);
	
	total = 0;
	
	pb = buffer;
	
	do {
		size = FillBuffer(msocket1, pb);
		total += size;
		if (total > (MAX_BUF-4096)) break;
		pb += size;
	} while(size > 0);
	
	closesocket(msocket1);
	process(buffer, total);
	
	free((char *)buffer);
}

//-------------------------------------------------------------------------


	email_find::email_find()
	: finder()
{
	long	i;

	for (i = 0; i < 128; i++) {
		match_list[i] = 0;
	}
	match_count = 0;
}

//-------------------------------------------------------------------------

	email_find::~email_find()
{
	long	i;
	
	for (i = 0; i < 128; i++) {
		if (match_list[i])
			free((char *)match_list[i]);
	}
}

//-------------------------------------------------------------------------

void	email_find::process(char *buffer, long size)
{
	long	i;
	long	pos;
	long	pos1;
	char	last_name[64];
	char	first_name[64];
	char	email[256];
		
	pos = 0;
	
	pos = find(buffer, "short results", pos, size);
	if (pos < 0) {
		goto done;
	}

	
	do {	
		pos = find(buffer, "&qlastname=", pos, size);
		pos1 = find(buffer, "&q", pos + 1, size);
		if (pos < 0)
			goto done;
			
		pos += 11;
		memcpy(last_name, buffer+pos, pos1-pos);
		last_name[pos1-pos] = 0;

		pos = find(buffer, "&qfirstname=", pos, size);
		pos1 = find(buffer, "&q", pos + 1, size);
		if (pos < 0)
			goto done;
		
		pos = pos + 12;
		memcpy(first_name, buffer+pos, pos1-pos);
		first_name[pos1-pos] = 0;
		
		
		pos = find(buffer, "&qemail=", pos, size);
		pos1 = find(buffer, ">", pos + 1, size);
		if (pos < 0)
			goto done;
		
		pos += 8;
		pos1--; 	
		memcpy(email, buffer+pos, pos1-pos);
		email[pos1-pos] = 0;
		
		pos = pos1 + 8;
	
		//printf("%s %s %s\n", first_name, last_name, email);


		remove_pluses(last_name);
		remove_pluses(first_name);
		remove_pluses(email);
	
		add_email(first_name, last_name, email);
			
	} while(1);
	
	
done:;
}

//-------------------------------------------------------------------------

long email_find::email_count()
{
	return match_count;
}

//-------------------------------------------------------------------------

full_email	*email_find::get_ind_email(long i)
{
	return match_list[i];
}

//-------------------------------------------------------------------------


void email_find::add_email		(char *first_name,
								 char *last_name,
								 char *email)
	{
	full_email	*p;
	
	p = (full_email *)malloc(sizeof(full_email));
	
	strcpy(p->first_name, first_name);
	strcpy(p->last_name, last_name);
	strcpy(p->email, email);
	match_list[match_count] = p;
	match_count++;
}

//-------------------------------------------------------------------------

int	email_find::doit(char *first_name, char *last_name)
{
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	search_path[512];
	char	first = 1;
	long	i;
	long	msocket;
	
	buffer = (char *)malloc(MAX_BUF);
	
	match_count = 0;
	
	for (i = 0; i < 1; i++) {
		msocket = Connect("www3.switchboard.com");
		pb = buffer;
		sprintf(search_path, "/bin/cgiemail.dll?F=%s&L=%s&SR=&MEM=1", first_name, last_name);
		Request(msocket, "www3.switchboard.com", search_path);
		
		do {
			size = FillBuffer(msocket, pb);
			total += size;
			if (total > (MAX_BUF-4096)) break;
			pb += size;
		} while(size > 0);
		printf("fe got %ld bytes\n", total);
		closesocket(msocket);
		process(buffer, total);
	}
	
	free((char *)buffer);
}

//-------------------------------------------------------------------------


	usenet_find::usenet_find()
	: finder()
{
	long	i;

	for (i = 0; i < 128; i++) {
		match_list[i] = 0;
	}
	match_count = 0;
}

//-------------------------------------------------------------------------

	usenet_find::~usenet_find()
{
	long	i;
	
	for (i = 0; i < 128; i++) {
		if (match_list[i])
			free((char *)match_list[i]);
	}
}

//-------------------------------------------------------------------------

void	usenet_find::process(char *buffer, long size)
{
	long	i;
	long	pos;
	long	pos1;
	char	newsgroup[128];
	char	extra[64];
	char	buf[64];
		
	pos = 0;
	
//	for (i = 0; i < size; i++)
//		printf("%c", buffer[i]);
	

	
	i = 0;
	
	do {	
		i++;
		sprintf(buf, "<dt><font size=-1><b>%ld.", i);
		pos = find(buffer, buf, pos, size);
		if (pos < 0)
			goto done;
			
		pos = find(buffer, "</strong></a><dd>", pos, size);
		if (pos < 0)
			goto done;
			
		pos = find(buffer, "<b>", pos, size);
		if (pos < 0)
			goto done;
			
		pos1 = find(buffer, "</b>", pos + 1, size);
		if (pos1 < 0)
			goto done;
	
		memcpy(newsgroup, buffer + pos + 3, pos1 - pos - 3);
		newsgroup[pos1 - pos - 3] = 0;
		remove_pluses(newsgroup);
//		printf("got newsgroup %s\n", newsgroup);
		
		add_usenet(newsgroup, "");
	} while(1);
	
	
done:;
}

//-------------------------------------------------------------------------

long usenet_find::newsgroup_count()
{
	return match_count;
}

//-------------------------------------------------------------------------

usenet_match	*usenet_find::get_ind_newsgroup(long i)
{
	return match_list[i];
}

//-------------------------------------------------------------------------


void usenet_find::add_usenet	(char *newsgroup,
								 char *extra)
{
	usenet_match	*p;
	long			i;
	
	
	for (i = 0; i < match_count; i++) {
		if (strcmp(newsgroup, match_list[i]->newsgroup) == 0)
			return;
	}
	
	p = (usenet_match *)malloc(sizeof(usenet_match));
	
	strcpy(p->newsgroup, newsgroup);
	strcpy(p->extra, extra);
	match_list[match_count] = p;
	match_count++;
}

//-------------------------------------------------------------------------
//http://www.altavista.digital.com/cgi-bin/query?pg=q&ufmt=d&what=news&kl=XX&q="Eric+Knight"&search.x=36&search.y=7

int	usenet_find::doit(char *string)
{
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	search_path[512];
	char	first = 1;
	long	i;
	long	msocket;
	
	buffer = (char *)malloc(MAX_BUF);
	
	match_count = 0;
	
	for (i = 0; i < 1; i++) {
		msocket = Connect("www.altavista.digital.com");
		pb = buffer;
		sprintf(search_path,
		"/cgi-bin/query?pg=q&ufmt=d&what=news&kl=XX&q=%s&search.x=36&search.y=7", string);
		Request(msocket, "www.altavista.digital.com", search_path);
		
		do {
			size = FillBuffer(msocket, pb);
			total += size;
			if (total > (MAX_BUF-4096)) break;
			pb += size;
		} while(size > 0);
		
		printf("usetet find %ld\n", total);
		
		closesocket(msocket);
		process(buffer, total);
	}
	
	free((char *)buffer);
}

//-------------------------------------------------------------------------

	map_getter::map_getter()
	: finder()
{
}

//-------------------------------------------------------------------------

	map_getter::~map_getter()
{
}


//-------------------------------------------------------------------------


BBitmap	*map_getter::get_map()
{
	return bits;
}


//-------------------------------------------------------------------------

int	map_getter::doit(char *address, char *city, char *state, char *zip)
{
	char	search_path[512];
	char	new_path[512];
	char	*buffer;
	char	*pb;
	long	size;
	long	total = 0;
	double	start, end;
	char	first = 1;
	long	i;
	long	pos;
	long	pos1;
	BRect	bnd;
	long	result;
	long	msocket;
	
	bits = 0;
	buffer = (char *)malloc(MAX_BUF);
	
	msocket = Connect("maps.yahoo.com");
	pb = buffer;
	sprintf(search_path,
	"/yahoo/yt.hm?FAM=yahoo&CMD=GEO&SEC=geo&AD2=%s&AD3=%s+%s+%s", address, city, state, zip);
	
	set_pluses(search_path);
	
	Request(msocket, "maps.yahoo.com", search_path);
	
	do {
		size = FillBuffer(msocket, pb);
		total += size;
		if (total > (MAX_BUF-4096)) break;
		pb += size;
	} while(size > 0);
	
	printf("got from map %ld\n", total);
	
	pos = find(buffer, "/gif?&ct", 0, total);
	if (pos < 0)
		goto out;
	
	pos1 = find(buffer, ">", pos, total);
	if (pos1 < 0)
		goto out;	
		
	memcpy(new_path, buffer+pos, pos1 - pos);
	new_path[pos1-pos] = 0;
	
	for (i = 0; i < strlen(new_path); i++)
		if (new_path[i] < 32) new_path[i] = 32;
	
	pos = 0;
	
	do {	
		pos = find(new_path, "=400", pos, strlen(new_path));
		if (pos > 0) {
			new_path[pos+1] = '6';
			pos++;
		}
	} while (pos > 0);
		
	pos = 0;
	
	do {	
		pos = find(new_path, "=250", pos, strlen(new_path));
		if (pos > 0) {
			new_path[pos+1] = '5';
			pos++;
		}
	} while (pos > 0);
	//printf("new path = %s\n", new_path);
	
	closesocket(msocket);
	
	msocket = Connect("maps.yahoo.com");
	Request(msocket, "maps.yahoo.com", new_path);
	
	total = 0;
	pb = buffer;
	
	do {
		size = FillBuffer(msocket, pb);
		total += size;
		if (total > (MAX_BUF-4096))
			break;
		pb += size;
	} while(size > 0);

		
	pos = find(buffer, "gif87", 0, total);
	
	if (pos >= 0) {
		a_gif = new CGIF();
		do {
			result = a_gif->Write((uchar *)buffer + pos, total - pos);
			if (result > 0)
				pos += result;
		} while(result > 0);
		bits = a_gif->pixels;
	}
	
	
out:;

	closesocket(msocket);
	
	free((char *)buffer);
}

//-------------------------------------------------------------------------

int finder::Request(long msocket, char *usrstring, char *cpath)
{
	static	char *HTTPVERS = 		" HTTP/1.0\r\n";
	static	char *USER_AGENT =		"User-Agent: Mozilla/2.0 (compatible; NetPositive; BeOS)\r\n";
	static	char *ACCCEPT =			"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n";
	static 	char *HOST = 			"Host: ";

	CString	req;
	CString path;
	
	if (path.Length() == 0)
		path = cpath;		//was /

//	GET or POST a request

	req = "GET ";
		
	req += path;
	req += HTTPVERS;			// HTTP/1.0
	req += USER_AGENT;			// User-Agent: Mozilla/NetPositive0.75
	req += ACCCEPT;				// Accept: text/html, image/jpeg, image/gif
	req += HOST;
	//req += mURL.HostName();
	req += usrstring;
	req += "\r\n";
	req += "\r\n";
	req += "\r\n";				// ??

	//printf("req=%s\n", req);
	long result = NET_NO_ERROR;
	printf("socket=%ld\n", msocket);
	result = send(msocket, req, req.Length(), 0);
	if (result < 0) {
		printf("can't send\n");
		printf("%s\n", 
		NETRESULTSTR((NetResult)result));
		result = NET_CANT_SEND;
	}
	return result;
}
