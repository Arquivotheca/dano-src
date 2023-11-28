#ifndef _MEDIA2_CONVERTFORMAT_H_
#define _MEDIA2_CONVERTFORMAT_H_

#include <support2/IValueStream.h>

#include <media2/MediaDefs.h>
#include <media2/MediaConstraint.h>
#include <media2/MediaFormat.h>
#include <media2/MediaPreference.h>

namespace B {
namespace Media2 {

IValueOutput::ptr export_format_values(
	const media_format & format,
	IValueOutput::arg stream);

status_t import_format_value(
	const BValue & value,
	media_format & format);

} } // B::Media2
#endif // _MEDIA2_CONVERTFORMAT_H_
