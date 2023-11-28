// ============================================================
//  KanaString.h	by Hiroshi Lockheimer
// ============================================================

#ifndef _KANA_STRING
#define _KANA_STRING

#include <SupportDefs.h>


enum {
	HIRA_TO_KATA,
	KATA_TO_HIRA,
	ZENKATA_TO_HANKATA
};

enum {
	HIRA_INPUT,
	ZEN_KATA_INPUT,
	ZEN_EISUU_INPUT,
	HAN_KATA_INPUT,
	HAN_EISUU_INPUT,
	DIRECT_INPUT,
	DIRECT_HIRA_INPUT,
	DIRECT_KATA_INPUT
};


class KanaString {
public:
						KanaString();
	virtual				~KanaString();

	void				Append(const char* theChar, uint32 charLen);
	void				Backspace();
	void				Delete();
	void				Clear();

	int32				MoveInsertionPoint(bool left);
	int32				InsertionPoint() const;

	const char*			String(bool confirm) const;
	int32				Length() const;

	void				SetMode(uint32 mode);

	static char*		ConvertKanaString(const char *kana, int32 *kanaLen, uint32 type);

protected:
	void				RomaToKana();
	void				DirectKana();
	bool				TransformLastKana(char theChar);
	bool				CheckForN(int32 offset);

private:
	int32				fStringLen;
	char*				fString;
	int32				fInsertionPoint;
	uint32				fInputMode;

public:
	static uint32		sKutoutenMode;
	static bool			sFullSpaceMode;
};

#endif
