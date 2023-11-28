
#ifndef	_APPSERVER2_APPSERVERBITMAP_H_
#define	_APPSERVER2_APPSERVERBITMAP_H_

#include <appserver2_p/AppServer.h>
#include <appserver2_p/AppServerLegacy.h>
#include <raster2/Bitmap.h>
#include <kernel/OS.h>

namespace B {
namespace AppServer2 {

/*----------------------------------------------------------------*/
/*----- BBitmap class --------------------------------------------*/

class BAppServerBitmap : public BBitmap {

	public:

										BAppServerBitmap(const atom_ptr<BAppServer> &server, int32 width, int32 height, pixel_format format);
		virtual							~BAppServerBitmap();

				int32					ServerToken() const;

		virtual	status_t				PutPixels(
											int32 left, int32 top,
											int32 width, int32 height,
											void *data,  pixel_format format);

		virtual	status_t				GetPixels(
											int32 left, int32 top,
											int32 width, int32 height,
											void *data,  pixel_format format);

	protected:

				atom_ptr<BAppServer>	m_server;
				int32					m_token;
				pixel_format			m_format;
				color_space				m_oldFormat;
				void *					m_serverPixelData;
				area_id					m_serverPixelDataArea;
				area_id					m_serverPixelDataClonedArea;
				int32					m_serverPixelDataSize;
				int32					m_serverBytesPerRow;
				status_t				m_initError;
				uint32					m_flags;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::AppServer2

using namespace B::AppServer2;

#endif /* _APPSERVER2_APPSERVERBITMAP_H_ */
