#ifndef _POP_HASH_H_
#define _POP_HASH_H_

#include <SupportDefs.h>
#include <String.h>
#include <stdio.h>

class PopHash {
	public:
						PopHash();
						~PopHash();

		bool			IsValid() const {return (fUsed != 0);};
		status_t		ResizeTable(int32 size);
		
		BString *		Insert(const char *uid, int32 listNum);
		int32			Lookup(const char *uid);
	
		void			PrintToStream(FILE *stream);
			
	private:
		int32			find_slot(const char *uid, bool empty);
		
		uint32		fTableSize;
		typedef struct id_map {
						id_map() : listnum(-1) {};
			BString	uid;
			int32	listnum;
		};
		id_map *	fTable;
		uint32		fUsed;
};

#endif
