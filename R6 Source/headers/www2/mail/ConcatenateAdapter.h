/*
	ConcatenateAdapter.h
*/
#ifndef _CONCATENATE_ADAPTER_H
	#define _CONCATENATE_ADAPTER_H
	#include <DataIO.h>
	#include <List.h>

class ConcatenateAdapter : public BDataIO {
	public:
								ConcatenateAdapter(bool owning = true);
		virtual 				~ConcatenateAdapter();

		virtual	ssize_t 		Read(void*, size_t);
		virtual	ssize_t 		Write(const void*, size_t);

		void 					AddStream(BDataIO*);

	private:

		bool fOwning;
		int fIndex;
		BList fSources;
};

#endif
