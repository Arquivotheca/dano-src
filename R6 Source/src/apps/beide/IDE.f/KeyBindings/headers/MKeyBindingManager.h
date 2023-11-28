//========================================================================
//	MKeyBindingManager.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MKEYBINDINGMANAGER_H
#define _MKEYBINDINGMANAGER_H

#include "IDEConstants.h"
#include "MList.h"

#include <InterfaceDefs.h>

class BMemoryIO;
class BMallocIO;

enum EndianT {
	kBigEndian,
	kLittleEndian,
	kHostEndian
};

inline long long TicksToMicroSeconds(long long a) 
{ return a * 1000000 / 60; }
inline long long MicroSecondsToTicks(long long a) 
{ return a * 60 / 1000000; }

struct KeyBinding
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int16		prefixIndex;		// index of prefix character
	int16		modifiers;			// all the modifiers fit in 16 bits
	int16		keyCode;			// ascii character
	bool		isVKey;				// function key or related virtual key
	bool		allowAutoRepeat;	// can we do more than one of these commands in a row
};

struct KeyBindingInfo
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	CommandT	cmdNumber;
	KeyBinding	binding1;
	KeyBinding	binding2;
};

struct KeyBindingPrefHeader
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int16		version;
	int16		prefixTimeout;	// ticks (1/60 second)
	int16		prefixCount;
};

enum KeyBindingContext
{
	kBindingNone,		// doesn't exist, contexts count from 1
	kBindingGlobal,		// all menu items
	kBindingEditor,		// all editor commands
	kBindingTempGlobal	// scripts menu on Mac
};

const int32	kBindingContextCount = 4;

struct KeyBindingContextData
{
//	void		SwapBigToHost();
	void		SwapHostToBig();

	int16				whichContext;
	int16				bindingCount;
	KeyBindingInfo*		bindingData;
};


const int16 kGoodModifiers = B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY;
const int16 kKeyBindingsVersion = 0x0010;
const int16 kBindingPrefixCount = 4;
const int32 kInvalidPrefixIndex = 0;
const int32 kQuoteKeyIndex = 1;
const KeyBinding kEmptyBinding = { 0, 0, 0, 0, 0 };

class MKeyBindingManager {
public:
	static	MKeyBindingManager&	Manager()
								{
									return *sGlobalManager;
								}

								MKeyBindingManager();
								MKeyBindingManager(
									BMemoryIO&	inData,
									EndianT		inEndian,
									bool		inIsGlobal = false);
								~MKeyBindingManager();

	MKeyBindingManager& 		operator=(
									const MKeyBindingManager& inManager);

	static	bool				MessageToBinding(
									BMessage*	inMessage, 
									KeyBinding&	outBinding);
	bool						IsPrefixKey(
									KeyBinding&	inBinding) const;
	uint32						GetCommand(
 									KeyBindingContext	inContext,
									const KeyBinding&	inBinding) const;
	bool						GetBinding(
									KeyBindingContext	inContext,
									CommandT			inCommand,
									KeyBinding&			outBinding) const;
	bool						GetBinding(
									KeyBindingContext	inContext,
									CommandT			inCommand,
									KeyBindingInfo&		outBinding) const;
	void						SetBinding(
									KeyBindingContext		inContext,
									const KeyBindingInfo&	inBinding);
	bool						BindingExists(
									KeyBindingContext		inContext,
									const KeyBindingInfo&	inInfo,
									CommandT&				outCommand) const;
	void						AddNewBinding(
									KeyBindingContext		inContext,
									const KeyBindingInfo&	inBinding);
	void						AddNewBinding(
									CommandT			inCommand,
									const KeyBinding&	inBinding1,
									const KeyBinding&	inBinding2);
	void						GetPrefixBinding(
									int32		inPrefixIndex,
									KeyBinding&	outBinding) const;
	void						SetPrefixBinding(
									int32				inPrefixIndex,
									const KeyBinding&	inBinding);
	void						GetKeyBindingsHost(
									BMallocIO&	outData) const;
	void						GetKeyBindingsBig(
									BMallocIO&	outData) const;
	bool						SetKeyBindingsHost(
									BMemoryIO&	inData);
	bool						SetKeyBindingsBig(
									BMemoryIO&	inData);
	void						SetPrefixTimeout(
									bigtime_t	inNewTimeout)
								{
									fPrefixTimeout = inNewTimeout;
								}
	bigtime_t					PrefixTimeout();

	int32						ContextCount() const
								{
									return fBindingContexts.CountItems() - 1;
								}
	static void					BuildDefaultKeyBindings(
									BMallocIO&	outData);
private:

	bigtime_t			fPrefixTimeout;				// microseconds
	KeyBinding			fPrefixKeys[kBindingPrefixCount+1];

	MList<KeyBindingContextData*>	fBindingContexts;

static MKeyBindingManager*	sGlobalManager;

	void						InitContexts(
									int32	inHowMany);
	void						ReleaseContexts();

								MKeyBindingManager(
									const MKeyBindingManager& inManager);

};

// ---------------------------------------------------------------------------
//		KeyBinding comparison operators
// ---------------------------------------------------------------------------

inline bool
operator==(const KeyBinding& b1, const KeyBinding& b2)
{
	return (b1.keyCode == b2.keyCode &&
			b1.modifiers == b2.modifiers &&
			b1.isVKey == b2.isVKey &&
			b1.prefixIndex == b2.prefixIndex);
}

inline bool
operator!=(const KeyBinding& b1, const KeyBinding& b2)
{
	return (b1.keyCode != b2.keyCode ||
			b1.modifiers !=  b2.modifiers ||
			b1.isVKey != b2.isVKey ||
			b1.prefixIndex != b2.prefixIndex);
}

#endif
