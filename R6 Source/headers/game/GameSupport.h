/******************************************************************************
 **
 **	File:		GameSupport.h
 **
 **	Description:	Client class for game settings.
 **
 **	Copyright 1996-97, Be Incorporated, All Rights Reserved.
 **
 ******************************************************************************/


#ifndef	_GAME_SUPPORT_H
#define	_GAME_SUPPORT_H

#include <Looper.h>
#include <GameInput.h>
#include <Database.h>

enum {
	B_GENERAL_CONTROL_COUNT = 4,

	B_EDIT_SETTING_APP = 'GIED',
};

class BGameSupport : public BLooper {
 public:
	BGameSupport(BGameInput *input);
	void SetApplication(ulong signature, ulong version);
	void SetDefaultSetting();
	void GetUserSetting();
	void EditSetting(ulong signature = B_EDIT_SETTING_APP);
	
 private:
	typedef struct {
		uchar              type;
		short              cal[6];
		uchar              keys[8];
		short              key_f[8];
		short              joy_f[8];
		game_input_control ctrl[B_GENERAL_CONTROL_COUNT];
	} general_set;
	long               my_version;
	ulong              my_signature;
	BDatabase          *my_db;
	BGameInput         *my_input;
	general_set        g_set[4];

	BMessage *SettingToMessage(BGameInput *set, bool minimal);
	long     MessageToSetting(BGameInput *set, BMessage *m);
	BMessage *GeneralToMessage();
	long     MessageToGeneral(BMessage *m);
	BTable   *GetPrefTable();
	BMessage *LoadPrefs(long type);
	void     SavePrefs(BMessage *m, long type);
	virtual void MessageReceived(BMessage *message);
};

#endif















