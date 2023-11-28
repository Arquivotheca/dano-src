
#ifndef _ANSWER_TO_RESET_H
#define _ANSWER_TO_RESET_H

#include <SmartCardDefs.h>


class ANSWER_TO_RESET
{
public:

	enum encoding_convention
	{
		DIRECT_CONVENTION = 0x3B,
		INVERSE_CONVENTION = 0x3F
	};


				ANSWER_TO_RESET(BYTE *Atr);
	virtual 	~ANSWER_TO_RESET();

	status_t	InitCheck			() const;
	DWORD 		SupportedProtocols	() const;
	DWORD 		DefaultProtocol		() const;
	DWORD 		EncodingConvention	() const;
	void 		PrintToStream		() const;

private:
	encoding_convention		fConvention;
	DWORD					fDefaultProtocol;
	DWORD					fAvaillableProtocols;

private:
	status_t 	fInitOk;
	uint8		fAtr[B_PCSC_MAX_ATR_SIZE];
	int8		fAtrLength;
};


#endif
