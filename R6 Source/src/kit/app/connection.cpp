/******************************************************************************
//
//	File:		connection.c
//
//	Description:	functions for connection control with the server.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	6/11/92		bgs	new today
//
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <OS.h>		/* ports */

#ifndef _CONNECTION_H
#include "connection.h"
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif

/*---------------------------------------------------------*/

long	create_server_port(const char *tag)
{
	char	buffer[32];
	sprintf(buffer, "a>%16.16s", tag ? tag : "");
	return(create_port(100,buffer));
}

/*---------------------------------------------------------*/

static long	init_connection(long my_port, char* serverName)
{
	long	pid;
	message	a_message;
	long	msgCode;

	pid = find_thread(serverName);
	a_message.what = HELLO;			
	a_message.parm1 = my_port;
	send_data(pid, 0, (char *) &a_message, sizeof(a_message));
	while (read_port(my_port, &msgCode, &a_message, 64) == B_INTERRUPTED)
		;
	return(a_message.parm1);
}

/*---------------------------------------------------------*/

void	get_server_port(long *server_from, long *server_to, char* serverName)
{
	long	server_port;
	long	my_port;

	my_port = create_server_port("fServerFrom");
	server_port = init_connection(my_port, serverName);
	*server_from = my_port;
	*server_to   = server_port;
}

/*---------------------------------------------------------*/

#if 0
// DKH -- what is this for?
static long	server_msg(long server_to, message *a_message)
{
	long err;
	
	while ((err = write_port(server_to, 0, &a_message, 64)) == B_INTERRUPTED)
		;

	return err;
}
#endif
