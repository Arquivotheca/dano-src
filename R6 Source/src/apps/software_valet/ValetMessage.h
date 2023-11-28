#ifndef _VALETMESSAGE_H_
#define _VALETMESSAGE_H_


// BMessage types for communication between Valet and Transceiver

enum {
	PKG_REGISTER =				'PReg',
	PKG_UPDATE =				'PUpd',
	PKG_DISPLAY =				'PDis',
	
	F_UPDATE_CONNECT = 			'UpCn',
	F_UPDATE_LOOKING = 			'UpLk',
	F_UPDATE_NONE = 			'UpNn',
	F_UPDATE_DOWNL_PROCEED = 	'UpDl',
	F_UPDATE_ERROR =			'UpEr',
	
	F_REG_CONNECT = 			'RgCn',
	F_REG_SENDING = 			'RgSn',
	F_REG_DONE = 				'RgDn',
	F_REG_ERROR =				'RgEr',
	F_REG_ALLDONE =				'RgAd'
};

#endif
