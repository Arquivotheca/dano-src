#ifndef _MEDIA2_RESOLVEFORMAT_H_
#define _MEDIA2_RESOLVEFORMAT_H_

#include <media2/MediaConstraint.h>
#include <media2/MediaFormat.h>
#include <media2/MediaPreference.h>

namespace B {
namespace Media2 {

// given a constraint and any number of preference sets, attempt to
// resolve a completely-specified format.  this should return the
// highest-scoring format out of each possible format, of which
// there is one per alternative per preference.

status_t resolve_format(
	const BMediaConstraint & constraint,
	BMediaPreference const * const * prefs,
	size_t pref_count,
	BMediaFormat * outFormat);

} } // B::Media2
#endif // _MEDIA2_RESOLVEFORMAT_H_
