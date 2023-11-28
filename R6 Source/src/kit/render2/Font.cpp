
#include <render2/Font.h>

#warning BFont: need to define faces
#warning BFont: need to define flags
#warning BFont: need to define spacing

using namespace B::Render2;

font_height font_height::undefined(-1048576.0, -1048576.0, -1048576.0);
int32 BFont::invalid_token = 0x0000ffff;

BFont::BFont(const IFontEngine:: ptr &engine) :
	m_engine(engine),
	m_token(invalid_token),
	m_spacing(0),
	_m_pad(0),
	m_size(1.0),
	m_flags(0),
	m_cachedHeight(font_height::undefined)
{
	memset(m_transform, 0, 4 * sizeof(coord));

}


BFont::BFont(const BFont &font) :
	m_engine(font.m_engine),
	m_token(font.m_token),
	m_spacing(font.m_spacing),
	_m_pad(font._m_pad),
	m_size(font.m_size),
	m_flags(font.m_flags),
	m_cachedHeight(font.m_cachedHeight)
{
	memcpy(m_transform, font.m_transform, 4 * sizeof(coord));
}


BFont::~BFont()
{
	// we might want to do something here!
#warning implement BFont::~BFont() ?
}

status_t 
BFont::SetFamilyAndStyle(const char *family, const char *style)
{
	return m_engine->SetFamilyAndStyle(this, family, style);
}

status_t 
BFont::SetFamilyAndFace(const char *family, uint16 face)
{
	return m_engine->SetFamilyAndFace(this, family, face);
}

status_t 
BFont::SetStyle(const char *style)
{
	return SetFamilyAndStyle(NULL, style);
}

status_t 
BFont::SetFace(uint16 face)
{
	return SetFamilyAndFace(NULL, face);
}

status_t 
BFont::Status() const
{
#warning implement BFont::Status()
	return B_ERROR;
}

void 
BFont::SetSize(float size)
{
	m_size = size;	
}

void 
BFont::SetTransform(const B2dTransform &transform)
{
	const coord *matrix = transform.Matrix();
	memcpy(m_transform, matrix, 4 * sizeof(coord));
}

void 
BFont::SetSpacing(uint8 spacing)
{
	m_spacing = spacing;
}

void 
BFont::SetFlags(uint32 flags)
{
	m_flags = flags;
}

void 
BFont::GetFamilyAndStyle(BString *outFamily, BString *outStyle) const
{
	m_engine->GetFamilyAndStyle(*this, outFamily, outStyle);
}

float 
BFont::Size() const
{
	return m_size;
}

B2dTransform 
BFont::Transform() const
{
	return B2dTransform(m_transform[0], m_transform[1], m_transform[2], m_transform[3], 0, 0);
}

uint8 
BFont::Spacing() const
{
	return m_spacing;
}

uint16 
BFont::Face() const
{
	return (uint16) ((m_token >> 16) & 0xffff);
}

uint32 
BFont::Flags() const
{
	return m_flags;
}

status_t 
BFont::GetHeight(font_height *outHeight)
{
	if (!outHeight)
		return B_BAD_VALUE;
		
	if (m_cachedHeight != font_height::undefined) {
		*outHeight = m_cachedHeight;
		return B_OK;
	}
	
	status_t status = m_engine->GetHeight(*this, &m_cachedHeight);
	if (status == B_OK) {
		*outHeight = m_cachedHeight;
	}
	else
		m_cachedHeight = font_height::undefined;
	return status;
}

status_t 
BFont::GetGlyphShapes(const char *charArray, size_t numChars, const IRender:: ptr &dest) const
{
	return m_engine->GetGlyphShapes(*this, charArray, numChars, dest);
}

status_t 
BFont::GetGlyphFlags(const char *charArray, size_t numChars, uint8 *outFlags) const
{
	return m_engine->GetGlyphFlags(*this, charArray, numChars, outFlags);
}

BValue 
BFont::AsValue()
{
#warning implement BFont::AsValue()
}

void 
BFont::SetEngine(const IFontEngine:: ptr &engine)
{
	m_engine = engine;
	// invalidate token
	m_token = invalid_token;
}

void 
BFont::SetToken(int32 token)
{
	m_token = token;	
}

IFontEngine::ptr 
BFont::Engine() const
{
	return m_engine;
}

int32 
BFont::Token() const
{
	return m_token;
}

BValue 
BFont::AsParcel(uint8 type)
{
#warning implement BFont::AsParcel()
}

