//========================================================================
//	MSectionLine.h
//========================================================================

#ifndef _MSECTIONLINE_H
#define _MSECTIONLINE_H

#include "IDEConstants.h"
#include "MProjectLine.h"
#include "CString.h"
#include <string.h>

class BList;

struct SectionBlock
{
	void		SwapHostToBig();
	void		SwapBigToHost();

	uint32		sLinesInSection;
	uint32		sCodeSize;
	uint32		sDataSize;
	bool		sExpanded;
	char		sName[B_FILE_NAME_LENGTH_DR8];
	uchar		sunused1;
	uchar		sunused2;
};



class MSectionLine : public MProjectLine
{
public:
								MSectionLine(
									MProjectView& inProjectView,
									BList& inFileList);

								MSectionLine(
									MProjectView& 	inProjectView, 
									BList& 			inFileList,
									MBlockFile&		inFile);

	virtual						~MSectionLine();
					
	virtual	void				Draw(
									BRect inFrame, 
									BRect inIntersection, 
									MProjectView& inView);

	virtual bool				DoClick(
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual bool				SelectImmediately( 
									BRect 			inFrame, 
									BPoint 			inPoint,
									bool 			inIsSelected,
									uint32			inModifiers,
									uint32			inButtons);
	virtual void				Invoke();
			bool				DoArrowKey(
									BRect	inFrame,
									uint32	inArrowMessage,
									bool	inIsSelected);

	virtual void				BuildPopupMenu(
									MPopupMenu & inMenu) const;

	void						RemoveLine(
									MProjectLine* inLine);
	void						AddLine(
									MProjectLine* inLine);

	void						SetName(
									const char* inName);

	void						WriteToFile(
									MBlockFile & inFile);

	void						SetFirstLine(
									MProjectLine* inLine)
								{
									fFirstLineInSection = inLine;
								}

	MProjectLine*				GetFirstLine() const
								{
									return fFirstLineInSection;
								}

	uint32						GetLines() const
								{
									return fLinesInSection;
								}

	bool						IsExpanded() const
								{
									return fExpanded;
								}
	virtual const char *		Name() const
								{
									return fName;
								}
	virtual void				ExternalName(char* outName) const
								{
									// for sections, use "# name"
									strcpy(outName, "# ");
									strcat(outName, fName);
								}
								
	static const BBitmap*		ContractedBitmap();
	static const BBitmap*		ExpandedBitmap();
	static const BBitmap*		IntermediateBitmap();

private:

		BList&					fFileList;
		uint32					fLinesInSection;
		bool					fExpanded;
		MProjectLine*			fFirstLineInSection;
		String					fName;

	void						DrawExpanded(BRect inFrame, MProjectView& inView);
	void						DrawContracted(BRect inFrame, MProjectView& inView);
	void						DrawIntermediate(BRect inFrame);

	void						Contract(BRect inFrame, bool inIsSelected);
	void						Expand(BRect inFrame, bool inIsSelected);

static	BBitmap *				sContractedBitmap;
static	BBitmap *				sIntermediateBitmap;
static	BBitmap *				sExpandedBitmap;

static	void					InitBitmaps();
};

inline const BBitmap*		
MSectionLine::ContractedBitmap()
{
	InitBitmaps();
	return sContractedBitmap;
}
inline const BBitmap*		
MSectionLine::ExpandedBitmap()
{
	InitBitmaps();
	return sExpandedBitmap;
}
inline const BBitmap*		
MSectionLine::IntermediateBitmap()
{
	InitBitmaps();
	return sIntermediateBitmap;
}

#endif
