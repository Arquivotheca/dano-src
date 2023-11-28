///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ATI 3D RAGE BEOS DRIVER - CARD INITIALIZATION SCRIPT                      //
//                                                                           //
// alt.software inc. 1998                                                    //
// Written by Christopher Thomas                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
// This file contains the declaration of the register set startup script used
// by OpenGraphicsCard.c.

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Extern Variables                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// "standard" Rage cards
// Script run before memory and clock setup.
extern REGSET_STRUCT SetupStart[];
// Script run after memory and clock setup.
extern REGSET_STRUCT SetupEnd[];


// Rage cards with LCD support
// Script run before memory and clock setup.
extern REGSET_STRUCT SetupStartLT[];
// Script run after memory and clock setup.
extern REGSET_STRUCT SetupEndLT[];
