/* ex:set sw=2 ts=8 wm=0:
 * $Header: u:/rcs/cv/rcs/dibtoyuv.c 2.13 1995/05/09 09:23:17 bog Exp $

 * (C) Copyright 1992-1993 SuperMac Technology, Inc.
 * All rights reserved

 * This source code and any compilation or derivative thereof is the
 * sole property of SuperMac Technology, Inc. and is provided pursuant
 * to a Software License Agreement.  This code is the proprietary
 * information of SuperMac Technology and is confidential in nature.
 * Its use and dissemination by any party other than SuperMac Technology
 * is strictly limited by the confidential information provisions of the
 * Agreement referenced above.

 * $Log: dibtoyuv.c $
 * Revision 2.13  1995/05/09 09:23:17  bog
 * Move WINVER back into the makefile.  Sigh.
 * Revision 2.12  1994/10/23  17:22:35  bog
 * Try to allow big frames.
 * 
 * Revision 2.11  1994/07/18  13:30:43  bog
 * Move WINVER definition from makefile to each .c file.
 * 
 * Revision 2.10  1994/03/17  08:42:20  timr
 * USE32 segments are loaded as USE16 in Win3.1 and USE32 in Chicago.  So
 * we cannot simply reference baseFRAGTEXT32().  We didn't need to anyway.
 * 
 * Revision 2.9  1993/11/05  11:36:04  bog
 * GlobalFix/GlobalUnfix expect handle, not selector.
 * 
 * Revision 2.8  1993/10/12  17:24:47  bog
 * RGB555 is now Draw15/Expand15 and RGB565 is Draw15/Expand16.
 * 
 * Revision 2.7  1993/09/23  17:21:23  geoffs
 * Now correctly processing status callback during compression
 * 
 * Revision 2.6  1993/08/10  15:26:03  timr
 * Subtracting offset of start of compiled area should happen only on NT.
 * 
 * Revision 2.5  1993/08/05  17:06:03  timr
 * Correct another case of assuming the compiled code goes into a segment 
 * based at 0.
 * 
 * Revision 2.2  93/06/10  09:18:28  geoffs
 * Add 32 bit DIB support
 * 
 * Revision 2.1  93/06/08  16:47:51  geoffs
 * GlobalFix those selectors which get aliased
 * 
 * Revision 2.0  93/06/01  14:13:56  bog
 * Version 1.0 Release 1.3.0.1 of 1 June 1993.
 * 
 * Revision 1.17  93/04/21  15:47:31  bog
 * Fix up copyright and disclaimer.
 * 
 * Revision 1.16  93/03/08  14:21:17  geoffs
 * New code accessed selector variable incorrectly
 * 
 * Revision 1.15  93/03/08  14:10:17  geoffs
 * Avoid AllocDSToCSAlias side-effects on original selector by using AllocSelector,PrestoChangoSelector
 * 
 * Revision 1.14  93/02/16  14:48:25  geoffs
 * Added recreate of smooth vectors from detail vectors
 * 
 * Revision 1.13  93/01/27  19:01:28  geoffs
 * Fix up two problems having to do with loss of data due to short arithmetic
 * 
 * Revision 1.12  93/01/27  13:13:47  geoffs
 * Added more pointer checking code, fixed up gpf in 2nd pass vertical filter
 * 
 * Revision 1.11  93/01/27  08:01:18  geoffs
 * Added debug check for pointer ranges; added fix so that last input scan + 1 not processed
 * 
 * Revision 1.10  93/01/26  10:49:59  geoffs
 * Added some debug code, adjusted tileheight before call to compiled code down by 1
 * 
 * Revision 1.9  93/01/25  21:46:12  geoffs
 * Non 0 mod 4 input.  Remove checksum.
 * 
 * Revision 1.8  93/01/16  16:08:21  geoffs
 * Added code to checksum detail,smooth vectors being output and to display
 * results. Code now sets things up correctly for vector output rather than
 * the previous raster output.
 * 
 * Revision 1.7  93/01/13  17:22:17  geoffs
 * Will use globally available flatSel so cut out code here to alloc one
 * 
 * Revision 1.6  93/01/13  10:37:21  geoffs
 * The detail and smooth lists appear to be outputting consistent data now
 * 
 * Revision 1.5  93/01/12  17:15:32  geoffs
 * First halting steps towards running.
 * 
 * Revision 1.4  93/01/11  18:44:50  geoffs
 * Have all the fragment code in now, but not yet fully debugged
 * 
 * Revision 1.3  93/01/11  09:39:21  geoffs
 * Don't get impatient -- we're almost there. Have only the 2nd vertical
 * filtering pass on UV to go.
 * 
 * Revision 1.2  93/01/10  17:01:06  geoffs
 * 75% of the way towards a complete compiled code version
 * 
 * Revision 1.1  93/01/06  10:12:42  bog
 * Initial revision
 * 
 *
 * CompactVideo Codec Convert DIB to VECTOR
 */

#include <windows.h>
#include <windowsx.h>

#include "compddk.h"
#include "cv.h"
#include "cvcompre.h"
#include "dibtoyuv.h"


//#define	STATISTICS_ALWAYS		// for now
#define	STATISTICS			// for now
#if	(defined(DEBUG) && defined(STATISTICS)) || defined(STATISTICS_ALWAYS)
unsigned short genCodeSize = 0;		// compiled code size
#endif

#if	!defined(WIN32)
/**********************************************************************
 *
 * CopyMem()
 *
 * Copy source to destination. Move size can be no greater than 64kb
 * and must not wrap the end of source or destination segments.
 *
 * Returns a -> to the next destination location after the copy
 *
 **********************************************************************/

unsigned char far * PASCAL CopyMem(
  unsigned char far *pS,
  unsigned char far *pD,
  unsigned short len
) {
  _asm {
	push	si
	push	di
	push	ds

	lds	si,pS		// fetch -> source 
	les	di,pD		// fetch -> destination
	mov	cx,len		// # bytes to copy

	shr	cx,1		// get # whole words to copy
	rep	movsw		// move all whole words
	adc	cx,cx
	rep	movsb		// move any odd byte

	mov	word ptr pD,di	// remember next destination

	pop	ds
	pop	di
	pop	si
  }

  return (pD);			// return -> next destination
}
#else
#define	CopyMem(s,d,l)		(\
				 CopyMemory((d),(s),(l)),\
				 ((unsigned char far *) (d) + (l))\
				)
#endif

/**********************************************************************
 *
 * LoadFragment()
 *
 * Load a code fragment to the destination, applying any fixups
 *
 **********************************************************************/

unsigned char far * PASCAL LoadFragment(
  unsigned char far *pF,		// -> base of fragment segment
  unsigned char far *pC,		// -> building code area
  COMPILEDATA far *pCD,			// -> table of compile data
  FRAGINFO far *pFI			// -> fragment info for fragment
) {
  int fixup;				// current fixup being processed

  // copy the code fragment to the destination
  CopyMem(
  	  (pF + pFI->fragOffset),	// -> start of code fragment
	  pC,				// place to put it
	  pFI->fragLength		// # bytes of code to copy
	 );

  // process any fixups that need to be applied to the copied fragment
  for (fixup = 0; pFI->fragFixups[fixup].fixupType != FIXUP_EOT; fixup++) {
    FRAGFIXUP fu;

    fu = pFI->fragFixups[fixup];	// latch local copy of fixup record

    switch (fu.fixupType) { // depending on fixup type...

      case FIXUP_DSREL32: { // 32-bit DS relative fixup...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  *((unsigned long far *) (
	  			   pF +
				   pFI->fragOffset +
				   fu.fixupPCRelOffset
				  )
	   ) +
	  pCD->oPrivate;
        break;
      }

      case FIXUP_UVOFF: { // offset of V from U intermediate...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  pCD->oInterV2 - pCD->oInterU2;
        break;
      }

      case FIXUP_WORKYSTEP: { // work scan ystep * value in instruction...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  (
	   *((unsigned long far *) (pF+pFI->fragOffset+fu.fixupPCRelOffset)) &
	   0x7fffffffL
	  ) * 
	  pCD->workYStep;
        break;
      }

      case FIXUP_WORKPLUS: { // offset to work + (ystep * instruction value)...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  (
	   (
	    *((unsigned long far *) (pF+pFI->fragOffset+fu.fixupPCRelOffset)) &
	    0x7fffffffL
	   ) * 
	   pCD->workYStep
	  ) +
	  pCD->oWork;
        break;
      }

      case FIXUP_WORKWIDTH: { // workwidth >> value in instruction...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  pCD->workYStep >>
	  (
	   *((unsigned char far *) (pF+pFI->fragOffset+fu.fixupPCRelOffset)) &
	   0x7fffffffL
	  );
        break;
      }

      case FIXUP_JMPLP1: { // save -> operand field of jmp to loop1...
        pCD->pJmpToLoop1 = (unsigned long far *) (pC + fu.fixupPCRelOffset);
	break;
      }

      case FIXUP_SRCWIDTH: { // src width >> value in instruction...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  pCD->DIBWidth >>
	  (
	   *((unsigned char far *) (pF+pFI->fragOffset+fu.fixupPCRelOffset)) &
	   0x7fffffffL
	  );
        break;
      }

      case FIXUP_SRCYSTEP: { // add src ystep * instruction value
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  (
	   *((unsigned long far *) (pF+pFI->fragOffset+fu.fixupPCRelOffset)) &
	   0x7fffffffL
	  ) * 
	  pCD->DIBYStep;
        break;
      }

      case FIXUP_STOREPC0: { // save current PC at fixup...
	pCD->storedPC0 = (unsigned char far *) (pC + fu.fixupPCRelOffset);
        break;
      }

      case FIXUP_USE_STOREDPC0: { // fixup relative to previously stored PC...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  - (long)((pC + (fu.fixupPCRelOffset + 4)) - pCD->storedPC0);
        break;
      }

      case FIXUP_USE_STOREDPC1: { // fixup relative to previously stored PC...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) =
	  - (long)((pC + (fu.fixupPCRelOffset + 4)) - pCD->storedPC1);
        break;
      }

      case FIXUP_INTERU2: { // store 32-bit offset to U2 intermediate...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) = pCD->oInterU2;
	break;
      }

      case FIXUP_INTERV2: { // store 32-bit offset to V2 intermediate...
	*((unsigned long far *) (pC + fu.fixupPCRelOffset)) = pCD->oInterV2;
	break;
      }
    }
  }

  return (pC + pFI->fragLength);	// return -> spot for next fragment
}


/**********************************************************************
 *
 * CompileFragments()
 *
 * Compile optimized code for DIB to VECTOR conversion
 *
 **********************************************************************/

int PASCAL CompileFragments(
  DIBTOYUVPRIVATE far *pP,		// -> private context
  short Width,				// width of the maximum size tile
  short Height,				// height of the maximum size tile
  short DIBWidth,			// constant width of incoming DIB
  long DIBYStep,			// bytes from (x,y) to (x,y+1) in DIB
  DIBTYPE DibType,			// type of source DIB
  P8LOOKUP far *pLookup			// -> lookup if DibType == Dither8
) {
  union {
    DIBTOYUVPRIVATE far *pP;
    unsigned char far *pF;
  } pF;
  COMPILEDATA cd;			// data used during compilation
  unsigned char far *pC;		// -> building code area
  FRAGINFO far *pPF;			// -> pixel fetching code
  unsigned char far *startPC;

  // set up initial data in the COMPILEDATA structure
				      // work scan width in bytes
  cd.workYStep = (unsigned short) (Width >> 1);
#if	!defined(WIN32)
  cd.oPrivate = GetSelectorBase(SELECTOROF(pP));
#else
  cd.oPrivate = (unsigned long) pP;
#endif
  cd.oInterU2 = cd.oPrivate + sizeof(DIBTOYUVPRIVATE) + CODEMAXSIZE;
  cd.oInterV2 = cd.oInterU2 + (cd.workYStep * (Height >> 1));
  cd.oWork = cd.oInterV2 + (cd.workYStep * (Height >> 1));
  cd.DIBWidth = (unsigned short) DIBWidth;
  cd.DIBYStep = DIBYStep;

  // set up a data pointer to the start of the loaded code fragment segment
  pF.pF = (unsigned char far *) baseFRAGTEXT32;
#if	defined(WIN32)
  cd.oPrivate -= (unsigned long) pF.pF;
#endif

  // set up a data pointer to the start of the area where code is to be built
  pC = (unsigned char far *) pP + sizeof(DIBTOYUVPRIVATE);

  // load the appropriate portions of the fragment code segment to the
  // private data area we have allocated
  CopyMem(pF.pP->divBy7,pP->divBy7,sizeof(pP->divBy7));

  // set up -> pixel fetching code depending on incoming pixel depth
  switch (DibType) {
    case Dither8: {
      pPF = &H1331FetchPixel8;
      CopyMem( // copy the palette into our private data area
	      (unsigned char far *) pLookup->palPalEntry,
	      (unsigned char far *) pP->lookUp8,
	      (unsigned short) (
	       ((pLookup->palNumEntries) ? pLookup->palNumEntries : 256) *
	       sizeof(pLookup->palPalEntry)
	      )
	     );
      break;
    }
    case RGB555: {
      pPF = &H1331FetchPixel15;
      break;
    }
    case RGB565: {
      pPF = &H1331FetchPixel16;
      break;
    }
    case RGB888: {
      pPF = &H1331FetchPixel24;
      break;
    }
    case RGBa8888: {
      pPF = &H1331FetchPixel32;
      break;
    }
  }

  /****************************************************************************
				Begin Compilation
  ****************************************************************************/

  startPC = pC;			// for generated code size checking

  pC = LoadFragment(pF.pF,pC,&cd,&cEntry);		// entry code

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of input pixels.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch pixel (and convert to RGB24 if necessary)
   	convert RGB24 to YUV
   	store Y0 to detail list
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch pixel (and convert to RGB24 if necessary)
   	convert RGB24 to YUV
   	store Y0 to detail list
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch pixel (and convert to RGB24 if necessary)
   	convert RGB24 to YUV
   	store Y1 to detail list
   	process to continue a 1:3:3:1 filter sequence
   
   	bump detail list -> to next Y pair
   	drop count of pixel pairs
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (pop registers, etc...)
  ****************************************************************************/
  {
    unsigned char far *startH1331;	// -> start of hfilter code
    unsigned short sizeH1331;		// size of hfilter code

    pC = LoadFragment(pF.pF,pC,&cd,&H1331Init);		// one-time inits

    startH1331 = pC;			// remember start of hfilter code
    pC = LoadFragment(pF.pF,pC,&cd,&H1331Start);	// top-loop inits

    pC = LoadFragment(pF.pF,pC,&cd,pPF);		// pixel fetch
    pC = LoadFragment(pF.pF,pC,&cd,&H1331ToYUV);	// convert to YUV
    pC = LoadFragment(pF.pF,pC,&cd,&H1331StoreY0);	// store Y0 to detail
    pC = LoadFragment(pF.pF,pC,&cd,&H431);		// start is 431 filter

    cd.storedPC0 = pC;					// save -> loop0

    pC = LoadFragment(pF.pF,pC,&cd,pPF);		// pixel fetch
    pC = LoadFragment(pF.pF,pC,&cd,&H1331ToYUV);	// convert to YUV
    pC = LoadFragment(pF.pF,pC,&cd,&H1331StoreY0);	// store Y0 to detail
    pC = LoadFragment(pF.pF,pC,&cd,&H1331B);		// 1331B filter

							 // fixup jump into loop
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);

    pC = LoadFragment(pF.pF,pC,&cd,pPF);		// pixel fetch
    pC = LoadFragment(pF.pF,pC,&cd,&H1331ToYUV);	// convert to YUV
    pC = LoadFragment(pF.pF,pC,&cd,&H1331StoreY1);	// store Y1 to detail
    pC = LoadFragment(pF.pF,pC,&cd,&H1331C);		// 1331C filter

    pC = LoadFragment(pF.pF,pC,&cd,&H1331Loop);		// end of loop code

    pC = LoadFragment(pF.pF,pC,&cd,&H1331End);		// 134 + cleanups

    sizeH1331 = pC - startH1331;		// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    cd.storedPC1 = pC;			// remember pc: will be loop top

    pC = CopyMem(startH1331,pC,sizeH1331);		// 2nd line prefetch

    pC = LoadFragment(pF.pF,pC,&cd,&V1331EarlyOut);	// vert 134 early out

    pC = CopyMem(startH1331,pC,sizeH1331);		// 3rd line prefetch

						 	// fix early jump out
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);
  }

  /****************************************************************************
     Compile code which vertically filters the Y,U,V data into Y2,U,V forms

     Code will be generated that has the following logic
   
	load -> Y2 smooth list (start with even pixel ->)

	load work buffer registers
     loop0:
        fetch 4 vertically aligned pixels of Y from work scans and calculate
	    a + 3b + 3c + d
	store result to smooth Y2 list
	horizontally increment pointers to work scans

	increment -> next smooth Y2 pixel

	drop pixel count
   	jmp to loop0 if count != 0

	store -> next smooth Y2 pixel for next iteration
   

	load -> U2 intermediate

	load work buffer registers
     loop1:
        fetch 4 vertically aligned pixels of U2 from work scans and calculate
	    a + 3b + 3c + d
	store result to U2 intermediate
	horizontally increment pointers to work scans

	increment -> next U2 intermediate pixel

	drop pixel count
   	jmp to loop1 if count != 0

	store -> next U2 intermediate pixel for next iteration
   

	load -> V2 intermediate

	load work buffer registers
     loop2:
        fetch 4 vertically aligned pixels of V2 from work scans and calculate
	    a + 3b + 3c + d
	store result to V2 intermediate
	horizontally increment pointers to work scans

	increment -> next V2 intermediate pixel

	drop pixel count
   	jmp to loop2 if count != 0

	store -> next V2 intermediate pixel for next iteration
  ****************************************************************************/

  pC = LoadFragment(pF.pF,pC,&cd,&V1331FetchY2Dest);	// Y2 -> fetch
#if	defined(DEBUG) || defined(DBG)
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Body_Y2);	// inner loop body
#else
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Body);		// inner loop body
#endif
  pC = LoadFragment(pF.pF,pC,&cd,&V1331IncY2Dest);	// update Y2 dest ->
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Loop);		// loop back to top
  pC = LoadFragment(pF.pF,pC,&cd,&V1331StoreY2Dest);	// save -> next Y2

  pC = LoadFragment(pF.pF,pC,&cd,&V1331FetchU2Dest);	// U2 -> fetch
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Body);		// inner loop body
  pC = LoadFragment(pF.pF,pC,&cd,&V1331IncU2V2Dest);	// update U2 dest ->
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Loop);		// loop back to top
  pC = LoadFragment(pF.pF,pC,&cd,&V1331StoreU2Dest);	// save -> next U2

  pC = LoadFragment(pF.pF,pC,&cd,&V1331FetchV2Dest);	// V2 -> fetch
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Body);		// inner loop body
  pC = LoadFragment(pF.pF,pC,&cd,&V1331IncU2V2Dest);	// update V2 dest ->
  pC = LoadFragment(pF.pF,pC,&cd,&V1331Loop);		// loop back to top
  pC = LoadFragment(pF.pF,pC,&cd,&V1331StoreV2Dest);	// save -> next V2

  /****************************************************************************
   Load fragment which jumps back to top of horizontal,vertical filter code
  ****************************************************************************/

  pC = LoadFragment(pF.pF,pC,&cd,&HVLoop);		// loop to top...

  /****************************************************************************
   At this point all scans of the original input have been processed:

   	- original Y components exist in the detail VECTOR list
	- filtered Y components exist in the smooth VECTOR list
	- filtered U,V components are being held in the quarter-size
	  U,V intermediate buffers

   We now treat the U,V intermediate buffers as though they contained
   the original incoming pixels. We have to handle one less component since
   the filtered Y components have already been processed.

   Load fragments which process the U,V intermediates producing U2,V2 pixels
   in the smooth list as well as U,V pixels in the detail list
  ****************************************************************************/

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of input pixels.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch once filtered U,V and bump
   	store U,V to detail VECTOR list and bump
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch once filtered U,V and bump
   	store U,V to detail VECTOR list and bump
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch once filtered U,V and bump
   	store odd U,V to detail VECTOR list and bump
   	process to continue a 1:3:3:1 filter sequence
   
   	drop count of pixels
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (cleanups, etc...)
  ****************************************************************************/
  {
    unsigned char far *startU2V2H;	// -> start of hfilter code
    unsigned short sizeU2V2H;		// size of hfilter code

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2HInit);		// one-time inits

    startU2V2H = pC;			// remember start of hfilter code

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2HStart);	// top-loop inits

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2H431);		// start is 431 filter

    cd.storedPC0 = pC;					// save -> loop0

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2H1331B);	// 1331B filter

							// fixup jump into loop
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2H1331C);	// 1331C filter

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2HLoop);		// end of loop code

    pC = LoadFragment(pF.pF,pC,&cd,&U2V2HEnd);		// 134 + cleanups

    sizeU2V2H = pC - startU2V2H;	// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers
    cd.storedPC1 = pC;			// remember pc: will be loop top

    pC = CopyMem(startU2V2H,pC,sizeU2V2H);// 2nd line prefetch

    pC = LoadFragment(pF.pF,pC,&cd,&V1331EarlyOut);	// vert 134 early out

    pC = CopyMem(startU2V2H,pC,sizeU2V2H);// 3rd line prefetch

						 	// fix early jump out
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);

    // insert code which does vertical filtering on the contents of the
    // work buffers
    pC = LoadFragment(pF.pF,pC,&cd,&U2V2VBody);		// vertical code

    // finally loop back up to horizontal filtering code from the
    // intermediate buffers into the work buffers.
    pC = LoadFragment(pF.pF,pC,&cd,&HVLoop);		// loop to PC1
  }

  /****************************************************************************
   Load fragment which pops used registers and returns from compiled code
  ****************************************************************************/

  pC = LoadFragment(pF.pF,pC,&cd,&cExit);		// exit code


  /****************************************************************************
   ****************************************************************************
	Begin compilation of code to recreate smooth vectors from the
	detail vectors
   ****************************************************************************
  ****************************************************************************/

  // remember far function -> to this function
#if	!defined(WIN32)
  pP->pRecreateCode.parts.selector = pP->pCode.parts.selector;
  pP->pRecreateCode.parts.offset = pC - (unsigned char far *) pP;
#else
  pP->pRecreateCode.pRecreateSmoothFromDetail = (void (far *)(void)) (pC);
#endif

  pC = LoadFragment(pF.pF,pC,&cd,&cEntry);		// entry code

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of detail Y's.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch detail Y
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch detail Y
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch detail Y
   	process to continue a 1:3:3:1 filter sequence
   
   	bump detail list -> to next Y pair
   	drop count of pixel pairs
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (pop registers, etc...)
  ****************************************************************************/
  {
    unsigned char far *startH1331;	// -> start of hfilter code
    unsigned short sizeH1331;		// size of hfilter code

    pC = LoadFragment(pF.pF,pC,&cd,&rY_H1331Init);	// one-time inits

    startH1331 = pC;			// remember start of hfilter code
    pC = LoadFragment(pF.pF,pC,&cd,&rY_H1331Start);	// top-loop inits

    pC = LoadFragment(pF.pF,pC,&cd,&rY_H431);		// start is 431 filter

    cd.storedPC0 = pC;					// save -> loop0

    pC = LoadFragment(pF.pF,pC,&cd,&rY_H1331B);		// 1331B filter

							// fixup jump into loop
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);

    pC = LoadFragment(pF.pF,pC,&cd,&rY_H1331C);		// 1331C filter

    pC = LoadFragment(pF.pF,pC,&cd,&H1331Loop);		// end of loop code

    pC = LoadFragment(pF.pF,pC,&cd,&rY_H1331End);	// 134 + cleanups

    sizeH1331 = pC - startH1331;		// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    cd.storedPC1 = pC;			// remember pc: will be loop top

    pC = CopyMem(startH1331,pC,sizeH1331);		// 2nd line prefetch

    pC = LoadFragment(pF.pF,pC,&cd,&V1331EarlyOut);	// vert 134 early out

    pC = CopyMem(startH1331,pC,sizeH1331);		// 3rd line prefetch

						 	// fix early jump out
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);
  }

  /****************************************************************************
     Compile code which vertically filters the Y data into Y2 forms

     Code will be generated that has the following logic
   
	load -> Y2 smooth list (start with even pixel ->)

	load work buffer registers
     loop0:
        fetch 4 vertically aligned pixels of Y from work scans and calculate
	    a + 3b + 3c + d
	store result to smooth Y2 list
	horizontally increment pointers to work scans

	increment -> next smooth Y2 pixel

	drop pixel count
   	jmp to loop0 if count != 0

	store -> next smooth Y2 pixel for next iteration
  ****************************************************************************/

  pC = LoadFragment(pF.pF,pC,&cd,&rY_V1331Body);	// inner loop body

  /****************************************************************************
     Start compilation of inner looping construct which processes each scan
     of once-filtered U's and V's.

     Code will be generated that has the following logic
   
	entry code (push registers, load work registers, etc...)

   	fetch once-filtered U,V from detail list
   	process to begin a 4:3:1 filter sequence
   	jump to loop1
   
     loop0:
   
   	fetch once-filtered U,V from detail list
   	process to continue a 1:3:3:1 filter sequence
   
     loop1:
   
   	fetch once-filtered U,V from detail list
   	process to continue a 1:3:3:1 filter sequence
   
   	bump detail list -> to next U,V pair
   	drop count of pixel pairs
   	jmp to loop0 if count != 0
   
   	process to end a 1:3:4 filter sequence

	exit code (pop registers, etc...)
  ****************************************************************************/
  {
    unsigned char far *startH1331;	// -> start of hfilter code
    unsigned short sizeH1331;		// size of hfilter code

    pC = LoadFragment(pF.pF,pC,&cd,&rUV_H1331Init);	// one-time inits

    startH1331 = pC;			// remember start of hfilter code
    pC = LoadFragment(pF.pF,pC,&cd,&rUV_H1331Start);	// top-loop inits

    pC = LoadFragment(pF.pF,pC,&cd,&rUV_H431);		// start is 431 filter

    cd.storedPC0 = pC;					// save -> loop0

    pC = LoadFragment(pF.pF,pC,&cd,&rUV_H1331B);	// 1331B filter

							// fixup jump into loop
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);

    pC = LoadFragment(pF.pF,pC,&cd,&rUV_H1331C);	// 1331C filter

    pC = LoadFragment(pF.pF,pC,&cd,&H1331Loop);		// end of loop code

    pC = LoadFragment(pF.pF,pC,&cd,&rUV_H1331End);	// 134 + cleanups

    sizeH1331 = pC - startH1331;		// size of hfilter code

    // we need to duplicate the above code for 2nd and 3rd prefetches of scans
    // into the work buffers

    cd.storedPC1 = pC;			// remember pc: will be loop top

    pC = CopyMem(startH1331,pC,sizeH1331);		// 2nd line prefetch

    pC = LoadFragment(pF.pF,pC,&cd,&V1331EarlyOut);	// vert 134 early out

    pC = CopyMem(startH1331,pC,sizeH1331);		// 3rd line prefetch

						 	// fix early jump out
    *cd.pJmpToLoop1 = (pC - ((unsigned char far *) cd.pJmpToLoop1) - 4);
  }

  /****************************************************************************
     Compile code which vertically filters the U,V data into twice-filtered
     forms.

     Code will be generated that has the following logic:
   
	load -> UV smooth list (start with even pixel ->)

	load work buffer registers
     loop0:
        fetch 4 vertically aligned pixels of U,V from work scans and calculate
	    a + 3b + 3c + d
	store result to smooth U,V list
	horizontally increment pointers to work scans

	increment -> next smooth U,V pixel

	drop pixel count
   	jmp to loop0 if count != 0

	store -> next smooth U,V pixel for next iteration
  ****************************************************************************/

  pC = LoadFragment(pF.pF,pC,&cd,&rUV_V1331Body);	// inner loop body

  /****************************************************************************
   Load fragment which pops used registers and returns from compiled code
  ****************************************************************************/

  pC = LoadFragment(pF.pF,pC,&cd,&cExit);		// exit code

  /****************************************************************************
				End Compilation
  ****************************************************************************/

#if	((defined(DEBUG) || defined(DBG)) && defined(STATISTICS)) || defined(STATISTICS_ALWAYS)
  if (((unsigned short) (pC - startPC)) > genCodeSize) {
    genCodeSize = (unsigned short) (pC - startPC);// latch larger codesize...
  }
  {
   TCHAR foo[128];
#if	!defined(WIN32)
   wsprintf(foo,"Code Size: %d, Code Selector: 0x%x\n",genCodeSize,
   	pP->pCode.parts.selector);
#else
   wsprintf(foo,TEXT("Code Size: %d, Code Offset: 0x%lx\n"),(long) genCodeSize,
   	(unsigned long) pP->pCode.pDIBToYUV);
#endif
   MessageBox(NULL,foo,TEXT("DIBToYUV"),MB_OK);
  }
#endif

  // return success if our compiled code did not exceed the amount of
  // space that we had presumed would be needed for it
  return (((unsigned short) (pC - startPC)) <= CODEMAXSIZE);
}


/**********************************************************************
 *
 * DIBToYUVBegin()
 *
 * Initializations for ensuing calls to DIBToYUV()
 *
 **********************************************************************/

int DIBToYUVBegin(
  CCONTEXT *pC,				// compression context
  void far *pLookup			// -> 8 bpp lookup table
) {
  union {
    DIBTOYUVPRIVATE far *pP;		// -> private area for intermediates
    PTRPARTS parts;			// access to offset,selector
  } pP;
  unsigned long sizePrivate;		// size of private area

  // allocate space that we'll need for the following:
  //
  //	data for compiled code	--> size is sizeof(DIBTOYUVPRIVATE)
  //	compiled code		--> reserve CODEMAXSIZE bytes
  //	YUV intermediates	--> max size dependent on width,height
  //				    of the max size tile
  //	work buffers		--> room for 4 Y2, 4 U, 4 V scans
  sizePrivate = sizeof(DIBTOYUVPRIVATE) + CODEMAXSIZE +
	        (unsigned long) ( // 2 quarter-size spots for U,V
		 		 (unsigned long) (pC->FrameWidth >> 1) *
		 		 (unsigned) pC->pT[0]->Height
				) +
		(pC->FrameWidth * 6);	// space for 4 half-width scans of YUV
  if (pP.pP = (DIBTOYUVPRIVATE far *) GlobalAllocPtr(
  						     GMEM_MOVEABLE,
						     sizePrivate
						    )
     ) {

#if	!defined(WIN32)
    //
    // make sure the private area cannot move in linear memory (this we must
    // do since we are going to create a code alias for this memory)
    //
    GlobalFix((HGLOBAL)GlobalHandle(pP.parts.selector));

    // create and remember a code -> to the compiled function
    if (!(pP.pP->pCode.parts.selector = AllocSelector(NULL))) {
      goto fail_begin_0;
    }
    if (!PrestoChangoSelector(pP.parts.selector,pP.pP->pCode.parts.selector)) {
      goto fail_begin_1;
    }
    { // now transform the USE16 code descriptor into a USE32 descriptor
      char desc[8];

      _asm {
	push	di
	les	bx,pP.pP		// ES:BX is -> private area
	mov	bx,es:[bx].pCode.parts.selector // get info on selector
	mov	ax,ss
	mov	es,ax
	lea	di,desc			// ES:DI is -> desc array
	mov	ax,0bh			// DPMI: get descriptor
	int	031h			// get descriptor
	or	es:[di][6],040h		// make USE32
	mov	ax,0ch			// DPMI: set descriptor
	int	031h			// set descriptor
	pop	di
      }
    }
    pP.pP->pCode.parts.offset = pP.parts.offset + sizeof(DIBTOYUVPRIVATE);
#else
    pP.pP->pCode.pDIBToYUV = (void (far *)(void)) (
        (unsigned char far *) pP.pP + sizeof(DIBTOYUVPRIVATE)
    );
#endif

    // compile the code based on input parameters
    if (!CompileFragments(
    		          pP.pP,	// -> private context
		          pC->FrameWidth,// width of the maximum width tile
		          pC->pT[0]->Height,// height of the maximum size tile
		          pC->DIBWidth,	// width of incoming DIB
		          pC->DIBYStep,	// bytes from (x,y) to (x,y+1) in DIB
		          pC->DIBType,	// type of incoming DIB
		          pLookup	// -> lookup table for 8 bpp
		         )
       ) { // some error during compilation...
      goto fail_begin_1;
    }

    // remember -> to our private context in the global compression context
    pC->pDIBToYUVPrivate = pP.pP;

#if	defined(DEBUG) || defined(DBG)
  pP.pP->bitsYStep = pC->DIBYStep;
#if	!defined(WIN32)
  pP.pP->privBase = GetSelectorBase(SELECTOROF(pP.pP)) +
  	sizeof(DIBTOYUVPRIVATE) + CODEMAXSIZE;
#else
  pP.pP->privBase = (
      (unsigned long) pP.pP + sizeof(DIBTOYUVPRIVATE) + CODEMAXSIZE
  );
#endif
  pP.pP->privLimit = pP.pP->privBase + (sizePrivate - CODEMAXSIZE -
   	sizeof(DIBTOYUVPRIVATE)) - 1;
#endif
  }

  return (1);				// success

  //
  // We arrive here for all failure conditions from this function
  //

fail_begin_1:

#if	!defined(WIN32)
  SetSelectorBase(pP.pP->pCode.parts.selector,0L);
  SetSelectorLimit(pP.pP->pCode.parts.selector,0L);
  FreeSelector(pP.pP->pCode.parts.selector);

fail_begin_0:  // failure: clean up memory allocated for private use

  GlobalUnfix((HGLOBAL)GlobalHandle(pP.parts.selector));
#endif
  GlobalFreePtr(pP.pP);

  return (0);				// failure
}


/**********************************************************************
 *
 * DIBToYUV()
 *
 * Make the intermediate YUV bitmaps from the incoming RGB DIB
 *
 **********************************************************************/

void DIBToYUV(
	CCONTEXT *pC,				// compression context
	TILECONTEXT *pT,			// current tile context
	unsigned long oBits			// 32 bit offset of base of input tile
#if	!defined(WIN32)
	,unsigned short sBits			// selector to input tile
#endif
)
{
	DIBTOYUVPRIVATE far *pP;		// -> our private context
	
	/*
	  Transform tile of incoming DIB to internal YUV form.
	
	  Let w be the width of the tile rounded up to the next 0 mod 4
	  boundary, and h be the height of the tile similarly rounded.
	
	  Then the internal YUV form consists of:
	
	  1.  Y, a full size w*h bitmap of
	 Y[i] = ((4*G[i] + 2*R[i] + B[i]) + 3.5) / 7, range 0..255
	
	  2.  U, a 1/4 size (w/2)*(h/2) bitmap of U[i], range 0..255, filtered
	 with 431,1331,...,1331,134 in x and y from
	 u[i] = 128 + ((B[i] - Y[i]) / 2)
	
	  3.  V, a 1/4 size (w/2)*(h/2) bitmap of V[i], range 0..255, filtered
	 with 431,1331,...,1331,134 in x and y from
	 v[i] = 128 + ((R[i] - Y[i]) / 2)
	
	  4.  Y2, a 1/4 size (w/2)*(h/2) bitmap of Y2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from Y[i].
	
	  5.  U2, a 1/16 size (w/4)*(h/4) bitmap of U2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from U[i].
	
	  6.  V2, a 1/16 size (w/4)*(h/4) bitmap of V2[i], range 0..255,
	 filtered with 431,1331,...,1331,134 from V[i].
	
	  The 431,1331,...,1331,431 filtering means that given a scanline of
	  input i0,i1,i2,i3,i4,i5,i6,i7 we produce an output of o0,o1,o2,o3
	  where
	    o0 = 1*i0 + 3*i0 + 3*i1 + 1*i2 = 4*i0 + 3*i1 + 1*i2
	    o1 = 1*i1 + 3*i2 + 3*i3 + 1*i4
	    o2 = 1*i3 + 3*i4 + 3*i5 + 1*i6
	    o3 = 1*i5 + 3*i6 + 3*i7 + 1*i7 = 1*i5 + 3*i6 + 4*i7
	  with similar filtering done in y.
	
	  We remember #1, #2 and #3 above as an array of VECTOR elements,
	  called pC->VBook[Detail].pVectors, and #4, #5 and #6 as a similar
	  array called pC->VBook[Smooth].pVectors.
	
	  Each 2x2 patch of incoming DIB thus comprises a 6 element detail
	  vector, with 4 luminance values and 2 chroma values.  Each 4x4 patch
	  of incoming DIB comprises a 6 element smooth vector, also with 4
	  luma and 2 chroma.  Whether a given 4x4 patch is treated as 4 2x2
	  detail vectors or 1 4x4 smooth vector is decided later.
	
	  !!!
	    One VECTOR area is allocated per tile (for the prev tile) plus one
	    for the current tile at CompressBegin time.  They are shuffled
	    around as new tiles come in, so each tile has a prev when needed.
	  !!!
	 */
	
	/*
	  !!!
	    note that the internal form is an array of VECTOR but could be
	    built as separated arrays of Y then U then V as long as they are
	    shuffled before return
	  !!!
	 */
	
	// get a local -> to the private context
	pP = pC->pDIBToYUVPrivate;
	
	//
	// load up private data area with parameters needed for compiled code
	//

	pP->tileHeight = pT->Height;		// 0 mod 4 height of this tile
	pP->srcHeight = (unsigned) pT->DIBHeight - 1;// real height of tile - 1
	
						// flat -> incoming pixels
#if	!defined(WIN32)
	pP->oBits = GetSelectorBase(sBits) + oBits;
#else
	pP->oBits = oBits;
#endif
#if	defined(DEBUG) || defined(DBG)
  if (pP->bitsYStep < 0) {
    pP->bitsLimit = pP->oBits - pP->bitsYStep - 1;
    pP->bitsBase = pP->oBits + (pP->srcHeight * pP->bitsYStep);
  }
  else {
    pP->bitsBase = pP->oBits;
    pP->bitsLimit = pP->oBits + ((pP->srcHeight + 1) * pP->bitsYStep) - 1;
  }
#endif

	// flat ->s to detail VECTOR parts
#if	!defined(WIN32)
	pP->oDetail = GetSelectorBase(SELECTOROF(pC->VBook[Detail].pVectors)) +
		OFFSETOF(pC->VBook[Detail].pVectors);
#else
	pP->oDetail = (unsigned long) pC->VBook[Detail].pVectors;
#endif
#if	defined(DEBUG) || defined(DBG)
  pP->detailBase = pP->oDetail;
  pP->detailLimit = pP->detailBase + (
  				      (
				       (unsigned long) pT->nPatches *
				       sizeof(VECTOR)
				      ) << 2
				     ) - 1;
#endif

	// flat ->s to smooth VECTOR parts
#if	!defined(WIN32)
	pP->oSmooth = GetSelectorBase(SELECTOROF(pC->VBook[Smooth].pVectors)) +
		OFFSETOF(pC->VBook[Smooth].pVectors);
#else
	pP->oSmooth = (unsigned long) pC->VBook[Smooth].pVectors;
#endif
#if	defined(DEBUG) || defined(DBG)
  pP->smoothBase = pP->oSmooth;
  pP->smoothLimit = pP->smoothBase + (pT->nPatches * sizeof(VECTOR)) - 1;
#endif

	/*
	  Invoke the bottom-level do'er
	
	  All additional runtime parameters are initialized in the entry code of
	  the called code
	*/
#if	!defined(WIN32) && (defined(DEBUG) || defined(DBG))
  {
    extern unsigned short flatSel;

    if (GetSelectorBase(flatSel) != 0) {
      _asm int 3
    }
  }
#endif
printf("CALLING DIB!\n");
	(*pP->pCode.pDIBToYUV)();

	return;
}


/**********************************************************************
 *
 * RecreateSmoothFromDetail()
 *
 * Recreate the smooth vector list from the detail vector list
 *
 **********************************************************************/

void RecreateSmoothFromDetail(
  CCONTEXT *pC,				// compression context
  TILECONTEXT *pT			// current tile context
) {
  DIBTOYUVPRIVATE far *pP;		// -> our private context

  // get a local -> to the private context
  pP = pC->pDIBToYUVPrivate;

  //
  // load up private data area with parameters needed for compiled code
  //

  pP->tileHeight = pT->Height;		// 0 mod 4 height of this tile

  					// flat ->s to detail VECTOR parts
#if	!defined(WIN32)
  pP->oDetail = GetSelectorBase(SELECTOROF(pC->VBook[Detail].pVectors)) +
	        OFFSETOF(pC->VBook[Detail].pVectors);
#else
  pP->oDetail = (unsigned long) pC->VBook[Detail].pVectors;
#endif
#if	defined(DEBUG) || defined(DBG)
  pP->detailBase = pP->oDetail;
  pP->detailLimit = pP->detailBase + (
  				      (
				       (unsigned long) pT->nPatches *
				       sizeof(VECTOR)
				      ) << 2
				     ) - 1;
#endif

  					// flat ->s to smooth VECTOR parts
#if	!defined(WIN32)
  pP->oSmooth = GetSelectorBase(SELECTOROF(pC->VBook[Smooth].pVectors)) +
  		OFFSETOF(pC->VBook[Smooth].pVectors);
#else
  pP->oSmooth = (unsigned long) pC->VBook[Smooth].pVectors;
#endif
#if	defined(DEBUG) || defined(DBG)
  pP->smoothBase = pP->oSmooth;
  pP->smoothLimit = pP->smoothBase + (pT->nPatches * sizeof(VECTOR)) - 1;
#endif

  /*
    Invoke the bottom-level do'er

    All additional runtime parameters are initialized in the entry code of
    the called code
  */
#if	!defined(WIN32) && (defined(DEBUG) || defined(DBG))
  {
    extern unsigned short flatSel;

    if (GetSelectorBase(flatSel) != 0) {
      _asm int 3
    }
  }
#endif
#if	defined(DEBUG) || defined(DBG)
  {
    unsigned long css = 0L;
    int i;
    VECTOR huge *pv;
    TCHAR msg[64];
    
    for (
         pv = pC->VBook[Smooth].pVectors,
	   i = ((pC->FrameWidth >> 2) * (pT->Height >> 2));
	 i--;
	 pv++
        ) {
      css += (unsigned short) pv->yuv.y[0] + pv->yuv.y[1] + pv->yuv.y[2] +
      	     pv->yuv.y[3] + pv->yuv.u + pv->yuv.v;
    }
    wsprintf(msg,TEXT("Smooth checksum before = %lX"),css);
    MessageBox(NULL,msg,TEXT("RecreateSmoothFromDetail"),MB_OK);
  }
#endif
  (*pP->pRecreateCode.pRecreateSmoothFromDetail)();
#if	defined(DEBUG) || defined(DBG)
  {
    unsigned long css = 0L;
    int i;
    VECTOR huge *pv;
    TCHAR msg[64];
    
    for (
         pv = pC->VBook[Smooth].pVectors,
	   i = ((pC->FrameWidth >> 2) * (pT->Height >> 2));
	 i--;
	 pv++
        ) {
      css += (unsigned short) pv->yuv.y[0] + pv->yuv.y[1] + pv->yuv.y[2] +
      	     pv->yuv.y[3] + pv->yuv.u + pv->yuv.v;
    }
    wsprintf(msg,TEXT("Smooth checksum after = %lX"),css);
    MessageBox(NULL,msg,TEXT("RecreateSmoothFromDetail"),MB_OK);
  }
#endif

  return;
}


/**********************************************************************
 *
 * DIBToYUVEnd()
 *
 * Clean up after previous call to DIBToYUVBegin()
 *
 **********************************************************************/

void DIBToYUVEnd(
  CCONTEXT *pC				// compression context
) {
  DIBTOYUVPRIVATE far *pP;		// -> our private context

  if (pP = pC->pDIBToYUVPrivate) { // private context was allocated...

#if	!defined(WIN32)
    // free up the code selector
    SetSelectorBase(pP->pCode.parts.selector,0L);
    SetSelectorLimit(pP->pCode.parts.selector,0L);
    FreeSelector(pP->pCode.parts.selector);

    // finally free up space allocated to the private context
    GlobalUnfix((HGLOBAL)GlobalHandle(SELECTOROF(pP)));
#endif
    GlobalFreePtr(pP);

    pC->pDIBToYUVPrivate = (void far *) 0;// flag uninitialized
  }
}
