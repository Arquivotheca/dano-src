//========================================================================
//	MBuildersKeeper.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MBUILDERSKEEPER_H
#define _MBUILDERSKEEPER_H


#include "MList.h"
#include "PlugInPreferences.h"
#include "MPlugInBuilder.h"
#include "MTargetTypes.h"

#include <Locker.h>
#include <image.h>

class BMessage;
class MPlugInLinker;
class MPlugInBuilder;
class String;
struct TargetRec;
struct BuilderRec;
class BPopUpMenu;

struct AddOnRec
{
	image_id			id;
	MPlugInBuilder*		builder;
	MPlugInLinker*		linker;
};


class MBuildersKeeper
{
public:
								MBuildersKeeper();
								~MBuildersKeeper();

	static MPlugInLinker*		GetLinker(
									const char *	inLinkerToolName);

	static void					GetBuilderToolName(
									MPlugInBuilder*		inBuilder,
									String&				inoutToolName,
									MakeStageT			inMakeStage = kInvalidStage,
									MakeActionT			inAction = kInvalidAction);
	static void					BuildTargetList(
									const TargetRec*	inTargetRecs,
									int32				inTargetCount,
									ProjectTargetList&	inTargetList);
	static bool					ValidateGenericData(
									BMessage&	inMessage);

	static bool					GetNthAddOnID(
									image_id&		outID,
									int32			inIndex,
									MPlugInLinker*&	outLinker);
	static bool					GetNthBuilder(
									BuilderRec*&	outRec,
									int32		inIndex);
	static bool					GetNthLinker(
									MPlugInLinker*&	outLinker,
									int32		inIndex);

	static void					GetBuilderNames(
									MList<char*>&	outList);
	static void					BuildTargetsPopup(
									BPopUpMenu&		inPopup);
	static void					ProjectChanged(MProject& inProject,
											   ChangeT inChange);

private:

static	MList<MPlugInBuilder*>		fBuilderList;
static	MList<MPlugInLinker*>		fLinkerList;
static	MList<BuilderRec*>			fBuilderLinkedlist;
static	MList<AddOnRec*>			fAddOnlist;
static	BLocker						fLock;

	static void					GetBuilders();

	static void					GenerateBuilderLinkedList();
	static void					AddToBuilderLinkedList(
									MList<MPlugInBuilder*>&		inList);

	static void					AddToBuilderLinkedList(
									MPlugInBuilder&		inBuilder,
									bool						inIsLinker = false);

	static void					FillMessage(
									BMessage&		inFrom,
									BMessage&		inTo,
									uint32			inType);

static inline void				Lock();
static inline void				Unlock();

};

inline void MBuildersKeeper::Lock()
{
	fLock.Lock();
}
inline void MBuildersKeeper::Unlock()
{
	fLock.Unlock();
}

#endif
