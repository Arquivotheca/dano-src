
#include <support2/TokenSpace.h>
#include <appserver2_p/AppServerLegacy.h>
#include <appserver2_p/AppServerBitmap.h>
#include <appserver2_p/AppServerCommandProtocol.h>
#include <malloc.h>

struct format_conversion {
	color_space		oldFormat;
	pixel_format	newFormat;
};

static color_space mapNew2Old(pixel_format format)
{
	if ((format.components == GL_RED) ||
		(format.components == GL_BLUE) ||
		(format.components == GL_GREEN) ||
		(format.components == GL_RGB) ||
		(format.components == GL_RGBA) ||
		(format.components == GL_LUMINANCE_ALPHA) ||
		(format.components == GL_BGR) ||
		(format.components == GL_BGRA)) {
		if ((format.layout >= GL_UNSIGNED_INT_8_8_8_8) ||
			(format.layout < GL_UNSIGNED_BYTE_3_3_2)) return B_RGBA32;
		if ((format.components == GL_RGBA) ||
			(format.components == GL_BGRA) ||
			(format.components == GL_LUMINANCE_ALPHA)) return B_RGBA15;
		return B_RGB16;
	}
}

BAppServerBitmap::BAppServerBitmap(const atom_ptr<BAppServer> &server, int32 width, int32 height, pixel_format format)
	: BBitmap(width,height)
{
	BRect bound(0,0,width-1,height-1);
	uint8 dataType;
	uint32 data;

	m_server = server;
	m_format = format;
	m_oldFormat = mapNew2Old(format);
	m_serverPixelData = NULL;
	m_serverPixelDataArea = B_BAD_VALUE;
	m_serverPixelDataClonedArea = B_BAD_VALUE;
	m_initError = B_OK;
	
	BAppServerControlLink link(m_server);
	link.control->write32(GR_NEW_BITMAP);
	link.control->writeRect(bound);
	link.control->write32(m_oldFormat);
	link.control->write32(0);
	link.control->write32(-1);

	link.control->flush();

	m_token = link.control->read32();
	link.control->read(&dataType,1);
	link.control->read(&data,4);
	m_serverBytesPerRow = link.control->read32();

	if (dataType == 2) {
		m_serverPixelData = (void*)data;
		m_initError = m_serverPixelDataArea?B_OK:B_NO_MEMORY;
	} else if (dataType == 1) {
		m_serverPixelDataArea = data;
		m_initError = (m_serverPixelDataArea>0)?B_OK:B_NO_MEMORY;
	} else {
		m_serverPixelData = m_server->RWOffs2Ptr(data);
		m_initError = (m_serverPixelData!=NULL)?B_OK:B_NO_MEMORY;
	};
}

BAppServerBitmap::~BAppServerBitmap()
{
	if (m_serverPixelDataClonedArea != B_BAD_VALUE)
		delete_area(m_serverPixelDataClonedArea);
	else if (m_serverPixelData)
		free(m_serverPixelData);

	BAppServerControlLink link(m_server);
	link.control->write32(GR_DELETE_BITMAP);
	link.control->write32(m_token);
	link.control->flush();
	m_token = NO_TOKEN;
}

int32 
BAppServerBitmap::ServerToken() const
{
	return m_token;
}

status_t 
BAppServerBitmap::PutPixels(int32 left, int32 top, int32 width, int32 height, void *data, pixel_format format)
{
	return B_UNSUPPORTED;
}

status_t 
BAppServerBitmap::GetPixels(int32 left, int32 top, int32 width, int32 height, void *data, pixel_format format)
{
	return B_UNSUPPORTED;
}

