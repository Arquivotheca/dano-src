/******************************************************************************
//
//	File:		connection.h
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

#ifndef	_CONNECTION_H
#define	_CONNECTION_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
long	create_server_port(const char *tag = NULL);
void	get_server_port(long *server_from, long *server_to, char* serverName);

#endif
