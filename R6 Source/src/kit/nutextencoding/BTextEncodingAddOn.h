
#ifndef _B_TEXT_ENCODING_ADDON_H_
#define _B_TEXT_ENCODING_ADDON_H_

#include <AddOnManager.h>
#include <add-ons/textencoding/BTextCodec.h>
namespace B {

namespace TextEncoding {


class BTextEncodingAddOn : public BAddOnHandle {
	public:
										BTextEncodingAddOn(const entry_ref *entry, const node_ref *node);
										~BTextEncodingAddOn();
									
		BTextCodec *					MakeCodec(const char *encoding);
		bool							HasCodec(const char *encoding);

	protected:
		void							ImageUnloading(image_id image);
		const char *					AttrBaseName() const;
	private:
		void							LoadFunc();
		make_codec_t					fMakeCodec;
};

class BTextEncodingManager : public BAddOnManager {
	public:
										BTextEncodingManager();
										~BTextEncodingManager();
									
		static BTextEncodingManager &	Default();
		BTextCodec *					MakeCodec(const char *encoding);
//		void							CacheCodecFor(BTextCodec *codec, bigtime_t duration);

	protected:
		virtual	BAddOnHandle* 			InstantiateHandle(const entry_ref* entry, const node_ref* node);
};


} // end namespace TextEncoding

} // end namespace B

#endif

