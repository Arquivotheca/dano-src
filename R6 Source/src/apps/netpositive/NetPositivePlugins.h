/*******************************************************************************
/
/	File:			NetPositivePlugins.h
/
/	Copyright 1999, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_NETPOSITIVEPLUGINS_H
#define	_NETPOSITIVEPLUGINS_H

typedef status_t (*netpositive_plugin_init_func) (const BMessage *browserInfo, BMessage *pluginInfo);
typedef void (*netpositive_plugin_term_func) ();

/*----------------------------------------------------------------*/
/*-----  message command constants -------------------------------*/

enum {
	/* NetPositive -> Plug-in messages                                      */
	B_NETPOSITIVE_GET_PLUGIN_INFO			= 'NPPI',
	B_NETPOSITIVE_GET_PLUGIN_PREFERENCES	= 'NPGP',
	B_NETPOSITIVE_SET_PLUGIN_PREFERENCES	= 'NPSP',
	
	/* Plug-in -> NetPositive messages                                      */
	B_NETPOSITIVE_TERMINATE_PLUGIN			= 'NPTP',
	
	/* NetPostive -> Instance messages                                      */
	B_NETPOSITIVE_INIT_INSTANCE				= 'NPII',
	B_NETPOSITIVE_TERMINATE_INSTANCE		= 'NPTI',
	B_NETPOSITIVE_APPROVE_URL				= 'NPAU',
	B_NETPOSITIVE_FILTER_HTML				= 'NPFH',
	
	/* Instance -> NetPositive messages                                     */
	B_NETPOSITIVE_STATUS_MESSAGE			= 'NPSM',
	B_NETPOSITIVE_SAVE_INSTANCE_DATA		= 'NPSD',


	/* Plug-in types                                                        */
	B_NETPOSITIVE_DATATYPE_PLUGIN			= 'NPDP',
	B_NETPOSITIVE_HTMLFILTER_PLUIGN			= 'NPHF',
	B_NETPOSITIVE_URLAPPROVAL_PLUGIN		= 'NPUA'
};
	
/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

#endif /* _NETPOSITIVEPLUGINS_H */
