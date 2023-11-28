//******************************************************************************
//
//	File:		GameSupport.cpp
//
//	Description:	BGameSupport class.
//			        Client class for game input settings.
//
//	Written by:	Pierre Raynaud-Richard
//
//	Revision history
//
//	Copyright 1996, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>
#include <string.h>

#ifndef _GAME_SUPPORT_H
#include <GameSupport.h>
#endif
#ifndef _GAME_INPUT_H
#include <GameInput.h>
#endif
#ifndef _VOLUME_H
#include <Volume.h>
#endif
#ifndef _DATABASE_H
#include <Database.h>
#endif
#ifndef _QUERY_H
#include <Query.h>
#endif
#ifndef _TABLE_H
#include <Table.h>
#endif
#ifndef _RECORD_H
#include <Record.h>
#endif

static char m_names[24][5] = {
	"CAL1", "KEY1", "KFT1", "SFT1", "CTR1", "CLT1",
	"CAL2", "KEY2", "KFT2", "SFT2", "CTR2", "CLT2",
	"CAL3", "KEY3", "KFT3", "SFT3", "CTR3", "CLT3",
	"CAL4", "KEY4", "KFT4", "SFT4", "CTR4", "CLT4"
};

#define  TABLE_NAME  "GameInputSetting"

BGameSupport::BGameSupport(BGameInput *input):BLooper() {
	my_db = boot_volume().Database();
	my_input = input;
	Run();
}

void BGameSupport::SetApplication(ulong signature, ulong version) {
	my_signature = signature;
	my_version = version;
}

void BGameSupport::SetDefaultSetting() {
}

void BGameSupport::GetUserSetting() {
}

void BGameSupport::EditSetting(ulong signature) {
}

void BGameSupport::MessageReceived(BMessage *message) {
	switch (message->what) {
	case 1000 :
		if (my_input != 0L)
			MessageToSetting(my_input, message);
		break;
	}
}

BMessage *BGameSupport::SettingToMessage(BGameInput *set, bool minimal) {
	int                i, j, save;
	long               count;
    uchar              type;
	short              cal[6];
	uchar              keys[8];
	short              key_f[8];
	short              joy_f[8];
	BMessage           *m;
	game_input_control *ctrl;

	m = new BMessage();
	for (i=0;i<4;i++) {
		set->GetChannel(i, &type, cal, keys, key_f, joy_f, &count, &ctrl);
		m->AddInt32("TYPE", type);
		for (j=0;j<6;j++)
			if ((cal[j] != g_set[i].cal[j]) || (!minimal)) {
				m->AddData(m_names[i*6+0], B_RAW_TYPE, (void*)cal, 6*sizeof(short));
				break;
			}
		for (j=0;j<8;j++)
			if ((keys[j] != g_set[i].keys[j]) || (!minimal)) {
				m->AddData(m_names[i*6+1], B_RAW_TYPE, (void*)keys, 8*sizeof(uchar));
				break;
			}
		for (j=0;j<8;j++)
			if ((key_f[j] != g_set[i].key_f[j]) || (!minimal)) {
				m->AddData(m_names[i*6+2], B_RAW_TYPE, (void*)key_f, 8*sizeof(short));
				break;
			}
		for (j=0;j<8;j++)
			if ((joy_f[j] != g_set[i].joy_f[j]) || (!minimal)) {
				m->AddData(m_names[i*6+3], B_RAW_TYPE, (void*)joy_f, 8*sizeof(short));
				break;
			}
		save = TRUE;
		if ((count == B_GENERAL_CONTROL_COUNT) && minimal)
			if (memcmp(ctrl, g_set[i].ctrl,
					   B_GENERAL_CONTROL_COUNT*sizeof(game_input_control)) == 0)
				save = FALSE;
		if (save) {
			m->AddInt32(m_names[i*6+4], count);
			m->AddData(m_names[i*6+5], B_RAW_TYPE, (void*)ctrl,
					   B_GENERAL_CONTROL_COUNT*sizeof(game_input_control));
		}
	}
	return m;
}

long BGameSupport::MessageToSetting(BGameInput *set, BMessage *m) {
	int                i;
	long               num;
	long               count;
    uchar              type;
	short              *cal;
	uchar              *keys;
	short              *key_f;
	short              *joy_f;
	game_input_control *ctrl;

	for (i=0;i<4;i++) {
	    type = m->FindInt32("TYPE", i);
		if (m->Error() == B_ERROR) return FALSE;
		
		cal = (short*)m->FindData(m_names[i*6+0], B_RAW_TYPE, &num);
		if ((m->Error() == B_ERROR) || (num != 6*sizeof(short)))
			cal = g_set[i].cal;
		
		keys = (uchar*)m->FindData(m_names[i*6+1], B_RAW_TYPE, &num);
		if ((m->Error() == B_ERROR) || (num != 8*sizeof(uchar)))
			keys = g_set[i].keys;
		
		key_f = (short*)m->FindData(m_names[i*6+2], B_RAW_TYPE, &num);
		if ((m->Error() == B_ERROR) || (num != 8*sizeof(short)))
			key_f = g_set[i].key_f;
		
		joy_f = (short*)m->FindData(m_names[i*6+3], B_RAW_TYPE, &num);
		if ((m->Error() == B_ERROR) || (num != 8*sizeof(short)))
			joy_f = g_set[i].joy_f;

		count = m->FindInt32(m_names[i*6+4]);
		if (m->Error() != B_ERROR) {
			ctrl = (game_input_control*)m->FindData(m_names[i*6+5], B_RAW_TYPE, &num);
			if ((m->Error() == B_ERROR) ||
				(num != B_GENERAL_CONTROL_COUNT*sizeof(game_input_control))) {
				ctrl = g_set[i].ctrl;
				count = B_GENERAL_CONTROL_COUNT;
			}
		}
		else {
			ctrl = g_set[i].ctrl;
			count = B_GENERAL_CONTROL_COUNT;
		}
		set->SetChannel(i, type, cal, keys, key_f, joy_f, count, ctrl);
	}
	return TRUE;
}

long BGameSupport::MessageToGeneral(BMessage *m) {
	int  		  i, j;
	long  		 num;
	const void   *data;

	for (i=0;i<4;i++) {
		g_set[i].type = m->FindInt32("TYPE", i);
		if (m->Error() == B_ERROR) return FALSE;
		
		data = m->FindData(m_names[i*6+0], B_RAW_TYPE, &num);
		if (m->Error() == B_ERROR) return FALSE;
		memcpy(g_set[i].cal, data, 6*sizeof(short));
		
		data = m->FindData(m_names[i*6+1], B_RAW_TYPE, &num);
		if (m->Error() == B_ERROR) return FALSE;
		memcpy(g_set[i].keys, data, 8*sizeof(uchar));
		
		data = m->FindData(m_names[i*6+2], B_RAW_TYPE, &num);
		if (m->Error() == B_ERROR) return FALSE;
		memcpy(g_set[i].key_f, data, 8*sizeof(short));
		
		data = m->FindData(m_names[i*6+3], B_RAW_TYPE, &num);
		if (m->Error() == B_ERROR) return FALSE;
		memcpy(g_set[i].joy_f, data, 8*sizeof(short));
		
		data = m->FindData(m_names[i*6+5], B_RAW_TYPE, &num);
		if (m->Error() == B_ERROR) return FALSE;
		memcpy(g_set[i].ctrl, data, B_GENERAL_CONTROL_COUNT*sizeof(game_input_control));
	}
	return TRUE;
}

BMessage *BGameSupport::GeneralToMessage() {
	int      i, j;
	long     num;
	void     *data;
	BMessage *m;

	m = new BMessage();
	for (i=0;i<4;i++) {
		m->AddInt32("TYPE", g_set[i].type);
		m->AddData(m_names[i*6+0], B_RAW_TYPE, (void*)g_set[i].cal, 6*sizeof(short));
		m->AddData(m_names[i*6+1], B_RAW_TYPE, (void*)g_set[i].keys, 8*sizeof(uchar));
		m->AddData(m_names[i*6+2], B_RAW_TYPE, (void*)g_set[i].key_f, 8*sizeof(short));
		m->AddData(m_names[i*6+3], B_RAW_TYPE, (void*)g_set[i].joy_f, 8*sizeof(short));
		m->AddData(m_names[i*6+5], B_RAW_TYPE, (void*)g_set[i].ctrl,
				   B_GENERAL_CONTROL_COUNT*sizeof(game_input_control));
	}
	return m;
}

BTable *BGameSupport::GetPrefTable() {
	BTable    *table;
	
	table = my_db->FindTable(TABLE_NAME);
	if (table == NULL) {
		table = my_db->CreateTable(TABLE_NAME);
		table->AddInt32Field("signature");
		table->AddInt32Field("version");
		table->AddInt32Field("type");
		table->AddRawField("data");
	}
	return table;
}

BMessage *BGameSupport::LoadPrefs(long type) {
	void      *data;
	long      size;
	BTable    *table;
	BQuery    *query;
	BRecord   *rec;
	BMessage  *m;
	
	table = GetPrefTable();
	query = new BQuery();
	query->AddTable(table);
	query->PushField("signature");
	query->PushLong(my_signature);
	query->PushField("version");
	query->PushLong(my_version);
	query->PushField("type");
	query->PushLong(type);
	if (query->Fetch() != B_NO_ERROR) return 0L;
	rec = new BRecord(my_db, query->RecordIDAt(0));
	data = rec->FindRaw("data", &size);
	m = new BMessage();
	m->Unflatten((char*)data);
	delete query;
	return m;
}

void BGameSupport::SavePrefs(BMessage *m, long type) {
	void      *data;
	long      size;
	BTable    *table;
	BRecord   *rec;
		
	table = GetPrefTable();
	rec = new BRecord(table);
	size = m->FlattenedSize();
	data = malloc(size);
	m->Flatten(data, size);
	rec->SetLong("signature", my_signature);
	rec->SetLong("version", my_version);
	rec->SetLong("type", type);
	rec->SetRaw("data", data, size);
	free(data);
}














