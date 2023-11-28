//
// TranslatorStyle.cpp
//

#include <SupportDefs.h>
#include <string.h>

#include "TranslatorStyle.h"
#include "TranslationConsts.h"

char GeneralFormatString[] = "General";

const rgb_color	OUR_BLACK = {0, 0, 0, 255};
const rgb_color	OUR_WHITE = {255, 255, 255, 255};

#ifdef DEBUG
static int32 PropsCreated = 0;
static int32 PropsDeleted = 0;
static int32 StylesCreated = 0;
static int32 StylesDeleted = 0;
#endif

TTranslatorStyle::TTranslatorStyle()
{
	mStyleSheet = NULL;
	mIndex = 0xFFFF;
	mWritten = false;
	IFDEBUG(StylesCreated++);
	ASSERTC(StylesCreated);
}

TTranslatorStyle::TTranslatorStyle(const TTranslatorStyle& src)
	: mStyleChunk(src.mStyleChunk)
{
	mStyleSheet = NULL;
	mIndex = 0xFFFF;
	mWritten = false;
	
	int32 count = src.CountProperties();
	for (int32 x = 0; x < count; x++)
	{
		const TTransProperty* srcPropPtr = src.GetProperty(x);
		TTransProperty* propPtr = new TTransProperty(*srcPropPtr);
		mTransProps.AddItem( propPtr );
	}
	IFDEBUG(StylesCreated++);
	ASSERTC(StylesCreated);
}

TTranslatorStyle::~TTranslatorStyle()
{
	while (mTransProps.CountItems())
		delete static_cast<TTransProperty*>(mTransProps.RemoveItem(0L));
	IFDEBUG(StylesDeleted++);
	ASSERTC(StylesDeleted);
}

int32 TTranslatorStyle::Index(void) const
{
	return mIndex;
}

status_t TTranslatorStyle::Write(TBlockStreamWriter* writer) const
{
	// If style is not part of a stylesheet or this is the first time
	// it's been called to write it self write the full description
	// of the style to the stream.
	if (!mStyleSheet || (mStyleSheet && !mWritten))
	{
		writer->BeginChunk(kStyle_id);
		mStyleChunk.Write(writer);
		WriteProperties(writer);
		writer->EndChunk(kStyle_id);
	}
	// Otherwise write just the index of the style in the stylesheet.
	else if (mStyleSheet)
		writer->WriteInt32(kStyleIndex_id, mIndex);
	
	mWritten = true;
	return writer->Error();
}

// Called by styles manager or by translator to read the definition of the
// style in a kStyle_id chunk.
status_t TTranslatorStyle::Read(TBlockStreamReader* reader)
{
	status_t err = reader->Error();
	if (!err && reader->BlockKind() != START_CHUNK_TYPE)
		err = INVALID_STYLE_ERROR;
	
	block_id id;
	while (!err && reader->NextBlock(&id))
	{
		switch (id)
		{
			case kPropIndexArray_id:
			case kPropsArray_id:
				err = ReadProperties(reader);
				break;
			default:
				mStyleChunk.AddBlockFromStream(reader);
				break;
		}
	}
	return err ? err : reader->Error();
}

status_t TTranslatorStyle::WriteProperties(TBlockStreamWriter* writer) const
{
	if (mStyleSheet)
	{
		int32 propCount = mTransPropIDs.CountItems();
		writer->BeginChunk( kPropIndexArray_id );
		writer->WriteInt32( kArrayCount_id, propCount);
		for (long x = 0; x < propCount; x++)
		{
			int32 propIndex = reinterpret_cast<int32>(mTransPropIDs.ItemAt(x));
			writer->WriteInt32( kIndex_id, propIndex );
		}
		writer->EndChunk( kPropIndexArray_id );
	}
	else
	{
		int32 propCount = mTransProps.CountItems();
		writer->BeginChunk( kPropsArray_id );
		writer->WriteInt32( kArrayCount_id, mTransProps.CountItems());
		for (long x = 0; x < propCount; x++)
		{
			TTransProperty* propPtr = static_cast<TTransProperty*>(mTransProps.ItemAt(x));
			int32 kind = propPtr->Kind();
			writer->BeginChunk( kind );
			propPtr->Write(writer);
			writer->EndChunk( kind );
		}
		writer->EndChunk( kPropsArray_id );
	}
	return writer->Error();	
}

status_t TTranslatorStyle::ReadProperties( TBlockStreamReader* reader )
{
	status_t 	err = reader->Error();
	block_id	arrayID = reader->BlockID();
	block_id	id;

	while (!err && reader->NextBlock(&id))
	{
		if (arrayID == kPropIndexArray_id && id == kIndex_id)
		{
			int32 propIndex;
			if (!reader->ReadInt32(&propIndex))
				mTransPropIDs.AddItem( (void*) propIndex );
		}
		else if (arrayID == kPropsArray_id && reader->BlockKind() == START_CHUNK_TYPE)
		{
			TTransProperty* propPtr = new TTransProperty(id);
			mTransProps.AddItem(propPtr);
			err = propPtr->Read(reader);
		}
		else
			reader->SkipBlock();
	}
	return err ? err : reader->Error();
}

int32 TTranslatorStyle::CountProperties(void) const
{
	if (mStyleSheet)
		return mTransPropIDs.CountItems();
	return mTransProps.CountItems();
}

const TTransProperty* TTranslatorStyle::GetProperty(int32 index) const
{
	if (index < 0 || index > CountProperties())
		return NULL;
		
	const TTransProperty* propPtr = NULL;
	if (mStyleSheet)
	{
		int32 propIndex = reinterpret_cast<int32>(mTransPropIDs.ItemAt(index));
		propPtr = mStyleSheet->IndexToProperty( propIndex );
	}
	else
		propPtr = reinterpret_cast<const TTransProperty*>(mTransProps.ItemAt(index));
	return propPtr;		
}

const TTransProperty* TTranslatorStyle::FindProperty(int32 kind) const
{
	for (long x = 0; x < CountProperties(); x++)
	{
		const TTransProperty* propPtr = GetProperty(x);
		if (propPtr && propPtr->Kind() == kind)
			return propPtr;
	}
	return NULL;
}

status_t TTranslatorStyle::AddProperty( TTransProperty* propPtr )
{
	if (mStyleSheet)
	{
		int32 propIndex = mStyleSheet->AddProperty( propPtr );
		mTransPropIDs.AddItem( (void*) propIndex );
	}
	else
		mTransProps.AddItem( propPtr );
	return B_NO_ERROR;
}

void TTranslatorStyle::SetStyleSheet(TTranslatorStylesTable* styleSheetPtr, int32 index)
{
	if (!styleSheetPtr || mStyleSheet)
		return;
		
	mStyleSheet = styleSheetPtr;
	mIndex = index;
	
	// Convert our properties to indexes to the stylesheet property manager.
	while (mTransProps.CountItems())
	{
		TTransProperty* propPtr = static_cast<TTransProperty*>(mTransProps.RemoveItem(0L));
		int32 propIndex = mStyleSheet->AddProperty( propPtr );
		mTransPropIDs.AddItem( (void*) propIndex );
	}
}


#pragma mark -
void TTranslatorStyle::SetStyleType(StyleType styleType)
{
	mStyleChunk.AddInt32( kKind_id, styleType ); 
}

void TTranslatorStyle::SetStyleName(const char* styleName)
{
	mStyleChunk.AddString( kName_id, styleName ); 
}

void TTranslatorStyle::SetBaseStyleName(const char* styleName)
{
	mStyleChunk.AddString( kBaseStyleNameID, styleName ); 
}

void TTranslatorStyle::SetApplyStyleName(const char* styleName)
{
	mStyleChunk.AddString( kApplyStyleNameID, styleName ); 
}

bool TTranslatorStyle::GetStyleType(StyleType& styleType) const
{
	return mStyleChunk.FindInt32( kKind_id, (int32&) styleType );
}

bool TTranslatorStyle::GetStyleName(const char** str) const
{
	const char* theStr = NULL;
	if (mStyleChunk.FindString( kName_id, &theStr ) && theStr && *theStr)
	{
		*str = theStr;
		return true;
	}
	return false;
//	return mStyleChunk.FindString( kName_id, str );
}

bool TTranslatorStyle::HasStyleName(void) const
{
	const char* theStr = NULL;
	if (mStyleChunk.FindString( kName_id, &theStr ) && theStr && *theStr)
		return true;
	return false;
}

bool TTranslatorStyle::GetBaseStyleName(const char** str) const
{
	if (mStyleChunk.FindString( kBaseStyleNameID, str ))
		return true;
	int32 styleIndex;
	if (!mStyleSheet || !GetBaseStyleIndex(styleIndex) || styleIndex == -1)
		return false;
	TTranslatorStyle* stylePtr = (*mStyleSheet)[styleIndex];
	if (stylePtr)
		return stylePtr->GetStyleName(str);
	return false;
}

bool TTranslatorStyle::GetBaseStyleIndex(int32& styleIndex) const
{
	return mStyleChunk.FindInt32( kBaseStyleIndex_id, styleIndex );
}

bool TTranslatorStyle::GetApplyStyleName(const char** str) const
{
	if (mStyleChunk.FindString( kApplyStyleNameID, str ))
		return true;
	int32 styleIndex;
	if (!mStyleSheet || !GetApplyStyleIndex(styleIndex) || styleIndex == -1)
		return false;
	TTranslatorStyle* stylePtr = (*mStyleSheet)[styleIndex];
	if (stylePtr)
		return stylePtr->GetStyleName(str);
	return false;
}

bool TTranslatorStyle::GetApplyStyleIndex(int32& styleIndex) const
{
	return mStyleChunk.FindInt32( kApplyStyleIndex_id, styleIndex );
}

#pragma mark -
void TTranslatorStyle::SetTextFont(int32 fontID)
{
//	BFont		font;
//	font_family	family;
//	font_style	style;
//	
//	memset(family, 0, sizeof(font_family));
//	memset(style, 0, sizeof(font_style));
//	
//	font.SetFamilyAndStyle(fontID);
//	font.GetFamilyAndStyle(&family, &style);
//	SetTextFont(family, style);

	TTransProperty* propPtr = new TTransProperty( kPropTextFont );
	propPtr->AddInt32( kFontID_id, fontID );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetTextFont(const font_family fontName, const font_style fontStyle)
{
	BFont font;
	font.SetFamilyAndStyle(fontName, fontStyle);
	int32 fontID = (int32) font.FamilyAndStyle();

	TTransProperty* propPtr = new TTransProperty( kPropTextFont );
	propPtr->AddString( kFontName_id, fontName );
	propPtr->AddString( kFontStyle_id, fontStyle );
//	propPtr->AddInt32( kFontID_id, fontID );
	AddProperty( propPtr );
	
	if (strstr(fontStyle, "Bold"))	// local. bogus
	{
		propPtr = new TTransProperty( kPropTextFace );
		propPtr->AddInt32(0, kBoldFace);
		propPtr->AddInt32(1, true);
		AddProperty( propPtr );
	}
	
	if (strstr(fontStyle, "Italic"))	// local. bogus
	{
		propPtr = new TTransProperty( kPropTextFace );
		propPtr->AddInt32(0, kItalicFace);
		propPtr->AddInt32(1, true);
		AddProperty( propPtr );
	}
}

void TTranslatorStyle::SetTextSize(float textSize)
{
	TTransProperty* propPtr = new TTransProperty( kPropTextSize );
	propPtr->AddFloat( kValue_id, textSize );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetTextFace(int32 textFace, bool value)
{
	TTransProperty* propPtr = new TTransProperty( kPropTextFace );
	propPtr->AddInt32(0, textFace);
	propPtr->AddInt32(1, value);
	AddProperty( propPtr );
}

void TTranslatorStyle::SetTextAlignment(int32 align, bool horizontal)
{
	TTransProperty* propPtr = new TTransProperty( horizontal ? kPropHAlign : kPropVAlign );
	propPtr->AddInt16( kValue_id, align );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetTextColor(rgb_color high, rgb_color low, pattern pat)
{
	AddColor( kPropTextColor, high, low, pat );
}

void TTranslatorStyle::SetFillColor(rgb_color high, rgb_color low, pattern pat)
{
	AddColor( kPropFillColor, high, low, pat );
}

void TTranslatorStyle::SetPenColor(rgb_color high, rgb_color low, pattern pat)
{
	AddColor( kPropPenColor, high, low, pat );
}

void TTranslatorStyle::SetTextOpacity(float value)
{
	TTransProperty* propPtr = new TTransProperty( kPropTextOpacity );
	propPtr->AddFloat( kValue_id, value );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetFillOpacity(float value)
{
	TTransProperty* propPtr = new TTransProperty( kPropFillOpacity );
	propPtr->AddFloat( kValue_id, value );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetPenOpacity(float value)
{
	TTransProperty* propPtr = new TTransProperty( kPropPenOpacity );
	propPtr->AddFloat( kValue_id, value );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetNumberFormat(const char* formatStr)
{
	TTransProperty* propPtr = new TTransProperty( kPropNumFormat );
	propPtr->AddString( kValue_id, formatStr );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetDashPen(float dashWidth, float gapWidth, const char* name)
{
	TTransProperty* propPtr = new TTransProperty( kPropPen );
	propPtr->AddInt32(kKind_id, 'DASH');
	if (name)
		propPtr->AddString(kName_id, name);
	propPtr->AddFloat(2, dashWidth);
	propPtr->AddFloat(3, gapWidth);
	AddProperty( propPtr );
}

void TTranslatorStyle::SetFirstLineIndent(float value)
{
	TTransProperty* propPtr = new TTransProperty( kPropParIndent );
	propPtr->AddFloat( kValue_id, value );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetLeftIndent(float value)
{
	TTransProperty* propPtr = new TTransProperty( kPropParLeft );
	propPtr->AddFloat( kValue_id, value );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetRightIndent(float value)
{
	TTransProperty* propPtr = new TTransProperty( kPropParRight );
	propPtr->AddFloat( kValue_id, value );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetLineSpacing(float value, bool lines)
{
	AddParagraphSpacing( kPropParLineSpacing, value, lines );
}

void TTranslatorStyle::SetSpaceBefore(float value, bool lines)
{
	AddParagraphSpacing( kPropParSpaceBefore, value, lines );
}

void TTranslatorStyle::SetSpaceAfter(float value, bool lines)
{
	AddParagraphSpacing( kPropParSpaceAfter, value, lines );
}

void TTranslatorStyle::SetParagraphAlignment(int32 type)
{
	TTransProperty* propPtr = new TTransProperty( kPropParAlign );
	propPtr->AddInt16( kValue_id, type );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetParagraphLabel(int32 type)
{
	TTransProperty* propPtr = new TTransProperty( kPropParLabel );
	propPtr->AddInt32( kValue_id, type );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetTab(float pos, int32 type, int32 fillType, const uchar* alignChar, const uchar* fillChar)
{
	TTransProperty* propPtr = new TTransProperty( kPropParTab );
	propPtr->AddFloat(0, pos);
	propPtr->AddInt32(1, type);
	propPtr->AddInt32(2, fillType);
	if (alignChar)
		propPtr->AddInt32( 3, ReverseFlattenUTF8(alignChar));
	if (fillChar)
		propPtr->AddInt32( 4, ReverseFlattenUTF8(fillChar));
	AddProperty( propPtr );
}

void TTranslatorStyle::SetBorderColor(int32 border, rgb_color high, rgb_color low, pattern pat)
{
	// Create ink property with ink chunk as its value.
	TTransProperty* propPtr = new TTransProperty(kPropBorderColor);
	if (propPtr)
	{
		propPtr->AddInt32(kIndex_id, border);
		AddColorToProp(propPtr, high, low, pat);
		AddProperty( propPtr );
	}
}

void TTranslatorStyle::SetBorderSize(int32 border, float size)
{
	// BOGUS
}

void TTranslatorStyle::SetBorderDashPen(int32 border, float dashWidth, float gapWidth, const char* name)
{
	// BOGUS
}

void TTranslatorStyle::SetCellWrap(bool wrap)
{
	TTransProperty* propPtr = new TTransProperty( kPropCellWrap );
	propPtr->AddBool( kValue_id, wrap );
	AddProperty( propPtr );
}

void TTranslatorStyle::SetCellLock(bool lock)
{
	TTransProperty* propPtr = new TTransProperty( kPropCellLock );
	propPtr->AddBool( kValue_id, lock );
	AddProperty( propPtr );
}

#pragma mark -
bool TTranslatorStyle::GetFont(BFont& font) const
{
	font_family fontName;
	font_style	fontStyle;

	float textSize;
	GetTextSize(textSize);
	font.SetSize(textSize);

	bool hasFont = GetTextFont(fontName, fontStyle);
	font.SetFamilyAndStyle(fontName, fontStyle);
	return hasFont;
}

bool TTranslatorStyle::GetTextFont(font_family& fontName, font_style& fontStyle) const
{
	BFont font(be_plain_font);
	font.GetFamilyAndStyle(&fontName, &fontStyle);

	const char* strPtr;
	const TTransProperty* fontChunk = FindProperty( kPropTextFont );
	if (fontChunk)
	{
		if (fontChunk->FindString( kFontName_id, &strPtr ))
			strcpy( fontName, strPtr );
		if (fontChunk->FindString( kFontStyle_id, &strPtr ))
			strcpy( fontStyle, strPtr );
	}
	else if (mStyleSheet)
	{
		// See if the style inherits font information.
		int32 styleIndex;
		bool foundInfo = false;
		if (GetApplyStyleIndex(styleIndex) && styleIndex != -1)
			foundInfo = (*mStyleSheet)[styleIndex]->GetTextFont(fontName, fontStyle);
		if (!foundInfo && GetBaseStyleIndex(styleIndex) && styleIndex != -1)
			foundInfo = (*mStyleSheet)[styleIndex]->GetTextFont(fontName, fontStyle);
	}
	
	// If either bold or italic is defined for this same style then modify
	// the font styleName accordingly.
	int32 textFace, textMask;
	if (GetTextFace(textFace, textMask))
	{
		if (textFace & kPlainFace)
			fontStyle[0] = 0;
		else if (textFace & (kBoldFace | kItalicFace))
		{
			int32 		fontStyles = count_font_styles(fontName);
			font_style	theStyle;
			bool 		wantBold = !!(textFace & kBoldFace);
			bool 		wantItalic = !!(textFace & kItalicFace);
			for (long x = 0; x < fontStyles; x++)
			{
				if (B_OK != get_font_style(fontName, x, &theStyle))
					continue;
				if (wantBold == !strstr(theStyle, "Bold"))	// local. bogus
					continue;
				if (wantItalic == !strstr(theStyle, "Italic"))	// local. bogus
					continue;
				memcpy(fontStyle, theStyle, sizeof(font_style));
				break;
			}
		}
	}
	return (fontChunk != NULL);
}

bool TTranslatorStyle::GetTextSize(float& textSize) const
{
	textSize = 12.0;
	const TTransProperty* propPtr = FindProperty( kPropTextSize );
	if (propPtr)
		propPtr->FindFloat( kValue_id, textSize );
	return (propPtr != NULL);
}

bool TTranslatorStyle::GetTextFace(int32& textFace, int32& textMask) const
{
	textFace = kPlainFace;
	textMask = 0;
	bool propsFound = false;

	for (long x = 0; x < CountProperties(); x++)
	{
		const TTransProperty* propPtr = GetProperty(x);
		if (propPtr && propPtr->Kind() == kPropTextFace)
		{
			int32 	face;
			int32	value = 1;

			propsFound = true;
			propPtr->FindInt32( 1, value );
			if ( propPtr->FindInt32( 0, face ) )
			{
				textMask |= face;
				if (value && textFace == kPlainFace)
					textFace = face;
				else if (value)
					textFace |= face;
				else
				{
					textFace &= ~face;
					if (!textFace)
						textFace = kPlainFace;
				}
			}
		}
	}
	return propsFound;
}

bool TTranslatorStyle::GetTextAlignment(int32& align, bool horizontal) const
{
	align = kAlignDefault;
	const TTransProperty* propPtr = FindProperty( horizontal ? kPropHAlign : kPropVAlign );

	int16 theAlign;
	if (propPtr && propPtr->FindInt16( kValue_id, theAlign ))
	{
		align = theAlign;
		return true;
	}	
	return false;
}

bool TTranslatorStyle::GetTextColor(rgb_color& high, rgb_color& low, pattern& pat) const
{
	return GetColor( kPropTextColor, high, low, pat );
}

bool TTranslatorStyle::GetFillColor(rgb_color& high, rgb_color& low, pattern& pat) const
{
	return GetColor( kPropFillColor, high, low, pat );
}

bool TTranslatorStyle::GetPenColor(rgb_color& high, rgb_color& low, pattern& pat) const
{
	return GetColor( kPropPenColor, high, low, pat );
}

bool TTranslatorStyle::GetTextOpacity(float& value) const
{
	return GetFloatPropertyValue( kPropTextOpacity, value );	
}

bool TTranslatorStyle::GetFillOpacity(float& value) const
{
	return GetFloatPropertyValue( kPropFillOpacity, value );	
}

bool TTranslatorStyle::GetPenOpacity(float& value) const
{
	return GetFloatPropertyValue( kPropPenOpacity, value );	
}

bool TTranslatorStyle::GetFirstLineIndent(float& value) const
{
	return GetFloatPropertyValue( kPropParIndent, value );
}

bool TTranslatorStyle::GetLeftIndent(float& value) const
{
	return GetFloatPropertyValue( kPropParLeft, value );
}

bool TTranslatorStyle::GetRightIndent(float& value) const
{
	return GetFloatPropertyValue( kPropParRight, value );
}

bool TTranslatorStyle::GetNumberFormat(const char** strPtr) const
{
	*strPtr = GeneralFormatString;
	const TTransProperty* propPtr = FindProperty( kPropNumFormat );
	if (propPtr)
		propPtr->FindString( kValue_id, strPtr );
	return (propPtr != NULL);
}

bool TTranslatorStyle::GetLineSpacing(float& value, bool& lines) const
{
	return GetParagraphSpacing( kPropParLineSpacing, value, lines );
}

bool TTranslatorStyle::GetSpaceBefore(float& value, bool& lines) const
{
	return GetParagraphSpacing( kPropParSpaceBefore, value, lines );
}

bool TTranslatorStyle::GetSpaceAfter(float& value, bool& lines) const
{
	return GetParagraphSpacing( kPropParSpaceAfter, value, lines );
}

bool TTranslatorStyle::GetParagraphAlignment(int32& type) const
{
	const TTransProperty* propPtr = FindProperty( kPropParAlign );
	if (propPtr)
	{
		int16 mytype;
		propPtr->FindInt16(kValue_id, mytype);
		type = mytype;
	}
	return (propPtr != NULL);
}

int32 TTranslatorStyle::CountTabs(void) const
{
	int32 count = 0;
	for (long x = 0; x < CountProperties(); x++)
	{
		const TTransProperty* propPtr = GetProperty(x);
		if (propPtr && propPtr->Kind() == kPropParTab)
			count++;
	}
	return count;
}

bool TTranslatorStyle::GetTab(int32 index, float& pos, int32& type, int32& fillType, uchar* alignChar, uchar* fillChar) const
{
	int32 count = 0;
	const TTransProperty* propPtr = NULL;
	for (long x = 0; x < CountProperties(); x++)
	{
		propPtr = GetProperty(x);
		if (propPtr && propPtr->Kind() == kPropParTab)
		{
			if (count == index)
				break;
			count++;
		}
	}
	
	if (propPtr)
	{
		propPtr->FindFloat(0, pos);
		propPtr->FindInt32(1, type);
		propPtr->FindInt32(2, fillType);
		if (alignChar)
		{
			int32 utf8Char;
			propPtr->FindInt32(3, utf8Char);
			*alignChar = (char) utf8Char;	// BOGUS UTF8
		}
		if (fillChar)
		{
			int32 utf8Char;
			propPtr->FindInt32(4, utf8Char);
			*fillChar = (char) utf8Char;	// BOGUS UTF8
		}
	}
	return (propPtr != NULL);
}

bool TTranslatorStyle::GetBorderColor(int32 border, rgb_color& high, rgb_color& low, pattern& pat)
{
	high = OUR_BLACK;
	low = OUR_WHITE;
	pat = B_SOLID_HIGH;

	for (long x = 0; x < CountProperties(); x++)
	{
		const TTransProperty* propPtr = GetProperty(x);
		if (propPtr && propPtr->Kind() == kPropBorderColor)
		{
			int32 propBorder;
			if (propPtr->FindInt32( kIndex_id, propBorder ) && propBorder == border)
			{
				GetColorFromProp(propPtr, high, low, pat);
				return true;
			}
		}
	}
	return false;
}

bool TTranslatorStyle::GetBorderOpacity(int32 border, float& value) const
{
	for (long x = 0; x < CountProperties(); x++)
	{
		const TTransProperty* propPtr = GetProperty(x);
		if (propPtr && propPtr->Kind() == kPropBorderOpacity)
		{
			int32 propBorder;
			if (propPtr->FindInt32( kIndex_id, propBorder ) && propBorder == border)
				return propPtr->FindFloat( kValue_id, value );
		}
	}
	return false;
}

bool TTranslatorStyle::GetBorderSize(int32 border, float& size)
{
	// BOGUS
	return false;
}
bool TTranslatorStyle::GetCellWrap(bool& wrap)
{
	wrap = false;
	const TTransProperty* propPtr = FindProperty( kPropCellWrap );
	if (propPtr)
		propPtr->FindBool( kValue_id, wrap );
	return (propPtr != NULL);
}

bool TTranslatorStyle::GetCellLock(bool& lock)
{
	lock = false;
	const TTransProperty* propPtr = FindProperty( kPropCellLock );
	if (propPtr)
		propPtr->FindBool( kValue_id, lock );
	return (propPtr != NULL);
}

#pragma mark -

void TTranslatorStyle::AddColor( block_id id, rgb_color high, rgb_color low, pattern pat )
{
	// Create ink property with ink chunk as its value.
	TTransProperty* propPtr = new TTransProperty(id);
	if (propPtr)
	{
		AddColorToProp(propPtr, high, low, pat);
		AddProperty( propPtr );
	}
}

void TTranslatorStyle::AddColorToProp( TTransProperty* propPtr, rgb_color high, rgb_color low, pattern pat )
{
	// Add ink chunk as value property for propPtr.
	if (propPtr)
	{
		// Create ink block (ha ha).	
		TBlockDataTable* inkPtr = new TBlockDataTable();
		if (!inkPtr)
			return;
			
		rgb_color temp = OUR_BLACK;
		if (memcmp(&high, &temp, sizeof(rgb_color)))
			inkPtr->AddBytes( kHighColor_id, &high, sizeof(rgb_color) );
	
		pattern chkPat = B_SOLID_HIGH;
		if (memcmp(&chkPat, &pat, sizeof(pattern)))
		{
			inkPtr->AddBytes( kPattern_id, &pat, sizeof(pattern) );
			temp = OUR_WHITE;
			if (memcmp(&low, &temp, sizeof(rgb_color)))
				inkPtr->AddBytes( kLowColor_id, &low, sizeof(rgb_color) );
		}

		propPtr->AddChunk( kValue_id, inkPtr );	
	}
}

bool TTranslatorStyle::GetFloatPropertyValue(int32 propType, float& value) const
{
	const TTransProperty* propPtr = FindProperty( propType );
	if (propPtr)
		propPtr->FindFloat( kValue_id, value );
	return (propPtr != NULL);
}

bool TTranslatorStyle::GetColor( block_id id, rgb_color& high, rgb_color& low, pattern& pat ) const
{
	const TTransProperty* propPtr = FindProperty( id );
	return GetColorFromProp( propPtr, high, low, pat );
}

bool TTranslatorStyle::GetColorFromProp(const TTransProperty* propPtr, rgb_color& high, rgb_color& low, pattern& pat) const
{
	high = OUR_BLACK;
	low = OUR_BLACK;
	pat = B_SOLID_HIGH;
	
	if (propPtr)
	{
		// Look for value chunk to get actual color.
		const TBlockDataTable* colorPtr = propPtr->FindChunk( kValue_id );
		if (colorPtr)
		{
			const void* ptr;
			ssize_t	size;
			
			if (colorPtr->FindBytes( kHighColor_id, &ptr, &size ))
				memcpy( &high, ptr, sizeof(rgb_color) );
			if (colorPtr->FindBytes( kLowColor_id, &ptr, &size ))
				memcpy( &low, ptr, sizeof(rgb_color) );
			if (colorPtr->FindBytes( kPattern_id, &ptr, &size ))
				memcpy( &pat, ptr, sizeof(pattern) );

			// If solid pattern make sure low color is same as high color.
			if (!memcmp(&pat, &B_SOLID_HIGH, sizeof(pattern)))
				memcpy(&low, &high, sizeof(rgb_color));
			return true;
		}
	}
	return false;
}

void TTranslatorStyle::AddParagraphSpacing( block_id id, float value, bool lines )
{
	TTransProperty* propPtr = new TTransProperty(id);
	propPtr->AddFloat( 0, value );
	propPtr->AddBool( 1, lines );
	AddProperty( propPtr );
}

bool TTranslatorStyle::GetParagraphSpacing( block_id id, float& value, bool& lines ) const
{
	value = 1.0;
	lines = true;
	const TTransProperty* propPtr = FindProperty( id );
	if (propPtr)
	{
		propPtr->FindFloat( 0, value );
		propPtr->FindBool( 1, lines );
	}
	return (propPtr != NULL);
}

long TTranslatorStyle::ReverseFlattenUTF8(const uchar* textPtr)
{
	long result = 0;
	long charBytes = (((0xE5000000 >> ((*textPtr >> 3) & 0x1E)) & 3) + 1);
	for (long i = 0; i < charBytes; i++)
		result |= textPtr[i] << (i * 8);
	return (result);
}

TBlockDataTable& TTranslatorStyle::StyleDataTable(void)
{
	return mStyleChunk;
}


#pragma mark -
TTranslatorStylesTable::TTranslatorStylesTable()
{
	mWritten = false;
}

TTranslatorStylesTable::~TTranslatorStylesTable()
{
	IFDEBUG(fprintf(stderr, "Styles: %d/%d \n", StylesCreated, StylesDeleted));
	
	while (mStyles.CountItems())
	{
		TTranslatorStyle* stylePtr = static_cast<TTranslatorStyle*>(mStyles.RemoveItem(0L));
		ASSERTC(stylePtr->StyleSheet() == this);
		ASSERTC(stylePtr->Index() != 0xFFFF);
		if (stylePtr->StyleSheet() == this)
			delete stylePtr;
	}

	IFDEBUG(fprintf(stderr, "After cleanup Styles: %d/%d \n", StylesCreated, StylesDeleted));
}

status_t TTranslatorStylesTable::Read(TBlockStreamReader* reader)
{
	status_t err = reader->Error();
	if (!err && (reader->BlockKind() != START_CHUNK_TYPE || reader->BlockID() != kStyleSheet_id))
		err = INVALID_STYLESHEET_ERROR;
			
	block_id id;
	while (!err && reader->NextBlock(&id))
	{
		switch (id)
		{
			case kPropsArray_id:
				err = mPropMgr.Read(reader);
				break;
			case kStylesArray_id:
				err = ReadStyles(reader);
				break;
			default:
				reader->SkipBlock();
				break;
		}
	}		
	return err ? err : reader->Error();
}

status_t TTranslatorStylesTable::ReadStyles(TBlockStreamReader* reader)
{
	status_t 	err = reader->Error();
	if (!err && reader->BlockKind() != START_CHUNK_TYPE)
		err = INVALID_STYLE_ERROR;

	block_id id;
	while (!err && reader->NextBlock(&id))
	{
		switch (id)
		{
			case kStyle_id:
			{
				TTranslatorStyle* stylePtr = new TTranslatorStyle();
				AddStyle( stylePtr );
				err = stylePtr->Read(reader);
				break;
			}
			default:
				reader->SkipBlock();
				break;
		}
	}
	return err ? err : reader->Error();
}

status_t TTranslatorStylesTable::Write(TBlockStreamWriter* writer)
{
	status_t err = writer->Error();
	if (err)
		return err;

	mWritten = true;
	writer->BeginChunk( kStyleSheet_id );
	err = mPropMgr.Write(writer);
	if (!err)
	{
		writer->BeginChunk( kStylesArray_id );
		writer->WriteInt32( kArrayCount_id, mStyles.CountItems() );
		for (long x = 0; !err && x < mStyles.CountItems(); x++)
			err = (*this)[x]->Write(writer);
		writer->EndChunk( kStylesArray_id );
		writer->EndChunk( kStyleSheet_id );
	}
	return err ? err : writer->Error();
}

int32 TTranslatorStylesTable::AddProperty( TTransProperty* propPtr )
{
	if (mWritten)
	{
		delete propPtr;
		return 0xFFFF;
	}
	return mPropMgr.AddProperty( propPtr );
}

int32 TTranslatorStylesTable::AddStyle( TTranslatorStyle* stylePtr )
{
	int32 styleIndex = 0xFFFF;	
	if (!stylePtr)
		return styleIndex;

	// If style already added to style sheet then return its index.	
	if (stylePtr->StyleSheet())
	{
		ASSERTC(stylePtr->StyleSheet() == this);
		return stylePtr->Index();
	}

	// If style sheet table not already written to disk add the style.
	if (!mWritten)
	{
		styleIndex = mStyles.CountItems();
		mStyles.AddItem( stylePtr );
	}
	stylePtr->SetStyleSheet( this, styleIndex );
	return styleIndex;
}

TTranslatorStyle* TTranslatorStylesTable::operator[](int32 index) const
{
	ASSERTC(index >= 0 && index < mStyles.CountItems());
	if (index < 0 || index >= mStyles.CountItems())
		return NULL;
	return (TTranslatorStyle*) mStyles.ItemAt(index);
}

#pragma mark -
TTransProperty::TTransProperty(int32 kind)
{
	mKind = kind;
	IFDEBUG(PropsCreated++);
	ASSERTC(PropsCreated);
}

TTransProperty::TTransProperty(const TTransProperty& src)
	: TBlockDataTable(src)
{
	mKind = src.mKind;
	IFDEBUG(PropsCreated++);
	ASSERTC(PropsCreated);
}

TTransProperty::~TTransProperty()
{
	IFDEBUG(PropsDeleted++);
	ASSERTC(PropsDeleted);
}

int32 TTransProperty::Kind(void) const
{
	return mKind;
}

bool TTransProperty::operator==(const TTransProperty& prop) const
{
	if (mKind != prop.mKind)
		return false;
	return TBlockDataTable::operator==(prop);
}

#pragma mark -
TTransPropertyMgr::TTransPropertyMgr()
{
}

TTransPropertyMgr::~TTransPropertyMgr()
{
	IFDEBUG(fprintf(stderr, "Properties: %d/%d \n", PropsCreated, PropsDeleted));
	IFDEBUG(fprintf(stderr, "~TTransPropertyMgr - items:%d \n", mProps.CountItems()));
	while (mProps.CountItems())
		delete static_cast<TTransProperty*>(mProps.RemoveItem(0L));

	IFDEBUG(fprintf(stderr, "After cleanup Properties: %d/%d \n", PropsCreated, PropsDeleted));
}

status_t TTransPropertyMgr::Read( TBlockStreamReader* reader )
{
	// should be on start chunk.
	if (reader->BlockKind() != START_CHUNK_TYPE || reader->BlockID() != kPropsArray_id)
		return B_ERROR;

	status_t 	err;
	int32 		kind;
	while (reader->NextBlock(&kind))
	{
		if (reader->BlockKind() == START_CHUNK_TYPE)
		{
			TTransProperty* propPtr = new TTransProperty(kind);
			mProps.AddItem(propPtr);
			err = propPtr->Read(reader);
		}
	}
	return reader->Error();
}

status_t TTransPropertyMgr::Write( TBlockStreamWriter* writer ) const
{
	status_t err = writer->Error();
	if (err)
		return err;
		
	writer->BeginChunk( kPropsArray_id );
	writer->WriteInt32( kArrayCount_id, mProps.CountItems() );
	for (long x = 0; !err && x < mProps.CountItems(); x++)
	{
		TTransProperty* propPtr = static_cast<TTransProperty*>(mProps.ItemAt(x));
		int32 kind = propPtr->Kind();
		writer->BeginChunk( kind );
		err = propPtr->Write(writer);
		writer->EndChunk( kind );
	}
	writer->EndChunk( kPropsArray_id );
	return err ? err : writer->Error();
}

int32 TTransPropertyMgr::AddProperty( TTransProperty* prop )
{
	for (long x = 0; x < mProps.CountItems(); x++)
	{
		TTransProperty* propPtr = static_cast<TTransProperty*>(mProps.ItemAt(x));
		if (*propPtr == *prop)
		{
			delete prop;
			return x;
		}
	}
	mProps.AddItem( prop );
	return mProps.CountItems() - 1;
}

const TTransProperty* TTransPropertyMgr::FindProperty(int32 kind) const
{
	for (long x = 0; x < mProps.CountItems(); x++)
	{
		const TTransProperty* propPtr = reinterpret_cast<const TTransProperty*>(mProps.ItemAt(x));
		if (propPtr->Kind() == kind)
			return propPtr;
	}
	return NULL;
}

int32 TTransPropertyMgr::CountProperties(int32 kind) const
{
	int32 count = 0;
	for (long x = 0; x < mProps.CountItems(); x++)
	{
		const TTransProperty* propPtr = reinterpret_cast<const TTransProperty*>(mProps.ItemAt(x));
		if (propPtr->Kind() == kind)
			count++;
	}
	return count;
}

const TTransProperty* TTransPropertyMgr::operator[](int32 index) const
{
	if (index < 0 || index > mProps.CountItems())
		return NULL;
	return reinterpret_cast<const TTransProperty*>(mProps.ItemAt(index));
}

// $Header: /usr/local/cvsroot/8ball/Datatypes/TranslatorStyle.cpp,v 1.37 2000/02/17 00:22:09 tom Exp $

