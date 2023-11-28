/* Copyright (C) 1991, 93, 95, 96, 97, 98 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <libintl.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <stdio-common/_itoa.h>
#include <errno.h>
#include <MediaDefs.h>

/* Return a string describing the errno code in ERRNUM.  */
char *
__strerror_r (int errnum, char *buf, size_t buflen)
{
  switch (errnum) {
  case ENOERR:		
	  return (char *) ("No Error");                   
  case EACCES:
	  return (char *) ("Permission denied");
  case E2BIG:         
	  return (char *) ("Argument too big");
  case EBADF:         
	  return (char *) ("Bad file descriptor");        
  case EBUSY:         
	  return (char *) ("Device/File/Resource busy");  
  case ECHILD:        
	  return (char *) ("No child process");           
  case EDEADLK:       
	  return (char *) ("Resource deadlock");          
  case EEXIST:        
	  return (char *) ("File or Directory already exists");    
  case EFAULT:        
	  return (char *) ("Bad address");                
  case EFBIG:         
	  return (char *) ("File too large");             
  case EINVAL:        
	  return (char *) ("Invalid argument");           
  case EISDIR:        
	  return (char *) ("Is a directory");             
  case EMFILE:        
	  return (char *) ("Too many open files");        
  case EMLINK:        
	  return (char *) ("Too many links");             
  case ENAMETOOLONG:  
	  return (char *) ("File name too long");         
  case ENFILE:        
	  return (char *) ("File table overflow");        
  case ENODEV:        
	  return (char *) ("No such device");             
  case ENOENT:        
	  return (char *) ("No such file or directory");  
  case ENOEXEC:       
	  return (char *) ("Not an executable");          
  case ENOLCK:        
	  return (char *) ("No record locks available");  
  case ENOMEM:        
	  return (char *) ("No memory");                  
  case ENOSPC:        
	  return (char *) ("No space left on device");    
  case ENOSYS:        
	  return (char *) ("Function not implemented");   
  case ENOTDIR:       
	  return (char *) ("Not a directory");            
  case ENOTEMPTY:     
	  return (char *) ("Directory not empty");        
  case ELOOP:
	  return (char *) ("Too many symbolic links");        
  case ENOTTY:        
	  return (char *) ("Not a tty");                  
  case ENXIO:         
	  return (char *) ("No such device");             
  case EPERM:         
	  return (char *) ("Operation not allowed");      
  case EPIPE:         
	  return (char *) ("Broken pipe");                
  case EROFS:         
	  return (char *) ("Read-only file system");      
  case ESPIPE:        
	  return (char *) ("Seek not allowed on file descriptor"); 
  case ESRCH:         
	  return (char *) ("No such process");            
  case EXDEV:         
	  return (char *) ("Cross-device link");          
  case EFPOS:			
	  return (char *) ("File Position Error");        
  case ESIGPARM:	    
	  return (char *) ("Signal Error");               
  case EDOM:			
	  return (char *) ("Domain Error");               
  case ERANGE:		
	  return (char *) ("Range Error");                
  case EPROTOTYPE: 	
	  return (char *) ("Protocol wrong type for socket");		
  case EPROTONOSUPPORT:	
	  return (char *) ("Protocol not supported");			
  case EPFNOSUPPORT:	
	  return (char *) ("Protocol family not supported");		
  case EAFNOSUPPORT:	
	  return (char *) ("Address family not supported by protocol family"); 
  case EADDRINUSE: 	
	  return (char *) ("Address already in use"); 				
  case EADDRNOTAVAIL:	
	  return (char *) ("Can't assign requested address"); 		
  case ENETDOWN:		
	  return (char *) ("Network is down");						
  case ENETUNREACH:	
	  return (char *) ("Network is unreachable");				
  case ENETRESET: 	
	  return (char *) ("Network dropped connection on reset");	
  case ECONNABORTED:	
	  return (char *) ("Software caused connection abort");		
  case ECONNRESET:	
	  return (char *) ("Connection reset by peer");			
  case EISCONN: 		
	  return (char *) ("Socket is already connected");			
  case ENOTCONN:		
	  return (char *) ("Socket is not connected");				
  case ESHUTDOWN: 	
	  return (char *) ("Can't send after socket shutdown");	
  case ECONNREFUSED:	
	  return (char *) ("Connection refused");					
  case EHOSTUNREACH:	
	  return (char *) ("No route to host");					
  case ENOPROTOOPT:	
	  return (char *) ("Protocol option not available"); 		
  case EINPROGRESS:
	  return (char *) ("Operation now in progress");
  case EALREADY:
	  return (char *) ("Operation already in progress");
  case ENOBUFS:
	  return (char *) ("No buffer space available");
  case EILSEQ:
	  return (char *) ("Illegal byte sequence");
  case ENOMSG:
	  return (char *) ("No message of desired type");
  case EOVERFLOW:
	  return (char *) ("Value too large for defined type");
  case EMSGSIZE:
	  return (char *) ("Message too long");
  case EOPNOTSUPP:
	  return (char *) ("Operation not supported");

		
	  /*
	   * Be errors start here
	   */
  case B_IO_ERROR:
	  return (char *) ("I/O error");
  case B_FILE_NOT_FOUND:
	  return (char *) ("File not found");
#if 0 /* Be-mani same as EACCES */
  case B_PERMISSION_DENIED:
	  return (char *) ("Permission denied");
#endif		
  case B_BAD_INDEX:
	  return (char *) ("Index not in range for the data set");
  case B_BAD_TYPE:
	  return (char *) ("Bad argument type passed to function");
#if 0 /* same as EINVAL Be-mani */
  case B_BAD_VALUE:
	  return (char *) ("Bad value passed to function");
#endif		
  case B_MISMATCHED_VALUES:
	  return (char *) ("Mismatched values passed to function");
  case B_NAME_NOT_FOUND:
	  return (char *) ("Name not found");
  case B_NAME_IN_USE:
	  return (char *) ("Name in use");
  case B_TIMED_OUT:
	  return (char *) ("Operation timed out");
  case B_INTERRUPTED:
	  return (char *) ("Interrupted system call");
  case B_WOULD_BLOCK:
	  return (char *) ("Operation would block");
  case B_CANCELED:
	  return (char *) ("Operation canceled");
  case B_NO_INIT:
	  return (char *) ("Initialization failed");
  case B_ERROR:
	  return (char *) ("General OS error");
  case B_BAD_SEM_ID:
	  return (char *) ("Bad semaphore ID");
  case B_NO_MORE_SEMS:
	  return (char *) ("No more semaphores");
  case B_BAD_THREAD_ID:
	  return (char *) ("Bad thread ID");
  case B_NO_MORE_THREADS:
	  return (char *) ("No more threads");
  case B_BAD_THREAD_STATE:
	  return (char *) ("Thread is inappropriate state");
  case B_BAD_TEAM_ID:
	  return (char *) ("Operation on invalid team");
  case B_NO_MORE_TEAMS:
	  return (char *) ("No more teams");
  case B_BAD_PORT_ID:
	  return (char *) ("Bad port ID");
  case B_NO_MORE_PORTS:
	  return (char *) ("No more ports");
  case B_BAD_IMAGE_ID:
	  return (char *) ("Bad image ID");
  case B_MISSING_LIBRARY:
	  return (char *) ("Missing library");
  case B_MISSING_SYMBOL:
	  return (char *) ("Symbol not found");
  case B_DEBUGGER_ALREADY_INSTALLED:
	  return (char *) ("Debugger already installed for this team");
  case B_BAD_REPLY:
	  return (char *) ("Invalid or unwanted reply");
  case B_DUPLICATE_REPLY:
	  return (char *) ("Duplicate reply");
  case B_MESSAGE_TO_SELF:
	  return (char *) ("Can't send message to self");
  case B_BAD_HANDLER:
	  return (char *) ("Bad handler");
  case B_ALREADY_RUNNING:
	  return (char *) ("Already running");
  case B_LAUNCH_FAILED:
	  return (char *) ("Launch failed");
  case B_AMBIGUOUS_APP_LAUNCH:
	  return (char *) ("Ambiguous app launch");
  case B_UNKNOWN_MIME_TYPE:
	  return (char *) ("Unknown MIME type");
  case B_BAD_SCRIPT_SYNTAX:
	  return (char *) ("Bad script syntax");

/* Be-mani 980107 start */

  case B_LAUNCH_FAILED_NO_RESOLVE_LINK:
	  return (char *) ("Could not resolve a link");
  case B_LAUNCH_FAILED_EXECUTABLE:
	  return (char *) ("File is mistakenly marked as executable");
  case B_LAUNCH_FAILED_APP_NOT_FOUND:
	  return (char *) ("Application could not be found");
  case B_LAUNCH_FAILED_APP_IN_TRASH:
	  return (char *) ("Application is in the trash");
  case B_LAUNCH_FAILED_NO_PREFERRED_APP:
	  return (char *) ("There is no preferred application for this type of file");
  case B_LAUNCH_FAILED_FILES_APP_NOT_FOUND:
	  return (char *) ("This file has a preferred app, but it could not be found");
  case B_BAD_MIME_SNIFFER_RULE:
	  return (char *) ("Bad sniffer rule");

/* Be-mani 980107 end */

  case B_STREAM_NOT_FOUND:
	  return (char *) ("Stream not found");
  case B_SERVER_NOT_FOUND:
	  return (char *) ("Server not found");
  case B_RESOURCE_NOT_FOUND:
	  return (char *) ("Resource not found");
  case B_RESOURCE_UNAVAILABLE:
	  return (char *) ("Resource unavailable");
  case B_BAD_SUBSCRIBER:
	  return (char *) ("Bad subscriber");
  case B_SUBSCRIBER_NOT_ENTERED:
	  return (char *) ("Subscriber not entered");
  case B_BUFFER_NOT_AVAILABLE:
	  return (char *) ("Buffer not available");
  case B_LAST_BUFFER_ERROR:
	  return (char *) ("Last buffer");
  case B_MAIL_NO_DAEMON:
	  return (char *) ("No mail daemon");
  case B_MAIL_UNKNOWN_USER:
	  return (char *) ("Unknown mail user");
  case B_MAIL_WRONG_PASSWORD:
	  return (char *) ("Wrong password (mail)");
  case B_MAIL_UNKNOWN_HOST:
	  return (char *) ("Mail unknown host");
  case B_MAIL_ACCESS_ERROR:
	  return (char *) ("Mail access error");
  case B_MAIL_UNKNOWN_FIELD:
	  return (char *) ("Unknown mail field");
  case B_MAIL_NO_RECIPIENT:
	  return (char *) ("No mail recipient");
  case B_MAIL_INVALID_MAIL:
	  return (char *) ("Invaild mail");
  case B_DEV_INVALID_IOCTL:
	  return (char *) ("Invalid device ioctl");
  case B_DEV_NO_MEMORY:
	  return (char *) ("No device memory");
  case B_DEV_BAD_DRIVE_NUM:
	  return (char *) ("Bad drive number");
  case B_DEV_NO_MEDIA:
	  return (char *) ("No media present");
  case B_DEV_UNREADABLE:
	  return (char *) ("Device unreadable");
  case B_DEV_FORMAT_ERROR:
	  return (char *) ("Device format error");
  case B_DEV_TIMEOUT:
	  return (char *) ("Device timeout");
  case B_DEV_RECALIBRATE_ERROR:
	  return (char *) ("Device recalibrate error");
  case B_DEV_SEEK_ERROR:
	  return (char *) ("Device seek error");
  case B_DEV_ID_ERROR:
	  return (char *) ("Device ID error");
  case B_DEV_READ_ERROR:
	  return (char *) ("Device read error");
  case B_DEV_WRITE_ERROR:
	  return (char *) ("Device write error");
  case B_DEV_NOT_READY:
	  return (char *) ("Device not ready");
  case B_DEV_MEDIA_CHANGED:
	  return (char *) ("Device media changed");
  case B_DEV_MEDIA_CHANGE_REQUESTED:
	  return (char *) ("Device media change requested");
  case B_DEV_RESOURCE_CONFLICT:
      return (char *) ("Resource conflict");
  case B_DEV_CONFIGURATION_ERROR:
      return (char *) ("Configuration error");
  case B_DEV_DISABLED_BY_USER:
      return (char *) ("Disabled by user");
  case B_DEV_DOOR_OPEN:
      return (char *) ("Drive door open");
  case B_UNSUPPORTED:         
	  return (char *) ("Operation not supported");      
  case B_PARTITION_TOO_SMALL:
      return (char *) ("Partition too small to contain filesystem");
  case B_MEDIA_BAD_NODE:
      return (char *) ("Bad media node");
  case B_MEDIA_NODE_BUSY:
      return (char *) ("Media node busy");
  case B_MEDIA_BAD_FORMAT:
      return (char *) ("Bad media format");
  case B_MEDIA_BAD_BUFFER:
      return (char *) ("Bad buffer");
  case B_MEDIA_TOO_MANY_NODES:
      return (char *) ("Too many nodes");
  case B_MEDIA_TOO_MANY_BUFFERS:
      return (char *) ("Too many buffers");
  case B_MEDIA_NODE_ALREADY_EXISTS:
      return (char *) ("Media node already exists");
  case B_MEDIA_BUFFER_ALREADY_EXISTS:
      return (char *) ("Buffer already exists");
  case B_MEDIA_CANNOT_SEEK:
      return (char *) ("Cannot seek");
  case B_MEDIA_CANNOT_CHANGE_RUN_MODE:
      return (char *) ("Cannot change run mode");
  case B_MEDIA_APP_ALREADY_REGISTERED:
      return (char *) ("App already registered");
  case B_MEDIA_APP_NOT_REGISTERED:
      return (char *) ("App not registered");
  case B_MEDIA_CANNOT_RECLAIM_BUFFERS:
      return (char *) ("Cannot reclaim buffers");
  case B_MEDIA_BUFFERS_NOT_RECLAIMED:
      return (char *) ("Buffers not reclaimed");
  case B_MEDIA_TIME_SOURCE_STOPPED:
      return (char *) ("Time source stopped");
  case B_MEDIA_TIME_SOURCE_BUSY:				
      return (char *) ("Time source busy");
  case B_MEDIA_BAD_SOURCE:
      return (char *) ("Bad source");
  case B_MEDIA_BAD_DESTINATION:
      return (char *) ("Bad destination");
  case B_MEDIA_ALREADY_CONNECTED:
      return (char *) ("Already connected");
  case B_MEDIA_NOT_CONNECTED:
      return (char *) ("Not connected");
  case B_MEDIA_BAD_CLIP_FORMAT:
      return (char *) ("Bad clipping format");
  case B_MEDIA_ADDON_FAILED:
      return (char *) ("Media addon failed");
  case B_MEDIA_ADDON_DISABLED:
      return (char *) ("Media addon disabled");
  case B_MEDIA_CHANGE_IN_PROGRESS:
      return (char *) ("Change in progress");
  case B_MEDIA_STALE_CHANGE_COUNT:
      return (char *) ("Stale change count");
  case B_MEDIA_ADDON_RESTRICTED:
      return (char *) ("Media addon restricted");
  case B_MEDIA_NO_HANDLER: 
      return (char *) ("No handler");
  case B_MEDIA_DUPLICATE_FORMAT:
      return (char *) ("Duplicate format");
/* Printing errors */
  case B_NO_PRINT_SERVER:
      return (char *) ("No Print Server");
  case B_PAPER_JAM:
      return (char *) ("Paper jam");
  case B_NO_PAPER:
      return (char *) ("Paper out");
  case B_NO_INK:
      return (char *) ("Ink out");
  case B_NO_TRANSPORT:
      return (char *) ("Transport not found");
  case B_BAD_TRANSPORT:
      return (char *) ("Invalid transport");
  case B_TRANSPORT_INIT_ERROR:
      return (char *) ("Transport failed to initialize");
  case B_PRINTER_BUSY:
      return (char *) ("Printer already in use");
  case B_NO_PRINTER:
      return (char *) ("No printer selected");
  case B_INVALID_PRINT_SETTINGS:
      return (char *) ("Invalid printer settings");
  case B_INVALID_PRINTER:
      return (char *) ("Invalid printer folder");
  case B_NO_SPOOL_FILE:
      return (char *) ("Spool file not found");
  case B_BAD_SPOOL_FILE:
      return (char *) ("Spool file contains incorrect data");
  case B_NO_DRIVER:
      return (char *) ("Driver not found");
  case B_BAD_DRIVER:
      return (char *) ("Invalid printer driver");
  }

  {
	  static char errstr[64];
	  sprintf(errstr, "Unknown Error (%d)", errnum);
	  return errstr;
  }
}
weak_alias (__strerror_r, strerror_r)
