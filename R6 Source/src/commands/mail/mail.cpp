/*
 * Copyright (C) 1997 Be, Inc.
 * All Rights Reserved
 */

/*
 * Mail -- a clone of the BSD command-line mail program
 *   Only supports sending, not receiving mail.
 */
#include <Application.h>
#include <Entry.h>
#include <E-mail.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNKSIZE BUFSIZ  /* Number of bytes to allocate at once */
#define LINESIZE  BUFSIZ  /* Maximum line length */

void help(char *name) {
  fprintf(stderr, "usage: %s [-v] [-s subject] [-c cc-addr] [-b bcc-addr] to-addr ...\n", name);
}

int main(int argc, char **argv) {
  BApplication myapp("application/x-vnc.be-jhmail");   // needed to send mail
  BMailMessage msg;	// create message
  bool verbose = false, recipients = false;

  // check args and add header fields
  // print help text if bad
  int c;
  while((c = getopt(argc, argv, "dvs:c:b:h")) != -1) {
	switch (c) {
	case 'd':
	    break;	// undocumented: ignore for compatibility
	case 'v':
	    verbose = true;
	    break;
	case 's':
	    msg.AddHeaderField(B_MAIL_SUBJECT, optarg);
	    break;
	case 'c':
	    msg.AddHeaderField(B_MAIL_CC, optarg);
	    break;
	case 'b':
	    msg.AddHeaderField(B_MAIL_BCC, optarg);
	    recipients = true;
	    break;
	case 'h':
	default:
		help(argv[0]);	// print help with program name
		return 1;
	}
  }

  // extra args
  for (; optind < argc; optind++) {
    msg.AddHeaderField(B_MAIL_TO, argv[optind]);
    recipients = true;
  }

  // complain if no recipients
  if (!recipients) {
    fprintf(stderr, "Sorry: This program can only send mail, not read it.\n");
    help(argv[0]);
    return 1;
  }

  // allocate body in 4k chunks (realloc if needed)
  char linebuf[LINESIZE];					// line buffer
  char *body = (char *)malloc(CHUNKSIZE);	// msg body
  int bodylen=0, bytesleft=CHUNKSIZE;
  bool headers=true;
  // add each line until "." or CTRL-D
  for(;;) {
	  char *ret = fgets(linebuf, LINESIZE, stdin);
	  if(!ret || !strcmp(linebuf, ".\n"))	/* EOF or "." */
	  	break;
	  
	  // This is a bit tricky:  some programs may type header lines, like
	  // From:, To:, Subject:, X-Cron-Env:, etc. here.  We need to make
	  // sure they get added as Header Fields and not part of the Body
	  if (headers) {
		char *space = strchr(linebuf, ' ');
		if(!space || space==linebuf || *(space-1) != ':')
		    headers = false;  // doesn't start with "xxx: "

		if(headers) {
			// make copy of field name
			char *field_name = (char*)malloc(space-linebuf+2);
			memcpy(field_name, linebuf, space-linebuf+1);
			field_name[space-linebuf+1] = '\0';
			char *str = space + 1;
			str[strlen(str)-1] = '\0';  // strip off newline
		   	msg.AddHeaderField(field_name, str);
			free(field_name);
			continue;	// get next line
		}
	  }

	  int linelen = strlen(linebuf);
	  if (linelen > bytesleft) {
	  	body = (char *)realloc(body, bodylen + bytesleft + CHUNKSIZE);
	  	if (!body) {
	  		fprintf(stderr, "Out of memory!\n");
	  		return 1;
	  	}
	  	bytesleft += CHUNKSIZE;
	  }
	  memcpy(&body[bodylen], linebuf, linelen);
	  bodylen += linelen;
	  bytesleft -= linelen;
  }

  // add body to message
  printf("EOT\n");
  msg.AddContent(body, bodylen);

  // send it, and report status
  switch(msg.Send(true, true)) {
  case B_NO_ERROR:
	if(verbose)
	    fprintf(stderr, "sent successfully!\n");
	break;
  case B_MAIL_NO_RECIPIENT:
	fprintf(stderr, "Error: No recipients!\n");
	return 1;
  case B_MAIL_NO_DAEMON:
	fprintf(stderr, "Error: mail_daemon not running\n");
	return 1;
  case B_MAIL_UNKNOWN_USER:
	fprintf(stderr, "Error: unknown user\n");
	return 1;
  case B_MAIL_WRONG_PASSWORD:
	fprintf(stderr, "Error: wrong password\n");
	return 1;
  case B_MAIL_UNKNOWN_HOST:
	fprintf(stderr, "Error: unknown SMTP host\n");
	return 1;
  case B_MAIL_ACCESS_ERROR:
	fprintf(stderr, "Error: can't access server\n");
	return 1;
  default:
	fprintf(stderr, "Unknown error: %d\n");
	return 1;
  }
  return 0;
}
