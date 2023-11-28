//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: SFModTbl.H
//
// Author: Mike Guzewicz
//
// Description: Data structure containing a modulator image for a given sound
//
// IMPORTANT: sfmodnam.cpp should be updated when enumeration lists are modified!
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// MikeGuz             Jun ??, 1996  Initial development.
//
//*****************************************************************************

#ifndef __SFMODTBL_H
#define __SFMODTBL_H

#include "datatype.h"
//#include "emu_rw.h"
#include "se8210.h"
#include "omega.h"

// Forward declaration
typedef enum VoiceParamTag SEVOICEPARAM;

enum enSupportedSources
{

  // Make sure you update 'sfmodnam.cpp' too!

  ssBeginSupportedSources,
  ssKeyNumber=ssBeginSupportedSources,
  ssKeyOnVelocity,
  ssPitchWheel,
  ssChanPressure,
  ssPitchBendSensitivity,
  ssCC1,
  ssCC2,
  ssCC7,
  ssCC10,
  ssCC11,
  ssCC21,
  ssCC22,
  ssCC23,
  ssCC24,
  ssCC91,
  ssCC93,

  // Make sure this is the last PROGRAMMABLE source!
  ssEndSupportedSources,

  ssNRPNRelative,
  ssNRPNAbsolute,

  // Make sure this is the last one!
  ssEndAllSources

};

enum enSupportedSourceType
{
  // Make sure you update 'sfmodnam.cpp' too!

  syBeginSupportedSourceTypes,
  syPositiveUnipolar = syBeginSupportedSourceTypes,
  syNegativeUnipolar,
  syPositiveBipolar,
  syNegativeBipolar,

  // Make sure this is the last one!
  syEndSupportedSourceTypes
};

enum enSupportedSourceStyles
{
  // Make sure you update 'sfmodnam.cpp' too!

  slBeginSupportedSourceStyles,

  slLinear=slBeginSupportedSourceStyles,
  slConcave,
  slConvex,
  slSwitch,

  // Make sure this is the last one!
  slEndSupportedSourceStyles
};

enum enSupportedTransforms
{

  // Make sure you update 'sfmodnam.cpp' too!

  stBeginSupportedTransforms,
  stLinear = stBeginSupportedTransforms,
  //stAmplitudeSquared,
  stAbsoluteValue,

  // Make sure this is the last one!
  stEndSupportedTransforms
};
 
struct modInstance
{
  enSupportedSources    src;
  SEVOICEPARAM          dest;
  LONG                  amount;
  enSupportedTransforms xform;
  enSupportedSources    asrc;
  enSupportedSourceType srcType;
  enSupportedSourceType asrcType;
  BOOL					isASrc;

  modInstance           *pNextInstanceThisDest;
  modInstance           *pNextInstanceThisSource; 
  modInstance           *pNextInstanceThisSourceAndDest;
  modInstance           *pAsrcInstance;
  
};


class MakeDefaultModulatorTable;
  
class ModulatorTable : public OmegaClass
{
  
  public:

    ModulatorTable();
    ~ModulatorTable();

    void *operator new(size_t size) {return NewAlloc(size);}
    void operator delete(void *ptr) {DeleteAlloc(ptr);}

    static void dealloc(void *ptr) {delete (ModulatorTable *)ptr;}
    static void *ccons(void *ptr)
      {return new ModulatorTable(*(ModulatorTable *)ptr);}

    void  SetDefaultModulators(void);
    void  ZapAllModulators(void);

    void  AddRef(void);
    DWORD DeleteRef(void);
  
    // SERIOUS kludge until API supports Source Styles
    modInstance * NewModulator(enSupportedSources src,
                             enSupportedSourceType type,
							 enSupportedSourceStyles style,
                             WORD dest,
                             LONG amount,
                             enSupportedTransforms xform, 
                             enSupportedSources asrc, 
							 enSupportedSourceType atype,
							 enSupportedSourceStyles astyle,
							 BOOL addTo=FALSE)
    {
    
      type = (enSupportedSourceType)( ((WORD)type&3) | ((WORD)(style<<2)) );
	  atype = (enSupportedSourceType)( ((WORD)atype&3) | ((WORD)(astyle<<2)) );
      return NewModulator(src, type, dest, amount, xform, asrc, atype, addTo);
    }

    modInstance * NewModulator(enSupportedSources src,
                             enSupportedSourceType type,
                             WORD dest,
                             LONG amount,
                             enSupportedTransforms xform, 
                             enSupportedSources asrc, 
			     enSupportedSourceType atype,
			     BOOL addTo=FALSE);
    
    modInstance *GetInstance   (enSupportedSources, SEVOICEPARAM);
    void         SetInstance   (enSupportedSources, SEVOICEPARAM, modInstance*);
    modInstance *GetIdenticalInstance(enSupportedSources, enSupportedSourceType,
				    SEVOICEPARAM, enSupportedTransforms,
				    enSupportedSources, enSupportedSourceType);

	DWORD	GetMaxSupportedDestinations(void);
    void  ZapModulator(enSupportedSources src, SEVOICEPARAM dest, modInstance *);

    modInstance * FirstModThisSource(enSupportedSources src, SEVOICEPARAM& dest);
    modInstance * NextModThisSource(SEVOICEPARAM& dest, BOOL retInst=TRUE);

    modInstance * FirstModThisDest(SEVOICEPARAM dest, enSupportedSources& src);
    modInstance * NextModThisDest(enSupportedSources& src, BOOL retInst=TRUE);

    static ModulatorTable *GetDefaultModulatorTable(void) {return def;}

    BOOL               srcModulating[ssEndSupportedSources];
    BOOL               *destModulated;

  friend class MakeDefaultModulatorTable;

  private:

    static ModulatorTable *def;
#ifdef DEBUG
	static DWORD mtCount;
#endif

    modInstance *_NewInstance   (enSupportedSources, SEVOICEPARAM, BOOL isAsrc=FALSE);
 
    void  _ZapModInstance(enSupportedSources src, SEVOICEPARAM dest, modInstance *inst);
   
    modInstance *_FindASourceInstance   (enSupportedSources src, SEVOICEPARAM dest, modInstance *inst);
    modInstance *_FindASourceGivenSource(enSupportedSources src, SEVOICEPARAM dest, modInstance *inst);
    modInstance *_FindSourceGivenASource(enSupportedSources src, SEVOICEPARAM dest, modInstance *inst);
	 
	void _GetPreviousInstances(enSupportedSources src, SEVOICEPARAM dest, modInstance **pPrevSource, modInstance **pPrevDest);
      
 	modInstance          **pFirstInstanceThisDest;    // Maximum destinations
	modInstance          **pFirstInstanceThisSource;  // Maximum sources

    enSupportedSources searchSource;
    SEVOICEPARAM       searchDest;
    modInstance        *searchInstance;
    DWORD              dwRefCount;

#ifdef DEBUG_SFMODTBL
	FILE *logFile;
#endif

};

#endif
