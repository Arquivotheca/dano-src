//--------------------------------------------------------------------
//	
//	DrawEngine.h
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _DRAWENGINE_H_
#define _DRAWENGINE_H_

/* Enable/disable support for different file format revision */
#define	ENABLE_REVISION_0	1
#define	ENABLE_REVISION_1	1
//#define ENABLE_DRAWING		1

#include "LBX.h"
#include "Unpack.h"

class LBX_Container;

#if ENABLE_DRAWING
/*--------------------------- Private class. Don't use. */
class LBX_Stepper {
public:
	virtual			~LBX_Stepper();
		
	virtual void	SeekToLine(uint32 index, uint8 **ptr) = NULL;
	virtual void	NextLine(uint8 **ptr) = NULL;
};

/*---------------------------- Here are the public API you should use */
class LBX_DrawEngine : public LBX_Unpack {
public:
	LBX_DrawEngine(LBX_Container *ctxt, uint32 bitmap_index);
	~LBX_DrawEngine();
	
	void		DrawFirstLine(uint32 *dst, uint32 left, uint32 top, uint32 right);
	void		DrawNextLine(uint32 *dst);
	void		DrawRect(uint16 *dst, uint32 row, uint32 left, uint32 top, uint32 right, uint32 bottom);
	
private:
	uint16		left, right;
	uint8		*ptr[3];
	LBX_Stepper	*stepper;
};
#endif

// Call this function, don't call the constructor of LBX_Container directly.
extern LBX_Container *LBX_BuildContainer(uint8 *buffer);

class LBX_Container {
friend class LBX_DrawEngine;
public:
	LBX_Container(uint8 *buffer);		
	virtual ~LBX_Container();
	
	bool				IsRotated() { return status & 1; };
	uint32				BitmapCount() { return bitmap_count; };
	virtual void		GetBitmapName(uint32 index, char *name) = NULL;
	virtual uint16		BitmapWidth(uint32 index) = NULL;
	virtual uint16		BitmapHeight(uint32 index) = NULL;
	virtual int32		GetIndexFromName(char *name) = NULL; // return -1 if unknown
#if ENABLE_DRAWING
	virtual LBX_Stepper	*Stepper(uint32 bitmap_index) = NULL;

private:
	virtual uint16		*CmapBuffer(uint32 index) = NULL;
#endif
	
protected:
#if ENABLE_DRAWING
	uint8				*pixel_buffer;
	uint16				*cmap_list;
#endif
	uint8				bitmap_count;
	uint8				status;
};

#endif
